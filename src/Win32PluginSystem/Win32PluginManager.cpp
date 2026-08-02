#include "Win32PluginSystem/Win32PluginManager.h"

#include <QMainWindow>
#include <QFileInfo>
#include "MISC/PluginsManager/PluginArtifactResolver.h"
#include "ScintillaComponent/ScintillaEditView.h"
#include "Win32PluginSystem/Win32EditorMessageAdapter.h"

namespace {

constexpr wchar_t PluginReceiverClassName[] =
    L"NotepadPlusPlusQt.PluginMessageReceiver";

LRESULT CALLBACK pluginReceiverWindowProc(
    HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    Win32EditorMessageAdapter* adapter =
        reinterpret_cast<Win32EditorMessageAdapter*>(
            GetWindowLongPtrW(window, GWLP_USERDATA));
    bool handled = false;
    const LRESULT result = adapter
        ? adapter->handleMessage(message, wParam, lParam, &handled) : 0;
    if (handled)
        return result;
    return DefWindowProcW(window, message, wParam, lParam);
}

bool ensurePluginReceiverClass()
{
    static const bool registered = []() {
        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.lpfnWndProc = pluginReceiverWindowProc;
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.lpszClassName = PluginReceiverClassName;
        if (RegisterClassExW(&windowClass))
            return true;
        return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    }();
    return registered;
}

HWND nativeHandle(QWidget* window)
{
    return window
        ? reinterpret_cast<HWND>(window->winId())
        : nullptr;
}

HWND createPluginReceiver(HWND parent)
{
    if (!parent || !ensurePluginReceiverClass())
        return nullptr;

    HWND receiver = CreateWindowExW(
        0, PluginReceiverClassName, L"", WS_CHILD,
        0, 0, 1, 1, parent, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (receiver) {
        SetWindowPos(receiver, nullptr, 0, 0, 1, 1,
                     SWP_NOACTIVATE | SWP_NOZORDER | SWP_HIDEWINDOW);
    }
    return receiver;
}

void destroyPluginReceiver(HWND receiver)
{
    if (receiver && IsWindow(receiver))
        DestroyWindow(receiver);
}

template <typename Function>
Function resolveExport(HMODULE module, const char* name)
{
    return reinterpret_cast<Function>(GetProcAddress(module, name));
}

} // namespace

Win32PluginManager::Win32PluginManager(
    QMainWindow* mainWindow,
    ScintillaEditView* mainEditor,
    ScintillaEditView* subEditor)
    : QObject(mainWindow),
      _mainWindowAdapter(
          mainWindow, mainEditor, subEditor, nativeHandle(mainWindow)),
      _mainEditorReceiver(createPluginReceiver(_mainWindowAdapter.handle())),
      _subEditorReceiver(createPluginReceiver(_mainWindowAdapter.handle())),
      _mainEditorAdapter(mainEditor, _mainEditorReceiver),
      _subEditorAdapter(subEditor, _subEditorReceiver)
{
    Q_ASSERT(_mainWindowAdapter.isValid());
    Q_ASSERT(_mainEditorAdapter.isValid());
    Q_ASSERT(_subEditorAdapter.isValid());
    SetWindowLongPtrW(
        _mainEditorReceiver, GWLP_USERDATA,
        reinterpret_cast<LONG_PTR>(&_mainEditorAdapter));
    SetWindowLongPtrW(
        _subEditorReceiver, GWLP_USERDATA,
        reinterpret_cast<LONG_PTR>(&_subEditorAdapter));
}

Win32PluginManager::~Win32PluginManager()
{
    unloadPlugins();
    if (_mainEditorReceiver && IsWindow(_mainEditorReceiver))
        SetWindowLongPtrW(_mainEditorReceiver, GWLP_USERDATA, 0);
    if (_subEditorReceiver && IsWindow(_subEditorReceiver))
        SetWindowLongPtrW(_subEditorReceiver, GWLP_USERDATA, 0);
    destroyPluginReceiver(_mainEditorReceiver);
    destroyPluginReceiver(_subEditorReceiver);
}

HWND Win32PluginManager::mainWindowHandle() const
{
    return _mainWindowAdapter.handle();
}

HWND Win32PluginManager::mainEditorHandle() const
{
    return _mainEditorAdapter.handle();
}

HWND Win32PluginManager::secondaryEditorHandle() const
{
    return _subEditorAdapter.handle();
}

bool Win32PluginManager::loadPlugin(
    const QString& filePath, QString* errorMessage)
{
    const QString absolutePath = QFileInfo(filePath).absoluteFilePath();
    for (const LoadedPlugin& plugin : _loadedPlugins) {
        if (QFileInfo(plugin.filePath) == QFileInfo(absolutePath))
            return true;
    }

    HMODULE module = LoadLibraryW(
        reinterpret_cast<LPCWSTR>(absolutePath.utf16()));
    if (!module) {
        if (errorMessage)
            *errorMessage = QStringLiteral("LoadLibraryW failed with error %1")
                .arg(GetLastError());
        return false;
    }

    const PluginIsUnicode isUnicode =
        resolveExport<PluginIsUnicode>(module, "isUnicode");
    const PluginSetInfo setInfo =
        resolveExport<PluginSetInfo>(module, "setInfo");
    const PluginGetName getName =
        resolveExport<PluginGetName>(module, "getName");
    const PluginBeNotified beNotified =
        resolveExport<PluginBeNotified>(module, "beNotified");
    const PluginMessageProc messageProc =
        resolveExport<PluginMessageProc>(module, "messageProc");
    const PluginGetFuncsArray getFuncsArray =
        resolveExport<PluginGetFuncsArray>(module, "getFuncsArray");

    QString validationError;
    if (!isUnicode || !isUnicode())
        validationError = QStringLiteral("Missing or false isUnicode export");
    else if (!setInfo)
        validationError = QStringLiteral("Missing setInfo export");
    else if (!getName)
        validationError = QStringLiteral("Missing getName export");
    else if (!beNotified)
        validationError = QStringLiteral("Missing beNotified export");
    else if (!messageProc)
        validationError = QStringLiteral("Missing messageProc export");
    else if (!getFuncsArray)
        validationError = QStringLiteral("Missing getFuncsArray export");

    if (!validationError.isEmpty()) {
        FreeLibrary(module);
        if (errorMessage)
            *errorMessage = validationError;
        return false;
    }

    const TCHAR* pluginName = getName();
    if (!pluginName || !*pluginName) {
        FreeLibrary(module);
        if (errorMessage)
            *errorMessage = QStringLiteral("Plugin name is invalid");
        return false;
    }

    const NppData nppData{
        mainWindowHandle(), mainEditorHandle(), secondaryEditorHandle()
    };
    setInfo(nppData);

    int functionCount = 0;
    FuncItem* functions = getFuncsArray(&functionCount);
    if (!functions || functionCount <= 0) {
        FreeLibrary(module);
        if (errorMessage)
            *errorMessage = QStringLiteral("Plugin function array is invalid");
        return false;
    }

    LoadedPlugin plugin;
    plugin.module = module;
    plugin.filePath = absolutePath;
    plugin.name = QString::fromWCharArray(pluginName);
    plugin.beNotified = beNotified;
    plugin.messageProc = messageProc;
    plugin.functions = functions;
    plugin.functionCount = functionCount;
    _loadedPlugins.append(plugin);
    return true;
}

int Win32PluginManager::loadPlugins(
    const QString& pluginRoot, const QStringList& folderFilter,
    QStringList* errors)
{
    if (errors)
        errors->clear();
    int loaded = 0;
    const QVector<PluginArtifact> artifacts =
        PluginArtifactResolver::discover(pluginRoot);
    for (const PluginArtifact& artifact : artifacts) {
        bool selected = folderFilter.isEmpty();
        for (const QString& folder : folderFilter) {
            if (artifact.folderName.compare(folder, Qt::CaseInsensitive) == 0) {
                selected = true;
                break;
            }
        }
        if (!selected)
            continue;

        QString error;
        if (loadPlugin(artifact.binaryPath, &error)) {
            ++loaded;
        } else if (errors) {
            errors->append(QStringLiteral("%1: %2")
                               .arg(artifact.folderName, error));
        }
    }
    return loaded;
}

QStringList Win32PluginManager::loadedPluginNames() const
{
    QStringList names;
    for (const LoadedPlugin& plugin : _loadedPlugins)
        names.append(plugin.name);
    return names;
}

int Win32PluginManager::loadedPluginFunctionCount(int pluginIndex) const
{
    if (pluginIndex < 0 || pluginIndex >= _loadedPlugins.size())
        return 0;
    return _loadedPlugins[pluginIndex].functionCount;
}

bool Win32PluginManager::executePluginCommand(
    int pluginIndex, int functionIndex, QString* errorMessage)
{
    if (errorMessage)
        errorMessage->clear();
    if (pluginIndex < 0 || pluginIndex >= _loadedPlugins.size()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Plugin index is invalid");
        return false;
    }
    const LoadedPlugin& plugin = _loadedPlugins.at(pluginIndex);
    if (functionIndex < 0 || functionIndex >= plugin.functionCount ||
        !plugin.functions[functionIndex]._pFunc) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Plugin command is invalid");
        return false;
    }
    plugin.functions[functionIndex]._pFunc();
    return true;
}

LRESULT Win32PluginManager::relayMessage(
    UINT message, WPARAM wParam, LPARAM lParam) const
{
    LRESULT result = 0;
    for (const LoadedPlugin& plugin : _loadedPlugins) {
        if (plugin.messageProc)
            result = plugin.messageProc(message, wParam, lParam);
    }
    return result;
}

void Win32PluginManager::unloadPlugins()
{
    for (int i = _loadedPlugins.size() - 1; i >= 0; --i) {
        LoadedPlugin& plugin = _loadedPlugins[i];
        if (plugin.beNotified) {
            SCNotification notification{};
            notification.nmhdr.code = NppNotificationShutdown;
            notification.nmhdr.hwndFrom = mainWindowHandle();
            plugin.beNotified(&notification);
        }
        if (plugin.module)
            FreeLibrary(plugin.module);
    }
    _loadedPlugins.clear();
}
