// PreferenceDlg.h - 偏好设置对话框（与原版 19 个分类页面对应）
// 移植自: v8.4.6:PowerEditor/src/WinControls/Preference/

#ifndef PREFERENCEDLG_H
#define PREFERENCEDLG_H

#include <QDialog>
#include <QListWidget>
#include <QStackedWidget>
#include <QFontComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QComboBox>
#include <QScrollArea>
#include <QLineEdit>

class PreferenceDlg : public QDialog
{
    Q_OBJECT
public:
    explicit PreferenceDlg(QWidget* parent = nullptr);
    void loadSettings();
    void saveSettings();

signals:
    void settingsChanged();

private slots:
    void onApply();
    void onTabHideToggled(bool hidden);
    void onSnapshotModeToggled(bool checked);
    void onAcEnableToggled(bool checked);

private:
    void setupUi();

    // 页面构建（顺序与原版一致）
    QWidget* makePage_General();        // 1
    QWidget* makePage_Editing();        // 2
    QWidget* makePage_DarkMode();       // 3
    QWidget* makePage_MarginsBorderEdge(); // 4
    QWidget* makePage_NewDocument();    // 5
    QWidget* makePage_DefaultDirectory(); // 6
    QWidget* makePage_RecentFilesHistory(); // 7
    QWidget* makePage_FileAssociation(); // 8
    QWidget* makePage_Language();       // 9
    QWidget* makePage_Highlighting();   // 10
    QWidget* makePage_Print();          // 11
    QWidget* makePage_Searching();      // 12
    QWidget* makePage_Backup();         // 13
    QWidget* makePage_AutoCompletion(); // 14
    QWidget* makePage_MultiInstance();  // 15
    QWidget* makePage_Delimiter();      // 16
    QWidget* makePage_CloudLink();      // 17
    QWidget* makePage_SearchEngine();   // 18
    QWidget* makePage_Misc();           // 19

    static QScrollArea* wrapScroll(QWidget* inner);

    QListWidget*    _pageList  = nullptr;
    QStackedWidget* _pageStack = nullptr;

    // ── 1. General ────────────────────────────────────────────────────────────
    QCheckBox* _hideToolbarCB        = nullptr;
    QCheckBox* _showStatusBarCB      = nullptr;
    QCheckBox* _showMenuBarCB        = nullptr;
    // Tab bar
    QCheckBox* _tabHideCB            = nullptr;
    QCheckBox* _tabDragDropCB        = nullptr;
    QCheckBox* _tabTopBarCB          = nullptr;
    QCheckBox* _tabInactiveTabCB     = nullptr;
    QCheckBox* _tabCloseBtnCB        = nullptr;
    QCheckBox* _tabDblClickCloseCB   = nullptr;
    QCheckBox* _tabVerticalCB        = nullptr;
    QCheckBox* _tabMultiLineCB       = nullptr;
    QCheckBox* _tabQuitOnEmptyCB     = nullptr;
    // Editor font (原版在 Style Configurator，移植版简化放此处)
    QFontComboBox* _fontCombo        = nullptr;
    QSpinBox*      _fontSizeSB       = nullptr;

    // ── 3. Dark Mode ─────────────────────────────────────────────────────────
    QCheckBox* _darkModeEnableCB     = nullptr;

    // ── 2. Editing ────────────────────────────────────────────────────────────
    QCheckBox*    _autoIndentCB            = nullptr;
    QCheckBox*    _scrollBeyondLastLineCB  = nullptr;
    QCheckBox*    _showWhitespaceCB        = nullptr;
    QCheckBox*    _showEolCB              = nullptr;
    // Line wrap
    QCheckBox*    _wordWrapCB             = nullptr;
    QRadioButton* _lwDefaultRB            = nullptr;
    QRadioButton* _lwAlignRB              = nullptr;
    QRadioButton* _lwIndentRB             = nullptr;

    // ── 4. Margins/Border/Edge ────────────────────────────────────────────────
    QCheckBox* _lineNumberMarginCB     = nullptr;
    QCheckBox* _bookMarkMarginCB       = nullptr;
    QCheckBox* _foldMarginCB           = nullptr;
    QCheckBox* _indentGuideLineCB      = nullptr;
    QCheckBox* _currentLineHighlightCB = nullptr;
    QCheckBox* _wrapSymbolShowCB       = nullptr;
    QCheckBox* _edgeShowCB             = nullptr;
    QSpinBox*  _edgeColumnSB           = nullptr;
    QLabel*    _edgeColumnLabel        = nullptr;

    // ── 5. New Document ───────────────────────────────────────────────────────
    QRadioButton* _fmtWindowsRB  = nullptr;
    QRadioButton* _fmtUnixRB     = nullptr;
    QRadioButton* _fmtMacRB      = nullptr;
    QComboBox*    _encodingCombo  = nullptr;

    // ── 7. Recent Files History ───────────────────────────────────────────────
    QSpinBox*  _maxRecentFilesSB  = nullptr;
    QCheckBox* _recentInSubMenuCB = nullptr;

    QComboBox* _defaultDirModeCombo = nullptr;
    QLineEdit* _defaultDirEdit = nullptr;

    QCheckBox* _smartHighlightCB = nullptr;
    QCheckBox* _smartMatchCaseCB = nullptr;
    QCheckBox* _smartWholeWordCB = nullptr;
    QCheckBox* _smartUseFindSettingsCB = nullptr;
    QCheckBox* _smartAnotherViewCB = nullptr;
    QCheckBox* _tagMatchCB = nullptr;
    QCheckBox* _tagAttributesCB = nullptr;

    QCheckBox* _printLineNumberCB = nullptr;
    QComboBox* _printOptionCombo = nullptr;
    QLineEdit* _headerLeftEdit = nullptr;
    QLineEdit* _headerMiddleEdit = nullptr;
    QLineEdit* _headerRightEdit = nullptr;
    QLineEdit* _footerLeftEdit = nullptr;
    QLineEdit* _footerMiddleEdit = nullptr;
    QLineEdit* _footerRightEdit = nullptr;

    QCheckBox* _searchMatchWordCB = nullptr;
    QCheckBox* _searchMatchCaseCB = nullptr;
    QCheckBox* _searchWrapCB = nullptr;
    QCheckBox* _searchRecursiveCB = nullptr;
    QCheckBox* _searchHiddenCB = nullptr;

    QComboBox* _multiInstanceCombo = nullptr;
    QLineEdit* _dateTimeFormatEdit = nullptr;
    QCheckBox* _dateReverseCB = nullptr;

    QSpinBox* _leftDelimiterSB = nullptr;
    QSpinBox* _rightDelimiterSB = nullptr;
    QCheckBox* _delimiterWholeDocumentCB = nullptr;

    QComboBox* _urlModeCombo = nullptr;
    QComboBox* _searchEngineCombo = nullptr;
    QLineEdit* _searchEngineCustomEdit = nullptr;

    // ── 9. Language ───────────────────────────────────────────────────────────
    QComboBox* _languageCombo     = nullptr;

    // ── 13. Backup ────────────────────────────────────────────────────────────
    QRadioButton* _backupNoneRB       = nullptr;
    QRadioButton* _backupSimpleRB     = nullptr;
    QRadioButton* _backupVerboseRB    = nullptr;
    QCheckBox*    _backupCustomDirCB  = nullptr;
    QLineEdit*    _backupDirEdit      = nullptr;
    QCheckBox*    _snapshotModeCB     = nullptr;
    QSpinBox*     _snapshotTimingSB   = nullptr;
    QLabel*       _snapshotTimingLabel= nullptr;

    // ── 14. Auto-Completion ───────────────────────────────────────────────────
    QCheckBox*    _acEnableCB         = nullptr;
    QRadioButton* _acNoneRB           = nullptr;
    QRadioButton* _acFunctionRB       = nullptr;
    QRadioButton* _acWordRB           = nullptr;
    QRadioButton* _acBothRB           = nullptr;
    QSpinBox*     _acFromNbCharSB     = nullptr;
    QLabel*       _acFromNbCharLabel  = nullptr;
    QCheckBox*    _acIgnoreNumbersCB  = nullptr;
    QCheckBox*    _funcParamsCB       = nullptr;
    QCheckBox*    _pairParenthesesCB  = nullptr;
    QCheckBox*    _pairBracketsCB     = nullptr;
    QCheckBox*    _pairCurlyCB        = nullptr;
    QCheckBox*    _pairQuotesCB       = nullptr;
    QCheckBox*    _pairDoubleQuotesCB = nullptr;
    QCheckBox*    _pairTagsCB         = nullptr;

    // ── 19. MISC. ─────────────────────────────────────────────────────────────
    QComboBox* _fileAutoDetectCombo   = nullptr;
    QCheckBox* _checkHistoryFilesCB   = nullptr;
    QCheckBox* _restoreSessionCB      = nullptr;

    bool _langChanged = false;
};

#endif // PREFERENCEDLG_H
