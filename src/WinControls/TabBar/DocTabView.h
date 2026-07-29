// DocTabView.h - 文档标签页视图
// 移植自: v8.4.6:PowerEditor/src/WinControls/TabBar/DocTabView.h

#ifndef DOCTABVIEW_H
#define DOCTABVIEW_H

#include <QTabWidget>
#include "ScintillaComponent/Buffer.h"

class DocTabView : public QTabWidget
{
    Q_OBJECT

public:
    explicit DocTabView(QWidget* parent = nullptr);
    ~DocTabView() = default;

    // 添加标签页（buffer 必须已关联 view）
    void addBuffer(Buffer* buf);
    void addBufferView(Buffer* buf, ScintillaEditView* view);

    // 添加克隆标签页（cloneView 与原 view 共享同一 QsciDocument）
    void addClone(Buffer* buf, ScintillaEditView* cloneView);

    // 关闭并移除标签页（不销毁 buffer，由调用方决定）
    void removeBuffer(Buffer* buf);

    // 切换到指定缓冲区
    void activateBuffer(Buffer* buf);

    // 获取当前活跃的缓冲区
    Buffer* currentBuffer() const;

    // 按索引获取缓冲区
    Buffer* bufferAt(int index) const;

    // 查找缓冲区对应的标签索引，找不到返回 -1
    int indexOfBuffer(Buffer* buf) const;

    // 更新标签页标题（脏标记变化时调用）
    void updateTabTitle(Buffer* buf);
    void setIndividualTabColour(Buffer* buf, int colour);
    void sortBuffersByName(bool ascending);

signals:
    // 用户点击关闭按钮时发出
    void bufferCloseRequested(Buffer* buf);
    // 双击 Tab 栏空白区域时发出（请求新建文档）
    void newTabRequested();

private slots:
    void onTabCloseRequested(int index);
    void onCurrentChanged(int index);
};

#endif // DOCTABVIEW_H
