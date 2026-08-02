// PluginManager.h - 插件管理器
// 移植自: v8.4.6:PowerEditor/src/PluginsManager/

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>
#include <QList>
#include <QSet>
#include "IPlugin.h"

class QLibrary;

class PluginManager : public QObject
{
    Q_OBJECT

public:
    explicit PluginManager(QObject* parent = nullptr);
    ~PluginManager();

    // 扫描目录并加载所有插件 DLL
    void loadPlugins(const QString& pluginDir, IPluginHost* host);

    // 卸载所有已加载的插件
    void unloadAll(IPluginHost* host);

    const QList<IPlugin*>& plugins() const { return _plugins; }

private:
    void tryLoad(const QString& filePath, IPluginHost* host);

    QList<IPlugin*>  _plugins;
    QList<QLibrary*> _libs;
    QList<DestroyPluginFn> _destroyFns;
    QSet<QString> _loadedPaths;
};

#endif // PLUGINMANAGER_H
