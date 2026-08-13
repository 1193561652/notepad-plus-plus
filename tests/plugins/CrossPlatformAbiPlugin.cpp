#include "CrossPlatformPluginSystem/PluginInterface.h"

#include <cstring>
#include <string>

namespace {

const NppPluginHostInfo* host = nullptr;
bool hostInfoValid = false;
bool currentPathReceived = false;
int readyCount = 0;
int commandCount = 0;

void NPP_PLUGIN_CALL runCommand(void*)
{
    ++commandCount;
    if (host && host->open_file)
        host->open_file(host->host_context, "abi-plugin-open.txt");
}

NppPluginFuncItem functions[] = {{
    sizeof(NppPluginFuncItem),
    "Run ABI test command",
    &runCommand,
    nullptr,
    1,
    {0, 0, 0, 0, 0, 0, 0},
    {1, 1, 0, 0, 'K'}
}};

bool hasText(const char* value)
{
    return value && value[0];
}

} // namespace

extern "C" {

NPP_PLUGIN_EXPORT uint32_t NPP_PLUGIN_CALL nppGetPluginAbiVersion(void)
{
    return NPP_PLUGIN_ABI_VERSION;
}

NPP_PLUGIN_EXPORT const char* NPP_PLUGIN_CALL nppGetName(void)
{
    return "Cross-platform ABI test";
}

NPP_PLUGIN_EXPORT int NPP_PLUGIN_CALL nppSetInfo(
    const NppPluginHostInfo* hostInfo)
{
    host = hostInfo;
    hostInfoValid = host
        && host->struct_size >= sizeof(NppPluginHostInfo)
        && host->abi_version == NPP_PLUGIN_ABI_VERSION
        && host->system_type != NPP_PLUGIN_SYSTEM_UNKNOWN
        && hasText(host->system_name_utf8)
        && hasText(host->system_version_utf8)
        && hasText(host->application_name_utf8)
        && std::strcmp(host->application_version_utf8, "8.4.6") == 0
        && hasText(host->plugin_home_path_utf8)
        && hasText(host->plugin_config_path_utf8)
        && host->get_current_file_path && host->open_file && host->log;
    if (hostInfoValid) {
        char path[128]{};
        const size_t length = host->get_current_file_path(
            host->host_context, path, sizeof(path));
        currentPathReceived = length == std::strlen("current.txt")
            && std::strcmp(path, "current.txt") == 0;
    }
    return hostInfoValid ? 1 : 0;
}

NPP_PLUGIN_EXPORT const NppPluginFuncItem* NPP_PLUGIN_CALL nppGetFuncsArray(
    uint32_t* count)
{
    if (count)
        *count = 1;
    return functions;
}

NPP_PLUGIN_EXPORT void NPP_PLUGIN_CALL nppBeNotified(
    const NppPluginNotification* notification)
{
    if (!notification || !host)
        return;
    if (notification->code == NPP_PLUGIN_NOTIFICATION_READY) {
        ++readyCount;
        host->log(host->host_context, NPP_PLUGIN_LOG_INFO, "abi-ready");
    } else if (notification->code == NPP_PLUGIN_NOTIFICATION_SHUTDOWN) {
        host->log(host->host_context, NPP_PLUGIN_LOG_INFO, "abi-shutdown");
    }
}

NPP_PLUGIN_EXPORT intptr_t NPP_PLUGIN_CALL nppMessageProc(
    uint32_t message, uintptr_t, intptr_t)
{
    switch (message) {
        case 1: return hostInfoValid ? 1 : 0;
        case 2: return currentPathReceived ? 1 : 0;
        case 3: return readyCount;
        case 4: return commandCount;
        case 5: return host ? host->system_type : 0;
        case 6: return host ? host->cpu_architecture : 0;
        default: return 0;
    }
}

} // extern "C"
