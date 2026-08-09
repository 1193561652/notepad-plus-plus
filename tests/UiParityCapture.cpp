#include <QApplication>
#include <QAction>
#include <QAbstractButton>
#include <QDialog>
#include <QDebug>
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
#include <QRegExp>
#include <QScrollArea>
#include <QScrollBar>
#include <QScreen>
#include <QSet>
#include <QTabBar>
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
#include "Parameters.h"
#include "WinControls/Preference/PreferenceDlg.h"
#include "WinControls/TabBar/DocTabView.h"
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
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Notepad++"));
    app.setApplicationVersion(QStringLiteral("8.4.6"));
    app.setOrganizationName(QStringLiteral("Notepad++"));
    app.setOrganizationDomain(QStringLiteral("notepad-plus-plus.org"));
    app.setFont(notepadPlusPlusUiFont());

    if (argc < 2 || argc > 4) {
        qCritical("Usage: ui-parity-capture <output-directory> "
                  "[light|dark|noPlugin|pluginRecovery|registrationRollback] "
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

    const bool forceChinese =
        argc == 4
        && QString::fromLocal8Bit(argv[3]).compare(
               QStringLiteral("zh_CN"), Qt::CaseInsensitive) == 0;
    NppParameters& parameters = NppParameters::getInstance();
    if (forceChinese || noPluginMode || registrationRollbackMode
        || pluginRecoveryMode) {
        const QString settingsPath =
            output + QStringLiteral("/settings");
        if (!QDir().mkpath(settingsPath)
            || !parameters.setUserPathOverride(settingsPath)) {
            return 17;
        }
    }
    if (!parameters.load())
        return 18;
    if (forceChinese)
        parameters.setNativeLang(QStringLiteral("zh_CN"));
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
    if (mainWindow.windowTitle() != QStringLiteral("new 1 - Notepad++"))
        return 23;
    if (mainTabs->tabBar()->count() != 1
        || mainTabs->tabBar()->tabRect(0).width() >=
            mainTabs->tabBar()->width() / 2) {
        return 42;
    }
    if (noPluginMode) {
        QAction* placeholder = mainWindow.findChild<QAction*>(
            QStringLiteral("noPluginsLoadedAction"));
        if (!placeholder || placeholder->isEnabled()
            || !mainWindow.findChildren<QMenu*>(
                    QRegExp(QStringLiteral("win32PluginMenu_.*"))).isEmpty()
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
        using CountFunction = LONG (*)();
        using BufferIdFunction = ULONG_PTR (*)();
        const auto closeCount = probeModule
            ? reinterpret_cast<CountFunction>(
                  GetProcAddress(probeModule, "getFileBeforeCloseCount"))
            : nullptr;
        const auto closedBufferId = probeModule
            ? reinterpret_cast<BufferIdFunction>(
                  GetProcAddress(probeModule, "getLastClosedBufferId"))
            : nullptr;
        QAction* closeAction = mainWindow.findChild<QAction*>(
            QStringLiteral("closeAction"));
        if (!probeModule || !closeCount || !closedBufferId || !closeAction) {
            if (probeModule)
                FreeLibrary(probeModule);
            return 84;
        }
        closeAction->trigger();
        QApplication::processEvents();
        const bool closeNotificationReceived = closeCount() == 1
            && closedBufferId() != 0;
        FreeLibrary(probeModule);
        if (!closeNotificationReceived)
            return 86;
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
        {"NppPluginDemo", "4.2"}
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

    const QStringList expectedSynchronousPlugins = {
        QStringLiteral("BracketsCheck"),
        QStringLiteral("Code alignment"),
        QStringLiteral("Converter"),
        QStringLiteral("Notepad++ plugin demo"),
        QStringLiteral("Reverse Lines"),
        QStringLiteral("Remove Duplicate lines"),
        QStringLiteral("SecurePad"),
        QStringLiteral("SelectQuotedText")
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
    for (const QString& pluginName : expectedSynchronousPlugins) {
        if (!loadedPluginNames.contains(pluginName))
            return 71;
    }
    const QStringList loadErrors =
        mainWindow.property("win32PluginLoadErrors").toStringList();
    if (loadedPluginNames.contains(QStringLiteral("BetterMultiSelection"))
        || loadedPluginNames.contains(
            QStringLiteral("Poor Man's T-Sql Formatter"))
        || !loadErrors.isEmpty()) {
        return 71;
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
                mainWindow.currentPathForWin32Plugin())
        || QString::fromWCharArray(currentName)
            != QFileInfo(mainWindow.currentPathForWin32Plugin()).fileName()) {
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
