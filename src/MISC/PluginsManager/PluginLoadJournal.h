#pragma once

#include <QJsonObject>
#include <QString>

class PluginLoadJournal
{
public:
    struct RecoveryEntry {
        QString sessionId;
        QString folderName;
        QString binaryPath;

        bool isValid() const
        {
            return !folderName.isEmpty() && !binaryPath.isEmpty();
        }
    };

    explicit PluginLoadJournal(const QString& stateDirectory);

    bool beginSession(QString* error = nullptr);
    bool beginPlugin(const QString& folderName, const QString& binaryPath,
                     QString* error = nullptr);
    bool completePlugin(const QString& folderName, const QString& binaryPath,
                        const QString& pluginName,
                        const QString& error = QString());
    bool skipRecoveredPlugin(const QString& folderName,
                             const QString& binaryPath);
    bool finishSession(int loadedCount, int failedCount);

    RecoveryEntry recoveryEntry() const { return _recoveryEntry; }
    QString sessionId() const { return _sessionId; }
    QString journalPath() const { return _journalPath; }
    QString markerPath() const { return _markerPath; }

private:
    bool appendEvent(const QString& event, const QJsonObject& details,
                     QString* error = nullptr);
    bool writeMarker(const QString& folderName, const QString& binaryPath,
                     QString* error);
    bool clearMarker(QString* error = nullptr);
    void readRecoveryEntry();
    void rotateJournal();

    QString _stateDirectory;
    QString _journalPath;
    QString _markerPath;
    QString _sessionId;
    RecoveryEntry _recoveryEntry;
};

