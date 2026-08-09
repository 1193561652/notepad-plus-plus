#include "Win32PluginSystem/Win32PluginDockAdapter.h"

#include "Win32PluginSystem/Win32NativeDockHost.h"
#include "Win32PluginSystem/Win32PluginInterface.h"
#include "WinControls/DockingWnd/DockingManager.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QDockWidget>
#include <QMainWindow>

namespace {

QString stableDockObjectName(const QString& moduleName, int dialogId)
{
    QString module = moduleName.toLower();
    for (int index = 0; index < module.size(); ++index) {
        if (!module.at(index).isLetterOrNumber())
            module[index] = QLatin1Char('_');
    }
    if (module.isEmpty())
        module = QStringLiteral("unknown");
    return QStringLiteral("Win32PluginDock_%1_%2")
        .arg(module).arg(dialogId);
}

} // namespace

Win32PluginDockAdapter::Win32PluginDockAdapter(
    QMainWindow* mainWindow, DockingManager* dockingManager,
    HWND mainWindowHandle)
    : QObject(mainWindow), _mainWindow(mainWindow),
      _dockingManager(dockingManager), _mainWindowHandle(mainWindowHandle)
{
    Q_ASSERT(_mainWindow);
    Q_ASSERT(_dockingManager);
    Q_ASSERT(_mainWindowHandle);
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->installNativeEventFilter(this);
        _nativeFilterInstalled = true;
    }
}

Win32PluginDockAdapter::~Win32PluginDockAdapter()
{
    if (_nativeFilterInstalled && QCoreApplication::instance())
        QCoreApplication::instance()->removeNativeEventFilter(this);
    releaseAll();
}

LRESULT Win32PluginDockAdapter::handleMessage(
    UINT message, WPARAM wParam, LPARAM lParam, bool* handled)
{
    if (handled)
        *handled = true;

    if (message == NppMessageModelessDialog) {
        HWND dialog = reinterpret_cast<HWND>(lParam);
        if (wParam == Win32ModelessDialogAdd) {
            if (!dialog || _modelessDialogs.contains(dialog))
                return 0;
            _modelessDialogs.append(dialog);
            return lParam;
        }
        if (wParam == Win32ModelessDialogRemove) {
            const int index = _modelessDialogs.indexOf(dialog);
            if (index < 0)
                return lParam;
            _modelessDialogs.remove(index);
            return 0;
        }
        return TRUE;
    }

    if (message == NppMessageDmmRegisterAsDockDialog)
        return registerDock(reinterpret_cast<Win32DockingData*>(lParam));

    if (message == NppMessageDmmViewOtherTab) {
        const QString name = lParam
            ? QString::fromWCharArray(reinterpret_cast<const wchar_t*>(lParam))
            : QString();
        for (Entry& entry : _entries) {
            if (entry.name.compare(name, Qt::CaseInsensitive) == 0) {
                _dockingManager->showDockableDlg(entry.host, true);
                return TRUE;
            }
        }
        return TRUE;
    }

    HWND client = reinterpret_cast<HWND>(lParam);
    Entry* entry = entryForClient(client);
    if (message == NppMessageDmmShow || message == NppMessageDmmHide) {
        if (entry)
            _dockingManager->showDockableDlg(
                entry->host, message == NppMessageDmmShow);
        return TRUE;
    }
    if (message == NppMessageDmmUpdateDisplayInfo) {
        if (entry)
            updateDisplayInfo(*entry);
        return TRUE;
    }
    if (message == NppMessageDmmGetPluginHwndByName) {
        const QString windowName = wParam
            ? QString::fromWCharArray(reinterpret_cast<const wchar_t*>(wParam))
            : QString();
        const QString moduleName = lParam
            ? QString::fromWCharArray(reinterpret_cast<const wchar_t*>(lParam))
            : QString();
        for (const Entry& item : _entries) {
            if (item.name.compare(windowName, Qt::CaseInsensitive) == 0
                && item.moduleName.compare(
                       moduleName, Qt::CaseInsensitive) == 0) {
                return reinterpret_cast<LRESULT>(item.client);
            }
        }
        return 0;
    }

    if (handled)
        *handled = false;
    return 0;
}

bool Win32PluginDockAdapter::nativeEventFilter(
    const QByteArray&, void* message, long* result)
{
    MSG* nativeMessage = reinterpret_cast<MSG*>(message);
    if (!nativeMessage)
        return false;
    for (int i = _modelessDialogs.size() - 1; i >= 0; --i) {
        HWND dialog = _modelessDialogs.at(i);
        if (!IsWindow(dialog)) {
            _modelessDialogs.remove(i);
            continue;
        }
        if (IsDialogMessageW(dialog, nativeMessage)) {
            if (result)
                *result = 0;
            return true;
        }
    }
    return false;
}

bool Win32PluginDockAdapter::eventFilter(QObject* watched, QEvent* event)
{
    Entry* entry = entryForDock(watched);
    if (!entry || _releasing)
        return QObject::eventFilter(watched, event);
    if (event->type() == QEvent::Close) {
        if (notifyClient(*entry, Win32DockNotifyClose) != 0) {
            static_cast<QCloseEvent*>(event)->ignore();
            return true;
        }
    } else if (event->type() == QEvent::Show) {
        notifyClient(*entry, Win32DockNotifySwitchIn);
    } else if (event->type() == QEvent::Hide) {
        notifyClient(*entry, Win32DockNotifySwitchOff);
    }
    return QObject::eventFilter(watched, event);
}

void Win32PluginDockAdapter::releaseAll()
{
    if (_entries.isEmpty())
        return;
    _releasing = true;
    while (!_entries.isEmpty()) {
        Entry entry = _entries.takeLast();
        if (entry.host)
            _dockingManager->removeDockableDlg(entry.host);
    }
    _modelessDialogs.clear();
    _releasing = false;
}

int Win32PluginDockAdapter::nativeHostCount() const
{
    int count = 0;
    for (const Entry& entry : _entries) {
        if (entry.host && entry.host->hostHandle())
            ++count;
    }
    return count;
}

int Win32PluginDockAdapter::promotedAncestorCount() const
{
    int count = 0;
    for (const Entry& entry : _entries) {
        if (entry.host)
            count += entry.host->promotedAncestorCount();
    }
    return count;
}

Win32NativeDockHost* Win32PluginDockAdapter::hostForClient(HWND client) const
{
    const Entry* entry = entryForClient(client);
    return entry ? entry->host.data() : nullptr;
}

Win32PluginDockAdapter::Entry* Win32PluginDockAdapter::entryForClient(
    HWND client)
{
    for (Entry& entry : _entries) {
        if (entry.client == client)
            return &entry;
    }
    return nullptr;
}

const Win32PluginDockAdapter::Entry*
Win32PluginDockAdapter::entryForClient(HWND client) const
{
    for (const Entry& entry : _entries) {
        if (entry.client == client)
            return &entry;
    }
    return nullptr;
}

Win32PluginDockAdapter::Entry* Win32PluginDockAdapter::entryForDock(
    QObject* dock)
{
    for (Entry& entry : _entries) {
        if (entry.dock == dock)
            return &entry;
    }
    return nullptr;
}

bool Win32PluginDockAdapter::registerDock(const Win32DockingData* source)
{
    if (!source || !source->hClient || !IsWindow(source->hClient)
        || entryForClient(source->hClient)) {
        return false;
    }

    Entry entry;
    entry.client = source->hClient;
    entry.source = source;
    entry.name = source->pszName
        ? QString::fromWCharArray(source->pszName) : QString();
    entry.addInfo = source->pszAddInfo
        ? QString::fromWCharArray(source->pszAddInfo) : QString();
    entry.moduleName = source->pszModuleName
        ? QString::fromWCharArray(source->pszModuleName) : QString();
    entry.mask = source->uMask;
    entry.dialogId = source->dlgID;
    entry.container = static_cast<int>((source->uMask >> 28) & 0x3);

    Win32NativeDockHost* host = new Win32NativeDockHost;
    // Create the one required native Qt host before inserting it into the
    // alien QWidget hierarchy. Qt can then reparent that HWND without
    // promoting QDockWidget or its internal layout widgets.
    if (!host->attachClient(entry.client)) {
        delete host;
        return false;
    }
    DockingData data;
    data.hClient = host;
    data.pszName = entry.name;
    data.pszAddInfo = (entry.mask & Win32DwsAddInfo)
        ? entry.addInfo : QString();
    data.dlgID = entry.dialogId;
    // Attach the native child while the Qt dock is still docked. A floating
    // QDockWidget must become a native top-level window, but it must not be
    // mistaken for an ancestor promoted by the HWND host itself.
    data.uMask = entry.mask & ~Win32DwsDefaultFloating;
    data.pszModuleName = entry.moduleName;
    data.objectName = stableDockObjectName(
        entry.moduleName, entry.dialogId);
    QDockWidget* dock = _dockingManager->createDockableDlg(
        data, entry.container, true);
    if (!dock) {
        delete host;
        return false;
    }

    entry.host = host;
    entry.dock = dock;
    _entries.append(entry);
    dock->installEventFilter(this);
    connect(dock, &QDockWidget::topLevelChanged, this,
            [this, client = entry.client](bool floating) {
        Entry* current = entryForClient(client);
        if (!current || _releasing)
            return;
        const int container = current->dock && !floating
            ? containerForArea(_mainWindow->dockWidgetArea(current->dock))
            : current->container;
        notifyClient(*current, MAKELONG(
            floating ? Win32DockNotifyFloat : Win32DockNotifyDock,
            container));
    });
    connect(dock, &QDockWidget::dockLocationChanged, this,
            [this, client = entry.client](Qt::DockWidgetArea area) {
        Entry* current = entryForClient(client);
        if (current)
            current->container = containerForArea(area);
    });
    if ((entry.mask & Win32DwsDefaultFloating) != 0) {
        dock->setFloating(true);
        const RECT& rect = source->rcFloat;
        if (rect.right > rect.left && rect.bottom > rect.top)
            dock->setGeometry(rect.left, rect.top,
                              rect.right - rect.left,
                              rect.bottom - rect.top);
    }
    // A newly tabified dock can otherwise remain behind the previously active
    // dock even though the registering plugin considers its panel open.
    _dockingManager->showDockableDlg(entry.host, true);
    return true;
}

void Win32PluginDockAdapter::updateDisplayInfo(Entry& entry)
{
    if (entry.source) {
        entry.name = entry.source->pszName
            ? QString::fromWCharArray(entry.source->pszName) : QString();
        entry.addInfo = entry.source->pszAddInfo
            ? QString::fromWCharArray(entry.source->pszAddInfo) : QString();
        entry.mask = entry.source->uMask;
    }
    const QString title = (entry.mask & Win32DwsAddInfo)
        ? entry.name + entry.addInfo : entry.name;
    if (entry.host)
        _dockingManager->setDockableDlgTitle(entry.host, title);
}

LRESULT Win32PluginDockAdapter::notifyClient(
    const Entry& entry, UINT code) const
{
    if (!entry.client || !IsWindow(entry.client))
        return 0;
    NMHDR notification{};
    notification.code = code;
    notification.hwndFrom = _mainWindowHandle;
    notification.idFrom = static_cast<UINT_PTR>(
        GetDlgCtrlID(_mainWindowHandle));
    SendMessageW(entry.client, WM_NOTIFY, notification.idFrom,
                 reinterpret_cast<LPARAM>(&notification));
    return GetWindowLongPtrW(entry.client, DWLP_MSGRESULT);
}

int Win32PluginDockAdapter::containerForArea(Qt::DockWidgetArea area)
{
    switch (area) {
        case Qt::RightDockWidgetArea: return Win32DockContainerRight;
        case Qt::TopDockWidgetArea: return Win32DockContainerTop;
        case Qt::BottomDockWidgetArea: return Win32DockContainerBottom;
        case Qt::LeftDockWidgetArea:
        default: return Win32DockContainerLeft;
    }
}
