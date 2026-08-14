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
    if (!getAbiVersion) {
        library->unload();
        delete library;
        return false;
    }
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
    if (!getName || !setInfo || !getFuncsArray
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
    plugin->hostInfo.get_current_buffer_id = &PluginManager::currentBufferId;
    plugin->hostInfo.get_current_view = &PluginManager::currentView;
    plugin->hostInfo.get_current_document = &PluginManager::copyCurrentDocument;
    plugin->hostInfo.replace_current_document = &PluginManager::replaceCurrentDocument;
    plugin->hostInfo.get_current_selection = &PluginManager::copyCurrentSelection;
    plugin->hostInfo.replace_current_selection = &PluginManager::replaceCurrentSelection;
    plugin->hostInfo.set_current_selection = &PluginManager::setCurrentSelection;
    plugin->hostInfo.create_document = &PluginManager::createDocument;
    plugin->hostInfo.get_clipboard_text = &PluginManager::copyClipboardText;
    plugin->hostInfo.set_clipboard_text = &PluginManager::setClipboardText;
    plugin->hostInfo.set_status_text = &PluginManager::setStatusText;
    plugin->hostInfo.get_view_document = &PluginManager::copyViewDocument;
    plugin->hostInfo.show_buffer_in_view = &PluginManager::showBufferInView;
    plugin->hostInfo.clear_compare_marks = &PluginManager::clearCompareMarks;
    plugin->hostInfo.add_compare_mark = &PluginManager::addCompareMark;
    plugin->hostInfo.get_first_visible_line = &PluginManager::firstVisibleLine;
    plugin->hostInfo.set_first_visible_line = &PluginManager::setFirstVisibleLine;
    plugin->hostInfo.goto_line = &PluginManager::gotoLine;
    plugin->hostInfo.send_scintilla = &PluginManager::sendScintilla;
    plugin->hostInfo.get_buffer_file_path = &PluginManager::copyBufferFilePath;
    plugin->hostInfo.save_current_file = &PluginManager::saveCurrentFile;
    plugin->hostInfo.execute_menu_command = &PluginManager::executeMenuCommand;

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
    plugin->getEditorContextMenu =
        reinterpret_cast<NppGetEditorContextMenuFn>(
            library->resolve("nppGetEditorContextMenu"));
    plugin->executeEditorContextMenu =
        reinterpret_cast<NppExecuteEditorContextMenuCommandFn>(
            library->resolve("nppExecuteEditorContextMenuCommand"));
    _plugins.push_back(plugin);
    _loadedPaths.insert(canonicalPath);
    if (_readySent) {
        NppPluginNotification notification{};
        notification.struct_size = sizeof(notification);
        notification.code = NPP_PLUGIN_NOTIFICATION_READY;
        notify(*_plugins.constLast(), notification);
    }
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
    notifyPlugins(NPP_PLUGIN_NOTIFICATION_READY);
}

void PluginManager::unloadAll()
{
    for (int index = _plugins.size() - 1; index >= 0; --index) {
        LoadedPlugin* plugin = _plugins[index];
        NppPluginNotification notification{};
        notification.struct_size = sizeof(notification);
        notification.code = NPP_PLUGIN_NOTIFICATION_SHUTDOWN;
        notify(*plugin, notification);
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

QVector<PluginEditorContextMenuItem> PluginManager::editorContextMenu(
    int view, qint64 bytePosition) const
{
    constexpr uint32_t MaximumContextItems = 256;
    QVector<PluginEditorContextMenuItem> result;
    NppPluginEditorContext context{};
    context.struct_size = sizeof(context);
    context.view = view;
    context.byte_position = bytePosition;
    for (int pluginIndex = 0; pluginIndex < _plugins.size(); ++pluginIndex) {
        const LoadedPlugin* plugin = _plugins[pluginIndex];
        if (!plugin->getEditorContextMenu || !plugin->executeEditorContextMenu)
            continue;
        uint32_t count = 0;
        const NppPluginContextMenuItem* items =
            plugin->getEditorContextMenu(&context, &count);
        if (!items || count > MaximumContextItems)
            continue;
        for (uint32_t index = 0; index < count; ++index) {
            const NppPluginContextMenuItem& item = items[index];
            if (item.struct_size != sizeof(item))
                continue;
            const bool separator =
                (item.flags & NPP_PLUGIN_CONTEXT_MENU_ITEM_SEPARATOR) != 0;
            if (!separator && (!item.item_name_utf8 || !item.item_name_utf8[0]))
                continue;
            PluginEditorContextMenuItem entry;
            entry.pluginIndex = pluginIndex;
            entry.commandId = item.command_id;
            entry.separator = separator;
            if (!separator)
                entry.text = QString::fromUtf8(item.item_name_utf8);
            result.append(entry);
        }
    }
    return result;
}

void PluginManager::executeEditorContextMenuCommand(
    int pluginIndex, quint32 commandId) const
{
    if (pluginIndex < 0 || pluginIndex >= _plugins.size())
        return;
    const LoadedPlugin* plugin = _plugins[pluginIndex];
    if (plugin->executeEditorContextMenu)
        plugin->executeEditorContextMenu(commandId);
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

uint64_t NPP_PLUGIN_CALL PluginManager::currentBufferId(void* context)
{
    PluginHostServices* services = static_cast<PluginHostServices*>(context);
    return services ? static_cast<uint64_t>(services->currentBufferId()) : 0;
}

int32_t NPP_PLUGIN_CALL PluginManager::currentView(void* context)
{
    PluginHostServices* services = static_cast<PluginHostServices*>(context);
    return services ? services->currentViewIndex() : -1;
}

size_t NPP_PLUGIN_CALL PluginManager::copyCurrentDocument(
    void* context, uint8_t* output, size_t capacity)
{
    PluginHostServices* services = static_cast<PluginHostServices*>(context);
    const QByteArray data = services ? services->currentDocumentBytes() : QByteArray();
    const size_t required = static_cast<size_t>(data.size());
    if (output && capacity)
        std::memcpy(output, data.constData(), qMin(required, capacity));
    return required;
}

int NPP_PLUGIN_CALL PluginManager::replaceCurrentDocument(
    void* context, const uint8_t* data, size_t size)
{
    PluginHostServices* services = static_cast<PluginHostServices*>(context);
    return services && (data || size == 0)
        && services->replaceCurrentDocument(QByteArray(
            reinterpret_cast<const char*>(data), static_cast<int>(size)));
}

size_t NPP_PLUGIN_CALL PluginManager::copyCurrentSelection(
    void* context, uint8_t* output, size_t capacity,
    int64_t* start, int64_t* end)
{
    PluginHostServices* services = static_cast<PluginHostServices*>(context);
    qint64 qtStart = 0;
    qint64 qtEnd = 0;
    const QByteArray data = services
        ? services->currentSelectionBytes(&qtStart, &qtEnd) : QByteArray();
    if (start)
        *start = qtStart;
    if (end)
        *end = qtEnd;
    const size_t required = static_cast<size_t>(data.size());
    if (output && capacity)
        std::memcpy(output, data.constData(), qMin(required, capacity));
    return required;
}

int NPP_PLUGIN_CALL PluginManager::replaceCurrentSelection(
    void* context, const uint8_t* data, size_t size)
{
    PluginHostServices* services = static_cast<PluginHostServices*>(context);
    return services && (data || size == 0)
        && services->replaceCurrentSelection(QByteArray(
            reinterpret_cast<const char*>(data), static_cast<int>(size)));
}

int NPP_PLUGIN_CALL PluginManager::setCurrentSelection(
    void* context, int64_t start, int64_t end)
{
    PluginHostServices* services = static_cast<PluginHostServices*>(context);
    return services && services->setCurrentSelection(start, end);
}

int NPP_PLUGIN_CALL PluginManager::createDocument(
    void* context, const uint8_t* data, size_t size)
{
    PluginHostServices* services = static_cast<PluginHostServices*>(context);
    return services && (data || size == 0)
        && services->createDocument(QByteArray(
            reinterpret_cast<const char*>(data), static_cast<int>(size)));
}

size_t NPP_PLUGIN_CALL PluginManager::copyClipboardText(
    void* context, char* output, size_t capacity)
{
    PluginHostServices* services = static_cast<PluginHostServices*>(context);
    const QByteArray data = services ? services->clipboardText().toUtf8() : QByteArray();
    const size_t required = static_cast<size_t>(data.size());
    if (output && capacity) {
        const size_t copied = qMin(required, capacity - 1);
        std::memcpy(output, data.constData(), copied);
        output[copied] = '\0';
    }
    return required;
}

int NPP_PLUGIN_CALL PluginManager::setClipboardText(
    void* context, const char* text)
{
    PluginHostServices* services = static_cast<PluginHostServices*>(context);
    return services && text && services->setClipboardText(QString::fromUtf8(text));
}

void NPP_PLUGIN_CALL PluginManager::setStatusText(
    void* context, const char* text)
{
    PluginHostServices* services = static_cast<PluginHostServices*>(context);
    if (services)
        services->setStatusBarText(0, QString::fromUtf8(text ? text : ""));
}

size_t NPP_PLUGIN_CALL PluginManager::copyViewDocument(
    void* context, int32_t view, uint8_t* output, size_t capacity)
{
    auto* services = static_cast<PluginHostServices*>(context);
    const QByteArray data = services ? services->viewDocumentBytes(view) : QByteArray();
    const size_t required = static_cast<size_t>(data.size());
    if (output && capacity)
        std::memcpy(output, data.constData(), qMin(required, capacity));
    return required;
}

int NPP_PLUGIN_CALL PluginManager::showBufferInView(
    void* context, uint64_t bufferId, int32_t view)
{
    auto* services = static_cast<PluginHostServices*>(context);
    return services && services->showBufferInView(
        static_cast<quintptr>(bufferId), view);
}

void NPP_PLUGIN_CALL PluginManager::clearCompareMarks(void* context, int32_t view)
{
    auto* services = static_cast<PluginHostServices*>(context);
    if (services) services->clearCompareMarks(view);
}

int NPP_PLUGIN_CALL PluginManager::addCompareMark(
    void* context, int32_t view, int64_t line, uint32_t kind)
{
    auto* services = static_cast<PluginHostServices*>(context);
    return services && services->addCompareMark(view, line, kind);
}

int64_t NPP_PLUGIN_CALL PluginManager::firstVisibleLine(void* context, int32_t view)
{
    auto* services = static_cast<PluginHostServices*>(context);
    return services ? services->firstVisibleLine(view) : -1;
}

int NPP_PLUGIN_CALL PluginManager::setFirstVisibleLine(
    void* context, int32_t view, int64_t line)
{
    auto* services = static_cast<PluginHostServices*>(context);
    return services && services->setFirstVisibleLine(view, line);
}

int NPP_PLUGIN_CALL PluginManager::gotoLine(
    void* context, int32_t view, int64_t line)
{
    auto* services = static_cast<PluginHostServices*>(context);
    return services && services->gotoLine(view, line);
}

intptr_t NPP_PLUGIN_CALL PluginManager::sendScintilla(
    void* context, int32_t view, uint32_t message,
    uintptr_t wParam, intptr_t lParam)
{
    auto* services = static_cast<PluginHostServices*>(context);
    return services ? services->sendScintilla(
        view, message, static_cast<quintptr>(wParam),
        static_cast<qintptr>(lParam)) : 0;
}

size_t NPP_PLUGIN_CALL PluginManager::copyBufferFilePath(
    void* context, uint64_t bufferId, char* output, size_t capacity)
{
    auto* services = static_cast<PluginHostServices*>(context);
    const QByteArray path = services
        ? services->pathForBuffer(static_cast<quintptr>(bufferId)).toUtf8()
        : QByteArray();
    const size_t required = static_cast<size_t>(path.size());
    if (output && capacity) {
        const size_t copied = qMin(required, capacity - 1);
        std::memcpy(output, path.constData(), copied);
        output[copied] = '\0';
    }
    return required;
}

int NPP_PLUGIN_CALL PluginManager::saveCurrentFile(void* context)
{
    auto* services = static_cast<PluginHostServices*>(context);
    return services && services->saveCurrentFile();
}

int NPP_PLUGIN_CALL PluginManager::executeMenuCommand(
    void* context, int32_t commandId)
{
    auto* services = static_cast<PluginHostServices*>(context);
    return services && services->executeMenuCommand(commandId);
}

void PluginManager::notifyPlugins(
    uint32_t code, quintptr bufferId, int sourceView,
    qint64 position, qint64 length, quint32 modificationType,
    quint32 updated, const QByteArray& text)
{
    NppPluginNotification notification{};
    notification.struct_size = sizeof(notification);
    notification.code = code;
    notification.buffer_id = static_cast<uint64_t>(bufferId);
    notification.source_view = sourceView;
    notification.position = position;
    notification.length = length;
    notification.modification_type = modificationType;
    notification.updated = updated;
    notification.text_utf8 = text.isEmpty() ? nullptr : text.constData();
    for (const LoadedPlugin* plugin : _plugins)
        notify(*plugin, notification);
}

void PluginManager::notify(
    const LoadedPlugin& plugin,
    const NppPluginNotification& notification) const
{
    try {
        plugin.beNotified(&notification);
    } catch (...) {
        qWarning() << "Plugin notification failed:"
                   << plugin.name << notification.code;
    }
}
