// DocTabView.cpp - 文档标签页视图实现
// 移植自: v8.4.6:PowerEditor/src/WinControls/TabBar/DocTabView.cpp

#include "DocTabView.h"
#include "ScintillaComponent/ScintillaEditView.h"
#include <QTabBar>
#include <QVariant>
#include <QIcon>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QStyleOptionTab>
#include <QCollator>
#include <algorithm>

static QColor individualTabColour(int id)
{
    static const QColor colours[] = {
        QColor(255, 214, 102),
        QColor(126, 211, 127),
        QColor(105, 170, 255),
        QColor(255, 151, 105),
        QColor(190, 132, 255)
    };
    return id >= 0 && id < 5 ? colours[id] : QColor();
}

// ─── 自定义 TabBar：在当前标签顶部绘制黄色横线 ──────────────────────────────

class NppTabBar : public QTabBar
{
    Q_OBJECT
public:
    explicit NppTabBar(QWidget* parent = nullptr) : QTabBar(parent)
    {
        // 为标签顶部预留 4px 空间，使图标/文字不被黄色横线遮挡
        setStyleSheet(
            "QTabBar::tab {"
            "  padding-top: 5px;"
            "  padding-bottom: 3px;"
            "  padding-left: 6px;"
            "  padding-right: 6px;"
            "  margin-right: 1px;"
            /* 轻微突起边框：上/左用亮色，右/下用暗色，模拟 Windows 经典 3D raised */
            "  border-top: 1px solid #E0E0E0;"
            "  border-left: 1px solid #E0E0E0;"
            "  border-right: 1px solid #808080;"
            "  border-bottom: none;"
            "}"
            "QTabBar::tab:!selected {"
            "  color: gray;"
            "  background-color: #E8E8E8;"
            "  margin-top: 2px;"
            "}"
            "QTabBar::tab:selected {"
            "  color: black;"
            "  background-color: white;"
            "  margin-top: 0;"
            "}"
        );
    }

signals:
    void emptyAreaDoubleClicked();

protected:
    void mouseDoubleClickEvent(QMouseEvent* e) override
    {
        // 双击落在已有 tab 上时走默认行为（如 dbclkToClose）
        if (tabAt(e->pos()) == -1)
            emit emptyAreaDoubleClicked();
        else
            QTabBar::mouseDoubleClickEvent(e);
    }

    void paintEvent(QPaintEvent* e) override
    {
        QTabBar::paintEvent(e);   // 先绘制默认标签

        int cur = currentIndex();
        if (cur < 0) return;

        QPainter p(this);
        for (int i = 0; i < count(); ++i) {
            const quintptr value = tabData(i).value<quintptr>();
            const Buffer* buffer = reinterpret_cast<const Buffer*>(value);
            const QColor colour = buffer
                ? individualTabColour(buffer->individualTabColour())
                : QColor();
            if (colour.isValid()) {
                const QRect tab = tabRect(i);
                p.fillRect(tab.left() + 2, tab.bottom() - 2,
                           qMax(0, tab.width() - 4), 3, colour);
            }
        }

        const QRect currentRect = tabRect(cur);
        const quintptr value = tabData(cur).value<quintptr>();
        const Buffer* buffer = reinterpret_cast<const Buffer*>(value);
        QColor topColour = buffer
            ? individualTabColour(buffer->individualTabColour())
            : QColor();
        if (!topColour.isValid())
            topColour = QColor(0xFA, 0xAA, 0x3C);
        p.fillRect(currentRect.left() + 1, currentRect.top(),
                   currentRect.width() - 2, 4, topColour);
    }
};

// ─── 图标辅助 ────────────────────────────────────────────────────────────────

static const QIcon& savedIcon()
{
    static QIcon icon(":/icons/saved.ico");
    return icon;
}

static const QIcon& unsavedIcon()
{
    static QIcon icon(":/icons/unsaved.ico");
    return icon;
}

// ─── DocTabView ──────────────────────────────────────────────────────────────

DocTabView::DocTabView(QWidget* parent)
    : QTabWidget(parent)
{
    NppTabBar* bar = new NppTabBar(this);
    setTabBar(bar);
    setTabsClosable(true);
    setMovable(true);
    setDocumentMode(true);

    connect(this, &QTabWidget::tabCloseRequested,
            this, &DocTabView::onTabCloseRequested);
    connect(this, &QTabWidget::currentChanged,
            this, &DocTabView::onCurrentChanged);
    connect(bar, &NppTabBar::emptyAreaDoubleClicked,
            this, &DocTabView::newTabRequested);
}

void DocTabView::addBuffer(Buffer* buf)
{
    addBufferView(buf, buf ? buf->getView() : nullptr);
}

void DocTabView::addBufferView(Buffer* buf, ScintillaEditView* view)
{
    if (!buf || !view)
        return;
    if (indexOfBuffer(buf) != -1)
        return;

    buf->addView(view);
    QIcon icon = buf->isDirty() ? unsavedIcon() : savedIcon();
    int idx = addTab(view, icon, buf->getTabLabel());
    // 用 quintptr 存储指针以避免 QVariant 对 void* 的限制
    tabBar()->setTabData(idx, QVariant(static_cast<quintptr>(
        reinterpret_cast<quintptr>(buf))));
    setCurrentIndex(idx);
}

void DocTabView::addClone(Buffer* buf, ScintillaEditView* cloneView)
{
    if (!buf || !cloneView) return;
    if (indexOfBuffer(buf) != -1) return;
    buf->addView(cloneView);
    QIcon icon = buf->isDirty() ? unsavedIcon() : savedIcon();
    int idx = addTab(cloneView, icon, buf->getTabLabel());
    tabBar()->setTabData(idx, QVariant(static_cast<quintptr>(
        reinterpret_cast<quintptr>(buf))));
    setCurrentIndex(idx);
}

void DocTabView::setIndividualTabColour(Buffer* buf, int colour)
{
    if (!buf)
        return;
    buf->setIndividualTabColour(colour);
    tabBar()->update();
}

void DocTabView::removeBuffer(Buffer* buf)
{
    int idx = indexOfBuffer(buf);
    if (idx != -1)
        removeTab(idx);
}

void DocTabView::activateBuffer(Buffer* buf)
{
    int idx = indexOfBuffer(buf);
    if (idx != -1)
        setCurrentIndex(idx);
}

Buffer* DocTabView::currentBuffer() const
{
    return bufferAt(currentIndex());
}

Buffer* DocTabView::bufferAt(int index) const
{
    if (index < 0 || index >= count())
        return nullptr;
    quintptr val = tabBar()->tabData(index).value<quintptr>();
    return reinterpret_cast<Buffer*>(val);
}

int DocTabView::indexOfBuffer(Buffer* buf) const
{
    for (int i = 0; i < count(); ++i) {
        if (bufferAt(i) == buf)
            return i;
    }
    return -1;
}

void DocTabView::updateTabTitle(Buffer* buf)
{
    int idx = indexOfBuffer(buf);
    if (idx != -1) {
        setTabText(idx, buf->getTabLabel());
        setTabIcon(idx, buf->isDirty() ? unsavedIcon() : savedIcon());
    }
}

void DocTabView::sortBuffersByName(bool ascending)
{
    QList<Buffer*> ordered;
    for (int i = 0; i < count(); ++i)
        ordered.append(bufferAt(i));

    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    collator.setNumericMode(true);
    std::stable_sort(ordered.begin(), ordered.end(),
        [&](Buffer* left, Buffer* right) {
            const int result = collator.compare(
                left ? left->getFileName() : QString(),
                right ? right->getFileName() : QString());
            return ascending ? result < 0 : result > 0;
        });

    for (int target = 0; target < ordered.size(); ++target) {
        const int current = indexOfBuffer(ordered.at(target));
        if (current != target)
            tabBar()->moveTab(current, target);
    }
}

void DocTabView::onTabCloseRequested(int index)
{
    Buffer* buf = bufferAt(index);
    if (buf)
        emit bufferCloseRequested(buf);
}

void DocTabView::onCurrentChanged(int /*index*/)
{
    // 预留：切换标签时通知主窗口更新状态
}

// NppTabBar 的 Q_OBJECT 在 .cpp 文件内部定义，需要包含 moc 生成文件
#include "DocTabView.moc"
