#pragma once

#include <QtGlobal>

#ifndef Q_OS_WIN
#error Win32MainWindowAdapter is available only on Windows.
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

class QMainWindow;
class ScintillaEditView;
class Win32PluginDockAdapter;

class Win32MainWindowAdapter final
{
public:
    Win32MainWindowAdapter(QMainWindow* window,
                           ScintillaEditView* mainEditor,
                           ScintillaEditView* subEditor,
                           HWND handle,
                           Win32PluginDockAdapter* dockAdapter);
    ~Win32MainWindowAdapter();

    QMainWindow* window() const { return _window; }
    HWND handle() const { return _handle; }
    bool isValid() const
        { return _window != nullptr && _handle && IsWindow(_handle) &&
                 _subclassInstalled; }

    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam,
                          bool* handled);

private:
    static LRESULT CALLBACK subclassProc(
        HWND window, UINT message, WPARAM wParam, LPARAM lParam,
        UINT_PTR subclassId, DWORD_PTR referenceData);

    QMainWindow* _window = nullptr;
    ScintillaEditView* _mainEditor = nullptr;
    ScintillaEditView* _subEditor = nullptr;
    HWND _handle = nullptr;
    Win32PluginDockAdapter* _dockAdapter = nullptr;
    bool _subclassInstalled = false;
};
