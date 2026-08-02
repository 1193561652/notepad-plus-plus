#include <QApplication>
#include <QAction>
#include <QAbstractButton>
#include <QDialog>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QMouseEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegExp>
#include <QScrollArea>
#include <QScrollBar>
#include <QTabBar>
#include <QTextStream>
#include <QTimer>

#include "MainWindow.h"
#include "MISC/UiFont.h"
#include "Parameters.h"
#include "WinControls/Preference/PreferenceDlg.h"
#include "WinControls/TabBar/DocTabView.h"
#include "ScintillaComponent/FindReplaceDlg.h"
#include "ScintillaComponent/ScintillaEditView.h"
#ifdef Q_OS_WIN
#include "Win32PluginSystem/Win32PluginManager.h"
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
                  "[light|dark] [zh_CN]");
        return 2;
    }

    const QString output = QDir::cleanPath(QString::fromLocal8Bit(argv[1]));
    if (!QDir().mkpath(output))
        return 3;

    const bool forceChinese =
        argc == 4
        && QString::fromLocal8Bit(argv[3]).compare(
               QStringLiteral("zh_CN"), Qt::CaseInsensitive) == 0;
    NppParameters& parameters = NppParameters::getInstance();
    if (forceChinese) {
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

    MainWindow mainWindow;
    mainWindow.resize(1100, 760);
    if (!saveWidget(mainWindow, output + QStringLiteral("/main.png")))
        return 4;
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
    if (receiptObject.value(QStringLiteral("folder-name")).toString() !=
            QStringLiteral("mimeTools")
        || receiptObject.value(QStringLiteral("version")).toString() !=
            QStringLiteral("2.8")
        || win32Plugins->loadedPluginCount() != 1
        || win32Plugins->loadedPluginNames() !=
            QStringList{QStringLiteral("MIME Tools")}
        || win32Plugins->loadedPluginFunctionCount(0) <= 0) {
        return 39;
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
        QString commandError;
        return win32Plugins->executePluginCommand(
                   0, commandIndex, &commandError)
            && editorView->text().toUtf8() == expected;
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

    subTabs->show();
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
