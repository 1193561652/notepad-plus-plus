#include "MISC/PluginsManager/PluginLoadJournal.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QUuid>

namespace {

constexpr qint64 MaximumJournalSize = 1024 * 1024;

QString timestamp()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

} // namespace

PluginLoadJournal::PluginLoadJournal(const QString& stateDirectory)
    : _stateDirectory(QDir::cleanPath(stateDirectory)),
      _journalPath(QDir(_stateDirectory).filePath(
          QStringLiteral("plugin-load.jsonl"))),
      _markerPath(QDir(_stateDirectory).filePath(
          QStringLiteral("plugin-load-in-progress.json"))),
      _sessionId(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    readRecoveryEntry();
}

bool PluginLoadJournal::beginSession(QString* error)
{
    if (!QDir().mkpath(_stateDirectory)) {
        if (error)
            *error = QStringLiteral("Could not create plugin state directory");
        return false;
    }
    rotateJournal();
    QJsonObject details;
    details.insert(QStringLiteral("recovery-pending"),
                   _recoveryEntry.isValid());
    if (_recoveryEntry.isValid()) {
        details.insert(QStringLiteral("recovery-folder"),
                       _recoveryEntry.folderName);
        details.insert(QStringLiteral("recovery-session"),
                       _recoveryEntry.sessionId);
    }
    return appendEvent(QStringLiteral("session-start"), details, error);
}

bool PluginLoadJournal::beginPlugin(const QString& folderName,
                                    const QString& binaryPath,
                                    QString* error)
{
    if (!writeMarker(folderName, binaryPath, error))
        return false;
    QJsonObject details;
    details.insert(QStringLiteral("folder"), folderName);
    details.insert(QStringLiteral("path"), binaryPath);
    if (!appendEvent(QStringLiteral("load-start"), details, error)) {
        clearMarker();
        return false;
    }
    return true;
}

bool PluginLoadJournal::completePlugin(const QString& folderName,
                                       const QString& binaryPath,
                                       const QString& pluginName,
                                       const QString& error)
{
    QJsonObject details;
    details.insert(QStringLiteral("folder"), folderName);
    details.insert(QStringLiteral("path"), binaryPath);
    if (!pluginName.isEmpty())
        details.insert(QStringLiteral("plugin"), pluginName);
    if (!error.isEmpty())
        details.insert(QStringLiteral("error"), error);
    const bool logged = appendEvent(
        error.isEmpty() ? QStringLiteral("load-success")
                        : QStringLiteral("load-failure"),
        details);
    return clearMarker() && logged;
}

bool PluginLoadJournal::skipRecoveredPlugin(const QString& folderName,
                                            const QString& binaryPath)
{
    QJsonObject details;
    details.insert(QStringLiteral("folder"), folderName);
    details.insert(QStringLiteral("path"), binaryPath);
    details.insert(QStringLiteral("previous-session"),
                   _recoveryEntry.sessionId);
    const bool logged = appendEvent(QStringLiteral("recovery-skip"), details);
    const bool cleared = clearMarker();
    _recoveryEntry = RecoveryEntry{};
    return logged && cleared;
}

bool PluginLoadJournal::finishSession(int loadedCount, int failedCount)
{
    QJsonObject details;
    details.insert(QStringLiteral("loaded"), loadedCount);
    details.insert(QStringLiteral("failed"), failedCount);
    if (_recoveryEntry.isValid()) {
        details.insert(QStringLiteral("stale-recovery-folder"),
                       _recoveryEntry.folderName);
        clearMarker();
        _recoveryEntry = RecoveryEntry{};
    }
    return appendEvent(QStringLiteral("session-complete"), details);
}

bool PluginLoadJournal::appendEvent(const QString& event,
                                    const QJsonObject& details,
                                    QString* error)
{
    QJsonObject object = details;
    object.insert(QStringLiteral("schema"), 1);
    object.insert(QStringLiteral("timestamp"), timestamp());
    object.insert(QStringLiteral("session"), _sessionId);
    object.insert(QStringLiteral("event"), event);
    QFile file(_journalPath);
    if (!file.open(QFile::WriteOnly | QFile::Append | QFile::Text)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    const QByteArray line = QJsonDocument(object).toJson(
        QJsonDocument::Compact) + '\n';
    if (file.write(line) != line.size() || !file.flush()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}

bool PluginLoadJournal::writeMarker(const QString& folderName,
                                    const QString& binaryPath,
                                    QString* error)
{
    QJsonObject object;
    object.insert(QStringLiteral("schema"), 1);
    object.insert(QStringLiteral("timestamp"), timestamp());
    object.insert(QStringLiteral("session"), _sessionId);
    object.insert(QStringLiteral("folder"), folderName);
    object.insert(QStringLiteral("path"), binaryPath);
    const QByteArray marker =
        QJsonDocument(object).toJson(QJsonDocument::Compact);
    QSaveFile file(_markerPath);
    if (!file.open(QFile::WriteOnly | QFile::Text)
        || file.write(marker) != marker.size()
        || !file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}

bool PluginLoadJournal::clearMarker(QString* error)
{
    if (!QFileInfo::exists(_markerPath) || QFile::remove(_markerPath))
        return true;
    if (error)
        *error = QStringLiteral("Could not remove plugin load marker");
    return false;
}

void PluginLoadJournal::readRecoveryEntry()
{
    QFile file(_markerPath);
    if (!file.open(QFile::ReadOnly | QFile::Text))
        return;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject())
        return;
    const QJsonObject object = document.object();
    if (object.value(QStringLiteral("schema")).toInt() != 1)
        return;
    _recoveryEntry.sessionId =
        object.value(QStringLiteral("session")).toString();
    _recoveryEntry.folderName =
        object.value(QStringLiteral("folder")).toString();
    _recoveryEntry.binaryPath =
        object.value(QStringLiteral("path")).toString();
}

void PluginLoadJournal::rotateJournal()
{
    const QFileInfo info(_journalPath);
    if (!info.isFile() || info.size() <= MaximumJournalSize)
        return;
    const QString previous = _journalPath + QStringLiteral(".1");
    QFile::remove(previous);
    QFile::rename(_journalPath, previous);
}
