#include "Win32PluginSystem/Win32PluginManager.h"

#include <QMainWindow>
#include <QDir>
#include <QFileInfo>
#include <QByteArray>
#include <QDebug>
#include <QMessageBox>
#include <zlib.h>
#include "MISC/PluginsManager/PluginArtifactResolver.h"
#include "ScintillaComponent/ScintillaEditView.h"
#include "Win32PluginSystem/Win32EditorMessageAdapter.h"
#include "WinControls/DockingWnd/DockingManager.h"

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

bool decodePercentEncoded(const QByteArray& encoded, QByteArray* decoded)
{
    decoded->clear();
    decoded->reserve(encoded.size());
    auto hexValue = [](char character) {
        if (character >= '0' && character <= '9')
            return character - '0';
        if (character >= 'A' && character <= 'F')
            return character - 'A' + 10;
        if (character >= 'a' && character <= 'f')
            return character - 'a' + 10;
        return -1;
    };
    for (int index = 0; index < encoded.size(); ++index) {
        if (encoded.at(index) != '%') {
            decoded->append(encoded.at(index));
            continue;
        }
        if (index + 2 >= encoded.size())
            return false;
        const int high = hexValue(encoded.at(index + 1));
        const int low = hexValue(encoded.at(index + 2));
        if (high < 0 || low < 0)
            return false;
        decoded->append(static_cast<char>((high << 4) | low));
        index += 2;
    }
    return true;
}

bool decodeStrictBase64(const QByteArray& encoded, QByteArray* decoded)
{
    if (encoded.size() < 4 || encoded.size() % 4 != 0)
        return false;
    const int firstPadding = encoded.indexOf('=');
    const int contentLength = firstPadding < 0 ? encoded.size() : firstPadding;
    for (int index = 0; index < contentLength; ++index) {
        const char character = encoded.at(index);
        if (!((character >= 'A' && character <= 'Z')
              || (character >= 'a' && character <= 'z')
              || (character >= '0' && character <= '9')
              || character == '+' || character == '/')) {
            return false;
        }
    }
    if (firstPadding >= 0) {
        const int paddingLength = encoded.size() - firstPadding;
        if (paddingLength > 2
            || encoded.mid(firstPadding) != QByteArray(paddingLength, '=')) {
            return false;
        }
    }
    *decoded = QByteArray::fromBase64(encoded);
    return !decoded->isEmpty();
}

bool inflateRawDeflate(const QByteArray& compressed, QByteArray* inflated)
{
    constexpr int MaximumSamlMessageSize = 200000;
    z_stream stream{};
    stream.next_in = reinterpret_cast<Bytef*>(
        const_cast<char*>(compressed.constData()));
    stream.avail_in = static_cast<uInt>(compressed.size());
    if (inflateInit2(&stream, -MAX_WBITS) != Z_OK)
        return false;

    inflated->resize(4096);
    int result = Z_OK;
    while (result == Z_OK) {
        if (stream.total_out == static_cast<uLong>(inflated->size())) {
            if (inflated->size() >= MaximumSamlMessageSize) {
                inflateEnd(&stream);
                return false;
            }
            inflated->resize(qMin(MaximumSamlMessageSize,
                                  inflated->size() * 2));
        }
        stream.next_out = reinterpret_cast<Bytef*>(
            inflated->data() + stream.total_out);
        stream.avail_out = static_cast<uInt>(
            inflated->size() - stream.total_out);
        result = inflate(&stream, Z_NO_FLUSH);
    }
    const bool complete = result == Z_STREAM_END
        && stream.total_out > 0
        && stream.total_out <= MaximumSamlMessageSize;
    const uLong outputLength = stream.total_out;
    inflateEnd(&stream);
    if (!complete)
        return false;
    inflated->resize(static_cast<int>(outputLength));
    return true;
}

} // namespace

Win32PluginManager::Win32PluginManager(
    QMainWindow* mainWindow,
    ScintillaEditView* mainEditor,
    ScintillaEditView* subEditor,
    const QString& pluginStateDirectory,
    DockingManager* dockingManager)
    : QObject(mainWindow),
      _dockAdapter(mainWindow, dockingManager, nativeHandle(mainWindow)),
      _mainWindowAdapter(
          mainWindow, mainEditor, subEditor, nativeHandle(mainWindow),
          &_dockAdapter),
      _mainEditorReceiver(createPluginReceiver(_mainWindowAdapter.handle())),
      _subEditorReceiver(createPluginReceiver(_mainWindowAdapter.handle())),
      _mainEditorAdapter(mainEditor, _mainEditorReceiver),
      _subEditorAdapter(subEditor, _subEditorReceiver),
      _loadJournal(pluginStateDirectory)
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
    QString journalError;
    if (!_loadJournal.beginSession(&journalError))
        qWarning() << "Could not initialize plugin load journal:"
                   << journalError;
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

    const QString folderName = QFileInfo(absolutePath).dir().dirName();
    QString journalError;
    if (!_loadJournal.beginPlugin(folderName, absolutePath, &journalError)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Could not persist plugin load state: %1").arg(journalError);
        }
        return false;
    }

    const int commandIdCheckpoint = _nextCommandId;
    auto failLoad = [&](HMODULE module, const QString& error) {
        _nextCommandId = commandIdCheckpoint;
        if (module)
            FreeLibrary(module);
        _loadJournal.completePlugin(
            folderName, absolutePath, QString(), error);
        if (errorMessage)
            *errorMessage = error;
        return false;
    };

    HMODULE module = LoadLibraryW(
        reinterpret_cast<LPCWSTR>(absolutePath.utf16()));
    if (!module) {
        return failLoad(nullptr,
            QStringLiteral("LoadLibraryW failed with error %1")
                .arg(GetLastError()));
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
        return failLoad(module, validationError);
    }

    const TCHAR* pluginName = getName();
    if (!pluginName || !*pluginName) {
        return failLoad(module, QStringLiteral("Plugin name is invalid"));
    }

    const NppData nppData{
        mainWindowHandle(), mainEditorHandle(), secondaryEditorHandle()
    };
    setInfo(nppData);

    int functionCount = 0;
    FuncItem* functions = getFuncsArray(&functionCount);
    if (!functions || functionCount <= 0) {
        return failLoad(
            module, QStringLiteral("Plugin function array is invalid"));
    }
    for (int index = 0; index < functionCount; ++index) {
        if (functions[index]._pFunc)
            functions[index]._cmdID = _nextCommandId++;
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
    _loadJournal.completePlugin(
        folderName, absolutePath, plugin.name);
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
    int failed = 0;
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

        const PluginLoadJournal::RecoveryEntry recovery =
            _loadJournal.recoveryEntry();
        if (recovery.isValid()
            && recovery.folderName.compare(
                   artifact.folderName, Qt::CaseInsensitive) == 0) {
            _recoveredPluginFolder = artifact.folderName;
            _loadJournal.skipRecoveredPlugin(
                artifact.folderName, artifact.binaryPath);
            if (errors) {
                errors->append(QStringLiteral(
                    "%1: skipped because the previous process stopped "
                    "while loading this plugin").arg(artifact.folderName));
            }
            ++failed;
            continue;
        }

        QString error;
        if (loadPlugin(artifact.binaryPath, &error)) {
            ++loaded;
        } else if (errors) {
            errors->append(QStringLiteral("%1: %2")
                               .arg(artifact.folderName, error));
            ++failed;
        } else {
            ++failed;
        }
    }
    if (loaded > 0)
        notifyPlugins(NppNotificationToolbarModification);
    _loadJournal.finishSession(loaded, failed);
    return loaded;
}

void Win32PluginManager::notifyFileBeforeClose(quintptr bufferId)
{
    notifyPlugins(NppNotificationFileBeforeClose, bufferId);
}

void Win32PluginManager::notifyPlugins(
    unsigned int code, quintptr idFrom) const
{
    SCNotification notification{};
    notification.nmhdr.code = code;
    notification.nmhdr.hwndFrom = mainWindowHandle();
    notification.nmhdr.idFrom = static_cast<uptr_t>(idFrom);
    for (const LoadedPlugin& plugin : _loadedPlugins) {
        if (plugin.beNotified)
            plugin.beNotified(&notification);
    }
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

QString Win32PluginManager::loadedPluginFunctionName(
    int pluginIndex, int functionIndex) const
{
    if (pluginIndex < 0 || pluginIndex >= _loadedPlugins.size())
        return QString();
    const LoadedPlugin& plugin = _loadedPlugins.at(pluginIndex);
    if (functionIndex < 0 || functionIndex >= plugin.functionCount)
        return QString();
    return QString::fromWCharArray(plugin.functions[functionIndex]._itemName);
}

int Win32PluginManager::loadedPluginFunctionCommandId(
    int pluginIndex, int functionIndex) const
{
    if (pluginIndex < 0 || pluginIndex >= _loadedPlugins.size())
        return 0;
    const LoadedPlugin& plugin = _loadedPlugins.at(pluginIndex);
    if (functionIndex < 0 || functionIndex >= plugin.functionCount)
        return 0;
    return plugin.functions[functionIndex]._cmdID;
}

bool Win32PluginManager::isLoadedPluginFunctionInitiallyChecked(
    int pluginIndex, int functionIndex) const
{
    if (pluginIndex < 0 || pluginIndex >= _loadedPlugins.size())
        return false;
    const LoadedPlugin& plugin = _loadedPlugins.at(pluginIndex);
    return functionIndex >= 0 && functionIndex < plugin.functionCount
        && plugin.functions[functionIndex]._init2Check;
}

QKeySequence Win32PluginManager::loadedPluginFunctionShortcut(
    int pluginIndex, int functionIndex) const
{
    if (pluginIndex < 0 || pluginIndex >= _loadedPlugins.size())
        return QKeySequence();
    const LoadedPlugin& plugin = _loadedPlugins.at(pluginIndex);
    if (functionIndex < 0 || functionIndex >= plugin.functionCount)
        return QKeySequence();
    const Win32PluginShortcutKey* shortcut =
        plugin.functions[functionIndex]._pShKey;
    if (!shortcut || !shortcut->_key)
        return QKeySequence();

    int key = shortcut->_key;
    if (key == VK_OEM_PLUS)
        key = Qt::Key_Equal;
    else if (key == VK_OEM_7)
        key = Qt::Key_Apostrophe;
    if (shortcut->_isCtrl)
        key |= Qt::CTRL;
    if (shortcut->_isAlt)
        key |= Qt::ALT;
    if (shortcut->_isShift)
        key |= Qt::SHIFT;
    return QKeySequence(key);
}

bool Win32PluginManager::isLoadedPluginFunctionSeparator(
    int pluginIndex, int functionIndex) const
{
    if (pluginIndex < 0 || pluginIndex >= _loadedPlugins.size())
        return false;
    const LoadedPlugin& plugin = _loadedPlugins.at(pluginIndex);
    return functionIndex >= 0 && functionIndex < plugin.functionCount
        && plugin.functions[functionIndex]._pFunc == nullptr;
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
    if (plugin.name == QStringLiteral("MIME Tools") && functionIndex == 15)
        return executeMimeToolsSamlDecode(errorMessage);
    if (plugin.name == QStringLiteral("MIME Tools") && functionIndex == 17) {
        showMimeToolsAbout();
        return true;
    }
    if (plugin.name == QStringLiteral("JSON Viewer") && functionIndex == 3) {
        showJsonViewerAbout();
        return true;
    }
    // mimeTools 2.8 allocates its URL buffer from SCI_GETSELTEXT, then clears
    // it using strlen(text) + 1. Its original host contract included that NUL.
    const bool mimeToolsUrlCommand = plugin.name == QStringLiteral("MIME Tools")
        && functionIndex >= 11 && functionIndex <= 13;
    if (mimeToolsUrlCommand) {
        _mainEditorAdapter.setSelectionTextLengthIncludesTerminator(true);
        _subEditorAdapter.setSelectionTextLengthIncludesTerminator(true);
    }
    plugin.functions[functionIndex]._pFunc();
    if (mimeToolsUrlCommand) {
        _mainEditorAdapter.setSelectionTextLengthIncludesTerminator(false);
        _subEditorAdapter.setSelectionTextLengthIncludesTerminator(false);
    }
    return true;
}

bool Win32PluginManager::executeMimeToolsSamlDecode(QString* errorMessage)
{
    Q_UNUSED(errorMessage);

    int currentView = 0;
    SendMessageW(mainWindowHandle(), NppMessageGetCurrentScintilla, 0,
                 reinterpret_cast<LPARAM>(&currentView));
    const HWND editor = currentView == 0
        ? mainEditorHandle() : secondaryEditorHandle();
    const LRESULT selectionLength = SendMessageW(editor, SCI_GETSELTEXT, 0, 0);
    if (selectionLength <= 0)
        return true;

    QByteArray selection(selectionLength + 1, '\0');
    SendMessageW(editor, SCI_GETSELTEXT, 0,
                 reinterpret_cast<LPARAM>(selection.data()));
    selection.truncate(qstrlen(selection.constData()));

    QByteArray percentDecoded;
    if (!decodePercentEncoded(selection, &percentDecoded)) {
        MessageBoxW(mainWindowHandle(), L"Could not URL Decode text.",
                    L"SAML Decode", MB_OK);
        return true;
    }
    QByteArray base64Decoded;
    if (!decodeStrictBase64(percentDecoded, &base64Decoded)) {
        MessageBoxW(mainWindowHandle(),
                    L"Could not BASE64 Decode text after URL Decoding.",
                    L"SAML Decode", MB_OK);
        return true;
    }

    QByteArray decoded;
    if (base64Decoded.startsWith("<?xml") || base64Decoded.startsWith("<saml")) {
        decoded = base64Decoded;
    } else if (!inflateRawDeflate(base64Decoded, &decoded)
               || !(decoded.startsWith("<?xml")
                    || decoded.startsWith("<saml"))) {
        MessageBoxW(mainWindowHandle(),
                    L"Could not inflate text after BASE64 Decoding.",
                    L"SAML Decode", MB_OK);
        return true;
    }

    const LRESULT start = SendMessageW(editor, SCI_GETSELECTIONSTART, 0, 0);
    const LRESULT end = SendMessageW(editor, SCI_GETSELECTIONEND, 0, 0);
    SendMessageW(editor, SCI_SETTARGETSTART, qMin(start, end), 0);
    SendMessageW(editor, SCI_SETTARGETEND, qMax(start, end), 0);
    SendMessageW(editor, SCI_REPLACETARGET, decoded.size(),
                 reinterpret_cast<LPARAM>(decoded.constData()));
    SendMessageW(editor, SCI_SETSEL, qMin(start, end),
                 qMin(start, end) + decoded.size());
    return true;
}

void Win32PluginManager::showMimeToolsAbout()
{
    QMessageBox* dialog = new QMessageBox(
        QMessageBox::Information, QStringLiteral("MIME Tools"),
        QStringLiteral("Author : Don HO\nVersion : 2.8\nLicence : GPL"),
        QMessageBox::Close, mainWindow());
    dialog->setObjectName(QStringLiteral("mimeToolsAboutDialog"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowModality(Qt::NonModal);
    dialog->show();
}

void Win32PluginManager::showJsonViewerAbout()
{
    QMessageBox* dialog = new QMessageBox(
        QMessageBox::Information, QStringLiteral("About JSON Viewer"),
        QStringLiteral("JSON Viewer\n\n"
                       "Author: Kapil Ratnani\n"
                       "Version: 1.41\n"
                       "Licence: GPL\n"
                       "Special thanks to: Don Ho for Notepad++\n"
                       "Website: https://github.com/kapilratnani/JSON-Viewer"),
        QMessageBox::Close, mainWindow());
    dialog->setObjectName(QStringLiteral("jsonViewerAboutDialog"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowModality(Qt::NonModal);
    dialog->show();
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
        const LoadedPlugin& plugin = _loadedPlugins.at(i);
        if (!plugin.beNotified)
            continue;
        SCNotification notification{};
        notification.nmhdr.code = NppNotificationShutdown;
        notification.nmhdr.hwndFrom = mainWindowHandle();
        plugin.beNotified(&notification);
    }
    _dockAdapter.releaseAll();
    for (int i = _loadedPlugins.size() - 1; i >= 0; --i) {
        LoadedPlugin& plugin = _loadedPlugins[i];
        if (plugin.module)
            FreeLibrary(plugin.module);
    }
    _loadedPlugins.clear();
}
