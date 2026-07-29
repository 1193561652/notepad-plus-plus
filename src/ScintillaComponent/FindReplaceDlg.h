// FindReplaceDlg.h - 查找替换对话框
// 移植自: v8.4.6:PowerEditor/src/ScintillaComponent/FindReplaceDlg.h

#ifndef FINDREPLACEDLG_H
#define FINDREPLACEDLG_H

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTabBar>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QSlider>

class ScintillaEditView;

enum class SearchMode { Normal, Extended, Regex };

// 对应原版 FoundInfo：Find All 单条结果
struct FindAllResult {
    qint64  lineMatchStart = 0;
    int     lineNo;     // 0-based 行号
    qint64  matchStart; // 文档字节偏移（用于导航）
    qint64  matchLen;   // 匹配长度（字节）
    QString lineText;   // 行文本（去掉行尾空白）
    QString filePath;   // 跨文档/文件查找时用于定位来源
    QString sourceName; // 未命名 Buffer 或文件显示名
};

struct FindOption {
    bool       _isMatchCase  = true;   // 原版默认 true
    bool       _isWholeWord  = false;
    bool       _isWrapAround = true;   // 原版默认 true
    bool       _isForward    = true;
    bool       _isRegex      = false;
    bool       _dotMatchesNewline = false;
    SearchMode _mode         = SearchMode::Normal;
};

class FindReplaceDlg : public QDialog
{
    Q_OBJECT

public:
    explicit FindReplaceDlg(QWidget* parent = nullptr);
    ~FindReplaceDlg() = default;

    void setCurrentView(ScintillaEditView* view) { _currentView = view; }

signals:
    // 查找全部完成后发出，供主窗口在结果面板中显示
    void findAllResultsReady(const QString& searchText,
                             const QList<FindAllResult>& results);
    void findAllOpenedDocsRequested(const QString& searchText,
                                    const FindOption& opt);
    void findInFilesRequested(const QString& searchText,
                              const FindOption& opt,
                              const QString& directory,
                              const QString& filters,
                              bool recursive,
                              bool includeHidden);
    void replaceAllOpenedDocsRequested(const QString& searchText,
                                       const QString& replaceText,
                                       const FindOption& opt);
    void replaceInFilesRequested(const QString& searchText,
                                 const QString& replaceText,
                                 const FindOption& opt,
                                 const QString& directory,
                                 const QString& filters,
                                 bool recursive,
                                 bool includeHidden);
    void findInProjectsRequested(const QString& searchText,
                                 const FindOption& opt,
                                 const QString& filters,
                                 int panelMask);
    void replaceInProjectsRequested(const QString& searchText,
                                    const QString& replaceText,
                                    const FindOption& opt,
                                    const QString& filters,
                                    int panelMask);

public:
    void openFindTab();
    void openReplaceTab();
    void openMarkTab();
    void openFindInFilesTab();
    void openFindInProjectsTab(int panelMask);
    bool findNext(bool forward = true);
    void findAllInOpenedDocs();

    void setSearchText(const QString& text);
    void loadFindHistory();
    void saveFindHistory();
    bool executeSavedMacroAction(
        int message, int value, const QString& text);

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void changeEvent(QEvent* event) override;   // 透明度（失去焦点后）

private slots:
    void onTabChanged(int index);
    void onFindNext();
    void onCount();
    void onFindAllInCurrentDoc();
    void onFindAllInOpenedDocs();
    void onFindAllInFiles();
    void onFindAllInProjects();
    void onReplace();
    void onReplaceAll();
    void onReplaceAllOpenedDocs();
    void onReplaceInFiles();
    void onReplaceInProjects();
    void onMarkAll();
    void onClearAllMarks();
    void onCopyMarkedText();
    void onTransparencyChanged();   // 透明度控件联动

private:
    void setupUi();
    QWidget* makeInputArea();

    // 每 tab 的选项区
    QWidget* makeFindOptions();
    QWidget* makeReplaceOptions();
    QWidget* makeFifOptions();
    QWidget* makeFipOptions();
    QWidget* makeMarkOptions();

    // 每 tab 的按钮列（含取消按钮）
    QWidget* makeFindButtons();
    QWidget* makeReplaceButtons();
    QWidget* makeFifButtons();
    QWidget* makeFipButtons();
    QWidget* makeMarkButtons();

    // 搜索模式 + 透明度并排底部
    void makeBottomRow(QLayout* parentLayout);

    bool    doFind(bool forward);
    int     countOccurrences();
    void    markAllOccurrences(bool bookmarkLine, bool purge);
    QString processExtended(const QString& text) const;
    QString currentFindText() const;
    QString currentReplaceText() const;
    FindOption buildOptions() const;
    bool isInSelectionMode() const;
    void applyTransparency();

    void addToFindHistory(const QString& text);
    void addToReplaceHistory(const QString& text);
    void trimComboHistory(QComboBox* combo, int maxItems);
    void setStatus(const QString& msg, bool isError = true);

    // ── Tab 条 ──────────────────────────────────────────────────────────────
    QTabBar* _tabBar = nullptr;

    // ── 输入行 ──────────────────────────────────────────────────────────────
    QComboBox*   _findCombo    = nullptr;
    QWidget*     _replaceRow   = nullptr;
    QWidget*     _inSelFindRow = nullptr;   // "仅在选区中" 行（Find tab 专用）
    QLabel*      _replaceLbl   = nullptr;
    QComboBox*   _replaceCombo = nullptr;
    QWidget*     _filtersRow   = nullptr;
    QComboBox*   _filtersCombo = nullptr;
    QWidget*     _dirRow       = nullptr;
    QComboBox*   _dirCombo     = nullptr;
    QPushButton* _browseDirBtn = nullptr;

    // ── 搜索模式（底部左侧，与透明度并排） ──────────────────────────────────
    QGroupBox*    _modeBox         = nullptr;
    QRadioButton* _modeNormal      = nullptr;
    QRadioButton* _modeExtended    = nullptr;
    QRadioButton* _modeRegex       = nullptr;
    QCheckBox*    _dotMatchNewline = nullptr;
    QButtonGroup* _modeGroup       = nullptr;

    // ── 透明度（底部右侧，对应原版 IDC_TRANSPARENT_GRPBOX） ─────────────────
    QGroupBox*    _transGB               = nullptr;  // checkable groupbox
    QRadioButton* _transOnLostFocusRB    = nullptr;
    QRadioButton* _transAlwaysRB         = nullptr;
    QButtonGroup* _transModeGroup        = nullptr;
    QSlider*      _transSlider           = nullptr;

    // ── 每 tab 选项区 ────────────────────────────────────────────────────────
    QStackedWidget* _optionsStack = nullptr;

    // Find tab 选项（page 0）—— 裸复选框，无 GroupBox
    QCheckBox* _backwardDir  = nullptr;
    QCheckBox* _wholeWord    = nullptr;
    QCheckBox* _matchCase    = nullptr;
    QCheckBox* _wrapAround   = nullptr;
    QCheckBox* _inSelCB      = nullptr;

    // Replace tab 选项（page 1）
    QCheckBox* _matchCase2   = nullptr;
    QCheckBox* _wholeWord2   = nullptr;
    QCheckBox* _wrapAround2  = nullptr;
    QCheckBox* _inSelCB2     = nullptr;

    // Find in Files 选项（page 2）
    QCheckBox* _matchCase3     = nullptr;
    QCheckBox* _wholeWord3     = nullptr;
    QCheckBox* _followDocCB    = nullptr;
    QCheckBox* _recursiveCB    = nullptr;
    QCheckBox* _inHiddenDirCB  = nullptr;
    QCheckBox* _projectPanelCB[3] = {nullptr, nullptr, nullptr};

    // Mark 选项（page 4）
    QCheckBox* _bookmarkLineCB = nullptr;
    QCheckBox* _purgeMarksCB   = nullptr;
    QCheckBox* _matchCase4     = nullptr;
    QCheckBox* _wholeWord4     = nullptr;
    QCheckBox* _inSelCB4       = nullptr;

    // ── 每 tab 按钮列（含取消按钮） ──────────────────────────────────────────
    QStackedWidget* _btnStack = nullptr;

    // Find tab 按钮
    QPushButton* _findNextBtn      = nullptr;
    QPushButton* _countBtn         = nullptr;
    QPushButton* _findAllOpenedBtn = nullptr;  // 查找所有打开文件（3rd）
    QPushButton* _findAllCurBtn    = nullptr;  // 在当前文件中查找（4th）

    // Replace tab 按钮
    QPushButton* _findNextBtn2  = nullptr;
    QPushButton* _replaceBtn    = nullptr;
    QPushButton* _replaceAllBtn = nullptr;

    // Find in Files 按钮
    QPushButton* _findAllFifBtn     = nullptr;
    QPushButton* _replaceInFilesBtn = nullptr;

    // Mark 按钮
    QPushButton* _markAllBtn    = nullptr;
    QPushButton* _clearMarksBtn = nullptr;
    QPushButton* _copyMarkedBtn = nullptr;

    // ── 状态栏 ──────────────────────────────────────────────────────────────
    QLabel* _statusLabel = nullptr;

    ScintillaEditView* _currentView = nullptr;
    struct SavedMacroSearch {
        QString findText;
        QString replaceText;
        QString directory;
        QString filters;
        FindOption options;
        bool recursive = true;
        bool includeHidden = false;
        bool purge = false;
        bool bookmarkLine = false;
        bool inSelection = false;
        int projectMask = 0;
    } _savedMacroSearch;
};

#endif // FINDREPLACEDLG_H
