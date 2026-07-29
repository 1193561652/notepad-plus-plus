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
#include <QRegExp>
#include <QScrollArea>
#include <QScrollBar>
#include <QTabBar>
#include <QTextStream>
#include <QTimer>

#include "MainWindow.h"
#include "MISC/UiFont.h"
#include "Parameters.h"
#include "Preferences/PreferenceDlg.h"
#include "ScintillaComponent/FindReplaceDlg.h"

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

    QAction* projectPanelsAction =
        mainWindow.findChild<QAction*>(QStringLiteral("projectPanelsAction"));
    QDockWidget* projectPanelsDock =
        mainWindow.findChild<QDockWidget*>(QStringLiteral("ProjectPanelsDock"));
    if (!projectPanelsAction || !projectPanelsDock)
        return 12;
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
        if (!shortcutTabs || shortcutTabs->count() != 4) {
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
        bool autoInsertCaptured = false;
        const QList<QScrollArea*> scrollAreas =
            preferences->findChildren<QScrollArea*>();
        for (QScrollArea* area : scrollAreas) {
            if (!area->isVisible()
                || area->verticalScrollBar()->maximum() <= 0) {
                continue;
            }
            area->verticalScrollBar()->setValue(
                area->verticalScrollBar()->maximum());
            QApplication::processEvents();
            autoInsertCaptured = preferences->grab().save(
                output + QStringLiteral("/preferences-auto-insert.png"));
            break;
        }
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

    return 0;
}
