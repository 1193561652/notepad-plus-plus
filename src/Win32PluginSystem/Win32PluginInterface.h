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
constexpr unsigned int NppNotificationFirst = 1000;
constexpr unsigned int NppNotificationShutdown = NppNotificationFirst + 9;

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
