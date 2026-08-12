#pragma once

#include <QtGlobal>

#ifndef Q_OS_WIN
#error Win32PluginManager is available only on Windows.
#endif

#include <QObject>
#include <QKeySequence>
#include <QStringList>
#include <QVector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "Win32PluginSystem/Win32MainEditorAdapter.h"
#include "Win32PluginSystem/Win32MainWindowAdapter.h"
#include "Win32PluginSystem/Win32PluginDockAdapter.h"
#include "Win32PluginSystem/Win32SubEditorAdapter.h"
#include "Win32PluginSystem/Win32PluginInterface.h"
#include "MISC/PluginsManager/PluginLoadJournal.h"

class QMainWindow;
class QWidget;
class ScintillaEditView;
class DockingManager;
namespace Scintilla { struct NotificationData; }

class Win32PluginManager final : public QObject
{
public:
    Win32PluginManager(QMainWindow* mainWindow,
                       ScintillaEditView* mainEditor,
                       ScintillaEditView* subEditor,
                       const QString& pluginStateDirectory,
                       DockingManager* dockingManager);
    ~Win32PluginManager() override;

    QMainWindow* mainWindow() const { return _mainWindowAdapter.window(); }
    QWidget* mainEditorWindow() const { return _mainEditorAdapter.window(); }
    QWidget* secondaryEditorWindow() const
        { return _subEditorAdapter.window(); }

    const Win32MainWindowAdapter& mainWindowAdapter() const
        { return _mainWindowAdapter; }
    const Win32MainEditorAdapter& mainEditorAdapter() const
        { return _mainEditorAdapter; }
    const Win32SubEditorAdapter& subEditorAdapter() const
        { return _subEditorAdapter; }
    const Win32PluginDockAdapter& dockAdapter() const
        { return _dockAdapter; }

    HWND mainWindowHandle() const;
    HWND mainEditorHandle() const;
    HWND secondaryEditorHandle() const;

    bool loadPlugin(const QString& filePath, QString* errorMessage = nullptr);
    int loadPlugins(const QString& pluginRoot,
                    const QStringList& folderFilter = QStringList(),
                    QStringList* errors = nullptr);
    int loadedPluginCount() const { return _loadedPlugins.size(); }
    QStringList loadedPluginNames() const;
    QString recoveredPluginFolder() const { return _recoveredPluginFolder; }
    QString loadJournalPath() const { return _loadJournal.journalPath(); }
    QString loadMarkerPath() const { return _loadJournal.markerPath(); }
    int loadedPluginFunctionCount(int pluginIndex) const;
    QString loadedPluginFunctionName(int pluginIndex, int functionIndex) const;
    int loadedPluginFunctionCommandId(int pluginIndex, int functionIndex) const;
    bool isLoadedPluginFunctionInitiallyChecked(
        int pluginIndex, int functionIndex) const;
    QKeySequence loadedPluginFunctionShortcut(
        int pluginIndex, int functionIndex) const;
    bool isLoadedPluginFunctionSeparator(int pluginIndex,
                                         int functionIndex) const;
    bool executePluginCommand(int pluginIndex, int functionIndex,
                              QString* errorMessage = nullptr);
    LRESULT relayMessage(UINT message, WPARAM wParam, LPARAM lParam) const;
    void notifyScintilla(const Scintilla::NotificationData& notification,
                         bool fromMainEditor);
    void notifyReady();
    void notifyFileBeforeLoad();
    void notifyFileBeforeOpen(quintptr bufferId);
    void notifyFileOpened(quintptr bufferId);
    void notifyFileLoadFailed(quintptr bufferId);
    void notifyFileBeforeClose(quintptr bufferId);
    void notifyFileClosed(quintptr bufferId);
    void notifyFileBeforeSave(quintptr bufferId);
    void notifyFileSaved(quintptr bufferId);
    void notifyBufferActivated(quintptr bufferId);
    void notifyLanguageChanged(quintptr bufferId);
    void notifyWordStylesUpdated(quintptr bufferId);
    void notifyReadOnlyChanged(quintptr bufferId, bool readOnly, bool dirty);
    void notifyDarkModeChanged();
    void notifyBeforeShutdown();
    void notifyCancelShutdown();

private:
    struct LoadedPlugin {
        HMODULE module = nullptr;
        QString filePath;
        QString name;
        PluginBeNotified beNotified = nullptr;
        PluginMessageProc messageProc = nullptr;
        FuncItem* functions = nullptr;
        int functionCount = 0;
    };

    void unloadPlugins();
    void notifyPlugins(unsigned int code, quintptr idFrom = 0,
                       HWND hwndFrom = nullptr);
    void setEditorAbiForPlugin(const LoadedPlugin& plugin);
    bool executeMimeToolsSamlDecode(QString* errorMessage);
    void showMimeToolsAbout();
    void showJsonViewerAbout();

    Win32PluginDockAdapter _dockAdapter;
    Win32MainWindowAdapter _mainWindowAdapter;
    HWND _mainEditorReceiver = nullptr;
    HWND _subEditorReceiver = nullptr;
    Win32MainEditorAdapter _mainEditorAdapter;
    Win32SubEditorAdapter _subEditorAdapter;
    PluginLoadJournal _loadJournal;
    QVector<LoadedPlugin> _loadedPlugins;
    QString _recoveredPluginFolder;
    int _nextCommandId = 50000;
};
