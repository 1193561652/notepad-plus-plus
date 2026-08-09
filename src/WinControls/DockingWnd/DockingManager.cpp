#include "WinControls/DockingWnd/DockingManager.h"

#include <QDockWidget>
#include <QMainWindow>
#include <QObject>
#include <QVariant>
#include <QWidget>

DockingManager::DockingManager()
{
    for (int i = 0; i < DOCKCONT_MAX; ++i) {
        DockingCont container;
        container.id = i;
        container.area = areaForContainer(i);
        _containers.append(container);
    }
}

DockingManager::~DockingManager()
{
    destroy();
}

void DockingManager::init(QMainWindow* mainWindow, QWidget* clientWindow)
{
    _mainWindow = mainWindow;
    _clientWindow = clientWindow;
}

void DockingManager::destroy()
{
    if (_mainWindow) {
        for (QDockWidget* dock : dockWidgets()) {
            if (dock)
                QObject::disconnect(dock, nullptr, _mainWindow, nullptr);
        }
    }
    _containers.clear();
    _mainWindow = nullptr;
    _clientWindow = nullptr;
    _stateRestored = false;
}

QDockWidget* DockingManager::createDockableDlg(
    const DockingData& data, int iCont, bool isVisible)
{
    if (!_mainWindow || !data.hClient)
        return nullptr;
    if (QDockWidget* existing = dockForClient(data.hClient))
        return existing;

    const QString title = data.pszAddInfo.isEmpty()
        ? data.pszName : data.pszName + data.pszAddInfo;
    QDockWidget* dock = new QDockWidget(title, _mainWindow);
    dock->setObjectName(data.objectName);
    dock->setWidget(data.hClient);
    dock->setAllowedAreas(data.allowedAreas);
    if (data.minimumWidth > 0)
        dock->setMinimumWidth(data.minimumWidth);
    if ((data.uMask & DWS_ICONTAB) && !data.hIconTab.isNull())
        dock->setWindowIcon(data.hIconTab);
    dock->setProperty("dockingDialogId", data.dlgID);
    dock->setProperty("dockingModuleName", data.pszModuleName);

    int container = normalizedContainer(data, iCont);
    const bool restored = _stateRestored
        && _mainWindow->restoreDockWidget(dock);
    if (!restored) {
        _mainWindow->addDockWidget(areaForContainer(container), dock);
        if (!_containers[container].docks.isEmpty()) {
            _mainWindow->tabifyDockWidget(
                _containers[container].docks.constLast(), dock);
        }
        if (data.uMask & DWS_DF_FLOATING)
            dock->setFloating(true);
        dock->setVisible(isVisible);
    } else if (!dock->isFloating()) {
        const int restoredContainer = containerForArea(
            _mainWindow->dockWidgetArea(dock));
        if (restoredContainer >= 0)
            container = restoredContainer;
    }
    _containers[container].docks.append(dock);

    QObject::connect(dock, &QDockWidget::dockLocationChanged, _mainWindow,
                     [this, dock](Qt::DockWidgetArea) {
        if (dock->widget())
            updateContainerInfo(dock->widget());
    });
    QObject::connect(dock, &QObject::destroyed, _mainWindow,
                     [this, dock]() { removeFromContainers(dock); });
    return dock;
}

bool DockingManager::removeDockableDlg(QWidget* hClient)
{
    QDockWidget* dock = dockForClient(hClient);
    if (!dock || !_mainWindow)
        return false;
    removeFromContainers(dock);
    _mainWindow->removeDockWidget(dock);
    delete dock;
    return true;
}

bool DockingManager::setDockableDlgTitle(
    QWidget* hClient, const QString& title)
{
    QDockWidget* dock = dockForClient(hClient);
    if (!dock)
        return false;
    dock->setWindowTitle(title);
    return true;
}

void DockingManager::updateContainerInfo(QWidget* hClient)
{
    QDockWidget* dock = dockForClient(hClient);
    if (!dock || !_mainWindow || dock->isFloating())
        return;
    const int container = containerForArea(_mainWindow->dockWidgetArea(dock));
    if (container < 0)
        return;
    removeFromContainers(dock);
    _containers[container].docks.append(dock);
}

void DockingManager::setActiveTab(int iCont, int iItem)
{
    if (iCont < 0 || iCont >= _containers.size()
        || iItem < 0 || iItem >= _containers[iCont].docks.size()) {
        return;
    }
    QDockWidget* dock = _containers[iCont].docks.at(iItem);
    dock->show();
    dock->raise();
}

void DockingManager::showDockableDlg(QWidget* hClient, bool view)
{
    if (QDockWidget* dock = dockForClient(hClient)) {
        dock->setVisible(view);
        if (view) {
            dock->raise();
            for (int i = 0; i < _containers.size(); ++i) {
                if (_containers[i].docks.contains(dock)
                    && _dockedSizes[i] > 0) {
                    setDockedContSize(i, _dockedSizes[i]);
                    break;
                }
            }
        }
    }
}

void DockingManager::showDockableDlg(const QString& name, bool view)
{
    if (QDockWidget* dock = dockByName(name))
        showDockableDlg(dock->widget(), view);
}

void DockingManager::toggleDockableDlg(QWidget* hClient)
{
    if (QDockWidget* dock = dockForClient(hClient))
        showDockableDlg(hClient, !dock->isVisible());
}

QDockWidget* DockingManager::dockForClient(QWidget* hClient) const
{
    if (!hClient)
        return nullptr;
    for (const DockingCont& container : _containers) {
        for (QDockWidget* dock : container.docks) {
            if (dock && dock->widget() == hClient)
                return dock;
        }
    }
    return nullptr;
}

QDockWidget* DockingManager::dockByName(const QString& name) const
{
    for (QDockWidget* dock : dockWidgets()) {
        if (dock && (dock->windowTitle() == name
            || dock->objectName() == name)) {
            return dock;
        }
    }
    return nullptr;
}

QVector<QDockWidget*> DockingManager::dockWidgets() const
{
    QVector<QDockWidget*> result;
    for (const DockingCont& container : _containers) {
        for (QDockWidget* dock : container.docks) {
            if (dock && !result.contains(dock))
                result.append(dock);
        }
    }
    return result;
}

int DockingManager::getDockedContSize(int iCont) const
{
    if (iCont < 0 || iCont >= _containers.size())
        return 0;
    int size = _dockedSizes[iCont];
    for (QDockWidget* dock : _containers[iCont].docks) {
        if (!dock || dock->isFloating() || !dock->isVisible())
            continue;
        size = qMax(size, iCont == CONT_LEFT || iCont == CONT_RIGHT
                              ? dock->width() : dock->height());
    }
    return size;
}

void DockingManager::setDockedContSize(int iCont, int iSize)
{
    if (!_mainWindow || iCont < 0 || iCont >= _containers.size()
        || iSize <= 0) {
        return;
    }
    _dockedSizes[iCont] = iSize;
    QList<QDockWidget*> visible;
    for (QDockWidget* dock : _containers[iCont].docks) {
        if (dock && dock->isVisible() && !dock->isFloating())
            visible.append(dock);
    }
    if (!visible.isEmpty()) {
        QList<int> sizes;
        for (int i = 0; i < visible.size(); ++i)
            sizes.append(iSize);
        _mainWindow->resizeDocks(
            visible, sizes,
            iCont == CONT_LEFT || iCont == CONT_RIGHT
                ? Qt::Horizontal : Qt::Vertical);
    }
}

void DockingManager::resize()
{
    if (_mainWindow)
        _mainWindow->updateGeometry();
    if (_clientWindow)
        _clientWindow->updateGeometry();
}

QByteArray DockingManager::saveState(int version) const
{
    return _mainWindow ? _mainWindow->saveState(version) : QByteArray();
}

bool DockingManager::restoreState(const QByteArray& state, int version)
{
    _stateRestored = _mainWindow && !state.isEmpty()
        && _mainWindow->restoreState(state, version);
    return _stateRestored;
}

Qt::DockWidgetArea DockingManager::areaForContainer(int iCont)
{
    switch (iCont) {
        case CONT_RIGHT: return Qt::RightDockWidgetArea;
        case CONT_TOP: return Qt::TopDockWidgetArea;
        case CONT_BOTTOM: return Qt::BottomDockWidgetArea;
        case CONT_LEFT:
        default: return Qt::LeftDockWidgetArea;
    }
}

int DockingManager::containerForArea(Qt::DockWidgetArea area)
{
    switch (area) {
        case Qt::LeftDockWidgetArea: return CONT_LEFT;
        case Qt::RightDockWidgetArea: return CONT_RIGHT;
        case Qt::TopDockWidgetArea: return CONT_TOP;
        case Qt::BottomDockWidgetArea: return CONT_BOTTOM;
        default: return -1;
    }
}

int DockingManager::normalizedContainer(
    const DockingData& data, int iCont) const
{
    if (iCont >= 0 && iCont < DOCKCONT_MAX)
        return iCont;
    const int masked = static_cast<int>((data.uMask >> 28) & 0x3);
    return masked >= 0 && masked < DOCKCONT_MAX ? masked : CONT_LEFT;
}

void DockingManager::removeFromContainers(QDockWidget* dock)
{
    for (DockingCont& container : _containers)
        container.docks.removeAll(dock);
}
