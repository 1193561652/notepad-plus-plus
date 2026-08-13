#include "MISC/PluginsManager/PluginManager.h"

#include <QDebug>
#include <QFileInfo>
#include <QLibrary>

#include <cstring>

#include "MISC/PluginsManager/PluginArtifactResolver.h"
#include "MISC/PluginsManager/PluginHostServices.h"

namespace {

constexpr uint32_t MaximumPluginFunctions = 4096;

QString loadError(const QString& filePath, const QString& detail)
{
    return QStringLiteral("%1: %2").arg(filePath, detail);
}

bool hasValidFunction(const NppPluginFuncItem& item)
{
    return item.struct_size == sizeof(NppPluginFuncItem)
        && item.item_name_utf8 && item.item_name_utf8[0];
}

} // namespace

PluginManager::PluginManager(PluginHostServices* hostServices, QObject* parent)
    : QObject(parent), _hostServices(hostServices)
{
}

PluginManager::~PluginManager()
{
    unloadAll();
}

bool PluginManager::loadPlugin(const QString& filePath,
                               const QString& folderName,
                               QString* errorMessage)
{
    const QString canonicalPath = QFileInfo(filePath).canonicalFilePath();
    if (canonicalPath.isEmpty() || _loadedPaths.contains(canonicalPath))
        return false;

    QLibrary* library = new QLibrary(canonicalPath, this);
    if (!library->load()) {
        if (errorMessage)
            *errorMessage = loadError(canonicalPath, library->errorString());
        delete library;
        return false;
    }

    const auto getAbiVersion = reinterpret_cast<NppGetPluginAbiVersionFn>(
        library->resolve("nppGetPluginAbiVersion"));
    const auto getName = reinterpret_cast<NppGetNameFn>(
        library->resolve("nppGetName"));
    const auto setInfo = reinterpret_cast<NppSetInfoFn>(
        library->resolve("nppSetInfo"));
    const auto getFuncsArray = reinterpret_cast<NppGetFuncsArrayFn>(
        library->resolve("nppGetFuncsArray"));
    const auto beNotified = reinterpret_cast<NppBeNotifiedFn>(
        library->resolve("nppBeNotified"));
    const auto messageProc = reinterpret_cast<NppMessageProcFn>(
        library->resolve("nppMessageProc"));
    if (!getAbiVersion || !getName || !setInfo || !getFuncsArray
        || !beNotified || !messageProc) {
        if (errorMessage) {
            *errorMessage = loadError(
                canonicalPath, QStringLiteral("missing ABI v1 exports"));
        }
        library->unload();
        delete library;
        return false;
    }

    if (getAbiVersion() != NPP_PLUGIN_ABI_VERSION) {
        if (errorMessage) {
            *errorMessage = loadError(
                canonicalPath, QStringLiteral("unsupported plugin ABI version"));
        }
        library->unload();
        delete library;
        return false;
    }

    LoadedPlugin* plugin = new LoadedPlugin;
    plugin->library = library;
    plugin->filePath = canonicalPath;
    plugin->folderName = folderName.isEmpty()
        ? QFileInfo(canonicalPath).completeBaseName() : folderName;
    plugin->name = QString::fromUtf8(getName());
    if (plugin->name.isEmpty()) {
        if (errorMessage)
            *errorMessage = loadError(canonicalPath, QStringLiteral("empty plugin name"));
        library->unload();
        delete library;
        delete plugin;
        return false;
    }

    const PluginHostEnvironment environment = _hostServices
        ? _hostServices->environment() : PluginHostEnvironment{};
    plugin->systemName = environment.systemName.toUtf8();
    plugin->systemVersion = environment.systemVersion.toUtf8();
    plugin->applicationName = environment.applicationName.toUtf8();
    plugin->applicationVersion = environment.applicationVersion.toUtf8();
    plugin->pluginHomePath = _hostServices
        ? _hostServices->pluginHomePath().toUtf8() : QByteArray();
    plugin->pluginConfigPath = _hostServices
        ? _hostServices->pluginConfigPath().toUtf8() : QByteArray();
    plugin->hostInfo.struct_size = sizeof(NppPluginHostInfo);
    plugin->hostInfo.abi_version = NPP_PLUGIN_ABI_VERSION;
    plugin->hostInfo.host_context = _hostServices;
    plugin->hostInfo.system_type = static_cast<uint32_t>(environment.systemType);
    plugin->hostInfo.cpu_architecture =
        static_cast<uint32_t>(environment.cpuArchitecture);
    plugin->hostInfo.system_name_utf8 = plugin->systemName.constData();
    plugin->hostInfo.system_version_utf8 = plugin->systemVersion.constData();
    plugin->hostInfo.application_name_utf8 = plugin->applicationName.constData();
    plugin->hostInfo.application_version_utf8 =
        plugin->applicationVersion.constData();
    plugin->hostInfo.plugin_home_path_utf8 = plugin->pluginHomePath.constData();
    plugin->hostInfo.plugin_config_path_utf8 = plugin->pluginConfigPath.constData();
    plugin->hostInfo.get_current_file_path = &PluginManager::copyCurrentFilePath;
    plugin->hostInfo.open_file = &PluginManager::openFile;
    plugin->hostInfo.log = &PluginManager::log;

    if (!setInfo(&plugin->hostInfo)) {
        if (errorMessage)
            *errorMessage = loadError(canonicalPath, QStringLiteral("nppSetInfo rejected host"));
        library->unload();
        delete library;
        delete plugin;
        return false;
    }

    uint32_t count = 0;
    plugin->functions = getFuncsArray(&count);
    if (count > MaximumPluginFunctions
        || (count != 0 && !plugin->functions)) {
        if (errorMessage)
            *errorMessage = loadError(canonicalPath, QStringLiteral("invalid function array"));
        library->unload();
        delete library;
        delete plugin;
        return false;
    }
    for (uint32_t index = 0; index < count; ++index) {
        if (!hasValidFunction(plugin->functions[index])) {
            if (errorMessage) {
                *errorMessage = loadError(
                    canonicalPath, QStringLiteral("invalid function item"));
            }
            library->unload();
            delete library;
            delete plugin;
            return false;
        }
    }
    plugin->functionCount = static_cast<int>(count);
    plugin->beNotified = beNotified;
    plugin->messageProc = messageProc;
    _plugins.push_back(plugin);
    _loadedPaths.insert(canonicalPath);
    if (_readySent)
        notify(*_plugins.constLast(), NPP_PLUGIN_NOTIFICATION_READY);
    return true;
}

int PluginManager::loadPlugins(const QString& pluginDir,
                               const QStringList& enabledFolders,
                               QStringList* errors)
{
    QSet<QString> enabled;
    for (const QString& folder : enabledFolders)
        enabled.insert(folder.toCaseFolded());
    const int previousCount = _plugins.size();
    for (const PluginArtifact& artifact :
         PluginArtifactResolver::discoverCrossPlatform(pluginDir)) {
        if (!enabled.contains(artifact.folderName.toCaseFolded()))
            continue;
        QString error;
        if (!loadPlugin(artifact.binaryPath, artifact.folderName, &error)
            && !error.isEmpty() && errors) {
            errors->append(error);
        }
    }
    return _plugins.size() - previousCount;
}

void PluginManager::notifyReady()
{
    if (_readySent)
        return;
    _readySent = true;
    for (const LoadedPlugin* plugin : _plugins)
        notify(*plugin, NPP_PLUGIN_NOTIFICATION_READY);
}

void PluginManager::unloadAll()
{
    for (int index = _plugins.size() - 1; index >= 0; --index) {
        LoadedPlugin* plugin = _plugins[index];
        notify(*plugin, NPP_PLUGIN_NOTIFICATION_SHUTDOWN);
        plugin->library->unload();
        delete plugin->library;
        plugin->library = nullptr;
        delete plugin;
    }
    _plugins.clear();
    _loadedPaths.clear();
    _readySent = false;
}

QStringList PluginManager::loadedPluginNames() const
{
    QStringList names;
    for (const LoadedPlugin* plugin : _plugins)
        names.append(plugin->name);
    return names;
}

QStringList PluginManager::loadedPluginFolders() const
{
    QStringList folders;
    for (const LoadedPlugin* plugin : _plugins)
        folders.append(plugin->folderName);
    return folders;
}

int PluginManager::loadedPluginFunctionCount(int pluginIndex) const
{
    return pluginIndex >= 0 && pluginIndex < _plugins.size()
        ? _plugins[pluginIndex]->functionCount : 0;
}

QString PluginManager::loadedPluginFunctionName(
    int pluginIndex, int functionIndex) const
{
    if (pluginIndex < 0 || pluginIndex >= _plugins.size())
        return QString();
    const LoadedPlugin& plugin = *_plugins[pluginIndex];
    if (functionIndex < 0 || functionIndex >= plugin.functionCount)
        return QString();
    return QString::fromUtf8(plugin.functions[functionIndex].item_name_utf8);
}

bool PluginManager::isLoadedPluginFunctionInitiallyChecked(
    int pluginIndex, int functionIndex) const
{
    if (pluginIndex < 0 || pluginIndex >= _plugins.size())
        return false;
    const LoadedPlugin& plugin = *_plugins[pluginIndex];
    return functionIndex >= 0 && functionIndex < plugin.functionCount
        && plugin.functions[functionIndex].initially_checked != 0;
}

QKeySequence PluginManager::loadedPluginFunctionShortcut(
    int pluginIndex, int functionIndex) const
{
    if (pluginIndex < 0 || pluginIndex >= _plugins.size())
        return QKeySequence();
    const LoadedPlugin& plugin = *_plugins[pluginIndex];
    if (functionIndex < 0 || functionIndex >= plugin.functionCount)
        return QKeySequence();
    const NppPluginShortcutKey& shortcut =
        plugin.functions[functionIndex].shortcut;
    if (shortcut.key == 0)
        return QKeySequence();
    int key = static_cast<int>(shortcut.key);
    if (shortcut.is_ctrl)
        key |= Qt::CTRL;
    if (shortcut.is_alt)
        key |= Qt::ALT;
    if (shortcut.is_shift)
        key |= Qt::SHIFT;
    return QKeySequence(key);
}

bool PluginManager::executePluginCommand(
    int pluginIndex, int functionIndex, QString* errorMessage)
{
    if (pluginIndex < 0 || pluginIndex >= _plugins.size())
        return false;
    const LoadedPlugin& plugin = *_plugins[pluginIndex];
    if (functionIndex < 0 || functionIndex >= plugin.functionCount)
        return false;
    const NppPluginFuncItem& function = plugin.functions[functionIndex];
    if (!function.command)
        return true;
    try {
        function.command(function.user_data);
        return true;
    } catch (...) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Plugin command failed: %1")
                .arg(plugin.name);
        return false;
    }
}

qintptr PluginManager::sendPluginMessage(
    int pluginIndex, quint32 message, quintptr wParam, qintptr lParam) const
{
    if (pluginIndex < 0 || pluginIndex >= _plugins.size())
        return 0;
    return _plugins[pluginIndex]->messageProc(
        message, static_cast<uintptr_t>(wParam),
        static_cast<intptr_t>(lParam));
}

size_t NPP_PLUGIN_CALL PluginManager::copyCurrentFilePath(
    void* context, char* output, size_t capacity)
{
    PluginHostServices* services =
        static_cast<PluginHostServices*>(context);
    const QByteArray path = services
        ? services->currentFilePath().toUtf8() : QByteArray();
    const size_t required = static_cast<size_t>(path.size());
    if (output && capacity != 0) {
        const size_t copied = qMin(required, capacity - 1);
        std::memcpy(output, path.constData(), copied);
        output[copied] = '\0';
    }
    return required;
}

int NPP_PLUGIN_CALL PluginManager::openFile(void* context, const char* path)
{
    PluginHostServices* services =
        static_cast<PluginHostServices*>(context);
    return services && path && services->openFile(QString::fromUtf8(path));
}

void NPP_PLUGIN_CALL PluginManager::log(
    void*, uint32_t level, const char* message)
{
    const QString text = QString::fromUtf8(message ? message : "");
    if (level >= NPP_PLUGIN_LOG_ERROR)
        qCritical().noquote() << text;
    else if (level == NPP_PLUGIN_LOG_WARNING)
        qWarning().noquote() << text;
    else
        qInfo().noquote() << text;
}

void PluginManager::notify(const LoadedPlugin& plugin, uint32_t code) const
{
    NppPluginNotification notification{};
    notification.struct_size = sizeof(notification);
    notification.code = code;
    try {
        plugin.beNotified(&notification);
    } catch (...) {
        qWarning() << "Plugin notification failed:" << plugin.name << code;
    }
}
