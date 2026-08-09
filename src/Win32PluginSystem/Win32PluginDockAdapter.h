#pragma once

#include <QtGlobal>

#ifndef Q_OS_WIN
#error Win32PluginDockAdapter is available only on Windows.
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <QAbstractNativeEventFilter>
#include <QObject>
#include <QPointer>
#include <QString>
#include <Qt>
#include <QVector>

class DockingManager;
class QDockWidget;
class QMainWindow;
class Win32NativeDockHost;
struct Win32DockingData;

class Win32PluginDockAdapter final : public QObject,
                                     public QAbstractNativeEventFilter
{
public:
    Win32PluginDockAdapter(QMainWindow* mainWindow,
                           DockingManager* dockingManager,
                           HWND mainWindowHandle);
    ~Win32PluginDockAdapter() override;

    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam,
                          bool* handled);
    bool nativeEventFilter(const QByteArray& eventType, void* message,
                           long* result) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

    void releaseAll();
    int dockCount() const { return _entries.size(); }
    int nativeHostCount() const;
    int promotedAncestorCount() const;
    Win32NativeDockHost* hostForClient(HWND client) const;

private:
    struct Entry {
        HWND client = nullptr;
        const Win32DockingData* source = nullptr;
        QString name;
        QString addInfo;
        QString moduleName;
        UINT mask = 0;
        int dialogId = 0;
        int container = 0;
        QPointer<Win32NativeDockHost> host;
        QPointer<QDockWidget> dock;
    };

    Entry* entryForClient(HWND client);
    const Entry* entryForClient(HWND client) const;
    Entry* entryForDock(QObject* dock);
    bool registerDock(const Win32DockingData* source);
    void updateDisplayInfo(Entry& entry);
    LRESULT notifyClient(const Entry& entry, UINT code) const;
    static int containerForArea(Qt::DockWidgetArea area);

    QMainWindow* _mainWindow = nullptr;
    DockingManager* _dockingManager = nullptr;
    HWND _mainWindowHandle = nullptr;
    QVector<Entry> _entries;
    QVector<HWND> _modelessDialogs;
    bool _nativeFilterInstalled = false;
    bool _releasing = false;
};
