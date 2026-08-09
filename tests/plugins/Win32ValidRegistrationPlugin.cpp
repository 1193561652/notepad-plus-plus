#include "Win32PluginSystem/Win32PluginInterface.h"

namespace {
void testCommand() {}
constexpr int notificationSlotCount = 64;
constexpr int notificationSequenceCapacity = 256;
constexpr UINT scintillaNotificationFirst = 2000;
constexpr int scintillaNotificationSlotCount = 100;
LONG notificationCounts[notificationSlotCount]{};
ULONG_PTR lastNotificationIds[notificationSlotCount]{};
ULONG_PTR lastNotificationWindows[notificationSlotCount]{};
LONG notificationSequenceCount = 0;
UINT notificationSequence[notificationSequenceCapacity]{};
LONG scintillaNotificationCounts[scintillaNotificationSlotCount]{};
ULONG_PTR lastScintillaNotificationWindow = 0;
LONG fileBeforeCloseCount = 0;
ULONG_PTR lastClosedBufferId = 0;
FuncItem functions[] = {
    {TEXT("Registration rollback probe"), testCommand, 0, false, nullptr}
};

int notificationSlot(UINT code)
{
    const UINT offset = code - NppNotificationFirst;
    return offset < notificationSlotCount ? static_cast<int>(offset) : -1;
}
} // namespace

extern "C" __declspec(dllexport) BOOL isUnicode() { return TRUE; }
extern "C" __declspec(dllexport) void setInfo(NppData) {}
extern "C" __declspec(dllexport) const TCHAR* getName()
{
    return TEXT("Valid Registration Test");
}
extern "C" __declspec(dllexport) void beNotified(SCNotification* notification)
{
    if (!notification)
        return;
    const int slot = notificationSlot(notification->nmhdr.code);
    if (slot >= 0) {
        InterlockedIncrement(&notificationCounts[slot]);
        lastNotificationIds[slot] = notification->nmhdr.idFrom;
        lastNotificationWindows[slot] =
            reinterpret_cast<ULONG_PTR>(notification->nmhdr.hwndFrom);
        const LONG sequenceIndex =
            InterlockedIncrement(&notificationSequenceCount) - 1;
        if (sequenceIndex < notificationSequenceCapacity)
            notificationSequence[sequenceIndex] = notification->nmhdr.code;
    }
    const UINT scintillaOffset =
        notification->nmhdr.code - scintillaNotificationFirst;
    if (scintillaOffset < scintillaNotificationSlotCount) {
        InterlockedIncrement(&scintillaNotificationCounts[scintillaOffset]);
        lastScintillaNotificationWindow =
            reinterpret_cast<ULONG_PTR>(notification->nmhdr.hwndFrom);
    }
    if (notification->nmhdr.code == NppNotificationFileBeforeClose) {
        InterlockedIncrement(&fileBeforeCloseCount);
        lastClosedBufferId = notification->nmhdr.idFrom;
    }
}
extern "C" __declspec(dllexport) void resetNotifications()
{
    for (int i = 0; i < notificationSlotCount; ++i) {
        InterlockedExchange(&notificationCounts[i], 0);
        lastNotificationIds[i] = 0;
        lastNotificationWindows[i] = 0;
    }
    InterlockedExchange(&notificationSequenceCount, 0);
    for (UINT& code : notificationSequence)
        code = 0;
    for (LONG& count : scintillaNotificationCounts)
        InterlockedExchange(&count, 0);
    lastScintillaNotificationWindow = 0;
    InterlockedExchange(&fileBeforeCloseCount, 0);
    lastClosedBufferId = 0;
}
extern "C" __declspec(dllexport) LONG getNotificationCount(UINT code)
{
    const int slot = notificationSlot(code);
    return slot >= 0 ? notificationCounts[slot] : 0;
}
extern "C" __declspec(dllexport) ULONG_PTR getLastNotificationId(UINT code)
{
    const int slot = notificationSlot(code);
    return slot >= 0 ? lastNotificationIds[slot] : 0;
}
extern "C" __declspec(dllexport) ULONG_PTR getLastNotificationWindow(UINT code)
{
    const int slot = notificationSlot(code);
    return slot >= 0 ? lastNotificationWindows[slot] : 0;
}
extern "C" __declspec(dllexport) LONG getNotificationSequenceCount()
{
    return notificationSequenceCount;
}
extern "C" __declspec(dllexport) UINT getNotificationSequenceCode(int index)
{
    return index >= 0 && index < notificationSequenceCount
        && index < notificationSequenceCapacity
        ? notificationSequence[index] : 0;
}
extern "C" __declspec(dllexport) LONG getScintillaNotificationCount(UINT code)
{
    const UINT offset = code - scintillaNotificationFirst;
    return offset < scintillaNotificationSlotCount
        ? scintillaNotificationCounts[offset] : 0;
}
extern "C" __declspec(dllexport) ULONG_PTR getLastScintillaNotificationWindow()
{
    return lastScintillaNotificationWindow;
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
