// Independent Notepad++ Qt port implementation by Jiang Liwei.
// Derived from v8.4.6:PowerEditor/src/; see ../QT_PORT_NOTICE.md and ../LICENSE.

#include "MainWindow.h"
#include "NppCommandRegistry.h"
#include "ScintillaComponent/ScintillaEditView.h"
#include "ScintillaComponent/Buffer.h"
#include "ScintillaComponent/DocTabView.h"
#include "ScintillaComponent/FileManager.h"
#include "EncodingMapper.h"
#include "MISC/PlatformServices.h"
#include "MISC/TextFileCodec.h"
#include "WinControls/ToolBar/ToolbarIconTheme.h"
#include "ScintillaComponent/FindReplaceDlg.h"
#include "ScintillaComponent/ScintillaTextSearch.h"
#include "ScintillaComponent/EditorMacro.h"
#include "WinControls/FileBrowser/fileBrowser.h"
#include "WinControls/DockingWnd/DockingManager.h"
#include "WinControls/Preference/preferenceDlg.h"
#include "WinControls/DocumentMap/documentMap.h"
#include "WinControls/FunctionList/functionListPanel.h"
#include "WinControls/ProjectPanel/ProjectPanel.h"
#include "WinControls/Grid/ShortcutMapper.h"
#include "MISC/PluginsManager/PluginManager.h"
#include "MISC/PluginsManager/PluginHostServices.h"
#include "WinControls/PluginsAdmin/PluginAdminDialog.h"
#include "WinControls/PluginsAdmin/PluginAdminModel.h"
#include "MISC/PluginsManager/PluginCatalog.h"
#include "MISC/PluginsManager/PluginUpdatePlan.h"
#include "ScintillaComponent/Printer.h"
#ifdef Q_OS_WIN
#include "Win32PluginSystem/Win32PluginManager.h"
#endif

#include "Parameters.h"
#include "localization.h"
#include <QDockWidget>
#include <QTextCodec>
#include <QFileDialog>
#include <QInputDialog>
#include <QDateTime>
#include <QFileInfo>
#include <QMessageBox>
#include <QCloseEvent>
#include <QMenuBar>
#include <QActionGroup>
#include <QToolBar>
#include <QStatusBar>
#include <QFile>
#include <QSaveFile>
#include <QTextStream>
#include <QTemporaryFile>
#include <QSplitter>
#include <QScrollBar>
#include <QListWidget>
#include <QApplication>
#include <QCoreApplication>
#include <QFileSystemWatcher>
#include <QDirIterator>
#include <QRegularExpression>
#include <QPrintDialog>
#include <QProgressDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QPalette>
#include <QRandomGenerator>
#include <QCollator>
#include <QCryptographicHash>
#include <QClipboard>
#include <QMimeData>
#include <QDesktopServices>
#include <QDebug>
#include <QProcess>
#include <QFontDialog>
#include <QColorDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QTabBar>
#include <QTabWidget>
#include <QXmlStreamReader>
#include <QSharedPointer>
#include <QWindow>
#include <algorithm>
#include <functional>

// Main controller construction and top-level coordination.

MainWindow::MainWindow(const CommandLineOptions& startupOptions, QWidget *parent)
    : QMainWindow(parent), _startupOptions(startupOptions),
      _titleAddition(startupOptions.titleAddition),
      _suppressSessionPersistence(startupOptions.noSession)
{
    setWindowTitle("Notepad++");
    setWindowIcon(QIcon(":/icons/npp.ico"));
    resize(900, 600);

    setupTabViews();
    _dockingManager.init(this, _splitter);
    _pluginHostServices = new MainWindowPluginHostServices(this);
#ifdef Q_OS_WIN
    if (!startupOptions.noPlugin) {
        const QString pluginStateDirectory =
            QDir(NppParameters::getInstance().getUserPath())
                .filePath(QStringLiteral("plugin-load"));
        _win32PluginManager = new Win32PluginManager(
            this, _mainDocTab->editor(), _subDocTab->editor(),
            pluginStateDirectory, &_dockingManager,
            _pluginHostServices);
    }
#endif
    createActions();
    createMenus();
    registerNppCommandIds();
    applyConfiguredShortcuts();
    rebuildConfiguredMacroMenu();
    createToolBars();
    createStatusBar();
    applyDarkMode();
    applyPreferencesToView(_mainDocTab->editor());
    applyPreferencesToView(_subDocTab->editor());

    _findReplaceDlg = new FindReplaceDlg(this);
    _preferenceDlg  = new PreferenceDlg(this);
    connectFindReplaceDialogSignals();

    setupFileBrowser();

    connect(_preferenceDlg, &PreferenceDlg::settingsChanged,
            this, &MainWindow::applyPreferencesToAllViews);

    setupDocumentMap();
    setupFunctionList();
    setupFindResultPanel();
    setupAuxiliaryPanels();
    if (!startupOptions.noPlugin)
        setupPluginSystem();

    // 所有菜单、动作和停靠面板创建完成后再保存原文并应用翻译。
    applyNativeLang();

    // 监听全局焦点变化，用于确定活动视图
    connect(qApp, &QApplication::focusChanged,
            this, &MainWindow::onFocusChanged);

    // 从 NppParameters 恢复窗口位置/状态
    NppParameters& params = NppParameters::getInstance();
    const NppGUI& gui = params.getNppGUI();

    _dockingManager.setDockedContSize(
        CONT_LEFT, gui._dockingLeftWidth);
    _dockingManager.setDockedContSize(
        CONT_RIGHT, gui._dockingRightWidth);
    _dockingManager.setDockedContSize(
        CONT_TOP, gui._dockingTopHeight);
    _dockingManager.setDockedContSize(
        CONT_BOTTOM, gui._dockingBottomHeight);

    QRect pos = gui._appPos;
    if (pos.width() > 100 && pos.height() > 100) {
        const QMargins frameMargins = windowHandle()
            ? windowHandle()->frameMargins() : QMargins();
        resize(qMax(100, pos.width() - frameMargins.left()
                               - frameMargins.right()),
               qMax(100, pos.height() - frameMargins.top()
                               - frameMargins.bottom()));
        move(pos.topLeft());
    }
    if (gui._isMaximized)
        showMaximized();
    if (!gui._windowState.isEmpty())
        _dockingManager.restoreState(gui._windowState);
    if (startupOptions.hasWindowPosition())
        move(startupOptions.windowX, startupOptions.windowY);
    if (startupOptions.alwaysOnTop)
        setWindowFlag(Qt::WindowStaysOnTopHint, true);
    if (startupOptions.noTabBar) {
        _mainDocTab->tabBar()->hide();
        _subDocTab->tabBar()->hide();
    }

    // 根据配置恢复会话或新建空白文档
    bool restored = false;
    if (!startupOptions.noSession && !startupOptions.openSession &&
        gui._rememberLastSession) {
        restoreSession();
        restored = (_mainDocTab->count() > 0 || _subDocTab->count() > 0);
    }
    if (!restored && !startupOptions.hasFiles() &&
        startupOptions.quoteType < 0)
        doNewBuffer(_mainDocTab);

    updateRecentFilesMenu();
    initActionStates();

    _fileWatcher = new QFileSystemWatcher(this);
    connect(_fileWatcher, &QFileSystemWatcher::fileChanged,
            this, &MainWindow::onWatchedFileChanged);
    connect(_fileWatcher, &QFileSystemWatcher::directoryChanged,
            this, &MainWindow::onWatchedDirectoryChanged);
    _externalFilePollTimer = new QTimer(this);
    _externalFilePollTimer->setInterval(2500);
    connect(_externalFilePollTimer, &QTimer::timeout,
            this, &MainWindow::pollWatchedFiles);
    _externalFilePollTimer->start();

    // 备份定时器：与原版 snapshot 模式一致，由 config.xml 控制间隔和开关
    _backupTimer = new QTimer(this);
    _backupTimer->setInterval(qMax(1000, gui._snapshotBackupTiming));
    connect(_backupTimer, &QTimer::timeout, this, &MainWindow::onBackupTimer);
    if (gui._isSnapshotMode)
        _backupTimer->start();

    applyCommandLineInvocation(startupOptions);
#ifdef Q_OS_WIN
    if (_win32PluginManager)
        _win32PluginManager->notifyReady();
#endif
    if (_pluginManager)
        _pluginManager->notifyReady();
}

MainWindow::~MainWindow()
{
    // PluginAdminModel is not a QObject and is normally released by the
    // dialog's destroyed handler.  Close it explicitly while MainWindow is
    // still fully alive so shutdown cannot leave the model behind.
    delete _pluginAdminDlg;
    _pluginAdminDlg = nullptr;
    if (_pluginAdminModel) {
        delete _pluginAdminModel;
        _pluginAdminModel = nullptr;
    }
#ifdef Q_OS_WIN
    delete _win32PluginManager;
    _win32PluginManager = nullptr;
#endif
    if (_recordingMacro) {
        _recordingMacro->endRecording();
        delete _recordingMacro;
        _recordingMacro = nullptr;
    }
    if (_pluginManager)
        _pluginManager->unloadAll();
    delete _pluginHostServices;
    _pluginHostServices = nullptr;
    for (Buffer* buffer : MainFileManager.buffers()) {
        if (buffer)
            buffer->releaseDocument();
    }
}

void MainWindow::applyCommandLineInvocation(
    const CommandLineOptions& options)
{
    if (!options.titleAddition.isEmpty()) {
        _titleAddition = options.titleAddition;
        updateWindowTitle(
            _activeDocTab ? _activeDocTab->currentBuffer() : nullptr);
    }

    if (options.openSession && options.paths.size() == 1) {
        if (NppParameters::getInstance().loadSession(options.paths.first())) {
            restoreSession();
        } else if (_mainDocTab->count() == 0) {
            doNewBuffer(_mainDocTab);
        }
        return;
    }

    if (options.openFoldersAsWorkspace) {
        for (const QString& path : options.paths) {
            if (!QFileInfo(path).isDir())
                continue;
            _fileBrowserPanel->setRootPath(path);
            _dockingManager.showDockableDlg(_fileBrowserPanel, true);
            _fileBrowserAction->setChecked(true);
        }
        return;
    }

    for (const QString& path : options.paths) {
        Buffer* buffer = nullptr;
        if (QFileInfo::exists(path)) {
            if (!doOpenFile(path, _activeDocTab))
                continue;
            buffer = MainFileManager.findBufferByPath(path);
        } else {
            buffer = doNewBuffer(_activeDocTab);
            if (!buffer)
                continue;
            buffer->setFilePath(path);
            if (ScintillaEditView* view = activateBufferView(buffer))
                view->setLexerForFile(path);
            _activeDocTab->updateTabTitle(buffer);
        }
        applyFileCommandLineState(buffer, options);
    }

    if (options.quoteType >= 0) {
        QString text = options.quote;
        if (options.quoteType == 2) {
            QFile file(options.quote);
            if (file.open(QFile::ReadOnly))
                text = QString::fromUtf8(file.readAll());
        }
        Buffer* buffer = doNewBuffer(_activeDocTab);
        ScintillaEditView* view = activateBufferView(buffer);
        if (view) {
            if (options.ghostTypingSpeed < 0) {
                view->setText(text);
            } else {
                const int interval =
                    options.ghostTypingSpeed == 1 ? 100 :
                    options.ghostTypingSpeed == 2 ? 20 : 0;
                QTimer* timer = new QTimer(view);
                timer->setInterval(interval);
                const QSharedPointer<QString> remaining(
                    new QString(text));
                connect(timer, &QTimer::timeout, view,
                        [view, timer, remaining]() {
                    if (remaining->isEmpty()) {
                        timer->stop();
                        timer->deleteLater();
                        return;
                    }
                    view->insert(remaining->left(1));
                    remaining->remove(0, 1);
                });
                timer->start();
            }
        }
    }

    if (options.quickPrint && !options.paths.isEmpty()) {
        QTimer::singleShot(0, this, [this]() {
            printDocumentNow();
            close();
        });
    }
    if (options.exportFunctionList && !options.paths.isEmpty()) {
        const QString sourcePath = options.paths.last();
        QTimer::singleShot(0, this, [this, sourcePath]() {
            if (_funcListPanel) {
                _funcListPanel->updateForView(currentActiveView());
                _funcListPanel->serialize(
                    sourcePath + QStringLiteral(".result.json"),
                    QFileInfo(sourcePath).fileName());
            }
            close();
        });
    }

    raise();
    activateWindow();
}
