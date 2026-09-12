#include "CrossPlatformPluginSystem/PluginInterface.h"

#include <cstring>
#include <string>

namespace {

const NppPluginHostInfo* host = nullptr;
bool hostInfoValid = false;
bool currentPathReceived = false;
int readyCount = 0;
int commandCount = 0;
bool extendedCallbacksValid = false;
bool notificationPayloadValid = false;
uint32_t contextCommand = 0;

void NPP_PLUGIN_CALL runCommand(void*)
{
    ++commandCount;
    if (host && host->open_file)
        host->open_file(host->host_context, "abi-plugin-open.txt");
    if (host && host->get_current_document
        && host->replace_current_document && host->create_document
        && host->get_clipboard_text && host->set_clipboard_text
        && host->set_current_selection && host->set_status_text) {
        uint8_t document[16]{};
        const size_t size = host->get_current_document(
            host->host_context, document, sizeof(document));
        const uint8_t replacement[] = {'u','p','d','a','t','e','d'};
        const uint8_t created[] = {'n','e','w'};
        char clipboard[16]{};
        extendedCallbacksValid = size == 8
            && std::memcmp(document, "document", 8) == 0
            && host->get_current_buffer_id(host->host_context) == 0
            && host->get_current_view(host->host_context) == 1
            && host->replace_current_document(host->host_context,
                   replacement, sizeof(replacement))
            && host->create_document(host->host_context, created, sizeof(created))
            && host->get_clipboard_text(host->host_context,
                   clipboard, sizeof(clipboard)) == 4
            && std::strcmp(clipboard, "clip") == 0
            && host->set_clipboard_text(host->host_context, "updated clipboard")
            && host->set_current_selection(host->host_context, 2, 5);
        if (extendedCallbacksValid && host->get_view_document
            && host->show_buffer_in_view && host->clear_compare_marks
            && host->add_compare_mark && host->get_first_visible_line
            && host->set_first_visible_line && host->goto_line) {
            uint8_t viewDocument[32]{};
            extendedCallbacksValid = host->get_view_document(host->host_context,
                    0, viewDocument, sizeof(viewDocument)) == 13
                && std::memcmp(viewDocument, "view-document", 13) == 0
                && host->show_buffer_in_view(host->host_context, 42, 1);
            host->clear_compare_marks(host->host_context, 1);
            extendedCallbacksValid = extendedCallbacksValid
                && host->add_compare_mark(host->host_context, 0, 7, 2)
                && host->get_first_visible_line(host->host_context, 0) == 12;
            host->set_first_visible_line(host->host_context, 1, 12);
            host->goto_line(host->host_context, 0, 7);
            char bufferPath[32]{};
            extendedCallbacksValid = extendedCallbacksValid
                && host->send_scintilla && host->get_buffer_file_path
                && host->save_current_file && host->execute_menu_command
                && host->send_scintilla(host->host_context,1,2006,2,3)==99
                && host->get_buffer_file_path(host->host_context,42,
                       bufferPath,sizeof(bufferPath))==10
                && std::strcmp(bufferPath,"buffer.txt")==0
                && host->save_current_file(host->host_context)
                && host->execute_menu_command(host->host_context,41007);
        }
        host->set_status_text(host->host_context, "updated status");
        char language[64]{}, shortName[2] = {'x','x'};
        const char* expectedLanguage = u8"自定义语言";
        extendedCallbacksValid = extendedCallbacksValid
            && host->get_current_language_name
            && host->get_current_language_name(host->host_context,nullptr,0) == std::strlen(expectedLanguage)
            && host->get_current_language_name(host->host_context,language,sizeof(language)) == std::strlen(expectedLanguage)
            && std::strcmp(language,expectedLanguage) == 0
            && host->get_current_language_name(host->host_context,shortName,sizeof(shortName)) == std::strlen(expectedLanguage)
            && shortName[1] == '\0';
    }
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
    } else if (notification->code == NPP_PLUGIN_NOTIFICATION_TEXT_MODIFIED) {
        notificationPayloadValid = notification->buffer_id==42
            && notification->source_view==1 && notification->position==5
            && notification->length==3 && notification->modification_type==7
            && notification->struct_size >= offsetof(NppPluginNotification, lines_added) + sizeof(notification->lines_added)
            && notification->lines_added == -2 && notification->text_utf8
            && std::strcmp(notification->text_utf8,"abc")==0;
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
        case 7: return extendedCallbacksValid ? 1 : 0;
        case 8: return notificationPayloadValid ? 1 : 0;
        case 9: return contextCommand;
        default: return 0;
    }
}

NPP_PLUGIN_EXPORT uint32_t NPP_PLUGIN_CALL nppGetCommandState(uint32_t index)
{
    return index == 0 ? NPP_PLUGIN_COMMAND_CHECKABLE |
        (commandCount == 0 ? NPP_PLUGIN_COMMAND_CHECKED : 0) : 0;
}

NPP_PLUGIN_EXPORT const NppPluginContextMenuItem* NPP_PLUGIN_CALL
nppGetEditorContextMenu(const NppPluginEditorContext* context,
                        uint32_t* count)
{
    static const NppPluginContextMenuItem items[] = {
        {sizeof(NppPluginContextMenuItem), 41,
         NPP_PLUGIN_CONTEXT_MENU_ITEM_NONE, 0, "ABI context command"},
        {sizeof(NppPluginContextMenuItem), 0,
         NPP_PLUGIN_CONTEXT_MENU_ITEM_SEPARATOR, 0, nullptr}
    };
    if (count)
        *count = context && context->view == 1 && context->byte_position == 7
            ? 2u : 0u;
    return count && *count ? items : nullptr;
}

NPP_PLUGIN_EXPORT void NPP_PLUGIN_CALL
nppExecuteEditorContextMenuCommand(uint32_t commandId)
{
    contextCommand = commandId;
}

struct LegacyNppData { void* npp; void* mainEditor; void* subEditor; };
struct LegacyFuncItem { wchar_t name[64]; void (*command)(); int commandId; bool checked; void* shortcut; };
NPP_PLUGIN_EXPORT void NPP_PLUGIN_CALL setInfo(LegacyNppData) {}
NPP_PLUGIN_EXPORT const wchar_t* NPP_PLUGIN_CALL getName()
    { return L"Legacy ABI test"; }
NPP_PLUGIN_EXPORT LegacyFuncItem* NPP_PLUGIN_CALL getFuncsArray(int* count)
    { if (count) *count = 0; return nullptr; }
NPP_PLUGIN_EXPORT void NPP_PLUGIN_CALL beNotified(void*) {}
NPP_PLUGIN_EXPORT intptr_t NPP_PLUGIN_CALL messageProc(
    uint32_t, uintptr_t, intptr_t) { return 1; }
NPP_PLUGIN_EXPORT int NPP_PLUGIN_CALL isUnicode() { return 1; }

} // extern "C"
