#include "MISC/PluginsManager/PluginLoadJournal.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}

QStringList readEvents(const QString& path)
{
    QFile file(path);
    require(file.open(QFile::ReadOnly | QFile::Text),
            "could not read plugin load journal");
    QStringList events;
    while (!file.atEnd()) {
        const QByteArray line = file.readLine().trimmed();
        if (line.isEmpty())
            continue;
        QJsonParseError parseError{};
        const QJsonDocument document =
            QJsonDocument::fromJson(line, &parseError);
        require(parseError.error == QJsonParseError::NoError
                    && document.isObject(),
                "journal line is not a JSON object");
        const QJsonObject object = document.object();
        require(object.value(QStringLiteral("schema")).toInt() == 1,
                "journal schema is invalid");
        require(!object.value(QStringLiteral("timestamp")).toString().isEmpty()
                    && !object.value(QStringLiteral("session")).toString().isEmpty(),
                "journal event lacks structured identity fields");
        events.append(object.value(QStringLiteral("event")).toString());
    }
    return events;
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temporary;
    require(temporary.isValid(), "temporary directory is invalid");

    QString journalPath;
    QString markerPath;
    QString crashedSession;
    {
        PluginLoadJournal journal(temporary.path());
        require(journal.beginSession(), "could not begin first session");
        require(journal.beginPlugin(
                    QStringLiteral("CrashPlugin"),
                    QStringLiteral("C:/plugins/CrashPlugin/CrashPlugin.dll")),
                "could not mark plugin loading");
        journalPath = journal.journalPath();
        markerPath = journal.markerPath();
        crashedSession = journal.sessionId();
        require(QFile::exists(markerPath), "load marker was not persisted");
    }

    {
        PluginLoadJournal recovered(temporary.path());
        const PluginLoadJournal::RecoveryEntry entry =
            recovered.recoveryEntry();
        require(entry.isValid()
                    && entry.sessionId == crashedSession
                    && entry.folderName == QStringLiteral("CrashPlugin"),
                "crashed plugin was not recovered");
        require(recovered.beginSession(),
                "could not begin recovery session");
        require(recovered.skipRecoveredPlugin(
                    entry.folderName, entry.binaryPath),
                "could not quarantine recovered plugin");
        require(!QFile::exists(markerPath),
                "recovery did not clear the stale marker");

        require(recovered.beginPlugin(
                    QStringLiteral("InvalidPlugin"),
                    QStringLiteral("C:/plugins/InvalidPlugin/InvalidPlugin.dll")),
                "could not begin failed plugin transaction");
        require(recovered.completePlugin(
                    QStringLiteral("InvalidPlugin"),
                    QStringLiteral("C:/plugins/InvalidPlugin/InvalidPlugin.dll"),
                    QString(), QStringLiteral("invalid function table")),
                "could not record failed plugin transaction");
        require(!QFile::exists(markerPath),
                "ordinary load failure left a recovery marker");
        require(recovered.finishSession(0, 2),
                "could not complete recovery session");
    }

    const QStringList events = readEvents(journalPath);
    const QStringList expected = {
        QStringLiteral("session-start"),
        QStringLiteral("load-start"),
        QStringLiteral("session-start"),
        QStringLiteral("recovery-skip"),
        QStringLiteral("load-start"),
        QStringLiteral("load-failure"),
        QStringLiteral("session-complete")
    };
    require(events == expected, "plugin load event sequence is invalid");
    return 0;
}

