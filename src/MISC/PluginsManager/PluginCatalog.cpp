#include "PluginCatalog.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

namespace {

QByteArray resourceBytes(const QString& path)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly))
        return QByteArray();
    return file.readAll();
}

QString jsonString(const QJsonObject& object, const char* name)
{
    const QJsonValue value = object.value(QLatin1String(name));
    return value.isString() ? value.toString() : QString();
}

} // namespace

PluginVersion::PluginVersion(const QString& text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return;

    const QStringList parts = trimmed.split(QLatin1Char('.'));
    if (parts.isEmpty() || parts.size() > 4)
        return;

    for (const QString& part : parts) {
        if (part.isEmpty())
            return;
        bool ok = false;
        const int value = part.toInt(&ok);
        if (!ok || value < 0)
            return;
        _parts.append(value);
    }
    while (_parts.size() < 4)
        _parts.append(0);
    _text = trimmed;
    _valid = true;
}

int PluginVersion::compare(const PluginVersion& other) const
{
    if (!_valid && !other._valid)
        return 0;
    if (!_valid)
        return -1;
    if (!other._valid)
        return 1;
    for (int i = 0; i < 4; ++i) {
        if (_parts.at(i) < other._parts.at(i))
            return -1;
        if (_parts.at(i) > other._parts.at(i))
            return 1;
    }
    return 0;
}

bool PluginVersionInterval::contains(const PluginVersion& version) const
{
    if (!version.isValid())
        return false;
    if (from.isValid() && version < from)
        return false;
    if (to.isValid() && version > to)
        return false;
    return true;
}

bool PluginCatalogEntry::isValid() const
{
    return !folderName.isEmpty() && !displayName.isEmpty() &&
           version.isValid() && !repository.isEmpty() &&
           packageSha256.size() == 64 &&
           QRegularExpression(QStringLiteral("^[0-9A-Fa-f]{64}$"))
               .match(packageSha256)
               .hasMatch();
}

bool PluginCatalogEntry::supportsHost(
    const PluginVersion& hostVersion) const
{
    return hostVersions.contains(hostVersion);
}

bool PluginCatalogEntry::supportsInstalledVersion(
    const PluginVersion& installedVersion,
    const PluginVersion& hostVersion) const
{
    if (!installedVersion.isValid())
        return true;
    if (installedVersion == version)
        return supportsHost(hostVersion);
    if (oldVersionCompatibility.valid &&
        oldVersionCompatibility.pluginVersions.contains(installedVersion)) {
        return oldVersionCompatibility.hostVersions.contains(hostVersion);
    }

    // v8.4.6 only filters an older version when it has an explicit mapping.
    return true;
}

PluginVersionInterval PluginCatalog::parseInterval(const QString& text)
{
    PluginVersionInterval result;
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return result;

    const QRegularExpression expression(
        QStringLiteral("^\\[\\s*([^,\\]]*)\\s*,\\s*([^\\]]*)\\s*\\]$"));
    const QRegularExpressionMatch match = expression.match(trimmed);
    if (match.hasMatch()) {
        result.from = PluginVersion(match.captured(1).trimmed());
        result.to = PluginVersion(match.captured(2).trimmed());
        return result;
    }

    const PluginVersion exact(trimmed);
    if (exact.isValid()) {
        result.from = exact;
        result.to = exact;
    }
    return result;
}

PluginOldVersionCompatibility PluginCatalog::parseOldCompatibility(
    const QString& text)
{
    PluginOldVersionCompatibility result;
    const QRegularExpression expression(
        QStringLiteral("^(\\[[^\\]]*\\])(\\[[^\\]]*\\])$"));
    const QRegularExpressionMatch match = expression.match(text.trimmed());
    if (!match.hasMatch())
        return result;

    result.pluginVersions = parseInterval(match.captured(1));
    result.hostVersions = parseInterval(match.captured(2));
    result.valid = result.pluginVersions.from.isValid() ||
                   result.pluginVersions.to.isValid();
    return result;
}

PluginCatalogEntry PluginCatalog::parseEntry(const QJsonObject& object,
                                             QString* error)
{
    PluginCatalogEntry entry;
    entry.folderName = jsonString(object, "folder-name").trimmed();
    entry.displayName = jsonString(object, "display-name").trimmed();
    entry.author = jsonString(object, "author");
    entry.description = jsonString(object, "description");
    entry.packageSha256 = jsonString(object, "id").trimmed().toLower();
    entry.versionText = jsonString(object, "version").trimmed();
    entry.version = PluginVersion(entry.versionText);
    entry.repository = jsonString(object, "repository").trimmed();
    entry.homepage = jsonString(object, "homepage").trimmed();
    entry.hostVersions = parseInterval(
        jsonString(object, "npp-compatible-versions"));
    entry.oldVersionCompatibility = parseOldCompatibility(
        jsonString(object, "old-versions-compatibility"));

    if (!entry.isValid() && error) {
        *error = QStringLiteral("Invalid plugin catalog entry: %1")
                     .arg(entry.folderName.isEmpty()
                              ? QStringLiteral("<missing folder-name>")
                              : entry.folderName);
    }
    return entry;
}

PluginCatalog PluginCatalog::fromJson(const QByteArray& json, QString* error)
{
    if (error)
        error->clear();
    PluginCatalog catalog;
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError ||
        !document.isObject()) {
        if (error)
            *error = parseError.errorString();
        return catalog;
    }

    const QJsonObject root = document.object();
    catalog._version = jsonString(root, "version").trimmed();
    const QJsonValue pluginsValue = root.value(QStringLiteral("npp-plugins"));
    if (catalog._version.isEmpty() || !pluginsValue.isArray()) {
        if (error)
            *error = QStringLiteral(
                "Plugin catalog requires version and npp-plugins.");
        return catalog;
    }

    QSet<QString> folders;
    for (const QJsonValue& value : pluginsValue.toArray()) {
        // Match v8.4.6: one malformed plugin record is ignored without
        // making the complete catalog unusable.
        if (!value.isObject())
            continue;
        QString entryError;
        const PluginCatalogEntry entry =
            parseEntry(value.toObject(), &entryError);
        const QString key = entry.folderName.toCaseFolded();
        if (!entry.isValid() || folders.contains(key))
            continue;
        folders.insert(key);
        catalog._entries.append(entry);
    }

    catalog._valid = true;
    return catalog;
}

PluginCatalog PluginCatalog::embedded()
{
    return fromJson(embeddedJson());
}

QByteArray PluginCatalog::embeddedJson()
{
    return resourceBytes(QStringLiteral(":/pluginList/catalog.json"));
}

const PluginCatalogEntry* PluginCatalog::findByFolder(
    const QString& folderName) const
{
    for (const PluginCatalogEntry& entry : _entries) {
        if (entry.folderName.compare(folderName, Qt::CaseInsensitive) == 0)
            return &entry;
    }
    return nullptr;
}
