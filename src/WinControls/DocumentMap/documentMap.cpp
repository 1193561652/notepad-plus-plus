// DocumentMapPanel.cpp - 文档地图面板实现

#include "documentMap.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QScrollBar>

// ─── MapIndicator ─────────────────────────────────────────────────────────────

MapIndicator::MapIndicator(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
}

void MapIndicator::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(80, 140, 255, 55));
    p.setPen(QColor(60, 100, 220, 160));
    p.drawRect(rect().adjusted(0, 0, -1, -1));
}

// ─── DocumentMapPanel ─────────────────────────────────────────────────────────

DocumentMapPanel::DocumentMapPanel(QWidget* parent)
    : QWidget(parent)
{
    _miniView = new ScintillaEditView(this);
    _miniView->setReadOnly(true);
    _miniView->SendScintilla(SCI_SETZOOM, -8);
    _miniView->setMarginWidth(0, 0);
    _miniView->setMarginWidth(1, 0);
    _miniView->setMarginWidth(2, 0);
    _miniView->SendScintilla(SCI_SETCARETWIDTH, 0);
    _miniView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _miniView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _miniView->setFocusPolicy(Qt::NoFocus);

    _indicator = new MapIndicator(_miniView);
    _indicator->hide();

    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(_miniView);

    _miniView->installEventFilter(this);
}

void DocumentMapPanel::syncWith(ScintillaEditView* mainView)
{
    // 断开旧视图的连接
    if (_trackedView) {
        _trackedView->verticalScrollBar()->disconnect(this);
        _trackedView->removeEventFilter(this);
    }

    _trackedView = mainView;

    if (!mainView) {
        _miniView->setText("");
        _indicator->hide();
        return;
    }

    // 共享文档（与克隆视图同样的机制）
    _miniView->setDocument(mainView->document());
    _miniView->setLargeFileMode(mainView->isLargeFileMode());
    _miniView->setReadOnly(true);

    // 跟踪主视图的滚动和大小变化
    connect(mainView->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &DocumentMapPanel::updatePositionIndicator);
    mainView->installEventFilter(this);

    updatePositionIndicator();
}

BufferMapState DocumentMapPanel::sessionState() const
{
    BufferMapState state;
    if (!_trackedView || !_miniView)
        return state;

    state.firstVisibleDisplayLine = static_cast<int>(_miniView->SendScintilla(
        SCI_GETFIRSTVISIBLELINE));
    state.firstVisibleDocLine = static_cast<int>(_trackedView->SendScintilla(
        SCI_GETFIRSTVISIBLELINE));
    const int visibleLines = static_cast<int>(_trackedView->SendScintilla(
        SCI_LINESONSCREEN));
    state.lastVisibleDocLine =
        qMin(_trackedView->lines() - 1,
             state.firstVisibleDocLine + qMax(0, visibleLines - 1));
    state.lineCount = _trackedView->lines();
    state.higherPosition = static_cast<int>(_trackedView->SendScintilla(
        SCI_GETCURRENTPOS));
    state.width = width();
    state.height = height();
    state.kBytesInDocument =
        (_trackedView->documentLengthNpp() + 1023) / 1024;
    state.wrapIndentMode = static_cast<int>(_trackedView->SendScintilla(
        SCI_GETWRAPINDENTMODE));
    state.isWrap = _trackedView->wrapMode() != WrapNone;
    return state;
}

void DocumentMapPanel::restoreSessionState(const BufferMapState& state)
{
    if (!_trackedView || !_miniView)
        return;
    if (state.firstVisibleDisplayLine >= 0) {
        _miniView->SendScintilla(
            SCI_SETFIRSTVISIBLELINE,
            static_cast<unsigned long>(state.firstVisibleDisplayLine));
    }
    const int miniFirstLine = static_cast<int>(_miniView->SendScintilla(
        SCI_GETFIRSTVISIBLELINE));
    const int firstLine = static_cast<int>(_trackedView->SendScintilla(
        SCI_GETFIRSTVISIBLELINE));
    const int visibleLines = static_cast<int>(_trackedView->SendScintilla(
        SCI_LINESONSCREEN));
    const int lineHeight = static_cast<int>(_miniView->SendScintilla(
        SCI_TEXTHEIGHT, 0));
    if (lineHeight <= 0)
        return;
    const int indicatorY = qMax(0, firstLine - miniFirstLine) * lineHeight;
    const int indicatorHeight = qMin(
        qMax(4, visibleLines * lineHeight),
        qMax(0, _miniView->height() - indicatorY));
    _indicator->setGeometry(
        0, indicatorY, _miniView->width(), indicatorHeight);
    _indicator->raise();
    _indicator->show();
}

void DocumentMapPanel::updatePositionIndicator()
{
    if (!_trackedView || !_miniView || !isVisible()) return;

    int firstLine   = (int)_trackedView->SendScintilla(SCI_GETFIRSTVISIBLELINE);
    int visibleLines= (int)_trackedView->SendScintilla(SCI_LINESONSCREEN);
    int totalLines  = _trackedView->lines();
    if (totalLines <= 0) return;

    // 滚动缩略图使当前位置居中
    int miniFirstLine = qMax(0, firstLine - visibleLines);
    _miniView->SendScintilla(SCI_SETFIRSTVISIBLELINE, miniFirstLine);

    // 计算指示器位置
    int lineH = (int)_miniView->SendScintilla(SCI_TEXTHEIGHT, 0);
    if (lineH <= 0) return;

    int relFirst = firstLine - miniFirstLine;
    int indY = relFirst * lineH;
    int indH = qMax(4, visibleLines * lineH);
    indH = qMin(indH, _miniView->height() - indY);

    _indicator->setGeometry(0, indY, _miniView->width(), indH);
    _indicator->raise();
    _indicator->show();
}

bool DocumentMapPanel::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == _miniView && event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        int pos  = (int)_miniView->SendScintilla(SCI_POSITIONFROMPOINT,
                                                 (ulong)me->x(), (ulong)me->y());
        int line = (int)_miniView->SendScintilla(SCI_LINEFROMPOSITION, pos);
        if (_trackedView) {
            int half = (int)_trackedView->SendScintilla(SCI_LINESONSCREEN) / 2;
            int target = qMax(0, line - half);
            _trackedView->SendScintilla(SCI_SETFIRSTVISIBLELINE, target);
        }
        return true;
    }
    if (obj == _trackedView && event->type() == QEvent::Resize) {
        updatePositionIndicator();
    }
    return false;
}
