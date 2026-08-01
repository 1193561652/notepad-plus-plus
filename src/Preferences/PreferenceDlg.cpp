// PreferenceDlg.cpp - 偏好设置对话框实现（19 个分类页面，与原版对应）

#include "PreferenceDlg.h"
#include "Parameters.h"
#include "MISC/FileAssociationModel.h"
#include "MISC/PlatformServices.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QSplitter>
#include <QFrame>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QDir>
#include <QCoreApplication>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QStyle>
#include <QSlider>
#include <QTextEdit>
#include <QRegExp>

namespace {

QRect prefGeometry(int x, int y, int width, int height)
{
    return QRect(qRound(x * 1.45), qRound(y * 1.58),
                 qRound(width * 1.45), qRound(height * 1.58));
}

template <typename T>
T* placePreferenceControl(T* control, int x, int y, int width, int height)
{
    control->setGeometry(prefGeometry(x, y, width, height));
    return control;
}

QWidget* preferenceCanvas()
{
    QWidget* page = new QWidget();
    page->setFixedSize(660, 320);
    return page;
}

} // namespace

// ── 辅助：将内容 widget 包入 QScrollArea ─────────────────────────────────────
QScrollArea* PreferenceDlg::wrapScroll(QWidget* inner)
{
    QScrollArea* sa = new QScrollArea();
    sa->setWidget(inner);
    sa->setWidgetResizable(true);
    sa->setFrameShape(QFrame::NoFrame);
    return sa;
}

// ─────────────────────────────────────────────────────────────────────────────

PreferenceDlg::PreferenceDlg(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Preferences"));
    setFixedSize(830, 372);
    setupUi();
    loadSettings();
}

void PreferenceDlg::setupUi()
{
    // 左侧列表
    _pageList = new QListWidget(this);
    _pageList->setObjectName("pageList");
    _pageList->setGeometry(10, 10, 120, 330);
    _pageList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _pageList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 右侧堆叠
    _pageStack = new QStackedWidget(this);
    _pageStack->setGeometry(158, 1, 666, 336);

    // 按原版顺序添加页面
    struct { QString name; QWidget* page; } pages[] = {
        { tr("General"),               makePage_General()           },
        { tr("Editing"),               makePage_Editing()           },
        { tr("Dark Mode"),             makePage_DarkMode()          },
        { tr("Margins/Border/Edge"),   makePage_MarginsBorderEdge() },
        { tr("New Document"),          makePage_NewDocument()       },
        { tr("Default Directory"),     makePage_DefaultDirectory()  },
        { tr("Recent Files History"),  makePage_RecentFilesHistory()},
        { tr("File Association"),      makePage_FileAssociation()   },
        { tr("Language"),              makePage_Language()          },
        { tr("Highlighting"),          makePage_Highlighting()      },
        { tr("Print"),                 makePage_Print()             },
        { tr("Searching"),             makePage_Searching()         },
        { tr("Backup"),                makePage_Backup()            },
        { tr("Auto-Completion"),       makePage_AutoCompletion()    },
        { tr("Multi-Instance & Date"), makePage_MultiInstance()     },
        { tr("Delimiter"),             makePage_Delimiter()         },
        { tr("Cloud & Link"),          makePage_CloudLink()         },
        { tr("Search Engine"),         makePage_SearchEngine()      },
        { tr("MISC."),                 makePage_Misc()              },
    };

    for (auto& p : pages) {
        _pageList->addItem(p.name);
        _pageStack->addWidget(p.page);
    }

    auto ensureStableName = [](QObject* object, const QString& source) {
        if (!object->objectName().isEmpty() || source.trimmed().isEmpty())
            return;
        QString slug = source;
        slug.remove(QLatin1Char('&'));
        slug.replace(QRegExp(QStringLiteral("[^A-Za-z0-9]+")),
                     QStringLiteral("_"));
        slug = slug.left(72).trimmed();
        if (!slug.isEmpty())
            object->setObjectName(QStringLiteral("prefText_") + slug);
    };
    for (QLabel* label : findChildren<QLabel*>())
        ensureStableName(label, label->text());
    for (QGroupBox* group : findChildren<QGroupBox*>())
        ensureStableName(group, group->title());
    for (QAbstractButton* button : findChildren<QAbstractButton*>())
        ensureStableName(button, button->text());

    connect(_pageList, &QListWidget::currentRowChanged,
            _pageStack, &QStackedWidget::setCurrentIndex);
    _pageList->setCurrentRow(0);

    // 分隔线
    QFrame* sep = new QFrame(this);
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);

    sep->setGeometry(148, 8, 2, 332);

    QPushButton* closeButton = new QPushButton(tr("Close"), this);
    closeButton->setObjectName("btnPrefsClose");
    closeButton->setGeometry(380, 343, 70, 23);
    connect(closeButton, &QPushButton::clicked, this, [this] {
        onApply();
        accept();
    });

    setStyleSheet(QStringLiteral(
        "QGroupBox { margin-top: 7px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 7px; padding: 0 2px; }"
        "QCheckBox, QRadioButton, QLabel { min-height: 17px; }"
        "QListWidget::item { height: 16px; padding: 0 2px; }"));
}

// ─────────────────────────────────────────────────────────────────────────────
// 1. General
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_General()
{
    QWidget* w = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(w);
    lay->setContentsMargins(10, 8, 8, 6);
    lay->setSpacing(4);

    QHBoxLayout* localization = new QHBoxLayout();
    QLabel* localizationLabel = new QLabel(tr("Localization:"), w);
    localizationLabel->setObjectName("lblLocalization");
    localization->addWidget(localizationLabel);
    _languageCombo = new QComboBox(w);
    _languageCombo->setObjectName("comboInterfaceLanguage");
    _languageCombo->addItem(tr("English"), "en");
    _languageCombo->addItem(tr("Chinese Simplified"), "zh_CN");
    _languageCombo->setFixedWidth(145);
    localization->addWidget(_languageCombo);
    localization->addStretch();
    lay->addLayout(localization);

    QHBoxLayout* upper = new QHBoxLayout();
    upper->setSpacing(8);

    // Toolbar
    QGroupBox* tbGroup = new QGroupBox(tr("Tool Bar"));
    tbGroup->setObjectName("grpToolBar");
    QVBoxLayout* tbLay = new QVBoxLayout(tbGroup);
    tbLay->setContentsMargins(8, 8, 8, 5);
    tbLay->setSpacing(0);
    _hideToolbarCB = new QCheckBox(tr("Hide"));
    _hideToolbarCB->setObjectName("chkHideToolbar");
    tbLay->addWidget(_hideToolbarCB);
    const QStringList toolbarModes = {
        tr("Standard icons: small"), tr("Standard icons: large"),
        tr("Fluent UI icons: small"), tr("Fluent UI icons: large"),
        tr("Fluent UI icons: small (dark mode)")
    };
    for (int i = 0; i < toolbarModes.size(); ++i) {
        QRadioButton* mode = new QRadioButton(toolbarModes.at(i), tbGroup);
        mode->setObjectName(QStringLiteral("rbToolbarMode%1").arg(i));
        mode->setProperty("toolbarIconSet", i);
        mode->setChecked(i == 0);
        tbLay->addWidget(mode);
    }
    tbGroup->setFixedWidth(220);
    upper->addWidget(tbGroup);

    // Tab Bar
    QGroupBox* tabGroup = new QGroupBox(tr("Tab Bar"));
    tabGroup->setObjectName("grpTabBar");
    QGridLayout* tabLay = new QGridLayout(tabGroup);
    tabLay->setContentsMargins(8, 8, 8, 5);
    tabLay->setVerticalSpacing(0);
    _tabHideCB          = new QCheckBox(tr("Hide"));
    _tabHideCB->setObjectName("chkTabHide");
    _tabDragDropCB      = new QCheckBox(tr("Enable drag and drop"));
    _tabDragDropCB->setObjectName("chkTabDragDrop");
    _tabTopBarCB        = new QCheckBox(tr("Draw a coloured bar on active tab"));
    _tabTopBarCB->setObjectName("chkTabTopBar");
    _tabInactiveTabCB   = new QCheckBox(tr("Draw inactive tabs differently"));
    _tabInactiveTabCB->setObjectName("chkTabInactiveTab");
    _tabCloseBtnCB      = new QCheckBox(tr("Show close button on each tab"));
    _tabCloseBtnCB->setObjectName("chkTabCloseBtn");
    _tabDblClickCloseCB = new QCheckBox(tr("Double click to close document"));
    _tabDblClickCloseCB->setObjectName("chkTabDblClickClose");
    _tabVerticalCB      = new QCheckBox(tr("Vertical (rotate by 90°)"));
    _tabVerticalCB->setObjectName("chkTabVertical");
    _tabMultiLineCB     = new QCheckBox(tr("Multi-line"));
    _tabMultiLineCB->setObjectName("chkTabMultiLine");
    _tabQuitOnEmptyCB   = new QCheckBox(tr("Exit program when last tab is closed"));
    _tabQuitOnEmptyCB->setObjectName("chkTabQuitOnEmpty");

    QCheckBox* reduceTabBar = new QCheckBox(tr("Reduce"));
    QCheckBox* alternateIcons = new QCheckBox(tr("Use alternate icons"));
    QCheckBox* lockTabBar = new QCheckBox(tr("Lock (no drag and drop)"));
    reduceTabBar->setObjectName("chkTabReduce");
    alternateIcons->setObjectName("chkTabAlternateIcons");
    lockTabBar->setObjectName("chkTabLock");
    tabLay->addWidget(_tabHideCB,          0, 0);
    tabLay->addWidget(_tabMultiLineCB,     1, 0);
    tabLay->addWidget(_tabVerticalCB,      2, 0);
    tabLay->addWidget(reduceTabBar,        3, 0);
    tabLay->addWidget(alternateIcons,      4, 0);
    tabLay->addWidget(lockTabBar,          5, 0);
    tabLay->addWidget(_tabInactiveTabCB,   0, 1);
    tabLay->addWidget(_tabTopBarCB,        1, 1);
    tabLay->addWidget(_tabCloseBtnCB,      2, 1);
    tabLay->addWidget(_tabDblClickCloseCB, 3, 1);
    tabLay->addWidget(_tabQuitOnEmptyCB,   4, 1);
    _tabDragDropCB->hide();
    connect(lockTabBar, &QCheckBox::toggled, this,
            [this](bool locked) { _tabDragDropCB->setChecked(!locked); });
    upper->addWidget(tabGroup, 1);
    lay->addLayout(upper);

    connect(_tabHideCB, &QCheckBox::toggled,
            this, &PreferenceDlg::onTabHideToggled);

    // Status / Menu bar
    QGroupBox* barGroup = new QGroupBox(tr("Menu"));
    barGroup->setObjectName("grpBarGroup");
    QVBoxLayout* barLay = new QVBoxLayout(barGroup);
    _showStatusBarCB = new QCheckBox(tr("Show status bar"));
    _showStatusBarCB->setObjectName("chkShowStatusBar");
    _showMenuBarCB   = new QCheckBox(tr("Show menu bar"));
    _showMenuBarCB->setObjectName("chkShowMenuBar");
    barLay->addWidget(_showMenuBarCB);
    QHBoxLayout* bottom = new QHBoxLayout();
    bottom->addWidget(_showStatusBarCB);
    bottom->addWidget(barGroup, 1);
    barGroup->setFixedHeight(58);
    lay->addStretch();
    lay->addLayout(bottom);

    _fontCombo = new QFontComboBox(w);
    _fontSizeSB = new QSpinBox(w);
    _fontCombo->hide();
    _fontSizeSB->hide();

    // Editor Font（原版在 Style Configurator，此处简化）
    return wrapScroll(w);
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. Editing
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_Editing()
{
    QWidget* w = new QWidget();
    QGridLayout* lay = new QGridLayout(w);
    lay->setContentsMargins(10, 8, 8, 8);
    lay->setSpacing(8);

    // Caret / Cursor behaviour
    QGroupBox* editGroup = new QGroupBox(tr("Caret & Scrolling"));
    editGroup->setObjectName("grpCaretScrolling");
    QVBoxLayout* editLay = new QVBoxLayout(editGroup);
    _autoIndentCB           = new QCheckBox(tr("Auto indent"));
    _autoIndentCB->setObjectName("chkAutoIndent");
    _scrollBeyondLastLineCB = new QCheckBox(tr("Scroll beyond last line"));
    _scrollBeyondLastLineCB->setObjectName("chkScrollBeyondLastLine");
    editLay->addWidget(_autoIndentCB);
    editLay->addWidget(_scrollBeyondLastLineCB);
    lay->addWidget(editGroup, 0, 2);

    // Line Wrap
    QGroupBox* wrapGroup = new QGroupBox(tr("Line Wrap"));
    wrapGroup->setObjectName("grpLineWrap");
    QVBoxLayout* wrapLay = new QVBoxLayout(wrapGroup);
    _wordWrapCB  = new QCheckBox(tr("Enable line wrap"));
    _wordWrapCB->setObjectName("chkWordWrap");
    _lwDefaultRB = new QRadioButton(tr("Default (continuous)"));
    _lwDefaultRB->setObjectName("rbLwDefault");
    _lwAlignRB   = new QRadioButton(tr("Aligned to opening brace"));
    _lwAlignRB->setObjectName("rbLwAlign");
    _lwIndentRB  = new QRadioButton(tr("Indented"));
    _lwIndentRB->setObjectName("rbLwIndent");
    QLabel* lblWrapIndentMode = new QLabel(tr("Wrapped line indent mode:"));
    lblWrapIndentMode->setObjectName("lblWrapIndentMode");
    wrapLay->addWidget(_wordWrapCB);
    wrapLay->addWidget(lblWrapIndentMode);
    wrapLay->addWidget(_lwDefaultRB);
    wrapLay->addWidget(_lwAlignRB);
    wrapLay->addWidget(_lwIndentRB);
    lay->addWidget(wrapGroup, 0, 1);

    // Display
    QGroupBox* dispGroup = new QGroupBox(tr("Display"));
    dispGroup->setObjectName("grpDisplay");
    QVBoxLayout* dispLay = new QVBoxLayout(dispGroup);
    _showWhitespaceCB = new QCheckBox(tr("Show whitespace characters"));
    _showWhitespaceCB->setObjectName("chkShowWhitespace");
    _showEolCB        = new QCheckBox(tr("Show end-of-line characters"));
    _showEolCB->setObjectName("chkShowEol");
    dispLay->addWidget(_showWhitespaceCB);
    dispLay->addWidget(_showEolCB);
    lay->addWidget(dispGroup, 0, 0);
    QGroupBox* options = new QGroupBox(tr("Editing options"), w);
    options->setObjectName("grpEditingOptions");
    QGridLayout* optionLayout = new QGridLayout(options);
    QCheckBox* smoothFont = new QCheckBox(tr("Enable smooth font"), options);
    smoothFont->setObjectName("chkSmoothFont");
    QCheckBox* virtualSpace = new QCheckBox(tr("Enable virtual space"), options);
    virtualSpace->setObjectName("chkVirtualSpace");
    QCheckBox* multiEditing = new QCheckBox(tr("Enable multi-editing"), options);
    multiEditing->setObjectName("chkMultiEditing");
    QCheckBox* rightClickKeeps = new QCheckBox(tr("Right click keeps selection"), options);
    rightClickKeeps->setObjectName("chkRightClickKeepsSelection");
    optionLayout->addWidget(smoothFont, 0, 0);
    optionLayout->addWidget(virtualSpace, 1, 0);
    optionLayout->addWidget(multiEditing, 0, 1);
    optionLayout->addWidget(rightClickKeeps, 1, 1);
    lay->addWidget(options, 1, 0, 1, 3);
    lay->setRowStretch(2, 1);
    return wrapScroll(w);
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. Margins/Border/Edge
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_MarginsBorderEdge()
{
    QWidget* w = preferenceCanvas();
    QGroupBox* fold = placePreferenceControl(
        new QGroupBox(tr("Fold Margin Style"), w), 22, 21, 83, 89);
    const QStringList foldStyles = {
        tr("Simple"), tr("Arrow"), tr("Circle tree"), tr("Box tree"), tr("None")
    };
    for (int i = 0; i < foldStyles.size(); ++i) {
        QRadioButton* radio = placePreferenceControl(
            new QRadioButton(foldStyles.at(i), fold), 9, 12 + i * 14, 68, 12);
        if (i == 3) radio->setChecked(true);
        if (i == 4) connect(radio, &QRadioButton::toggled, this,
                            [this](bool none) { _foldMarginCB->setChecked(!none); });
    }
    Q_UNUSED(fold);

    QGroupBox* edgeGroup = placePreferenceControl(
        new QGroupBox(tr("Vertical Edge Settings"), w), 116, 21, 148, 136);
    QLabel* edgeHelp = placePreferenceControl(new QLabel(
        tr("Add column markers with decimal numbers.\n"
           "Separate several markers with spaces."), edgeGroup),
        8, 12, 134, 47);
    edgeHelp->setWordWrap(true);
    _edgeShowCB = placePreferenceControl(
        new QCheckBox(tr("Display"), edgeGroup), 10, 63, 60, 12);
    _edgeShowCB->setObjectName("chkEdgeShow");
    _edgeColumnLabel = placePreferenceControl(
        new QLabel(tr("Column:"), edgeGroup), 10, 79, 55, 12);
    _edgeColumnLabel->setObjectName("lblEdgeColumn");
    _edgeColumnSB = placePreferenceControl(new QSpinBox(edgeGroup), 65, 76, 68, 16);
    _edgeColumnSB->setRange(1, 500);
    placePreferenceControl(new QCheckBox(tr("Background mode"), edgeGroup),
                           10, 105, 125, 12);

    QGroupBox* border = placePreferenceControl(
        new QGroupBox(tr("Border Width"), w), 22, 112, 83, 45);
    QSlider* borderWidth = placePreferenceControl(
        new QSlider(Qt::Horizontal, border), 7, 13, 60, 14);
    borderWidth->setRange(0, 4);
    placePreferenceControl(new QCheckBox(tr("No edge"), border), 7, 28, 65, 12);

    QGroupBox* lineNumber = placePreferenceControl(
        new QGroupBox(tr("Line Number"), w), 274, 21, 135, 66);
    _lineNumberMarginCB = placePreferenceControl(
        new QCheckBox(tr("Display"), lineNumber), 7, 12, 85, 12);
    _lineNumberMarginCB->setObjectName("chkLineNumbers");
    QRadioButton* dynamicWidth = placePreferenceControl(
        new QRadioButton(tr("Dynamic width"), lineNumber), 18, 28, 110, 12);
    placePreferenceControl(new QRadioButton(tr("Constant width"), lineNumber),
                           18, 43, 110, 12);
    dynamicWidth->setChecked(true);

    QGroupBox* padding = placePreferenceControl(
        new QGroupBox(tr("Padding"), w), 274, 94, 135, 63);
    placePreferenceControl(new QLabel(tr("Left"), padding), 8, 13, 35, 12);
    placePreferenceControl(new QSlider(Qt::Horizontal, padding), 45, 10, 78, 14);
    placePreferenceControl(new QLabel(tr("Right"), padding), 8, 29, 35, 12);
    placePreferenceControl(new QSlider(Qt::Horizontal, padding), 45, 26, 78, 14);
    placePreferenceControl(new QLabel(tr("Distraction Free"), padding), 8, 45, 72, 12);
    placePreferenceControl(new QSlider(Qt::Horizontal, padding), 82, 42, 41, 14);

    _bookMarkMarginCB = placePreferenceControl(
        new QCheckBox(tr("Display bookmark"), w), 281, 163, 145, 12);
    _bookMarkMarginCB->setObjectName("chkBookmarkMargin");
    placePreferenceControl(new QCheckBox(tr("Display Change History"), w),
                           120, 163, 150, 12);

    _foldMarginCB = new QCheckBox(w);
    _foldMarginCB->setObjectName("chkFoldMargin");
    _foldMarginCB->hide();
    _indentGuideLineCB = new QCheckBox(w);
    _indentGuideLineCB->setObjectName("chkIndentGuideLine");
    _indentGuideLineCB->hide();
    _currentLineHighlightCB = new QCheckBox(w);
    _currentLineHighlightCB->setObjectName("chkCurrentLineHighlight");
    _currentLineHighlightCB->hide();
    _wrapSymbolShowCB = new QCheckBox(w);
    _wrapSymbolShowCB->setObjectName("chkWrapSymbolShow");
    _wrapSymbolShowCB->hide();
    connect(_edgeShowCB, &QCheckBox::toggled, _edgeColumnSB, &QSpinBox::setEnabled);
    return w;
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. New Document
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_NewDocument()
{
    QWidget* w = preferenceCanvas();
    placePreferenceControl(new QGroupBox(tr("New Document"), w), 15, 8, 424, 161);
    QGroupBox* fmtGroup = placePreferenceControl(
        new QGroupBox(tr("Format (Line ending)"), w), 58, 29, 129, 79);
    fmtGroup->setObjectName("grpFormat");
    _fmtWindowsRB = placePreferenceControl(
        new QRadioButton(tr("Windows (CR LF)"), fmtGroup), 7, 16, 105, 12);
    _fmtWindowsRB->setObjectName("rbFmtWindows");
    _fmtUnixRB = placePreferenceControl(
        new QRadioButton(tr("Unix (LF)"), fmtGroup), 7, 32, 105, 12);
    _fmtUnixRB->setObjectName("rbFmtUnix");
    _fmtMacRB = placePreferenceControl(
        new QRadioButton(tr("Macintosh (CR)"), fmtGroup), 7, 48, 105, 12);
    _fmtMacRB->setObjectName("rbFmtMac");

    QGroupBox* encGroup = placePreferenceControl(
        new QGroupBox(tr("Encoding"), w), 232, 28, 175, 122);
    encGroup->setObjectName("grpEncoding");
    _encodingCombo = new QComboBox(encGroup);
    _encodingCombo->setObjectName("comboDefaultEncoding");
    const QList<QPair<QString, int>> encodings = {
        {tr("ANSI"), 0}, {tr("UTF-8"), 1}, {tr("UTF-8 with BOM"), 4},
        {tr("UTF-16 Big Endian with BOM"), 2},
        {tr("UTF-16 Little Endian with BOM"), 3}
    };
    for (int i = 0; i < encodings.size(); ++i) {
        _encodingCombo->addItem(encodings.at(i).first, encodings.at(i).second);
        QRadioButton* radio = placePreferenceControl(
            new QRadioButton(encodings.at(i).first, encGroup),
            10, 10 + i * 18 + (i > 1 ? 14 : 0), 155, 14);
        connect(radio, &QRadioButton::toggled, this, [this, i](bool checked) {
            if (checked) _encodingCombo->setCurrentIndex(i);
        });
        connect(_encodingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                radio, [radio, i](int index) { radio->setChecked(index == i); });
    }
    _encodingCombo->hide();
    _openAnsiAsUtf8CB = placePreferenceControl(
        new QCheckBox(tr("Apply to opened ANSI files"), encGroup), 20, 41, 145, 14);
    _openAnsiAsUtf8CB->setObjectName("chkOpenAnsiAsUtf8");
    QLabel* languageLabel = placePreferenceControl(
        new QLabel(tr("Default language:"), w), 16, 130, 77, 14);
    languageLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QComboBox* defaultLanguage = placePreferenceControl(
        new QComboBox(w), 98, 128, 100, 17);
    defaultLanguage->addItems({tr("Normal Text"), tr("C++"), tr("HTML"), tr("Python")});
    return w;
}

// ─────────────────────────────────────────────────────────────────────────────
// 7. Recent Files History
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_RecentFilesHistory()
{
    QWidget* w = preferenceCanvas();
    QGroupBox* grp = placePreferenceControl(
        new QGroupBox(tr("Recent Files History"), w), 104, 25, 260, 126);
    grp->setObjectName("grpRecentFiles");
    _checkHistoryFilesCB = placePreferenceControl(
        new QCheckBox(tr("Don't check at launch time"), grp), 17, 12, 180, 13);
    _checkHistoryFilesCB->setObjectName("chkDontCheckHistory");
    QLabel* lblMaxRecentFiles = placePreferenceControl(
        new QLabel(tr("Max. number of entries:"), grp), 8, 29, 112, 14);
    lblMaxRecentFiles->setObjectName("lblMaxRecentFiles");
    lblMaxRecentFiles->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    _maxRecentFilesSB = placePreferenceControl(new QSpinBox(grp), 124, 27, 45, 17);
    _maxRecentFilesSB->setRange(0, 30);
    QGroupBox* display = placePreferenceControl(
        new QGroupBox(tr("Display"), grp), 17, 40, 225, 73);
    _recentInSubMenuCB = placePreferenceControl(
        new QCheckBox(tr("In Submenu"), display), 11, 10, 100, 13);
    _recentInSubMenuCB->setObjectName("chkRecentInSubMenu");
    placePreferenceControl(new QRadioButton(tr("Only File Name"), display),
                           11, 27, 170, 13);
    QRadioButton* full = placePreferenceControl(
        new QRadioButton(tr("Full File Name Path"), display), 11, 42, 170, 13);
    full->setChecked(true);
    placePreferenceControl(new QRadioButton(tr("Customize Maximum Length:"), display),
                           11, 57, 190, 13);
    return w;
}

// ─────────────────────────────────────────────────────────────────────────────
// 9. Language
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_Language()
{
    QWidget* w = new QWidget();
    QHBoxLayout* lay = new QHBoxLayout(w);
    lay->setContentsMargins(10, 18, 10, 12);
    lay->setSpacing(8);

    QGroupBox* menuGroup = new QGroupBox(tr("Language Menu"), w);
    menuGroup->setObjectName("grpLanguageMenu");
    QGridLayout* menuLayout = new QGridLayout(menuGroup);
    QListWidget* available = new QListWidget(menuGroup);
    QListWidget* excluded = new QListWidget(menuGroup);
    available->addItems({"Normal Text", "PHP", "C", "C++", "C#", "Java",
                         "HTML", "XML", "JavaScript", "Python", "JSON"});
    QPushButton* exclude = new QPushButton(">", menuGroup);
    QPushButton* include = new QPushButton("<", menuGroup);
    exclude->setFixedWidth(28);
    include->setFixedWidth(28);
    QLabel* availableLabel = new QLabel(tr("Available items:"), menuGroup);
    availableLabel->setObjectName("lblAvailableLanguages");
    QLabel* disabledLabel = new QLabel(tr("Disabled items:"), menuGroup);
    disabledLabel->setObjectName("lblDisabledLanguages");
    menuLayout->addWidget(availableLabel, 0, 0);
    menuLayout->addWidget(disabledLabel, 0, 2);
    menuLayout->addWidget(available, 1, 0, 3, 1);
    menuLayout->addWidget(exclude, 1, 1);
    menuLayout->addWidget(include, 2, 1);
    menuLayout->addWidget(excluded, 1, 2, 3, 1);
    connect(exclude, &QPushButton::clicked, this, [available, excluded]() {
        if (QListWidgetItem* item = available->takeItem(available->currentRow()))
            excluded->addItem(item);
    });
    connect(include, &QPushButton::clicked, this, [available, excluded]() {
        if (QListWidgetItem* item = excluded->takeItem(excluded->currentRow()))
            available->addItem(item);
    });
    lay->addWidget(menuGroup, 3);

    QGroupBox* tabGroup = new QGroupBox(tr("Tab Settings"), w);
    tabGroup->setObjectName("grpLanguageTabSettings");
    QVBoxLayout* tabLayout = new QVBoxLayout(tabGroup);
    QListWidget* languages = new QListWidget(tabGroup);
    languages->addItems({"Default", "Normal", "PHP", "C", "C++", "Java",
                         "HTML", "XML", "JavaScript", "Python"});
    tabLayout->addWidget(languages);
    QHBoxLayout* tabSize = new QHBoxLayout();
    QLabel* tabSizeLabel = new QLabel(tr("Tab size:"), tabGroup);
    tabSizeLabel->setObjectName("lblLanguageTabSize");
    tabSize->addWidget(tabSizeLabel);
    QSpinBox* size = new QSpinBox(tabGroup);
    size->setRange(1, 16);
    size->setValue(4);
    tabSize->addWidget(size);
    tabLayout->addLayout(tabSize);
    QCheckBox* replaceBySpace = new QCheckBox(tr("Replace by space"), tabGroup);
    replaceBySpace->setObjectName("chkLanguageReplaceBySpace");
    tabLayout->addWidget(replaceBySpace);
    lay->addWidget(tabGroup, 2);
    return wrapScroll(w);
}

// ─────────────────────────────────────────────────────────────────────────────
// 13. Backup
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_Backup()
{
    QWidget* w = preferenceCanvas();
    QGroupBox* snapGroup = placePreferenceControl(
        new QGroupBox(tr("Session snapshot and periodic backup"), w),
        79, 1, 289, 75);
    snapGroup->setObjectName("grpSnapshot");
    _restoreSessionCB = placePreferenceControl(
        new QCheckBox(tr("Remember current session for next launch"), snapGroup),
        11, 8, 270, 13);
    _restoreSessionCB->setObjectName("chkRestoreSession");
    _snapshotModeCB = placePreferenceControl(
        new QCheckBox(tr("Enable session snapshot and periodic backup"), snapGroup),
        11, 24, 270, 13);
    _snapshotModeCB->setObjectName("chkSnapshotMode");
    QLabel* every = placePreferenceControl(
        new QLabel(tr("Backup in every"), snapGroup), 3, 41, 78, 13);
    every->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    _snapshotTimingSB = placePreferenceControl(
        new QSpinBox(snapGroup), 85, 38, 48, 17);
    _snapshotTimingSB->setObjectName("spinSnapshotTiming");
    _snapshotTimingSB->setRange(1, 600);
    _snapshotTimingLabel = placePreferenceControl(
        new QLabel(tr("seconds"), snapGroup), 137, 41, 66, 13);
    QLabel* pathLabel = placePreferenceControl(
        new QLabel(tr("Backup path:"), snapGroup), 6, 58, 61, 13);
    pathLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QLineEdit* snapshotPath = placePreferenceControl(
        new QLineEdit(snapGroup), 74, 55, 208, 17);
    snapshotPath->setReadOnly(true);
    snapshotPath->setText(QDir::toNativeSeparators(
        NppParameters::getInstance().backupDirPath()));
    snapshotPath->setCursorPosition(0);

    QGroupBox* modeGroup = placePreferenceControl(
        new QGroupBox(tr("Backup on save"), w), 79, 81, 289, 101);
    modeGroup->setObjectName("grpBackupSave");
    _backupNoneRB = placePreferenceControl(
        new QRadioButton(tr("None"), modeGroup), 25, 8, 120, 13);
    _backupNoneRB->setObjectName("rbBackupNone");
    _backupSimpleRB = placePreferenceControl(
        new QRadioButton(tr("Simple backup"), modeGroup), 25, 23, 150, 13);
    _backupSimpleRB->setObjectName("rbBackupSimple");
    _backupVerboseRB = placePreferenceControl(
        new QRadioButton(tr("Verbose backup"), modeGroup), 25, 38, 150, 13);
    _backupVerboseRB->setObjectName("rbBackupVerbose");
    QGroupBox* custom = placePreferenceControl(
        new QGroupBox(tr("Custom Backup Directory"), modeGroup), 16, 52, 260, 40);
    _backupCustomDirCB = placePreferenceControl(
        new QCheckBox(custom), -4, -1, 14, 14);
    _backupCustomDirCB->setObjectName("chkBackupCustomDir");
    QLabel* directory = placePreferenceControl(
        new QLabel(tr("Directory:"), custom), 4, 15, 50, 13);
    directory->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    _backupDirEdit = placePreferenceControl(
        new QLineEdit(custom), 56, 12, 179, 17);
    QPushButton* browseBackupDir = placePreferenceControl(
        new QPushButton(QStringLiteral("..."), custom), 238, 11, 18, 18);
    browseBackupDir->setObjectName("btnBackupDirBrowse");
    connect(_backupCustomDirCB, &QCheckBox::toggled,
            _backupDirEdit, &QWidget::setEnabled);
    connect(_backupCustomDirCB, &QCheckBox::toggled,
            browseBackupDir, &QWidget::setEnabled);
    connect(browseBackupDir, &QPushButton::clicked, this, [this]() {
        const QString directory = QFileDialog::getExistingDirectory(
            this, tr("Choose Backup Directory"), _backupDirEdit->text());
        if (!directory.isEmpty())
            _backupDirEdit->setText(QDir::toNativeSeparators(directory));
    });
    connect(_snapshotModeCB, &QCheckBox::toggled,
            this, &PreferenceDlg::onSnapshotModeToggled);
    return w;
}

// ─────────────────────────────────────────────────────────────────────────────
// 14. Auto-Completion
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_AutoCompletion()
{
    QWidget* w = preferenceCanvas();
    QCheckBox* autoIndent = placePreferenceControl(
        new QCheckBox(tr("Auto-indent"), w), 347, 15, 100, 13);
    connect(autoIndent, &QCheckBox::toggled, _autoIndentCB, &QCheckBox::setChecked);
    connect(_autoIndentCB, &QCheckBox::toggled, autoIndent, &QCheckBox::setChecked);
    QGroupBox* acGroup = placePreferenceControl(
        new QGroupBox(tr("Auto-Completion"), w), 33, 4, 289, 90);
    acGroup->setObjectName("grpAutoCompletion");
    _acEnableCB = placePreferenceControl(
        new QCheckBox(tr("Enable auto-completion on each input"), acGroup),
        5, 9, 150, 13);
    _acEnableCB->setObjectName("chkAcEnable");
    _acNoneRB = new QRadioButton(acGroup);
    _acNoneRB->hide();
    _acNoneRB->setObjectName("rbAcNone");
    _acFunctionRB = placePreferenceControl(
        new QRadioButton(tr("Function completion"), acGroup), 27, 22, 145, 13);
    _acFunctionRB->setObjectName("rbAcFunction");
    _acWordRB = placePreferenceControl(
        new QRadioButton(tr("Word completion"), acGroup), 27, 37, 145, 13);
    _acWordRB->setObjectName("rbAcWord");
    _acBothRB = placePreferenceControl(
        new QRadioButton(tr("Function and word completion"), acGroup), 27, 52, 145, 13);
    _acBothRB->setObjectName("rbAcBoth");
    _acFromNbCharLabel = placePreferenceControl(
        new QLabel(tr("From"), acGroup), 160, 7, 42, 13);
    _acFromNbCharLabel->setObjectName("lblAcFromNbChar");
    _acFromNbCharSB = placePreferenceControl(new QSpinBox(acGroup), 205, 4, 42, 17);
    _acFromNbCharSB->setObjectName("spinAcFromNbChar");
    _acFromNbCharSB->setRange(1, 9);
    placePreferenceControl(new QLabel(tr("th character"), acGroup), 250, 7, 55, 13);
    _acIgnoreNumbersCB = placePreferenceControl(
        new QCheckBox(tr("Ignore numbers"), acGroup), 181, 67, 100, 13);
    _acIgnoreNumbersCB->setObjectName("chkAcIgnoreNumbers");

    QGroupBox* insertSelection = placePreferenceControl(
        new QGroupBox(tr("Insert Selection"), acGroup), 180, 31, 96, 38);
    placePreferenceControl(new QCheckBox(tr("TAB"), insertSelection), 12, 10, 54, 13);
    placePreferenceControl(new QCheckBox(tr("ENTER"), insertSelection), 12, 23, 55, 13);
    _funcParamsCB = placePreferenceControl(
        new QCheckBox(tr("Function parameters hint on input"), acGroup),
        5, 67, 160, 13);
    _funcParamsCB->setObjectName("chkFuncParams");
    QGroupBox* pairGroup = placePreferenceControl(
        new QGroupBox(tr("Auto-Insert"), w), 33, 99, 289, 84);
    pairGroup->setObjectName("grpAutoInsert");
    _pairParenthesesCB = placePreferenceControl(
        new QCheckBox(QStringLiteral("("), pairGroup), 15, 17, 35, 13);
    _pairParenthesesCB->setObjectName("chkPairParentheses");
    _pairBracketsCB = placePreferenceControl(
        new QCheckBox(QStringLiteral("["), pairGroup), 15, 35, 35, 13);
    _pairBracketsCB->setObjectName("chkPairBrackets");
    _pairCurlyCB = placePreferenceControl(
        new QCheckBox(QStringLiteral("{"), pairGroup), 15, 54, 35, 13);
    _pairCurlyCB->setObjectName("chkPairCurly");
    _pairQuotesCB = placePreferenceControl(
        new QCheckBox(QStringLiteral("'"), pairGroup), 59, 35, 35, 13);
    _pairQuotesCB->setObjectName("chkPairQuotes");
    _pairDoubleQuotesCB = placePreferenceControl(
        new QCheckBox(QStringLiteral("\""), pairGroup), 59, 17, 35, 13);
    _pairDoubleQuotesCB->setObjectName("chkPairDoubleQuotes");
    _pairTagsCB = placePreferenceControl(
        new QCheckBox(tr("html/xml close tag"), pairGroup), 59, 54, 100, 13);
    _pairTagsCB->setObjectName("chkPairTags");
    placePreferenceControl(new QLabel(tr("Open"), pairGroup), 220, 7, 30, 13);
    placePreferenceControl(new QLabel(tr("Close"), pairGroup), 258, 7, 30, 13);
    for (int i = 0; i < 3; ++i) {
        placePreferenceControl(new QLabel(tr("Matched pair %1:").arg(i + 1), pairGroup),
                               160, 20 + i * 20, 70, 13);
        placePreferenceControl(new QLineEdit(pairGroup), 232, 18 + i * 20, 20, 17);
        placePreferenceControl(new QLineEdit(pairGroup), 258, 18 + i * 20, 20, 17);
    }

    connect(_acEnableCB, &QCheckBox::toggled,
            this, &PreferenceDlg::onAcEnableToggled);

    return w;
}

// ─────────────────────────────────────────────────────────────────────────────
// 19. MISC.
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_Misc()
{
    QWidget* w = preferenceCanvas();
    QGroupBox* detGroup = placePreferenceControl(
        new QGroupBox(tr("File Status Auto-Detection"), w), 36, 4, 155, 60);
    detGroup->setObjectName("grpFileAutoDetect");
    _fileAutoDetectCombo = placePreferenceControl(
        new QComboBox(detGroup), 8, 10, 140, 17);
    _fileAutoDetectCombo->setObjectName("comboFileAutoDetect");
    _fileAutoDetectCombo->addItem(tr("Disable"),                 0);
    _fileAutoDetectCombo->addItem(tr("Enable"),                  1);
    _fileAutoDetectCombo->addItem(tr("Enable for all opened files"), 2);
    placePreferenceControl(new QCheckBox(tr("Update silently"), detGroup), 8, 28, 140, 13);
    placePreferenceControl(new QCheckBox(tr("Scroll to the last line after update"), detGroup),
                           8, 43, 145, 13);

    QGroupBox* switcher = placePreferenceControl(
        new QGroupBox(tr("Document Switcher (Ctrl+TAB)"), w), 261, 4, 155, 39);
    placePreferenceControl(new QCheckBox(tr("Enable"), switcher), 8, 9, 140, 13);
    placePreferenceControl(new QCheckBox(tr("Enable MRU behaviour"), switcher), 8, 24, 140, 13);
    QGroupBox* peeker = placePreferenceControl(
        new QGroupBox(tr("Document Peeker"), w), 261, 47, 155, 39);
    placePreferenceControl(new QCheckBox(tr("Peek on tab"), peeker), 8, 9, 140, 13);
    placePreferenceControl(new QCheckBox(tr("Peek on document map"), peeker), 8, 24, 140, 13);

    const QStringList options = {
        tr("Enable Notepad++ auto-updater"), tr("Mute all sounds"),
        tr("Autodetect character encoding"), tr("Minimize to system tray"),
        tr("Show only filename in title bar"),
        tr("Use DirectWrite (need to restart Notepad++)"),
        tr("Enable Save All confirm dialog")
    };
    for (int i = 0; i < options.size(); ++i) {
        QCheckBox* option = placePreferenceControl(
            new QCheckBox(options.at(i), w), 37, 94 + i * 15, 380, 13);
        if (i == 2) {
            _detectEncodingCB = option;
            _detectEncodingCB->setObjectName("chkDetectEncoding");
        }
    }
    placePreferenceControl(new QLabel(tr("Session file ext.:"), w), 270, 130, 108, 13);
    placePreferenceControl(new QLineEdit(QStringLiteral("session"), w), 380, 127, 50, 17);
    placePreferenceControl(new QLabel(tr("Workspace file ext.:"), w), 270, 147, 108, 13);
    placePreferenceControl(new QLineEdit(QStringLiteral("workspace"), w), 380, 144, 50, 17);
    return w;
}

// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
// 3. Dark Mode
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_DarkMode()
{
    QWidget* w = preferenceCanvas();
    _darkModeEnableCB = placePreferenceControl(
        new QCheckBox(tr("Enable dark mode"), w), 15, 12, 150, 13);
    _darkModeEnableCB->setObjectName("chkDarkModeEnable");
    QGroupBox* tones = placePreferenceControl(
        new QGroupBox(tr("Tones"), w), 10, 25, 440, 154);
    tones->setObjectName("grpDarkModeTones");
    const QStringList names = {
        tr("Black"), tr("Red"), tr("Green"), tr("Blue"),
        tr("Purple"), tr("Cyan"), tr("Olive")
    };
    for (int i = 0; i < names.size(); ++i) {
        QRadioButton* tone = placePreferenceControl(
            new QRadioButton(names.at(i), tones), 5, 8 + i * 15, 80, 13);
        tone->setObjectName(QStringLiteral("rbDarkTone%1").arg(i));
        if (i == 0) tone->setChecked(true);
    }
    placePreferenceControl(new QRadioButton(tr("Customized"), tones),
                           90, 8, 120, 13);
    const QStringList swatches = {
        tr("Top"), tr("Menu hot track"), tr("Active"), tr("Main"), tr("Error"),
        tr("Text"), tr("Darker text"), tr("Disabled text"), tr("Link"),
        tr("Edge"), tr("Edge highlight"), tr("Edge disabled")
    };
    for (int i = 0; i < swatches.size(); ++i) {
        const int column = i < 5 ? 0 : i < 9 ? 1 : 2;
        const int row = column == 0 ? i : column == 1 ? i - 5 : i - 9;
        const int x = 115 + column * 115;
        placePreferenceControl(new QLabel(swatches.at(i), tones),
                               x, 27 + row * 20, 92, 13);
        QPushButton* colour = placePreferenceControl(
            new QPushButton(tones), x - 18, 26 + row * 20, 14, 14);
        colour->setObjectName(QStringLiteral("btnDarkColour%1").arg(i));
    }
    placePreferenceControl(new QPushButton(tr("Reset"), tones), 242, 126, 45, 16);
    return w;
}

// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_DefaultDirectory()
{
    QWidget* w = preferenceCanvas();
    QGroupBox* group = placePreferenceControl(
        new QGroupBox(tr("Default Open/Save file Directory"), w),
        110, 29, 232, 101);
    group->setObjectName("grpDefaultDirectory");
    _defaultDirModeCombo = new QComboBox(group);
    _defaultDirModeCombo->setObjectName("comboDefaultDirMode");
    _defaultDirModeCombo->addItem(tr("Follow current document"), 0);
    _defaultDirModeCombo->addItem(tr("Remember last used directory"), 1);
    _defaultDirModeCombo->addItem(tr("Use custom directory"), 2);
    _defaultDirModeCombo->hide();
    QRadioButton* follow = placePreferenceControl(
        new QRadioButton(tr("Follow current document"), group), 8, 24, 200, 13);
    QRadioButton* remember = placePreferenceControl(
        new QRadioButton(tr("Remember last used directory"), group), 8, 40, 217, 13);
    QRadioButton* custom = placePreferenceControl(
        new QRadioButton(group), 8, 56, 12, 13);
    _defaultDirEdit = placePreferenceControl(
        new QLineEdit(group), 24, 54, 179, 17);
    QPushButton* browse = placePreferenceControl(
        new QPushButton(QStringLiteral("..."), group), 208, 53, 18, 18);
    browse->setObjectName("btnDefaultDirBrowse");
    const QList<QRadioButton*> modes = {follow, remember, custom};
    for (int i = 0; i < modes.size(); ++i) {
        connect(modes.at(i), &QRadioButton::toggled, this,
                [this, i](bool checked) {
            if (checked) _defaultDirModeCombo->setCurrentIndex(i);
        });
        connect(_defaultDirModeCombo,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                modes.at(i), [radio = modes.at(i), i](int index) {
            radio->setChecked(index == i);
        });
    }
    connect(browse, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getExistingDirectory(
            this, tr("Default Directory"), _defaultDirEdit->text());
        if (!path.isEmpty())
            _defaultDirEdit->setText(path);
    });
    connect(_defaultDirModeCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) {
        _defaultDirEdit->setEnabled(_defaultDirModeCombo->currentData().toInt() == 2);
    });
    placePreferenceControl(new QCheckBox(
        tr("Open all files of folder instead of launching Folder as Workspace on folder dropping"), w),
        110, 145, 342, 13);
    return w;
}

QWidget* PreferenceDlg::makePage_FileAssociation()
{
    QWidget* w = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(w);
    QGroupBox* group = new QGroupBox(tr("System File Associations"), w);
    group->setObjectName("grpFileAssociations");
    QVBoxLayout* groupLayout = new QVBoxLayout(group);

#ifdef Q_OS_WIN
    QLabel* supportedLabel = new QLabel(tr("Supported extensions:"), group);
    supportedLabel->setObjectName("lblSupportedExtensions");
    QLabel* registeredLabel = new QLabel(tr("Registered extensions:"), group);
    registeredLabel->setObjectName("lblRegisteredExtensions");
    QGridLayout* associationLayout = new QGridLayout();
    associationLayout->addWidget(supportedLabel, 0, 0, 1, 2);
    associationLayout->addWidget(registeredLabel, 0, 3);

    QListWidget* categories = new QListWidget(group);
    categories->setObjectName("fileAssociationCategories");
    QListWidget* available = new QListWidget(group);
    available->setObjectName("fileAssociationAvailable");
    QListWidget* registered = new QListWidget(group);
    registered->setObjectName("fileAssociationRegistered");
    QLineEdit* customExtension = new QLineEdit(group);
    customExtension->setObjectName("fileAssociationCustom");
    customExtension->setMaxLength(18);
    customExtension->setPlaceholderText(tr(".ext"));
    customExtension->hide();

    QWidget* availableContainer = new QWidget(group);
    QVBoxLayout* availableLayout = new QVBoxLayout(availableContainer);
    availableLayout->setContentsMargins(0, 0, 0, 0);
    availableLayout->addWidget(available);
    availableLayout->addWidget(customExtension);

    QToolButton* addButton = new QToolButton(group);
    addButton->setObjectName("btnAddFileAssociation");
    addButton->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
    addButton->setToolTip(tr("Register selected extension"));
    QToolButton* removeButton = new QToolButton(group);
    removeButton->setObjectName("btnRemoveFileAssociation");
    removeButton->setIcon(style()->standardIcon(QStyle::SP_ArrowLeft));
    removeButton->setToolTip(tr("Remove selected extension"));
    QVBoxLayout* buttonLayout = new QVBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);
    buttonLayout->addStretch();

    associationLayout->addWidget(categories, 1, 0);
    associationLayout->addWidget(availableContainer, 1, 1);
    associationLayout->addLayout(buttonLayout, 1, 2);
    associationLayout->addWidget(registered, 1, 3);
    associationLayout->setColumnStretch(0, 1);
    associationLayout->setColumnStretch(1, 1);
    associationLayout->setColumnStretch(3, 1);
    groupLayout->addLayout(associationLayout);

    for (const FileAssociationCategory& category : fileAssociationCategories())
        categories->addItem(category.name);
    registered->addItems(PlatformServices::registeredFileAssociations());

    auto refreshAvailable = [categories, available, registered, customExtension]() {
        available->clear();
        const int row = categories->currentRow();
        if (row < 0 || row >= fileAssociationCategories().size())
            return;
        const bool custom = row == fileAssociationCategories().size() - 1;
        available->setVisible(!custom);
        customExtension->setVisible(custom);
        if (custom)
            return;
        QStringList registeredExtensions;
        for (int i = 0; i < registered->count(); ++i)
            registeredExtensions.append(registered->item(i)->text());
        for (const QString& extension :
             fileAssociationCategories().at(row).extensions) {
            if (!registeredExtensions.contains(extension, Qt::CaseInsensitive))
                available->addItem(extension);
        }
    };
    auto updateButtons = [categories, available, registered, customExtension,
                          addButton, removeButton]() {
        const bool custom =
            categories->currentRow() == fileAssociationCategories().size() - 1;
        const QString extension = custom
            ? normalizeFileAssociationExtension(customExtension->text())
            : (available->currentItem() ? available->currentItem()->text()
                                        : QString());
        addButton->setEnabled(!extension.isEmpty());
        removeButton->setEnabled(registered->currentItem() != nullptr);
    };
    auto addAssociation = [this, categories, available, registered,
                           customExtension, refreshAvailable, updateButtons]() {
        const bool custom =
            categories->currentRow() == fileAssociationCategories().size() - 1;
        const QString extension = normalizeFileAssociationExtension(custom
            ? customExtension->text()
            : (available->currentItem() ? available->currentItem()->text()
                                        : QString()));
        if (extension.isEmpty())
            return;
        QString error;
        if (!PlatformServices::registerFileAssociation(
                extension, QCoreApplication::applicationFilePath(), &error)) {
            QMessageBox::warning(this, tr("File Association"), error);
            return;
        }
        if (registered->findItems(extension, Qt::MatchFixedString).isEmpty())
            registered->addItem(extension);
        registered->sortItems(Qt::AscendingOrder);
        customExtension->clear();
        refreshAvailable();
        updateButtons();
    };
    auto removeAssociation = [this, registered, refreshAvailable, updateButtons]() {
        QListWidgetItem* item = registered->currentItem();
        if (!item)
            return;
        QString error;
        if (!PlatformServices::unregisterFileAssociation(item->text(), &error)) {
            QMessageBox::warning(this, tr("File Association"), error);
            return;
        }
        delete registered->takeItem(registered->row(item));
        refreshAvailable();
        updateButtons();
    };

    connect(categories, &QListWidget::currentRowChanged, this,
            [refreshAvailable, updateButtons](int) {
        refreshAvailable();
        updateButtons();
    });
    connect(available, &QListWidget::itemSelectionChanged, this, updateButtons);
    connect(registered, &QListWidget::itemSelectionChanged, this, updateButtons);
    connect(customExtension, &QLineEdit::textChanged, this,
            [customExtension, updateButtons](const QString& text) {
        if (text.size() == 1 && text.at(0) != '.')
            customExtension->setText(QStringLiteral(".") + text);
        updateButtons();
    });
    connect(addButton, &QToolButton::clicked, this, addAssociation);
    connect(removeButton, &QToolButton::clicked, this, removeAssociation);
    connect(available, &QListWidget::itemDoubleClicked, this,
            [addAssociation](QListWidgetItem*) { addAssociation(); });
    connect(registered, &QListWidget::itemDoubleClicked, this,
            [removeAssociation](QListWidgetItem*) { removeAssociation(); });

    categories->setCurrentRow(0);
    const bool canManage = PlatformServices::canManageFileAssociations();
    categories->setEnabled(canManage);
    available->setEnabled(canManage);
    registered->setEnabled(canManage);
    customExtension->setEnabled(canManage);
    if (!canManage) {
        addButton->setEnabled(false);
        removeButton->setEnabled(false);
        QLabel* adminMessage = new QLabel(
            tr("Administrator privileges are required. Restart as administrator "
               "to change file associations."), group);
        adminMessage->setObjectName("lblFileAssociationAdmin");
        adminMessage->setWordWrap(true);
        groupLayout->addWidget(adminMessage);
    }
#else
    QLabel* platformMessage = new QLabel(
        tr("File associations are managed by the operating system on this platform."),
        group);
    platformMessage->setObjectName("lblFileAssociationPlatform");
    platformMessage->setWordWrap(true);
    groupLayout->addWidget(platformMessage);
#endif

    QPushButton* settings =
        new QPushButton(tr("Open system default application settings"), group);
    settings->setObjectName("btnOpenDefaultApps");
    groupLayout->addWidget(settings);
    connect(settings, &QPushButton::clicked, this,
            []() { PlatformServices::openDefaultApplicationsSettings(); });
    layout->addWidget(group);
    layout->addStretch();
    return w;
}

QWidget* PreferenceDlg::makePage_Highlighting()
{
    QWidget* w = preferenceCanvas();
    QGroupBox* occurrences = placePreferenceControl(
        new QGroupBox(tr("Style All Occurrences of Token"), w), 62, 29, 155, 45);
    placePreferenceControl(new QCheckBox(tr("Match case"), occurrences),
                           8, 12, 142, 13);
    placePreferenceControl(new QCheckBox(tr("Match whole word only"), occurrences),
                           8, 27, 142, 13);

    QGroupBox* group = placePreferenceControl(
        new QGroupBox(tr("Smart Highlighting"), w), 226, 29, 172, 108);
    group->setObjectName("grpSmartHighlighting");
    _smartHighlightCB = placePreferenceControl(
        new QCheckBox(tr("Enable"), group), 8, 10, 142, 13);
    _smartHighlightCB->setObjectName("chkSmartHighlight");
    _smartAnotherViewCB = placePreferenceControl(
        new QCheckBox(tr("Highlight another view"), group), 8, 26, 142, 13);
    _smartAnotherViewCB->setObjectName("chkSmartAnotherView");
    QGroupBox* matching = placePreferenceControl(
        new QGroupBox(tr("Matching"), group), 7, 43, 155, 55);
    _smartMatchCaseCB = placePreferenceControl(
        new QCheckBox(tr("Match case"), matching), 8, 10, 142, 13);
    _smartMatchCaseCB->setObjectName("chkSmartMatchCase");
    _smartWholeWordCB = placePreferenceControl(
        new QCheckBox(tr("Match whole word only"), matching), 8, 25, 142, 13);
    _smartWholeWordCB->setObjectName("chkSmartWholeWord");
    _smartUseFindSettingsCB = placePreferenceControl(
        new QCheckBox(tr("Use Find dialog settings"), matching), 8, 40, 142, 13);
    _smartUseFindSettingsCB->setObjectName("chkSmartUseFindSettings");

    QGroupBox* tagGroup = placePreferenceControl(
        new QGroupBox(tr("Highlight Matching Tags"), w), 62, 82, 155, 55);
    tagGroup->setObjectName("grpTagMatching");
    _tagMatchCB = placePreferenceControl(
        new QCheckBox(tr("Enable"), tagGroup), 8, 10, 140, 13);
    _tagMatchCB->setObjectName("chkTagMatch");
    _tagAttributesCB = placePreferenceControl(
        new QCheckBox(tr("Highlight tag attributes"), tagGroup), 8, 25, 140, 13);
    _tagAttributesCB->setObjectName("chkTagAttributes");
    _tagNonHtmlCB = placePreferenceControl(
        new QCheckBox(tr("Highlight comment/php/asp zone"), tagGroup),
        8, 40, 140, 13);
    _tagNonHtmlCB->setObjectName("chkTagNonHtml");
    return w;
}

QWidget* PreferenceDlg::makePage_Print()
{
    QWidget* w = preferenceCanvas();
    _printLineNumberCB = placePreferenceControl(
        new QCheckBox(tr("Print line number"), w), 6, 6, 133, 13);
    _printLineNumberCB->setObjectName("chkPrintLineNumbers");
    QGroupBox* optionsGroup = placePreferenceControl(
        new QGroupBox(tr("Colour Options"), w), 6, 20, 133, 73);
    optionsGroup->setObjectName("grpPrintOptions");
    _printOptionCombo = new QComboBox(optionsGroup);
    _printOptionCombo->setObjectName("comboPrintColorMode");
    _printOptionCombo->addItem(tr("WYSIWYG"), 0);
    _printOptionCombo->addItem(tr("Invert colors"), 1);
    _printOptionCombo->addItem(tr("Black on white"), 2);
    _printOptionCombo->addItem(tr("Color on white"), 3);
    _printOptionCombo->hide();
    const QStringList colourModes = {
        tr("WYSIWYG"), tr("Invert"), tr("Black on white"),
        tr("No background colour")
    };
    for (int i = 0; i < colourModes.size(); ++i) {
        QRadioButton* radio = placePreferenceControl(
            new QRadioButton(colourModes.at(i), optionsGroup),
            6, 8 + i * 15, 123, 13);
        connect(radio, &QRadioButton::toggled, this, [this, i](bool checked) {
            if (checked) _printOptionCombo->setCurrentIndex(i);
        });
        connect(_printOptionCombo,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                radio, [radio, i](int index) { radio->setChecked(index == i); });
    }
    QGroupBox* margins = placePreferenceControl(
        new QGroupBox(tr("Margin Setting (Unit:mm)"), w), 6, 98, 133, 82);
    const QStringList marginLabels = {tr("Left"), tr("Top"), tr("Right"), tr("Bottom")};
    const QList<QPoint> labelPoints = {{8, 36}, {37, 15}, {72, 36}, {34, 57}};
    const QList<QPoint> valuePoints = {{30, 33}, {66, 12}, {100, 33}, {66, 54}};
    for (int i = 0; i < marginLabels.size(); ++i) {
        const QPoint labelPoint = labelPoints.at(i);
        placePreferenceControl(new QLabel(marginLabels.at(i), margins),
                               labelPoint.x(), labelPoint.y(), 34, 13);
        const QPoint valuePoint = valuePoints.at(i);
        QSpinBox* value = placePreferenceControl(
            new QSpinBox(margins), valuePoint.x(), valuePoint.y(), 32, 17);
        value->setRange(0, 99);
    }

    QGroupBox* headerFooter = placePreferenceControl(
        new QGroupBox(tr("Header and Footer"), w), 150, 7, 296, 172);
    headerFooter->setObjectName("grpPrintHeaderFooter");
    placePreferenceControl(new QLabel(tr("Variable:"), headerFooter), 48, 10, 58, 13);
    QComboBox* variable = placePreferenceControl(new QComboBox(headerFooter), 108, 8, 94, 17);
    variable->addItems({tr("Full file name path"), tr("File name"), tr("Date"), tr("Time")});
    placePreferenceControl(new QPushButton(tr("Add"), headerFooter), 210, 8, 44, 17);
    QGroupBox* header = placePreferenceControl(
        new QGroupBox(tr("Header"), headerFooter), 8, 30, 279, 56);
    QGroupBox* footer = placePreferenceControl(
        new QGroupBox(tr("Footer"), headerFooter), 8, 88, 279, 58);
    const QStringList parts = {tr("Left part"), tr("Middle part"), tr("Right part")};
    QLineEdit** headerEdits[] = {&_headerLeftEdit, &_headerMiddleEdit, &_headerRightEdit};
    QLineEdit** footerEdits[] = {&_footerLeftEdit, &_footerMiddleEdit, &_footerRightEdit};
    for (int i = 0; i < 3; ++i) {
        placePreferenceControl(new QLabel(parts.at(i), header), 10 + i * 90, 8, 80, 13);
        *headerEdits[i] = placePreferenceControl(new QLineEdit(header), 8 + i * 90, 21, 83, 17);
        placePreferenceControl(new QLabel(parts.at(i), footer), 10 + i * 90, 8, 80, 13);
        *footerEdits[i] = placePreferenceControl(new QLineEdit(footer), 8 + i * 90, 21, 83, 17);
    }
    return w;
}

QWidget* PreferenceDlg::makePage_Searching()
{
    QWidget* w = preferenceCanvas();
    QGroupBox* group = placePreferenceControl(
        new QGroupBox(tr("When Find Dialog is Invoked"), w), 31, 4, 323, 43);
    group->setObjectName("grpFindInvocation");
    _fillFindSelectedCB = placePreferenceControl(
        new QCheckBox(tr("Fill Find Field with Selected Text"), group),
        6, 10, 275, 13);
    _fillFindSelectedCB->setObjectName("chkFillFindSelected");
    _fillFindCaretCB = placePreferenceControl(
        new QCheckBox(tr("Select Word Under Caret when Nothing Selected"), group),
        21, 25, 275, 13);
    _fillFindCaretCB->setObjectName("chkFillFindCaret");
    const QList<QPair<QCheckBox**, QString>> rows = {
        {&_findMonospacedCB, tr("Use Monospaced font in Find dialog (Need to restart Notepad++)")},
        {&_findAlwaysVisibleCB, tr("Find dialog remains open after search that outputs to results window")},
        {&_confirmReplaceOpenedCB, tr("Confirm Replace All in All Opened Documents")},
        {&_replaceStopsCB, tr("Replace: Don't move to the following occurrence")},
        {&_showOneEntryCB, tr("Search Result window: show only one entry per found line")}
    };
    for (int i = 0; i < rows.size(); ++i)
        *rows.at(i).first = placePreferenceControl(
            new QCheckBox(rows.at(i).second, w), 37, 52 + i * 15, 380, 13);
    return w;
}

QWidget* PreferenceDlg::makePage_MultiInstance()
{
    QWidget* w = preferenceCanvas();
    QGroupBox* instanceGroup = placePreferenceControl(
        new QGroupBox(tr("Multi-instance settings"), w), 89, 3, 268, 92);
    instanceGroup->setObjectName("grpMultiInstance");
    _multiInstanceCombo = new QComboBox(instanceGroup);
    _multiInstanceCombo->setObjectName("comboMultiInstance");
    _multiInstanceCombo->addItem(tr("Open in existing instance"), 0);
    _multiInstanceCombo->addItem(tr("New instance for each session"), 1);
    _multiInstanceCombo->addItem(tr("Always open a new instance"), 2);
    _multiInstanceCombo->hide();
    const QStringList instanceModes = {
        tr("Default (mono-instance)"),
        tr("Open session in a new instance (and save session automatically on exit)"),
        tr("Always in multi-instance mode")
    };
    const QList<int> ys = {52, 9, 35};
    for (int i = 0; i < instanceModes.size(); ++i) {
        QRadioButton* radio = placePreferenceControl(
            new QRadioButton(instanceModes.at(i), instanceGroup),
            18, ys.at(i), 235, i == 1 ? 24 : 13);
        connect(radio, &QRadioButton::toggled, this, [this, i](bool checked) {
            if (checked) _multiInstanceCombo->setCurrentIndex(i);
        });
        connect(_multiInstanceCombo,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                radio, [radio, i](int index) { radio->setChecked(index == i); });
    }
    placePreferenceControl(new QLabel(
        tr("* The modification of this setting needs to restart Notepad++"),
        instanceGroup), 10, 70, 239, 20);

    QGroupBox* dateGroup = placePreferenceControl(
        new QGroupBox(tr("Customize insert Date Time"), w), 90, 100, 268, 82);
    dateGroup->setObjectName("grpDateTime");
    _dateReverseCB = placePreferenceControl(
        new QCheckBox(tr("Reverse default date time order (short && long formats)"), dateGroup),
        13, 10, 241, 13);
    _dateReverseCB->setObjectName("chkDateReverse");
    placePreferenceControl(new QLabel(
        QStringLiteral("yyyy-MM-dd HH:mm:ss\nH:m d/M/yyyy\nMMM d, yyyy  tt h:m"),
        dateGroup), 44, 27, 95, 40);
    placePreferenceControl(new QLabel(
        QStringLiteral("1985-10-26 16:24:42\n16:24 26/10/1985\nOct 26, 1985 PM 4:24"),
        dateGroup), 144, 27, 110, 40);
    QLabel* dateFormatLabel = placePreferenceControl(
        new QLabel(tr("Custom format:"), dateGroup), 2, 66, 77, 13);
    dateFormatLabel->setObjectName("lblDateTimeFormat");
    _dateTimeFormatEdit = placePreferenceControl(
        new QLineEdit(dateGroup), 80, 63, 182, 17);
    return w;
}

QWidget* PreferenceDlg::makePage_Delimiter()
{
    QWidget* w = preferenceCanvas();
    QGroupBox* word = placePreferenceControl(
        new QGroupBox(tr("Word character list"), w), 89, 2, 268, 93);
    QRadioButton* defaults = placePreferenceControl(
        new QRadioButton(tr("Use default Word character list as it is"), word),
        11, 12, 250, 13);
    defaults->setChecked(true);
    QRadioButton* customWordCharacters = placePreferenceControl(new QRadioButton(
        tr("Add your character as part of word\n(don't choose it unless you know what you're doing)"), word),
        11, 27, 250, 25);
    customWordCharacters->setObjectName("rbCustomWordCharacters");
    placePreferenceControl(new QLineEdit(word), 22, 52, 180, 17);
    placePreferenceControl(new QPushButton(QStringLiteral("?"), word), 214, 51, 18, 18);

    QGroupBox* group = placePreferenceControl(
        new QGroupBox(tr("Delimiter selection settings (Ctrl + Mouse double click)"), w),
        89, 113, 268, 70);
    group->setObjectName("grpDelimiterSelection");
    _leftDelimiterSB = placePreferenceControl(new QSpinBox(group), 67, 13, 34, 17);
    _rightDelimiterSB = placePreferenceControl(new QSpinBox(group), 148, 13, 34, 17);
    _leftDelimiterSB->setRange(0, 0xFFFF);
    _rightDelimiterSB->setRange(0, 0xFFFF);
    placePreferenceControl(new QLabel(tr("Open"), group), 28, 16, 34, 13);
    placePreferenceControl(new QLabel(tr("bla bla bla bla"), group), 104, 16, 62, 13);
    _rightDelimiterSB->setGeometry(prefGeometry(170, 13, 34, 17));
    placePreferenceControl(new QLabel(tr("Close"), group), 207, 16, 47, 13);
    _delimiterWholeDocumentCB = placePreferenceControl(
        new QCheckBox(tr("Allow on several lines"), group), 29, 50, 160, 13);
    _delimiterWholeDocumentCB->setObjectName("chkDelimiterWholeDocument");
    return w;
}

QWidget* PreferenceDlg::makePage_CloudLink()
{
    QWidget* w = preferenceCanvas();
    QGroupBox* cloud = placePreferenceControl(
        new QGroupBox(tr("Settings on cloud"), w), 89, 7, 268, 88);
    QRadioButton* noCloud = placePreferenceControl(
        new QRadioButton(tr("No Cloud"), cloud), 36, 10, 180, 13);
    noCloud->setChecked(true);
    placePreferenceControl(new QRadioButton(tr("Set your cloud location path here:"), cloud),
                           36, 25, 180, 13);
    placePreferenceControl(new QLineEdit(cloud), 45, 41, 179, 17);
    placePreferenceControl(new QPushButton(QStringLiteral("..."), cloud), 229, 40, 18, 18);

    QGroupBox* linkGroup = placePreferenceControl(
        new QGroupBox(tr("Clickable Link Settings"), w), 89, 100, 268, 90);
    linkGroup->setObjectName("grpClickableLinks");
    _urlModeCombo = new QComboBox(linkGroup);
    _urlModeCombo->setObjectName("comboUrlMode");
    _urlModeCombo->addItem(tr("Disabled"), 0);
    _urlModeCombo->addItem(tr("Enabled without underline"), 1);
    _urlModeCombo->addItem(tr("Enabled with underline"), 2);
    _urlModeCombo->hide();
    QCheckBox* enable = placePreferenceControl(
        new QCheckBox(tr("Enable"), linkGroup), 34, 10, 83, 13);
    QCheckBox* noUnderline = placePreferenceControl(
        new QCheckBox(tr("No underline"), linkGroup), 120, 10, 140, 13);
    placePreferenceControl(new QCheckBox(tr("Enable fullbox mode"), linkGroup),
                           120, 25, 140, 13);
    connect(enable, &QCheckBox::toggled, this, [this](bool checked) {
        _urlModeCombo->setCurrentIndex(checked ? 2 : 0);
    });
    connect(noUnderline, &QCheckBox::toggled, this, [this](bool checked) {
        if (_urlModeCombo->currentData().toInt() != 0)
            _urlModeCombo->setCurrentIndex(checked ? 1 : 2);
    });
    connect(_urlModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this, enable, noUnderline](int) {
        const int mode = _urlModeCombo->currentData().toInt();
        enable->setChecked(mode != 0);
        noUnderline->setChecked(mode == 1);
    });
    placePreferenceControl(new QLabel(tr("URI customized schemes:"), linkGroup),
                           18, 40, 120, 13);
    _uriSchemesEdit = placePreferenceControl(new QLineEdit(linkGroup), 17, 53, 238, 24);
    return w;
}

QWidget* PreferenceDlg::makePage_SearchEngine()
{
    QWidget* w = preferenceCanvas();
    QGroupBox* group = placePreferenceControl(
        new QGroupBox(tr("Search Engine (for command \"Search on Internet\")"), w),
        74, 24, 297, 145);
    group->setObjectName("grpSearchEngine");
    _searchEngineCombo = new QComboBox(group);
    _searchEngineCombo->setObjectName("comboSearchEngine");
    _searchEngineCombo->addItem(tr("Custom"), 0);
    _searchEngineCombo->addItem("DuckDuckGo", 1);
    _searchEngineCombo->addItem("Google", 2);
    _searchEngineCombo->addItem("Yahoo!", 4);
    _searchEngineCombo->addItem("Stack Overflow", 5);
    _searchEngineCombo->hide();
    const QList<QPair<QString, int>> engines = {
        {QStringLiteral("DuckDuckGo"), 1}, {QStringLiteral("Google"), 2},
        {QStringLiteral("Yahoo!"), 4}, {QStringLiteral("Stack Overflow"), 5},
        {tr("Set your search engine here:"), 0}
    };
    for (int i = 0; i < engines.size(); ++i) {
        QRadioButton* radio = placePreferenceControl(
            new QRadioButton(engines.at(i).first, group), 31, 14 + i * 15, 210, 13);
        connect(radio, &QRadioButton::toggled, this,
                [this, value = engines.at(i).second](bool checked) {
            if (checked) {
                const int index = _searchEngineCombo->findData(value);
                _searchEngineCombo->setCurrentIndex(index);
            }
        });
        connect(_searchEngineCombo,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                radio, [this, radio, value = engines.at(i).second](int) {
            radio->setChecked(_searchEngineCombo->currentData().toInt() == value);
        });
    }
    _searchEngineCustomEdit = placePreferenceControl(
        new QLineEdit(group), 40, 92, 179, 17);
    placePreferenceControl(new QLabel(
        tr("Example: https://www.google.com/search?q=$(CURRENT_WORD)"), group),
        40, 109, 245, 26);
    connect(_searchEngineCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) {
        _searchEngineCustomEdit->setEnabled(
            _searchEngineCombo->currentData().toInt() == 0);
    });
    return w;
}

// ─────────────────────────────────────────────────────────────────────────────
// loadSettings
// ─────────────────────────────────────────────────────────────────────────────
void PreferenceDlg::loadSettings()
{
    const NppParameters&       params = NppParameters::getInstance();
    const NppGUI&              gui    = params.getNppGUI();
    const ScintillaViewParams& svp    = params.getSVP();

    // ── General ──────────────────────────────────────────────────────────────
    _hideToolbarCB->setChecked(!gui._toolBarShow);
    _showStatusBarCB->setChecked(gui._statusBarShow);
    _showMenuBarCB->setChecked(gui._menuBarShow);

    _tabHideCB->setChecked(gui._tabHide);
    _tabDragDropCB->setChecked(gui._tabDragAndDrop);
    _tabTopBarCB->setChecked(gui._tabDrawTopBar);
    _tabInactiveTabCB->setChecked(gui._tabDrawInactiveTab);
    _tabCloseBtnCB->setChecked(gui._tabCloseButton);
    _tabDblClickCloseCB->setChecked(gui._tabDbclkToClose);
    _tabVerticalCB->setChecked(gui._tabVertical);
    _tabMultiLineCB->setChecked(gui._tabMultiLine);
    _tabQuitOnEmptyCB->setChecked(gui._tabQuitOnEmpty);
    if (QCheckBox* lock = findChild<QCheckBox*>("chkTabLock"))
        lock->setChecked(!gui._tabDragAndDrop);
    const QList<QRadioButton*> toolbarModes = findChildren<QRadioButton*>();
    for (QRadioButton* mode : toolbarModes) {
        if (mode->property("toolbarIconSet").isValid())
            mode->setChecked(mode->property("toolbarIconSet").toInt()
                             == gui._tabIconSetNumber);
    }
    onTabHideToggled(gui._tabHide);

    _fontCombo->setCurrentFont(QFont(gui._editorFontName));
    _fontSizeSB->setValue(gui._editorFontSize);

    // ── Dark Mode ───────────────────────────────────────────────────────────
    _darkModeEnableCB->setChecked(gui._darkModeEnabled);

    // ── Editing ──────────────────────────────────────────────────────────────
    _autoIndentCB->setChecked(gui._autoIndent);
    _scrollBeyondLastLineCB->setChecked(svp._scrollBeyondLastLine);
    _showWhitespaceCB->setChecked(svp._whiteSpaceShow);
    _showEolCB->setChecked(svp._eolShow);
    _wordWrapCB->setChecked(svp._doWrap);
    switch (svp._lineWrapMethod) {
        case 1:  _lwAlignRB->setChecked(true);  break;
        case 2:  _lwIndentRB->setChecked(true); break;
        default: _lwDefaultRB->setChecked(true);
    }

    // ── Margins/Border/Edge ───────────────────────────────────────────────────
    _lineNumberMarginCB->setChecked(svp._lineNumberMarginShow);
    _bookMarkMarginCB->setChecked(svp._bookMarkMarginShow);
    _foldMarginCB->setChecked(svp._foldMarginShow);
    _indentGuideLineCB->setChecked(svp._indentGuideLineShow);
    _currentLineHighlightCB->setChecked(svp._currentLineHilitingShow);
    _wrapSymbolShowCB->setChecked(svp._wrapSymbolShow);
    _edgeShowCB->setChecked(svp._edgeShow);
    _edgeColumnSB->setValue(svp._edgeNbColumn);
    _edgeColumnSB->setEnabled(svp._edgeShow);

    // ── New Document ─────────────────────────────────────────────────────────
    switch (gui._newDocDefaultFormat) {
        case 0:  _fmtWindowsRB->setChecked(true); break;
        case 1:  _fmtMacRB->setChecked(true);     break;
        default: _fmtUnixRB->setChecked(true);
    }
    {
        int idx = _encodingCombo->findData(gui._newDocDefaultEncoding);
        _encodingCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    }
    _openAnsiAsUtf8CB->setChecked(gui._openAnsiAsUtf8);

    _defaultDirModeCombo->setCurrentIndex(qMax(
        0, _defaultDirModeCombo->findData(gui._openSaveDir)));
    _defaultDirEdit->setText(gui._defaultDirPath);
    _defaultDirEdit->setEnabled(gui._openSaveDir == 2);

    _smartHighlightCB->setChecked(gui._smartHighlight);
    _smartMatchCaseCB->setChecked(gui._smartHighlightMatchCase);
    _smartWholeWordCB->setChecked(gui._smartHighlightWholeWord);
    _smartUseFindSettingsCB->setChecked(gui._smartHighlightUseFindSettings);
    _smartAnotherViewCB->setChecked(gui._smartHighlightAnotherView);
    _tagMatchCB->setChecked(gui._enableTagsMatchHighlight);
    _tagAttributesCB->setChecked(gui._enableTagAttrsHighlight);
    _tagNonHtmlCB->setChecked(gui._highlightNonHtmlZone);

    _printLineNumberCB->setChecked(gui._printLineNumber);
    _printOptionCombo->setCurrentIndex(qMax(
        0, _printOptionCombo->findData(gui._printOption)));
    _headerLeftEdit->setText(gui._printHeaderLeft);
    _headerMiddleEdit->setText(gui._printHeaderMiddle);
    _headerRightEdit->setText(gui._printHeaderRight);
    _footerLeftEdit->setText(gui._printFooterLeft);
    _footerMiddleEdit->setText(gui._printFooterMiddle);
    _footerRightEdit->setText(gui._printFooterRight);

    _fillFindSelectedCB->setChecked(gui._fillFindFieldWithSelected);
    _fillFindCaretCB->setChecked(gui._fillFindFieldSelectCaret);
    _findMonospacedCB->setChecked(gui._monospacedFontFindDlg);
    _findAlwaysVisibleCB->setChecked(gui._findDlgAlwaysVisible);
    _confirmReplaceOpenedCB->setChecked(gui._confirmReplaceInAllOpenDocs);
    _replaceStopsCB->setChecked(gui._replaceStopsWithoutFindingNext);
    _showOneEntryCB->setChecked(gui._showOnlyOneEntryPerFoundLine);

    _multiInstanceCombo->setCurrentIndex(qMax(
        0, _multiInstanceCombo->findData(gui._multiInstSetting)));
    _dateTimeFormatEdit->setText(gui._dateTimeFormat);
    _dateReverseCB->setChecked(gui._dateTimeReverseDefaultOrder);
    _leftDelimiterSB->setValue(gui._leftmostDelimiter);
    _rightDelimiterSB->setValue(gui._rightmostDelimiter);
    _delimiterWholeDocumentCB->setChecked(
        gui._delimiterSelectionOnEntireDocument);
    _urlModeCombo->setCurrentIndex(qMax(
        0, _urlModeCombo->findData(gui._urlMode)));
    _uriSchemesEdit->setText(gui._uriCustomizedSchemes);
    const int searchEngineChoice = gui._searchEngineChoice == 3
        ? 1
        : gui._searchEngineChoice;
    int searchEngineIndex = _searchEngineCombo->findData(searchEngineChoice);
    if (searchEngineIndex < 0)
        searchEngineIndex = _searchEngineCombo->findData(2);
    _searchEngineCombo->setCurrentIndex(searchEngineIndex);
    _searchEngineCustomEdit->setText(gui._searchEngineCustom);
    _searchEngineCustomEdit->setEnabled(searchEngineChoice == 0);

    // ── Recent Files History ─────────────────────────────────────────────────
    _maxRecentFilesSB->setValue(gui._nbMaxRecentFile);
    _recentInSubMenuCB->setChecked(gui._putRecentFileInSubMenu);

    // ── Language ─────────────────────────────────────────────────────────────
    {
        QString lang = params.getNativeLang();
        int idx = _languageCombo->findData(lang);
        _languageCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    }

    // ── Backup ───────────────────────────────────────────────────────────────
    switch (gui._backup) {
        case 1:  _backupSimpleRB->setChecked(true);  break;
        case 2:  _backupVerboseRB->setChecked(true); break;
        default: _backupNoneRB->setChecked(true);
    }
    _backupCustomDirCB->setChecked(gui._useBackupDir);
    _backupDirEdit->setText(QDir::toNativeSeparators(gui._backupDir));
    _backupDirEdit->setEnabled(gui._useBackupDir);
    _snapshotModeCB->setChecked(gui._isSnapshotMode);
    _snapshotTimingSB->setValue(gui._snapshotBackupTiming / 1000);
    _snapshotTimingSB->setEnabled(gui._isSnapshotMode);

    // ── Auto-Completion ──────────────────────────────────────────────────────
    bool acEnabled = (gui._autocAction != 0) || gui._autoCompleteEnable;
    _acEnableCB->setChecked(acEnabled);
    switch (gui._autocAction) {
        case 1: _acFunctionRB->setChecked(true); break;
        case 2: _acWordRB->setChecked(true);     break;
        case 3: _acBothRB->setChecked(true);     break;
        default: _acNoneRB->setChecked(true);
    }
    _acFromNbCharSB->setValue(qMax(gui._autocFromNbChar, gui._autoCompleteThreshold));
    _acIgnoreNumbersCB->setChecked(gui._autocIgnoreNumbers);
    _pairParenthesesCB->setChecked(gui._autoInsertParentheses);
    _pairBracketsCB->setChecked(gui._autoInsertBrackets);
    _pairCurlyCB->setChecked(gui._autoInsertCurlyBrackets);
    _pairQuotesCB->setChecked(gui._autoInsertQuotes);
    _pairDoubleQuotesCB->setChecked(gui._autoInsertDoubleQuotes);
    _pairTagsCB->setChecked(gui._autoInsertHtmlXmlTag);
    _funcParamsCB->setChecked(gui._funcParams);
    onAcEnableToggled(acEnabled);

    // ── MISC. ────────────────────────────────────────────────────────────────
    {
        int v = gui._fileAutoDetection > 1 ? 2 : gui._fileAutoDetection;
        int idx = _fileAutoDetectCombo->findData(v);
        _fileAutoDetectCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    }
    _checkHistoryFilesCB->setChecked(gui._checkHistoryFiles);
    _restoreSessionCB->setChecked(gui._rememberLastSession);
    _detectEncodingCB->setChecked(gui._detectEncoding);
}

// ─────────────────────────────────────────────────────────────────────────────
// saveSettings
// ─────────────────────────────────────────────────────────────────────────────
void PreferenceDlg::saveSettings()
{
    NppParameters&       params = NppParameters::getInstance();
    NppGUI&              gui    = params.getNppGUI();
    ScintillaViewParams& svp    = params.getSVP();

    // ── General ──────────────────────────────────────────────────────────────
    gui._toolBarShow      = !_hideToolbarCB->isChecked();
    gui._statusBarShow    =  _showStatusBarCB->isChecked();
    gui._menuBarShow      =  _showMenuBarCB->isChecked();

    gui._tabHide            = _tabHideCB->isChecked();
    gui._tabDragAndDrop     = _tabDragDropCB->isChecked();
    gui._tabDrawTopBar      = _tabTopBarCB->isChecked();
    gui._tabDrawInactiveTab = _tabInactiveTabCB->isChecked();
    gui._tabCloseButton     = _tabCloseBtnCB->isChecked();
    gui._tabDbclkToClose    = _tabDblClickCloseCB->isChecked();
    gui._tabVertical        = _tabVerticalCB->isChecked();
    gui._tabMultiLine       = _tabMultiLineCB->isChecked();
    gui._tabQuitOnEmpty     = _tabQuitOnEmptyCB->isChecked();
    const QList<QRadioButton*> toolbarModes = findChildren<QRadioButton*>();
    for (QRadioButton* mode : toolbarModes) {
        if (mode->property("toolbarIconSet").isValid() && mode->isChecked())
            gui._tabIconSetNumber = mode->property("toolbarIconSet").toInt();
    }

    gui._editorFontName = _fontCombo->currentFont().family();
    gui._editorFontSize = _fontSizeSB->value();
    gui._darkModeEnabled = _darkModeEnableCB->isChecked();

    // ── Editing ──────────────────────────────────────────────────────────────
    gui._autoIndent               = _autoIndentCB->isChecked();
    svp._scrollBeyondLastLine     = _scrollBeyondLastLineCB->isChecked();
    svp._whiteSpaceShow           = _showWhitespaceCB->isChecked();
    svp._eolShow                  = _showEolCB->isChecked();
    svp._doWrap                   = _wordWrapCB->isChecked();
    if      (_lwAlignRB->isChecked())  svp._lineWrapMethod = 1;
    else if (_lwIndentRB->isChecked()) svp._lineWrapMethod = 2;
    else                               svp._lineWrapMethod = 0;

    // ── Margins/Border/Edge ───────────────────────────────────────────────────
    svp._lineNumberMarginShow    = _lineNumberMarginCB->isChecked();
    svp._bookMarkMarginShow      = _bookMarkMarginCB->isChecked();
    svp._foldMarginShow          = _foldMarginCB->isChecked();
    svp._indentGuideLineShow     = _indentGuideLineCB->isChecked();
    svp._currentLineHilitingShow = _currentLineHighlightCB->isChecked();
    svp._wrapSymbolShow          = _wrapSymbolShowCB->isChecked();
    svp._edgeShow                = _edgeShowCB->isChecked();
    svp._edgeNbColumn            = _edgeColumnSB->value();

    // ── New Document ─────────────────────────────────────────────────────────
    if      (_fmtWindowsRB->isChecked()) gui._newDocDefaultFormat = 0;
    else if (_fmtMacRB->isChecked())     gui._newDocDefaultFormat = 1;
    else                                  gui._newDocDefaultFormat = 2;
    gui._newDocDefaultEncoding = _encodingCombo->currentData().toInt();
    gui._openAnsiAsUtf8 = _openAnsiAsUtf8CB->isChecked();

    gui._openSaveDir = _defaultDirModeCombo->currentData().toInt();
    gui._defaultDirPath = _defaultDirEdit->text();

    gui._smartHighlight = _smartHighlightCB->isChecked();
    gui._smartHighlightMatchCase = _smartMatchCaseCB->isChecked();
    gui._smartHighlightWholeWord = _smartWholeWordCB->isChecked();
    gui._smartHighlightUseFindSettings = _smartUseFindSettingsCB->isChecked();
    gui._smartHighlightAnotherView = _smartAnotherViewCB->isChecked();
    gui._enableTagsMatchHighlight = _tagMatchCB->isChecked();
    gui._enableTagAttrsHighlight = _tagAttributesCB->isChecked();
    gui._highlightNonHtmlZone = _tagNonHtmlCB->isChecked();

    gui._printLineNumber = _printLineNumberCB->isChecked();
    gui._printOption = _printOptionCombo->currentData().toInt();
    gui._printHeaderLeft = _headerLeftEdit->text();
    gui._printHeaderMiddle = _headerMiddleEdit->text();
    gui._printHeaderRight = _headerRightEdit->text();
    gui._printFooterLeft = _footerLeftEdit->text();
    gui._printFooterMiddle = _footerMiddleEdit->text();
    gui._printFooterRight = _footerRightEdit->text();

    gui._fillFindFieldWithSelected = _fillFindSelectedCB->isChecked();
    gui._fillFindFieldSelectCaret = _fillFindCaretCB->isChecked();
    gui._monospacedFontFindDlg = _findMonospacedCB->isChecked();
    gui._findDlgAlwaysVisible = _findAlwaysVisibleCB->isChecked();
    gui._confirmReplaceInAllOpenDocs = _confirmReplaceOpenedCB->isChecked();
    gui._replaceStopsWithoutFindingNext = _replaceStopsCB->isChecked();
    gui._showOnlyOneEntryPerFoundLine = _showOneEntryCB->isChecked();

    gui._multiInstSetting = _multiInstanceCombo->currentData().toInt();
    gui._dateTimeFormat = _dateTimeFormatEdit->text();
    gui._dateTimeReverseDefaultOrder = _dateReverseCB->isChecked();
    gui._leftmostDelimiter = _leftDelimiterSB->value();
    gui._rightmostDelimiter = _rightDelimiterSB->value();
    gui._delimiterSelectionOnEntireDocument =
        _delimiterWholeDocumentCB->isChecked();
    gui._urlMode = _urlModeCombo->currentData().toInt();
    gui._uriCustomizedSchemes = _uriSchemesEdit->text();
    gui._searchEngineChoice = _searchEngineCombo->currentData().toInt();
    gui._searchEngineCustom = _searchEngineCustomEdit->text();

    // ── Recent Files History ─────────────────────────────────────────────────
    gui._nbMaxRecentFile        = _maxRecentFilesSB->value();
    gui._putRecentFileInSubMenu = _recentInSubMenuCB->isChecked();

    // ── Language ─────────────────────────────────────────────────────────────
    QString newLang = _languageCombo->currentData().toString();
    if (newLang != params.getNativeLang()) {
        _langChanged = true;
        params.setNativeLang(newLang);
    }

    // ── Backup ───────────────────────────────────────────────────────────────
    if      (_backupSimpleRB->isChecked())  gui._backup = 1;
    else if (_backupVerboseRB->isChecked()) gui._backup = 2;
    else                                     gui._backup = 0;
    gui._useBackupDir = _backupCustomDirCB->isChecked();
    gui._backupDir = QDir::fromNativeSeparators(_backupDirEdit->text());
    gui._isSnapshotMode       = _snapshotModeCB->isChecked();
    gui._snapshotBackupTiming = _snapshotTimingSB->value() * 1000;

    // ── Auto-Completion ──────────────────────────────────────────────────────
    if (_acEnableCB->isChecked()) {
        if      (_acFunctionRB->isChecked()) gui._autocAction = 1;
        else if (_acWordRB->isChecked())     gui._autocAction = 2;
        else if (_acBothRB->isChecked())     gui._autocAction = 3;
        else                                  gui._autocAction = 1; // default to function
        gui._autoCompleteEnable = true;
    } else {
        gui._autocAction        = 0;
        gui._autoCompleteEnable = false;
    }
    gui._autocFromNbChar      = _acFromNbCharSB->value();
    gui._autoCompleteThreshold = _acFromNbCharSB->value();
    gui._autocIgnoreNumbers   = _acIgnoreNumbersCB->isChecked();
    gui._funcParams            = _funcParamsCB->isChecked();
    gui._autoInsertParentheses = _pairParenthesesCB->isChecked();
    gui._autoInsertBrackets = _pairBracketsCB->isChecked();
    gui._autoInsertCurlyBrackets = _pairCurlyCB->isChecked();
    gui._autoInsertQuotes = _pairQuotesCB->isChecked();
    gui._autoInsertDoubleQuotes = _pairDoubleQuotesCB->isChecked();
    gui._autoInsertHtmlXmlTag = _pairTagsCB->isChecked();

    // ── MISC. ────────────────────────────────────────────────────────────────
    gui._fileAutoDetection  = _fileAutoDetectCombo->currentData().toInt();
    gui._checkHistoryFiles  = _checkHistoryFilesCB->isChecked();
    gui._rememberLastSession = _restoreSessionCB->isChecked();
    gui._detectEncoding = _detectEncodingCB->isChecked();

    // 更新 Qt 扩展冗余副本
    gui._showWhitespace  = svp._whiteSpaceShow;
    gui._showEol         = svp._eolShow;
    gui._doWordWrap      = svp._doWrap;
    gui._restoreSession  = gui._rememberLastSession;

    params.writeNppGUI();
}

// ─────────────────────────────────────────────────────────────────────────────
// Slots
// ─────────────────────────────────────────────────────────────────────────────
void PreferenceDlg::onApply()
{
    _langChanged = false;
    saveSettings();
    // 对应原版：切换语言后立即调用 _nativeLangSpeaker.init() + changeMenuLang()
    // 不需要重启，运行时热切换
    if (_langChanged) {
        NppParameters::getInstance().reloadNativeLang();
    }
    emit settingsChanged();  // MainWindow 连接此信号，会调用 applyNativeLang()
}

void PreferenceDlg::onTabHideToggled(bool hidden)
{
    _tabDragDropCB->setEnabled(!hidden);
    _tabTopBarCB->setEnabled(!hidden);
    _tabInactiveTabCB->setEnabled(!hidden);
    _tabCloseBtnCB->setEnabled(!hidden);
    _tabDblClickCloseCB->setEnabled(!hidden);
    _tabVerticalCB->setEnabled(!hidden);
    _tabMultiLineCB->setEnabled(!hidden);
    _tabQuitOnEmptyCB->setEnabled(!hidden);
}

void PreferenceDlg::onSnapshotModeToggled(bool checked)
{
    _snapshotTimingSB->setEnabled(checked);
}

void PreferenceDlg::onAcEnableToggled(bool checked)
{
    _acNoneRB->setEnabled(checked);
    _acFunctionRB->setEnabled(checked);
    _acWordRB->setEnabled(checked);
    _acBothRB->setEnabled(checked);
    _acFromNbCharLabel->setEnabled(checked);
    _acFromNbCharSB->setEnabled(checked);
    _acIgnoreNumbersCB->setEnabled(checked);
}
