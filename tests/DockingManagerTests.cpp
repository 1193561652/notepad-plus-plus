#include "WinControls/DockingWnd/DockingManager.h"

#include <QApplication>
#include <QDockWidget>
#include <QMainWindow>
#include <QWidget>

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}

} // namespace

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QMainWindow window;
    QWidget* central = new QWidget;
    window.setCentralWidget(central);

    DockingManager manager;
    manager.init(&window, central);
    require(manager.isInitialized(), "manager was not initialized");

    QWidget* leftClient = new QWidget;
    DockingData leftData;
    leftData.hClient = leftClient;
    leftData.pszName = QStringLiteral("Left panel");
    leftData.objectName = QStringLiteral("LeftPanelDock");
    leftData.allowedAreas =
        Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea;
    QDockWidget* leftDock = manager.createDockableDlg(
        leftData, CONT_LEFT, false);
    require(leftDock && manager.dockForClient(leftClient) == leftDock,
            "left dock registration failed");
    require(window.dockWidgetArea(leftDock) == Qt::LeftDockWidgetArea,
            "left dock area is incorrect");

    QWidget* secondClient = new QWidget;
    DockingData secondData;
    secondData.hClient = secondClient;
    secondData.pszName = QStringLiteral("Second panel");
    secondData.objectName = QStringLiteral("SecondPanelDock");
    QDockWidget* secondDock = manager.createDockableDlg(
        secondData, CONT_LEFT, false);
    require(secondDock && manager.dockWidgets().size() == 2,
            "second dock registration failed");

    window.show();
    manager.showDockableDlg(leftClient, true);
    manager.showDockableDlg(secondClient, true);
    QApplication::processEvents();
    require(window.tabifiedDockWidgets(leftDock).contains(secondDock),
            "same-side docks were not placed in one tab container");
    require(leftDock->isVisible(), "showDockableDlg did not show the dock");
    manager.toggleDockableDlg(leftClient);
    require(!leftDock->isVisible(), "toggleDockableDlg did not hide the dock");
    manager.showDockableDlg(QStringLiteral("LeftPanelDock"), true);
    require(leftDock->isVisible(), "dock lookup by name failed");

    window.addDockWidget(Qt::RightDockWidgetArea, leftDock);
    QApplication::processEvents();
    manager.updateContainerInfo(leftClient);
    require(manager.getContainerInfo().at(CONT_RIGHT).docks.contains(leftDock),
            "dock container metadata was not updated");

    manager.setDockedContSize(CONT_RIGHT, 240);
    require(manager.getDockedContSize(CONT_RIGHT) >= 240,
            "dock size was not retained");
    const QByteArray state = manager.saveState();
    require(!state.isEmpty() && manager.restoreState(state),
            "dock state round trip failed");

    require(manager.setDockableDlgTitle(
                secondClient, QStringLiteral("Renamed panel"))
            && secondDock->windowTitle() == QStringLiteral("Renamed panel"),
            "dock title update failed");
    require(manager.removeDockableDlg(secondClient)
            && manager.dockForClient(secondClient) == nullptr,
            "dock removal failed");

    QMainWindow lateWindow;
    QWidget* lateCentral = new QWidget;
    lateWindow.setCentralWidget(lateCentral);
    DockingManager lateManager;
    lateManager.init(&lateWindow, lateCentral);
    require(lateManager.restoreState(state),
            "state restore before late registration failed");
    QWidget* lateClient = new QWidget;
    DockingData lateData;
    lateData.hClient = lateClient;
    lateData.pszName = QStringLiteral("Left panel");
    lateData.objectName = QStringLiteral("LeftPanelDock");
    QDockWidget* lateDock = lateManager.createDockableDlg(
        lateData, CONT_LEFT, false);
    QApplication::processEvents();
    require(lateDock
            && lateWindow.dockWidgetArea(lateDock)
                == Qt::RightDockWidgetArea,
            "late dock registration did not restore its saved area");
    return 0;
}
