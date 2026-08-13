#include <QApplication>
#include <QAction>
#include <QAbstractButton>
#include <QDialog>
#include <QDebug>
#include <QDeadlineTimer>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QMouseEvent>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QProcess>
#include <QRegExp>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QScreen>
#include <QSet>
#include <QTabBar>
#include <QTableWidget>
#include <QTextStream>
#include <QTimer>
#include <QWindow>

#include <atomic>
#include <chrono>
#include <thread>

#include "MainWindow.h"
#include "CommandLineOptions.h"
#include "MISC/UiFont.h"
#include "MISC/PluginsManager/PluginLoadJournal.h"
#include "MISC/PluginsManager/PluginEnablementConfig.h"
#include "Parameters.h"
#include "WinControls/PluginsAdmin/PluginAdminDialog.h"
#include "WinControls/PluginsAdmin/PluginAdminModel.h"
#include "WinControls/Preference/preferenceDlg.h"
#include "ScintillaComponent/DocTabView.h"
#include "ScintillaComponent/FindReplaceDlg.h"
#include "ScintillaComponent/ScintillaEditView.h"
#ifdef Q_OS_WIN
#include "Win32PluginSystem/Win32PluginManager.h"
#include "Win32PluginSystem/Win32NativeDockHost.h"
#include <commctrl.h>
#endif

namespace {

bool saveWidget(QWidget& widget, const QString& path)
{
    widget.show();
    QApplication::processEvents();
    return widget.grab().save(path);
}

QString safeName(QString value)
{
    value.replace(QRegExp("[^A-Za-z0-9._-]+"), "-");
    return value.isEmpty() ? QStringLiteral("unnamed") : value;
}

#ifdef Q_OS_WIN
HWND findCurrentProcessDialog(const wchar_t* expectedTitle, bool* exactMatch)
{
    struct SearchContext {
        DWORD processId;
        const wchar_t* expectedTitle;
        HWND exact = nullptr;
        HWND fallback = nullptr;
    } context{GetCurrentProcessId(), expectedTitle};
    EnumWindows([](HWND window, LPARAM data) -> BOOL {
        SearchContext* search =
            reinterpret_cast<SearchContext*>(data);
        DWORD processId = 0;
        wchar_t className[64]{};
        wchar_t title[256]{};
        GetWindowThreadProcessId(window, &processId);
        GetClassNameW(window, className, 64);
        if (processId != search->processId
            || wcscmp(className, L"#32770") != 0) {
            return TRUE;
        }
        GetWindowTextW(window, title, 256);
        if (wcscmp(title, search->expectedTitle) == 0) {
            search->exact = window;
            return FALSE;
        }
        if (!search->fallback)
            search->fallback = window;
        return TRUE;
    }, reinterpret_cast<LPARAM>(&context));
    if (exactMatch)
        *exactMatch = context.exact != nullptr;
    return context.exact ? context.exact : context.fallback;
}

QString nativeDialogControlReport(HWND dialog)
{
    QStringList controls;
    EnumChildWindows(dialog, [](HWND child, LPARAM data) -> BOOL {
        QStringList* output = reinterpret_cast<QStringList*>(data);
        wchar_t className[128]{};
        wchar_t text[512]{};
        GetClassNameW(child, className, 128);
        GetWindowTextW(child, text, 512);
        const LRESULT check = SendMessageW(child, BM_GETCHECK, 0, 0);
        output->append(QStringLiteral(
            "id=%1 class=%2 check=%3 enabled=%4 visible=%5 text=%6")
            .arg(GetDlgCtrlID(child))
            .arg(QString::fromWCharArray(className))
            .arg(check)
            .arg(IsWindowEnabled(child) != FALSE)
            .arg(IsWindowVisible(child) != FALSE)
            .arg(QString::fromWCharArray(text)));
        return TRUE;
    }, reinterpret_cast<LPARAM>(&controls));
    return controls.join(QLatin1Char('\n'));
}

HWND findCurrentProcessWindow(const wchar_t* expectedTitle)
{
    struct SearchContext {
        DWORD processId;
        const wchar_t* expectedTitle;
        HWND result = nullptr;
    } context{GetCurrentProcessId(), expectedTitle};
    EnumWindows([](HWND window, LPARAM data) -> BOOL {
        SearchContext* search = reinterpret_cast<SearchContext*>(data);
        DWORD processId = 0;
        wchar_t title[256]{};
        GetWindowThreadProcessId(window, &processId);
        GetWindowTextW(window, title, 256);
        if (processId == search->processId
            && wcscmp(title, search->expectedTitle) == 0) {
            search->result = window;
            return FALSE;
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&context));
    return context.result;
}

HWND findDescendantWindow(HWND parent, const wchar_t* text,
                          const wchar_t* classSubstring = nullptr)
{
    struct SearchContext {
        const wchar_t* text;
        const wchar_t* classSubstring;
        HWND result = nullptr;
    } context{text, classSubstring};
    EnumChildWindows(parent, [](HWND window, LPARAM data) -> BOOL {
        SearchContext* search = reinterpret_cast<SearchContext*>(data);
        wchar_t windowText[256]{};
        wchar_t className[128]{};
        GetWindowTextW(window, windowText, 256);
        GetClassNameW(window, className, 128);
        const bool textMatches = !search->text
            || wcscmp(windowText, search->text) == 0;
        const bool classMatches = !search->classSubstring
            || wcsstr(className, search->classSubstring) != nullptr;
        if (textMatches && classMatches) {
            search->result = window;
            return FALSE;
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&context));
    return context.result;
}

int treeNodeCount(HWND tree)
{
    return tree ? static_cast<int>(
        SendMessageW(tree, TVM_GETCOUNT, 0, 0)) : 0;
}

HTREEITEM findDirectTreeChild(HWND tree, const wchar_t* expectedPrefix)
{
    if (!tree)
        return nullptr;
    HTREEITEM root = reinterpret_cast<HTREEITEM>(
        SendMessageW(tree, TVM_GETNEXTITEM, TVGN_ROOT, 0));
    SendMessageW(tree, TVM_EXPAND, TVE_EXPAND,
                 reinterpret_cast<LPARAM>(root));
    for (HTREEITEM item = reinterpret_cast<HTREEITEM>(
             SendMessageW(tree, TVM_GETNEXTITEM, TVGN_CHILD,
                          reinterpret_cast<LPARAM>(root)));
         item;
         item = reinterpret_cast<HTREEITEM>(
             SendMessageW(tree, TVM_GETNEXTITEM, TVGN_NEXT,
                          reinterpret_cast<LPARAM>(item)))) {
        wchar_t text[256]{};
        TVITEMW treeItem{};
        treeItem.mask = TVIF_TEXT;
        treeItem.hItem = item;
        treeItem.pszText = text;
        treeItem.cchTextMax = 256;
        SendMessageW(tree, TVM_GETITEMW, 0,
                     reinterpret_cast<LPARAM>(&treeItem));
        if (wcsncmp(text, expectedPrefix, wcslen(expectedPrefix)) == 0)
            return item;
    }
    return nullptr;
}

bool clickTreeItem(HWND tree, HTREEITEM item)
{
    if (!tree || !item)
        return false;
    SendMessageW(tree, TVM_ENSUREVISIBLE, 0,
                 reinterpret_cast<LPARAM>(item));
    RECT bounds{};
    *reinterpret_cast<HTREEITEM*>(&bounds) = item;
    if (!SendMessageW(tree, TVM_GETITEMRECT, TRUE,
                      reinterpret_cast<LPARAM>(&bounds))) {
        return false;
    }
    const LPARAM point = MAKELPARAM(
        (bounds.left + bounds.right) / 2,
        (bounds.top + bounds.bottom) / 2);
    std::atomic_bool completed{false};
    std::thread clickWorker([&] {
        DWORD_PTR ignored = 0;
        SendMessageTimeoutW(tree, WM_LBUTTONDOWN, MK_LBUTTON, point,
                            SMTO_ABORTIFHUNG, 1000, &ignored);
        SendMessageTimeoutW(tree, WM_LBUTTONUP, 0, point,
                            SMTO_ABORTIFHUNG, 1000, &ignored);
        completed = true;
    });
    for (int attempt = 0; attempt < 500 && !completed.load(); ++attempt) {
        QApplication::processEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    clickWorker.join();
    return completed.load();
}

int descendantWindowCount(HWND parent)
{
    int count = 0;
    EnumChildWindows(parent, [](HWND, LPARAM data) -> BOOL {
        ++*reinterpret_cast<int*>(data);
        return TRUE;
    }, reinterpret_cast<LPARAM>(&count));
    return count;
}

int nativeQtWidgetCount(QWidget* root)
{
    int count = root && root->internalWinId() ? 1 : 0;
    if (!root)
        return count;
    for (QWidget* widget : root->findChildren<QWidget*>()) {
        if (widget->internalWinId())
            ++count;
    }
    return count;
}

QSet<QWidget*> nativeQtWidgets(QWidget* root)
{
    QSet<QWidget*> result;
    if (root && root->internalWinId())
        result.insert(root);
    if (root) {
        for (QWidget* widget : root->findChildren<QWidget*>()) {
            if (widget->internalWinId())
                result.insert(widget);
        }
    }
    return result;
}
#endif

} // namespace

int main(int argc, char* argv[])
{
#ifdef Q_OS_WIN
    SetErrorMode(SetErrorMode(0) | SEM_NOGPFAULTERRORBOX);
#endif
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Notepad++"));
    app.setApplicationVersion(QStringLiteral("8.4.6"));
    app.setOrganizationName(QStringLiteral("Notepad++"));
    app.setOrganizationDomain(QStringLiteral("notepad-plus-plus.org"));
    app.setFont(notepadPlusPlusUiFont());

    if (argc < 2 || argc > 4) {
        qCritical("Usage: ui-parity-capture <output-directory> "
                  "[light|dark|noPlugin|pluginRecovery|registrationRollback|"
                  "betterMultiSelection|pluginEnablement|p0Plugins|"
                  "pluginCoexistence] "
                  "[configurationPlugins] "
                  "[zh_CN]");
        return 2;
    }

    const QString output = QDir::cleanPath(QString::fromLocal8Bit(argv[1]));
    if (!QDir().mkpath(output))
        return 3;

    const bool noPluginMode = argc >= 3
        && QString::fromLocal8Bit(argv[2]).compare(
               QStringLiteral("noPlugin"), Qt::CaseInsensitive) == 0;
    const bool registrationRollbackMode = argc >= 3
        && QString::fromLocal8Bit(argv[2]).compare(
               QStringLiteral("registrationRollback"),
               Qt::CaseInsensitive) == 0;
    const bool pluginRecoveryMode = argc >= 3
        && QString::fromLocal8Bit(argv[2]).compare(
               QStringLiteral("pluginRecovery"),
               Qt::CaseInsensitive) == 0;
    const bool betterMultiSelectionMode = argc >= 3
        && QString::fromLocal8Bit(argv[2]).compare(
               QStringLiteral("betterMultiSelection"),
               Qt::CaseInsensitive) == 0;
    const bool pluginEnablementMode = argc >= 3
        && QString::fromLocal8Bit(argv[2]).compare(
               QStringLiteral("pluginEnablement"),
               Qt::CaseInsensitive) == 0;
    const bool p0PluginsMode = argc >= 3
        && QString::fromLocal8Bit(argv[2]).compare(
               QStringLiteral("p0Plugins"), Qt::CaseInsensitive) == 0;
    const bool configurationPluginsMode = argc >= 3
        && QString::fromLocal8Bit(argv[2]).compare(
               QStringLiteral("configurationPlugins"),
               Qt::CaseInsensitive) == 0;
    const bool sessionManagerMode = argc >= 3
        && QString::fromLocal8Bit(argv[2]).compare(
               QStringLiteral("sessionManager"),
               Qt::CaseInsensitive) == 0;
    const bool documentPolicyPluginsMode = argc >= 3
        && QString::fromLocal8Bit(argv[2]).compare(
               QStringLiteral("documentPolicyPlugins"),
               Qt::CaseInsensitive) == 0;
    const bool pluginCoexistenceMode = argc >= 3
        && QString::fromLocal8Bit(argv[2]).compare(
               QStringLiteral("pluginCoexistence"),
               Qt::CaseInsensitive) == 0;
    const int pluginCoexistencePhase = pluginCoexistenceMode
        ? qEnvironmentVariableIntValue("NPP_QT_TEST_PLUGIN_RESTART_PHASE")
        : 0;

    const bool forceChinese =
        argc == 4
        && QString::fromLocal8Bit(argv[3]).compare(
               QStringLiteral("zh_CN"), Qt::CaseInsensitive) == 0;
    NppParameters& parameters = NppParameters::getInstance();
    if (forceChinese || noPluginMode || registrationRollbackMode
        || pluginRecoveryMode || betterMultiSelectionMode
        || pluginEnablementMode || p0PluginsMode
        || configurationPluginsMode || sessionManagerMode
        || documentPolicyPluginsMode || pluginCoexistenceMode) {
        const QString settingsPath =
            output + QStringLiteral("/settings");
        if (pluginCoexistenceMode && pluginCoexistencePhase == 1)
            QDir(settingsPath).removeRecursively();
        if (!QDir().mkpath(settingsPath)
            || !parameters.setUserPathOverride(settingsPath)) {
            return 17;
        }
    }
    if (!parameters.load())
        return 18;
    if (pluginEnablementMode) {
        QFile::remove(
            PluginEnablementConfig::filePathForConfigDirectory(
                parameters.getUserPath()));
    }
    if (forceChinese)
        parameters.setNativeLang(QStringLiteral("zh_CN"));
#ifdef NPP_WIN32_PLUGIN_CORPUS_MANAGED_TEST
    if (!pluginEnablementMode) {
        PluginEnablementConfig pluginEnablement(
            PluginEnablementConfig::filePathForConfigDirectory(
                parameters.getUserPath()));
        const QStringList enabledPluginFolders = {
        QStringLiteral("mimeTools"),
        QStringLiteral("qkNppReverseLines"),
        QStringLiteral("Remove Duplicate Lines"),
        QStringLiteral("SelectQuotedText"),
        QStringLiteral("BracketsCheck"),
        QStringLiteral("SecurePad"),
        QStringLiteral("CodeAlignmentNpp"),
        QStringLiteral("BetterMultiSelection"),
        QStringLiteral("NPPJSONViewer"),
        QStringLiteral("JsonTools"),
        QStringLiteral("nppConverter"),
        QStringLiteral("NppPluginDemo"),
        QStringLiteral("GotoLineCol"),
        QStringLiteral("RandomValuesNppPlugin"),
        QStringLiteral("Merge files in one"),
        QStringLiteral("SelectToClipboard"),
        QStringLiteral("urlPlugin")
        };
        for (const QString& folder : enabledPluginFolders)
            pluginEnablement.setEnabled(folder, true);
        if (p0PluginsMode || pluginCoexistenceMode) {
            const QStringList p0PluginFolders = {
                QStringLiteral("DoxyIt"),
                QStringLiteral("ElasticTabstops"),
                QStringLiteral("SurroundSelection"),
                QStringLiteral("XMLTools")
            };
            for (const QString& folder : p0PluginFolders)
                pluginEnablement.setEnabled(folder, true);
        }
        if (configurationPluginsMode || pluginCoexistenceMode) {
            pluginEnablement.setEnabled(QStringLiteral("AutoSave"), true);
            pluginEnablement.setEnabled(
                QStringLiteral("NppEditorConfig"), true);
        }
        if (sessionManagerMode || pluginCoexistenceMode)
            pluginEnablement.setEnabled(QStringLiteral("SessionMgr"), true);
        if (documentPolicyPluginsMode || pluginCoexistenceMode) {
            pluginEnablement.setEnabled(
                QStringLiteral("nppAutoDetectIndent"), true);
            pluginEnablement.setEnabled(QStringLiteral("AutoCodepage"), true);
            pluginEnablement.setEnabled(QStringLiteral("AutoEolFormat"), true);
        }
        QString enablementError;
        if (!pluginEnablement.save(&enablementError))
            return 92;
    }
#endif
    if (p0PluginsMode || pluginCoexistenceMode) {
        const QString pluginConfigDir = QDir(parameters.getUserPath())
            .filePath(QStringLiteral("plugins/Config"));
        if (!QDir().mkpath(pluginConfigDir))
            return 109;
        QFile elasticConfig(QDir(pluginConfigDir).filePath(
            QStringLiteral("ElasticTabstops.ini")));
        if (!elasticConfig.open(QIODevice::WriteOnly | QIODevice::Text)
            || elasticConfig.write(
                "enabled true\nextensions *\npadding 1\n"
                "convert_leading_tabs_to_spaces false\n") <= 0) {
            return 109;
        }
        elasticConfig.close();
        QFile xmlToolsConfig(QDir(pluginConfigDir).filePath(
            QStringLiteral("XMLTools.ini")));
        if (!xmlToolsConfig.open(QIODevice::WriteOnly | QIODevice::Text)
            || xmlToolsConfig.write(
                "[XML Tools]\n"
                "doCheckXML=0\n"
                "doCloseTag=1\n"
                "doPreventXXE=1\n"
                "errorDisplayMode=Annotation\n"
                "xpathOnStatusbar=0\n") <= 0) {
            return 109;
        }
    }
    QString configurationPluginProjectDir;
    QString editorConfigSamplePath;
    QString autoSaveSamplePath;
    QString documentPolicyProjectDir;
    QString autoCodepageSamplePath;
    QString autoEolSamplePath;
    QString autoIndentSamplePath;
    if (configurationPluginsMode || pluginCoexistenceMode) {
        const QString pluginConfigDir = QDir(parameters.getUserPath())
            .filePath(QStringLiteral("plugins/Config"));
        if (!QDir().mkpath(pluginConfigDir))
            return 129;
        QFile autoSaveConfig(QDir(pluginConfigDir).filePath(
            QStringLiteral("AutoSave.ini")));
        const bool timerTest = qEnvironmentVariableIsSet(
            "NPP_QT_TEST_AUTOSAVE_TIMER");
        const QByteArray autoSaveSettings = QByteArray(
                "[Options]\r\nTimer=1\r\nSaveOnActivateApp=")
            + (timerTest ? "0" : "1")
            + "\r\nSaveOnTimer=" + (timerTest ? "1" : "0")
            + "\r\nSaveCurrentFileOnly=1\r\n"
              "NamedFilesMode=1\r\nUnNamedFilesMode=0\r\n"
              "UnNamedFilesSaveFolder=$CDIR$\\autorecover\r\n"
              "UnNamedFilesRecoverFolder=$CDIR$\\autorecover\r\n"
              "MaxFileSize=0\r\n";
        if (!autoSaveConfig.open(QIODevice::WriteOnly | QIODevice::Text)
            || autoSaveConfig.write(autoSaveSettings) <= 0) {
            return 129;
        }
        autoSaveConfig.close();

        configurationPluginProjectDir = QFileInfo(QDir(output).filePath(
            QStringLiteral("configuration-plugin-project")))
            .absoluteFilePath();
        if (!QDir().mkpath(configurationPluginProjectDir))
            return 129;
        QFile editorConfig(QDir(configurationPluginProjectDir).filePath(
            QStringLiteral(".editorconfig")));
        if (!editorConfig.open(QIODevice::WriteOnly | QIODevice::Text)
            || editorConfig.write(
                "root = true\n\n"
                "[*]\n"
                "indent_style = space\n"
                "indent_size = 3\n"
                "tab_width = 6\n"
                "end_of_line = lf\n"
                "trim_trailing_whitespace = true\n"
                "insert_final_newline = true\n") <= 0) {
            return 129;
        }
        editorConfig.close();
        editorConfigSamplePath = QDir(configurationPluginProjectDir).filePath(
            QStringLiteral("editor-config-sample.txt"));
        autoSaveSamplePath = QDir(configurationPluginProjectDir).filePath(
            QStringLiteral("auto-save-sample.txt"));
        for (const QString& path : {editorConfigSamplePath,
                                    autoSaveSamplePath}) {
            QFile sample(path);
            if (!sample.open(QIODevice::WriteOnly)
                || sample.write("initial\n") <= 0) {
                return 129;
            }
        }
    }
    if (documentPolicyPluginsMode || pluginCoexistenceMode) {
        const QString pluginConfigDir = QDir(parameters.getUserPath())
            .filePath(QStringLiteral("plugins/Config"));
        if (!QDir().mkpath(pluginConfigDir))
            return 137;
        const struct ConfigFile {
            const char* name;
            const char* contents;
        } configs[] = {
            {"AutoCodepage.ini",
             "[Header]\r\nVersion=1.0\r\n[Groups]\r\n"
             "Cyrillic=active\r\n[Cyrillic]\r\nCodepage=45021\r\n"
             "Language=-1\r\nExt1=acp\r\n"},
            {"AutoEolFormat.ini",
             "[Header]\r\nVersion=1.0\r\n[Groups]\r\n"
             "Unix=active\r\n[Unix]\r\nEolFormat=45002\r\n"
             "Ext1=eolpolicy\r\n"}
        };
        for (const ConfigFile& config : configs) {
            QFile file(QDir(pluginConfigDir).filePath(
                QString::fromLatin1(config.name)));
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text)
                || file.write(config.contents) <= 0) {
                return 137;
            }
        }
        documentPolicyProjectDir = QFileInfo(QDir(output).filePath(
            QStringLiteral("document-policy-project"))).absoluteFilePath();
        if (!QDir().mkpath(documentPolicyProjectDir))
            return 137;
        autoCodepageSamplePath = QDir(documentPolicyProjectDir).filePath(
            QStringLiteral("sample.acp"));
        autoEolSamplePath = QDir(documentPolicyProjectDir).filePath(
            QStringLiteral("sample.eolpolicy"));
        autoIndentSamplePath = QDir(documentPolicyProjectDir).filePath(
            QStringLiteral("sample.cpp"));
        const QList<QPair<QString, QByteArray>> samples = {
            {autoCodepageSamplePath, QByteArray("plain text\r\n")},
            {autoEolSamplePath, QByteArray("one\r\ntwo\r\n")},
            {autoIndentSamplePath, QByteArray("if (true) {\n\tvalue();\n}\n")}
        };
        for (const auto& sample : samples) {
            QFile file(sample.first);
            if (!file.open(QIODevice::WriteOnly)
                || file.write(sample.second) != sample.second.size()) {
                return 137;
            }
        }
    }
    const QString requestedTheme = argc == 3
        ? QString::fromLocal8Bit(argv[2]).toLower()
        : argc == 4 ? QString::fromLocal8Bit(argv[2]).toLower() : QString();
    if (requestedTheme == QStringLiteral("dark"))
        parameters.getNppGUI()._darkModeEnabled = true;
    else if (requestedTheme == QStringLiteral("light"))
        parameters.getNppGUI()._darkModeEnabled = false;
    parameters.reloadNativeLang();

#ifdef Q_OS_WIN
    if (pluginRecoveryMode) {
        PluginLoadJournal interruptedLoad(QDir(parameters.getUserPath())
            .filePath(QStringLiteral("plugin-load")));
        QString journalError;
        const QString mimeToolsPath = QDir(
            QCoreApplication::applicationDirPath()).filePath(
                QStringLiteral("plugins/mimeTools/mimeTools.dll"));
        if (!interruptedLoad.beginSession(&journalError)
            || !interruptedLoad.beginPlugin(
                QStringLiteral("mimeTools"), mimeToolsPath, &journalError)) {
            return 84;
        }
    }
#endif

    const CommandLineOptions startupOptions = noPluginMode
        ? CommandLineParser::parse(
              {QStringLiteral("notepadpp-qt"), QStringLiteral("-noPlugin")},
              QDir::currentPath())
        : CommandLineOptions();
    if (startupOptions.noPlugin != noPluginMode)
        return 81;
    MainWindow mainWindow(startupOptions);
    mainWindow.resize(1100, 760);
    if (!saveWidget(mainWindow, output + QStringLiteral("/main.png")))
        return 4;
    const QRect availableGeometry =
        mainWindow.windowHandle()->screen()->availableGeometry();
    if (!availableGeometry.contains(mainWindow.frameGeometry().topLeft()))
        return 79;
    DocTabView* mainTabs = mainWindow.findChild<DocTabView*>(
        QStringLiteral("MainDocTab"));
    DocTabView* subTabs = mainWindow.findChild<DocTabView*>(
        QStringLiteral("SubDocTab"));
    if (!mainTabs || !subTabs || !mainTabs->editor() || !subTabs->editor() ||
        mainTabs->editor() == subTabs->editor() ||
        mainTabs->findChildren<ScintillaEditView*>(
            QString(), Qt::FindDirectChildrenOnly).size() != 1 ||
        subTabs->findChildren<ScintillaEditView*>(
            QString(), Qt::FindDirectChildrenOnly).size() != 1) {
        return 32;
    }
    const bool restoredCoexistenceSession = pluginCoexistenceMode
        && pluginCoexistencePhase == 2;
    if (!restoredCoexistenceSession
        && mainWindow.windowTitle() != QStringLiteral("new 1 - Notepad++"))
        return 23;
    if (!restoredCoexistenceSession
        && (mainTabs->tabBar()->count() != 1
        || mainTabs->tabBar()->tabRect(0).width() >=
            mainTabs->tabBar()->width() / 2)) {
        return 42;
    }
    if (noPluginMode) {
        QAction* placeholder = mainWindow.findChild<QAction*>(
            QStringLiteral("noPluginsLoadedAction"));
        if (!placeholder || placeholder->isEnabled()
            || !mainWindow.findChildren<QMenu*>(
                    QRegularExpression(
                        QStringLiteral("win32PluginMenu_.*"))).isEmpty()
            || QFileInfo(QDir(parameters.getUserPath()).filePath(
                    QStringLiteral("plugin-load/plugin-load.jsonl"))).exists()) {
            return 80;
        }
#ifdef Q_OS_WIN
        if (mainWindow.win32PluginManager())
            return 80;
#endif
        return 0;
    }
#ifdef Q_OS_WIN
    Win32PluginManager* win32Plugins = mainWindow.win32PluginManager();
    if (!win32Plugins
        || win32Plugins->mainWindow() != &mainWindow
        || !win32Plugins->mainEditorWindow()
        || !win32Plugins->secondaryEditorWindow()
        || win32Plugins->mainEditorWindow() != mainTabs->editor()
        || win32Plugins->secondaryEditorWindow() != subTabs->editor()
        || !win32Plugins->mainWindowHandle()
        || !win32Plugins->mainEditorHandle()
        || !win32Plugins->secondaryEditorHandle()) {
        return 31;
    }
    if (!win32Plugins->mainWindowAdapter().isValid()
        || !win32Plugins->mainEditorAdapter().isValid()
        || !win32Plugins->subEditorAdapter().isValid()
        || win32Plugins->mainWindowAdapter().window() != &mainWindow
        || win32Plugins->mainEditorAdapter().editor() != mainTabs->editor()
        || win32Plugins->subEditorAdapter().editor() != subTabs->editor()
        || win32Plugins->mainWindowAdapter().handle()
            != win32Plugins->mainWindowHandle()
        || win32Plugins->mainEditorAdapter().handle()
            != win32Plugins->mainEditorHandle()
        || win32Plugins->subEditorAdapter().handle()
            != win32Plugins->secondaryEditorHandle()) {
        return 37;
    }
    auto directApiWorks = [](HWND handle, ScintillaEditView* editor) {
        const SciFnDirect directFunction = reinterpret_cast<SciFnDirect>(
            SendMessageW(handle, SCI_GETDIRECTFUNCTION, 0, 0));
        const sptr_t directPointer = static_cast<sptr_t>(
            SendMessageW(handle, SCI_GETDIRECTPOINTER, 0, 0));
        return directFunction && directPointer
            && directFunction(directPointer, SCI_GETLENGTH, 0, 0)
                == editor->SendScintillaNpp(SCI_GETLENGTH);
    };
    if (!directApiWorks(win32Plugins->mainEditorHandle(), mainTabs->editor())
        || !directApiWorks(win32Plugins->secondaryEditorHandle(),
                           subTabs->editor())) {
        return 38;
    }
    if (sessionManagerMode) {
        const QStringList names = win32Plugins->loadedPluginNames();
        const int index = names.indexOf(QStringLiteral("Session Manager"));
        if (names.size() != 1 || index < 0
            || win32Plugins->loadedPluginFunctionCount(index) != 8)
            return 138;
        const QString sessionPath = QDir(output).filePath(
            QStringLiteral("plugin-session.xml"));
        const std::wstring nativePath = QDir::toNativeSeparators(
            sessionPath).toStdWString();
        if (!SendMessageW(win32Plugins->mainWindowHandle(),
                          NppMessageSaveCurrentSession, 0,
                          reinterpret_cast<LPARAM>(nativePath.c_str())))
            return 138;
        QFile sessionFile(sessionPath);
        if (!sessionFile.open(QIODevice::ReadOnly)
            || !sessionFile.readAll().contains("<Session"))
            return 138;
        wchar_t nppDirectory[MAX_PATH]{};
        if (!SendMessageW(win32Plugins->mainWindowHandle(),
                          NppMessageGetNppDirectory, MAX_PATH,
                          reinterpret_cast<LPARAM>(nppDirectory))
            || QString::fromWCharArray(nppDirectory).isEmpty())
            return 138;
        const quintptr bufferId = mainWindow.currentBufferIdForPlugin();
        if (SendMessageW(win32Plugins->mainWindowHandle(),
                         NppMessageGetPosFromBufferId,
                         static_cast<WPARAM>(bufferId), 0) < 0)
            return 138;
        return 0;
    }
    if (documentPolicyPluginsMode) {
        const QStringList names = win32Plugins->loadedPluginNames();
        const QStringList expected = {
            QStringLiteral("AutoCodepage"),
            QStringLiteral("AutoEolFormat"),
            QStringLiteral("Auto Detect Indention")
        };
        if (names.isEmpty())
            return 139;
        if (names.contains(expected.at(1))) {
            if (!mainWindow.openFileForPlugin(autoEolSamplePath))
                return 139;
            for (int i = 0; i < 5; ++i) {
                QApplication::processEvents();
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            if (mainTabs->editor()->eolMode() != EolUnix
                || mainTabs->editor()->text().contains(QStringLiteral("\r")))
                return 139;
        }
        if (names.contains(expected.at(0))) {
            if (!mainWindow.openFileForPlugin(autoCodepageSamplePath))
                return 139;
            for (int i = 0; i < 5; ++i) {
                QApplication::processEvents();
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            Buffer* codepageBuffer = mainTabs->currentBuffer();
            if (!codepageBuffer
                || codepageBuffer->getEncoding().compare(
                    QStringLiteral("windows-1251"),
                    Qt::CaseInsensitive) != 0)
                return 139;
        }
        if (names.contains(expected.at(2))) {
            if (!mainWindow.openFileForPlugin(autoIndentSamplePath))
                return 139;
            for (int i = 0; i < 5; ++i) {
                QApplication::processEvents();
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            if (!mainTabs->editor()->SendScintillaNpp(SCI_GETUSETABS))
                return 139;
        }
        return 0;
    }
    if (pluginEnablementMode) {
        const QString enablementPath =
            PluginEnablementConfig::filePathForConfigDirectory(
                parameters.getUserPath());
        if (win32Plugins->loadedPluginCount() != 0
            || QFileInfo(enablementPath).exists()) {
            return 93;
        }
        const QString pluginRoot = QDir(parameters.getNppPath())
            .filePath(QStringLiteral("plugins"));
        PluginAdminModel model(
            pluginRoot, PluginCatalog::embedded(),
            PluginVersion(QCoreApplication::applicationVersion()),
            enablementPath);
        PluginAdminDialog dialog(&model, &mainWindow);
        QTableWidget* installed = dialog.findChild<QTableWidget*>(
            QStringLiteral("installedPluginsTable"));
        if (!installed || installed->columnCount() != 3
            || installed->rowCount() == 0 || !installed->item(0, 1)
            || installed->item(0, 1)->checkState() != Qt::Unchecked) {
            return 94;
        }
        installed->item(0, 1)->setCheckState(Qt::Checked);
        QApplication::processEvents();
        PluginEnablementConfig saved(enablementPath);
        QString error;
        if (!saved.load(&error) || saved.enabledPlugins().size() != 1
            || win32Plugins->loadedPluginCount() != 0) {
            return 95;
        }
        return 0;
    }
#ifdef NPP_PLUGIN_REGISTRATION_TEST
    if (pluginRecoveryMode) {
        QApplication::processEvents();
        if (win32Plugins->recoveredPluginFolder().compare(
                QStringLiteral("mimeTools"), Qt::CaseInsensitive) != 0
            || win32Plugins->loadedPluginNames().contains(
                QStringLiteral("MIME Tools"), Qt::CaseInsensitive)
            || !mainWindow.findChild<QMessageBox*>(
                QStringLiteral("pluginLoadRecoveryNotice"))
            || QFileInfo(win32Plugins->loadMarkerPath()).exists()) {
            return 85;
        }
        return 0;
    }
    if (registrationRollbackMode) {
        int highestCommandId = 49999;
        for (int pluginIndex = 0;
             pluginIndex < win32Plugins->loadedPluginCount(); ++pluginIndex) {
            for (int functionIndex = 0;
                 functionIndex < win32Plugins->loadedPluginFunctionCount(
                     pluginIndex); ++functionIndex) {
                highestCommandId = qMax(
                    highestCommandId,
                    win32Plugins->loadedPluginFunctionCommandId(
                        pluginIndex, functionIndex));
            }
        }
        const int initialPluginCount = win32Plugins->loadedPluginCount();
        QString error;
        if (win32Plugins->loadPlugin(
                QString::fromUtf8(NPP_INVALID_REGISTRATION_PLUGIN), &error)
            || !error.contains(QStringLiteral("function array"),
                               Qt::CaseInsensitive)
            || win32Plugins->loadedPluginCount() != initialPluginCount
            || QFileInfo(win32Plugins->loadMarkerPath()).exists()) {
            return 82;
        }
        error.clear();
        if (!win32Plugins->loadPlugin(
                QString::fromUtf8(NPP_VALID_REGISTRATION_PLUGIN), &error)
            || !error.isEmpty()
            || win32Plugins->loadedPluginCount() != initialPluginCount + 1
            || win32Plugins->loadedPluginFunctionCommandId(
                   initialPluginCount, 0) != highestCommandId + 1
            || QFileInfo(win32Plugins->loadMarkerPath()).exists()) {
            return 83;
        }
        HMODULE probeModule = LoadLibraryW(
            reinterpret_cast<LPCWSTR>(
                QString::fromUtf8(NPP_VALID_REGISTRATION_PLUGIN).utf16()));
        using ResetFunction = void (*)();
        using CountFunction = LONG (*)(UINT);
        using ValueFunction = ULONG_PTR (*)(UINT);
        using PointerFunction = ULONG_PTR (*)();
        using SequenceCountFunction = LONG (*)();
        using SequenceCodeFunction = UINT (*)(int);
        const auto resetNotifications = probeModule
            ? reinterpret_cast<ResetFunction>(
                  GetProcAddress(probeModule, "resetNotifications"))
            : nullptr;
        const auto notificationCount = probeModule
            ? reinterpret_cast<CountFunction>(
                  GetProcAddress(probeModule, "getNotificationCount"))
            : nullptr;
        const auto lastNotificationId = probeModule
            ? reinterpret_cast<ValueFunction>(
                  GetProcAddress(probeModule, "getLastNotificationId"))
            : nullptr;
        const auto lastNotificationWindow = probeModule
            ? reinterpret_cast<ValueFunction>(
                  GetProcAddress(probeModule, "getLastNotificationWindow"))
            : nullptr;
        const auto notificationSequenceCount = probeModule
            ? reinterpret_cast<SequenceCountFunction>(GetProcAddress(
                  probeModule, "getNotificationSequenceCount"))
            : nullptr;
        const auto notificationSequenceCode = probeModule
            ? reinterpret_cast<SequenceCodeFunction>(GetProcAddress(
                  probeModule, "getNotificationSequenceCode"))
            : nullptr;
        const auto scintillaNotificationCount = probeModule
            ? reinterpret_cast<CountFunction>(GetProcAddress(
                  probeModule, "getScintillaNotificationCount"))
            : nullptr;
        const auto lastScintillaNotificationWindow = probeModule
            ? reinterpret_cast<PointerFunction>(GetProcAddress(
                  probeModule, "getLastScintillaNotificationWindow"))
            : nullptr;
        QAction* closeAction = mainWindow.findChild<QAction*>(
            QStringLiteral("closeAction"));
        QAction* saveAction = mainWindow.findChild<QAction*>(
            QStringLiteral("saveAction"));
        QAction* readOnlyAction = mainWindow.findChild<QAction*>(
            QStringLiteral("readOnlyAction"));
        if (!probeModule || !resetNotifications || !notificationCount
            || !lastNotificationId || !lastNotificationWindow
            || !notificationSequenceCount || !notificationSequenceCode
            || !scintillaNotificationCount
            || !lastScintillaNotificationWindow
            || !closeAction || !saveAction || !readOnlyAction) {
            if (probeModule)
                FreeLibrary(probeModule);
            return 84;
        }

        resetNotifications();
        mainTabs->editor()->setText(QStringLiteral("SCN probe"));
        SendMessageW(win32Plugins->mainEditorHandle(), SCI_SETSEL, 0, 3);
        QApplication::processEvents();
        const bool scintillaNotificationsReceived =
            scintillaNotificationCount(SCN_MODIFIED) >= 1
            && scintillaNotificationCount(SCN_UPDATEUI) >= 1
            && lastScintillaNotificationWindow()
                == reinterpret_cast<ULONG_PTR>(
                    win32Plugins->mainEditorHandle());
        resetNotifications();
        const QString lifecyclePath = QDir::temp().filePath(
            QStringLiteral("npp-qt-plugin-notification-%1.txt")
                .arg(GetCurrentProcessId()));
        QFile lifecycleFile(lifecyclePath);
        if (!lifecycleFile.open(QFile::WriteOnly | QFile::Truncate)
            || lifecycleFile.write("initial\n") < 0) {
            FreeLibrary(probeModule);
            return 86;
        }
        lifecycleFile.close();
        if (!mainWindow.openFileForPlugin(lifecyclePath)) {
            QFile::remove(lifecyclePath);
            FreeLibrary(probeModule);
            return 86;
        }
        QApplication::processEvents();
        const ULONG_PTR openedBufferId =
            lastNotificationId(NppNotificationFileOpened);
        const bool openNotificationsReceived =
            notificationCount(NppNotificationFileBeforeLoad) == 1
            && notificationCount(NppNotificationFileBeforeOpen) == 1
            && notificationCount(NppNotificationFileOpened) == 1
            && openedBufferId != 0
            && lastNotificationId(NppNotificationFileBeforeOpen)
                == openedBufferId
            && notificationCount(NppNotificationBufferActivated) >= 1;

        mainTabs->editor()->setText(QStringLiteral("updated\n"));
        saveAction->trigger();
        QApplication::processEvents();
        const bool saveNotificationsReceived =
            notificationCount(NppNotificationFileBeforeSave) == 1
            && notificationCount(NppNotificationFileSaved) == 1
            && lastNotificationId(NppNotificationFileBeforeSave)
                == openedBufferId
            && lastNotificationId(NppNotificationFileSaved)
                == openedBufferId;

        const bool languageChanged =
            mainWindow.setCurrentLanguageTypeFromPlugin(57)
            && notificationCount(NppNotificationLanguageChanged) == 1
            && lastNotificationId(NppNotificationLanguageChanged)
                == openedBufferId;
        readOnlyAction->trigger();
        readOnlyAction->trigger();
        QApplication::processEvents();
        const bool readOnlyNotificationsReceived =
            notificationCount(NppNotificationReadOnlyChanged) == 2
            && lastNotificationWindow(NppNotificationReadOnlyChanged)
                == openedBufferId
            && lastNotificationId(NppNotificationReadOnlyChanged) == 0;

        win32Plugins->notifyWordStylesUpdated(openedBufferId);
        win32Plugins->notifyDarkModeChanged();
        win32Plugins->notifyBeforeShutdown();
        win32Plugins->notifyCancelShutdown();
        const bool commonNotificationsReceived =
            notificationCount(NppNotificationWordStylesUpdated) == 1
            && notificationCount(NppNotificationDarkModeChanged) == 1
            && notificationCount(NppNotificationBeforeShutdown) == 1
            && notificationCount(NppNotificationCancelShutdown) == 1;

        closeAction->trigger();
        QApplication::processEvents();
        const bool closeNotificationsReceived =
            notificationCount(NppNotificationFileBeforeClose) == 1
            && notificationCount(NppNotificationFileClosed) == 1
            && lastNotificationId(NppNotificationFileBeforeClose)
                == openedBufferId
            && lastNotificationId(NppNotificationFileClosed)
                == openedBufferId;
        auto firstSequenceIndex = [&](UINT code) {
            for (int i = 0; i < notificationSequenceCount(); ++i) {
                if (notificationSequenceCode(i) == code)
                    return i;
            }
            return -1;
        };
        const int beforeLoadIndex =
            firstSequenceIndex(NppNotificationFileBeforeLoad);
        const int beforeOpenIndex =
            firstSequenceIndex(NppNotificationFileBeforeOpen);
        const int openedIndex = firstSequenceIndex(NppNotificationFileOpened);
        const int beforeSaveIndex =
            firstSequenceIndex(NppNotificationFileBeforeSave);
        const int savedIndex = firstSequenceIndex(NppNotificationFileSaved);
        const int beforeCloseIndex =
            firstSequenceIndex(NppNotificationFileBeforeClose);
        const int closedIndex = firstSequenceIndex(NppNotificationFileClosed);
        const bool lifecycleOrderCorrect = beforeLoadIndex >= 0
            && beforeLoadIndex < beforeOpenIndex
            && beforeOpenIndex < openedIndex
            && openedIndex < beforeSaveIndex
            && beforeSaveIndex < savedIndex
            && savedIndex < beforeCloseIndex
            && beforeCloseIndex < closedIndex;
        QFile::remove(lifecyclePath);
        FreeLibrary(probeModule);
        if (!scintillaNotificationsReceived
            || !openNotificationsReceived || !saveNotificationsReceived
            || !languageChanged || !readOnlyNotificationsReceived
            || !commonNotificationsReceived || !closeNotificationsReceived
            || !lifecycleOrderCorrect) {
            return 86;
        }
        return 0;
    }
#endif
    auto isHiddenReceiver = [&](HWND receiver, ScintillaEditView* editorView) {
        RECT bounds{};
        wchar_t className[64]{};
        if (!receiver || receiver == reinterpret_cast<HWND>(editorView->winId())
            || GetParent(receiver) != win32Plugins->mainWindowHandle()
            || IsWindowVisible(receiver)
            || (GetWindowLongPtrW(receiver, GWL_STYLE) & WS_CHILD) == 0
            || !GetWindowRect(receiver, &bounds)
            || bounds.right - bounds.left != 1
            || bounds.bottom - bounds.top != 1
            || !GetClassNameW(receiver, className, 64)
            || QString::fromWCharArray(className) !=
                QStringLiteral("NotepadPlusPlusQt.PluginMessageReceiver")) {
            return false;
        }
        POINT origin{bounds.left, bounds.top};
        if (!ScreenToClient(win32Plugins->mainWindowHandle(), &origin))
            return false;
        return origin.x == 0 && origin.y == 0;
    };
    if (win32Plugins->mainEditorHandle() ==
            win32Plugins->secondaryEditorHandle()
        || !isHiddenReceiver(
            win32Plugins->mainEditorHandle(), mainTabs->editor())
        || !isHiddenReceiver(
            win32Plugins->secondaryEditorHandle(), subTabs->editor())) {
        return 38;
    }
    if (pluginCoexistenceMode) {
        const QStringList pluginNames = win32Plugins->loadedPluginNames();
        const bool diagnosticSubset = qEnvironmentVariableIsSet(
            "NPP_QT_TEST_PLUGIN_COEXISTENCE_SUBSET");
        auto markCoexistenceStage = [&](const QString& stage) {
            QFile progress(QDir(output).filePath(
                QStringLiteral("plugin-coexistence-progress.txt")));
            if (!progress.open(QIODevice::WriteOnly | QIODevice::Text))
                return;
            progress.write(QStringLiteral("phase=%1\nstage=%2\n")
                .arg(pluginCoexistencePhase).arg(stage).toUtf8());
            progress.write(pluginNames.join(QLatin1Char('\n')).toUtf8());
            progress.write("\n");
        };
        markCoexistenceStage(QStringLiteral("loaded"));
        const QStringList expectedPluginNames = {
            QStringLiteral("MIME Tools"),
            QStringLiteral("Reverse Lines"),
            QStringLiteral("Remove Duplicate lines"),
            QStringLiteral("SelectQuotedText"),
            QStringLiteral("BracketsCheck"),
            QStringLiteral("SecurePad"),
            QStringLiteral("Code alignment"),
            QStringLiteral("BetterMultiSelection"),
            QStringLiteral("DoxyIt"),
            QStringLiteral("Elastic Tabstops"),
            QStringLiteral("S&urroundSelection"),
            QStringLiteral("XML Tools"),
            QStringLiteral("Auto Save"),
            QStringLiteral("EditorConfig"),
            QStringLiteral("Session Manager"),
            QStringLiteral("Auto Detect Indention"),
            QStringLiteral("AutoCodepage"),
            QStringLiteral("AutoEolFormat"),
            QStringLiteral("JSON Viewer"),
            QStringLiteral("JsonTools"),
            QStringLiteral("Converter"),
            QStringLiteral("Notepad++ plugin demo"),
            QStringLiteral("Goto Line, Column"),
            QStringLiteral("Random values"),
            QStringLiteral("Merge files in one"),
            QStringLiteral("Selection to Clipboard"),
            QStringLiteral("URL Plugin")
        };
        if (pluginCoexistencePhase < 1 || pluginCoexistencePhase > 2
            || (!diagnosticSubset
                && pluginNames.size() != expectedPluginNames.size())
            || !mainWindow.property("win32PluginLoadErrors")
                    .toStringList().isEmpty()) {
            return 141;
        }
        if (!diagnosticSubset) {
            for (const QString& name : expectedPluginNames) {
                if (!pluginNames.contains(name))
                    return 141;
            }
        }

        if (pluginNames.contains(QStringLiteral("EditorConfig"))) {
            markCoexistenceStage(QStringLiteral("before-editorconfig"));
            if (!mainWindow.openFileForPlugin(editorConfigSamplePath))
                return 142;
            QApplication::processEvents();
            win32Plugins->notifyBufferActivated(
                mainWindow.currentBufferIdForPlugin());
            QApplication::processEvents();
            if (mainTabs->editor()->SendScintillaNpp(SCI_GETUSETABS) != 0
                || mainTabs->editor()->SendScintillaNpp(SCI_GETINDENT) != 3
                || mainTabs->editor()->SendScintillaNpp(SCI_GETTABWIDTH) != 6
                || mainTabs->editor()->SendScintillaNpp(SCI_GETEOLMODE)
                    != SC_EOL_LF) {
                return 142;
            }
            markCoexistenceStage(QStringLiteral("editorconfig"));
        }
        if (pluginNames.contains(QStringLiteral("AutoEolFormat"))) {
            if (!mainWindow.openFileForPlugin(autoEolSamplePath))
                return 143;
            QApplication::processEvents();
            if (mainTabs->editor()->eolMode() != EolUnix
                || mainTabs->editor()->text().contains(QLatin1Char('\r'))) {
                return 143;
            }
            markCoexistenceStage(QStringLiteral("eol"));
        }
        if (pluginNames.contains(QStringLiteral("AutoCodepage"))) {
            if (!mainWindow.openFileForPlugin(autoCodepageSamplePath))
                return 144;
            QApplication::processEvents();
            Buffer* codepageBuffer = mainTabs->currentBuffer();
            if (!codepageBuffer
                || codepageBuffer->getEncoding().compare(
                    QStringLiteral("windows-1251"),
                    Qt::CaseInsensitive) != 0) {
                return 144;
            }
            markCoexistenceStage(QStringLiteral("codepage"));
        }
        if (pluginNames.contains(QStringLiteral("Auto Detect Indention"))) {
            if (!mainWindow.openFileForPlugin(autoIndentSamplePath))
                return 145;
            QApplication::processEvents();
            if (!mainTabs->editor()->SendScintillaNpp(SCI_GETUSETABS))
                return 145;
            markCoexistenceStage(QStringLiteral("indent"));
        }

        if (pluginNames.contains(QStringLiteral("Session Manager"))) {
            const QString sessionPath = QDir(output).filePath(
                QStringLiteral("coexistence-session.xml"));
            const std::wstring nativeSessionPath = QDir::toNativeSeparators(
                sessionPath).toStdWString();
            if (!SendMessageW(win32Plugins->mainWindowHandle(),
                              NppMessageSaveCurrentSession, 0,
                              reinterpret_cast<LPARAM>(
                                  nativeSessionPath.c_str()))
                || !QFileInfo::exists(sessionPath)) {
                return 146;
            }
            markCoexistenceStage(QStringLiteral("session"));
        }

        QFile journal(QDir(parameters.getUserPath()).filePath(
            QStringLiteral("plugin-load/plugin-load.jsonl")));
        if (!journal.open(QIODevice::ReadOnly | QIODevice::Text))
            return 147;
        QSet<QString> startedSessions;
        QSet<QString> completedSessions;
        while (!journal.atEnd()) {
            const QJsonObject event = QJsonDocument::fromJson(
                journal.readLine()).object();
            const QString session = event.value(
                QStringLiteral("session")).toString();
            if (event.value(QStringLiteral("event")).toString()
                    == QStringLiteral("session-start")) {
                startedSessions.insert(session);
            } else if (event.value(QStringLiteral("event")).toString()
                       == QStringLiteral("session-complete")) {
                completedSessions.insert(session);
            }
        }
        if (startedSessions.size() < pluginCoexistencePhase
            || completedSessions.size() < pluginCoexistencePhase
            || QFileInfo(QDir(parameters.getUserPath()).filePath(
                    QStringLiteral(
                        "plugin-load/plugin-load-in-progress.json")))
                    .exists()) {
            return 147;
        }
        markCoexistenceStage(QStringLiteral("journal"));

        QFile report(QDir(output).filePath(QStringLiteral(
            "plugin-coexistence-phase-%1.txt")
                .arg(pluginCoexistencePhase)));
        if (!report.open(QIODevice::WriteOnly | QIODevice::Text))
            return 148;
        report.write(QStringLiteral(
            "phase=%1\nloaded=%2\nsessions-started=%3\n"
            "sessions-completed=%4\n")
            .arg(pluginCoexistencePhase)
            .arg(pluginNames.size())
            .arg(startedSessions.size())
            .arg(completedSessions.size()).toUtf8());
        report.write(pluginNames.join(QLatin1Char('\n')).toUtf8());
        report.write("\n");
        report.close();

        markCoexistenceStage(QStringLiteral("before-close"));
        if (!mainWindow.close())
            return 149;
        QApplication::processEvents();
        if (mainWindow.isVisible())
            return 149;
        markCoexistenceStage(QStringLiteral("closed"));
        return 0;
    }
    if (betterMultiSelectionMode) {
        const QStringList pluginNames = win32Plugins->loadedPluginNames();
        const int pluginIndex = pluginNames.indexOf(
            QStringLiteral("BetterMultiSelection"));
        QAction* enableAction = pluginIndex < 0 ? nullptr
            : mainWindow.findChild<QAction*>(
                QStringLiteral("win32PluginAction_%1_0").arg(pluginIndex));
        if (pluginNames.size() != 1 || pluginIndex < 0
            || win32Plugins->loadedPluginFunctionCount(pluginIndex) != 3
            || !enableAction || !enableAction->isCheckable()
            || !enableAction->isChecked()
            || mainTabs->editor()->SendScintillaNpp(SCI_AUTOCGETMULTI)
                != SC_MULTIAUTOC_EACH) {
            return 87;
        }
        enableAction->trigger();
        QApplication::processEvents();
        if (enableAction->isChecked())
            return 88;
        enableAction->trigger();
        QApplication::processEvents();
        if (!enableAction->isChecked()
            || mainTabs->editor()->SendScintillaNpp(SCI_AUTOCGETMULTI)
                != SC_MULTIAUTOC_EACH) {
            return 89;
        }
        return 0;
    }
    if (p0PluginsMode) {
        const QStringList pluginNames = win32Plugins->loadedPluginNames();
        auto markStage = [&](const QByteArray& stage) {
            QFile progress(output + QStringLiteral("/p0-progress.txt"));
            if (progress.open(QIODevice::WriteOnly))
                progress.write(stage);
        };
        markStage("loaded");
        struct PluginExpectation {
            const char* name;
            int commandCount;
        };
        const PluginExpectation expectations[] = {
            {"DoxyIt", 7},
            {"Elastic Tabstops", 6},
            {"S&urroundSelection", 3},
            {"XML Tools", 37}
        };
        for (const PluginExpectation& expectation : expectations) {
            const int index = pluginNames.indexOf(
                QString::fromLatin1(expectation.name));
            if (index < 0 || win32Plugins->loadedPluginFunctionCount(index)
                    != expectation.commandCount) {
                return 110;
            }
        }

        auto actionFor = [&](const QString& pluginName,
                             const QString& commandName) -> QAction* {
            const int pluginIndex = pluginNames.indexOf(pluginName);
            if (pluginIndex < 0)
                return nullptr;
            for (int functionIndex = 0;
                 functionIndex < win32Plugins->loadedPluginFunctionCount(
                     pluginIndex); ++functionIndex) {
                if (win32Plugins->loadedPluginFunctionName(
                        pluginIndex, functionIndex) == commandName) {
                    return mainWindow.findChild<QAction*>(
                        QStringLiteral("win32PluginAction_%1_%2")
                            .arg(pluginIndex).arg(functionIndex));
                }
            }
            return nullptr;
        };
        auto actionStartingWith = [&](const QString& pluginName,
                                      const QString& commandPrefix) {
            const int pluginIndex = pluginNames.indexOf(pluginName);
            if (pluginIndex < 0)
                return static_cast<QAction*>(nullptr);
            for (int functionIndex = 0;
                 functionIndex < win32Plugins->loadedPluginFunctionCount(
                     pluginIndex); ++functionIndex) {
                if (win32Plugins->loadedPluginFunctionName(
                        pluginIndex, functionIndex).startsWith(commandPrefix)) {
                    return mainWindow.findChild<QAction*>(
                        QStringLiteral("win32PluginAction_%1_%2")
                            .arg(pluginIndex).arg(functionIndex));
                }
            }
            return static_cast<QAction*>(nullptr);
        };
        auto setSelection = [&](const QByteArray& text) {
            SendMessageW(win32Plugins->mainEditorHandle(),
                         SCI_CLEARSELECTIONS, 0, 0);
            mainTabs->editor()->setText(QString::fromUtf8(text));
            SendMessageW(win32Plugins->mainEditorHandle(), SCI_SETSEL, 0,
                         static_cast<LPARAM>(text.size()));
        };

        mainTabs->editor()->setBuiltinLanguage(QStringLiteral("cpp"));
        win32Plugins->notifyLanguageChanged(
            mainWindow.currentBufferIdForPlugin());
        int currentLanguageType = -1;
        SendMessageW(win32Plugins->mainWindowHandle(),
                     NppMessageGetCurrentLangType, 0,
                     reinterpret_cast<LPARAM>(&currentLanguageType));
        if (currentLanguageType != 3)
            return 119;
        wchar_t languageName[16]{};
        if (SendMessageW(win32Plugins->mainWindowHandle(),
                         NppMessageGetLanguageName, 3,
                         reinterpret_cast<LPARAM>(languageName)) != 3
            || QString::fromWCharArray(languageName)
                != QStringLiteral("C++")) {
            return 119;
        }
        setSelection(QByteArray("int add(int a, int b);\n"));
        SendMessageW(win32Plugins->mainEditorHandle(), SCI_SETSEL, 0, 0);
        QAction* doxyFile = actionFor(
            QStringLiteral("DoxyIt"), QStringLiteral("Comment File"));
        if (!doxyFile)
            return 111;
        doxyFile->trigger();
        QApplication::processEvents();
        const QByteArray doxyText = mainTabs->editor()->text().toUtf8();
        if (!doxyText.contains("\\file") || !doxyText.contains("/**")) {
            QFile diagnostic(output + QStringLiteral("/doxyit-output.txt"));
            if (diagnostic.open(QIODevice::WriteOnly))
                diagnostic.write(doxyText);
            return 120;
        }
        markStage("doxy-command");
        setSelection(QByteArray("int add(int a, int b);\n"));
        SendMessageW(win32Plugins->mainEditorHandle(), SCI_SETSEL, 0, 0);
        QAction* doxyFunction = actionFor(
            QStringLiteral("DoxyIt"), QStringLiteral("Comment Function"));
        if (!doxyFunction)
            return 124;
        doxyFunction->trigger();
        QApplication::processEvents();
        const QByteArray doxyFunctionText =
            mainTabs->editor()->text().toUtf8();
        if (!doxyFunctionText.contains("\\param [in] a")
            || !doxyFunctionText.contains("\\param [in] b")) {
            QFile diagnostic(
                output + QStringLiteral("/doxyit-function-output.txt"));
            if (diagnostic.open(QIODevice::WriteOnly))
                diagnostic.write(doxyFunctionText);
            return 124;
        }

        QAction* doxySettings = actionFor(
            QStringLiteral("DoxyIt"), QStringLiteral("Settings..."));
        if (!doxySettings)
            return 112;
        doxySettings->trigger();
        QApplication::processEvents();
        bool exactSettingsTitle = false;
        HWND settingsDialog = findCurrentProcessDialog(
            L"DoxyIt - Settings", &exactSettingsTitle);
        if (!settingsDialog || !exactSettingsTitle
            || GetWindow(settingsDialog, GW_OWNER)
                != win32Plugins->mainWindowHandle()) {
            return 112;
        }
        PostMessageW(settingsDialog, WM_CLOSE, 0, 0);
        QApplication::processEvents();
        markStage("doxy-settings");

        setSelection(QByteArray("a\t1\nlonger\t2\n"));
        markStage("elastic-text");
        win32Plugins->notifyBufferActivated(
            mainWindow.currentBufferIdForPlugin());
        markStage("elastic-activated");
        QApplication::processEvents();
        const LRESULT tabStop = SendMessageW(
            win32Plugins->mainEditorHandle(), SCI_GETNEXTTABSTOP, 0, 0);
        markStage("elastic-tabstop");
        QAction* convertTabs = actionFor(
            QStringLiteral("Elastic Tabstops"),
            QStringLiteral("Convert Tabstops to Spaces"));
        if (!convertTabs)
            return 121;
        if (tabStop <= 0) {
            QFile diagnostic(output + QStringLiteral("/elastic-tabstop.txt"));
            if (diagnostic.open(QIODevice::WriteOnly | QIODevice::Text)) {
                diagnostic.write(QByteArray::number(tabStop));
                diagnostic.write("\n");
                diagnostic.write(mainTabs->editor()->text().toUtf8());
            }
            return 122;
        }
        convertTabs->trigger();
        markStage("elastic-command");
        QApplication::processEvents();
        if (mainTabs->editor()->text().contains(QLatin1Char('\t')))
            return 113;
        markStage("elastic");

        QAction* surroundEnable = actionFor(
            QStringLiteral("S&urroundSelection"), QStringLiteral("&Enable"));
        if (!surroundEnable || !surroundEnable->isCheckable()
            || !surroundEnable->isChecked()) {
            return 114;
        }
        surroundEnable->trigger();
        QApplication::processEvents();
        if (surroundEnable->isChecked())
            return 114;
        surroundEnable->trigger();
        QApplication::processEvents();
        if (!surroundEnable->isChecked())
            return 114;
        markStage("surround");

        mainTabs->editor()->setBuiltinLanguage(QStringLiteral("xml"));
        win32Plugins->notifyLanguageChanged(
            mainWindow.currentBufferIdForPlugin());
        setSelection(QByteArray("<root><item a=\"1\">x</item></root>"));
        QAction* prettyPrint = actionFor(
            QStringLiteral("XML Tools"), QStringLiteral("Pretty print"));
        QAction* linearize = actionFor(
            QStringLiteral("XML Tools"), QStringLiteral("Linearize"));
        if (!prettyPrint || !linearize)
            return 115;
        prettyPrint->trigger();
        QApplication::processEvents();
        if (!mainTabs->editor()->text().contains(QLatin1Char('\n')))
            return 115;
        linearize->trigger();
        QApplication::processEvents();
        if (mainTabs->editor()->text().contains(QLatin1Char('\n'))
            || !mainTabs->editor()->text().contains(
                QStringLiteral("<item a=\"1\">x</item>"))) {
            return 115;
        }
        markStage("xml-format");

        QAction* escape = actionStartingWith(
            QStringLiteral("XML Tools"),
            QStringLiteral("Escape characters in selection"));
        QAction* unescape = actionStartingWith(
            QStringLiteral("XML Tools"),
            QStringLiteral("Unescape characters in selection"));
        QAction* comment = actionFor(
            QStringLiteral("XML Tools"), QStringLiteral("Comment selection"));
        QAction* uncomment = actionFor(
            QStringLiteral("XML Tools"), QStringLiteral("Uncomment selection"));
        if (!escape || !unescape || !comment || !uncomment)
            return 123;
        setSelection(QByteArray("<node>&</node>"));
        escape->trigger();
        QApplication::processEvents();
        if (!mainTabs->editor()->text().contains(
                QStringLiteral("&lt;node&gt;"))) {
            QFile diagnostic(output + QStringLiteral("/xml-escape.txt"));
            if (diagnostic.open(QIODevice::WriteOnly))
                diagnostic.write(mainTabs->editor()->text().toUtf8());
            return 116;
        }
        SendMessageW(win32Plugins->mainEditorHandle(), SCI_SELECTALL, 0, 0);
        unescape->trigger();
        QApplication::processEvents();
        if (mainTabs->editor()->text() != QStringLiteral("<node>&</node>"))
            return 116;
        setSelection(QByteArray("value"));
        comment->trigger();
        QApplication::processEvents();
        if (mainTabs->editor()->text() != QStringLiteral("<!--value-->"))
            return 117;
        SendMessageW(win32Plugins->mainEditorHandle(), SCI_SELECTALL, 0, 0);
        uncomment->trigger();
        QApplication::processEvents();
        if (mainTabs->editor()->text() != QStringLiteral("value"))
            return 117;
        markStage("xml-text");

        QAction* checkXml = actionFor(
            QStringLiteral("XML Tools"),
            QStringLiteral("Check XML syntax now"));
        if (!checkXml)
            return 118;
        setSelection(QByteArray("<root><item></root>"));
        markStage("xml-malformed-text");
        checkXml->trigger();
        markStage("xml-check-triggered");
        QApplication::processEvents();
        if (mainTabs->editor()->SendScintillaNpp(
                SCI_ANNOTATIONGETTEXT, 0, 0) <= 0) {
            return 118;
        }
        markStage("xml-annotation");
        mainTabs->editor()->SendScintillaNpp(SCI_ANNOTATIONCLEARALL);
        mainTabs->editor()->setText(QStringLiteral("<root>"));
        mainTabs->editor()->SendScintillaNpp(
            SCI_SETSEL, mainTabs->editor()->text().size(),
            mainTabs->editor()->text().size());
        Scintilla::NotificationData charAdded{};
        charAdded.nmhdr.code = Scintilla::Notification::CharAdded;
        charAdded.ch = '>';
        win32Plugins->notifyScintilla(charAdded, true);
        QApplication::processEvents();
        markStage("xml-char-added");
        if (mainTabs->editor()->text()
                != QStringLiteral("<root></root>")) {
            return 125;
        }

        QAction* preventXxe = actionFor(
            QStringLiteral("XML Tools"), QStringLiteral("Prevent XXE"));
        if (!preventXxe || !preventXxe->isCheckable()
            || !preventXxe->isChecked()) {
            return 126;
        }
        preventXxe->trigger();
        QApplication::processEvents();
        if (preventXxe->isChecked())
            return 126;
        preventXxe->trigger();
        QApplication::processEvents();
        if (!preventXxe->isChecked())
            return 126;
        markStage("xml-toggle");

        QAction* xmlOptions = actionFor(
            QStringLiteral("XML Tools"), QStringLiteral("Options..."));
        if (!xmlOptions)
            return 127;
        xmlOptions->trigger();
        QApplication::processEvents();
        markStage("xml-options-opened");
        bool exactOptionsTitle = false;
        HWND optionsDialog = findCurrentProcessDialog(
            L"Options", &exactOptionsTitle);
        if (!optionsDialog || !exactOptionsTitle)
            return 127;
        PostMessageW(optionsDialog, WM_CLOSE, 0, 0);
        QApplication::processEvents();
        markStage("complete");
        return 0;
    }
    if (configurationPluginsMode) {
        const QStringList pluginNames = win32Plugins->loadedPluginNames();
        const auto windowProc = reinterpret_cast<const void*>(
            GetWindowLongPtrW(win32Plugins->mainWindowHandle(),
                              GWLP_WNDPROC));
        HMODULE windowProcModule = nullptr;
        wchar_t modulePath[MAX_PATH]{};
        if (windowProc
            && GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
                    | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(windowProc),
                &windowProcModule)) {
            GetModuleFileNameW(windowProcModule, modulePath, MAX_PATH);
        }
        const QString windowProcModulePath =
            QString::fromWCharArray(modulePath);
        QFile report(output + QStringLiteral(
            "/configuration-plugin-functions.txt"));
        if (report.open(QIODevice::WriteOnly | QIODevice::Text)) {
            for (int pluginIndex = 0;
                 pluginIndex < win32Plugins->loadedPluginCount();
                 ++pluginIndex) {
                report.write(QStringLiteral("[%1]\n")
                    .arg(pluginNames.value(pluginIndex)).toUtf8());
                for (int functionIndex = 0;
                     functionIndex < win32Plugins->loadedPluginFunctionCount(
                         pluginIndex); ++functionIndex) {
                    report.write(QStringLiteral("%1:%2\n")
                        .arg(functionIndex)
                        .arg(win32Plugins->loadedPluginFunctionName(
                            pluginIndex, functionIndex)).toUtf8());
                }
            }
            report.write(QStringLiteral("wndproc:%1\n")
                .arg(windowProcModulePath).toUtf8());
        }
        if (win32Plugins->loadedPluginCount() != 2
            || !pluginNames.contains(QStringLiteral("Auto Save"))
            || !pluginNames.contains(QStringLiteral("EditorConfig"))
            || !windowProcModulePath.endsWith(
                QStringLiteral("AutoSave.dll"), Qt::CaseInsensitive)
            || !mainWindow.property("win32PluginLoadErrors")
                    .toStringList().isEmpty()) {
            return 128;
        }

        if (!mainWindow.openFileForPlugin(editorConfigSamplePath))
            return 130;
        QApplication::processEvents();
        win32Plugins->notifyBufferActivated(
            mainWindow.currentBufferIdForPlugin());
        QApplication::processEvents();
        const sptr_t useTabs = mainTabs->editor()->SendScintillaNpp(
            SCI_GETUSETABS);
        const sptr_t indent = mainTabs->editor()->SendScintillaNpp(
            SCI_GETINDENT);
        const sptr_t tabWidth = mainTabs->editor()->SendScintillaNpp(
            SCI_GETTABWIDTH);
        const sptr_t eolMode = mainTabs->editor()->SendScintillaNpp(
            SCI_GETEOLMODE);
        if (useTabs != 0 || indent != 3 || tabWidth != 6
            || eolMode != SC_EOL_LF) {
            QFile diagnostic(output + QStringLiteral(
                "/editor-config-values.txt"));
            if (diagnostic.open(QIODevice::WriteOnly | QIODevice::Text)) {
                diagnostic.write(QStringLiteral(
                    "path=%1\nuseTabs=%2\nindent=%3\ntabWidth=%4\neol=%5\n")
                    .arg(mainWindow.currentPathForPlugin())
                    .arg(useTabs).arg(indent).arg(tabWidth).arg(eolMode)
                    .toUtf8());
            }
            return 130;
        }
        mainTabs->editor()->setText(
            QStringLiteral("first   \r\nsecond   "));
        QApplication::processEvents();
        if (!mainWindow.executePluginMenuCommand(41006))
            return 131;
        QFile editorConfigResult(editorConfigSamplePath);
        if (!editorConfigResult.open(QIODevice::ReadOnly)
            || editorConfigResult.readAll() != QByteArray("first\nsecond\n")) {
            return 131;
        }

        const int editorConfigIndex = pluginNames.indexOf(
            QStringLiteral("EditorConfig"));
        if (editorConfigIndex < 0
            || !win32Plugins->executePluginCommand(
                editorConfigIndex, 0)) {
            return 132;
        }

        const int autoSaveIndex = pluginNames.indexOf(
            QStringLiteral("Auto Save"));
        if (autoSaveIndex < 0)
            return 134;
        std::atomic_bool appliedOptions{false};
        std::thread applyOptionsWorker([&] {
            for (int attempt = 0; attempt < 300; ++attempt) {
                bool exactMatch = false;
                HWND dialog = findCurrentProcessDialog(
                    L"AutoSave Options", &exactMatch);
                if (dialog) {
                    appliedOptions = true;
                    PostMessageW(dialog, WM_COMMAND,
                                 MAKEWPARAM(IDOK, BN_CLICKED), 0);
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        const bool optionsApplied = win32Plugins->executePluginCommand(
            autoSaveIndex, 2);
        applyOptionsWorker.join();
        QApplication::processEvents();
        if (!optionsApplied || !appliedOptions.load())
            return 134;

        if (!mainWindow.openFileForPlugin(autoSaveSamplePath))
            return 133;
        QApplication::processEvents();
        mainTabs->editor()->setText(QStringLiteral("saved on deactivate\n"));
        QApplication::processEvents();
        QDir autoSaveDirectory(QFileInfo(autoSaveSamplePath).absolutePath());
        const QStringList filesBeforeCopyList =
            autoSaveDirectory.entryList(QDir::Files);
        const QSet<QString> filesBeforeCopy =
            QSet<QString>::fromList(filesBeforeCopyList);
        if (!win32Plugins->executePluginCommand(autoSaveIndex, 0))
            return 136;
        QApplication::processEvents();
        const QStringList filesAfterCopyList =
            autoSaveDirectory.entryList(QDir::Files);
        const QSet<QString> filesAfterCopy =
            QSet<QString>::fromList(filesAfterCopyList);
        const QSet<QString> copiedFiles = filesAfterCopy - filesBeforeCopy;
        if (copiedFiles.size() != 1)
            return 136;
        QFile copiedFile(autoSaveDirectory.filePath(*copiedFiles.cbegin()));
        QFile originalFile(autoSaveSamplePath);
        if (!copiedFile.open(QIODevice::ReadOnly)
            || copiedFile.readAll() != QByteArray("saved on deactivate\n")
            || !originalFile.open(QIODevice::ReadOnly)
            || originalFile.readAll() != QByteArray("initial\n")
            || !mainTabs->editor()->isModified()) {
            return 136;
        }
        copiedFile.close();
        originalFile.close();
        if (qEnvironmentVariableIsSet("NPP_QT_TEST_AUTOSAVE_TIMER")) {
            const QDeadlineTimer deadline(75000);
            QByteArray diskContents;
            do {
                QApplication::processEvents();
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                QFile result(autoSaveSamplePath);
                if (result.open(QIODevice::ReadOnly))
                    diskContents = result.readAll();
            } while (diskContents != QByteArray("saved on deactivate\n")
                     && !deadline.hasExpired());
            if (diskContents != QByteArray("saved on deactivate\n")
                || mainTabs->editor()->isModified())
                return 140;
        }
        if (qEnvironmentVariableIsSet("NPP_QT_TEST_REAL_FOCUS")) {
            SetForegroundWindow(win32Plugins->mainWindowHandle());
            QApplication::processEvents();
#ifdef NPP_WIN32_FOCUS_WINDOW
            QProcess focusWindow;
            focusWindow.start(
                QString::fromUtf8(NPP_WIN32_FOCUS_WINDOW),
                {QString::number(reinterpret_cast<quintptr>(
                    win32Plugins->mainWindowHandle()))});
            if (!focusWindow.waitForStarted(3000))
                return 133;
            while (focusWindow.state() != QProcess::NotRunning) {
                QApplication::processEvents();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            if (focusWindow.exitStatus() != QProcess::NormalExit
                || focusWindow.exitCode() != 0) {
                QFile diagnostic(output + QStringLiteral(
                    "/auto-save-focus.txt"));
                if (diagnostic.open(
                        QIODevice::WriteOnly | QIODevice::Text)) {
                    diagnostic.write(QStringLiteral(
                        "status=%1\nexit=%2\nforeground=%3\nmain=%4\n")
                        .arg(static_cast<int>(focusWindow.exitStatus()))
                        .arg(focusWindow.exitCode())
                        .arg(reinterpret_cast<quintptr>(
                            GetForegroundWindow()))
                        .arg(reinterpret_cast<quintptr>(
                            win32Plugins->mainWindowHandle()))
                        .toUtf8());
                }
                return 135;
            }
#endif
            QApplication::processEvents();
            QFile autoSaveResult(autoSaveSamplePath);
            if (!autoSaveResult.open(QIODevice::ReadOnly)
                || autoSaveResult.readAll()
                    != QByteArray("saved on deactivate\n")) {
                QFile diagnostic(output + QStringLiteral(
                    "/auto-save-values.txt"));
                if (diagnostic.open(
                        QIODevice::WriteOnly | QIODevice::Text)) {
                    QFile current(autoSaveSamplePath);
                    current.open(QIODevice::ReadOnly);
                    diagnostic.write(QStringLiteral(
                        "path=%1\nmodified=%2\ndisk=%3\n")
                        .arg(mainWindow.currentPathForPlugin())
                        .arg(mainTabs->editor()->isModified())
                        .arg(QString::fromUtf8(current.readAll()))
                        .toUtf8());
                }
                return 133;
            }
        }

        std::atomic_bool optionsFound{false};
        std::thread optionsWorker([&] {
            for (int attempt = 0; attempt < 300; ++attempt) {
                bool exactMatch = false;
                HWND dialog = findCurrentProcessDialog(
                    L"AutoSave Options", &exactMatch);
                if (dialog) {
                    optionsFound = true;
                    PostMessageW(dialog, WM_CLOSE, 0, 0);
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        const bool optionsExecuted = win32Plugins->executePluginCommand(
            autoSaveIndex, 2);
        optionsWorker.join();
        QApplication::processEvents();
        if (!optionsExecuted || !optionsFound.load())
            return 134;
        return 0;
    }
#ifdef NPP_MIMETOOLS_MANAGED_TEST
    QFile receipt(QDir(QCoreApplication::applicationDirPath()).filePath(
        QStringLiteral("plugins/mimeTools/.npp-package.json")));
    if (!receipt.open(QFile::ReadOnly))
        return 39;
    const QJsonObject receiptObject =
        QJsonDocument::fromJson(receipt.readAll()).object();
    const QStringList loadedPluginNames = win32Plugins->loadedPluginNames();
    QFile loadedPluginsFile(output + QStringLiteral(
        "/loaded-win32-plugins.txt"));
    if (loadedPluginsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        loadedPluginsFile.write(loadedPluginNames.join(QLatin1Char('\n'))
                                    .toUtf8());
        loadedPluginsFile.write("\n");
    }
    QFile pluginErrorsFile(output + QStringLiteral(
        "/win32-plugin-load-errors.txt"));
    if (pluginErrorsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        pluginErrorsFile.write(
            mainWindow.property("win32PluginLoadErrors").toStringList()
                .join(QLatin1Char('\n')).toUtf8());
        pluginErrorsFile.write("\n");
    }
    QFile pluginFunctionsFile(output + QStringLiteral(
        "/loaded-win32-plugin-functions.txt"));
    if (pluginFunctionsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        for (int pluginIndex = 0;
             pluginIndex < win32Plugins->loadedPluginCount(); ++pluginIndex) {
            pluginFunctionsFile.write(
                QStringLiteral("[%1]\n")
                    .arg(loadedPluginNames.value(pluginIndex)).toUtf8());
            const int functionCount =
                win32Plugins->loadedPluginFunctionCount(pluginIndex);
            for (int functionIndex = 0; functionIndex < functionCount;
                 ++functionIndex) {
                pluginFunctionsFile.write(
                    QStringLiteral("%1:%2\n")
                        .arg(functionIndex)
                        .arg(win32Plugins->loadedPluginFunctionName(
                            pluginIndex, functionIndex)).toUtf8());
            }
        }
    }
    const int mimeToolsPluginIndex = loadedPluginNames.indexOf(
        QStringLiteral("MIME Tools"));
    if (receiptObject.value(QStringLiteral("folder-name")).toString() !=
            QStringLiteral("mimeTools")
        || receiptObject.value(QStringLiteral("version")).toString() !=
            QStringLiteral("2.8")
        || mimeToolsPluginIndex < 0
        || win32Plugins->loadedPluginFunctionCount(mimeToolsPluginIndex) <= 0) {
        return 39;
    }

    QMenu* mimeToolsMenu = mainWindow.findChild<QMenu*>(
        QStringLiteral("win32PluginMenu_%1").arg(mimeToolsPluginIndex));
    if (!mimeToolsMenu || mimeToolsMenu->title() != QStringLiteral("MIME Tools")
        || mimeToolsMenu->actions().size() != 18
        || mainWindow.findChild<QAction*>(
               QStringLiteral("noPluginsLoadedAction"))) {
        return 43;
    }
    const QStringList expectedMimeToolsItems = {
        QStringLiteral("Base64 Encode"),
        QStringLiteral("Base64 Encode with padding"),
        QStringLiteral("Base64 Encode with Unix EOL"),
        QStringLiteral("Base64 Encode by line"),
        QStringLiteral("Base64 Decode"),
        QStringLiteral("Base64 Decode strict"),
        QStringLiteral("Base64 Decode by line"),
        QStringLiteral("-SEPARATOR-"),
        QStringLiteral("Quoted-printable Encode"),
        QStringLiteral("Quoted-printable Decode"),
        QStringLiteral("-SEPARATOR-"),
        QStringLiteral("URL Encode"),
        QStringLiteral("Full URL Encode"),
        QStringLiteral("URL Decode"),
        QStringLiteral("-SEPARATOR-"),
        QStringLiteral("SAML Decode"),
        QStringLiteral("-SEPARATOR-"),
        QStringLiteral("About")
    };
    for (int index = 0; index < expectedMimeToolsItems.size(); ++index) {
        QAction* action = mimeToolsMenu->actions().at(index);
        if (expectedMimeToolsItems.at(index) == QStringLiteral("-SEPARATOR-")) {
            if (!action->isSeparator())
                return 43;
        } else if (action->text() != expectedMimeToolsItems.at(index)
                   || action->objectName() !=
                       QStringLiteral("win32PluginAction_%1_%2")
                           .arg(mimeToolsPluginIndex).arg(index)) {
            return 43;
        }
    }

    auto verifyMimeToolsCommand = [&](ScintillaEditView* editorView,
                                      HWND receiver,
                                      const QByteArray& source,
                                      const QByteArray& expected,
                                      int expectedView,
                                      int commandIndex) {
        editorView->setText(QString::fromUtf8(source));
        SendMessageW(receiver, SCI_SETSEL, 0,
                     static_cast<LPARAM>(source.size()));
        int currentView = -1;
        if (!SendMessageW(
                win32Plugins->mainWindowHandle(),
                NppMessageGetCurrentScintilla, 0,
                reinterpret_cast<LPARAM>(&currentView))
            || currentView != expectedView) {
            return false;
        }
        QAction* action = mainWindow.findChild<QAction*>(
            QStringLiteral("win32PluginAction_%1_%2")
                .arg(mimeToolsPluginIndex).arg(commandIndex));
        if (!action)
            return false;
        action->trigger();
        QApplication::processEvents();
        return editorView->text().toUtf8() == expected;
    };

    mainTabs->editor()->setFocus(Qt::OtherFocusReason);
    QApplication::processEvents();
    if (mainWindow.currentView() != mainTabs->editor()
        || !verifyMimeToolsCommand(
            mainTabs->editor(), win32Plugins->mainEditorHandle(),
            QByteArray("hello"), QByteArray("aGVsbG8"), 0, 0)
        || !verifyMimeToolsCommand(
            mainTabs->editor(), win32Plugins->mainEditorHandle(),
            QByteArray("a b"), QByteArray("a%20b"), 0, 11)) {
        return 40;
    }

    struct MimeToolsCase {
        int commandIndex;
        QByteArray source;
        QByteArray expected;
    };
    const QVector<MimeToolsCase> mimeToolsCases = {
        {0, QByteArray("a"), QByteArray("YQ")},
        {1, QByteArray("a"), QByteArray("YQ==")},
        {2, QByteArray("a"), QByteArray("YQ==")},
        {3, QByteArray("a\r\nb"), QByteArray("YQ\r\nYg")},
        {4, QByteArray("YQ=="), QByteArray("a")},
        {5, QByteArray("YQ=="), QByteArray("a")},
        {6, QByteArray("YQ==\r\nYg=="), QByteArray("a\r\nb")},
        {8, QByteArray("A=B"), QByteArray("A=3DB")},
        {9, QByteArray("A=3DB"), QByteArray("A=B")},
        {11, QByteArray("a b/?"), QByteArray("a%20b%2F%3F")},
        {12, QByteArray("AZ"), QByteArray("%41%5A")},
        {13, QByteArray("%41%5A"), QByteArray("AZ")},
        {15, QByteArray("AQ8A8P88c2FtbD5vazwvc2FtbD4%3D"),
         QByteArray("<saml>ok</saml>")}
    };
    for (const MimeToolsCase& testCase : mimeToolsCases) {
        if (!verifyMimeToolsCommand(
                mainTabs->editor(), win32Plugins->mainEditorHandle(),
                testCase.source, testCase.expected, 0,
                testCase.commandIndex)) {
            return 44;
        }
    }

    for (const MimeToolsCase& testCase : mimeToolsCases) {
        mainTabs->editor()->setText(QStringLiteral("unchanged"));
        SendMessageW(win32Plugins->mainEditorHandle(), SCI_SETSEL, 0, 0);
        QAction* action = mainWindow.findChild<QAction*>(
            QStringLiteral("win32PluginAction_%1_%2")
                .arg(mimeToolsPluginIndex).arg(testCase.commandIndex));
        if (!action)
            return 44;
        action->trigger();
        QApplication::processEvents();
        if (mainTabs->editor()->text() != QStringLiteral("unchanged"))
            return 44;
    }

#ifdef NPP_WIN32_PLUGIN_CORPUS_MANAGED_TEST
    struct PluginReceiptExpectation {
        const char* folder;
        const char* version;
    };
    const PluginReceiptExpectation pluginReceipts[] = {
        {"qkNppReverseLines", "1.0.0.0"},
        {"Remove Duplicate Lines", "1.3.0.0"},
        {"SelectQuotedText", "1.0.0"},
        {"BracketsCheck", "1.2.2"},
        {"PoorMansTSqlFormatterNppPlugin", "1.6.13.31508"},
        {"SecurePad", "2.4"},
        {"CodeAlignmentNpp", "14.1.107"},
        {"BetterMultiSelection", "1.5"},
        {"NPPJSONViewer", "1.41"},
        {"JsonTools", "3.2.0"},
        {"nppConverter", "4.4.0"},
        {"NppPluginDemo", "4.2"},
        {"GotoLineCol", "2.4.2.0"},
        {"RandomValuesNppPlugin", "0.2.1"},
        {"Merge files in one", "1.2.0.0"},
        {"SelectToClipboard", "1.0.3"},
        {"urlPlugin", "1.2.0.0"}
    };
    const QDir pluginRoot(QDir(QCoreApplication::applicationDirPath())
                              .filePath(QStringLiteral("plugins")));
    for (const PluginReceiptExpectation& expectation : pluginReceipts) {
        QFile pluginReceipt(pluginRoot.filePath(
            QString::fromLatin1(expectation.folder)
            + QStringLiteral("/.npp-package.json")));
        if (!pluginReceipt.open(QFile::ReadOnly))
            return 70;
        const QJsonObject object =
            QJsonDocument::fromJson(pluginReceipt.readAll()).object();
        if (object.value(QStringLiteral("folder-name")).toString()
                != QString::fromLatin1(expectation.folder)
            || object.value(QStringLiteral("version")).toString()
                != QString::fromLatin1(expectation.version)) {
            return 70;
        }
    }

    const QStringList expectedAuditedPlugins = {
        QStringLiteral("BracketsCheck"),
        QStringLiteral("BetterMultiSelection"),
        QStringLiteral("Code alignment"),
        QStringLiteral("Converter"),
        QStringLiteral("Goto Line, Column"),
        QStringLiteral("Merge files in one"),
        QStringLiteral("Notepad++ plugin demo"),
        QStringLiteral("Random values"),
        QStringLiteral("Reverse Lines"),
        QStringLiteral("Remove Duplicate lines"),
        QStringLiteral("SecurePad"),
        QStringLiteral("SelectQuotedText"),
        QStringLiteral("Selection to Clipboard"),
        QStringLiteral("URL Plugin")
    };
    QFile jsonViewerDiagnostic(
        output + QStringLiteral("/json-viewer-commands.txt"));
    if (jsonViewerDiagnostic.open(QIODevice::WriteOnly | QIODevice::Text)) {
        for (int pluginIndex = 0;
             pluginIndex < win32Plugins->loadedPluginCount(); ++pluginIndex) {
            jsonViewerDiagnostic.write(
                win32Plugins->loadedPluginNames().at(pluginIndex).toUtf8());
            jsonViewerDiagnostic.write("\n");
            for (int functionIndex = 0;
                 functionIndex < win32Plugins->loadedPluginFunctionCount(
                     pluginIndex); ++functionIndex) {
                jsonViewerDiagnostic.write("  ");
                jsonViewerDiagnostic.write(
                    win32Plugins->loadedPluginFunctionName(
                        pluginIndex, functionIndex).toUtf8());
                jsonViewerDiagnostic.write("\n");
            }
        }
        jsonViewerDiagnostic.close();
    }
    for (const QString& pluginName : expectedAuditedPlugins) {
        if (!loadedPluginNames.contains(pluginName))
            return 71;
    }
    const QStringList loadErrors =
        mainWindow.property("win32PluginLoadErrors").toStringList();
    if (loadedPluginNames.contains(
            QStringLiteral("Poor Man's T-Sql Formatter"))
        || !loadErrors.isEmpty()) {
        return 71;
    }
    const int betterMultiSelectionIndex = loadedPluginNames.indexOf(
        QStringLiteral("BetterMultiSelection"));
    QAction* betterMultiSelectionEnable =
        betterMultiSelectionIndex < 0 ? nullptr
        : mainWindow.findChild<QAction*>(
            QStringLiteral("win32PluginAction_%1_0")
                .arg(betterMultiSelectionIndex));
    if (betterMultiSelectionIndex < 0
        || win32Plugins->loadedPluginFunctionCount(
               betterMultiSelectionIndex) != 3
        || !betterMultiSelectionEnable
        || !betterMultiSelectionEnable->isCheckable()
        || !betterMultiSelectionEnable->isChecked()
        || mainTabs->editor()->SendScintillaNpp(SCI_AUTOCGETMULTI)
            != SC_MULTIAUTOC_EACH) {
        return 72;
    }

    auto pluginAction = [&](const QString& pluginName, int functionIndex) {
        const int pluginIndex = loadedPluginNames.indexOf(pluginName);
        return mainWindow.findChild<QAction*>(
            QStringLiteral("win32PluginAction_%1_%2")
                .arg(pluginIndex).arg(functionIndex));
    };
    auto runSelectedCommand = [&](const QString& pluginName,
                                  int functionIndex,
                                  const QByteArray& source) {
        mainTabs->editor()->setText(QString::fromUtf8(source));
        SendMessageW(win32Plugins->mainEditorHandle(), SCI_SETSEL, 0,
                     static_cast<LPARAM>(source.size()));
        QAction* action = pluginAction(pluginName, functionIndex);
        if (!action)
            return QByteArray();
        action->trigger();
        QApplication::processEvents();
        return mainTabs->editor()->text().toUtf8();
    };

    const int converterIndex = loadedPluginNames.indexOf(
        QStringLiteral("Converter"));
    const int pluginDemoIndex = loadedPluginNames.indexOf(
        QStringLiteral("Notepad++ plugin demo"));
    if (converterIndex < 0 || pluginDemoIndex < 0
        || win32Plugins->loadedPluginFunctionCount(converterIndex) != 7
        || win32Plugins->loadedPluginFunctionCount(pluginDemoIndex) != 16
        || win32Plugins->loadedPluginFunctionName(converterIndex, 0)
            != QStringLiteral("ASCII -> HEX")
        || win32Plugins->loadedPluginFunctionName(converterIndex, 1)
            != QStringLiteral("HEX -> ASCII")
        || win32Plugins->loadedPluginFunctionName(pluginDemoIndex, 0)
            != QStringLiteral("Hello Notepad++")
        || !win32Plugins->isLoadedPluginFunctionSeparator(converterIndex, 2)
        || !win32Plugins->isLoadedPluginFunctionSeparator(pluginDemoIndex, 3)) {
        return 107;
    }
    if (runSelectedCommand(QStringLiteral("Converter"), 0,
                           QByteArray("Npp")) != QByteArray("4E7070")
        || runSelectedCommand(QStringLiteral("Converter"), 1,
                              QByteArray("4E7070")) != QByteArray("Npp")) {
        return 108;
    }

    struct PluginCommandTableExpectation {
        const char* name;
        int count;
        int separatorIndex;
    };
    const PluginCommandTableExpectation addedPluginTables[] = {
        {"Goto Line, Column", 4, 2},
        {"Random values", 11, 5},
        {"Merge files in one", 2, -1},
        {"Selection to Clipboard", 2, -1},
        {"URL Plugin", 5, 3}
    };
    for (const PluginCommandTableExpectation& expectation
         : addedPluginTables) {
        const int index = loadedPluginNames.indexOf(
            QString::fromLatin1(expectation.name));
        if (index < 0
            || win32Plugins->loadedPluginFunctionCount(index)
                != expectation.count
            || (expectation.separatorIndex >= 0
                && !win32Plugins->isLoadedPluginFunctionSeparator(
                    index, expectation.separatorIndex))) {
            return 110;
        }
    }
    const QString randomGuid = QString::fromUtf8(
        runSelectedCommand(QStringLiteral("Random values"), 2, QByteArray()))
        .trimmed();
    if (!QRegExp(QStringLiteral(
            "^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$"))
             .exactMatch(randomGuid)) {
        return 110;
    }

    wchar_t pluginConfigPath[MAX_PATH]{};
    if (!SendMessageW(win32Plugins->mainWindowHandle(),
                      NppMessageGetPluginConfigDir, MAX_PATH,
                      reinterpret_cast<LPARAM>(pluginConfigPath))
        || !QFileInfo(QDir(QString::fromWCharArray(pluginConfigPath))
                          .filePath(QStringLiteral("converter.ini")))
                .isFile()) {
        return 109;
    }

    auto dockSet = [&] {
        QSet<QDockWidget*> docks;
        for (QDockWidget* dock : mainWindow.findChildren<QDockWidget*>())
            docks.insert(dock);
        return docks;
    };
    auto newDockSince = [&](const QSet<QDockWidget*>& before) {
        for (QDockWidget* dock : mainWindow.findChildren<QDockWidget*>()) {
            if (!before.contains(dock))
                return dock;
        }
        return static_cast<QDockWidget*>(nullptr);
    };

    const int tabsBeforeDemoHello = mainTabs->count();
    QAction* demoHello = pluginAction(
        QStringLiteral("Notepad++ plugin demo"), 0);
    if (!demoHello)
        return 111;
    demoHello->trigger();
    QApplication::processEvents();
    if (mainTabs->count() != tabsBeforeDemoHello + 1
        || mainTabs->editor()->text() != QStringLiteral("Hello, Notepad++!")) {
        return 111;
    }

    if (runSelectedCommand(QStringLiteral("Reverse Lines"), 0,
                           QByteArray("one\r\ntwo\r\nthree"))
            != QByteArray("three\r\ntwo\r\none")) {
        return 72;
    }
    mainTabs->editor()->setText(QStringLiteral("one\r\ntwo\r\nthree"));
    SendMessageW(win32Plugins->mainEditorHandle(), SCI_SETSEL, 2, 2);
    QAction* reverseDocument = pluginAction(QStringLiteral("Reverse Lines"), 1);
    if (!reverseDocument)
        return 72;
    reverseDocument->trigger();
    QApplication::processEvents();
    if (mainTabs->editor()->text() != QStringLiteral("three\r\ntwo\r\none"))
        return 72;

    if (runSelectedCommand(QStringLiteral("Remove Duplicate lines"), 0,
                           QByteArray("alpha\r\nalpha\r\n\r\n\r\nbeta"))
            != QByteArray("alpha\r\n\r\n\r\nbeta")) {
        return 73;
    }

    mainTabs->editor()->setText(QStringLiteral("alpha beta"));
    SendMessageW(win32Plugins->mainEditorHandle(), SCI_SETSEL, 7, 7);
    QAction* selectQuotedText =
        pluginAction(QStringLiteral("SelectQuotedText"), 0);
    if (!selectQuotedText
        || selectQuotedText->shortcut() !=
            QKeySequence(Qt::ALT | Qt::Key_Apostrophe)) {
        return 74;
    }
    selectQuotedText->trigger();
    QApplication::processEvents();
    if (SendMessageW(win32Plugins->mainEditorHandle(), SCI_GETSELECTIONSTART, 0, 0)
            != 6
        || SendMessageW(win32Plugins->mainEditorHandle(), SCI_GETSELECTIONEND, 0, 0)
            != 10) {
        return 74;
    }

    const QByteArray aligned = runSelectedCommand(
        QStringLiteral("Code alignment"), 1,
        QByteArray("a=1\r\nlong=2"));
    QFile alignmentDiagnostic(output + QStringLiteral(
        "/code-alignment-diagnostic.txt"));
    if (alignmentDiagnostic.open(QIODevice::WriteOnly | QIODevice::Text)) {
        alignmentDiagnostic.write(aligned.toHex(' '));
        alignmentDiagnostic.write("\n");
    }
    const QList<QByteArray> alignedLines = aligned.split('\n');
    if (alignedLines.size() != 2
        || alignedLines.at(0).indexOf('=') != alignedLines.at(1).indexOf('=')) {
        qWarning() << "Code Alignment output:" << aligned;
        return 75;
    }
    QAction* alignByDialog = pluginAction(QStringLiteral("Code alignment"), 0);
    QAction* alignByKey = pluginAction(QStringLiteral("Code alignment"), 8);
    if (alignmentDiagnostic.isOpen()) {
        alignmentDiagnostic.write(
            (alignByDialog ? alignByDialog->shortcut().toString()
                           : QStringLiteral("missing")).toUtf8());
        alignmentDiagnostic.write("\n");
        alignmentDiagnostic.write(
            (alignByKey ? alignByKey->shortcut().toString()
                        : QStringLiteral("missing")).toUtf8());
        alignmentDiagnostic.write("\n");
        alignmentDiagnostic.close();
    }
    if (!alignByDialog || !alignByKey
        || alignByDialog->shortcut() !=
            QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Equal)
        || alignByKey->shortcut() !=
            QKeySequence(Qt::CTRL | Qt::Key_Equal)) {
        qWarning() << "Code Alignment shortcuts:"
                   << (alignByDialog ? alignByDialog->shortcut().toString()
                                     : QStringLiteral("missing"))
                   << (alignByKey ? alignByKey->shortcut().toString()
                                  : QStringLiteral("missing"));
        return 75;
    }

    const int bracketsIndex =
        loadedPluginNames.indexOf(QStringLiteral("BracketsCheck"));
    QMenu* bracketsMenu = mainWindow.findChild<QMenu*>(
        QStringLiteral("win32PluginMenu_%1").arg(bracketsIndex));
    QAction* roundBrackets = pluginAction(QStringLiteral("BracketsCheck"), 3);
    if (!bracketsMenu || bracketsMenu->actions().size() != 7
        || !bracketsMenu->actions().at(2)->isSeparator()
        || !roundBrackets || !roundBrackets->isCheckable()) {
        return 76;
    }
    const bool roundBracketsInitiallyChecked = roundBrackets->isChecked();
    roundBrackets->trigger();
    QApplication::processEvents();
    if (roundBrackets->isChecked() == roundBracketsInitiallyChecked)
        return 76;
    roundBrackets->trigger();
    QApplication::processEvents();
    if (roundBrackets->isChecked() != roundBracketsInitiallyChecked)
        return 76;

    auto triggerWithNativeDialog = [&](QAction* action,
                                       const wchar_t* title,
                                       bool fillCryptKey) {
        if (!action)
            return false;
        std::atomic_bool handled{false};
        std::thread dialogWorker([&] {
            for (int attempt = 0; attempt < 500; ++attempt) {
                bool exactMatch = false;
                HWND dialog = findCurrentProcessDialog(title, &exactMatch);
                if (dialog) {
                    if (exactMatch && fillCryptKey) {
                        SetDlgItemTextW(dialog, 1001, L"test-key");
                        SetDlgItemTextW(dialog, 1002, L"test-key");
                        PostMessageW(dialog, WM_COMMAND,
                                     MAKEWPARAM(IDOK, BN_CLICKED),
                                     reinterpret_cast<LPARAM>(
                                         GetDlgItem(dialog, IDOK)));
                    } else {
                        PostMessageW(dialog, WM_CLOSE, 0, 0);
                    }
                    handled = exactMatch;
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        action->trigger();
        dialogWorker.join();
        QApplication::processEvents();
        return handled.load();
    };

    mainTabs->editor()->setText(QStringLiteral("([])"));
    if (!triggerWithNativeDialog(
            pluginAction(QStringLiteral("BracketsCheck"), 0),
            L"Brackets balanced!", false)) {
        return 77;
    }

    auto runSecurePadPair = [&](int encryptCommand, int decryptCommand,
                                bool selectedText) {
        const QByteArray source("secret text");
        mainTabs->editor()->setText(QString::fromUtf8(source));
        if (selectedText) {
            SendMessageW(win32Plugins->mainEditorHandle(), SCI_SETSEL, 0,
                         static_cast<LPARAM>(source.size()));
        }
        if (!triggerWithNativeDialog(
                pluginAction(QStringLiteral("SecurePad"), encryptCommand),
                L"Enter Crypt Key", true)) {
            return false;
        }
        const QByteArray encrypted = mainTabs->editor()->text().toUtf8();
        if (encrypted.isEmpty() || encrypted == source)
            return false;
        if (selectedText) {
            SendMessageW(win32Plugins->mainEditorHandle(), SCI_SETSEL, 0,
                         static_cast<LPARAM>(encrypted.size()));
        }
        return triggerWithNativeDialog(
                   pluginAction(QStringLiteral("SecurePad"), decryptCommand),
                   L"Enter Crypt Key", true)
            && mainTabs->editor()->text().toUtf8() == source;
    };
    if (!runSecurePadPair(2, 3, true)
        || !runSecurePadPair(0, 1, false)) {
        return 78;
    }

    const int jsonViewerIndex =
        loadedPluginNames.indexOf(QStringLiteral("JSON Viewer"));
    if (jsonViewerIndex < 0
        || win32Plugins->loadedPluginFunctionCount(jsonViewerIndex) != 4) {
        return 87;
    }
    const int qtNativeBefore = nativeQtWidgetCount(&mainWindow);
    const QSet<QWidget*> qtNativeWidgetsBefore =
        nativeQtWidgets(&mainWindow);
    const int win32ChildrenBefore =
        descendantWindowCount(win32Plugins->mainWindowHandle());
    mainTabs->editor()->setText(
        QStringLiteral("{\"name\":\"npp\",\"items\":[1,true,null]}"));
    QAction* showJsonViewer = pluginAction(QStringLiteral("JSON Viewer"), 0);
    if (!showJsonViewer)
        return 87;
    showJsonViewer->trigger();
    QApplication::processEvents();

    const QList<Win32NativeDockHost*> jsonHosts =
        mainWindow.findChildren<Win32NativeDockHost*>();
    QFile registrationDiagnostic(
        output + QStringLiteral("/json-viewer-registration.txt"));
    if (registrationDiagnostic.open(
            QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&registrationDiagnostic);
        stream << "hosts=" << jsonHosts.size() << '\n'
               << "adapter-docks="
               << win32Plugins->dockAdapter().dockCount() << '\n'
               << "native-hosts="
               << win32Plugins->dockAdapter().nativeHostCount() << '\n'
               << "promoted-ancestors="
               << win32Plugins->dockAdapter().promotedAncestorCount() << '\n';
        for (QDockWidget* dock : mainWindow.findChildren<QDockWidget*>())
            stream << "dock=" << dock->objectName() << '|'
                   << dock->windowTitle() << '|'
                   << dock->isVisible() << '|'
                   << dock->isFloating() << '\n';
    }
    if (jsonHosts.size() != 1
        || win32Plugins->dockAdapter().dockCount() != 1
        || win32Plugins->dockAdapter().nativeHostCount() != 1
        || win32Plugins->dockAdapter().promotedAncestorCount() != 1) {
        return 88;
    }
    Win32NativeDockHost* jsonHost = jsonHosts.constFirst();
    QDockWidget* jsonDock = qobject_cast<QDockWidget*>(jsonHost->parentWidget());
    if (!jsonDock)
        jsonDock = mainWindow.dockingManager().dockForClient(jsonHost);
    RECT hostRect{};
    RECT clientRect{};
    QFile hostDiagnostic(
        output + QStringLiteral("/json-viewer-native-host.txt"));
    const bool gotHostRect = jsonHost->hostHandle()
        && GetClientRect(jsonHost->hostHandle(), &hostRect);
    const bool gotClientRect = jsonHost->clientHandle()
        && GetWindowRect(jsonHost->clientHandle(), &clientRect);
    if (hostDiagnostic.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&hostDiagnostic);
        stream << "dock-found=" << (jsonDock != nullptr) << '\n'
               << "dock-visible=" << (jsonDock && jsonDock->isVisible()) << '\n'
               << "host=" << reinterpret_cast<quintptr>(jsonHost->hostHandle()) << '\n'
               << "client=" << reinterpret_cast<quintptr>(jsonHost->clientHandle()) << '\n'
               << "client-parent=" << reinterpret_cast<quintptr>(
                      GetParent(jsonHost->clientHandle())) << '\n'
               << "host-size=" << hostRect.right - hostRect.left << 'x'
               << hostRect.bottom - hostRect.top << '\n'
               << "client-size=" << clientRect.right - clientRect.left << 'x'
               << clientRect.bottom - clientRect.top << '\n';
    }
    if (!jsonDock || !jsonDock->isVisible() || !jsonHost->hostHandle()
        || !jsonHost->clientHandle()
        || GetParent(jsonHost->clientHandle()) != jsonHost->hostHandle()
        || !gotHostRect || !gotClientRect
        || hostRect.right - hostRect.left != clientRect.right - clientRect.left
        || hostRect.bottom - hostRect.top != clientRect.bottom - clientRect.top) {
        return 89;
    }
    for (QWidget* ancestor = jsonHost->parentWidget();
         ancestor && ancestor != &mainWindow;
         ancestor = ancestor->parentWidget()) {
        if (ancestor->internalWinId() && ancestor != jsonDock)
            return 89;
    }
    const int qtNativeAfter = nativeQtWidgetCount(&mainWindow);
    const QSet<QWidget*> qtNativeWidgetsAfter =
        nativeQtWidgets(&mainWindow);
    const int win32ChildrenAfter =
        descendantWindowCount(win32Plugins->mainWindowHandle());
    QFile hierarchyDiagnostic(
        output + QStringLiteral("/json-viewer-window-hierarchy.txt"));
    if (hierarchyDiagnostic.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&hierarchyDiagnostic);
        stream << "qt-native-before=" << qtNativeBefore << '\n'
               << "qt-native-after=" << qtNativeAfter << '\n'
               << "qt-native-promoted="
               << qtNativeAfter - qtNativeBefore << '\n'
               << "adapter-promoted-ancestors="
               << win32Plugins->dockAdapter().promotedAncestorCount() << '\n'
               << "win32-descendants-before=" << win32ChildrenBefore << '\n'
               << "win32-descendants-after=" << win32ChildrenAfter << '\n'
               << "win32-descendants-added="
               << win32ChildrenAfter - win32ChildrenBefore << '\n';
        for (QWidget* widget : qtNativeWidgetsAfter - qtNativeWidgetsBefore) {
            stream << "new-qt-native=" << widget->metaObject()->className()
                   << '|' << widget->objectName() << "|parent="
                   << (widget->parentWidget()
                       ? widget->parentWidget()->metaObject()->className()
                       : "null") << '\n';
        }
    }
    // Qt 5 promotes the host's QDockWidget and its two standard title
    // buttons together with the explicitly native host.
    const int expectedQtNativeIncrease = 4;
    if (qtNativeAfter - qtNativeBefore != expectedQtNativeIncrease
        || !jsonDock->grab().save(
            output + QStringLiteral("/json-viewer-dock.png"))) {
        return 90;
    }

    SendMessageW(win32Plugins->mainWindowHandle(), NppMessageDmmHide, 0,
                 reinterpret_cast<LPARAM>(jsonHost->clientHandle()));
    QApplication::processEvents();
    if (jsonDock->isVisible())
        return 91;
    SendMessageW(win32Plugins->mainWindowHandle(), NppMessageDmmShow, 0,
                 reinterpret_cast<LPARAM>(jsonHost->clientHandle()));
    QApplication::processEvents();
    jsonDock->setFloating(true);
    QApplication::processEvents();
    if (!jsonDock->isFloating()
        || GetParent(jsonHost->clientHandle()) != jsonHost->hostHandle()) {
        return 91;
    }
    jsonDock->setFloating(false);
    QApplication::processEvents();

    const QByteArray compactJson(
        "{\"name\":\"npp\",\"items\":[1,true,null]}");
    mainTabs->editor()->setText(QString::fromUtf8(compactJson));
    SendMessageW(win32Plugins->mainEditorHandle(), SCI_SETSEL, 0,
                 compactJson.size());
    QAction* formatJson = pluginAction(QStringLiteral("JSON Viewer"), 1);
    QAction* compressJson = pluginAction(QStringLiteral("JSON Viewer"), 2);
    if (!formatJson || !compressJson)
        return 92;
    formatJson->trigger();
    QApplication::processEvents();
    const QByteArray formattedJson = mainTabs->editor()->text().toUtf8();
    QJsonParseError parseError{};
    const QJsonDocument formattedDocument =
        QJsonDocument::fromJson(formattedJson, &parseError);
    if (parseError.error != QJsonParseError::NoError
        || formattedDocument.isNull() || !formattedJson.contains('\n')
        || mainTabs->editor()->lexerLanguage().compare(
               QStringLiteral("json"), Qt::CaseInsensitive) != 0) {
        return 92;
    }
    SendMessageW(win32Plugins->mainEditorHandle(), SCI_SETSEL, 0,
                 formattedJson.size());
    compressJson->trigger();
    QApplication::processEvents();
    if (QJsonDocument::fromJson(mainTabs->editor()->text().toUtf8()).isNull()
        || mainTabs->editor()->text().toUtf8().contains('\n')) {
        return 92;
    }

    QAction* jsonAbout = pluginAction(QStringLiteral("JSON Viewer"), 3);
    if (!jsonAbout)
        return 93;
    jsonAbout->trigger();
    QApplication::processEvents();
    QMessageBox* jsonAboutDialog = mainWindow.findChild<QMessageBox*>(
        QStringLiteral("jsonViewerAboutDialog"));
    if (!jsonAboutDialog
        || jsonAboutDialog->windowTitle()
            != QStringLiteral("About JSON Viewer")
        || !jsonAboutDialog->text().contains(QStringLiteral("Version: 1.41"))) {
        return 93;
    }
    jsonAboutDialog->close();
    QApplication::processEvents();

    const int jsonToolsIndex =
        loadedPluginNames.indexOf(QStringLiteral("JsonTools"));
    const QStringList expectedJsonToolsItems = {
        QStringLiteral("Documentation"),
        QStringLiteral("Pretty-print current JSON file"),
        QStringLiteral("Compress current JSON file"),
        QStringLiteral("---"),
        QStringLiteral("Open JSON tree viewer"),
        QStringLiteral("---"),
        QStringLiteral("Settings"),
        QStringLiteral("Parse JSON Lines document"),
        QStringLiteral("JSON to YAML"),
        QStringLiteral("Run tests")
    };
    if (jsonToolsIndex < 0
        || win32Plugins->loadedPluginFunctionCount(jsonToolsIndex)
            != expectedJsonToolsItems.size()) {
        return 94;
    }
    for (int index = 0; index < expectedJsonToolsItems.size(); ++index) {
        if (win32Plugins->loadedPluginFunctionName(jsonToolsIndex, index)
                != expectedJsonToolsItems.at(index)) {
            return 94;
        }
    }
    QAction* jsonToolsPretty = pluginAction(QStringLiteral("JsonTools"), 1);
    QAction* jsonToolsCompress = pluginAction(QStringLiteral("JsonTools"), 2);
    QAction* jsonToolsTree = pluginAction(QStringLiteral("JsonTools"), 4);
    QAction* jsonToolsSettings = pluginAction(QStringLiteral("JsonTools"), 6);
    QAction* jsonToolsLines = pluginAction(QStringLiteral("JsonTools"), 7);
    QAction* jsonToolsYaml = pluginAction(QStringLiteral("JsonTools"), 8);
    QAction* jsonToolsTests = pluginAction(QStringLiteral("JsonTools"), 9);
    if (!jsonToolsPretty || !jsonToolsCompress || !jsonToolsTree
        || !jsonToolsSettings || !jsonToolsLines || !jsonToolsYaml
        || !jsonToolsTests
        || jsonToolsPretty->shortcut()
            != QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_P)
        || jsonToolsCompress->shortcut()
            != QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_C)
        || jsonToolsTree->shortcut()
            != QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_J)
        || jsonToolsSettings->shortcut()
            != QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_S)) {
        return 94;
    }

    std::atomic_bool jsonToolsSettingsVerified{false};
    std::thread settingsWorker([&] {
        for (int attempt = 0; attempt < 500; ++attempt) {
            HWND dialog = findCurrentProcessWindow(
                L"Settings - JSON Viewer plug-in");
            if (dialog) {
                HWND ok = findDescendantWindow(dialog, L"&Ok");
                HWND cancel = findDescendantWindow(dialog, L"&Cancel");
                HWND reset = findDescendantWindow(dialog, L"&Reset");
                jsonToolsSettingsVerified = ok && cancel && reset
                    && descendantWindowCount(dialog) >= 6;
                if (reset)
                    SendMessageW(reset, BM_CLICK, 0, 0);
                else
                    PostMessageW(dialog, WM_CLOSE, 0, 0);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    jsonToolsSettings->trigger();
    settingsWorker.join();
    QApplication::processEvents();
    if (!jsonToolsSettingsVerified.load())
        return 100;

    wchar_t currentPath[MAX_PATH]{};
    wchar_t currentName[MAX_PATH]{};
    if (!SendMessageW(win32Plugins->mainWindowHandle(),
                      NppMessageGetFullCurrentPath, 0,
                      reinterpret_cast<LPARAM>(currentPath))
        || !SendMessageW(win32Plugins->mainWindowHandle(),
                         NppMessageGetFileName, 0,
                         reinterpret_cast<LPARAM>(currentName))
        || QString::fromWCharArray(currentPath)
            != QDir::toNativeSeparators(
                mainWindow.currentPathForPlugin())
        || QString::fromWCharArray(currentName)
            != QFileInfo(mainWindow.currentPathForPlugin()).fileName()) {
        return 95;
    }

    const int tabsBeforePluginNew = mainTabs->count();
    if (!SendMessageW(win32Plugins->mainWindowHandle(), NppMessageMenuCommand,
                      0, NppMenuCommandFileNew)
        || mainTabs->count() != tabsBeforePluginNew + 1
        || !mainTabs->editor()->text().isEmpty()) {
        return 95;
    }
    const QString jsonToolsOpenPath =
        output + QStringLiteral("/json-tools-open.json");
    QFile jsonToolsOpenFile(jsonToolsOpenPath);
    if (!jsonToolsOpenFile.open(QIODevice::WriteOnly | QIODevice::Truncate)
        || jsonToolsOpenFile.write("{\"opened\":true}\n") <= 0) {
        return 95;
    }
    jsonToolsOpenFile.close();
    const QString nativeJsonToolsOpenPath =
        QDir::toNativeSeparators(QFileInfo(jsonToolsOpenPath).absoluteFilePath());
    if (!SendMessageW(win32Plugins->mainWindowHandle(), NppMessageDoOpen, 0,
                      reinterpret_cast<LPARAM>(
                          nativeJsonToolsOpenPath.utf16()))
        || QFileInfo(mainWindow.currentFilePath()).canonicalFilePath()
            != QFileInfo(jsonToolsOpenPath).canonicalFilePath()) {
        return 95;
    }

    mainTabs->editor()->setText(QStringLiteral("first\nsecond"));
    SendMessageW(win32Plugins->mainEditorHandle(), SCI_GOTOLINE, 1, 0);
    if (SendMessageW(win32Plugins->mainEditorHandle(), SCI_GETCURRENTPOS, 0, 0)
            != 6) {
        return 96;
    }
    const QByteArray appended("!");
    SendMessageW(win32Plugins->mainEditorHandle(), SCI_APPENDTEXT,
                 appended.size(),
                 reinterpret_cast<LPARAM>(appended.constData()));
    SendMessageW(win32Plugins->mainEditorHandle(), SCI_GOTOPOS, 2, 0);
    if (mainTabs->editor()->text() != QStringLiteral("first\nsecond!")
        || SendMessageW(win32Plugins->mainEditorHandle(), SCI_GETCURRENTPOS,
                        0, 0) != 2) {
        return 96;
    }

    mainTabs->editor()->setText(
        QStringLiteral("{\"name\":\"qt\",\"items\":[1,true,null]}"));
    jsonToolsPretty->trigger();
    QApplication::processEvents();
    const QByteArray jsonToolsFormatted =
        mainTabs->editor()->text().toUtf8();
    QJsonParseError jsonToolsParseError{};
    if (QJsonDocument::fromJson(
            jsonToolsFormatted, &jsonToolsParseError).isNull()
        || jsonToolsParseError.error != QJsonParseError::NoError
        || !jsonToolsFormatted.contains('\n')
        || mainTabs->editor()->lexerLanguage().compare(
               QStringLiteral("json"), Qt::CaseInsensitive) != 0) {
        return 97;
    }
    jsonToolsCompress->trigger();
    QApplication::processEvents();
    if (QJsonDocument::fromJson(mainTabs->editor()->text().toUtf8()).isNull()
        || mainTabs->editor()->text().contains(QLatin1Char('\n'))) {
        return 97;
    }

    mainTabs->editor()->setText(QStringLiteral(
        "{\n  \"bar\": \"qt\",\n  \"foo\": [1, true, null]\n}"));

    const int jsonToolsDockCommandId =
        win32Plugins->loadedPluginFunctionCommandId(jsonToolsIndex, 4);
    const int docksBeforeJsonTools = win32Plugins->dockAdapter().dockCount();
    jsonToolsTree->trigger();
    QApplication::processEvents();
    // JsonTools deliberately uses its function-list index as the dock ID,
    // while menu-check messages use the host-assigned command ID.
    QDockWidget* jsonToolsDock = mainWindow.findChild<QDockWidget*>(
        QStringLiteral("Win32PluginDock_jsontools_4"));
    Win32NativeDockHost* jsonToolsHost = jsonToolsDock
        ? qobject_cast<Win32NativeDockHost*>(jsonToolsDock->widget()) : nullptr;
    QFile jsonToolsDockDiagnostic(
        output + QStringLiteral("/json-tools-tree-dock.txt"));
    if (jsonToolsDockDiagnostic.open(
            QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&jsonToolsDockDiagnostic);
        stream << "command-id=" << jsonToolsDockCommandId << '\n'
               << "dock-count-before=" << docksBeforeJsonTools << '\n'
               << "dock-count-after="
               << win32Plugins->dockAdapter().dockCount() << '\n'
               << "dock-found=" << (jsonToolsDock != nullptr) << '\n'
               << "dock-visible="
               << (jsonToolsDock && jsonToolsDock->isVisible()) << '\n'
               << "host-found=" << (jsonToolsHost != nullptr) << '\n'
               << "action-checkable=" << jsonToolsTree->isCheckable() << '\n'
               << "action-checked=" << jsonToolsTree->isChecked() << '\n';
        for (QDockWidget* dock : mainWindow.findChildren<QDockWidget*>())
            stream << "dock=" << dock->objectName() << '|'
                   << dock->windowTitle() << '|'
                   << dock->isVisible() << '\n';
    }
    RECT jsonToolsClientRect{};
    GetClientRect(jsonToolsHost ? jsonToolsHost->clientHandle() : nullptr,
                  &jsonToolsClientRect);
    if (jsonToolsDockCommandId < 50000
        || !jsonToolsDock || !jsonToolsHost || !jsonToolsDock->isVisible()
        || win32Plugins->dockAdapter().dockCount() != docksBeforeJsonTools + 1
        || !jsonToolsTree->isCheckable() || !jsonToolsTree->isChecked()
        || !jsonToolsHost->clientHandle()
        || !IsWindowVisible(jsonToolsHost->clientHandle())
        || jsonToolsClientRect.right <= jsonToolsClientRect.left
        || jsonToolsClientRect.bottom <= jsonToolsClientRect.top
        || GetParent(jsonToolsHost->clientHandle())
            != jsonToolsHost->hostHandle()) {
        return 98;
    }
    if (!jsonToolsDock->grab().save(
            output + QStringLiteral("/json-tools-tree-dock.png"))) {
        return 99;
    }
    QScreen* jsonToolsScreen = jsonToolsDock->windowHandle()
        ? jsonToolsDock->windowHandle()->screen()
        : QApplication::primaryScreen();
    if (!jsonToolsScreen
        || !jsonToolsScreen->grabWindow(
                reinterpret_cast<WId>(jsonToolsHost->clientHandle()))
                .save(output
                      + QStringLiteral("/json-tools-tree-native-client.png"))) {
        return 99;
    }

    const bool runJsonToolsDeepMatrix =
        QFileInfo(output).fileName()
        == QStringLiteral("ui-localization-runtime-100");
    if (runJsonToolsDeepMatrix) {
    QFile jsonToolsDeepDiagnostic(
        output + QStringLiteral("/json-tools-deep-checkpoints.txt"));
    jsonToolsDeepDiagnostic.open(
        QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    auto jsonToolsCheckpoint = [&](const char* value) {
        if (!jsonToolsDeepDiagnostic.isOpen())
            return;
        jsonToolsDeepDiagnostic.write(value);
        jsonToolsDeepDiagnostic.write("\n");
        jsonToolsDeepDiagnostic.flush();
    };
    jsonToolsCheckpoint("tree-controls");

    HWND jsonToolsTreeControl = findDescendantWindow(
        jsonToolsHost->clientHandle(), nullptr, L"SysTreeView32");
    HWND jsonToolsQuery = findDescendantWindow(
        jsonToolsHost->clientHandle(), nullptr, L"EDIT");
    HWND jsonToolsSubmit = findDescendantWindow(
        jsonToolsHost->clientHandle(), L"Submit query");
    if (!jsonToolsTreeControl || !jsonToolsQuery || !jsonToolsSubmit)
        return 101;
    SetWindowTextW(jsonToolsQuery, L"@.foo");
    wchar_t remesQueryText[256]{};
    GetWindowTextW(jsonToolsQuery, remesQueryText, 256);
    jsonToolsDeepDiagnostic.write("remes-text=");
    jsonToolsDeepDiagnostic.write(
        QString::fromWCharArray(remesQueryText).toUtf8());
    jsonToolsDeepDiagnostic.write("\n");
    jsonToolsDeepDiagnostic.flush();
    jsonToolsCheckpoint("remes-query-before");
    SendMessageW(jsonToolsSubmit, BM_CLICK, 0, 0);
    jsonToolsCheckpoint("remes-query-after");
    QApplication::processEvents();
    if (treeNodeCount(jsonToolsTreeControl) != 4)
        return 101;
    SetWindowTextW(jsonToolsQuery, L"@.bar = `changed`");
    jsonToolsCheckpoint("remes-assignment-before");
    SendMessageW(jsonToolsSubmit, BM_CLICK, 0, 0);
    jsonToolsCheckpoint("remes-assignment-after");
    QApplication::processEvents();
    if (!mainTabs->editor()->text().contains(
            QStringLiteral("\"bar\": \"changed\""))) {
        return 101;
    }

    SendMessageW(win32Plugins->mainEditorHandle(), SCI_GOTOPOS, 0, 0);
    HTREEITEM fooNode = findDirectTreeChild(
        jsonToolsTreeControl, L"foo");
    if (!clickTreeItem(jsonToolsTreeControl, fooNode))
        return 102;
    QApplication::processEvents();
    const LRESULT caretAfterTreeClick = SendMessageW(
        win32Plugins->mainEditorHandle(), SCI_GETCURRENTPOS, 0, 0);
    const LRESULT lineAfterTreeClick = SendMessageW(
        win32Plugins->mainEditorHandle(), SCI_LINEFROMPOSITION,
        caretAfterTreeClick, 0);
    jsonToolsDeepDiagnostic.write(
        QStringLiteral("tree-navigation-line=%1\n")
            .arg(lineAfterTreeClick).toUtf8());
    jsonToolsDeepDiagnostic.flush();
    if (lineAfterTreeClick <= 0) {
        return 102;
    }
    jsonToolsCheckpoint("tree-navigation-after");

    auto currentJsonToolsDocks = [&] {
        QSet<QDockWidget*> docks;
        for (QDockWidget* dock : mainWindow.findChildren<QDockWidget*>()) {
            if (dock->objectName()
                    == QStringLiteral("Win32PluginDock_jsontools_4")) {
                docks.insert(dock);
            }
        }
        return docks;
    };
    auto newlyCreatedJsonToolsDock = [&](const QSet<QDockWidget*>& before) {
        for (QDockWidget* dock : currentJsonToolsDocks()) {
            if (!before.contains(dock))
                return dock;
        }
        return static_cast<QDockWidget*>(nullptr);
    };

    mainTabs->editor()->setText(
        QStringLiteral("{\"row\":1}\n{\"row\":2}\n"));
    const QSet<QDockWidget*> beforeJsonLines = currentJsonToolsDocks();
    jsonToolsLines->trigger();
    jsonToolsCheckpoint("json-lines-after");
    QApplication::processEvents();
    QDockWidget* jsonLinesDock = newlyCreatedJsonToolsDock(beforeJsonLines);
    Win32NativeDockHost* jsonLinesHost = jsonLinesDock
        ? qobject_cast<Win32NativeDockHost*>(jsonLinesDock->widget()) : nullptr;
    HWND jsonLinesTree = jsonLinesHost
        ? findDescendantWindow(jsonLinesHost->clientHandle(), nullptr,
                               L"SysTreeView32")
        : nullptr;
    if (!jsonLinesDock || !jsonLinesDock->isVisible() || !jsonLinesTree
        || treeNodeCount(jsonLinesTree) != 5) {
        return 103;
    }

    mainTabs->editor()->setText(QStringLiteral(
        "{\"name\":\"qt\",\"items\":[1,true,null]}"));
    const int tabsBeforeYaml = mainTabs->count();
    std::atomic_bool yamlWarningAccepted{false};
    std::thread yamlWorker([&] {
        for (int attempt = 0; attempt < 500; ++attempt) {
            HWND dialog = findCurrentProcessWindow(
                L"JSON to YAML feature has some bugs");
            if (dialog) {
                HWND ok = GetDlgItem(dialog, IDOK);
                yamlWarningAccepted = ok != nullptr;
                if (ok)
                    SendMessageW(ok, BM_CLICK, 0, 0);
                else
                    PostMessageW(dialog, WM_CLOSE, 0, 0);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    jsonToolsYaml->trigger();
    jsonToolsCheckpoint("yaml-after");
    yamlWorker.join();
    QApplication::processEvents();
    if (!yamlWarningAccepted.load()
        || mainTabs->count() != tabsBeforeYaml + 1
        || !mainTabs->editor()->text().contains(QStringLiteral("name: qt"))
        || !mainTabs->editor()->text().contains(QStringLiteral("items:"))) {
        return 104;
    }

    QString largeJson = QStringLiteral("{\"outer\":{\"payload\":\"");
    largeJson += QString(4000100, QLatin1Char('a'));
    largeJson += QStringLiteral("\"}}");
    mainTabs->editor()->setText(largeJson);
    const QSet<QDockWidget*> beforeLargeTree = currentJsonToolsDocks();
    jsonToolsTree->trigger();
    jsonToolsCheckpoint("large-partial-after");
    QApplication::processEvents();
    QDockWidget* largeTreeDock = newlyCreatedJsonToolsDock(beforeLargeTree);
    Win32NativeDockHost* largeTreeHost = largeTreeDock
        ? qobject_cast<Win32NativeDockHost*>(largeTreeDock->widget()) : nullptr;
    HWND largeTree = largeTreeHost
        ? findDescendantWindow(largeTreeHost->clientHandle(), nullptr,
                               L"SysTreeView32")
        : nullptr;
    HWND fullTreeCheck = largeTreeHost
        ? findDescendantWindow(largeTreeHost->clientHandle(),
                               L"View all subtrees")
        : nullptr;
    if (!largeTreeDock || !largeTreeDock->isVisible() || !largeTree
        || !fullTreeCheck || treeNodeCount(largeTree) != 2
        || SendMessageW(fullTreeCheck, BM_GETCHECK, 0, 0) != BST_UNCHECKED) {
        return 105;
    }
    std::atomic_bool fullTreeWarningAccepted{false};
    std::thread fullTreeWorker([&] {
        for (int attempt = 0; attempt < 500; ++attempt) {
            HWND dialog = findCurrentProcessWindow(
                L"Loading the full tree could be slow");
            if (dialog) {
                HWND ok = GetDlgItem(dialog, IDOK);
                fullTreeWarningAccepted = ok != nullptr;
                if (ok)
                    SendMessageW(ok, BM_CLICK, 0, 0);
                else
                    PostMessageW(dialog, WM_CLOSE, 0, 0);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    SendMessageW(fullTreeCheck, BM_CLICK, 0, 0);
    jsonToolsCheckpoint("large-full-after");
    fullTreeWorker.join();
    QApplication::processEvents();
    jsonToolsDeepDiagnostic.write(
        QStringLiteral("large-full-warning=%1 checked=%2 nodes=%3\n")
            .arg(fullTreeWarningAccepted.load())
            .arg(SendMessageW(fullTreeCheck, BM_GETCHECK, 0, 0))
            .arg(treeNodeCount(largeTree)).toUtf8());
    jsonToolsDeepDiagnostic.flush();
    if (!fullTreeWarningAccepted.load()
        || treeNodeCount(largeTree) != 3) {
        return 106;
    }
    jsonToolsCheckpoint("large-threshold-passed");
    }

    mainTabs->editor()->setText(QStringLiteral("x"));
    SendMessageW(win32Plugins->mainEditorHandle(), SCI_SETSEL, 1, 1);
    const QSet<QDockWidget*> beforeConverterDock = dockSet();
    QAction* converterPanel = pluginAction(QStringLiteral("Converter"), 3);
    if (!converterPanel)
        return 110;
    converterPanel->trigger();
    QApplication::processEvents();
    QDockWidget* converterDock = newDockSince(beforeConverterDock);
    Win32NativeDockHost* converterHost = converterDock
        ? qobject_cast<Win32NativeDockHost*>(converterDock->widget()) : nullptr;
    HWND converterClient = converterHost ? converterHost->clientHandle() : nullptr;
    HWND decimalEdit = converterClient ? GetDlgItem(converterClient, 2510) : nullptr;
    HWND asciiInsert = converterClient ? GetDlgItem(converterClient, 2516) : nullptr;
    if (!converterDock || !converterDock->isVisible() || !converterClient
        || !decimalEdit || !asciiInsert) {
        return 110;
    }
    SetWindowTextW(decimalEdit, L"65");
    SendMessageW(asciiInsert, BM_CLICK, 0, 0);
    QApplication::processEvents();
    if (mainTabs->editor()->text() != QStringLiteral("xA"))
        return 110;

    mainTabs->editor()->setText(QStringLiteral("one\ntwo\nthree\n"));
    const QSet<QDockWidget*> beforeDemoDock = dockSet();
    QAction* demoDockAction = pluginAction(
        QStringLiteral("Notepad++ plugin demo"), 15);
    if (!demoDockAction)
        return 112;
    demoDockAction->trigger();
    QApplication::processEvents();
    QDockWidget* demoDock = newDockSince(beforeDemoDock);
    Win32NativeDockHost* demoHost = demoDock
        ? qobject_cast<Win32NativeDockHost*>(demoDock->widget()) : nullptr;
    HWND demoClient = demoHost ? demoHost->clientHandle() : nullptr;
    HWND lineEdit = demoClient ? GetDlgItem(demoClient, 2501) : nullptr;
    HWND goButton = demoClient ? GetDlgItem(demoClient, IDOK) : nullptr;
    if (!demoDock || !demoDock->isVisible() || !lineEdit || !goButton)
        return 112;
    SetWindowTextW(lineEdit, L"3");
    SendMessageW(goButton, BM_CLICK, 0, 0);
    QApplication::processEvents();
    const LRESULT demoCaret = SendMessageW(
        win32Plugins->mainEditorHandle(), SCI_GETCURRENTPOS, 0, 0);
    if (SendMessageW(win32Plugins->mainEditorHandle(), SCI_LINEFROMPOSITION,
                     demoCaret, 0) != 2) {
        return 112;
    }
#endif

    QAction* aboutAction = mainWindow.findChild<QAction*>(
        QStringLiteral("win32PluginAction_%1_17")
            .arg(mimeToolsPluginIndex));
    if (!aboutAction)
        return 45;
    aboutAction->trigger();
    QApplication::processEvents();
    QMessageBox* aboutDialog = mainWindow.findChild<QMessageBox*>(
        QStringLiteral("mimeToolsAboutDialog"));
    if (!aboutDialog || aboutDialog->windowTitle() != QStringLiteral("MIME Tools")
        || !aboutDialog->text().contains(QStringLiteral("Version : 2.8"))) {
        return 45;
    }
    aboutDialog->close();
    QApplication::processEvents();

    QAction* splitViewAction = mainWindow.findChild<QAction*>(
        QStringLiteral("splitViewAction"));
    if (!splitViewAction)
        return 41;
    splitViewAction->trigger();
    QApplication::processEvents();
    subTabs->editor()->setFocus(Qt::OtherFocusReason);
    QApplication::processEvents();
    if (mainWindow.currentView() != subTabs->editor()
        || !verifyMimeToolsCommand(
            subTabs->editor(), win32Plugins->secondaryEditorHandle(),
            QByteArray("world"), QByteArray("d29ybGQ"), 1, 0)) {
        return 41;
    }
    mainTabs->editor()->setFocus(Qt::OtherFocusReason);
    QApplication::processEvents();
#endif
#endif
    ScintillaEditView* editor = mainTabs->editor();
    if (!editor)
        return 25;
    if (requestedTheme == QStringLiteral("dark")) {
        if (editor->frameShape() != QFrame::Box
            || editor->frameShadow() != QFrame::Plain) {
            return 26;
        }
    } else if (editor->frameShape() != QFrame::WinPanel
               || editor->frameShadow() != QFrame::Sunken) {
        return 27;
    }
    if (mainTabs->styleSheet().contains(
            QStringLiteral("padding: 2px")) == false) {
        return 28;
    }

    const int lineNumberMarginWidth = editor->marginWidth(0);
    if (lineNumberMarginWidth <= 0)
        return 29;
    const QPoint marginPoint(lineNumberMarginWidth / 2, 20);
    QMouseEvent marginMove(QEvent::MouseMove, marginPoint,
                           Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(editor->viewport(), &marginMove);
    QApplication::processEvents();
    const QCursor marginCursor = editor->viewport()->cursor();
    if (marginCursor.shape() != Qt::BitmapCursor
        || marginCursor.pixmap().isNull()
        || marginCursor.hotSpot().x() <= marginCursor.pixmap().width() / 2) {
        return 30;
    }

    QAction* projectPanelsAction =
        mainWindow.findChild<QAction*>(QStringLiteral("projectPanelsAction"));
    QDockWidget* projectPanelsDock =
        mainWindow.findChild<QDockWidget*>(QStringLiteral("ProjectPanelsDock"));
    if (!projectPanelsAction || !projectPanelsDock)
        return 12;
    const DockingManager& dockingManager = mainWindow.dockingManager();
    if (!dockingManager.isInitialized()
        || dockingManager.dockWidgets().size() < 10
        || dockingManager.dockForClient(projectPanelsDock->widget())
            != projectPanelsDock
        || !dockingManager.getContainerInfo().at(CONT_LEFT).docks.contains(
            projectPanelsDock)) {
        return 86;
    }
    if (!mainWindow.findChild<QDockWidget*>(QStringLiteral("ProjectPanelsDock2"))
        || !mainWindow.findChild<QDockWidget*>(QStringLiteral("ProjectPanelsDock3")))
        return 24;
    projectPanelsAction->trigger();
    QApplication::processEvents();
    if (!projectPanelsDock->isVisible()
        || !mainWindow.grab().save(
            output + QStringLiteral("/main-project-panels.png"))) {
        return 13;
    }
    projectPanelsAction->trigger();

    QAction* shortcutMapperAction =
        mainWindow.findChild<QAction*>(QStringLiteral("shortcutMapperAction"));
    if (!shortcutMapperAction)
        return 14;
    bool shortcutMapperCaptured = false;
    QTimer::singleShot(0, [&]() {
        QWidget* modal = QApplication::activeModalWidget();
        if (!modal)
            return;
        shortcutMapperCaptured =
            modal->grab().save(output + QStringLiteral("/shortcut-mapper.png"));
        QTabWidget* shortcutTabs = modal->findChild<QTabWidget*>(
            QStringLiteral("shortcutMapperTabs"));
        if (!shortcutTabs || shortcutTabs->count() != 5) {
            shortcutMapperCaptured = false;
        } else {
            for (int i = 0; i < shortcutTabs->count(); ++i) {
                shortcutTabs->setCurrentIndex(i);
                QApplication::processEvents();
                const QString name =
                    QStringLiteral("/shortcut-mapper-tab-%1-%2.png")
                        .arg(i, 2, 10, QLatin1Char('0'))
                        .arg(safeName(shortcutTabs->tabText(i)));
                if (!modal->grab().save(output + name)) {
                    shortcutMapperCaptured = false;
                    break;
                }
            }
        }
        if (QDialog* dialog = qobject_cast<QDialog*>(modal))
            dialog->reject();
        else
            modal->close();
    });
    shortcutMapperAction->trigger();
    if (!shortcutMapperCaptured)
        return 15;

    QAction* markDialogAction =
        mainWindow.findChild<QAction*>(QStringLiteral("markDialogAction"));
    if (!markDialogAction)
        return 19;
    markDialogAction->trigger();
    QApplication::processEvents();
    FindReplaceDlg* findDialog = mainWindow.findChild<FindReplaceDlg*>();
    if (!findDialog || !findDialog->isVisible()
        || !findDialog->grab().save(output + QStringLiteral("/find-dialog.png")))
        return 5;
    QTabBar* tabs =
        findDialog->findChild<QTabBar*>(QStringLiteral("tabBar"));
    if (!tabs || tabs->count() != 5 || tabs->currentIndex() != 4)
        return 6;
    if (forceChinese) {
        const QStringList expectedTabs = {
            QStringLiteral("查找"), QStringLiteral("替换"),
            QStringLiteral("文件查找"), QStringLiteral("项目查找"),
            QStringLiteral("标记")
        };
        for (int i = 0; i < expectedTabs.size(); ++i) {
            if (tabs->tabText(i) != expectedTabs.at(i))
                return 20;
        }
        QAbstractButton* markAll =
            findDialog->findChild<QAbstractButton*>(
                QStringLiteral("btnMarkAll"));
        if (findDialog->windowTitle() != QStringLiteral("查找 / 替换")
            || !markAll || markAll->text() != QStringLiteral("全部标记")) {
            return 21;
        }
    }
    for (int i = 0; i < tabs->count(); ++i) {
        tabs->setCurrentIndex(i);
        QApplication::processEvents();
        const QString name = QStringLiteral("find-tab-%1-%2.png")
            .arg(i, 2, 10, QLatin1Char('0'))
            .arg(safeName(tabs->tabText(i)));
        if (!findDialog->grab().save(output + QLatin1Char('/') + name))
            return 7;
    }
    const QString findTitle = findDialog->windowTitle();
    const QSize findSize = findDialog->size();
    findDialog->close();

    QAction* preferencesAction =
        mainWindow.findChild<QAction*>(QStringLiteral("preferencesAction"));
    if (!preferencesAction)
        return 22;
    bool preferencesCaptured = false;
    QString preferencesTitle;
    QSize preferencesSize;
    int preferencesPageCount = 0;
    QTimer::singleShot(0, [&]() {
        PreferenceDlg* preferences =
            qobject_cast<PreferenceDlg*>(QApplication::activeModalWidget());
        if (!preferences)
            return;
        QListWidget* pages =
            preferences->findChild<QListWidget*>(QStringLiteral("pageList"));
        if (!pages || pages->count() != 19) {
            preferences->reject();
            return;
        }
        if (forceChinese
            && (preferences->windowTitle() != QStringLiteral("偏好设置")
                || pages->item(0)->text() != QStringLiteral("通用"))) {
            preferences->reject();
            return;
        }
        if (forceChinese) {
            QAbstractButton* closeButton =
                preferences->findChild<QAbstractButton*>(
                    QStringLiteral("btnPrefsClose"));
            if (!closeButton
                || closeButton->text() != QStringLiteral("关闭")) {
                preferences->reject();
                return;
            }
            QGroupBox* autoInsert =
                preferences->findChild<QGroupBox*>(
                    QStringLiteral("grpAutoInsert"));
            if (!autoInsert
                || autoInsert->title() != QStringLiteral("自动插入")) {
                preferences->reject();
                return;
            }
#ifdef Q_OS_WIN
            QListWidget* categories =
                preferences->findChild<QListWidget*>(
                    QStringLiteral("fileAssociationCategories"));
            QLabel* supported =
                preferences->findChild<QLabel*>(
                    QStringLiteral("lblSupportedExtensions"));
            if (!categories || categories->count() != 10
                || categories->item(0)->text() != QStringLiteral("记事本")
                || categories->item(9)->text() != QStringLiteral("自定义")
                || !supported
                || supported->text() != QStringLiteral("支持的扩展名：")) {
                preferences->reject();
                return;
            }
#endif
        }
        preferencesCaptured = preferences->grab().save(
            output + QStringLiteral("/preferences-dialog.png"));
        for (int i = 0; preferencesCaptured && i < pages->count(); ++i) {
            pages->setCurrentRow(i);
            QApplication::processEvents();
            const QString name =
                QStringLiteral("preferences-page-%1-%2.png")
                    .arg(i, 2, 10, QLatin1Char('0'))
                    .arg(safeName(pages->item(i)->text()));
            preferencesCaptured = preferences->grab().save(
                output + QLatin1Char('/') + name);
        }

        pages->setCurrentRow(13);
        QApplication::processEvents();
        const bool autoInsertCaptured = preferences->grab().save(
            output + QStringLiteral("/preferences-auto-insert.png"));
        preferencesCaptured = preferencesCaptured && autoInsertCaptured;
        preferencesTitle = preferences->windowTitle();
        preferencesSize = preferences->size();
        preferencesPageCount = pages->count();
        preferences->reject();
    });
    preferencesAction->trigger();
    if (!preferencesCaptured)
        return 16;

    QFile metadata(output + QStringLiteral("/metadata.txt"));
    if (!metadata.open(QIODevice::WriteOnly | QIODevice::Text))
        return 11;
    QTextStream stream(&metadata);
    stream << "main.title=" << mainWindow.windowTitle() << '\n'
           << "main.size=" << mainWindow.width() << 'x'
           << mainWindow.height() << '\n'
           << "find.title=" << findTitle << '\n'
           << "find.size=" << findSize.width() << 'x'
           << findSize.height() << '\n'
           << "preferences.title=" << preferencesTitle << '\n'
           << "preferences.size=" << preferencesSize.width() << 'x'
           << preferencesSize.height() << '\n'
           << "ui.font=" << app.font().family() << ','
           << app.font().pointSizeF() << '\n'
           << "find.tabs=" << tabs->count() << '\n'
           << "preferences.pages=" << preferencesPageCount << '\n';
    stream << "ui.theme="
           << (NppParameters::getInstance().getNppGUI()._darkModeEnabled
                   ? "dark" : "light")
           << '\n';

    const int firstBufferIndex = mainTabs->currentIndex();
    const int initialBufferCount = mainTabs->count();
    ScintillaEditView* const permanentMainEditor = mainTabs->editor();
    permanentMainEditor->setText(QStringLiteral("permanent-view-buffer-one"));
    QAction* newAction = mainWindow.findChild<QAction*>(
        QStringLiteral("newAction"));
    if (!newAction)
        return 33;
    newAction->trigger();
    QApplication::processEvents();
    if (mainTabs->count() != initialBufferCount + 1 ||
        mainTabs->editor() != permanentMainEditor)
        return 34;
    const int secondBufferIndex = mainTabs->currentIndex();
    permanentMainEditor->setText(QStringLiteral("permanent-view-buffer-two"));
    mainTabs->setCurrentIndex(firstBufferIndex);
    if (permanentMainEditor->text() !=
        QStringLiteral("permanent-view-buffer-one"))
        return 35;
    mainTabs->setCurrentIndex(secondBufferIndex);
    if (permanentMainEditor->text() !=
        QStringLiteral("permanent-view-buffer-two"))
        return 36;

    return 0;
}
