#include "Win32PluginSystem/Win32MainWindowAdapter.h"

#include <QDebug>

#include "MainWindow.h"
#include "Win32PluginSystem/Win32PluginDockAdapter.h"
#include "Win32PluginSystem/Win32PluginInterface.h"

#include <QDir>
#include <QAction>
#include <QFileInfo>
#include <QMainWindow>
#include <QPalette>
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
    if (language == QStringLiteral("php"))
        return 1;
    if (language == QStringLiteral("c"))
        return 2;
    if (language == QStringLiteral("cpp")
        || language == QStringLiteral("c++"))
        return 3;
    if (language == QStringLiteral("csharp")
        || language == QStringLiteral("c#"))
        return 4;
    if (language == QStringLiteral("java"))
        return 6;
    if (language == QStringLiteral("html"))
        return 8;
    if (language == QStringLiteral("xml"))
        return 9;
    if (language == QStringLiteral("javascript")
        || language == QStringLiteral("js"))
        return 58;
    if (language == QStringLiteral("python"))
        return 22;
    if (language == QStringLiteral("json"))
        return 57;
    return 0;
}

QString languageName(int languageType)
{
    switch (languageType) {
        case 0: return QStringLiteral("Normal Text");
        case 1: return QStringLiteral("PHP");
        case 2: return QStringLiteral("C");
        case 3: return QStringLiteral("C++");
        case 4: return QStringLiteral("C#");
        case 6: return QStringLiteral("Java");
        case 8: return QStringLiteral("HTML");
        case 9: return QStringLiteral("XML");
        case 19:
        case 58: return QStringLiteral("JavaScript");
        case 22: return QStringLiteral("Python");
        case 57: return QStringLiteral("JSON");
        default: return QString();
    }
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
    if (message == NppMessageGetNppDirectory) {
        if (handled)
            *handled = true;
        return copyLegacyPathString(QDir::toNativeSeparators(
            NppParameters::getInstance().getNppPath()), wParam, lParam);
    }
    if (message == NppMessageGetEnableThemeTextureFunc) {
        if (handled)
            *handled = true;
        static HMODULE themeLibrary = LoadLibraryW(L"uxtheme.dll");
        return reinterpret_cast<LRESULT>(themeLibrary
            ? GetProcAddress(themeLibrary, "EnableThemeDialogTexture")
            : nullptr);
    }
    if (message == NppMessageGetFullCurrentPath
        || message == NppMessageGetFileName
        || message == NppMessageGetCurrentDirectory) {
        if (handled)
            *handled = true;
        QString path = nppWindow->currentPathForPlugin();
        if (message == NppMessageGetFileName)
            path = QFileInfo(path).fileName();
        else if (message == NppMessageGetCurrentDirectory)
            path = QFileInfo(path).absolutePath();
        return copyLegacyPathString(
            QDir::toNativeSeparators(path), wParam, lParam);
    }
    if (message == NppMessageMenuCommand) {
        if (handled)
            *handled = true;
        return nppWindow->executePluginMenuCommand(
            static_cast<int>(lParam)) ? TRUE : FALSE;
    }
    if (message == NppMessageSaveCurrentFile
        || message == NppMessageSaveAllFiles) {
        if (handled)
            *handled = true;
        const int commandId = message == NppMessageSaveCurrentFile
            ? 41006 : 41007;
        return nppWindow->executePluginMenuCommand(commandId)
            ? TRUE : FALSE;
    }
    if (message == WM_COMMAND && lParam == 0) {
        const int commandId = LOWORD(wParam);
        if (nppWindow->executePluginMenuCommand(commandId)) {
            if (handled)
                *handled = true;
            return TRUE;
        }
    }
    if (message == NppMessageDoOpen) {
        if (handled)
            *handled = true;
        if (!lParam)
            return FALSE;
        const QString path = QString::fromWCharArray(
            reinterpret_cast<const wchar_t*>(lParam));
        return nppWindow->openFileForPlugin(path) ? TRUE : FALSE;
    }
    if (message == NppMessageSaveCurrentFileAs) {
        if (handled)
            *handled = true;
        if (!lParam)
            return FALSE;
        const QString path = QString::fromWCharArray(
            reinterpret_cast<const wchar_t*>(lParam));
        return nppWindow->saveCurrentFileAsForPlugin(
            path, wParam == TRUE) ? TRUE : FALSE;
    }
    if (message == NppMessageSaveCurrentSession
        || message == NppMessageLoadSession) {
        if (handled)
            *handled = true;
        if (!lParam)
            return FALSE;
        const QString path = QString::fromWCharArray(
            reinterpret_cast<const wchar_t*>(lParam));
        return (message == NppMessageSaveCurrentSession
            ? nppWindow->saveCurrentSessionForPlugin(path)
            : nppWindow->loadSessionForPlugin(path)) ? TRUE : FALSE;
    }
    if (message == NppMessageGetCurrentBufferId) {
        if (handled)
            *handled = true;
        return static_cast<LRESULT>(nppWindow->currentBufferIdForPlugin());
    }
    if (message == NppMessageGetFullPathFromBufferId) {
        if (handled)
            *handled = true;
        return copyLegacyPathString(
            QDir::toNativeSeparators(nppWindow->pathForPluginBuffer(
                static_cast<quintptr>(wParam))), MAX_PATH, lParam);
    }
    if (message == NppMessageGetPosFromBufferId) {
        if (handled)
            *handled = true;
        return nppWindow->positionForPluginBuffer(
            static_cast<quintptr>(wParam), static_cast<int>(lParam));
    }
    if (message == NppMessageGetNbOpenFiles) {
        if (handled)
            *handled = true;
        return nppWindow->openFileCountForPlugin(
            static_cast<int>(lParam));
    }
    if (message == NppMessageGetCurrentDocIndex) {
        if (handled)
            *handled = true;
        return nppWindow->currentDocumentIndexForPlugin(
            static_cast<int>(lParam));
    }
    if (message == NppMessageActivateDoc) {
        if (handled)
            *handled = true;
        return nppWindow->activateDocumentForPlugin(
            static_cast<int>(wParam), static_cast<int>(lParam));
    }
    if (message == NppMessageGetCurrentLine) {
        if (handled)
            *handled = true;
        return nppWindow->currentLineForPlugin();
    }
    if (message == NppMessageGetBufferEncoding) {
        if (handled)
            *handled = true;
        return nppWindow->bufferEncodingForPlugin(
            static_cast<quintptr>(wParam));
    }
    if (message == NppMessageSetBufferEncoding) {
        if (handled)
            *handled = true;
        return nppWindow->setBufferEncodingForPlugin(
            static_cast<quintptr>(wParam), static_cast<int>(lParam));
    }
    if (message == NppMessageSetStatusBar) {
        if (handled)
            *handled = true;
        if (!lParam)
            return FALSE;
        nppWindow->setPluginStatusBarText(
            static_cast<int>(wParam), QString::fromWCharArray(
                reinterpret_cast<const wchar_t*>(lParam)));
        return TRUE;
    }
    if (message == NppMessageAddToolbarIconForDarkMode) {
        if (handled)
            *handled = true;
        return nppWindow->addPluginToolbarCommand(
            static_cast<int>(wParam));
    }
    if (message == NppMessageGetPluginHomePath) {
        if (handled)
            *handled = true;
        const QString path = QDir(NppParameters::getInstance().getNppPath())
            .filePath(QStringLiteral("plugins"));
        return copyPluginString(QDir::toNativeSeparators(path), wParam, lParam);
    }
    if (message == NppMessageGetNppVersion) {
        if (handled)
            *handled = true;
        return MAKELONG(46, 8);
    }
    if (message == NppMessageGetWindowsVersion) {
        if (handled)
            *handled = true;
        return 14;
    }
    if (message == NppMessageGetEditorDefaultForegroundColor
        || message == NppMessageGetEditorDefaultBackgroundColor) {
        if (handled)
            *handled = true;
        const QColor color = _window->palette().color(
            message == NppMessageGetEditorDefaultForegroundColor
                ? QPalette::Text : QPalette::Base);
        return RGB(color.red(), color.green(), color.blue());
    }
    if (message == NppMessageIsDarkModeEnabled) {
        if (handled)
            *handled = true;
        return NppParameters::getInstance().getNppGUI()._darkModeEnabled;
    }
    if (message == NppMessageGetDarkModeColors) {
        if (handled)
            *handled = true;
        if (wParam != sizeof(Win32NppDarkModeColors) || !lParam)
            return FALSE;
        Win32NppDarkModeColors* colors =
            reinterpret_cast<Win32NppDarkModeColors*>(lParam);
        auto nativeColor = [](const QColor& color) {
            return RGB(color.red(), color.green(), color.blue());
        };
        const QPalette palette = _window->palette();
        colors->background = nativeColor(palette.color(QPalette::Window));
        colors->softerBackground = nativeColor(palette.color(QPalette::Button));
        colors->hotBackground = nativeColor(palette.color(QPalette::Highlight));
        colors->pureBackground = nativeColor(palette.color(QPalette::Base));
        colors->errorBackground = RGB(176, 32, 37);
        colors->text = nativeColor(palette.color(QPalette::WindowText));
        colors->darkerText = nativeColor(palette.color(QPalette::Text));
        colors->disabledText = nativeColor(
            palette.color(QPalette::Disabled, QPalette::Text));
        colors->linkText = nativeColor(palette.color(QPalette::Link));
        colors->edge = nativeColor(palette.color(QPalette::Mid));
        colors->hotEdge = nativeColor(palette.color(QPalette::Highlight));
        colors->disabledEdge = nativeColor(
            palette.color(QPalette::Disabled, QPalette::Mid));
        return TRUE;
    }
    if (message == NppMessageGetCurrentCommandLine) {
        if (handled)
            *handled = true;
        return copyPluginString(
            QString::fromWCharArray(GetCommandLineW()), wParam, lParam);
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
        return copyPluginString(suffix, wParam, lParam);
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
    if (message == NppMessageGetLanguageName) {
        if (handled)
            *handled = true;
        const QString name = languageName(static_cast<int>(wParam));
        if (name.isEmpty())
            return 0;
        if (!lParam)
            return name.size();
        memcpy(reinterpret_cast<void*>(lParam), name.utf16(),
               static_cast<size_t>(name.size() + 1) * sizeof(wchar_t));
        return name.size();
    }
    if (message == NppMessageSetCurrentLangType) {
        if (handled)
            *handled = true;
        return nppWindow->setCurrentLanguageTypeFromPlugin(
            static_cast<int>(lParam)) ? TRUE : FALSE;
    }
    if (message != NppMessageGetCurrentScintilla) {
        if (qEnvironmentVariableIsSet(
                "NPP_QT_TEST_PLUGIN_MESSAGE_TRACE")
            && (message >= WM_USER || message == WM_COMMAND
                || message == WM_ACTIVATEAPP || message == WM_ACTIVATE)) {
            qWarning() << "Unhandled Win32 plugin NPP message"
                       << message << "wParam" << wParam;
        }
        return 0;
    }

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
    return handled
        ? result : DefSubclassProc(window, message, wParam, lParam);
}
