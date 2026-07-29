// DocumentMapPanel.h - 文档地图面板
// 移植自: v8.4.6:PowerEditor/src/WinControls/DocumentMap/

#ifndef DOCUMENTMAPPANEL_H
#define DOCUMENTMAPPANEL_H

#include <QWidget>
#include "ScintillaComponent/ScintillaEditView.h"
#include "ScintillaComponent/Buffer.h"

// 半透明当前位置指示器（叠加在缩略图上）
class MapIndicator : public QWidget
{
public:
    explicit MapIndicator(QWidget* parent = nullptr);
protected:
    void paintEvent(QPaintEvent* event) override;
};

class DocumentMapPanel : public QWidget
{
    Q_OBJECT

public:
    explicit DocumentMapPanel(QWidget* parent = nullptr);
    ~DocumentMapPanel() = default;

    // 切换跟踪的主编辑视图（tab 切换时由 MainWindow 调用）
    void syncWith(ScintillaEditView* mainView);
    bool isTracking(const ScintillaEditView* view) const {
        return _trackedView == view;
    }
    BufferMapState sessionState() const;
    void restoreSessionState(const BufferMapState& state);

public slots:
    void updatePositionIndicator();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    ScintillaEditView* _miniView    = nullptr;
    ScintillaEditView* _trackedView = nullptr;
    MapIndicator*      _indicator   = nullptr;
};

#endif // DOCUMENTMAPPANEL_H
