// PluginManager.cpp - 插件管理器实现

#include "PluginManager.h"
#include "PluginArtifactResolver.h"
#include <QLibrary>
#include <QFileInfo>
#include <QDebug>

PluginManager::PluginManager(QObject* parent)
    : QObject(parent)
{
}

PluginManager::~PluginManager()
{
    // 资源已在 unloadAll() 或析构时释放
    qDeleteAll(_libs);
}

void PluginManager::loadPlugins(
    const QString& pluginDir, IPluginHost* host,
    const QStringList& enabledFolders)
{
    QSet<QString> enabled;
    for (const QString& folder : enabledFolders)
        enabled.insert(folder.toCaseFolded());
    const QVector<PluginArtifact> artifacts =
        PluginArtifactResolver::discover(pluginDir);
    for (const PluginArtifact& artifact : artifacts) {
        if (!enabled.contains(artifact.folderName.toCaseFolded()))
            continue;
        const QString canonicalPath =
            QFileInfo(artifact.binaryPath).canonicalFilePath();
        if (!_loadedPaths.contains(canonicalPath))
            tryLoad(canonicalPath, host);
    }
}

void PluginManager::tryLoad(const QString& filePath, IPluginHost* host)
{
    QLibrary* lib = new QLibrary(filePath, this);
    if (!lib->load()) {
        qWarning() << "Plugin load failed:" << filePath << lib->errorString();
        delete lib;
        return;
    }

    auto createFn  = reinterpret_cast<CreatePluginFn>(lib->resolve("createPlugin"));
    auto destroyFn = reinterpret_cast<DestroyPluginFn>(lib->resolve("destroyPlugin"));

    if (!createFn || !destroyFn) {
        qWarning() << "Plugin missing export functions:" << filePath;
        lib->unload();
        delete lib;
        return;
    }

    IPlugin* plugin = createFn();
    if (!plugin) {
        qWarning() << "createPlugin() returned null:" << filePath;
        lib->unload();
        delete lib;
        return;
    }

    plugin->init(host);
    _plugins.append(plugin);
    _libs.append(lib);
    _destroyFns.append(destroyFn);
    _loadedPaths.insert(QFileInfo(filePath).canonicalFilePath());

    qDebug() << "Plugin loaded:" << plugin->getName() << plugin->getVersion();
}

void PluginManager::unloadAll(IPluginHost* host)
{
    Q_UNUSED(host)
    for (int i = 0; i < _plugins.size(); ++i) {
        _plugins[i]->cleanup();
        _destroyFns[i](_plugins[i]);
        _libs[i]->unload();
    }
    _plugins.clear();
    _destroyFns.clear();
    qDeleteAll(_libs);
    _libs.clear();
    _loadedPaths.clear();
}
