#pragma once

#include <QInputMethodEvent>
#include <QTemporaryDir>

static int runImeSessionPhase(const QString &settings, int phase)
{
    auto &parameters = NppParameters::getInstance();
    if (!parameters.setUserPathOverride(settings) || !parameters.load()) return 1;
    parameters.getNppGUI()._isSnapshotMode = true;
    parameters.getNppGUI()._rememberLastSession = true;
    CommandLineOptions options;
    options.noPlugin = true;
    MainWindow window(options);
    window.show();
    QApplication::processEvents();
    auto *tabs = window.findChild<DocTabView *>("MainDocTab");
    if (!tabs || !tabs->currentBuffer()) return 2;
    auto *editor = tabs->editor();
    auto *buffer = tabs->currentBuffer();
    if (phase == 1 || phase == 3) {
        const QString expected = phase == 1 ? QString() : QStringLiteral("keep");
        if (editor->text() != expected || buffer->isDirty() != (phase == 3)) {
            qCritical() << "Unexpected session text/dirty state" << editor->text()
                        << buffer->isDirty();
            return 3;
        }
        return window.close() ? 0 : 4;
    }
    if (phase == 2)
        editor->sends(SCI_ADDTEXT, 4, "keep");
    QInputMethodEvent preedit(QString::fromUtf8("年"),
        {{QInputMethodEvent::Cursor, 1, 1, QVariant()}});
    QApplication::sendEvent(editor, &preedit);
    if (!QMetaObject::invokeMethod(&window, "onBackupTimer", Qt::DirectConnection))
        return 5;
    const QString backup = buffer->getBackupFilePath();
    if (backup.isEmpty() || !QFile::exists(backup)) return 6;
    QInputMethodEvent cancel;
    QApplication::sendEvent(editor, &cancel);
    const QString expected = phase == 0 ? QString() : QStringLiteral("keep");
    if (editor->text() != expected || buffer->isDirty() != (phase == 2)) return 7;
    // Closing runs the real snapshot/session writer after cancellation.
    if (!window.close()) return 8;
    if (phase == 0 && (QFile::exists(backup) || !buffer->getBackupFilePath().isEmpty())) {
        qCritical() << "Cancelled preedit left a stale session backup" << backup;
        return 9;
    }
    return 0;
}

static int runImeSessionRestartTest()
{
    QTemporaryDir settings;
    if (!settings.isValid()) return 1;
    for (int phase = 0; phase < 4; ++phase) {
        QProcess child;
        child.setProcessChannelMode(QProcess::MergedChannels);
        child.start(QCoreApplication::applicationFilePath(),
                    {"--ime-session-phase", settings.path(), QString::number(phase)});
        if (!child.waitForStarted() || !child.waitForFinished(30000)
            || child.exitStatus() != QProcess::NormalExit || child.exitCode() != 0) {
            qCritical() << "IME session phase" << phase << "failed:"
                        << child.exitCode() << child.readAll();
            return 2;
        }
    }
    return 0;
}
