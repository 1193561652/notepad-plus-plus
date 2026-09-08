#pragma once

#include <QByteArray>
#include <QKeySequence>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QVector>

#include "CrossPlatformPluginSystem/PluginInterface.h"

class PluginHostServices;
class QLibrary;

struct PluginEditorContextMenuItem
{
    int pluginIndex = -1;
    quint32 commandId = 0;
    QString text;
    bool separator = false;
};

class PluginManager final : public QObject
{
public:
    explicit PluginManager(PluginHostServices* hostServices,
                           QObject* parent = nullptr);
    ~PluginManager() override;

    bool loadPlugin(const QString& filePath,
                    const QString& folderName = QString(),
                    QString* errorMessage = nullptr);
    int loadPlugins(const QString& pluginDir,
                    const QStringList& enabledFolders,
                    QStringList* errors = nullptr);
    void notifyReady();
    void notifyPlugins(uint32_t code, quintptr bufferId = 0,
                       int sourceView = -1, qint64 position = 0,
                       qint64 length = 0, quint32 modificationType = 0,
                       quint32 updated = 0, const QByteArray& text = {},
                       qint64 linesAdded = 0);
    void unloadAll();

    int loadedPluginCount() const { return _plugins.size(); }
    QStringList loadedPluginNames() const;
    QStringList loadedPluginFolders() const;
    int loadedPluginFunctionCount(int pluginIndex) const;
    QString loadedPluginFunctionName(int pluginIndex, int functionIndex) const;
    bool isLoadedPluginFunctionInitiallyChecked(
        int pluginIndex, int functionIndex) const;
    bool isLoadedPluginFunctionCheckable(int pluginIndex, int functionIndex) const;
    bool isLoadedPluginFunctionSeparator(int pluginIndex, int functionIndex) const;
    QKeySequence loadedPluginFunctionShortcut(
        int pluginIndex, int functionIndex) const;
    bool executePluginCommand(int pluginIndex, int functionIndex,
                              QString* errorMessage = nullptr);
    qintptr sendPluginMessage(int pluginIndex, quint32 message,
                              quintptr wParam = 0, qintptr lParam = 0) const;
    QVector<PluginEditorContextMenuItem> editorContextMenu(
        int view, qint64 bytePosition) const;
    void executeEditorContextMenuCommand(int pluginIndex,
                                         quint32 commandId) const;

private:
    struct LoadedPlugin {
        QLibrary* library = nullptr;
        QString filePath;
        QString folderName;
        QString name;
        NppBeNotifiedFn beNotified = nullptr;
        NppMessageProcFn messageProc = nullptr;
        NppGetCommandStateFn getCommandState = nullptr;
        NppGetEditorContextMenuFn getEditorContextMenu = nullptr;
        NppExecuteEditorContextMenuCommandFn executeEditorContextMenu = nullptr;
        const NppPluginFuncItem* functions = nullptr;
        int functionCount = 0;
        QByteArray systemName;
        QByteArray systemVersion;
        QByteArray applicationName;
        QByteArray applicationVersion;
        QByteArray pluginHomePath;
        QByteArray pluginConfigPath;
        NppPluginHostInfo hostInfo{};
    };

    static size_t NPP_PLUGIN_CALL copyCurrentFilePath(
        void* context, char* output, size_t capacity);
    static int NPP_PLUGIN_CALL openFile(void* context, const char* path);
    static void NPP_PLUGIN_CALL log(void* context, uint32_t level,
                                    const char* message);
    static uint64_t NPP_PLUGIN_CALL currentBufferId(void* context);
    static int32_t NPP_PLUGIN_CALL currentView(void* context);
    static size_t NPP_PLUGIN_CALL copyCurrentDocument(
        void* context, uint8_t* output, size_t capacity);
    static int NPP_PLUGIN_CALL replaceCurrentDocument(
        void* context, const uint8_t* data, size_t size);
    static size_t NPP_PLUGIN_CALL copyCurrentSelection(
        void* context, uint8_t* output, size_t capacity,
        int64_t* start, int64_t* end);
    static int NPP_PLUGIN_CALL replaceCurrentSelection(
        void* context, const uint8_t* data, size_t size);
    static int NPP_PLUGIN_CALL setCurrentSelection(
        void* context, int64_t start, int64_t end);
    static int NPP_PLUGIN_CALL createDocument(
        void* context, const uint8_t* data, size_t size);
    static size_t NPP_PLUGIN_CALL copyClipboardText(
        void* context, char* output, size_t capacity);
    static int NPP_PLUGIN_CALL setClipboardText(
        void* context, const char* text);
    static void NPP_PLUGIN_CALL setStatusText(
        void* context, const char* text);
    static size_t NPP_PLUGIN_CALL copyViewDocument(
        void* context, int32_t view, uint8_t* output, size_t capacity);
    static int NPP_PLUGIN_CALL showBufferInView(
        void* context, uint64_t bufferId, int32_t view);
    static void NPP_PLUGIN_CALL clearCompareMarks(void* context, int32_t view);
    static int NPP_PLUGIN_CALL addCompareMark(
        void* context, int32_t view, int64_t line, uint32_t kind);
    static int64_t NPP_PLUGIN_CALL firstVisibleLine(void* context, int32_t view);
    static int NPP_PLUGIN_CALL setFirstVisibleLine(
        void* context, int32_t view, int64_t line);
    static int NPP_PLUGIN_CALL gotoLine(
        void* context, int32_t view, int64_t line);
    static intptr_t NPP_PLUGIN_CALL sendScintilla(
        void* context, int32_t view, uint32_t message,
        uintptr_t wParam, intptr_t lParam);
    static size_t NPP_PLUGIN_CALL copyBufferFilePath(
        void* context, uint64_t bufferId, char* output, size_t capacity);
    static int NPP_PLUGIN_CALL saveCurrentFile(void* context);
    static int NPP_PLUGIN_CALL executeMenuCommand(
        void* context, int32_t commandId);
    static int NPP_PLUGIN_CALL saveFileAs(
        void* context, const char* path, int asCopy);
    static int NPP_PLUGIN_CALL saveSession(void* context, const char* path);
    static int NPP_PLUGIN_CALL loadSession(void* context, const char* path);
    static int32_t NPP_PLUGIN_CALL bufferPosition(
        void* context, uint64_t bufferId, int32_t priorityView);
    static int32_t NPP_PLUGIN_CALL openFileCount(void* context, int32_t scope);
    static int32_t NPP_PLUGIN_CALL currentDocumentIndex(
        void* context, int32_t view);
    static int NPP_PLUGIN_CALL activateDocument(
        void* context, int32_t view, int32_t index);
    static int32_t NPP_PLUGIN_CALL currentLine(void* context);
    static int32_t NPP_PLUGIN_CALL bufferEncoding(
        void* context, uint64_t bufferId);
    static int NPP_PLUGIN_CALL setBufferEncoding(
        void* context, uint64_t bufferId, int32_t encoding);
    static int NPP_PLUGIN_CALL setCurrentLanguage(
        void* context, int32_t languageType);
    static void NPP_PLUGIN_CALL setStatusBarText(
        void* context, int32_t section, const char* text);
    static uint64_t NPP_PLUGIN_CALL bufferAt(
        void* context, int32_t view, int32_t index);
    static int32_t NPP_PLUGIN_CALL currentLanguage(void* context);
    void notify(const LoadedPlugin& plugin,
                const NppPluginNotification& notification) const;

    PluginHostServices* _hostServices = nullptr;
    QVector<LoadedPlugin*> _plugins;
    QSet<QString> _loadedPaths;
    bool _readySent = false;
};
