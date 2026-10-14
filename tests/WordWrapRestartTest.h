#pragma once

#include <QTemporaryDir>
#include <QToolBar>
#include <QToolButton>

// Separate processes exercise config.xml persistence rather than singleton
// state surviving inside one test process. Always use disposable settings.
static int runWordWrapRestartPhase(const QString &settings, int phase)
{
    auto &parameters = NppParameters::getInstance();
    if (!parameters.setUserPathOverride(settings) || !parameters.load())
        return 1;
    CommandLineOptions options;
    options.noPlugin = true;
    options.noSession = true;
    MainWindow window(options);
    window.show();
    QApplication::processEvents();
    auto *action = window.findChild<QAction *>("wordWrapAction");
    auto *mainTabs = window.findChild<DocTabView *>("MainDocTab");
    auto *subTabs = window.findChild<DocTabView *>("SubDocTab");
    if (!action || !mainTabs || !subTabs)
        return 2;
    const auto matches = [&](bool expected) {
        return parameters.getSVP()._doWrap == expected
            && action->isChecked() == expected
            && (mainTabs->editor()->wrapMode() != WrapNone) == expected
            && (subTabs->editor()->wrapMode() != WrapNone) == expected;
    };
    if (!matches(phase == 1)) {
        qCritical() << "Word wrap was not restored on startup, phase" << phase;
        return 3;
    }
    if (phase == 0) {
        QToolButton *button = nullptr;
        for (auto *toolbar : window.findChildren<QToolBar *>()) {
            if (toolbar->actions().contains(action)) {
                button = qobject_cast<QToolButton *>(toolbar->widgetForAction(action));
                if (button) break;
            }
        }
        if (!button) return 4;
        button->click();
        if (!matches(true)) return 5;
    } else if (phase == 1) {
        bool inMenu = false;
        for (auto *menu : window.findChildren<QMenu *>())
            inMenu |= menu->actions().contains(action);
        if (!inMenu) return 6;
        action->trigger();
        if (!matches(false)) return 7;
    }
    // Exercise the production shutdown writer, not a test-specific save call.
    return window.close() ? 0 : 8;
}

static int runWordWrapRestartTest()
{
    QTemporaryDir settings;
    if (!settings.isValid()) return 1;
    for (int phase = 0; phase < 3; ++phase) {
        QProcess child;
        child.setProcessChannelMode(QProcess::MergedChannels);
        child.start(QCoreApplication::applicationFilePath(),
                    {"--word-wrap-phase", settings.path(), QString::number(phase)});
        if (!child.waitForStarted() || !child.waitForFinished(30000)
            || child.exitStatus() != QProcess::NormalExit || child.exitCode() != 0) {
            qCritical() << "Word wrap restart phase" << phase << "failed:"
                        << child.exitCode() << child.readAll();
            return 2;
        }
    }
    return 0;
}
