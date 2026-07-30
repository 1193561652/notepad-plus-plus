#pragma once

#include "PluginCatalog.h"

#include <QString>
#include <QVector>

struct PluginAdminItem
{
    QString folderName;
    QString displayName;
    QString description;
    QString author;
    QString homepage;
    QString repository;
    QString packageSha256;
    QString binaryPath;
    PluginVersion installedVersion;
    PluginVersion availableVersion;
    bool installed = false;
    bool available = false;
    bool updateAvailable = false;
    bool incompatible = false;
    bool managedInstall = false;
};

class PluginAdminModel
{
public:
    PluginAdminModel(const QString& pluginRoot,
                     const PluginCatalog& catalog,
                     const PluginVersion& hostVersion);

    void refresh();

    QString pluginRoot() const { return _pluginRoot; }
    QString catalogVersion() const { return _catalog.version(); }
    const QVector<PluginAdminItem>& items() const { return _items; }

    QVector<PluginAdminItem> availableItems() const;
    QVector<PluginAdminItem> updateItems() const;
    QVector<PluginAdminItem> installedItems() const;
    QVector<PluginAdminItem> incompatibleItems() const;

    static QString installReceiptPath(const QString& pluginRoot,
                                      const QString& folderName);

private:
    PluginVersion installedVersion(const QString& folderName,
                                   const QString& binaryPath,
                                   bool* managed) const;

    QString _pluginRoot;
    PluginCatalog _catalog;
    PluginVersion _hostVersion;
    QVector<PluginAdminItem> _items;
};
