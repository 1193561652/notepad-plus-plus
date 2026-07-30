#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

class QJsonObject;

class PluginVersion
{
public:
    PluginVersion() = default;
    explicit PluginVersion(const QString& text);

    bool isValid() const { return _valid; }
    QString toString() const { return _text; }

    int compare(const PluginVersion& other) const;

    bool operator==(const PluginVersion& other) const
    {
        return compare(other) == 0;
    }
    bool operator<(const PluginVersion& other) const
    {
        return compare(other) < 0;
    }
    bool operator>(const PluginVersion& other) const
    {
        return compare(other) > 0;
    }

private:
    QVector<int> _parts;
    QString _text;
    bool _valid = false;
};

struct PluginVersionInterval
{
    PluginVersion from;
    PluginVersion to;

    bool contains(const PluginVersion& version) const;
};

struct PluginOldVersionCompatibility
{
    PluginVersionInterval pluginVersions;
    PluginVersionInterval hostVersions;
    bool valid = false;
};

struct PluginCatalogEntry
{
    QString folderName;
    QString displayName;
    QString author;
    QString description;
    QString packageSha256;
    QString versionText;
    QString repository;
    QString homepage;
    PluginVersion version;
    PluginVersionInterval hostVersions;
    PluginOldVersionCompatibility oldVersionCompatibility;

    bool isValid() const;
    bool supportsHost(const PluginVersion& hostVersion) const;
    bool supportsInstalledVersion(const PluginVersion& installedVersion,
                                  const PluginVersion& hostVersion) const;
};

class PluginCatalog
{
public:
    static PluginCatalog fromJson(const QByteArray& json,
                                  QString* error = nullptr);
    static PluginCatalog embedded();
    static QByteArray embeddedJson();

    bool isValid() const { return _valid; }
    QString version() const { return _version; }
    const QVector<PluginCatalogEntry>& entries() const { return _entries; }
    const PluginCatalogEntry* findByFolder(const QString& folderName) const;

private:
    static PluginVersionInterval parseInterval(const QString& text);
    static PluginOldVersionCompatibility parseOldCompatibility(
        const QString& text);
    static PluginCatalogEntry parseEntry(const QJsonObject& object,
                                         QString* error);

    QString _version;
    QVector<PluginCatalogEntry> _entries;
    bool _valid = false;
};
