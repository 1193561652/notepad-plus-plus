#pragma once

#include <QtGlobal>

#ifndef Q_OS_WIN
#error Win32PluginManager is available only on Windows.
#endif

#include <QObject>
#include <QStringList>
#include <QVector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "Win32PluginSystem/Win32MainEditorAdapter.h"
#include "Win32PluginSystem/Win32MainWindowAdapter.h"
#include "Win32PluginSystem/Win32SubEditorAdapter.h"
#include "Win32PluginSystem/Win32PluginInterface.h"

class QMainWindow;
class QWidget;
class ScintillaEditView;

class Win32PluginManager final : public QObject
{
public:
    Win32PluginManager(QMainWindow* mainWindow,
                       ScintillaEditView* mainEditor,
                       ScintillaEditView* subEditor);
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

    HWND mainWindowHandle() const;
    HWND mainEditorHandle() const;
    HWND secondaryEditorHandle() const;

    bool loadPlugin(const QString& filePath, QString* errorMessage = nullptr);
    int loadPlugins(const QString& pluginRoot,
                    const QStringList& folderFilter = QStringList(),
                    QStringList* errors = nullptr);
    int loadedPluginCount() const { return _loadedPlugins.size(); }
    QStringList loadedPluginNames() const;
    int loadedPluginFunctionCount(int pluginIndex) const;
    bool executePluginCommand(int pluginIndex, int functionIndex,
                              QString* errorMessage = nullptr);
    LRESULT relayMessage(UINT message, WPARAM wParam, LPARAM lParam) const;

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

    Win32MainWindowAdapter _mainWindowAdapter;
    HWND _mainEditorReceiver = nullptr;
    HWND _subEditorReceiver = nullptr;
    Win32MainEditorAdapter _mainEditorAdapter;
    Win32SubEditorAdapter _subEditorAdapter;
    QVector<LoadedPlugin> _loadedPlugins;
};
