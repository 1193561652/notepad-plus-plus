// FindReplaceDlg.cpp - 查找替换对话框实现
// 移植自: v8.4.6:PowerEditor/src/ScintillaComponent/FindReplaceDlg.cpp
//
// 布局对照 find.png 截图和原版 FindReplaceDlg.rc（382×200 dialog units）：
//
//  [查找|替换|文件查找|标记]
//  查找目标(F): [___combo___]      □选取范围内①  [查找下一个  ]
//                                                [计数(D)     ]
//  □ 反向查找                                    [查找所有打开文件(O)]
//  □ 全词匹配(W)                                 [在当前文件中查找  ]
//  ☑ 匹配大小写(C)                               [取消             ]
//  ☑ 循环查找(P)
//  ┌查找模式───────────────────┐  ┌☑透明度(Y)──────────────┐
//  │( )普通  ( )扩展  ( )正则  │  │ (*)失去焦点后  ( )始终  │
//  └───────────────────────────┘  │ [====slider====]        │
//                                 └─────────────────────────┘

#include "FindReplaceDlg.h"
#include "ScintillaEditView.h"
#include "ScintillaTextSearch.h"
#include "../Parameters.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFrame>
#include <QCloseEvent>
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QLineEdit>
#include <QFileDialog>
#include <QEvent>
#include <QFontMetrics>

static const int BOOKMARK_MARKER = 1;
static const int BTN_MIN_WIDTH   = 135;
static const int LBL_WIDTH       = 108;

static int findInputLabelWidth(const QWidget* widget)
{
    const QFontMetrics metrics(widget->font());
    const QStringList labels = {
        FindReplaceDlg::tr("Find what:"),
        FindReplaceDlg::tr("Replace with:"),
        FindReplaceDlg::tr("Filters:"),
        FindReplaceDlg::tr("Directory:")
    };
    int width = LBL_WIDTH;
    for (const QString& label : labels)
        width = qMax(width, metrics.horizontalAdvance(label) + 12);
    return width;
}

static bool findFromPosition(ScintillaEditView* view, const QString& text,
                             const FindOption& option, qintptr position)
{
    view->setCurrentPositionNpp(position);
    return view->findFirst(
        text, option._isRegex, option._isMatchCase, option._isWholeWord,
        false, true);
}


// ─────────────────────────────────────────────────────────────────────────────

FindReplaceDlg::FindReplaceDlg(QWidget* parent)
    : QDialog(parent, Qt::Tool | Qt::CustomizeWindowHint
                          | Qt::WindowTitleHint
                          | Qt::WindowSystemMenuHint
                          | Qt::WindowCloseButtonHint
                          | Qt::WindowStaysOnTopHint)
{
    setWindowTitle(tr("Find"));
    setSizeGripEnabled(true);
    setupUi();
    loadFindHistory();
}

// ─── UI 构建 ──────────────────────────────────────────────────────────────────

void FindReplaceDlg::setupUi()
{
    auto* main = new QVBoxLayout(this);
    main->setSpacing(4);
    main->setContentsMargins(6, 6, 6, 6);

    // ── 1. Tab 条 ────────────────────────────────────────────────────────────
    _tabBar = new QTabBar(this);
    _tabBar->setObjectName("tabBar");
    _tabBar->addTab(tr("Find"));               // 0
    _tabBar->addTab(tr("Replace"));            // 1
    _tabBar->addTab(tr("Find in Files"));      // 2
    _tabBar->addTab(tr("Find in Projects"));
    _tabBar->addTab(tr("Mark"));               // 4
    _tabBar->setExpanding(false);
    _tabBar->setDrawBase(true);
    main->addWidget(_tabBar);

    // ── 2. 内容区：左（输入+选项）| 右（按钮列，含取消） ───────────────────
    auto* contentH = new QHBoxLayout();
    contentH->setSpacing(8);

    // 左侧
    auto* leftV = new QVBoxLayout();
    leftV->setSpacing(6);
    leftV->addWidget(makeInputArea());

    _optionsStack = new QStackedWidget();
    _optionsStack->addWidget(makeFindOptions());    // 0
    _optionsStack->addWidget(makeReplaceOptions()); // 1
    _optionsStack->addWidget(makeFifOptions());     // 2
    _optionsStack->addWidget(makeFipOptions());     // 3
    _optionsStack->addWidget(makeMarkOptions());    // 4
    leftV->addWidget(_optionsStack);
    leftV->addStretch();
    contentH->addLayout(leftV, 1);

    // 右侧按钮列（每页含取消按钮，对应原版 IDCANCEL y=98）
    _btnStack = new QStackedWidget();
    _btnStack->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    _btnStack->addWidget(makeFindButtons());    // 0
    _btnStack->addWidget(makeReplaceButtons()); // 1
    _btnStack->addWidget(makeFifButtons());     // 2
    _btnStack->addWidget(makeFipButtons());     // 3
    _btnStack->addWidget(makeMarkButtons());    // 4
    contentH->addWidget(_btnStack);
    main->addLayout(contentH);

    // ── 3. 底部行：搜索模式（左）| 透明度（右）—— 对应原版 y=131 水平布局 ──
    makeBottomRow(main);

    // ── 4. 状态栏 ─────────────────────────────────────────────────────────
    _statusLabel = new QLabel(this);
    main->addWidget(_statusLabel);

    connect(_tabBar,    &QTabBar::currentChanged,
            this,       &FindReplaceDlg::onTabChanged);
    connect(_findCombo, &QComboBox::currentTextChanged,
            this, [this](const QString&){ _statusLabel->clear(); });

    onTabChanged(0);

    // The original dialog is 382 x 200 dialog units and grows with the
    // system dialog font.  A fixed pixel height clipped the option rows on
    // Linux and with translated/high-DPI fonts.  Let Qt calculate the height
    // while keeping approximately the original minimum width and aspect.
    main->activate();
    const QSize naturalSize = main->sizeHint();
    setMinimumSize(qMax(573, naturalSize.width()),
                   qMax(360, naturalSize.height()));
    // v8.4.6 uses WS_THICKFRAME but constrains min/max track height to the
    // initial height.  Users can expand the dialog horizontally only.
    setMaximumHeight(minimumHeight());
    resize(minimumSize());
}

// ─── 底部行：搜索模式 + 透明度 ────────────────────────────────────────────────
// 对应原版 RC: IDC_MODE_STATIC(x=6,y=131,w=159) + IDC_TRANSPARENT_GRPBOX(x=258,y=131,w=99)

void FindReplaceDlg::makeBottomRow(QLayout* parentLayout)
{
    auto* row = new QHBoxLayout();
    row->setSpacing(8);

    // 搜索模式 GroupBox（左）
    _modeBox = new QGroupBox(tr("Search Mode"));
    _modeBox->setObjectName("grpSearchMode");
    auto* modeGrid = new QGridLayout(_modeBox);
    modeGrid->setContentsMargins(6, 4, 6, 4);
    modeGrid->setSpacing(4);
    _modeNormal   = new QRadioButton(tr("Normal"));
    _modeNormal->setObjectName("rbModeNormal");
    _modeExtended = new QRadioButton(tr("Extended") + " (\\n, \\r, \\t, \\0, \\x...)");
    _modeExtended->setObjectName("rbModeExtended");
    _modeRegex    = new QRadioButton(tr("Regular expression"));
    _modeRegex->setObjectName("rbModeRegex");
    _dotMatchNewline = new QCheckBox(tr(". matches newline"));
    _dotMatchNewline->setObjectName("chkDotMatchNewline");
    _dotMatchNewline->setVisible(false);
    _modeNormal->setChecked(true);
    _modeGroup = new QButtonGroup(this);
    _modeGroup->addButton(_modeNormal,   0);
    _modeGroup->addButton(_modeExtended, 1);
    _modeGroup->addButton(_modeRegex,    2);
    modeGrid->addWidget(_modeNormal,      0, 0);
    modeGrid->addWidget(_modeExtended,    1, 0);
    modeGrid->addWidget(_modeRegex,       2, 0);
    modeGrid->addWidget(_dotMatchNewline, 2, 1);
    connect(_modeRegex, &QRadioButton::toggled,
            _dotMatchNewline, &QCheckBox::setVisible);
    row->addWidget(_modeBox, 1);

    // 透明度 GroupBox（右）—— 可勾选标题，对应原版 IDC_TRANSPARENT_CHECK 在 GroupBox 标题位置
    _transGB = new QGroupBox(tr("Transparency"));
    _transGB->setObjectName("grpTransparency");
    _transGB->setCheckable(true);
    _transGB->setChecked(false);
    auto* transLay = new QVBoxLayout(_transGB);
    transLay->setContentsMargins(6, 4, 6, 4);
    transLay->setSpacing(2);

    _transOnLostFocusRB = new QRadioButton(tr("On losing focus"));
    _transOnLostFocusRB->setObjectName("rbTransOnLostFocus");
    _transAlwaysRB      = new QRadioButton(tr("Always"));
    _transAlwaysRB->setObjectName("rbTransAlways");
    _transOnLostFocusRB->setChecked(true);
    _transModeGroup = new QButtonGroup(this);
    _transModeGroup->addButton(_transOnLostFocusRB, 0);
    _transModeGroup->addButton(_transAlwaysRB,      1);

    _transSlider = new QSlider(Qt::Horizontal);
    _transSlider->setRange(20, 100);   // 20%~100% 透明度
    _transSlider->setValue(80);
    _transSlider->setTickPosition(QSlider::TicksBelow);
    _transSlider->setTickInterval(20);

    transLay->addWidget(_transOnLostFocusRB);
    transLay->addWidget(_transAlwaysRB);
    transLay->addWidget(_transSlider);
    // 宽度略大于按钮列（BTN_MIN_WIDTH），使左边稍微超出关闭按钮左边缘
    _transGB->setMinimumWidth(BTN_MIN_WIDTH + 20);
    row->addWidget(_transGB);

    // 信号连接
    connect(_transGB,            &QGroupBox::toggled,
            this,                &FindReplaceDlg::onTransparencyChanged);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(_transModeGroup,     &QButtonGroup::idClicked,
            this, [this](int){ onTransparencyChanged(); });
#else
    connect(_transModeGroup,     QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, [this](int){ onTransparencyChanged(); });
#endif
    connect(_transSlider,        &QSlider::valueChanged,
            this, [this](int){ onTransparencyChanged(); });

    static_cast<QVBoxLayout*>(parentLayout)->addLayout(row);
}

// ─── 输入行区域 ───────────────────────────────────────────────────────────────

QWidget* FindReplaceDlg::makeInputArea()
{
    auto* w = new QWidget(this);
    auto* vLay = new QVBoxLayout(w);
    vLay->setContentsMargins(0, 0, 0, 0);
    vLay->setSpacing(4);

    const int labelWidth = findInputLabelWidth(this);
    auto makeRow = [labelWidth](QLabel* lbl, QWidget* input) -> QWidget* {
        auto* row = new QWidget();
        auto* h = new QHBoxLayout(row);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(4);
        lbl->setFixedWidth(labelWidth);
        lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        h->addWidget(lbl);
        h->addWidget(input, 1);
        return row;
    };

    // Find what（始终可见）
    _findCombo = new QComboBox();
    _findCombo->setObjectName("findCombo");
    _findCombo->setEditable(true);
    _findCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* lblFindWhat = new QLabel(tr("Find what:"));
    lblFindWhat->setObjectName("lblFindWhat");
    vLay->addWidget(makeRow(lblFindWhat, _findCombo));

    // "仅在选区中"：在查找框正下方，左边与 combo 左边对齐（Find tab 专用，其他 tab 隐藏）
    _inSelCB = new QCheckBox(tr("In selection"));
    _inSelCB->setObjectName("chkInSel");
    _inSelFindRow = new QWidget();
    {
        auto* h = new QHBoxLayout(_inSelFindRow);
        h->setContentsMargins(0, 5, 0, 0);   // 上边距 5px，向下偏移
        h->setSpacing(0);
        h->addStretch();
        h->addWidget(_inSelCB);   // 右对齐，与 combo 右边缘对齐
    }
    vLay->addWidget(_inSelFindRow);

    // Replace with（Replace tab 时可见）
    _replaceLbl   = new QLabel(tr("Replace with:"));
    _replaceLbl->setObjectName("lblReplaceWith");
    _replaceCombo = new QComboBox();
    _replaceCombo->setObjectName("replaceCombo");
    _replaceCombo->setEditable(true);
    _replaceCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _replaceRow = makeRow(_replaceLbl, _replaceCombo);
    vLay->addWidget(_replaceRow);

    // Filters（FIF / FIP 时可见）
    _filtersCombo = new QComboBox();
    _filtersCombo->setObjectName("filtersCombo");
    _filtersCombo->setEditable(true);
    _filtersCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _filtersCombo->addItem("*.*");
    auto* lblFilters = new QLabel(tr("Filters:"));
    lblFilters->setObjectName("lblFilters");
    _filtersRow = makeRow(lblFilters, _filtersCombo);
    vLay->addWidget(_filtersRow);

    // Directory（FIF 时可见）
    auto* dirInner = new QWidget();
    auto* dirH     = new QHBoxLayout(dirInner);
    dirH->setContentsMargins(0, 0, 0, 0);
    dirH->setSpacing(2);
    _dirCombo = new QComboBox();
    _dirCombo->setObjectName("dirCombo");
    _dirCombo->setEditable(true);
    _dirCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _browseDirBtn = new QPushButton("...");
    _browseDirBtn->setObjectName("browseDirBtn");
    _browseDirBtn->setFixedWidth(28);
    dirH->addWidget(_dirCombo);
    dirH->addWidget(_browseDirBtn);
    auto* lblDirectory = new QLabel(tr("Directory:"));
    lblDirectory->setObjectName("lblDirectory");
    _dirRow = makeRow(lblDirectory, dirInner);
    vLay->addWidget(_dirRow);

    connect(_browseDirBtn, &QPushButton::clicked, this, [this](){
        QString dir = QFileDialog::getExistingDirectory(
            this, tr("Select Directory"), _dirCombo->currentText());
        if (!dir.isEmpty()) _dirCombo->setCurrentText(dir);
    });

    return w;
}

// ─── 每 Tab 选项区（裸复选框，无 GroupBox） ────────────────────────────────────

QWidget* FindReplaceDlg::makeFindOptions()
{
    auto* w = new QWidget();
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(2, 2, 2, 2);
    lay->setSpacing(4);

    _backwardDir = new QCheckBox(tr("Backward direction"));
    _backwardDir->setObjectName("chkBackwardDir");
    _wholeWord   = new QCheckBox(tr("Whole word only"));
    _wholeWord->setObjectName("chkWholeWord");
    _matchCase   = new QCheckBox(tr("Match case"));
    _matchCase->setObjectName("chkMatchCase");
    _wrapAround  = new QCheckBox(tr("Wrap around"));
    _wrapAround->setObjectName("chkWrapAround");

    _matchCase->setChecked(true);
    _wrapAround->setChecked(true);

    lay->addWidget(_backwardDir);
    lay->addWidget(_wholeWord);
    lay->addWidget(_matchCase);
    lay->addWidget(_wrapAround);
    lay->addStretch();
    return w;
}

QWidget* FindReplaceDlg::makeReplaceOptions()
{
    auto* w = new QWidget();
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(2, 2, 2, 2);
    lay->setSpacing(4);

    _matchCase2  = new QCheckBox(tr("Match case"));
    _matchCase2->setObjectName("chkMatchCase2");
    _wholeWord2  = new QCheckBox(tr("Whole word only"));
    _wholeWord2->setObjectName("chkWholeWord2");
    _wrapAround2 = new QCheckBox(tr("Wrap around"));
    _wrapAround2->setObjectName("chkWrapAround2");
    _inSelCB2    = new QCheckBox(tr("In selection"));
    _inSelCB2->setObjectName("chkInSel2");

    _matchCase2->setChecked(true);
    _wrapAround2->setChecked(true);

    lay->addWidget(_matchCase2);
    lay->addWidget(_wholeWord2);
    lay->addWidget(_wrapAround2);
    lay->addWidget(_inSelCB2);
    lay->addStretch();
    return w;
}

QWidget* FindReplaceDlg::makeFifOptions()
{
    auto* w = new QWidget();
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(2, 2, 2, 2);
    lay->setSpacing(4);

    _matchCase3    = new QCheckBox(tr("Match case"));
    _matchCase3->setObjectName("chkMatchCase3");
    _wholeWord3    = new QCheckBox(tr("Whole word only"));
    _wholeWord3->setObjectName("chkWholeWord3");
    _followDocCB   = new QCheckBox(tr("Follow current doc."));
    _followDocCB->setObjectName("chkFollowDoc");
    _recursiveCB   = new QCheckBox(tr("In all sub-folders"));
    _recursiveCB->setObjectName("chkRecursive");
    _recursiveCB->setChecked(true);
    _inHiddenDirCB = new QCheckBox(tr("In hidden folders"));
    _inHiddenDirCB->setObjectName("chkInHiddenDir");

    _matchCase3->setChecked(true);

    lay->addWidget(_matchCase3);
    lay->addWidget(_wholeWord3);
    lay->addWidget(_followDocCB);
    lay->addWidget(_recursiveCB);
    lay->addWidget(_inHiddenDirCB);
    lay->addStretch();
    return w;
}

QWidget* FindReplaceDlg::makeFipOptions()
{
    auto* w = new QWidget();
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(2, 2, 2, 2);
    _projectPanelCB[0] = new QCheckBox(tr("Project Panel 1"));
    _projectPanelCB[1] = new QCheckBox(tr("Project Panel 2"));
    _projectPanelCB[2] = new QCheckBox(tr("Project Panel 3"));
    _projectPanelCB[0]->setObjectName("chkProjectPanel1");
    _projectPanelCB[1]->setObjectName("chkProjectPanel2");
    _projectPanelCB[2]->setObjectName("chkProjectPanel3");
    _projectPanelCB[0]->setChecked(true);
    for (QCheckBox* panel : _projectPanelCB)
        lay->addWidget(panel);
    lay->addStretch();
    return w;
}

QWidget* FindReplaceDlg::makeMarkOptions()
{
    auto* w = new QWidget();
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(2, 2, 2, 2);
    lay->setSpacing(4);

    _bookmarkLineCB = new QCheckBox(tr("Bookmark line"));
    _bookmarkLineCB->setObjectName("chkBookmarkLine");
    _purgeMarksCB   = new QCheckBox(tr("Purge for each search"));
    _purgeMarksCB->setObjectName("chkPurgeMarks");
    _purgeMarksCB->setChecked(true);
    _matchCase4     = new QCheckBox(tr("Match case"));
    _matchCase4->setObjectName("chkMatchCase4");
    _wholeWord4     = new QCheckBox(tr("Whole word only"));
    _wholeWord4->setObjectName("chkWholeWord4");
    _inSelCB4       = new QCheckBox(tr("In selection"));
    _inSelCB4->setObjectName("chkInSel4");

    _matchCase4->setChecked(true);

    lay->addWidget(_bookmarkLineCB);
    lay->addWidget(_purgeMarksCB);
    lay->addWidget(_matchCase4);
    lay->addWidget(_wholeWord4);
    lay->addWidget(_inSelCB4);
    lay->addStretch();
    return w;
}

// ─── 每 Tab 按钮列（含取消按钮，对应原版 IDCANCEL y=98） ──────────────────────

// 辅助：创建按钮并设置最小宽度
static QPushButton* makeBtn(const QString& text) {
    auto* b = new QPushButton(text);
    b->setMinimumWidth(BTN_MIN_WIDTH);
    return b;
}

QWidget* FindReplaceDlg::makeFindButtons()
{
    // 按照截图顺序（对应原版 RC y 坐标）：
    //   查找下一个(y=20) → 计数(y=38) → 查找所有打开文件(y=80) → 在当前文件中查找(y=56) → 取消(y=98)
    auto* w = new QWidget();
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(4);

    _findNextBtn      = makeBtn(tr("Find Next"));
    _findNextBtn->setObjectName("btnFindNext");
    _countBtn         = makeBtn(tr("Count"));
    _countBtn->setObjectName("btnCount");
    _findAllOpenedBtn = makeBtn(tr("Find All in All Opened Docs")); // 3rd（查找所有打开文件）
    _findAllOpenedBtn->setObjectName("btnFindAllOpened");
    _findAllCurBtn    = makeBtn(tr("Find All in Current Doc"));     // 4th（在当前文件中查找）
    _findAllCurBtn->setObjectName("btnFindAllCur");
    auto* closeBtn = makeBtn(tr("Close"));
    closeBtn->setObjectName("btnClose");

    lay->addWidget(_findNextBtn);
    lay->addWidget(_countBtn);
    lay->addWidget(_findAllCurBtn);
    lay->addWidget(_findAllOpenedBtn);
    lay->addStretch();
    lay->addWidget(closeBtn);

    connect(_findNextBtn,   &QPushButton::clicked, this, &FindReplaceDlg::onFindNext);
    connect(_countBtn,      &QPushButton::clicked, this, &FindReplaceDlg::onCount);
    connect(_findAllOpenedBtn, &QPushButton::clicked, this, &FindReplaceDlg::onFindAllInOpenedDocs);
    connect(_findAllCurBtn, &QPushButton::clicked, this, &FindReplaceDlg::onFindAllInCurrentDoc);
    connect(closeBtn,       &QPushButton::clicked, this, &QDialog::hide);
    return w;
}

QWidget* FindReplaceDlg::makeReplaceButtons()
{
    auto* w = new QWidget();
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(4);

    _findNextBtn2  = makeBtn(tr("Find Next"));
    _findNextBtn2->setObjectName("btnFindNext2");
    _replaceBtn    = makeBtn(tr("Replace"));
    _replaceBtn->setObjectName("btnReplace");
    _replaceAllBtn = makeBtn(tr("Replace All"));
    _replaceAllBtn->setObjectName("btnReplaceAll");
    auto* replAllOpenedBtn = makeBtn(tr("Replace All in Opened Docs"));
    replAllOpenedBtn->setObjectName("btnReplAllOpened");
    auto* closeBtn = makeBtn(tr("Close"));
    closeBtn->setObjectName("btnClose2");

    lay->addWidget(_findNextBtn2);
    lay->addWidget(_replaceBtn);
    lay->addWidget(_replaceAllBtn);
    lay->addWidget(replAllOpenedBtn);
    lay->addStretch();
    lay->addWidget(closeBtn);

    connect(_findNextBtn2,  &QPushButton::clicked, this, &FindReplaceDlg::onFindNext);
    connect(_replaceBtn,    &QPushButton::clicked, this, &FindReplaceDlg::onReplace);
    connect(_replaceAllBtn, &QPushButton::clicked, this, &FindReplaceDlg::onReplaceAll);
    connect(replAllOpenedBtn, &QPushButton::clicked,
            this, &FindReplaceDlg::onReplaceAllOpenedDocs);
    connect(closeBtn,       &QPushButton::clicked, this, &QDialog::hide);
    return w;
}

QWidget* FindReplaceDlg::makeFifButtons()
{
    auto* w = new QWidget();
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(4);

    _findAllFifBtn     = makeBtn(tr("Find All"));
    _findAllFifBtn->setObjectName("btnFindAllFif");
    _replaceInFilesBtn = makeBtn(tr("Replace in Files"));
    _replaceInFilesBtn->setObjectName("btnReplaceInFiles");
    auto* closeBtn = makeBtn(tr("Close"));
    closeBtn->setObjectName("btnClose3");

    lay->addWidget(_findAllFifBtn);
    lay->addWidget(_replaceInFilesBtn);
    lay->addStretch();
    lay->addWidget(closeBtn);
    connect(_findAllFifBtn, &QPushButton::clicked, this, &FindReplaceDlg::onFindAllInFiles);
    connect(_replaceInFilesBtn, &QPushButton::clicked,
            this, &FindReplaceDlg::onReplaceInFiles);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::hide);
    return w;
}

QWidget* FindReplaceDlg::makeFipButtons()
{
    auto* w = new QWidget();
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(4);

    auto* findAllFipBtn = makeBtn(tr("Find All"));
    findAllFipBtn->setObjectName("btnFindAllFip");
    auto* replInProjBtn = makeBtn(tr("Replace in Projects"));
    replInProjBtn->setObjectName("btnReplaceInProjects");
    auto* closeBtn = makeBtn(tr("Close"));
    closeBtn->setObjectName("btnClose4");

    lay->addWidget(findAllFipBtn);
    lay->addWidget(replInProjBtn);
    lay->addStretch();
    lay->addWidget(closeBtn);
    connect(findAllFipBtn, &QPushButton::clicked,
            this, &FindReplaceDlg::onFindAllInProjects);
    connect(replInProjBtn, &QPushButton::clicked,
            this, &FindReplaceDlg::onReplaceInProjects);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::hide);
    return w;
}

QWidget* FindReplaceDlg::makeMarkButtons()
{
    auto* w = new QWidget();
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(4);

    _markAllBtn    = makeBtn(tr("Mark All"));
    _markAllBtn->setObjectName("btnMarkAll");
    _clearMarksBtn = makeBtn(tr("Clear All Marks"));
    _clearMarksBtn->setObjectName("btnClearMarks");
    _copyMarkedBtn = makeBtn(tr("Copy Marked Text"));
    _copyMarkedBtn->setObjectName("btnCopyMarked");
    auto* closeBtn = makeBtn(tr("Close"));
    closeBtn->setObjectName("btnClose5");

    lay->addWidget(_markAllBtn);
    lay->addWidget(_clearMarksBtn);
    lay->addWidget(_copyMarkedBtn);
    lay->addStretch();
    lay->addWidget(closeBtn);

    connect(_markAllBtn,    &QPushButton::clicked, this, &FindReplaceDlg::onMarkAll);
    connect(_clearMarksBtn, &QPushButton::clicked, this, &FindReplaceDlg::onClearAllMarks);
    connect(_copyMarkedBtn, &QPushButton::clicked,
            this, &FindReplaceDlg::onCopyMarkedText);
    connect(closeBtn,       &QPushButton::clicked, this, &QDialog::hide);
    return w;
}

// ─── 公开接口 ─────────────────────────────────────────────────────────────────

void FindReplaceDlg::openFindTab()
{
    _tabBar->setCurrentIndex(0);
    show(); raise(); activateWindow();
    _findCombo->setFocus();
    _findCombo->lineEdit()->selectAll();
}

void FindReplaceDlg::openReplaceTab()
{
    _tabBar->setCurrentIndex(1);
    show(); raise(); activateWindow();
    _findCombo->setFocus();
    _findCombo->lineEdit()->selectAll();
}

void FindReplaceDlg::openMarkTab()
{
    _tabBar->setCurrentIndex(4);
    show(); raise(); activateWindow();
    _findCombo->setFocus();
}

void FindReplaceDlg::openFindInFilesTab()
{
    _tabBar->setCurrentIndex(2);
    show(); raise(); activateWindow();
    _findCombo->setFocus();
}

void FindReplaceDlg::openFindInProjectsTab(int panelMask)
{
    for (int i = 0; i < 3; ++i)
        _projectPanelCB[i]->setChecked((panelMask & (1 << i)) != 0);
    _tabBar->setCurrentIndex(3);
    show();
    raise();
    activateWindow();
    _findCombo->setFocus();
}

bool FindReplaceDlg::findNext(bool forward)
{
    bool found = doFind(forward);
    if (!found)
        setStatus(tr("Not found: \"%1\"").arg(currentFindText()));
    else
        _statusLabel->clear();
    return found;
}

void FindReplaceDlg::findAllInOpenedDocs()
{
    onFindAllInOpenedDocs();
}

void FindReplaceDlg::setSearchText(const QString& text)
{
    if (!text.isEmpty())
        _findCombo->setCurrentText(text);
}

void FindReplaceDlg::loadFindHistory()
{
    const FindHistoryState& history = NppParameters::getInstance().getFindHistory();

    _findCombo->clear();
    _findCombo->addItems(history.finds);
    _replaceCombo->clear();
    _replaceCombo->addItems(history.replaces);
    _filtersCombo->clear();
    _filtersCombo->addItems(history.filters);
    _dirCombo->clear();
    _dirCombo->addItems(history.paths);

    _wholeWord->setChecked(history.matchWord);
    _wholeWord2->setChecked(history.matchWord);
    _wholeWord4->setChecked(history.matchWord);
    _matchCase->setChecked(history.matchCase);
    _matchCase2->setChecked(history.matchCase);
    _matchCase4->setChecked(history.matchCase);
    _wrapAround->setChecked(history.wrap);
    _wrapAround2->setChecked(history.wrap);
    _backwardDir->setChecked(!history.directionDown);
    _recursiveCB->setChecked(history.fifRecursive);
    _inHiddenDirCB->setChecked(history.fifInHiddenFolder);
    _followDocCB->setChecked(history.fifFilterFollowsDoc);
    _projectPanelCB[0]->setChecked(history.fifProjectPanel1);
    _projectPanelCB[1]->setChecked(history.fifProjectPanel2);
    _projectPanelCB[2]->setChecked(history.fifProjectPanel3);
    if (!history.fifProjectPanel1 && !history.fifProjectPanel2 &&
        !history.fifProjectPanel3)
        _projectPanelCB[0]->setChecked(true);

    _modeNormal->setChecked(history.searchMode == 0);
    _modeExtended->setChecked(history.searchMode == 1);
    _modeRegex->setChecked(history.searchMode == 2);
    _dotMatchNewline->setChecked(history.dotMatchesNewline);
    _transGB->setChecked(history.transparencyMode != 0);
    _transOnLostFocusRB->setChecked(history.transparencyMode == 1);
    _transAlwaysRB->setChecked(history.transparencyMode == 2);
    _transSlider->setValue(qBound(20, history.transparency * 100 / 255, 100));
    applyTransparency();
}

void FindReplaceDlg::saveFindHistory()
{
    FindHistoryState& history = NppParameters::getInstance().getFindHistory();
    trimComboHistory(_findCombo, history.nbMaxFind);
    trimComboHistory(_replaceCombo, history.nbMaxReplace);
    trimComboHistory(_filtersCombo, history.nbMaxFilter);
    trimComboHistory(_dirCombo, history.nbMaxPath);

    auto comboItems = [](QComboBox* combo, int maxItems) {
        QStringList items;
        for (int i = 0; combo && i < combo->count() && items.size() < maxItems; ++i) {
            QString text = combo->itemText(i);
            if (!text.isEmpty() && !items.contains(text))
                items.append(text);
        }
        return items;
    };

    history.finds = comboItems(_findCombo, history.nbMaxFind);
    history.replaces = comboItems(_replaceCombo, history.nbMaxReplace);
    history.filters = comboItems(_filtersCombo, history.nbMaxFilter);
    history.paths = comboItems(_dirCombo, history.nbMaxPath);

    int tab = _tabBar->currentIndex();
    history.matchWord =
        (tab == 1) ? _wholeWord2->isChecked() :
        (tab == 4) ? _wholeWord4->isChecked() : _wholeWord->isChecked();
    history.matchCase =
        (tab == 1) ? _matchCase2->isChecked() :
        (tab == 4) ? _matchCase4->isChecked() : _matchCase->isChecked();
    history.wrap = (tab == 1) ? _wrapAround2->isChecked() : _wrapAround->isChecked();
    history.directionDown = !_backwardDir->isChecked();
    history.fifRecursive = _recursiveCB->isChecked();
    history.fifInHiddenFolder = _inHiddenDirCB->isChecked();
    history.fifFilterFollowsDoc = _followDocCB->isChecked();
    history.fifProjectPanel1 = _projectPanelCB[0]->isChecked();
    history.fifProjectPanel2 = _projectPanelCB[1]->isChecked();
    history.fifProjectPanel3 = _projectPanelCB[2]->isChecked();
    history.searchMode = _modeRegex->isChecked() ? 2 : (_modeExtended->isChecked() ? 1 : 0);
    history.transparencyMode = !_transGB->isChecked() ? 0 : (_transAlwaysRB->isChecked() ? 2 : 1);
    history.transparency = qBound(0, _transSlider->value() * 255 / 100, 255);
    history.dotMatchesNewline = _dotMatchNewline->isChecked();

    NppParameters::getInstance().writeFindHistory();
}

// ─── Slots ────────────────────────────────────────────────────────────────────

void FindReplaceDlg::onTabChanged(int index)
{
    _replaceRow->setVisible(index == 1 || index == 2 || index == 3);
    _inSelFindRow->setVisible(index == 0);   // 仅在查找 tab 显示
    _filtersRow->setVisible(index == 2 || index == 3);
    _dirRow->setVisible(index == 2);

    _optionsStack->setCurrentIndex(index);
    _btnStack->setCurrentIndex(index);

    _modeBox->setEnabled(true);
    _statusLabel->clear();
}

void FindReplaceDlg::onFindNext()
{
    bool forward = true;
    if (_tabBar->currentIndex() == 0 && _backwardDir->isChecked())
        forward = false;

    if (!doFind(forward))
        setStatus(tr("Not found: \"%1\"").arg(currentFindText()));
    else
        _statusLabel->clear();
}

void FindReplaceDlg::onCount()
{
    int n = countOccurrences();
    if (n == 0)
        setStatus(tr("Not found: \"%1\"").arg(currentFindText()));
    else
        setStatus(tr("Count: %1 match(es).").arg(n), false);
}

void FindReplaceDlg::onFindAllInCurrentDoc()
{
    if (!_currentView) return;
    QString text = currentFindText();
    if (text.isEmpty()) return;

    if (_modeExtended->isChecked())
        text = processExtended(text);

    FindOption opt = buildOptions();
    if (opt._isRegex && opt._dotMatchesNewline)
        text.prepend("(?s)");
    addToFindHistory(currentFindText());

    // ── 1. 清除当前文档中旧的 Find All 高亮（indicator 31）─────────────────────
    const int FIND_MARK = ScintillaEditView::FIND_MARK_INDICATOR;
    const qintptr docLen = _currentView->documentLengthNpp();
    _currentView->SendScintilla(SCI_SETINDICATORCURRENT, (unsigned long)FIND_MARK);
    _currentView->SendScintillaNpp(
        SCI_INDICATORCLEARRANGE, 0, docLen);

    // ── 2. 确定搜索范围 ────────────────────────────────────────────────────────
    qintptr rangeStart = 0;
    qintptr rangeEnd   = docLen;
    bool inSel = isInSelectionMode() && _currentView->hasSelectedText();
    if (inSel)
        _currentView->getSelectionNpp(&rangeStart, &rangeEnd);

    // ── 3. 搜索循环：收集所有匹配并应用高亮 ────────────────────────────────────
    QList<FindAllResult> results;

    _currentView->setCurrentPositionNpp(rangeStart);
    bool found = _currentView->findFirst(
        text, opt._isRegex, opt._isMatchCase, opt._isWholeWord,
        /*wrap=*/false, /*forward=*/true);

    while (found) {
        qintptr matchStart = 0;
        qintptr matchEnd = 0;
        _currentView->getSelectionNpp(&matchStart, &matchEnd);
        if (inSel && matchStart >= rangeEnd) break;

        const qintptr matchLen = matchEnd - matchStart;

        // 对应原版 ProcessFindAll：SCI_SETINDICATORCURRENT + SCI_INDICATORFILLRANGE
        if (matchLen > 0) {
            _currentView->SendScintilla(SCI_SETINDICATORCURRENT,
                                        (unsigned long)FIND_MARK);
            _currentView->SendScintillaNpp(
                SCI_INDICATORFILLRANGE,
                static_cast<quintptr>(matchStart), matchLen);
        }

        // 收集结果信息（对应原版 FoundInfo）
        const int lineNo = static_cast<int>(_currentView->SendScintillaNpp(
            SCI_LINEFROMPOSITION,
            static_cast<quintptr>(matchStart)));
        QString lineText = _currentView->text(lineNo);
        // 去掉行尾换行符
        while (!lineText.isEmpty() &&
               (lineText.back() == '\n' || lineText.back() == '\r'))
            lineText.chop(1);

        FindAllResult r;
        r.lineNo     = lineNo;
        r.matchStart = matchStart;
        r.matchLen   = matchLen;
        const qintptr lineStart = _currentView->SendScintillaNpp(
            SCI_POSITIONFROMLINE,
            static_cast<quintptr>(lineNo));
        r.lineMatchStart = matchStart - lineStart;
        r.lineText   = lineText;
        results.append(r);

        found = _currentView->findNext();
    }

    // 终止查找状态并清除编辑器查找高亮。
    _currentView->findFirst(QString(), false, false, false, false, true);

    // ── 4. 显示结果 ────────────────────────────────────────────────────────────
    if (results.isEmpty()) {
        setStatus(tr("Not found: \"%1\"").arg(currentFindText()));
    } else {
        setStatus(tr("Find All: %1 match(es) found.").arg(results.count()), false);
        emit findAllResultsReady(currentFindText(), results);
    }
}

void FindReplaceDlg::onFindAllInOpenedDocs()
{
    QString text = currentFindText();
    if (text.isEmpty()) return;

    FindOption opt = buildOptions();
    if (_modeExtended->isChecked())
        text = processExtended(text);

    addToFindHistory(currentFindText());
    emit findAllOpenedDocsRequested(text, opt);
}

void FindReplaceDlg::onFindAllInFiles()
{
    QString text = currentFindText();
    QString dir = _dirCombo->currentText();
    if (text.isEmpty() || dir.isEmpty()) return;

    FindOption opt = buildOptions();
    opt._isMatchCase = _matchCase3->isChecked();
    opt._isWholeWord = _wholeWord3->isChecked();
    opt._isWrapAround = false;
    opt._isForward = true;
    if (_modeExtended->isChecked())
        text = processExtended(text);

    addToFindHistory(currentFindText());
    if (!_filtersCombo->currentText().isEmpty()) {
        int idx = _filtersCombo->findText(_filtersCombo->currentText());
        if (idx != -1) _filtersCombo->removeItem(idx);
        _filtersCombo->insertItem(0, _filtersCombo->currentText());
        _filtersCombo->setCurrentIndex(0);
    }
    if (!dir.isEmpty()) {
        int idx = _dirCombo->findText(dir);
        if (idx != -1) _dirCombo->removeItem(idx);
        _dirCombo->insertItem(0, dir);
        _dirCombo->setCurrentIndex(0);
    }
    saveFindHistory();

    emit findInFilesRequested(text, opt, dir, _filtersCombo->currentText(),
                              _recursiveCB->isChecked(),
                              _inHiddenDirCB->isChecked());
}

void FindReplaceDlg::onFindAllInProjects()
{
    QString text = currentFindText();
    if (text.isEmpty())
        return;
    FindOption opt = buildOptions();
    opt._isMatchCase = _matchCase3->isChecked();
    opt._isWholeWord = _wholeWord3->isChecked();
    opt._isWrapAround = false;
    if (_modeExtended->isChecked())
        text = processExtended(text);
    int panelMask = 0;
    for (int i = 0; i < 3; ++i)
        if (_projectPanelCB[i]->isChecked())
            panelMask |= 1 << i;
    if (panelMask == 0)
        return;
    addToFindHistory(currentFindText());
    saveFindHistory();
    emit findInProjectsRequested(
        text, opt, _filtersCombo->currentText(), panelMask);
}

void FindReplaceDlg::onReplace()
{
    if (!_currentView) return;
    QString search  = currentFindText();
    QString replace = currentReplaceText();
    if (search.isEmpty()) return;

    FindOption opt = buildOptions();
    if (_modeExtended->isChecked()) {
        search = processExtended(search);
        replace = processExtended(replace);
    }
    if (opt._isRegex && opt._dotMatchesNewline)
        search.prepend("(?s)");

    bool replacedSelection = false;
    if (_currentView->hasSelectedText()) {
        QString sel = _currentView->selectedText();
        const bool match = ScintillaTextSearch::replaceWholeMatch(
            sel, search, replace, opt._isRegex, opt._isMatchCase,
            opt._isWholeWord, opt._dotMatchesNewline);
        if (match) {
            _currentView->replaceSelectedText(sel);
            addToFindHistory(currentFindText());
            addToReplaceHistory(currentReplaceText());
            replacedSelection = true;
        }
    }

    bool found = replacedSelection || _currentView->findFirst(
        search, opt._isRegex, opt._isMatchCase, opt._isWholeWord,
        opt._isWrapAround, true);

    if (!found)
        setStatus(tr("Not found: \"%1\"").arg(currentFindText()));
    else
        _statusLabel->clear();
}

void FindReplaceDlg::onReplaceAll()
{
    if (!_currentView) return;
    QString search  = currentFindText();
    QString replace = currentReplaceText();
    if (search.isEmpty()) return;

    FindOption opt = buildOptions();
    if (_modeExtended->isChecked()) {
        search = processExtended(search);
        replace = processExtended(replace);
    }
    if (opt._isRegex && opt._dotMatchesNewline)
        search.prepend("(?s)");
    addToFindHistory(currentFindText());
    addToReplaceHistory(replace);

    int count = 0;

    if (isInSelectionMode() && _currentView->hasSelectedText()) {
        // ── 选区内替换全部 ────────────────────────────────────────────────
        qintptr selStart = 0;
        qintptr selEnd = 0;
        _currentView->getSelectionNpp(&selStart, &selEnd);

        _currentView->setCurrentPositionNpp(selStart);
        bool found = _currentView->findFirst(
            search, opt._isRegex, opt._isMatchCase, opt._isWholeWord,
            false, true);
        while (found) {
            qintptr matchStart = 0;
            qintptr matchEnd = 0;
            _currentView->getSelectionNpp(&matchStart, &matchEnd);
            if (matchStart >= selEnd) break;
            const qintptr lengthBefore = _currentView->documentLengthNpp();
            _currentView->replace(replace);
            const qintptr lengthAfter = _currentView->documentLengthNpp();
            selEnd += lengthAfter - lengthBefore;
            ++count;
            if (matchStart == matchEnd) {
                if (matchStart >= lengthBefore) {
                    found = false;
                } else {
                    const qintptr shiftedOriginal =
                        matchStart + (lengthAfter - lengthBefore);
                    const qintptr nextPosition =
                        _currentView->SendScintillaNpp(
                            SCI_POSITIONAFTER,
                            static_cast<quintptr>(shiftedOriginal));
                    found = findFromPosition(
                        _currentView, search, opt, nextPosition);
                }
            } else {
                found = _currentView->findNext();
            }
        }
    } else {
        _currentView->setCurrentPositionNpp(0);
        bool found = _currentView->findFirst(
            search, opt._isRegex, opt._isMatchCase, opt._isWholeWord,
            false, true);
        while (found) {
            qintptr matchStart = 0;
            qintptr matchEnd = 0;
            _currentView->getSelectionNpp(&matchStart, &matchEnd);
            const qintptr lengthBefore = _currentView->documentLengthNpp();
            _currentView->replace(replace);
            const qintptr lengthAfter = _currentView->documentLengthNpp();
            ++count;
            if (matchStart == matchEnd) {
                if (matchStart >= lengthBefore) {
                    found = false;
                } else {
                    const qintptr shiftedOriginal =
                        matchStart + (lengthAfter - lengthBefore);
                    const qintptr nextPosition =
                        _currentView->SendScintillaNpp(
                            SCI_POSITIONAFTER,
                            static_cast<quintptr>(shiftedOriginal));
                    found = findFromPosition(
                        _currentView, search, opt, nextPosition);
                }
            } else {
                found = _currentView->findNext();
            }
        }
    }

    if (count == 0)
        setStatus(tr("No replacements made."));
    else
        setStatus(tr("%1 replacement(s) made.").arg(count), false);
}

void FindReplaceDlg::onReplaceAllOpenedDocs()
{
    QString search = currentFindText();
    QString replacement = currentReplaceText();
    if (search.isEmpty()) return;
    FindOption options = buildOptions();
    if (_modeExtended->isChecked()) {
        search = processExtended(search);
        replacement = processExtended(replacement);
    }
    addToFindHistory(currentFindText());
    addToReplaceHistory(currentReplaceText());
    saveFindHistory();
    emit replaceAllOpenedDocsRequested(search, replacement, options);
}

void FindReplaceDlg::onReplaceInFiles()
{
    QString search = currentFindText();
    QString replacement = currentReplaceText();
    const QString directory = _dirCombo->currentText();
    if (search.isEmpty() || directory.isEmpty()) return;
    FindOption options = buildOptions();
    options._isMatchCase = _matchCase3->isChecked();
    options._isWholeWord = _wholeWord3->isChecked();
    options._isWrapAround = false;
    if (_modeExtended->isChecked()) {
        search = processExtended(search);
        replacement = processExtended(replacement);
    }
    addToFindHistory(currentFindText());
    addToReplaceHistory(currentReplaceText());
    saveFindHistory();
    emit replaceInFilesRequested(search, replacement, options, directory,
                                 _filtersCombo->currentText(),
                                 _recursiveCB->isChecked(),
                                 _inHiddenDirCB->isChecked());
}

void FindReplaceDlg::onReplaceInProjects()
{
    QString search = currentFindText();
    QString replacement = currentReplaceText();
    if (search.isEmpty())
        return;
    FindOption options = buildOptions();
    options._isMatchCase = _matchCase3->isChecked();
    options._isWholeWord = _wholeWord3->isChecked();
    options._isWrapAround = false;
    if (_modeExtended->isChecked()) {
        search = processExtended(search);
        replacement = processExtended(replacement);
    }
    int panelMask = 0;
    for (int i = 0; i < 3; ++i)
        if (_projectPanelCB[i]->isChecked())
            panelMask |= 1 << i;
    if (panelMask == 0)
        return;
    addToFindHistory(currentFindText());
    addToReplaceHistory(currentReplaceText());
    saveFindHistory();
    emit replaceInProjectsRequested(
        search, replacement, options, _filtersCombo->currentText(), panelMask);
}

void FindReplaceDlg::onMarkAll()
{
    if (!_currentView) return;
    if (currentFindText().isEmpty()) return;
    markAllOccurrences(_bookmarkLineCB->isChecked(), _purgeMarksCB->isChecked());
}

void FindReplaceDlg::onClearAllMarks()
{
    if (!_currentView) return;
    _currentView->markerDeleteAll(BOOKMARK_MARKER);
    _statusLabel->clear();
}

void FindReplaceDlg::onCopyMarkedText()
{
    if (!_currentView)
        return;
    const QString text =
        _currentView->markedText(ScintillaEditView::FIND_MARK_INDICATOR);
    if (text.isEmpty()) {
        setStatus(tr("No marked text to copy."));
        return;
    }
    QApplication::clipboard()->setText(text);
    setStatus(tr("Marked text copied to clipboard."), false);
}

bool FindReplaceDlg::executeSavedMacroAction(
    int message, int value, const QString& text)
{
    enum {
        FindWhat = 1601, ReplaceWith = 1602, Replace = 1608,
        ReplaceAll = 1609, CountAll = 1614, MarkAll = 1615,
        SearchModeCommand = 1625, ReplaceOpened = 1635,
        FindAllOpened = 1636, FindInFiles = 1638,
        FindAllCurrent = 1641, Filters = 1652, Directory = 1653,
        ReplaceInFiles = 1660, ReplaceInProjects = 1665,
        FindInProjects = 1666, SavedInit = 1700,
        SavedExecute = 1701, SavedBooleans = 1702,
        FindPrevious = 1721, FindNext = 1723
    };
    if (message == SavedInit) {
        _savedMacroSearch = SavedMacroSearch();
        return true;
    }
    if (message == FindWhat) {
        _savedMacroSearch.findText = text;
        return true;
    }
    if (message == ReplaceWith) {
        _savedMacroSearch.replaceText = text;
        return true;
    }
    if (message == Directory) {
        _savedMacroSearch.directory = text;
        return true;
    }
    if (message == Filters) {
        _savedMacroSearch.filters = text;
        return true;
    }
    if (message == SearchModeCommand) {
        _savedMacroSearch.options._mode =
            value == 1 ? SearchMode::Extended
                       : value == 2 ? SearchMode::Regex
                                    : SearchMode::Normal;
        _savedMacroSearch.options._isRegex =
            _savedMacroSearch.options._mode == SearchMode::Regex;
        return true;
    }
    if (message == SavedBooleans) {
        _savedMacroSearch.options._isWholeWord = value & 1;
        _savedMacroSearch.options._isMatchCase = value & 2;
        _savedMacroSearch.purge = value & 4;
        _savedMacroSearch.bookmarkLine = value & 16;
        _savedMacroSearch.recursive = value & 32;
        _savedMacroSearch.includeHidden = value & 64;
        _savedMacroSearch.inSelection = value & 128;
        _savedMacroSearch.options._isWrapAround = value & 256;
        _savedMacroSearch.options._isForward = !(value & 512);
        _savedMacroSearch.options._dotMatchesNewline = value & 1024;
        _savedMacroSearch.projectMask =
            ((value & 128) ? 1 : 0)
            | ((value & 256) ? 2 : 0)
            | ((value & 512) ? 4 : 0);
        return true;
    }
    if (message != SavedExecute)
        return false;

    setSearchText(_savedMacroSearch.findText);
    _replaceCombo->setEditText(_savedMacroSearch.replaceText);
    _dirCombo->setEditText(_savedMacroSearch.directory);
    _filtersCombo->setEditText(_savedMacroSearch.filters);
    _modeNormal->setChecked(
        _savedMacroSearch.options._mode == SearchMode::Normal);
    _modeExtended->setChecked(
        _savedMacroSearch.options._mode == SearchMode::Extended);
    _modeRegex->setChecked(
        _savedMacroSearch.options._mode == SearchMode::Regex);
    _dotMatchNewline->setChecked(
        _savedMacroSearch.options._dotMatchesNewline);
    for (QCheckBox* box : {_matchCase, _matchCase2,
                           _matchCase3, _matchCase4})
        if (box) box->setChecked(_savedMacroSearch.options._isMatchCase);
    for (QCheckBox* box : {_wholeWord, _wholeWord2,
                           _wholeWord3, _wholeWord4})
        if (box) box->setChecked(_savedMacroSearch.options._isWholeWord);
    for (QCheckBox* box : {_wrapAround, _wrapAround2})
        if (box) box->setChecked(_savedMacroSearch.options._isWrapAround);
    for (QCheckBox* box : {_inSelCB, _inSelCB2, _inSelCB4})
        if (box) box->setChecked(_savedMacroSearch.inSelection);
    _recursiveCB->setChecked(_savedMacroSearch.recursive);
    _inHiddenDirCB->setChecked(_savedMacroSearch.includeHidden);
    _purgeMarksCB->setChecked(_savedMacroSearch.purge);
    _bookmarkLineCB->setChecked(_savedMacroSearch.bookmarkLine);
    for (int i = 0; i < 3; ++i)
        _projectPanelCB[i]->setChecked(
            _savedMacroSearch.projectMask & (1 << i));

    switch (value) {
    case 1:
    case FindNext: return doFind(true);
    case FindPrevious: return doFind(false);
    case Replace: onReplace(); return true;
    case ReplaceAll: onReplaceAll(); return true;
    case CountAll: onCount(); return true;
    case MarkAll: onMarkAll(); return true;
    case FindAllOpened: onFindAllInOpenedDocs(); return true;
    case FindAllCurrent: onFindAllInCurrentDoc(); return true;
    case ReplaceOpened: onReplaceAllOpenedDocs(); return true;
    case FindInFiles: onFindAllInFiles(); return true;
    case ReplaceInFiles: onReplaceInFiles(); return true;
    case FindInProjects: onFindAllInProjects(); return true;
    case ReplaceInProjects: onReplaceInProjects(); return true;
    default: return false;
    }
}

void FindReplaceDlg::onTransparencyChanged()
{
    applyTransparency();
}

// ─── 透明度 ───────────────────────────────────────────────────────────────────

void FindReplaceDlg::applyTransparency()
{
    if (!_transGB || !_transGB->isChecked()) {
        setWindowOpacity(1.0);
        return;
    }
    double opacity = _transSlider->value() / 100.0;
    if (_transOnLostFocusRB->isChecked() && isActiveWindow()) {
        setWindowOpacity(1.0);  // 聚焦时完全不透明
    } else {
        setWindowOpacity(opacity);
    }
}

void FindReplaceDlg::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::ActivationChange) {
        applyTransparency();
    }
    QDialog::changeEvent(event);
}

// ─── 私有实现 ─────────────────────────────────────────────────────────────────

bool FindReplaceDlg::isInSelectionMode() const
{
    int tab = _tabBar->currentIndex();
    switch (tab) {
    case 0: return _inSelCB  && _inSelCB->isChecked();
    case 1: return _inSelCB2 && _inSelCB2->isChecked();
    case 4: return _inSelCB4 && _inSelCB4->isChecked();
    default: return false;
    }
}

bool FindReplaceDlg::doFind(bool forward)
{
    if (!_currentView) return false;
    QString text = currentFindText();
    if (text.isEmpty()) return false;

    if (_modeExtended->isChecked())
        text = processExtended(text);

    FindOption opt = buildOptions();
    if (opt._isRegex && opt._dotMatchesNewline)
        text.prepend("(?s)");
    opt._isForward = forward;
    addToFindHistory(currentFindText());

    return _currentView->findFirst(
        text, opt._isRegex, opt._isMatchCase, opt._isWholeWord,
        opt._isWrapAround, forward);
}

int FindReplaceDlg::countOccurrences()
{
    if (!_currentView) return 0;
    QString text = currentFindText();
    if (text.isEmpty()) return 0;

    if (_modeExtended->isChecked())
        text = processExtended(text);

    FindOption opt = buildOptions();
    if (opt._isRegex && opt._dotMatchesNewline)
        text.prepend("(?s)");
    int count = 0;
    qintptr originalStart = 0;
    qintptr originalEnd = 0;
    _currentView->getSelectionNpp(&originalStart, &originalEnd);
    const int originalFirstVisible = _currentView->firstVisibleLine();
    const int originalXOffset = static_cast<int>(_currentView->SendScintilla(
        SCI_GETXOFFSET));

    if (isInSelectionMode() && _currentView->hasSelectedText()) {
        // ── 选区内计数 ─────────────────────────────────────────────────────
        qintptr selStart = 0;
        qintptr selEnd = 0;
        _currentView->getSelectionNpp(&selStart, &selEnd);

        _currentView->setCurrentPositionNpp(selStart);
        bool found = _currentView->findFirst(
            text, opt._isRegex, opt._isMatchCase, opt._isWholeWord,
            false, true);
        while (found) {
            qintptr matchStart = 0;
            _currentView->getSelectionNpp(&matchStart, nullptr);
            if (matchStart >= selEnd) break;
            ++count;
            found = _currentView->findNext();
        }
    } else {
        _currentView->setCurrentPositionNpp(0);
        bool found = _currentView->findFirst(
            text, opt._isRegex, opt._isMatchCase, opt._isWholeWord,
            false, true);
        while (found) { ++count; found = _currentView->findNext(); }
    }

    _currentView->findFirst("", false, false, false, false, true);
    _currentView->SendScintillaNpp(
        SCI_SETSEL,
        static_cast<quintptr>(originalStart), originalEnd);
    _currentView->SendScintilla(SCI_SETFIRSTVISIBLELINE,
                                static_cast<unsigned long>(originalFirstVisible));
    _currentView->SendScintilla(SCI_SETXOFFSET,
                                static_cast<unsigned long>(originalXOffset));
    return count;
}

void FindReplaceDlg::markAllOccurrences(bool bookmarkLine, bool purge)
{
    if (!_currentView) return;
    QString text = currentFindText();
    if (text.isEmpty()) return;

    if (_modeExtended->isChecked())
        text = processExtended(text);

    if (purge)
        _currentView->markerDeleteAll(BOOKMARK_MARKER);

    _currentView->markerDefine(Circle, BOOKMARK_MARKER);
    _currentView->setMarkerBackgroundColor(QColor(0x0A, 0x24, 0x6A), BOOKMARK_MARKER);

    FindOption opt = buildOptions();
    if (opt._isRegex && opt._dotMatchesNewline)
        text.prepend("(?s)");
    int count = 0;
    qintptr originalStart = 0;
    qintptr originalEnd = 0;
    _currentView->getSelectionNpp(&originalStart, &originalEnd);

    if (isInSelectionMode() && _currentView->hasSelectedText()) {
        // ── 选区内标记 ─────────────────────────────────────────────────────
        qintptr selStart = 0;
        qintptr selEnd = 0;
        _currentView->getSelectionNpp(&selStart, &selEnd);

        _currentView->setCurrentPositionNpp(selStart);
        bool found = _currentView->findFirst(
            text, opt._isRegex, _matchCase4->isChecked(),
            _wholeWord4->isChecked(), false, true);
        while (found) {
            qintptr matchStart = 0;
            _currentView->getSelectionNpp(&matchStart, nullptr);
            if (matchStart >= selEnd) break;
            if (bookmarkLine) {
                const int line = static_cast<int>(
                    _currentView->SendScintillaNpp(
                        SCI_LINEFROMPOSITION,
                        static_cast<quintptr>(matchStart)));
                _currentView->markerAdd(line, BOOKMARK_MARKER);
            }
            ++count;
            found = _currentView->findNext();
        }
    } else {
        _currentView->setCurrentPositionNpp(0);
        bool found = _currentView->findFirst(
            text, opt._isRegex, _matchCase4->isChecked(),
            _wholeWord4->isChecked(), false, true);
        while (found) {
            int line, idx;
            _currentView->getCursorPosition(&line, &idx);
            if (bookmarkLine) _currentView->markerAdd(line, BOOKMARK_MARKER);
            ++count;
            found = _currentView->findNext();
        }
    }

    _currentView->findFirst("", false, false, false, false, true);
    _currentView->SendScintillaNpp(
        SCI_SETSEL,
        static_cast<quintptr>(originalStart), originalEnd);
    addToFindHistory(currentFindText());

    if (count == 0)
        setStatus(tr("Not found: \"%1\"").arg(currentFindText()));
    else
        setStatus(tr("Marked %1 match(es).").arg(count), false);
}

QString FindReplaceDlg::processExtended(const QString& text) const
{
    QString result;
    result.reserve(text.size());
    for (int i = 0; i < text.size(); ++i) {
        if (text[i] == '\\' && i + 1 < text.size()) {
            ++i;
            switch (text[i].toLatin1()) {
                case 'n':  result += '\n'; break;
                case 't':  result += '\t'; break;
                case 'r':  result += '\r'; break;
                case '0':  result += '\0'; break;
                case '\\': result += '\\'; break;
                default:   result += '\\'; result += text[i]; break;
            }
        } else {
            result += text[i];
        }
    }
    return result;
}

QString FindReplaceDlg::currentFindText() const  { return _findCombo->currentText(); }
QString FindReplaceDlg::currentReplaceText() const { return _replaceCombo->currentText(); }

FindOption FindReplaceDlg::buildOptions() const
{
    FindOption opt;
    int tab = _tabBar->currentIndex();
    opt._isRegex = _modeRegex->isChecked();
    opt._dotMatchesNewline = opt._isRegex && _dotMatchNewline->isChecked();

    switch (tab) {
    case 0:
        opt._isMatchCase  = _matchCase->isChecked();
        opt._isWholeWord  = _wholeWord->isChecked();
        opt._isWrapAround = _wrapAround->isChecked();
        opt._isForward    = !_backwardDir->isChecked();
        break;
    case 1:
        opt._isMatchCase  = _matchCase2->isChecked();
        opt._isWholeWord  = _wholeWord2->isChecked();
        opt._isWrapAround = _wrapAround2->isChecked();
        opt._isForward    = true;
        break;
    case 4:
        opt._isMatchCase  = _matchCase4->isChecked();
        opt._isWholeWord  = _wholeWord4->isChecked();
        opt._isWrapAround = false;
        opt._isForward    = true;
        break;
    default:
        break;
    }
    return opt;
}

void FindReplaceDlg::addToFindHistory(const QString& text)
{
    if (text.isEmpty()) return;
    int idx = _findCombo->findText(text);
    if (idx != -1) _findCombo->removeItem(idx);
    _findCombo->insertItem(0, text);
    _findCombo->setCurrentIndex(0);
    trimComboHistory(_findCombo, NppParameters::getInstance().getFindHistory().nbMaxFind);
    saveFindHistory();
}

void FindReplaceDlg::addToReplaceHistory(const QString& text)
{
    int idx = _replaceCombo->findText(text);
    if (idx != -1) _replaceCombo->removeItem(idx);
    _replaceCombo->insertItem(0, text);
    _replaceCombo->setCurrentIndex(0);
    trimComboHistory(_replaceCombo, NppParameters::getInstance().getFindHistory().nbMaxReplace);
    saveFindHistory();
}

void FindReplaceDlg::trimComboHistory(QComboBox* combo, int maxItems)
{
    if (!combo || maxItems <= 0) return;
    while (combo->count() > maxItems)
        combo->removeItem(combo->count() - 1);
}

void FindReplaceDlg::setStatus(const QString& msg, bool isError)
{
    _statusLabel->setText(msg);
    _statusLabel->setStyleSheet(isError ? "color: red;" : "color: blue;");
}

void FindReplaceDlg::closeEvent(QCloseEvent* event)
{
    saveFindHistory();
    hide();
    event->ignore();
}

void FindReplaceDlg::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) { hide(); return; }
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        && _findCombo->hasFocus()) {
        onFindNext();
        return;
    }
    QDialog::keyPressEvent(event);
}
