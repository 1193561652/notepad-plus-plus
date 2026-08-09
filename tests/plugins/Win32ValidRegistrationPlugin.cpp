#include "Win32PluginSystem/Win32PluginInterface.h"

namespace {
void testCommand() {}
LONG fileBeforeCloseCount = 0;
ULONG_PTR lastClosedBufferId = 0;
FuncItem functions[] = {
    {TEXT("Registration rollback probe"), testCommand, 0, false, nullptr}
};
} // namespace

extern "C" __declspec(dllexport) BOOL isUnicode() { return TRUE; }
extern "C" __declspec(dllexport) void setInfo(NppData) {}
extern "C" __declspec(dllexport) const TCHAR* getName()
{
    return TEXT("Valid Registration Test");
}
extern "C" __declspec(dllexport) void beNotified(SCNotification* notification)
{
    if (notification
        && notification->nmhdr.code == NppNotificationFileBeforeClose) {
        InterlockedIncrement(&fileBeforeCloseCount);
        lastClosedBufferId = notification->nmhdr.idFrom;
    }
}
extern "C" __declspec(dllexport) LONG getFileBeforeCloseCount()
{
    return fileBeforeCloseCount;
}
extern "C" __declspec(dllexport) ULONG_PTR getLastClosedBufferId()
{
    return lastClosedBufferId;
}
extern "C" __declspec(dllexport) LRESULT messageProc(UINT, WPARAM, LPARAM)
{
    return TRUE;
}
extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* count)
{
    if (count)
        *count = 1;
    return functions;
}
