// Qt host adapter. All plugin state and commands remain in each plugin module.
#pragma once
#include <PluginInterface.h>
#include <Scintilla.h>
#include <QByteArray>
#include <QMessageBox>
#include <QDir>
#include <vector>
#include <limits>
#include <stdexcept>

namespace QtPlugin {
inline const NppPluginHostInfo* host = nullptr;
inline std::vector<NppPluginFuncItem> commands;
inline std::vector<bool> checkable;
inline QByteArray currentPath() {
    if (!host || !host->get_current_file_path) return {};
    auto size = host->get_current_file_path(host->host_context, nullptr, 0);
    if (size >= size_t(std::numeric_limits<int>::max())) throw std::length_error("Path is too long");
    QByteArray path(static_cast<int>(size) + 1, '\0');
    host->get_current_file_path(host->host_context, path.data(), size + 1);
    path.resize(static_cast<int>(size));
    return path;
}
inline QString configPath(const char* name) {
    if (!host || !host->plugin_config_path_utf8 || !*host->plugin_config_path_utf8)
        throw std::runtime_error("Plugin configuration directory is unavailable");
    QDir directory(QString::fromUtf8(host->plugin_config_path_utf8));
    if (!directory.mkpath(".")) throw std::runtime_error("Cannot create plugin configuration directory");
    return directory.filePath(QString::fromUtf8(name));
}
inline intptr_t sci(uint32_t message, uintptr_t w = 0, intptr_t l = 0) {
    if (!host) return 0;
    return host->send_scintilla(host->host_context,
        host->get_current_view(host->host_context), message, w, l);
}
inline bool initialize(const NppPluginHostInfo* value) {
    host = nullptr;
    commands.clear();
    checkable.clear();
    if (!value || value->abi_version != NPP_PLUGIN_ABI_VERSION ||
        value->struct_size < offsetof(NppPluginHostInfo, send_scintilla) + sizeof(value->send_scintilla) ||
        !value->send_scintilla || !value->get_current_view) return false;
    host = value;
    return true;
}
inline void add(const char* label, NppPluginCommandProc proc,
                uintptr_t data = 0, NppPluginShortcutKey shortcut = {}) {
    NppPluginFuncItem item{};
    item.struct_size = sizeof(item);
    item.item_name_utf8 = label;
    item.command = proc;
    item.user_data = reinterpret_cast<void*>(data);
    item.shortcut = shortcut;
    commands.push_back(item);
    checkable.push_back(false);
}
inline void addToggle(const char* label, NppPluginCommandProc proc, bool checked) {
    add(label, proc);
    commands.back().initially_checked = checked;
    checkable.back() = true;
}
inline QByteArray targetText() {
    auto size = sci(SCI_GETTARGETTEXT);
    if (size < 0 || size >= std::numeric_limits<int>::max())
        throw std::length_error("Selection is too large");
    QByteArray result(static_cast<int>(size) + 1, '\0');
    sci(SCI_GETTARGETTEXT, 0, reinterpret_cast<intptr_t>(result.data()));
    result.resize(static_cast<int>(size));
    return result;
}
inline void error(const char* title, const char* message) {
    QMessageBox dialog(QMessageBox::Warning, QString::fromUtf8(title), QString::fromUtf8(message), QMessageBox::Ok);
    dialog.setObjectName(QStringLiteral("pluginErrorDialog"));
    dialog.exec();
}
inline void about(const char* name, const char* text) {
    QMessageBox dialog(QMessageBox::Information, QString::fromUtf8(name), QString::fromUtf8(text), QMessageBox::Ok);
    dialog.setObjectName(QStringLiteral("pluginAboutDialog"));
    dialog.exec();
}
template<void (*Function)(void*)> void NPP_PLUGIN_CALL invoke(void* data) noexcept {
    if (!host) return;
    try { Function(data); }
    catch (...) {
        if (host->log) host->log(host->host_context, NPP_PLUGIN_LOG_ERROR, "Plugin command failed");
    }
}
}

#define NPP_QT_EXPORTS(Name, Setup, Notify) \
extern "C" NPP_PLUGIN_EXPORT uint32_t NPP_PLUGIN_CALL nppGetCommandState(uint32_t index) { \
    if (index >= QtPlugin::commands.size()) return 0; \
    return (QtPlugin::checkable[index] ? NPP_PLUGIN_COMMAND_CHECKABLE : 0) | \
        (QtPlugin::commands[index].initially_checked ? NPP_PLUGIN_COMMAND_CHECKED : 0); } \
extern "C" NPP_PLUGIN_EXPORT uint32_t NPP_PLUGIN_CALL nppGetPluginAbiVersion() { return NPP_PLUGIN_ABI_VERSION; } \
extern "C" NPP_PLUGIN_EXPORT const char* NPP_PLUGIN_CALL nppGetName() { return Name; } \
extern "C" NPP_PLUGIN_EXPORT int NPP_PLUGIN_CALL nppSetInfo(const NppPluginHostInfo* h) { \
    try { if (!QtPlugin::initialize(h)) return 0; Setup(); return 1; } \
    catch (...) { QtPlugin::host = nullptr; QtPlugin::commands.clear(); return 0; } } \
extern "C" NPP_PLUGIN_EXPORT const NppPluginFuncItem* NPP_PLUGIN_CALL nppGetFuncsArray(uint32_t* count) { \
    if (count) *count = static_cast<uint32_t>(QtPlugin::commands.size()); return QtPlugin::commands.data(); } \
extern "C" NPP_PLUGIN_EXPORT void NPP_PLUGIN_CALL nppBeNotified(const NppPluginNotification* n) { \
    if (!n || n->struct_size < offsetof(NppPluginNotification, text_utf8) + sizeof(n->text_utf8) || !QtPlugin::host) return; \
    try { Notify(n); } catch (...) {} \
    if (n->code == NPP_PLUGIN_NOTIFICATION_SHUTDOWN) QtPlugin::host = nullptr; } \
extern "C" NPP_PLUGIN_EXPORT intptr_t NPP_PLUGIN_CALL nppMessageProc(uint32_t, uintptr_t, intptr_t) { return 0; }
