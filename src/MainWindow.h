// MainWindow.h - 主窗口类
// 移植自: v8.4.6:PowerEditor/src/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QMap>
#include <QTimer>
#include <QSet>

#include "ScintillaComponent/Buffer.h"
#include "ScintillaComponent/FindReplaceDlg.h"
#include "MISC/PluginsManager/PluginUpdatePlan.h"
#include "CommandLineOptions.h"
#include "MISC/ClosedFileHistory.h"
#include "WinControls/DockingWnd/DockingManager.h"

class ScintillaEditView;
class DocTabView;
class FindReplaceDlg;
class PreferenceDlg;
class DocumentMapPanel;
class FunctionListPanel;
class PluginManager;
class PluginHostServices;
#ifdef Q_OS_WIN
class Win32PluginManager;
#endif
class PluginAdminDialog;
class PluginAdminModel;
class EditorMacro;
class QSplitter;
class QDockWidget;
class QListWidget;
class FileBrowserPanel;
class ProjectPanel;
class QFileSystemWatcher;
struct MacroDef;

// 视图 ID（对应原版 MAIN_VIEW / SUB_VIEW）
static const int MAIN_VIEW = 0;
static const int SUB_VIEW  = 1;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const CommandLineOptions& startupOptions = {},
                        QWidget *parent = nullptr);
    ~MainWindow();
    void applyCommandLineInvocation(const CommandLineOptions& options);

    ScintillaEditView* currentView() { return currentActiveView(); }
    void openFile(const QString& path) { doOpenFile(path, _activeDocTab); }
    QString currentFilePath() const;
    DockingManager& dockingManager() { return _dockingManager; }
    const DockingManager& dockingManager() const { return _dockingManager; }
    QString currentPathForPlugin() const;
    bool executePluginMenuCommand(int commandId);
    bool openFileForPlugin(const QString& path);
    bool saveCurrentFileAsForPlugin(const QString& path, bool asCopy);
    bool saveCurrentFileForPlugin();
    bool saveCurrentSessionForPlugin(const QString& path);
    bool loadSessionForPlugin(const QString& path);
    bool setCurrentLanguageTypeFromPlugin(int languageType);
    quintptr currentBufferIdForPlugin() const;
    QString pathForPluginBuffer(quintptr bufferId) const;
    int positionForPluginBuffer(quintptr bufferId, int priorityView) const;
    int openFileCountForPlugin(int scope) const;
    int currentDocumentIndexForPlugin(int view) const;
    bool activateDocumentForPlugin(int view, int index);
    int currentLineForPlugin() const;
    int bufferEncodingForPlugin(quintptr bufferId) const;
    bool setBufferEncodingForPlugin(quintptr bufferId, int encoding);
    void setPluginStatusBarText(int section, const QString& text);
    bool addPluginToolbarCommand(int commandId);
    bool createDocumentForPlugin(const QByteArray& data);
    int currentViewIndexForPlugin() const;
    ScintillaEditView* pluginView(int view) const;
    bool showPluginBufferInView(quintptr bufferId, int view);
#ifdef Q_OS_WIN
    Win32PluginManager* win32PluginManager() const
    {
        return _win32PluginManager;
    }
#endif

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void createActions();
    void createMenus();
    void createEncodingMenu();
    void createToolBars();
    void applyToolbarIcons();
    void createStatusBar();
    void setupTabViews();

    // 编码操作
    void encodeIn(const QString& codec, bool hasBom);
    void convertTo(const QString& codec, bool hasBom);
    void reinterpretAs(const QString& codec);

    // 动作状态同步
    void initActionStates();   // 启动时从配置初始化 checked/enabled
    void updateActionStates(); // 每次状态变化时刷新

    // 文件操作
    void connectModificationSignal(ScintillaEditView* view, Buffer* buf);
    void showEditorContextMenu(ScintillaEditView* view, const QPoint& position);
    Buffer* doNewBuffer(DocTabView* targetTab = nullptr);
    bool doOpenFile(const QString& filePath, DocTabView* targetTab = nullptr,
                    const QString& forcedEncoding = QString());
    bool doSave(Buffer* buf, const QString& filePath);
    bool doSaveAs(Buffer* buf);
    bool closeBufferList(const QList<Buffer*>& buffers);
    void syncDocumentMap();

    // 视图操作
    void connectTabView(DocTabView* tab);
    DocTabView* otherTab() const;
    void setActiveTab(DocTabView* tab);
    void showSubView();
    void hideSubView();
    void setBufferEolMode(Buffer* buffer, int mode, bool convertText);
    void setupFileBrowser();
    void setupDocumentMap();
    void setupFunctionList();
    void setupFindResultPanel();
    void setupAuxiliaryPanels();
    void setupPluginSystem();
    void populateCrossPlatformPluginMenu();
#ifdef Q_OS_WIN
    void populateWin32PluginMenu();
#endif
    void showPluginAdmin();
    bool schedulePluginOperations(
        const QVector<PluginOperation>& operations);
    bool launchPendingPluginUpdater(QString* error = nullptr);
    FindReplaceDlg* ensureFindReplaceDialog();
    void connectFindReplaceDialogSignals();
    QStringList projectFiles(int panelMask) const;
    void watchBufferFile(Buffer* buf);
    void unwatchBufferFile(Buffer* buf);

    // 最近文件
    void addToRecentFiles(const QString& filePath);
    void updateRecentFilesMenu();
    void rememberClosedFile(const QString& filePath);

    // 会话
    void saveSession();
    void restoreSession();

    // 偏好设置
    void applyPreferencesToAllViews();
    void applyPreferencesToView(ScintillaEditView* view);
    void applyDarkMode();
    void notifyCurrentLanguageChanged();
    void setBufferReadOnly(Buffer* buffer, bool readOnly);
    void registerNppCommandIds();
    void applyConfiguredShortcuts();
    void applyScintillaShortcuts(ScintillaEditView* view);
    bool executeNppCommand(int commandId);
    void playConfiguredMacro(const MacroDef& macro);
    void rebuildConfiguredMacroMenu();

    // 辅助
    bool checkBufferSave(Buffer* buf);
    void updateWindowTitle(Buffer* buf);
    void updateFindReplaceView();
    void updateStatusBar();
    void updateLangStatus();   // 更新状态栏语言类型（对应原版 setLangStatus）
    ScintillaEditView* currentActiveView() const;
    ScintillaEditView* activateBufferView(
        Buffer* buffer, DocTabView* preferredTab = nullptr);
    void applyFileCommandLineState(Buffer* buffer,
                                   const CommandLineOptions& options);

    // 多语言（对应原版 _nativeLangSpeaker.changeMenuLang）
    // 将 NativeLangSpeaker 中的翻译应用到所有菜单/动作（运行时热切换）
    void applyNativeLang();

private slots:
    // 文件菜单
    void newFile();
    void openFile();
    void saveFile();
    void saveFileAs();
    void saveAllFiles();
    void closeFile();
    void closeAllFiles();
    void reloadFromDisk();
    void about();
    void openRecentFile();
    void clearRecentFiles();
    void restoreLastClosedFile();
    void printDocument();
    void printDocumentNow();
    void saveCopyAs();
    void renameCurrentFile();
    void moveCurrentFileToTrash();
    void closeAllButCurrent();
    void closeAllToLeft();
    void closeAllToRight();
    void closeAllUnchanged();
    void loadSessionFile();
    void saveSessionFile();
    void openContainingFolder();
    void openContainingTerminal();
    void openDefaultViewer();
    void openFolderAsWorkspace();
    // 搜索菜单
    void find();
    void replace();
    void goToLine();
    void toggleBookmark();
    void nextBookmark();
    void prevBookmark();
    void clearAllBookmarks();
    // 视图菜单
    void zoomIn();
    void zoomOut();
    void zoomRestore();
    void toggleWordWrap();
    void toggleWhitespace();
    void toggleIndentGuide();
    void toggleSplitView();
    void rotateSplitView();
    void moveToOtherView();
    void cloneToOtherView();
    void toggleFileBrowser();
    // 设置
    void showPreferences();
    // 宏
    void startMacroRecording();
    void stopMacroRecording();
    void playMacro();
    void saveMacro();
    void loadMacro();
    void columnEditor();
    void insertDateTime(int mode);
    void transformSelectionCase(int mode);
    void goToMatchingBrace();
    void jumpSearchMark(bool forward);
    void clearSearchMarks();
    // 内部
    void onBufferCloseRequested(Buffer* buf);
    void onCurrentTabChanged(int index);
    void onTextChanged();
    void onFocusChanged(QWidget* old, QWidget* now);
    void onCursorPositionChanged(int line, int col);
    void onBackupTimer();
    void onWatchedFileChanged(const QString& path);
    void onWatchedDirectoryChanged(const QString& path);
    void pollWatchedFiles();
    // Find All 结果（由 FindReplaceDlg::findAllResultsReady 触发）
    void onFindAllResults(const QString& searchText,
                          const QList<FindAllResult>& results);
    void onFindAllOpenedDocsRequested(const QString& searchText,
                                      const FindOption& opt);
    void onFindInFilesRequested(const QString& searchText,
                                const FindOption& opt,
                                const QString& directory,
                                const QString& filters,
                                bool recursive,
                                bool includeHidden);
    void onReplaceAllOpenedDocsRequested(const QString& searchText,
                                         const QString& replaceText,
                                         const FindOption& opt);
    void onReplaceInFilesRequested(const QString& searchText,
                                   const QString& replaceText,
                                   const FindOption& opt,
                                   const QString& directory,
                                   const QString& filters,
                                   bool recursive,
                                   bool includeHidden);
    void onFindInProjectsRequested(const QString& searchText,
                                   const FindOption& opt,
                                   const QString& filters,
                                   int panelMask);
    void onReplaceInProjectsRequested(const QString& searchText,
                                      const QString& replaceText,
                                      const FindOption& opt,
                                      const QString& filters,
                                      int panelMask);

    void sortLines(int mode);
    void transformLines(int mode);

private:
    static const int MaxRecentFiles = 10;

    // ── 视图 ──────────────────────────────────────────────
    QSplitter*        _splitter        = nullptr;
    DocTabView*       _mainDocTab      = nullptr;
    DocTabView*       _subDocTab       = nullptr;
    DocTabView*       _activeDocTab    = nullptr;
    DockingManager    _dockingManager;

    FindReplaceDlg*    _findReplaceDlg   = nullptr;
    PreferenceDlg*     _preferenceDlg    = nullptr;
    PluginAdminDialog* _pluginAdminDlg   = nullptr;
    PluginAdminModel*  _pluginAdminModel = nullptr;

    // Find All 结果面板
    QDockWidget*       _findResultDock   = nullptr;
    ScintillaEditView* _findResultView   = nullptr;
    QList<FindAllResult> _lastFindResults;
    QVector<int>       _findResultIndexByLine;
    QMetaObject::Connection _syncVerticalConnections[2];
    QMetaObject::Connection _syncHorizontalConnections[2];
    QDockWidget*       _fileBrowserDock  = nullptr;
    FileBrowserPanel*  _fileBrowserPanel = nullptr;
    QDockWidget*       _docMapDock       = nullptr;
    DocumentMapPanel*  _docMapPanel      = nullptr;
    Buffer*            _documentMapBuffer = nullptr;
    QDockWidget*       _funcListDock     = nullptr;
    FunctionListPanel* _funcListPanel    = nullptr;
    QDockWidget*       _documentListDock = nullptr;
    QListWidget*       _documentList     = nullptr;
    QDockWidget*       _projectPanelsDock[3] = {nullptr, nullptr, nullptr};
    ProjectPanel*      _projectPanels[3] = {nullptr, nullptr, nullptr};
    QDockWidget*       _clipboardDock    = nullptr;
    QListWidget*       _clipboardHistory = nullptr;
    QDockWidget*       _characterDock    = nullptr;
    QListWidget*       _characterList    = nullptr;
    PluginManager*     _pluginManager    = nullptr;
    PluginHostServices* _pluginHostServices = nullptr;
#ifdef Q_OS_WIN
    Win32PluginManager* _win32PluginManager = nullptr;
#endif
    QString _pendingPluginUpdatePlan;
    bool _pluginUpdaterStarted = false;
    bool _hasAppliedDarkMode = false;
    bool _appliedDarkMode = false;
    bool _shutdownNotificationPending = false;

    // 宏
    QString   _macroStr;
    bool      _isRecording = false;
    EditorMacro* _recordingMacro = nullptr;
    ScintillaEditView* _recordingView = nullptr;

    // 备份定时器（与原版 snapshot 模式一致，每 7 秒触发）
    QTimer*   _backupTimer = nullptr;
    QFileSystemWatcher* _fileWatcher = nullptr;
    QTimer* _externalFilePollTimer = nullptr;
    QSet<QString> _dismissedMissingPaths;
    QSet<QString> _savingPaths;
    CommandLineOptions _startupOptions;
    QString _titleAddition;
    bool _suppressSessionPersistence = false;
    bool _openingBuffer = false;
    ClosedFileHistory _closedFileHistory;

    // ── 状态栏 ────────────────────────────────────────────
    QLabel* _docTypeLabel  = nullptr;   // 语言类型（对应原版 STATUSBAR_DOC_TYPE）
    QLabel* _docSizeLabel  = nullptr;   // 文件长度+行数（对应原版 STATUSBAR_DOC_SIZE）
    QLabel* _posLabel      = nullptr;   // 行列+位置/选区（对应原版 STATUSBAR_CUR_POS）
    QLabel* _eolLabel      = nullptr;   // 换行符类型（对应原版 STATUSBAR_EOF_FORMAT）
    QLabel* _encodingLabel = nullptr;   // 编码（对应原版 STATUSBAR_UNICODE_TYPE）
    QLabel* _insertLabel   = nullptr;   // INS/OVR（对应原版 STATUSBAR_TYPING_MODE）

    // ── 菜单 ──────────────────────────────────────────────
    QMenu* _fileMenu         = nullptr;
    QMenu* _recentFilesMenu  = nullptr;
    QMenu* _editMenu         = nullptr;
    QMenu* _searchMenu       = nullptr;
    QMenu* _viewMenu         = nullptr;
    QMenu* _encodingMenu     = nullptr;
    QMenu* _languageMenu     = nullptr;
    QMenu* _formatMenu       = nullptr;
    QMenu* _settingsMenu     = nullptr;
    QMenu* _helpMenu         = nullptr;

    // ── 工具栏 ────────────────────────────────────────────
    QToolBar* _fileToolBar = nullptr;

    // ── 文件动作 ──────────────────────────────────────────
    QAction* _newAction      = nullptr;
    QAction* _openAction     = nullptr;
    QAction* _saveAction     = nullptr;
    QAction* _saveAsAction   = nullptr;
    QAction* _saveAllAction  = nullptr;
    QAction* _closeAction    = nullptr;
    QAction* _closeAllAction = nullptr;
    QAction* _restoreLastClosedAction = nullptr;
    QAction* _reloadAction   = nullptr;
    QAction* _exitAction     = nullptr;
    QAction* _printAction    = nullptr;
    QAction* _printNowAction = nullptr;

    // ── 编辑动作 ──────────────────────────────────────────
    QAction* _undoAction          = nullptr;
    QAction* _redoAction          = nullptr;
    QAction* _cutAction           = nullptr;
    QAction* _copyAction          = nullptr;
    QAction* _pasteAction         = nullptr;
    QAction* _selectAllAction     = nullptr;
    QAction* _deleteLineAction    = nullptr;
    QAction* _duplicateLineAction = nullptr;
    QAction* _moveLineUpAction    = nullptr;
    QAction* _moveLineDownAction  = nullptr;
    QAction* _toggleCommentAction = nullptr;
    QAction* _toUpperCaseAction   = nullptr;
    QAction* _toLowerCaseAction   = nullptr;

    // ── 搜索动作 ──────────────────────────────────────────
    QAction* _findAction           = nullptr;
    QAction* _replaceAction        = nullptr;
    QAction* _goToLineAction       = nullptr;
    QAction* _toggleBookmarkAction = nullptr;
    QAction* _nextBookmarkAction   = nullptr;
    QAction* _prevBookmarkAction   = nullptr;
    QAction* _clearBookmarksAction = nullptr;

    // ── 视图动作 ──────────────────────────────────────────
    QAction* _splitViewAction      = nullptr;
    QAction* _rotateSplitAction    = nullptr;
    QAction* _moveToOtherAction    = nullptr;
    QAction* _cloneToOtherAction   = nullptr;
    QAction* _fileBrowserAction    = nullptr;
    QAction* _zoomInAction         = nullptr;
    QAction* _zoomOutAction        = nullptr;
    QAction* _zoomRestoreAction    = nullptr;
    QAction* _wordWrapAction       = nullptr;
    QAction* _showWhitespaceAction = nullptr;
    QAction* _showIndentAction     = nullptr;

    // ── 格式动作 ──────────────────────────────────────────
    QAction* _eolWindowsAction = nullptr;
    QAction* _eolUnixAction    = nullptr;
    QAction* _eolMacAction     = nullptr;

    // ── 视图面板动作 ───────────────────────────────────────
    QAction* _docMapAction   = nullptr;
    QAction* _funcListAction = nullptr;

    // ── 宏动作 ────────────────────────────────────────────
    QAction* _startRecordAction = nullptr;
    QAction* _stopRecordAction  = nullptr;
    QAction* _playMacroAction   = nullptr;
    QAction* _saveMacroAction   = nullptr;
    QAction* _loadMacroAction   = nullptr;
    QMenu* _macroMenu           = nullptr;
    QList<QAction*> _documentActions;

    // ── 设置动作 ──────────────────────────────────────────
    QAction* _preferencesAction = nullptr;

    // ── 插件菜单 ──────────────────────────────────────────
    QMenu* _pluginsMenu = nullptr;

    // ── 帮助动作 ──────────────────────────────────────────
    QAction* _aboutAction = nullptr;
};

#endif // MAINWINDOW_H
