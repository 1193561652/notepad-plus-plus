#pragma once

#ifndef _WIN32
#error Win32PluginInterface is available only on Windows.
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tchar.h>

#include "Scintilla.h"

constexpr int Win32PluginNameLength = 64;
constexpr UINT NppMessageBase = WM_USER + 1000;
constexpr UINT NppMessageGetCurrentScintilla = NppMessageBase + 4;
constexpr UINT NppMessageGetCurrentLangType = NppMessageBase + 5;
constexpr UINT NppMessageSetCurrentLangType = NppMessageBase + 6;
constexpr UINT NppMessageModelessDialog = NppMessageBase + 12;
constexpr UINT NppMessageDmmShow = NppMessageBase + 30;
constexpr UINT NppMessageDmmHide = NppMessageBase + 31;
constexpr UINT NppMessageDmmUpdateDisplayInfo = NppMessageBase + 32;
constexpr UINT NppMessageDmmRegisterAsDockDialog = NppMessageBase + 33;
constexpr UINT NppMessageDmmViewOtherTab = NppMessageBase + 35;
constexpr UINT NppMessageSetMenuItemCheck = NppMessageBase + 40;
constexpr UINT NppMessageDmmGetPluginHwndByName = NppMessageBase + 43;
constexpr UINT NppMessageGetPluginConfigDir = NppMessageBase + 46;
constexpr UINT NppMessageMenuCommand = NppMessageBase + 48;
constexpr UINT NppMessageDoOpen = NppMessageBase + 77;
constexpr UINT NppMessageGetFullCurrentPath = WM_USER + 3001;
constexpr UINT NppMessageGetFileName = WM_USER + 3003;
constexpr UINT NppMessageGetExtensionPart = WM_USER + 3005;
constexpr unsigned int NppNotificationFirst = 1000;
constexpr unsigned int NppNotificationToolbarModification =
    NppNotificationFirst + 2;
constexpr unsigned int NppNotificationFileBeforeClose =
    NppNotificationFirst + 3;
constexpr unsigned int NppNotificationShutdown = NppNotificationFirst + 9;

constexpr int NppMenuCommandFileNew = 41001;

constexpr WPARAM Win32ModelessDialogAdd = 0;
constexpr WPARAM Win32ModelessDialogRemove = 1;

constexpr UINT Win32DwsIconTab = 0x00000001;
constexpr UINT Win32DwsIconBar = 0x00000002;
constexpr UINT Win32DwsAddInfo = 0x00000004;
constexpr UINT Win32DwsUseOwnDarkMode = 0x00000008;
constexpr UINT Win32DwsDefaultFloating = 0x80000000;
constexpr int Win32DockContainerLeft = 0;
constexpr int Win32DockContainerRight = 1;
constexpr int Win32DockContainerTop = 2;
constexpr int Win32DockContainerBottom = 3;

constexpr UINT Win32DockNotifyFirst = 1050;
constexpr UINT Win32DockNotifyClose = Win32DockNotifyFirst + 1;
constexpr UINT Win32DockNotifyDock = Win32DockNotifyFirst + 2;
constexpr UINT Win32DockNotifyFloat = Win32DockNotifyFirst + 3;
constexpr UINT Win32DockNotifySwitchIn = Win32DockNotifyFirst + 4;
constexpr UINT Win32DockNotifySwitchOff = Win32DockNotifyFirst + 5;
constexpr UINT Win32DockNotifyFloatDropped = Win32DockNotifyFirst + 6;

struct Win32DockingData
{
    HWND hClient = nullptr;
    const TCHAR* pszName = nullptr;
    int dlgID = 0;
    UINT uMask = 0;
    HICON hIconTab = nullptr;
    const TCHAR* pszAddInfo = nullptr;
    RECT rcFloat = {};
    int iPrevCont = 0;
    const TCHAR* pszModuleName = nullptr;
};

struct NppData
{
    HWND _nppHandle = nullptr;
    HWND _scintillaMainHandle = nullptr;
    HWND _scintillaSecondHandle = nullptr;
};

using PluginGetName = const TCHAR* (__cdecl*)();
using PluginSetInfo = void (__cdecl*)(NppData);
using PluginCommand = void (__cdecl*)();
using PluginBeNotified = void (__cdecl*)(SCNotification*);
using PluginMessageProc = LRESULT (__cdecl*)(UINT, WPARAM, LPARAM);
using PluginIsUnicode = BOOL (__cdecl*)();

struct Win32PluginShortcutKey
{
    bool _isCtrl = false;
    bool _isAlt = false;
    bool _isShift = false;
    UCHAR _key = 0;
};

struct FuncItem
{
    TCHAR _itemName[Win32PluginNameLength] = {_T('\0')};
    PluginCommand _pFunc = nullptr;
    int _cmdID = 0;
    bool _init2Check = false;
    Win32PluginShortcutKey* _pShKey = nullptr;
};

using PluginGetFuncsArray = FuncItem* (__cdecl*)(int*);
