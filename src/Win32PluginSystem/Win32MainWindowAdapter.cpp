#include "Win32PluginSystem/Win32MainWindowAdapter.h"

#include "MainWindow.h"
#include "Win32PluginSystem/Win32PluginDockAdapter.h"
#include "Win32PluginSystem/Win32PluginInterface.h"

#include <QDir>
#include <QAction>
#include <QFileInfo>
#include <QMainWindow>
#include <commctrl.h>

#include "Parameters.h"
#include "ScintillaComponent/ScintillaEditView.h"

namespace {

LRESULT copyPluginString(const QString& value, WPARAM capacity, LPARAM output)
{
    if (!output)
        return value.size();
    if (capacity == 0 || value.size() >= static_cast<qsizetype>(capacity))
        return FALSE;
    memcpy(reinterpret_cast<void*>(output), value.utf16(),
           static_cast<size_t>(value.size() + 1) * sizeof(wchar_t));
    return TRUE;
}

LRESULT copyLegacyPathString(const QString& value, WPARAM capacity,
                             LPARAM output)
{
    if (!output)
        return FALSE;
    const qsizetype effectiveCapacity = capacity == 0
        ? MAX_PATH : static_cast<qsizetype>(capacity);
    if (effectiveCapacity <= 0 || value.size() >= effectiveCapacity)
        return FALSE;
    memcpy(reinterpret_cast<void*>(output), value.utf16(),
           static_cast<size_t>(value.size() + 1) * sizeof(wchar_t));
    return TRUE;
}

int currentLanguageType(const ScintillaEditView* editor)
{
    if (!editor)
        return 0;
    const QString language = editor->lexerLanguage().toLower();
    if (language == QStringLiteral("html"))
        return 8;
    if (language == QStringLiteral("xml"))
        return 9;
    if (language == QStringLiteral("json"))
        return 57;
    return 0;
}

} // namespace

Win32MainWindowAdapter::Win32MainWindowAdapter(
    QMainWindow* window, ScintillaEditView* mainEditor,
    ScintillaEditView* subEditor, HWND handle,
    Win32PluginDockAdapter* dockAdapter)
    : _window(window), _mainEditor(mainEditor), _subEditor(subEditor),
      _handle(handle), _dockAdapter(dockAdapter)
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
    UINT message, WPARAM wParam, LPARAM lParam, bool* handled)
{
    if (handled)
        *handled = false;
    MainWindow* nppWindow = qobject_cast<MainWindow*>(_window);
    if (!nppWindow)
        return 0;

    if (_dockAdapter) {
        const LRESULT result = _dockAdapter->handleMessage(
            message, wParam, lParam, handled);
        if (handled && *handled)
            return result;
    }

    if (message == NppMessageGetPluginConfigDir) {
        if (handled)
            *handled = true;
        const QString path = QDir(NppParameters::getInstance().getUserPath())
            .filePath(QStringLiteral("plugins/Config"));
        QDir().mkpath(path);
        return copyPluginString(QDir::toNativeSeparators(path),
                                wParam, lParam);
    }
    if (message == NppMessageGetFullCurrentPath
        || message == NppMessageGetFileName) {
        if (handled)
            *handled = true;
        QString path = nppWindow->currentPathForWin32Plugin();
        if (message == NppMessageGetFileName)
            path = QFileInfo(path).fileName();
        return copyLegacyPathString(
            QDir::toNativeSeparators(path), wParam, lParam);
    }
    if (message == NppMessageMenuCommand) {
        if (handled)
            *handled = true;
        return nppWindow->executeMenuCommandFromWin32Plugin(
            static_cast<int>(lParam)) ? TRUE : FALSE;
    }
    if (message == NppMessageDoOpen) {
        if (handled)
            *handled = true;
        if (!lParam)
            return FALSE;
        const QString path = QString::fromWCharArray(
            reinterpret_cast<const wchar_t*>(lParam));
        return nppWindow->openFileFromWin32Plugin(path) ? TRUE : FALSE;
    }
    if (message == NppMessageSetMenuItemCheck) {
        if (handled)
            *handled = true;
        const QList<QAction*> actions = _window->findChildren<QAction*>();
        for (QAction* action : actions) {
            if (action->property("win32PluginCommandId").toInt()
                == static_cast<int>(wParam)) {
                action->setCheckable(true);
                action->setChecked(lParam != 0);
                return TRUE;
            }
        }
        return FALSE;
    }
    if (message == NppMessageGetExtensionPart) {
        if (handled)
            *handled = true;
        const QString suffix = QFileInfo(nppWindow->currentFilePath()).suffix();
        const QString extension = suffix.isEmpty()
            ? QString() : QStringLiteral(".") + suffix;
        return copyPluginString(extension, wParam, lParam);
    }
    if (message == NppMessageGetCurrentLangType) {
        if (handled)
            *handled = true;
        int* language = reinterpret_cast<int*>(lParam);
        if (!language)
            return FALSE;
        *language = currentLanguageType(nppWindow->currentView());
        return TRUE;
    }
    if (message == NppMessageSetCurrentLangType) {
        if (handled)
            *handled = true;
        return nppWindow->setCurrentLanguageTypeFromPlugin(
            static_cast<int>(lParam)) ? TRUE : FALSE;
    }
    if (message != NppMessageGetCurrentScintilla)
        return 0;

    if (handled)
        *handled = true;
    int* currentView = reinterpret_cast<int*>(lParam);
    if (!currentView)
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
