#include "Win32PluginSystem/Win32PluginInterface.h"

extern "C" __declspec(dllexport) BOOL isUnicode() { return TRUE; }
extern "C" __declspec(dllexport) void setInfo(NppData) {}
extern "C" __declspec(dllexport) const TCHAR* getName()
{
    return TEXT("Invalid Registration Test");
}
extern "C" __declspec(dllexport) void beNotified(SCNotification*) {}
extern "C" __declspec(dllexport) LRESULT messageProc(UINT, WPARAM, LPARAM)
{
    return TRUE;
}
extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* count)
{
    if (count)
        *count = 0;
    return nullptr;
}
