#include "Win32PluginSystem/Win32MainWindowAdapter.h"

#include "MainWindow.h"
#include "Win32PluginSystem/Win32PluginInterface.h"

#include <QMainWindow>
#include <commctrl.h>

Win32MainWindowAdapter::Win32MainWindowAdapter(
    QMainWindow* window, ScintillaEditView* mainEditor,
    ScintillaEditView* subEditor, HWND handle)
    : _window(window), _mainEditor(mainEditor), _subEditor(subEditor),
      _handle(handle)
{
    Q_ASSERT(_window);
    Q_ASSERT(_mainEditor);
    Q_ASSERT(_subEditor);
    Q_ASSERT(_handle);
    _subclassInstalled = SetWindowSubclass(
        _handle, &Win32MainWindowAdapter::subclassProc,
        reinterpret_cast<UINT_PTR>(this),
        reinterpret_cast<DWORD_PTR>(this)) != FALSE;
    Q_ASSERT(_subclassInstalled);
}

Win32MainWindowAdapter::~Win32MainWindowAdapter()
{
    if (_subclassInstalled && _handle && IsWindow(_handle)) {
        RemoveWindowSubclass(
            _handle, &Win32MainWindowAdapter::subclassProc,
            reinterpret_cast<UINT_PTR>(this));
    }
    _subclassInstalled = false;
}

LRESULT Win32MainWindowAdapter::handleMessage(
    UINT message, WPARAM, LPARAM lParam, bool* handled) const
{
    if (handled)
        *handled = false;
    if (message != NppMessageGetCurrentScintilla)
        return 0;

    if (handled)
        *handled = true;
    int* currentView = reinterpret_cast<int*>(lParam);
    MainWindow* nppWindow = qobject_cast<MainWindow*>(_window);
    if (!currentView || !nppWindow)
        return FALSE;

    ScintillaEditView* activeEditor = nppWindow->currentView();
    if (activeEditor != _mainEditor && activeEditor != _subEditor)
        return FALSE;

    *currentView = activeEditor == _mainEditor ? 0 : 1;
    return TRUE;
}

LRESULT CALLBACK Win32MainWindowAdapter::subclassProc(
    HWND window, UINT message, WPARAM wParam, LPARAM lParam,
    UINT_PTR, DWORD_PTR referenceData)
{
    Win32MainWindowAdapter* adapter =
        reinterpret_cast<Win32MainWindowAdapter*>(referenceData);
    bool handled = false;
    const LRESULT result = adapter
        ? adapter->handleMessage(message, wParam, lParam, &handled) : 0;
    return handled ? result
                   : DefSubclassProc(window, message, wParam, lParam);
}
