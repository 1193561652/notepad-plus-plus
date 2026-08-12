#include "PluginAdminModel.h"
#include "MISC/PluginsManager/PluginArtifactResolver.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

#if defined(Q_OS_WIN)
#include <windows.h>
#include <winver.h>
#endif

namespace {

QString fileVersion(const QString& path)
{
#if defined(Q_OS_WIN)
    const std::wstring nativePath = QDir::toNativeSeparators(path).toStdWString();
    DWORD ignored = 0;
    const DWORD size = GetFileVersionInfoSizeW(nativePath.c_str(), &ignored);
    if (size == 0)
        return QString();

    QByteArray data(static_cast<int>(size), '\0');
    if (!GetFileVersionInfoW(nativePath.c_str(), 0, size, data.data()))
        return QString();

    VS_FIXEDFILEINFO* info = nullptr;
    UINT infoSize = 0;
    if (!VerQueryValueW(data.data(), L"\\",
                        reinterpret_cast<void**>(&info), &infoSize) ||
        !info || infoSize < sizeof(VS_FIXEDFILEINFO)) {
        return QString();
    }
    return QStringLiteral("%1.%2.%3.%4")
        .arg(HIWORD(info->dwFileVersionMS))
        .arg(LOWORD(info->dwFileVersionMS))
        .arg(HIWORD(info->dwFileVersionLS))
        .arg(LOWORD(info->dwFileVersionLS));
#else
    Q_UNUSED(path)
    return QString();
#endif
}

PluginAdminItem catalogItem(const PluginCatalogEntry& entry)
{
    PluginAdminItem item;
    item.folderName = entry.folderName;
    item.displayName = entry.displayName;
    item.description = entry.description;
    item.author = entry.author;
    item.homepage = entry.homepage;
    item.repository = entry.repository;
    item.packageSha256 = entry.packageSha256;
    item.availableVersion = entry.version;
    return item;
}

} // namespace

PluginAdminModel::PluginAdminModel(const QString& pluginRoot,
                                   const PluginCatalog& catalog,
                                   const PluginVersion& hostVersion,
                                   const QString& enablementFilePath)
    : _pluginRoot(QDir::cleanPath(pluginRoot)),
      _catalog(catalog),
      _hostVersion(hostVersion),
      _enablement(enablementFilePath)
{
    _enablement.load();
    refresh();
}

QString PluginAdminModel::installReceiptPath(const QString& pluginRoot,
                                             const QString& folderName)
{
    return QDir(QDir(pluginRoot).filePath(folderName))
        .filePath(QStringLiteral(".npp-package.json"));
}

PluginVersion PluginAdminModel::installedVersion(
    const QString& folderName, const QString& binaryPath, bool* managed) const
{
    if (managed)
        *managed = false;

    QFile receipt(installReceiptPath(_pluginRoot, folderName));
    if (receipt.open(QFile::ReadOnly)) {
        const QJsonDocument document = QJsonDocument::fromJson(receipt.readAll());
        const QString version =
            document.object().value(QStringLiteral("version")).toString();
        const PluginVersion parsed(version);
        if (parsed.isValid()) {
            if (managed)
                *managed = true;
            return parsed;
        }
    }

    return PluginVersion(fileVersion(binaryPath));
}

void PluginAdminModel::refresh()
{
    _items.clear();

    QSet<QString> installedFolders;
    const QVector<PluginArtifact> artifacts =
        PluginArtifactResolver::discover(_pluginRoot);
    for (const PluginArtifact& artifact : artifacts) {
        const QString folderName = artifact.folderName;
        const QString binaryPath = artifact.binaryPath;

        installedFolders.insert(folderName.toCaseFolded());
        const PluginCatalogEntry* catalogEntry =
            _catalog.findByFolder(folderName);
        PluginAdminItem item =
            catalogEntry ? catalogItem(*catalogEntry) : PluginAdminItem();
        if (!catalogEntry) {
            item.folderName = folderName;
            item.displayName = folderName;
        }
        item.installed = true;
        item.enabled = _enablement.isEnabled(folderName);
        item.binaryPath = binaryPath;
        item.installedVersion =
            installedVersion(folderName, binaryPath, &item.managedInstall);
        if (catalogEntry) {
            item.incompatible = !catalogEntry->supportsInstalledVersion(
                item.installedVersion, _hostVersion);
            item.updateAvailable =
                catalogEntry->supportsHost(_hostVersion) &&
                item.installedVersion.isValid() &&
                catalogEntry->version > item.installedVersion;
        }
        _items.append(item);
    }

    for (const PluginCatalogEntry& entry : _catalog.entries()) {
        if (installedFolders.contains(entry.folderName.toCaseFolded()))
            continue;
        if (!entry.supportsHost(_hostVersion))
            continue;
        PluginAdminItem item = catalogItem(entry);
        item.available = true;
        _items.append(item);
    }
}

QVector<PluginAdminItem> PluginAdminModel::availableItems() const
{
    QVector<PluginAdminItem> result;
    for (const PluginAdminItem& item : _items) {
        if (item.available)
            result.append(item);
    }
    return result;
}

QVector<PluginAdminItem> PluginAdminModel::updateItems() const
{
    QVector<PluginAdminItem> result;
    for (const PluginAdminItem& item : _items) {
        if (item.updateAvailable)
            result.append(item);
    }
    return result;
}

QVector<PluginAdminItem> PluginAdminModel::installedItems() const
{
    QVector<PluginAdminItem> result;
    for (const PluginAdminItem& item : _items) {
        if (item.installed)
            result.append(item);
    }
    return result;
}

QVector<PluginAdminItem> PluginAdminModel::incompatibleItems() const
{
    QVector<PluginAdminItem> result;
    for (const PluginAdminItem& item : _items) {
        if (item.incompatible)
            result.append(item);
    }
    return result;
}

bool PluginAdminModel::setPluginEnabled(
    const QString& folderName, bool enabled, QString* error)
{
    if (error)
        error->clear();
    const bool previous = _enablement.isEnabled(folderName);
    if (!_enablement.setEnabled(folderName, enabled)) {
        if (error)
            *error = QStringLiteral("Invalid plugin folder name");
        return false;
    }
    if (!_enablement.save(error)) {
        _enablement.setEnabled(folderName, previous);
        return false;
    }
    for (PluginAdminItem& item : _items) {
        if (item.folderName.compare(folderName, Qt::CaseInsensitive) == 0)
            item.enabled = enabled;
    }
    return true;
}
