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
    void unloadAll();

    int loadedPluginCount() const { return _plugins.size(); }
    QStringList loadedPluginNames() const;
    QStringList loadedPluginFolders() const;
    int loadedPluginFunctionCount(int pluginIndex) const;
    QString loadedPluginFunctionName(int pluginIndex, int functionIndex) const;
    bool isLoadedPluginFunctionInitiallyChecked(
        int pluginIndex, int functionIndex) const;
    QKeySequence loadedPluginFunctionShortcut(
        int pluginIndex, int functionIndex) const;
    bool executePluginCommand(int pluginIndex, int functionIndex,
                              QString* errorMessage = nullptr);
    qintptr sendPluginMessage(int pluginIndex, quint32 message,
                              quintptr wParam = 0, qintptr lParam = 0) const;

private:
    struct LoadedPlugin {
        QLibrary* library = nullptr;
        QString filePath;
        QString folderName;
        QString name;
        NppBeNotifiedFn beNotified = nullptr;
        NppMessageProcFn messageProc = nullptr;
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
    void notify(const LoadedPlugin& plugin, uint32_t code) const;

    PluginHostServices* _hostServices = nullptr;
    QVector<LoadedPlugin*> _plugins;
    QSet<QString> _loadedPaths;
    bool _readySent = false;
};
