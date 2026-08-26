// DocTabView.cpp - 文档标签页视图实现
// 移植自: v8.4.6:PowerEditor/src/ScintillaComponent/DocTabView.cpp

#include "DocTabView.h"
#include "ScintillaComponent/ScintillaEditView.h"
#include "Parameters.h"
#include <QTabBar>
#include <QVariant>
#include <QIcon>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QStyleOptionTab>
#include <QCollator>
#include <QVBoxLayout>
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
        setExpanding(false);
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

static QIcon bufferIcon(const Buffer* buffer)
{
    const NppGUI& gui = NppParameters::getInstance().getNppGUI();
    const bool alternate = gui._tabIconSetNumber == 1;
    QString base = QStringLiteral(":/icons/");
    if (!alternate && gui._darkModeEnabled)
        base += QStringLiteral("darkMode/tabbar/");

    QString name;
    if (buffer && buffer->isMonitoring())
        name = QStringLiteral("monitoring.ico");
    else if (buffer && buffer->isReadOnly())
        name = alternate ? QStringLiteral("readonly_alt.ico")
                         : QStringLiteral("readonly.ico");
    else if (buffer && buffer->isDirty())
        name = alternate ? QStringLiteral("unsaved_alt.ico")
                         : QStringLiteral("unsaved.ico");
    else
        name = alternate ? QStringLiteral("saved_alt.ico")
                         : QStringLiteral("saved.ico");
    // Upstream's alternate set reuses the normal monitoring image.
    if (alternate && name == QStringLiteral("monitoring.ico"))
        base = QStringLiteral(":/icons/");
    return QIcon(base + name);
}

// ─── DocTabView ──────────────────────────────────────────────────────────────

DocTabView::DocTabView(QWidget* parent)
    : QWidget(parent)
{
    _tabBar = new NppTabBar(this);
    _tabBar->setTabsClosable(true);
    _tabBar->setMovable(true);
    _tabBar->setDocumentMode(true);
    _editor = new ScintillaEditView(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(_tabBar);
    layout->addWidget(_editor, 1);

    connect(_tabBar, &QTabBar::tabCloseRequested,
            this, &DocTabView::onTabCloseRequested);
    connect(_tabBar, &QTabBar::currentChanged,
            this, &DocTabView::onCurrentChanged);
    connect(static_cast<NppTabBar*>(_tabBar),
            &NppTabBar::emptyAreaDoubleClicked,
            this, &DocTabView::newTabRequested);
}

void DocTabView::addBuffer(Buffer* buf)
{
    addBufferView(buf, _editor);
}

void DocTabView::addBufferView(Buffer* buf, ScintillaEditView* view)
{
    if (!buf || !view || !buf->document())
        return;
    if (indexOfBuffer(buf) != -1)
        return;

    buf->addView(_editor);
    QIcon icon = bufferIcon(buf);
    int idx = _tabBar->addTab(icon, buf->getTabLabel());
    // 用 quintptr 存储指针以避免 QVariant 对 void* 的限制
    _tabBar->setTabData(idx, QVariant(static_cast<quintptr>(
        reinterpret_cast<quintptr>(buf))));
    _tabBar->setCurrentIndex(idx);
    attachBuffer(idx);
}

void DocTabView::addClone(Buffer* buf, ScintillaEditView* cloneView)
{
    Q_UNUSED(cloneView)
    if (!buf || !buf->document()) return;
    if (indexOfBuffer(buf) != -1) return;
    buf->addView(_editor);
    QIcon icon = bufferIcon(buf);
    int idx = _tabBar->addTab(icon, buf->getTabLabel());
    _tabBar->setTabData(idx, QVariant(static_cast<quintptr>(
        reinterpret_cast<quintptr>(buf))));
    _tabBar->setCurrentIndex(idx);
    attachBuffer(idx);
}

void DocTabView::setIndividualTabColour(Buffer* buf, int colour)
{
    if (!buf)
        return;
    buf->setIndividualTabColour(colour);
    _tabBar->update();
}

void DocTabView::setEditorBorderWidth(int width)
{
    width = qBound(0, width, 30);
    if (_editorBorderWidth == width)
        return;
    _editorBorderWidth = width;
    layout()->setContentsMargins(width, 0, width, width);
    setStyleSheet(QStringLiteral(
        "DocTabView { padding: %1px; }").arg(width));
}

void DocTabView::removeBuffer(Buffer* buf)
{
    int idx = indexOfBuffer(buf);
    if (idx != -1)
        removeTab(idx);
}

void DocTabView::removeTab(int index)
{
    Buffer* buffer = bufferAt(index);
    if (!buffer)
        return;
    if (buffer == _attachedBuffer)
        saveCurrentViewState();
    buffer->removeView(_editor);
    _viewStates.remove(buffer);
    _tabBar->removeTab(index);
    if (_tabBar->count() == 0) {
        _attachedBuffer = nullptr;
        _editor->createStandardDocument();
    }
}

void DocTabView::activateBuffer(Buffer* buf)
{
    int idx = indexOfBuffer(buf);
    if (idx != -1)
        _tabBar->setCurrentIndex(idx);
}

Buffer* DocTabView::currentBuffer() const
{
    return bufferAt(_tabBar->currentIndex());
}

Buffer* DocTabView::bufferAt(int index) const
{
    if (index < 0 || index >= _tabBar->count())
        return nullptr;
    quintptr val = _tabBar->tabData(index).value<quintptr>();
    return reinterpret_cast<Buffer*>(val);
}

int DocTabView::indexOfBuffer(Buffer* buf) const
{
    for (int i = 0; i < _tabBar->count(); ++i) {
        if (bufferAt(i) == buf)
            return i;
    }
    return -1;
}

void DocTabView::updateTabTitle(Buffer* buf)
{
    int idx = indexOfBuffer(buf);
    if (idx != -1) {
        _tabBar->setTabText(idx, buf->getTabLabel());
        _tabBar->setTabIcon(idx, bufferIcon(buf));
    }
}

void DocTabView::refreshTabIcons()
{
    for (int i = 0; i < _tabBar->count(); ++i)
        _tabBar->setTabIcon(i, bufferIcon(bufferAt(i)));
}

void DocTabView::sortBuffersByName(bool ascending)
{
    QList<Buffer*> ordered;
    for (int i = 0; i < _tabBar->count(); ++i)
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
            _tabBar->moveTab(current, target);
    }
}

void DocTabView::onTabCloseRequested(int index)
{
    Buffer* buf = bufferAt(index);
    if (buf)
        emit bufferCloseRequested(buf);
}

int DocTabView::count() const
{
    return _tabBar->count();
}

int DocTabView::currentIndex() const
{
    return _tabBar->currentIndex();
}

void DocTabView::setCurrentIndex(int index)
{
    _tabBar->setCurrentIndex(index);
}

QString DocTabView::tabText(int index) const
{
    return _tabBar->tabText(index);
}

QWidget* DocTabView::currentWidget() const
{
    return currentIndex() >= 0 ? _editor : nullptr;
}

QWidget* DocTabView::widget(int index) const
{
    return index >= 0 && index < count() ? _editor : nullptr;
}

void DocTabView::saveCurrentViewState()
{
    if (!_attachedBuffer)
        return;
    ViewState& state = _viewStates[_attachedBuffer];
    state.currentPosition = _editor->SendScintillaNpp(SCI_GETCURRENTPOS);
    state.anchor = _editor->SendScintillaNpp(SCI_GETANCHOR);
    state.firstVisibleLine = static_cast<int>(
        _editor->SendScintilla(SCI_GETFIRSTVISIBLELINE));
    state.xOffset = static_cast<int>(_editor->SendScintilla(SCI_GETXOFFSET));
}

void DocTabView::attachBuffer(int index)
{
    Buffer* buffer = bufferAt(index);
    if (!buffer || !buffer->document())
        return;
    if (_attachedBuffer != buffer) {
        saveCurrentViewState();
        _editor->setDocument(buffer->document());
        _attachedBuffer = buffer;
        _editor->setLargeFileMode(buffer->isLargeFile());
        _editor->setReadOnly(buffer->isReadOnly());
        const ViewState state = _viewStates.value(buffer);
        _editor->SendScintillaNpp(SCI_SETSEL, state.anchor,
                                  state.currentPosition);
        _editor->SendScintilla(SCI_SETFIRSTVISIBLELINE,
                               state.firstVisibleLine);
        _editor->SendScintilla(SCI_SETXOFFSET, state.xOffset);
    }
}

void DocTabView::onCurrentChanged(int index)
{
    attachBuffer(index);
    emit currentChanged(index);
}

// NppTabBar 的 Q_OBJECT 在 .cpp 文件内部定义，需要包含 moc 生成文件
#include "DocTabView.moc"
