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
    setMinimumSize(760, 360);
    resize(830, 372);
    setupUi();
    loadSettings();
}

void PreferenceDlg::setupUi()
{
    // 左侧列表
    _pageList = new QListWidget(this);
    _pageList->setObjectName("pageList");
    _pageList->setFixedWidth(180);
    _pageList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 右侧堆叠
    _pageStack = new QStackedWidget(this);

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

    connect(_pageList, &QListWidget::currentRowChanged,
            _pageStack, &QStackedWidget::setCurrentIndex);
    _pageList->setCurrentRow(0);

    // 分隔线
    QFrame* sep = new QFrame();
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);

    QHBoxLayout* body = new QHBoxLayout();
    body->addWidget(_pageList);
    body->addWidget(sep);
    body->addWidget(_pageStack, 1);

    QDialogButtonBox* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Ok)->setObjectName("btnPrefsOk");
    btns->button(QDialogButtonBox::Apply)->setObjectName("btnPrefsApply");
    btns->button(QDialogButtonBox::Cancel)->setObjectName("btnPrefsCancel");
    connect(btns, &QDialogButtonBox::accepted, this, [this]{ onApply(); accept(); });
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(btns->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &PreferenceDlg::onApply);

    QVBoxLayout* main = new QVBoxLayout(this);
    main->addLayout(body);
    main->addWidget(btns);
}

// ─────────────────────────────────────────────────────────────────────────────
// 1. General
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_General()
{
    QWidget* w = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(w);

    // Toolbar
    QGroupBox* tbGroup = new QGroupBox(tr("Tool Bar"));
    tbGroup->setObjectName("grpToolBar");
    QVBoxLayout* tbLay = new QVBoxLayout(tbGroup);
    _hideToolbarCB = new QCheckBox(tr("Hide"));
    _hideToolbarCB->setObjectName("chkHideToolbar");
    tbLay->addWidget(_hideToolbarCB);
    lay->addWidget(tbGroup);

    // Tab Bar
    QGroupBox* tabGroup = new QGroupBox(tr("Tab Bar"));
    tabGroup->setObjectName("grpTabBar");
    QGridLayout* tabLay = new QGridLayout(tabGroup);
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

    tabLay->addWidget(_tabHideCB,          0, 0);
    tabLay->addWidget(_tabDragDropCB,      1, 0);
    tabLay->addWidget(_tabTopBarCB,        2, 0);
    tabLay->addWidget(_tabInactiveTabCB,   3, 0);
    tabLay->addWidget(_tabCloseBtnCB,      0, 1);
    tabLay->addWidget(_tabDblClickCloseCB, 1, 1);
    tabLay->addWidget(_tabVerticalCB,      2, 1);
    tabLay->addWidget(_tabMultiLineCB,     3, 1);
    tabLay->addWidget(_tabQuitOnEmptyCB,   4, 0, 1, 2);
    lay->addWidget(tabGroup);

    connect(_tabHideCB, &QCheckBox::toggled,
            this, &PreferenceDlg::onTabHideToggled);

    // Status / Menu bar
    QGroupBox* barGroup = new QGroupBox(tr("Status Bar / Menu Bar"));
    barGroup->setObjectName("grpBarGroup");
    QVBoxLayout* barLay = new QVBoxLayout(barGroup);
    _showStatusBarCB = new QCheckBox(tr("Show status bar"));
    _showStatusBarCB->setObjectName("chkShowStatusBar");
    _showMenuBarCB   = new QCheckBox(tr("Show menu bar"));
    _showMenuBarCB->setObjectName("chkShowMenuBar");
    barLay->addWidget(_showStatusBarCB);
    barLay->addWidget(_showMenuBarCB);
    lay->addWidget(barGroup);

    // Editor Font（原版在 Style Configurator，此处简化）
    QGroupBox* fontGroup = new QGroupBox(tr("Editor Font"));
    fontGroup->setObjectName("grpEditorFont");
    QHBoxLayout* fontLay = new QHBoxLayout(fontGroup);
    _fontCombo = new QFontComboBox();
    _fontCombo->setFontFilters(QFontComboBox::MonospacedFonts);
    _fontSizeSB = new QSpinBox();
    _fontSizeSB->setRange(6, 72);
    _fontSizeSB->setSuffix(" pt");
    QLabel* lblFontSize = new QLabel(tr("Size:"));
    lblFontSize->setObjectName("lblFontSize");
    fontLay->addWidget(_fontCombo, 1);
    fontLay->addWidget(lblFontSize);
    fontLay->addWidget(_fontSizeSB);
    lay->addWidget(fontGroup);

    lay->addStretch();
    return wrapScroll(w);
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. Editing
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_Editing()
{
    QWidget* w = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(w);

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
    lay->addWidget(editGroup);

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
    lay->addWidget(wrapGroup);

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
    lay->addWidget(dispGroup);

    lay->addStretch();
    return wrapScroll(w);
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. Margins/Border/Edge
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_MarginsBorderEdge()
{
    QWidget* w = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(w);

    // Margins
    QGroupBox* marginGroup = new QGroupBox(tr("Margins"));
    marginGroup->setObjectName("grpMargins");
    QVBoxLayout* marginLay = new QVBoxLayout(marginGroup);
    _lineNumberMarginCB     = new QCheckBox(tr("Show line number margin"));
    _lineNumberMarginCB->setObjectName("chkLineNumbers");
    _bookMarkMarginCB       = new QCheckBox(tr("Show bookmark margin"));
    _bookMarkMarginCB->setObjectName("chkBookmarkMargin");
    _foldMarginCB           = new QCheckBox(tr("Show fold margin"));
    _foldMarginCB->setObjectName("chkFoldMargin");
    _indentGuideLineCB      = new QCheckBox(tr("Show indent guide line"));
    _indentGuideLineCB->setObjectName("chkIndentGuideLine");
    _currentLineHighlightCB = new QCheckBox(tr("Highlight current line"));
    _currentLineHighlightCB->setObjectName("chkCurrentLineHighlight");
    _wrapSymbolShowCB       = new QCheckBox(tr("Show wrap symbol at line end"));
    _wrapSymbolShowCB->setObjectName("chkWrapSymbolShow");
    marginLay->addWidget(_lineNumberMarginCB);
    marginLay->addWidget(_bookMarkMarginCB);
    marginLay->addWidget(_foldMarginCB);
    marginLay->addWidget(_indentGuideLineCB);
    marginLay->addWidget(_currentLineHighlightCB);
    marginLay->addWidget(_wrapSymbolShowCB);
    lay->addWidget(marginGroup);

    // Vertical Edge Line
    QGroupBox* edgeGroup = new QGroupBox(tr("Vertical Edge Line"));
    edgeGroup->setObjectName("grpEdgeLine");
    QHBoxLayout* edgeLay = new QHBoxLayout(edgeGroup);
    _edgeShowCB     = new QCheckBox(tr("Show vertical edge at column:"));
    _edgeShowCB->setObjectName("chkEdgeShow");
    _edgeColumnLabel = new QLabel(tr("Column:"));
    _edgeColumnLabel->setObjectName("lblEdgeColumn");
    _edgeColumnSB   = new QSpinBox();
    _edgeColumnSB->setRange(1, 500);
    edgeLay->addWidget(_edgeShowCB);
    edgeLay->addWidget(_edgeColumnSB);
    edgeLay->addStretch();

    connect(_edgeShowCB, &QCheckBox::toggled, _edgeColumnSB, &QSpinBox::setEnabled);

    lay->addWidget(edgeGroup);
    lay->addStretch();
    return wrapScroll(w);
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. New Document
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_NewDocument()
{
    QWidget* w = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(w);

    // Format (line ending)
    QGroupBox* fmtGroup = new QGroupBox(tr("Format (line ending)"));
    fmtGroup->setObjectName("grpFormat");
    QVBoxLayout* fmtLay = new QVBoxLayout(fmtGroup);
    _fmtWindowsRB = new QRadioButton(tr("Windows (CR LF)"));
    _fmtWindowsRB->setObjectName("rbFmtWindows");
    _fmtUnixRB    = new QRadioButton(tr("Unix (LF)"));
    _fmtUnixRB->setObjectName("rbFmtUnix");
    _fmtMacRB     = new QRadioButton(tr("Macintosh (CR)"));
    _fmtMacRB->setObjectName("rbFmtMac");
    fmtLay->addWidget(_fmtWindowsRB);
    fmtLay->addWidget(_fmtUnixRB);
    fmtLay->addWidget(_fmtMacRB);
    lay->addWidget(fmtGroup);

    // Encoding
    QGroupBox* encGroup = new QGroupBox(tr("Encoding"));
    encGroup->setObjectName("grpEncoding");
    QHBoxLayout* encLay = new QHBoxLayout(encGroup);
    _encodingCombo = new QComboBox();
    _encodingCombo->setObjectName("comboDefaultEncoding");
    _encodingCombo->addItem(tr("ANSI"),              0);  // uni8Bit
    _encodingCombo->addItem(tr("UTF-8"),             1);  // uniUTF8
    _encodingCombo->addItem(tr("UTF-8 with BOM"),    4);  // uniCookie
    _encodingCombo->addItem(tr("UCS-2 Big Endian"),  2);  // uni16BE
    _encodingCombo->addItem(tr("UCS-2 Little Endian"), 3); // uni16LE
    QLabel* lblDefaultEncoding = new QLabel(tr("Default encoding:"));
    lblDefaultEncoding->setObjectName("lblDefaultEncoding");
    encLay->addWidget(lblDefaultEncoding);
    encLay->addWidget(_encodingCombo);
    encLay->addStretch();
    lay->addWidget(encGroup);

    lay->addStretch();
    return wrapScroll(w);
}

// ─────────────────────────────────────────────────────────────────────────────
// 7. Recent Files History
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_RecentFilesHistory()
{
    QWidget* w = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(w);

    QGroupBox* grp = new QGroupBox(tr("Recent Files History"));
    grp->setObjectName("grpRecentFiles");
    QGridLayout* g = new QGridLayout(grp);
    QLabel* lblMaxRecentFiles = new QLabel(tr("Max number of entries:"));
    lblMaxRecentFiles->setObjectName("lblMaxRecentFiles");
    g->addWidget(lblMaxRecentFiles, 0, 0);
    _maxRecentFilesSB = new QSpinBox();
    _maxRecentFilesSB->setRange(0, 30);
    g->addWidget(_maxRecentFilesSB, 0, 1);
    _recentInSubMenuCB = new QCheckBox(tr("In sub-menu"));
    _recentInSubMenuCB->setObjectName("chkRecentInSubMenu");
    g->addWidget(_recentInSubMenuCB, 1, 0, 1, 2);
    lay->addWidget(grp);

    lay->addStretch();
    return wrapScroll(w);
}

// ─────────────────────────────────────────────────────────────────────────────
// 9. Language
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_Language()
{
    QWidget* w = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(w);

    QGroupBox* grp = new QGroupBox(tr("Interface Language"));
    grp->setObjectName("grpInterfaceLang");
    QVBoxLayout* grpLay = new QVBoxLayout(grp);

    QHBoxLayout* row = new QHBoxLayout();
    QLabel* lblLanguage = new QLabel(tr("Language:"));
    lblLanguage->setObjectName("lblLanguage");
    row->addWidget(lblLanguage);
    _languageCombo = new QComboBox();
    _languageCombo->setObjectName("comboInterfaceLanguage");
    _languageCombo->addItem(tr("English"),            "en");
    _languageCombo->addItem(tr("Chinese Simplified"), "zh_CN");
    row->addWidget(_languageCombo);
    row->addStretch();

    grpLay->addLayout(row);
    lay->addWidget(grp);
    lay->addStretch();
    return wrapScroll(w);
}

// ─────────────────────────────────────────────────────────────────────────────
// 13. Backup
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_Backup()
{
    QWidget* w = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(w);

    // Backup mode
    QGroupBox* modeGroup = new QGroupBox(tr("Backup on save"));
    modeGroup->setObjectName("grpBackupSave");
    QVBoxLayout* modeLay = new QVBoxLayout(modeGroup);
    _backupNoneRB    = new QRadioButton(tr("None"));
    _backupNoneRB->setObjectName("rbBackupNone");
    _backupSimpleRB  = new QRadioButton(tr("Simple backup"));
    _backupSimpleRB->setObjectName("rbBackupSimple");
    _backupVerboseRB = new QRadioButton(tr("Verbose backup (suffix with timestamp)"));
    _backupVerboseRB->setObjectName("rbBackupVerbose");
    modeLay->addWidget(_backupNoneRB);
    modeLay->addWidget(_backupSimpleRB);
    modeLay->addWidget(_backupVerboseRB);
    _backupCustomDirCB = new QCheckBox(tr("Use custom backup directory"));
    _backupCustomDirCB->setObjectName("chkBackupCustomDir");
    _backupDirEdit = new QLineEdit();
    QPushButton* browseBackupDir = new QPushButton(tr("..."));
    browseBackupDir->setObjectName("btnBackupDirBrowse");
    browseBackupDir->setFixedWidth(32);
    QHBoxLayout* backupDirRow = new QHBoxLayout();
    backupDirRow->addWidget(_backupDirEdit, 1);
    backupDirRow->addWidget(browseBackupDir);
    modeLay->addWidget(_backupCustomDirCB);
    modeLay->addLayout(backupDirRow);
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
    lay->addWidget(modeGroup);

    // Snapshot
    QGroupBox* snapGroup = new QGroupBox(tr("Session Snapshot and Periodic Backup"));
    snapGroup->setObjectName("grpSnapshot");
    QGridLayout* snapLay = new QGridLayout(snapGroup);
    _snapshotModeCB    = new QCheckBox(tr("Enable session snapshot and periodic backup every"));
    _snapshotModeCB->setObjectName("chkSnapshotMode");
    _snapshotTimingSB  = new QSpinBox();
    _snapshotTimingSB->setObjectName("spinSnapshotTiming");
    _snapshotTimingSB->setRange(1, 600);
    _snapshotTimingSB->setSuffix(tr(" seconds"));
    _snapshotTimingLabel = new QLabel(tr("seconds"));

    snapLay->addWidget(_snapshotModeCB,   0, 0, 1, 2);
    QLabel* backupIntervalLabel = new QLabel(tr("Backup interval:"));
    backupIntervalLabel->setObjectName("lblBackupInterval");
    snapLay->addWidget(backupIntervalLabel, 1, 0);
    snapLay->addWidget(_snapshotTimingSB, 1, 1);

    connect(_snapshotModeCB, &QCheckBox::toggled,
            this, &PreferenceDlg::onSnapshotModeToggled);

    lay->addWidget(snapGroup);
    lay->addStretch();
    return wrapScroll(w);
}

// ─────────────────────────────────────────────────────────────────────────────
// 14. Auto-Completion
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_AutoCompletion()
{
    QWidget* w = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(w);

    // Enable
    QGroupBox* acGroup = new QGroupBox(tr("Auto-Completion"));
    acGroup->setObjectName("grpAutoCompletion");
    QGridLayout* acLay = new QGridLayout(acGroup);
    _acEnableCB    = new QCheckBox(tr("Enable auto-completion on each input"));
    _acEnableCB->setObjectName("chkAcEnable");
    _acNoneRB      = new QRadioButton(tr("None"));
    _acNoneRB->setObjectName("rbAcNone");
    _acFunctionRB  = new QRadioButton(tr("Function completion"));
    _acFunctionRB->setObjectName("rbAcFunction");
    _acWordRB      = new QRadioButton(tr("Word completion"));
    _acWordRB->setObjectName("rbAcWord");
    _acBothRB      = new QRadioButton(tr("Function and word completion"));
    _acBothRB->setObjectName("rbAcBoth");
    _acFromNbCharLabel = new QLabel(tr("Trigger from"));
    _acFromNbCharLabel->setObjectName("lblAcFromNbChar");
    _acFromNbCharSB = new QSpinBox();
    _acFromNbCharSB->setObjectName("spinAcFromNbChar");
    _acFromNbCharSB->setRange(1, 9);
    _acFromNbCharSB->setSuffix(tr(" characters"));
    _acIgnoreNumbersCB = new QCheckBox(tr("Ignore numbers"));
    _acIgnoreNumbersCB->setObjectName("chkAcIgnoreNumbers");

    acLay->addWidget(_acEnableCB,         0, 0, 1, 3);
    acLay->addWidget(_acNoneRB,           1, 0);
    acLay->addWidget(_acFunctionRB,       2, 0);
    acLay->addWidget(_acWordRB,           3, 0);
    acLay->addWidget(_acBothRB,           4, 0);
    acLay->addWidget(_acFromNbCharLabel,  5, 0);
    acLay->addWidget(_acFromNbCharSB,     5, 1);
    acLay->addWidget(_acIgnoreNumbersCB,  6, 0, 1, 2);
    lay->addWidget(acGroup);

    // Function parameters hint
    QGroupBox* fpGroup = new QGroupBox(tr("Function Parameters Hint"));
    fpGroup->setObjectName("grpFuncParams");
    QVBoxLayout* fpLay = new QVBoxLayout(fpGroup);
    _funcParamsCB = new QCheckBox(tr("Enable function parameters hint on input"));
    _funcParamsCB->setObjectName("chkFuncParams");
    fpLay->addWidget(_funcParamsCB);
    lay->addWidget(fpGroup);

    QGroupBox* pairGroup = new QGroupBox(tr("Auto-insert"));
    pairGroup->setObjectName("grpAutoInsert");
    QGridLayout* pairLayout = new QGridLayout(pairGroup);
    _pairParenthesesCB = new QCheckBox(tr("Parentheses ()"));
    _pairParenthesesCB->setObjectName("chkPairParentheses");
    _pairBracketsCB = new QCheckBox(tr("Brackets []"));
    _pairBracketsCB->setObjectName("chkPairBrackets");
    _pairCurlyCB = new QCheckBox(tr("Curly brackets {}"));
    _pairCurlyCB->setObjectName("chkPairCurly");
    _pairQuotesCB = new QCheckBox(tr("Single quotes ''"));
    _pairQuotesCB->setObjectName("chkPairQuotes");
    _pairDoubleQuotesCB = new QCheckBox(tr("Double quotes \"\""));
    _pairDoubleQuotesCB->setObjectName("chkPairDoubleQuotes");
    _pairTagsCB = new QCheckBox(tr("HTML/XML close tag"));
    _pairTagsCB->setObjectName("chkPairTags");
    pairLayout->addWidget(_pairParenthesesCB, 0, 0);
    pairLayout->addWidget(_pairBracketsCB, 0, 1);
    pairLayout->addWidget(_pairCurlyCB, 1, 0);
    pairLayout->addWidget(_pairQuotesCB, 1, 1);
    pairLayout->addWidget(_pairDoubleQuotesCB, 2, 0);
    pairLayout->addWidget(_pairTagsCB, 2, 1);
    lay->addWidget(pairGroup);

    connect(_acEnableCB, &QCheckBox::toggled,
            this, &PreferenceDlg::onAcEnableToggled);

    lay->addStretch();
    return wrapScroll(w);
}

// ─────────────────────────────────────────────────────────────────────────────
// 19. MISC.
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_Misc()
{
    QWidget* w = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(w);

    // File auto-detection
    QGroupBox* detGroup = new QGroupBox(tr("File Status Auto-Detection"));
    detGroup->setObjectName("grpFileAutoDetect");
    QGridLayout* detLay = new QGridLayout(detGroup);
    _fileAutoDetectCombo = new QComboBox();
    _fileAutoDetectCombo->setObjectName("comboFileAutoDetect");
    _fileAutoDetectCombo->addItem(tr("Disable"),                 0);
    _fileAutoDetectCombo->addItem(tr("Enable"),                  1);
    _fileAutoDetectCombo->addItem(tr("Enable for all opened files"), 2);
    QLabel* lblFileAutoDetectStatus = new QLabel(tr("Status:"));
    lblFileAutoDetectStatus->setObjectName("lblFileAutoDetectStatus");
    detLay->addWidget(lblFileAutoDetectStatus, 0, 0);
    detLay->addWidget(_fileAutoDetectCombo, 0, 1);
    _checkHistoryFilesCB = new QCheckBox(tr("Update silently (don't prompt to reload)"));
    _checkHistoryFilesCB->setObjectName("chkCheckHistoryFiles");
    detLay->addWidget(_checkHistoryFilesCB, 1, 0, 1, 2);
    lay->addWidget(detGroup);

    // Session
    QGroupBox* sessGroup = new QGroupBox(tr("Session"));
    sessGroup->setObjectName("grpSession");
    QVBoxLayout* sessLay = new QVBoxLayout(sessGroup);
    _restoreSessionCB = new QCheckBox(tr("Remember current session for next launch"));
    _restoreSessionCB->setObjectName("chkRestoreSession");
    sessLay->addWidget(_restoreSessionCB);
    lay->addWidget(sessGroup);

    lay->addStretch();
    return wrapScroll(w);
}

// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
// 3. Dark Mode
// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_DarkMode()
{
    QWidget* w = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(w);

    QGroupBox* grp = new QGroupBox(tr("Dark Mode"));
    grp->setObjectName("grpDarkMode");
    QVBoxLayout* grpLay = new QVBoxLayout(grp);
    _darkModeEnableCB = new QCheckBox(tr("Enable dark mode"));
    _darkModeEnableCB->setObjectName("chkDarkModeEnable");
    grpLay->addWidget(_darkModeEnableCB);
    lay->addWidget(grp);
    lay->addStretch();
    return wrapScroll(w);
}

// ─────────────────────────────────────────────────────────────────────────────
QWidget* PreferenceDlg::makePage_DefaultDirectory()
{
    QWidget* w = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(w);
    QGroupBox* group = new QGroupBox(tr("Open/Save Directory"), w);
    group->setObjectName("grpDefaultDirectory");
    QFormLayout* form = new QFormLayout(group);
    _defaultDirModeCombo = new QComboBox(group);
    _defaultDirModeCombo->setObjectName("comboDefaultDirMode");
    _defaultDirModeCombo->addItem(tr("Follow current document"), 0);
    _defaultDirModeCombo->addItem(tr("Remember last used directory"), 1);
    _defaultDirModeCombo->addItem(tr("Use custom directory"), 2);
    QLabel* behaviorLabel = new QLabel(tr("Behavior:"), group);
    behaviorLabel->setObjectName("lblDefaultDirBehavior");
    form->addRow(behaviorLabel, _defaultDirModeCombo);
    QWidget* pathRow = new QWidget(group);
    QHBoxLayout* pathLayout = new QHBoxLayout(pathRow);
    pathLayout->setContentsMargins(0, 0, 0, 0);
    _defaultDirEdit = new QLineEdit(pathRow);
    QPushButton* browse = new QPushButton(tr("Browse..."), pathRow);
    browse->setObjectName("btnDefaultDirBrowse");
    pathLayout->addWidget(_defaultDirEdit, 1);
    pathLayout->addWidget(browse);
    QLabel* directoryLabel = new QLabel(tr("Directory:"), group);
    directoryLabel->setObjectName("lblDefaultDirectory");
    form->addRow(directoryLabel, pathRow);
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
    layout->addWidget(group);
    layout->addStretch();
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
    QWidget* w = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(w);
    QGroupBox* group = new QGroupBox(tr("Smart Highlighting"), w);
    group->setObjectName("grpSmartHighlighting");
    QVBoxLayout* options = new QVBoxLayout(group);
    _smartHighlightCB = new QCheckBox(tr("Enable"), group);
    _smartHighlightCB->setObjectName("chkSmartHighlight");
    _smartMatchCaseCB = new QCheckBox(tr("Match case"), group);
    _smartMatchCaseCB->setObjectName("chkSmartMatchCase");
    _smartWholeWordCB = new QCheckBox(tr("Whole word only"), group);
    _smartWholeWordCB->setObjectName("chkSmartWholeWord");
    _smartUseFindSettingsCB = new QCheckBox(tr("Use Find dialog settings"), group);
    _smartUseFindSettingsCB->setObjectName("chkSmartUseFindSettings");
    _smartAnotherViewCB = new QCheckBox(tr("Highlight in another view"), group);
    _smartAnotherViewCB->setObjectName("chkSmartAnotherView");
    options->addWidget(_smartHighlightCB);
    options->addWidget(_smartMatchCaseCB);
    options->addWidget(_smartWholeWordCB);
    options->addWidget(_smartUseFindSettingsCB);
    options->addWidget(_smartAnotherViewCB);
    layout->addWidget(group);
    QGroupBox* tagGroup = new QGroupBox(tr("HTML/XML Tag Matching"), w);
    tagGroup->setObjectName("grpTagMatching");
    QVBoxLayout* tagOptions = new QVBoxLayout(tagGroup);
    _tagMatchCB = new QCheckBox(tr("Highlight matching tags"), tagGroup);
    _tagMatchCB->setObjectName("chkTagMatch");
    _tagAttributesCB =
        new QCheckBox(tr("Highlight tag attributes"), tagGroup);
    _tagAttributesCB->setObjectName("chkTagAttributes");
    tagOptions->addWidget(_tagMatchCB);
    tagOptions->addWidget(_tagAttributesCB);
    layout->addWidget(tagGroup);
    layout->addStretch();
    return w;
}

QWidget* PreferenceDlg::makePage_Print()
{
    QWidget* w = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(w);
    QGroupBox* optionsGroup = new QGroupBox(tr("Print Options"), w);
    optionsGroup->setObjectName("grpPrintOptions");
    QFormLayout* options = new QFormLayout(optionsGroup);
    _printLineNumberCB = new QCheckBox(tr("Print line numbers"), optionsGroup);
    _printLineNumberCB->setObjectName("chkPrintLineNumbers");
    _printOptionCombo = new QComboBox(optionsGroup);
    _printOptionCombo->setObjectName("comboPrintColorMode");
    _printOptionCombo->addItem(tr("WYSIWYG"), 0);
    _printOptionCombo->addItem(tr("Invert colors"), 1);
    _printOptionCombo->addItem(tr("Black on white"), 2);
    _printOptionCombo->addItem(tr("Color on white"), 3);
    options->addRow(_printLineNumberCB);
    QLabel* colorModeLabel = new QLabel(tr("Color mode:"), optionsGroup);
    colorModeLabel->setObjectName("lblPrintColorMode");
    options->addRow(colorModeLabel, _printOptionCombo);
    layout->addWidget(optionsGroup);

    QGroupBox* headerFooter = new QGroupBox(tr("Header and Footer"), w);
    headerFooter->setObjectName("grpPrintHeaderFooter");
    QGridLayout* grid = new QGridLayout(headerFooter);
    _headerLeftEdit = new QLineEdit(headerFooter);
    _headerMiddleEdit = new QLineEdit(headerFooter);
    _headerRightEdit = new QLineEdit(headerFooter);
    _footerLeftEdit = new QLineEdit(headerFooter);
    _footerMiddleEdit = new QLineEdit(headerFooter);
    _footerRightEdit = new QLineEdit(headerFooter);
    QLabel* headerLabel = new QLabel(tr("Header:"), headerFooter);
    headerLabel->setObjectName("lblPrintHeader");
    grid->addWidget(headerLabel, 0, 0);
    grid->addWidget(_headerLeftEdit, 0, 1);
    grid->addWidget(_headerMiddleEdit, 0, 2);
    grid->addWidget(_headerRightEdit, 0, 3);
    QLabel* footerLabel = new QLabel(tr("Footer:"), headerFooter);
    footerLabel->setObjectName("lblPrintFooter");
    grid->addWidget(footerLabel, 1, 0);
    grid->addWidget(_footerLeftEdit, 1, 1);
    grid->addWidget(_footerMiddleEdit, 1, 2);
    grid->addWidget(_footerRightEdit, 1, 3);
    layout->addWidget(headerFooter);
    layout->addStretch();
    return wrapScroll(w);
}

QWidget* PreferenceDlg::makePage_Searching()
{
    QWidget* w = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(w);
    QGroupBox* group = new QGroupBox(tr("Default Search Options"), w);
    group->setObjectName("grpDefaultSearch");
    QVBoxLayout* options = new QVBoxLayout(group);
    _searchMatchWordCB = new QCheckBox(tr("Match whole word only"), group);
    _searchMatchWordCB->setObjectName("chkSearchWholeWord");
    _searchMatchCaseCB = new QCheckBox(tr("Match case"), group);
    _searchMatchCaseCB->setObjectName("chkSearchMatchCase");
    _searchWrapCB = new QCheckBox(tr("Wrap around"), group);
    _searchWrapCB->setObjectName("chkSearchWrap");
    _searchRecursiveCB = new QCheckBox(tr("Search subfolders"), group);
    _searchRecursiveCB->setObjectName("chkSearchRecursive");
    _searchHiddenCB = new QCheckBox(tr("Search hidden folders"), group);
    _searchHiddenCB->setObjectName("chkSearchHidden");
    options->addWidget(_searchMatchWordCB);
    options->addWidget(_searchMatchCaseCB);
    options->addWidget(_searchWrapCB);
    options->addWidget(_searchRecursiveCB);
    options->addWidget(_searchHiddenCB);
    layout->addWidget(group);
    layout->addStretch();
    return w;
}

QWidget* PreferenceDlg::makePage_MultiInstance()
{
    QWidget* w = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(w);
    QGroupBox* instanceGroup = new QGroupBox(tr("Multi-Instance"), w);
    instanceGroup->setObjectName("grpMultiInstance");
    QFormLayout* instanceForm = new QFormLayout(instanceGroup);
    _multiInstanceCombo = new QComboBox(instanceGroup);
    _multiInstanceCombo->setObjectName("comboMultiInstance");
    _multiInstanceCombo->addItem(tr("Open in existing instance"), 0);
    _multiInstanceCombo->addItem(tr("New instance for each session"), 1);
    _multiInstanceCombo->addItem(tr("Always open a new instance"), 2);
    QLabel* instanceModeLabel = new QLabel(tr("Mode:"), instanceGroup);
    instanceModeLabel->setObjectName("lblMultiInstanceMode");
    instanceForm->addRow(instanceModeLabel, _multiInstanceCombo);
    layout->addWidget(instanceGroup);
    QGroupBox* dateGroup = new QGroupBox(tr("Date and Time"), w);
    dateGroup->setObjectName("grpDateTime");
    QFormLayout* dateForm = new QFormLayout(dateGroup);
    _dateTimeFormatEdit = new QLineEdit(dateGroup);
    _dateReverseCB = new QCheckBox(tr("Reverse short and long command order"), dateGroup);
    _dateReverseCB->setObjectName("chkDateReverse");
    QLabel* dateFormatLabel = new QLabel(tr("Customized format:"), dateGroup);
    dateFormatLabel->setObjectName("lblDateTimeFormat");
    dateForm->addRow(dateFormatLabel, _dateTimeFormatEdit);
    dateForm->addRow(_dateReverseCB);
    layout->addWidget(dateGroup);
    layout->addStretch();
    return w;
}

QWidget* PreferenceDlg::makePage_Delimiter()
{
    QWidget* w = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(w);
    QGroupBox* group = new QGroupBox(tr("Delimiter Selection"), w);
    group->setObjectName("grpDelimiterSelection");
    QFormLayout* form = new QFormLayout(group);
    _leftDelimiterSB = new QSpinBox(group);
    _rightDelimiterSB = new QSpinBox(group);
    _leftDelimiterSB->setRange(0, 0xFFFF);
    _rightDelimiterSB->setRange(0, 0xFFFF);
    _delimiterWholeDocumentCB =
        new QCheckBox(tr("Search delimiters in entire document"), group);
    _delimiterWholeDocumentCB->setObjectName("chkDelimiterWholeDocument");
    QLabel* leftDelimiterLabel =
        new QLabel(tr("Left delimiter character code:"), group);
    leftDelimiterLabel->setObjectName("lblLeftDelimiter");
    QLabel* rightDelimiterLabel =
        new QLabel(tr("Right delimiter character code:"), group);
    rightDelimiterLabel->setObjectName("lblRightDelimiter");
    form->addRow(leftDelimiterLabel, _leftDelimiterSB);
    form->addRow(rightDelimiterLabel, _rightDelimiterSB);
    form->addRow(_delimiterWholeDocumentCB);
    layout->addWidget(group);
    layout->addStretch();
    return w;
}

QWidget* PreferenceDlg::makePage_CloudLink()
{
    QWidget* w = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(w);
    QGroupBox* linkGroup = new QGroupBox(tr("Clickable Links"), w);
    linkGroup->setObjectName("grpClickableLinks");
    QFormLayout* form = new QFormLayout(linkGroup);
    _urlModeCombo = new QComboBox(linkGroup);
    _urlModeCombo->setObjectName("comboUrlMode");
    _urlModeCombo->addItem(tr("Disabled"), 0);
    _urlModeCombo->addItem(tr("Enabled without underline"), 1);
    _urlModeCombo->addItem(tr("Enabled with underline"), 2);
    QLabel* linkStyleLabel = new QLabel(tr("Link style:"), linkGroup);
    linkStyleLabel->setObjectName("lblLinkStyle");
    form->addRow(linkStyleLabel, _urlModeCombo);
    layout->addWidget(linkGroup);
    QPushButton* openConfig = new QPushButton(tr("Open configuration directory"), w);
    openConfig->setObjectName("btnOpenConfigDirectory");
    connect(openConfig, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(
            NppParameters::getInstance().getUserPath()));
    });
    layout->addWidget(openConfig);
    layout->addStretch();
    return w;
}

QWidget* PreferenceDlg::makePage_SearchEngine()
{
    QWidget* w = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(w);
    QGroupBox* group = new QGroupBox(tr("Search Engine"), w);
    group->setObjectName("grpSearchEngine");
    QFormLayout* form = new QFormLayout(group);
    _searchEngineCombo = new QComboBox(group);
    _searchEngineCombo->setObjectName("comboSearchEngine");
    _searchEngineCombo->addItem("DuckDuckGo", 0);
    _searchEngineCombo->addItem("Bing", 1);
    _searchEngineCombo->addItem("Google", 2);
    _searchEngineCombo->addItem("Yahoo", 3);
    _searchEngineCombo->addItem(tr("Custom"), 4);
    _searchEngineCustomEdit = new QLineEdit(group);
    QLabel* providerLabel = new QLabel(tr("Provider:"), group);
    providerLabel->setObjectName("lblSearchProvider");
    QLabel* customUrlLabel = new QLabel(tr("Custom URL:"), group);
    customUrlLabel->setObjectName("lblSearchCustomUrl");
    form->addRow(providerLabel, _searchEngineCombo);
    form->addRow(customUrlLabel, _searchEngineCustomEdit);
    connect(_searchEngineCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) {
        _searchEngineCustomEdit->setEnabled(
            _searchEngineCombo->currentData().toInt() == 4);
    });
    layout->addWidget(group);
    layout->addStretch();
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

    _printLineNumberCB->setChecked(gui._printLineNumber);
    _printOptionCombo->setCurrentIndex(qMax(
        0, _printOptionCombo->findData(gui._printOption)));
    _headerLeftEdit->setText(gui._printHeaderLeft);
    _headerMiddleEdit->setText(gui._printHeaderMiddle);
    _headerRightEdit->setText(gui._printHeaderRight);
    _footerLeftEdit->setText(gui._printFooterLeft);
    _footerMiddleEdit->setText(gui._printFooterMiddle);
    _footerRightEdit->setText(gui._printFooterRight);

    const FindHistoryState& findHistory = params.getFindHistory();
    _searchMatchWordCB->setChecked(findHistory.matchWord);
    _searchMatchCaseCB->setChecked(findHistory.matchCase);
    _searchWrapCB->setChecked(findHistory.wrap);
    _searchRecursiveCB->setChecked(findHistory.fifRecursive);
    _searchHiddenCB->setChecked(findHistory.fifInHiddenFolder);

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
    _searchEngineCombo->setCurrentIndex(qMax(
        0, _searchEngineCombo->findData(gui._searchEngineChoice)));
    _searchEngineCustomEdit->setText(gui._searchEngineCustom);
    _searchEngineCustomEdit->setEnabled(gui._searchEngineChoice == 4);

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

    gui._openSaveDir = _defaultDirModeCombo->currentData().toInt();
    gui._defaultDirPath = _defaultDirEdit->text();

    gui._smartHighlight = _smartHighlightCB->isChecked();
    gui._smartHighlightMatchCase = _smartMatchCaseCB->isChecked();
    gui._smartHighlightWholeWord = _smartWholeWordCB->isChecked();
    gui._smartHighlightUseFindSettings = _smartUseFindSettingsCB->isChecked();
    gui._smartHighlightAnotherView = _smartAnotherViewCB->isChecked();
    gui._enableTagsMatchHighlight = _tagMatchCB->isChecked();
    gui._enableTagAttrsHighlight = _tagAttributesCB->isChecked();

    gui._printLineNumber = _printLineNumberCB->isChecked();
    gui._printOption = _printOptionCombo->currentData().toInt();
    gui._printHeaderLeft = _headerLeftEdit->text();
    gui._printHeaderMiddle = _headerMiddleEdit->text();
    gui._printHeaderRight = _headerRightEdit->text();
    gui._printFooterLeft = _footerLeftEdit->text();
    gui._printFooterMiddle = _footerMiddleEdit->text();
    gui._printFooterRight = _footerRightEdit->text();

    FindHistoryState& findHistory = params.getFindHistory();
    findHistory.matchWord = _searchMatchWordCB->isChecked();
    findHistory.matchCase = _searchMatchCaseCB->isChecked();
    findHistory.wrap = _searchWrapCB->isChecked();
    findHistory.fifRecursive = _searchRecursiveCB->isChecked();
    findHistory.fifInHiddenFolder = _searchHiddenCB->isChecked();

    gui._multiInstSetting = _multiInstanceCombo->currentData().toInt();
    gui._dateTimeFormat = _dateTimeFormatEdit->text();
    gui._dateTimeReverseDefaultOrder = _dateReverseCB->isChecked();
    gui._leftmostDelimiter = _leftDelimiterSB->value();
    gui._rightmostDelimiter = _rightDelimiterSB->value();
    gui._delimiterSelectionOnEntireDocument =
        _delimiterWholeDocumentCB->isChecked();
    gui._urlMode = _urlModeCombo->currentData().toInt();
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

    // 更新 Qt 扩展冗余副本
    gui._showWhitespace  = svp._whiteSpaceShow;
    gui._showEol         = svp._eolShow;
    gui._doWordWrap      = svp._doWrap;
    gui._restoreSession  = gui._rememberLastSession;

    params.writeNppGUI();
    params.writeFindHistory();
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
