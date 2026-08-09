#pragma once

#include <QByteArray>
#include <QIcon>
#include <QString>
#include <QVector>
#include <Qt>

class QDockWidget;
class QMainWindow;
class QWidget;

constexpr int CONT_LEFT = 0;
constexpr int CONT_RIGHT = 1;
constexpr int CONT_TOP = 2;
constexpr int CONT_BOTTOM = 3;
constexpr int DOCKCONT_MAX = 4;

constexpr quint32 DWS_ICONTAB = 0x00000001;
constexpr quint32 DWS_ADDINFO = 0x00000004;
constexpr quint32 DWS_DF_CONT_LEFT = CONT_LEFT << 28;
constexpr quint32 DWS_DF_CONT_RIGHT = CONT_RIGHT << 28;
constexpr quint32 DWS_DF_CONT_TOP = CONT_TOP << 28;
constexpr quint32 DWS_DF_CONT_BOTTOM = CONT_BOTTOM << 28;
constexpr quint32 DWS_DF_FLOATING = 0x80000000;

struct DockingData
{
    QWidget* hClient = nullptr;
    QString pszName;
    int dlgID = 0;
    quint32 uMask = 0;
    QIcon hIconTab;
    QString pszAddInfo;
    QString pszModuleName;
    QString objectName;
    Qt::DockWidgetAreas allowedAreas = Qt::AllDockWidgetAreas;
    int minimumWidth = 0;
};

struct DockingCont
{
    int id = CONT_LEFT;
    Qt::DockWidgetArea area = Qt::LeftDockWidgetArea;
    QVector<QDockWidget*> docks;
};

class DockingManager
{
public:
    DockingManager();
    ~DockingManager();

    void init(QMainWindow* mainWindow, QWidget* clientWindow = nullptr);
    void destroy();
    bool isInitialized() const { return _mainWindow != nullptr; }

    QDockWidget* createDockableDlg(
        const DockingData& data, int iCont = CONT_LEFT,
        bool isVisible = false);
    bool removeDockableDlg(QWidget* hClient);
    bool setDockableDlgTitle(QWidget* hClient, const QString& title);
    void updateContainerInfo(QWidget* hClient);
    void setActiveTab(int iCont, int iItem);
    void showDockableDlg(QWidget* hClient, bool view);
    void showDockableDlg(const QString& name, bool view);
    void toggleDockableDlg(QWidget* hClient);

    QDockWidget* dockForClient(QWidget* hClient) const;
    QDockWidget* dockByName(const QString& name) const;
    const QVector<DockingCont>& getContainerInfo() const
        { return _containers; }
    QVector<QDockWidget*> dockWidgets() const;

    int getDockedContSize(int iCont) const;
    void setDockedContSize(int iCont, int iSize);
    void resize();

    QByteArray saveState(int version = 0) const;
    bool restoreState(const QByteArray& state, int version = 0);

private:
    static Qt::DockWidgetArea areaForContainer(int iCont);
    static int containerForArea(Qt::DockWidgetArea area);
    int normalizedContainer(const DockingData& data, int iCont) const;
    void removeFromContainers(QDockWidget* dock);

    QMainWindow* _mainWindow = nullptr;
    QWidget* _clientWindow = nullptr;
    QVector<DockingCont> _containers;
    int _dockedSizes[DOCKCONT_MAX] = {0, 0, 0, 0};
    bool _stateRestored = false;
};
