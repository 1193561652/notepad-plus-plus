// DocTabView.h - 文档标签页视图
// 移植自: v8.4.6:PowerEditor/src/WinControls/TabBar/DocTabView.h

#ifndef DOCTABVIEW_H
#define DOCTABVIEW_H

#include <QWidget>
#include <QHash>
#include "ScintillaComponent/Buffer.h"

class QTabBar;

class DocTabView : public QWidget
{
    Q_OBJECT

public:
    explicit DocTabView(QWidget* parent = nullptr);
    ~DocTabView() = default;

    // 添加标签页（buffer 必须已关联 view）
    void addBuffer(Buffer* buf);
    void addBufferView(Buffer* buf, ScintillaEditView* view);

    // 添加克隆标签页（cloneView 与原 view 共享同一 Scintilla document）
    void addClone(Buffer* buf, ScintillaEditView* cloneView);

    // 关闭并移除标签页（不销毁 buffer，由调用方决定）
    void removeBuffer(Buffer* buf);
    void removeTab(int index);

    // 切换到指定缓冲区
    void activateBuffer(Buffer* buf);

    // 获取当前活跃的缓冲区
    Buffer* currentBuffer() const;

    // 按索引获取缓冲区
    Buffer* bufferAt(int index) const;

    // 查找缓冲区对应的标签索引，找不到返回 -1
    int indexOfBuffer(Buffer* buf) const;
    int count() const;
    int currentIndex() const;
    void setCurrentIndex(int index);
    QString tabText(int index) const;
    QTabBar* tabBar() const { return _tabBar; }
    ScintillaEditView* editor() const { return _editor; }
    QWidget* currentWidget() const;
    QWidget* widget(int index) const;

    // 更新标签页标题（脏标记变化时调用）
    void updateTabTitle(Buffer* buf);
    void setIndividualTabColour(Buffer* buf, int colour);
    void setEditorBorderWidth(int width);
    void sortBuffersByName(bool ascending);

    int editorBorderWidth() const { return _editorBorderWidth; }

signals:
    // 用户点击关闭按钮时发出
    void bufferCloseRequested(Buffer* buf);
    // 双击 Tab 栏空白区域时发出（请求新建文档）
    void newTabRequested();
    void currentChanged(int index);

private slots:
    void onTabCloseRequested(int index);
    void onCurrentChanged(int index);

private:
    struct ViewState {
        qintptr currentPosition = 0;
        qintptr anchor = 0;
        int firstVisibleLine = 0;
        int xOffset = 0;
    };

    void saveCurrentViewState();
    void attachBuffer(int index);

    QTabBar* _tabBar = nullptr;
    ScintillaEditView* _editor = nullptr;
    Buffer* _attachedBuffer = nullptr;
    QHash<Buffer*, ViewState> _viewStates;
    int _editorBorderWidth = -1;
};

#endif // DOCTABVIEW_H
