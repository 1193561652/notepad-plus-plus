#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

class PluginEnablementConfig
{
public:
    explicit PluginEnablementConfig(const QString& filePath = QString());

    static QString filePathForConfigDirectory(const QString& directory);

    QString filePath() const { return _filePath; }
    bool load(QString* error = nullptr);
    bool save(QString* error = nullptr) const;

    bool isEnabled(const QString& folderName) const;
    QStringList enabledPlugins() const;
    bool setEnabled(const QString& folderName, bool enabled);

private:
    struct Entry
    {
        QString folderName;
        bool enabled = false;
    };

    static QString keyForFolder(const QString& folderName);
    static bool isValidFolderName(const QString& folderName);

    QString _filePath;
    QMap<QString, Entry> _entries;
};
