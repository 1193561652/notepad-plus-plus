// MainWindow.cpp - 主窗口实现
// 移植自: v8.4.6:PowerEditor/src/

#include "MainWindow.h"
#include "NppCommandRegistry.h"
#include "ScintillaComponent/ScintillaEditView.h"
#include "ScintillaComponent/Buffer.h"
#include "WinControls/TabBar/DocTabView.h"
#include "MISC/FileManager.h"
#include "MISC/EncodingMapper.h"
#include "MISC/PlatformServices.h"
#include "MISC/TextFileCodec.h"
#include "MISC/ToolbarIconTheme.h"
#include "ScintillaComponent/FindReplaceDlg.h"
#include "ScintillaComponent/ScintillaTextSearch.h"
#include "ScintillaComponent/EditorMacro.h"
#include "WinControls/DockingWnd/FileBrowserPanel.h"
#include "WinControls/Preference/PreferenceDlg.h"
#include "WinControls/DockingWnd/DocumentMapPanel.h"
#include "WinControls/DockingWnd/FunctionListPanel.h"
#include "WinControls/ProjectPanel/ProjectPanel.h"
#include "WinControls/Grid/ShortcutMapper.h"
#include "MISC/PluginsManager/PluginManager.h"
#include "WinControls/PluginsAdmin/PluginAdminDialog.h"
#include "WinControls/PluginsAdmin/PluginAdminModel.h"
#include "MISC/PluginsManager/PluginCatalog.h"
#include "MISC/PluginsManager/PluginUpdatePlan.h"
#include "ScintillaComponent/Printer.h"
#ifdef Q_OS_WIN
#include "Win32PluginSystem/Win32PluginManager.h"
#endif

#include "Parameters.h"
#include "NativeLangSpeaker.h"
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
#include <algorithm>
#include <functional>

static EolMode toScintillaEol(TextEolMode mode,
                                             EolMode fallback)
{
    switch (mode) {
        case TextEolMode::Windows: return EolWindows;
        case TextEolMode::Mac: return EolMac;
        case TextEolMode::Unix: return EolUnix;
        case TextEolMode::Unknown: return fallback;
    }
    return fallback;
}

static TextEolMode fromScintillaEol(EolMode mode)
{
    switch (mode) {
        case EolWindows: return TextEolMode::Windows;
        case EolMac: return TextEolMode::Mac;
        case EolUnix: return TextEolMode::Unix;
    }
    return TextEolMode::Unknown;
}

static void captureBufferDocument(Buffer* buffer, ScintillaEditView* view)
{
    if (!buffer || !view)
        return;
    const qintptr document = static_cast<qintptr>(view->document());
    view->SendScintilla(SCI_ADDREFDOCUMENT, 0, document);
    buffer->captureDocument(document, view,
        [view](qintptr pointer) {
            view->SendScintilla(SCI_RELEASEDOCUMENT, 0, pointer);
        });
}

static int sessionEncodingForBuffer(const Buffer* buf)
{
    if (!buf)
        return -1;

    const QString encoding = TextFileCodec::normalizedEncoding(
        buf->getEncoding());
    if (encoding == "UTF-8" || encoding == "UTF-16LE" ||
        encoding == "UTF-16BE") {
        return -1;
    }
    return EncodingMapper::codePageForName(encoding);
}

static TextDecodingOptions decodingOptionsForPath(const QString& filePath)
{
    const NppGUI& gui = NppParameters::getInstance().getNppGUI();
    TextDecodingOptions options;
    options.filePath = filePath;
    options.detectEncoding = gui._detectEncoding;
    options.openAnsiAsUtf8 = gui._openAnsiAsUtf8;
    return options;
}

static bool confirmHugeFileOpen(QWidget* parent, qint64 fileSize)
{
    if (!Buffer::requiresHugeFileConfirmation(fileSize))
        return true;
#if QT_POINTER_SIZE == 4
    QMessageBox::warning(
        parent, QObject::tr("File size problem"),
        QObject::tr("File is too big to be opened by Notepad++"));
    return false;
#else
    return QMessageBox::question(
        parent, QObject::tr("Opening huge file warning"),
        QObject::tr(
            "Opening a huge file of 2GB+ could take several minutes.\n"
            "Do you want to open it?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ==
        QMessageBox::Yes;
#endif
}

static void toSessionFileTime(const QDateTime& dateTime,
                              qint64* lowPart, qint64* highPart)
{
    if (!lowPart || !highPart || !dateTime.isValid()) {
        if (lowPart) *lowPart = 0;
        if (highPart) *highPart = 0;
        return;
    }

    static const qint64 unixEpochInFileTimeMsecs = 11644473600000LL;
    const quint64 ticks = static_cast<quint64>(
        dateTime.toMSecsSinceEpoch() + unixEpochInFileTimeMsecs) * 10000ULL;
    *lowPart = static_cast<qint32>(ticks & 0xFFFFFFFFULL);
    *highPart = static_cast<qint32>(ticks >> 32);
}

static QDateTime fromSessionFileTime(qint64 lowPart, qint64 highPart)
{
    // Accept session files emitted by earlier Qt-port builds, which stored
    // Unix seconds in the low field and left the high field at zero.
    if (highPart == 0 && lowPart > 0 && lowPart < 0x7FFFFFFFLL)
        return QDateTime::fromSecsSinceEpoch(lowPart);

    const quint64 low = static_cast<quint32>(
        static_cast<qint32>(lowPart));
    const quint64 high = static_cast<quint32>(
        static_cast<qint32>(highPart));
    const quint64 ticks = (high << 32) | low;
    if (ticks == 0)
        return QDateTime();

    static const qint64 unixEpochInFileTimeMsecs = 11644473600000LL;
    const qint64 unixMsecs =
        static_cast<qint64>(ticks / 10000ULL) - unixEpochInFileTimeMsecs;
    return QDateTime::fromMSecsSinceEpoch(unixMsecs);
}

static QString configuredDialogDirectory(const Buffer* currentBuffer)
{
    const NppGUI& gui = NppParameters::getInstance().getNppGUI();
    if (gui._openSaveDir == 2 && !gui._defaultDirPath.isEmpty())
        return gui._defaultDirPath;
    if (gui._openSaveDir == 0 && currentBuffer && !currentBuffer->isUntitled())
        return QFileInfo(currentBuffer->getFullPath()).absolutePath();
    return QString();
}

static bool findInViewFromPosition(ScintillaEditView* view,
                                   const QString& searchText,
                                   const FindOption& option,
                                   qintptr position)
{
    view->setCurrentPositionNpp(position);
    return view->findFirst(
        searchText, option._isRegex, option._isMatchCase,
        option._isWholeWord, false, true);
}

static int replacePlainText(QString& text, const QString& searchText,
                            const QString& replacement, const FindOption& opt)
{
    return ScintillaTextSearch::replaceAll(
        text, searchText, replacement, opt._isRegex, opt._isMatchCase,
        opt._isWholeWord, opt._dotMatchesNewline);
}

static QList<FindAllResult> findAllInPlainText(const QString& text,
                                               const QString& searchText,
                                               const FindOption& opt,
                                               const QString& filePath,
                                               const QString& sourceName)
{
    QList<FindAllResult> results;
    if (searchText.isEmpty())
        return results;

    const QList<ScintillaTextMatch> matches = ScintillaTextSearch::findAll(
        text, searchText, opt._isRegex, opt._isMatchCase,
        opt._isWholeWord, opt._dotMatchesNewline);
    for (const ScintillaTextMatch& match : matches) {
        FindAllResult r;
        r.lineNo = match.line;
        r.matchStart = match.byteStart;
        r.matchLen = match.byteLength;
        r.lineMatchStart = match.byteColumn;
        r.lineText = match.lineText;
        r.filePath = filePath;
        r.sourceName = sourceName;
        results.append(r);
    }
    return results;
}

MainWindow::MainWindow(const CommandLineOptions& startupOptions, QWidget *parent)
    : QMainWindow(parent), _startupOptions(startupOptions),
      _titleAddition(startupOptions.titleAddition),
      _suppressSessionPersistence(startupOptions.noSession)
{
    setWindowTitle("Notepad++");
    setWindowIcon(QIcon(":/icons/npp.ico"));
    resize(900, 600);

    setupTabViews();
#ifdef Q_OS_WIN
    _win32PluginManager = new Win32PluginManager(
        this, _mainDocTab->editor(), _subDocTab->editor());
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

    QRect pos = gui._appPos;
    if (pos.width() > 100 && pos.height() > 100)
        setGeometry(pos);
    if (gui._isMaximized)
        showMaximized();
    if (!gui._windowState.isEmpty())
        restoreState(gui._windowState);
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
}

MainWindow::~MainWindow()
{
    if (_recordingMacro) {
        _recordingMacro->endRecording();
        delete _recordingMacro;
        _recordingMacro = nullptr;
    }
    if (_pluginManager)
        _pluginManager->unloadAll(this);
    for (Buffer* buffer : MainFileManager.buffers()) {
        if (buffer)
            buffer->releaseDocument();
    }
}

void MainWindow::applyFileCommandLineState(
    Buffer* buffer, const CommandLineOptions& options)
{
    if (!buffer)
        return;
    ScintillaEditView* view = activateBufferView(buffer);
    if (!view)
        return;

    if (!options.userDefinedLanguage.isEmpty()) {
        const UserLangDesc* language =
            NppParameters::getInstance().getUserLangByName(
                options.userDefinedLanguage);
        if (language)
            view->setUserDefinedLanguage(*language);
    } else if (!options.language.isEmpty()) {
        view->setLexerByName(options.language);
    }

    if (options.position >= 0) {
        qintptr position = qMin<qintptr>(
            options.position, view->documentLengthNpp());
        if (position > 0) {
            const qintptr before = view->SendScintillaNpp(
                SCI_POSITIONBEFORE,
                static_cast<quintptr>(position));
            position = view->SendScintillaNpp(
                SCI_POSITIONAFTER,
                static_cast<quintptr>(before));
        }
        view->setCurrentPositionNpp(position);
    } else if (options.line >= 0) {
        const int line = qMax<qint64>(0, options.line - 1);
        if (options.column >= 0) {
            const qintptr position = view->SendScintillaNpp(
                SCI_FINDCOLUMN,
                static_cast<quintptr>(line),
                qMax<qint64>(0, options.column - 1));
            view->setCurrentPositionNpp(position);
        } else {
            view->SendScintilla(
                SCI_GOTOLINE,
                static_cast<unsigned long>(line));
        }
    }
    view->SendScintilla(
        SCI_SCROLLCARET);

    if (options.readOnly) {
        buffer->setCommandLineReadOnly(true);
        buffer->setReadOnly(true);
        for (DocTabView* tab : {_mainDocTab, _subDocTab})
            if (tab->currentBuffer() == buffer)
                tab->editor()->setReadOnly(true);
    }
    if (options.monitorFiles) {
        buffer->setMonitoring(true);
        buffer->setReadOnly(true);
        for (DocTabView* tab : {_mainDocTab, _subDocTab})
            if (tab->currentBuffer() == buffer)
                tab->editor()->setReadOnly(true);
        watchBufferFile(buffer);
    }
    updateLangStatus();
    updateStatusBar();
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
            _fileBrowserDock->show();
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

// ─── 视图初始化 ───────────────────────────────────────────────────────────────

void MainWindow::setupTabViews()
{
    _splitter    = new QSplitter(Qt::Horizontal, this);
    _mainDocTab  = new DocTabView(_splitter);
    _subDocTab   = new DocTabView(_splitter);
    _mainDocTab->setObjectName(QStringLiteral("MainDocTab"));
    _subDocTab->setObjectName(QStringLiteral("SubDocTab"));
    _activeDocTab = _mainDocTab;

    const int borderWidth =
        NppParameters::getInstance().getSVP()._borderWidth;
    _mainDocTab->setEditorBorderWidth(borderWidth);
    _subDocTab->setEditorBorderWidth(borderWidth);

    _splitter->addWidget(_mainDocTab);
    _splitter->addWidget(_subDocTab);
    _splitter->setChildrenCollapsible(false);

    // 副视图初始隐藏
    _subDocTab->hide();

    setCentralWidget(_splitter);

    connectTabView(_mainDocTab);
    connectTabView(_subDocTab);
    connectModificationSignal(_mainDocTab->editor(), nullptr);
    connectModificationSignal(_subDocTab->editor(), nullptr);
}

void MainWindow::connectTabView(DocTabView* tab)
{
    connect(tab, &DocTabView::bufferCloseRequested,
            this, &MainWindow::onBufferCloseRequested);
    connect(tab, &DocTabView::currentChanged,
            this, &MainWindow::onCurrentTabChanged);
    connect(tab, &DocTabView::newTabRequested,
            this, [this, tab]() { doNewBuffer(tab); });
}

// ─── 视图辅助 ─────────────────────────────────────────────────────────────────

DocTabView* MainWindow::otherTab() const
{
    return (_activeDocTab == _mainDocTab) ? _subDocTab : _mainDocTab;
}

void MainWindow::setActiveTab(DocTabView* tab)
{
    if (_activeDocTab == tab) return;
    _activeDocTab = tab;
    Buffer* buf = _activeDocTab->currentBuffer();
    updateWindowTitle(buf);
    if (buf)
        updateFindReplaceView();
    updateStatusBar();
    syncDocumentMap();
    if (_funcListDock && _funcListDock->isVisible())
        _funcListPanel->updateForView(currentActiveView());
}

void MainWindow::showSubView()
{
    _subDocTab->show();
    // 平均分配宽度/高度
    int total = (_splitter->orientation() == Qt::Horizontal)
                ? _splitter->width() : _splitter->height();
    _splitter->setSizes({total / 2, total / 2});
    _splitViewAction->setChecked(true);
}

void MainWindow::hideSubView()
{
    if (_activeDocTab == _subDocTab)
        setActiveTab(_mainDocTab);
    _subDocTab->hide();
    _splitViewAction->setChecked(false);
}

void MainWindow::setBufferEolMode(Buffer* buffer, int mode, bool convertText)
{
    if (!buffer || buffer->isReadOnly())
        return;

    const EolMode eolMode =
        static_cast<EolMode>(mode);
    ScintillaEditView* sourceView = activateBufferView(buffer);

    if (convertText && sourceView)
        sourceView->convertEols(eolMode);
    if (sourceView)
        sourceView->setEolMode(eolMode);
    buffer->setEolMode(fromScintillaEol(eolMode));
    updateStatusBar();
    updateActionStates();
}

void MainWindow::setupFileBrowser()
{
    _fileBrowserPanel = new FileBrowserPanel(this);

    _fileBrowserDock = new QDockWidget(tr("Folder as Workspace"), this);
    _fileBrowserDock->setObjectName("FileBrowserDock");
    _fileBrowserDock->setWidget(_fileBrowserPanel);
    _fileBrowserDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, _fileBrowserDock);
    _fileBrowserDock->hide();

    connect(_fileBrowserPanel, &FileBrowserPanel::fileActivated,
            this, [this](const QString& path) {
                doOpenFile(path, _activeDocTab);
                activateWindow();
            });

    connect(_fileBrowserDock, &QDockWidget::visibilityChanged,
            this, [this](bool visible) {
                if (_fileBrowserAction)
                    _fileBrowserAction->setChecked(visible);
            });
}

void MainWindow::toggleFileBrowser()
{
    if (_fileBrowserDock->isVisible())
        _fileBrowserDock->hide();
    else
        _fileBrowserDock->show();
}

void MainWindow::updateFindReplaceView()
{
    if (!_findReplaceDlg) return;
    // 使用当前标签页实际显示的视图（可能是克隆视图）
    ScintillaEditView* view = _activeDocTab->editor();
    if (view)
        _findReplaceDlg->setCurrentView(view);
}

// ─── 核心文件操作 ────────────────────────────────────────────────────────────

ScintillaEditView* MainWindow::currentActiveView() const
{
    return _activeDocTab ? _activeDocTab->editor() : nullptr;
}

ScintillaEditView* MainWindow::activateBufferView(
    Buffer* buffer, DocTabView* preferredTab)
{
    if (!buffer)
        return nullptr;

    DocTabView* tab = nullptr;
    if (preferredTab && preferredTab->indexOfBuffer(buffer) >= 0)
        tab = preferredTab;
    else if (_activeDocTab && _activeDocTab->indexOfBuffer(buffer) >= 0)
        tab = _activeDocTab;
    else if (_mainDocTab->indexOfBuffer(buffer) >= 0)
        tab = _mainDocTab;
    else if (_subDocTab->indexOfBuffer(buffer) >= 0)
        tab = _subDocTab;
    if (!tab)
        return nullptr;

    tab->activateBuffer(buffer);
    ScintillaEditView* view = tab->editor();
    return view && static_cast<qintptr>(view->document()) == buffer->document()
        ? view : nullptr;
}

void MainWindow::updateStatusBar()
{
    ScintillaEditView* view = currentActiveView();
    if (!view) {
        _posLabel->setText("Ln : 1    Col : 1    Pos : 1");
        _docSizeLabel->setText("length : 0    lines : 1");
        return;
    }

    // ── STATUSBAR_DOC_SIZE：文件长度 + 行数 ──────────────────────────────────
    // 对应原版 "length : %s    lines : %s"（以千位分隔符格式化）
    const qintptr docLen = view->documentLengthNpp();
    int  nLines  = (int) view->SendScintilla(SCI_GETLINECOUNT);
    _docSizeLabel->setText(
        QString("length : %1    lines : %2")
            .arg(QLocale().toString((qlonglong)docLen))
            .arg(QLocale().toString((qlonglong)nLines)));

    // ── STATUSBAR_CUR_POS：行/列 + 位置或选区 ───────────────────────────────
    // 对应原版 "Ln : %s    Col : %s    %s"
    int line, col;
    view->getCursorPosition(&line, &col);
    QString posPart;
    const qintptr selStart =
        view->SendScintillaNpp(SCI_GETSELECTIONSTART);
    const qintptr selEnd =
        view->SendScintillaNpp(SCI_GETSELECTIONEND);
    if (selStart == selEnd) {
        // 无选区：显示字节位置（Pos，1-based）
        const qintptr pos =
            view->SendScintillaNpp(SCI_GETCURRENTPOS);
        posPart = QString("Pos : %1").arg(QLocale().toString((qlonglong)(pos + 1)));
    } else {
        // 有选区：显示选中字符数 | 选中行数
        const qintptr selChars = selEnd - selStart;
        const int selLineStart = static_cast<int>(view->SendScintillaNpp(
            SCI_LINEFROMPOSITION,
            static_cast<quintptr>(selStart)));
        const int selLineEnd = static_cast<int>(view->SendScintillaNpp(
            SCI_LINEFROMPOSITION,
            static_cast<quintptr>(selEnd)));
        int selLines = selLineEnd - selLineStart + 1;
        posPart = QString("Sel : %1 | %2")
            .arg(QLocale().toString((qlonglong)selChars))
            .arg(QLocale().toString(selLines));
    }
    _posLabel->setText(
        QString("Ln : %1    Col : %2    %3")
            .arg(line + 1).arg(col + 1).arg(posPart));

    // ── STATUSBAR_EOF_FORMAT：换行符类型 ─────────────────────────────────────
    // 对应原版显示名称（CR LF / LF / CR）
    switch (view->eolMode()) {
        case EolWindows: _eolLabel->setText("Windows (CR LF)"); break;
        case EolUnix:    _eolLabel->setText("Unix (LF)");       break;
        case EolMac:     _eolLabel->setText("Macintosh (CR)");  break;
    }

    // ── STATUSBAR_UNICODE_TYPE：编码 ─────────────────────────────────────────
    Buffer* curBuf = _activeDocTab->currentBuffer();
    QString enc = curBuf ? curBuf->getEncoding() : "UTF-8";
    if (curBuf && curBuf->hasBom()) enc += " BOM";
    _encodingLabel->setText(enc);

    // ── STATUSBAR_TYPING_MODE：INS / OVR ─────────────────────────────────────
    bool overtype = view->SendScintilla(SCI_GETOVERTYPE);
    _insertLabel->setText(overtype ? "OVR" : "INS");
}

void MainWindow::updateLangStatus()
{
    // 对应原版 setLangStatus()：更新状态栏左侧的语言类型（STATUSBAR_DOC_TYPE）
    ScintillaEditView* view = currentActiveView();
    if (!view || !_docTypeLabel) return;
    const QString language = view->lexerLanguage();
    _docTypeLabel->setText(language.isEmpty() ? tr("Normal Text") : language);
}

void MainWindow::connectModificationSignal(ScintillaEditView* view, Buffer* buf)
{
    connect(view, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
    connect(view, SIGNAL(cursorPositionChanged(int,int)),
            this, SLOT(onCursorPositionChanged(int,int)));
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, &QWidget::customContextMenuRequested, this,
            [this, view](const QPoint& position) {
        showEditorContextMenu(view, position);
    });
    Q_UNUSED(buf)
}

void MainWindow::showEditorContextMenu(ScintillaEditView* view,
                                       const QPoint& position)
{
    if (!view)
        return;
    QFile file(NppParameters::getInstance().getUserPath() + "/contextMenu.xml");
    if (!file.open(QFile::ReadOnly | QFile::Text))
        return;

    auto normalized = [](QString text) {
        text.remove('&');
        text.remove(QChar(0x2026));
        text.replace("...", "");
        return text.trimmed().toLower();
    };
    QMap<QString, QAction*> actionsByName;
    for (QAction* action : findChildren<QAction*>()) {
        if (!action->isSeparator() && !action->text().isEmpty())
            actionsByName.insert(normalized(action->text()), action);
    }
    const QMap<QString, QString> aliases = {
        {"delete", "deleteSelectionAction"},
        {"select all", "selectAllAction"},
        {"begin/end select", "beginEndSelectAction"},
        {"uppercase", "toUpperCaseAction"},
        {"lowercase", "toLowerCaseAction"},
        {"toggle single line comment", "toggleCommentAction"},
        {"block comment", "blockCommentAction"},
        {"block uncomment", "blockUncommentAction"},
        {"search on internet", "searchOnInternetAction"}
    };

    QMenu menu(view);
    QMap<QString, QMenu*> folders;
    QXmlStreamReader xml(&file);
    bool inContextMenu = false;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() &&
            xml.name().compare(QStringLiteral("ScintillaContextMenu"),
                               Qt::CaseInsensitive) == 0) {
            inContextMenu = true;
            continue;
        }
        if (xml.isEndElement() &&
            xml.name().compare(QStringLiteral("ScintillaContextMenu"),
                               Qt::CaseInsensitive) == 0) {
            break;
        }
        if (!inContextMenu || !xml.isStartElement() ||
            xml.name().compare(QStringLiteral("Item"), Qt::CaseInsensitive) != 0)
            continue;

        const QXmlStreamAttributes attrs = xml.attributes();
        if (!attrs.value("PluginEntryName").isEmpty())
            continue;
        const QString id = attrs.value("id").toString();
        const QString folderName = attrs.value("FolderName").toString();
        QMenu* target = &menu;
        if (!folderName.isEmpty()) {
            target = folders.value(folderName, nullptr);
            if (!target) {
                target = menu.addMenu(folderName);
                folders.insert(folderName, target);
            }
        }
        if (id == "0") {
            target->addSeparator();
            continue;
        }

        const QString itemName = attrs.value("MenuItemName").toString();
        if (itemName.isEmpty())
            continue;
        QAction* source = actionsByName.value(normalized(itemName), nullptr);
        if (!source && aliases.contains(normalized(itemName)))
            source = findChild<QAction*>(aliases.value(normalized(itemName)));
        if (!source)
            continue;
        QAction* entry = target->addAction(
            attrs.value("ItemNameAs").isEmpty()
                ? source->text()
                : attrs.value("ItemNameAs").toString());
        entry->setEnabled(source->isEnabled());
        entry->setCheckable(source->isCheckable());
        entry->setChecked(source->isChecked());
        connect(entry, &QAction::triggered, source, &QAction::trigger);
    }
    file.close();
    if (!menu.isEmpty())
        menu.exec(view->mapToGlobal(position));
}

Buffer* MainWindow::doNewBuffer(DocTabView* targetTab)
{
    if (!targetTab) targetTab = _activeDocTab;

    Buffer* buf = MainFileManager.newBuffer();
    ScintillaEditView* view = targetTab->editor();
    view->createStandardDocument();
    buf->setView(view);
    captureBufferDocument(buf, view);
    applyPreferencesToView(view);

    const NppGUI& gui = NppParameters::getInstance().getNppGUI();
    switch (gui._newDocDefaultFormat) {
        case EolType_windows:
            view->setEolMode(EolWindows);
            buf->setEolMode(TextEolMode::Windows);
            break;
        case EolType_macos:
            view->setEolMode(EolMac);
            buf->setEolMode(TextEolMode::Mac);
            break;
        case EolType_unix:
        default:
            view->setEolMode(EolUnix);
            buf->setEolMode(TextEolMode::Unix);
            break;
    }
    switch (gui._newDocDefaultEncoding) {
        case uniUTF8:
            buf->setEncoding("UTF-8");
            buf->setHasBom(true);
            buf->setUsesEncodingCookie(false);
            break;
        case uniCookie:
            buf->setEncoding("UTF-8");
            buf->setHasBom(false);
            buf->setUsesEncodingCookie(true);
            break;
        case uni16BE:
            buf->setEncoding("UTF-16BE");
            buf->setHasBom(true);
            buf->setUsesEncodingCookie(false);
            break;
        case uni16LE:
            buf->setEncoding("UTF-16LE");
            buf->setHasBom(true);
            buf->setUsesEncodingCookie(false);
            break;
        case uni8Bit:
        default:
            buf->setEncoding(QTextCodec::codecForLocale()->name());
            buf->setHasBom(false);
            buf->setUsesEncodingCookie(false);
            break;
    }

    targetTab->addBuffer(buf);
    if (targetTab == _activeDocTab)
        updateWindowTitle(buf);
    updateActionStates();
    return buf;
}

bool MainWindow::doOpenFile(const QString& filePath, DocTabView* targetTab,
                            const QString& forcedEncoding)
{
    if (!targetTab) targetTab = _activeDocTab;

    // 已打开则切换（在任意视图中查找）
    Buffer* existing = MainFileManager.findBufferByPath(filePath);
    if (existing) {
        // 优先在当前视图激活，若不存在则在另一视图
        if (targetTab->indexOfBuffer(existing) != -1)
            targetTab->activateBuffer(existing);
        else if (otherTab()->indexOfBuffer(existing) != -1)
            otherTab()->activateBuffer(existing);
        return true;
    }

    const qint64 fileSize = QFileInfo(filePath).size();
    if (!confirmHugeFileOpen(this, fileSize))
        return false;

    Buffer* buf = MainFileManager.loadBuffer(filePath);
    if (!buf) {
        QMessageBox::warning(this, tr("Open Failed"),
            tr("Cannot open file:\n%1").arg(filePath));
        return false;
    }

    ScintillaEditView* view = targetTab->editor();
    view->createStandardDocument();
    buf->setView(view);

    QString loadError;
    if (!MainFileManager.loadBufferContent(
            buf, view, decodingOptionsForPath(filePath), forcedEncoding,
            &loadError)) {
        MainFileManager.closeBuffer(buf);
        QMessageBox::warning(this, tr("Open Failed"),
            tr("Cannot open file:\n%1\n\n%2").arg(filePath, loadError));
        return false;
    }

    const NppGUI& gui = NppParameters::getInstance().getNppGUI();
    captureBufferDocument(buf, view);
    EolMode fallbackEol = EolUnix;
    if (gui._newDocDefaultFormat == EolType_windows)
        fallbackEol = EolWindows;
    else if (gui._newDocDefaultFormat == EolType_macos)
        fallbackEol = EolMac;

    buf->setEolMode(buf->getEolMode() == TextEolMode::Unknown
        ? fromScintillaEol(fallbackEol) : buf->getEolMode());
    buf->setReadOnly(!QFileInfo(filePath).isWritable());
    view->setEolMode(toScintillaEol(buf->getEolMode(), fallbackEol));
    view->setReadOnly(buf->isReadOnly());
    view->setLexerForFile(filePath);
    applyPreferencesToView(view);

    targetTab->addBuffer(buf);
    if (targetTab == _activeDocTab)
        updateWindowTitle(buf);
    watchBufferFile(buf);
    addToRecentFiles(filePath);
    updateLangStatus();   // 对应原版 setLangStatus()，文件打开后更新语言类型
    updateActionStates();
    return true;
}

bool MainWindow::doSave(Buffer* buf, const QString& filePath)
{
    ScintillaEditView* activeBufferView = activateBufferView(buf);
    if (!activeBufferView)
        return false;

    const NppGUI& gui = NppParameters::getInstance().getNppGUI();
    const bool savingExistingFile =
        buf && !buf->isUntitled() && QFileInfo::exists(buf->getFullPath()) &&
        QFileInfo(buf->getFullPath()).absoluteFilePath() ==
            QFileInfo(filePath).absoluteFilePath();
    if (savingExistingFile && !buf->isLargeFile() && gui._backup != 0) {
        QString backupPath;
        QString backupError;
        const QString timestamp =
            QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HHmmss"));
        if (!FileManager::backupFileBeforeSave(
                filePath, gui._backup, gui._useBackupDir, gui._backupDir,
                timestamp, &backupPath, &backupError)) {
            const QMessageBox::StandardButton answer = QMessageBox::warning(
                this, tr("File Backup Failed"),
                tr("The previous version could not be saved to:\n%1\n\n%2\n\n"
                   "Save the current file anyway?")
                    .arg(QDir::toNativeSeparators(backupPath), backupError),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes)
                return false;
        }
    }

    QString canonical = QFileInfo(filePath).canonicalFilePath();
    if (!canonical.isEmpty())
        _savingPaths.insert(canonical);

    QString errorMessage;
    if (!MainFileManager.saveBuffer(buf, filePath, &errorMessage)) {
        if (!canonical.isEmpty())
            _savingPaths.remove(canonical);
        QMessageBox::warning(this, tr("Save Failed"),
            tr("Cannot save file:\n%1\n\n%2").arg(filePath, errorMessage));
        return false;
    }
    if (!canonical.isEmpty())
        _savingPaths.remove(canonical);

    // 保存成功后删除对应备份文件（与原版一致）
    buf->clearBackupFile();

    buf->setReadOnly(!QFileInfo(filePath).isWritable());
    activeBufferView->SendScintilla(SCI_SETSAVEPOINT);
    activeBufferView->setLexerForFile(filePath);
    for (DocTabView* tab : {_mainDocTab, _subDocTab}) {
        if (tab->currentBuffer() == buf)
            tab->editor()->setReadOnly(buf->isReadOnly());
    }

    // 两个视图都可能显示同一文档，都更新标题
    _mainDocTab->updateTabTitle(buf);
    _subDocTab->updateTabTitle(buf);
    watchBufferFile(buf);
    if (!buf->isUntitled())
        addToRecentFiles(filePath);
    updateWindowTitle(buf);
    statusBar()->showMessage(tr("File saved"), 2000);
    if (_funcListDock && _funcListDock->isVisible())
        _funcListPanel->refresh();
    return true;
}

bool MainWindow::doSaveAs(Buffer* buf)
{
    QString filePath = QFileDialog::getSaveFileName(this,
        tr("Save As"), buf->isUntitled()
            ? configuredDialogDirectory(buf) : buf->getFullPath(),
        tr("All Files (*);;Text Files (*.txt)"));

    if (filePath.isEmpty())
        return false;

    return doSave(buf, filePath);
}

bool MainWindow::closeBufferList(const QList<Buffer*>& buffers)
{
    QList<Buffer*> unique;
    for (Buffer* buffer : buffers)
        if (buffer && !unique.contains(buffer)) unique.append(buffer);

    QList<Buffer*> discarded;
    for (Buffer* buffer : unique) {
        if (!checkBufferSave(buffer)) {
            for (Buffer* restored : discarded) {
                restored->setDirty(true);
                _mainDocTab->updateTabTitle(restored);
                _subDocTab->updateTabTitle(restored);
            }
            return false;
        }
        // checkBufferSave leaves a discarded document dirty. Marking it clean
        // avoids a second prompt in the regular close path.
        if (buffer->isDirty()) {
            discarded.append(buffer);
            buffer->setDirty(false);
        }
    }
    auto removeFromTab = [](DocTabView* tab, Buffer* buffer) {
        const int index = tab ? tab->indexOfBuffer(buffer) : -1;
        if (index < 0)
            return;
        tab->removeTab(index);
    };

    for (Buffer* buffer : unique) {
        if (!buffer->isUntitled())
            rememberClosedFile(buffer->getFullPath());
        removeFromTab(_mainDocTab, buffer);
        removeFromTab(_subDocTab, buffer);
        unwatchBufferFile(buffer);
        buffer->clearBackupFile();
        MainFileManager.closeBuffer(buffer);
    }
    if (_mainDocTab->count() == 0)
        doNewBuffer(_mainDocTab);
    if (_subDocTab->count() == 0)
        hideSubView();
    updateActionStates();
    return true;
}

bool MainWindow::checkBufferSave(Buffer* buf)
{
    if (!buf->isDirty())
        return true;

    QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("Unsaved Changes"),
        tr("File \"%1\" has been modified.\nDo you want to save it?")
            .arg(buf->getFileName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (reply == QMessageBox::Save)
        return buf->isUntitled() ? doSaveAs(buf) : doSave(buf, buf->getFullPath());
    else if (reply == QMessageBox::Cancel)
        return false;

    // Discard：用户放弃修改，删除备份文件
    buf->clearBackupFile();
    return true;
}

void MainWindow::updateWindowTitle(Buffer* buf)
{
    // 对应原版 Notepad_plus::setTitle()
    // 格式：[*]filename - Notepad++
    //   * 前缀（原版行为），而非后缀
    //   文件名在前，应用名在后，与原版一致
    if (!buf) {
        setWindowTitle(_titleAddition.isEmpty()
            ? QStringLiteral("Notepad++")
            : QStringLiteral("Notepad++ ") + _titleAddition);
        return;
    }
    QString name = buf->isUntitled() ? buf->getFileName() : buf->getFullPath();
    QString title = (buf->isDirty() ? "*" : "") + name + " - Notepad++";
    if (!_titleAddition.isEmpty())
        title += QLatin1Char(' ') + _titleAddition;
    setWindowTitle(title);
}

// ─── 槽函数 ──────────────────────────────────────────────────────────────────

void MainWindow::newFile()
{
    doNewBuffer(_activeDocTab);
}

void MainWindow::openFile()
{
    QString filePath = QFileDialog::getOpenFileName(this,
        tr("Open File"), configuredDialogDirectory(
            _activeDocTab ? _activeDocTab->currentBuffer() : nullptr),
        tr("All Files (*);;Text Files (*.txt)"));

    if (!filePath.isEmpty())
        doOpenFile(filePath, _activeDocTab);
}

void MainWindow::saveFile()
{
    Buffer* buf = _activeDocTab->currentBuffer();
    if (!buf) return;
    buf->isUntitled() ? doSaveAs(buf) : doSave(buf, buf->getFullPath());
}

void MainWindow::saveFileAs()
{
    Buffer* buf = _activeDocTab->currentBuffer();
    if (buf) doSaveAs(buf);
}

void MainWindow::saveAllFiles()
{
    DocTabView* const originalActiveTab = _activeDocTab;
    const int originalMainIndex = _mainDocTab->currentIndex();
    const int originalSubIndex = _subDocTab->currentIndex();
    auto saveTab = [this](DocTabView* tab) {
        for (int i = 0; i < tab->count(); ++i) {
            Buffer* buf = tab->bufferAt(i);
            if (!buf || !buf->isDirty()) continue;
            buf->isUntitled() ? doSaveAs(buf) : doSave(buf, buf->getFullPath());
        }
    };
    saveTab(_mainDocTab);
    saveTab(_subDocTab);
    if (originalMainIndex >= 0 && originalMainIndex < _mainDocTab->count())
        _mainDocTab->setCurrentIndex(originalMainIndex);
    if (originalSubIndex >= 0 && originalSubIndex < _subDocTab->count())
        _subDocTab->setCurrentIndex(originalSubIndex);
    setActiveTab(originalActiveTab);
}

void MainWindow::closeFile()
{
    Buffer* buf = _activeDocTab->currentBuffer();
    if (buf) onBufferCloseRequested(buf);
}

void MainWindow::closeAllFiles()
{
    // 收集所有 buffer 列表（去重，避免克隆/双视图文档被重复关闭）
    QList<Buffer*> bufs;
    for (int i = _mainDocTab->count() - 1; i >= 0; --i)
        if (auto* b = _mainDocTab->bufferAt(i)) bufs << b;
    for (int i = _subDocTab->count() - 1; i >= 0; --i) {
        if (auto* b = _subDocTab->bufferAt(i)) {
            if (!bufs.contains(b))
                bufs << b;
        }
    }
    closeBufferList(bufs);
}

void MainWindow::reloadFromDisk()
{
    Buffer* buf = _activeDocTab->currentBuffer();
    if (!buf || buf->isUntitled()) return;
    QString path = buf->getFullPath();

    if (buf->isDirty()) {
        auto reply = QMessageBox::question(this, tr("Reload"),
            tr("File \"%1\" has unsaved changes. Reload from disk?").arg(buf->getFileName()),
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) return;
    }

    ScintillaEditView* view = activateBufferView(buf, _activeDocTab);
    if (!view) return;
    const qint64 fileSize = QFileInfo(path).size();
    if (!confirmHugeFileOpen(this, fileSize))
        return;
    const bool wasLarge = buf->isLargeFile();
    const bool isLarge = Buffer::isLargeFileSize(fileSize);
    buf->setSourceFileSize(fileSize);
    buf->setLargeFile(isLarge);
    if (isLarge)
        buf->clearBackupFile();
    if (wasLarge && !isLarge)
        view->createStandardDocument();

    QString loadError;
    if (!MainFileManager.loadBufferContent(
            buf, view, decodingOptionsForPath(path), QString(), &loadError)) {
        QMessageBox::warning(this, tr("Reload"),
            tr("Cannot reload file:\n%1\n\n%2").arg(path, loadError));
        return;
    }

    buf->setEolMode(buf->getEolMode() == TextEolMode::Unknown
        ? fromScintillaEol(view->eolMode()) : buf->getEolMode());
    buf->setReadOnly(!QFileInfo(path).isWritable());
    buf->setLastKnownModificationTime(QFileInfo(path).lastModified());
    if (buf->document() != static_cast<qintptr>(view->document())) {
        buf->releaseDocument();
        captureBufferDocument(buf, view);
    }
    for (DocTabView* tab : {_mainDocTab, _subDocTab}) {
        if (tab->currentBuffer() != buf)
            continue;
        ScintillaEditView* bufferView = tab->editor();
        if (bufferView != view)
            bufferView->setDocument(buf->document());
        bufferView->setLargeFileMode(isLarge);
        bufferView->setEolMode(toScintillaEol(
            buf->getEolMode(), bufferView->eolMode()));
        bufferView->setLexerForFile(path);
        applyPreferencesToView(bufferView);
        bufferView->setReadOnly(buf->isReadOnly());
    }
    buf->setDirty(false);
    _mainDocTab->updateTabTitle(buf);
    _subDocTab->updateTabTitle(buf);
    updateWindowTitle(buf);
    watchBufferFile(buf);
    statusBar()->showMessage(tr("File reloaded"), 2000);
}

void MainWindow::saveCopyAs()
{
    Buffer* buffer = _activeDocTab->currentBuffer();
    if (!buffer) return;
    const QString path = QFileDialog::getSaveFileName(this, tr("Save a Copy As"),
        buffer->isUntitled() ? QString() : buffer->getFullPath(),
        tr("All Files (*);;Text Files (*.txt)"));
    if (path.isEmpty()) return;
    if (!activateBufferView(buffer) ||
        !MainFileManager.saveBufferCopy(buffer, path)) {
        QMessageBox::warning(this, tr("Save Failed"),
            tr("Cannot save file:\n%1").arg(path));
        return;
    }
    statusBar()->showMessage(tr("Copy saved"), 2000);
}

void MainWindow::renameCurrentFile()
{
    Buffer* buffer = _activeDocTab->currentBuffer();
    if (!buffer || buffer->isUntitled()) return;
    if (buffer->isDirty() && !doSave(buffer, buffer->getFullPath()))
        return;

    const QFileInfo oldInfo(buffer->getFullPath());
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Rename"),
        tr("New file name:"), QLineEdit::Normal, oldInfo.fileName(), &ok).trimmed();
    if (!ok || name.isEmpty() || name == oldInfo.fileName()) return;
    const QString newPath = oldInfo.dir().filePath(name);
    if (QFileInfo::exists(newPath)) {
        QMessageBox::warning(this, tr("Rename"), tr("The target file already exists."));
        return;
    }

    unwatchBufferFile(buffer);
    if (!QFile::rename(oldInfo.absoluteFilePath(), newPath)) {
        watchBufferFile(buffer);
        QMessageBox::warning(this, tr("Rename"),
            tr("Cannot rename file to:\n%1").arg(newPath));
        return;
    }
    buffer->setFilePath(newPath);
    buffer->setLastKnownModificationTime(QFileInfo(newPath).lastModified());
    if (ScintillaEditView* view = activateBufferView(buffer))
        view->setLexerForFile(newPath);
    _mainDocTab->updateTabTitle(buffer);
    _subDocTab->updateTabTitle(buffer);
    watchBufferFile(buffer);
    addToRecentFiles(newPath);
    updateWindowTitle(buffer);
}

void MainWindow::moveCurrentFileToTrash()
{
    Buffer* buffer = _activeDocTab->currentBuffer();
    if (!buffer || buffer->isUntitled()) return;
    if (!checkBufferSave(buffer)) return;
    if (buffer->isDirty()) buffer->setDirty(false);

    QString error;
    unwatchBufferFile(buffer);
    if (!PlatformServices::moveToTrash(buffer->getFullPath(), &error)) {
        watchBufferFile(buffer);
        QMessageBox::warning(this, tr("Move to Recycle Bin"),
            tr("Cannot move the file to the recycle bin.\n%1").arg(error));
        return;
    }
    onBufferCloseRequested(buffer);
}

void MainWindow::closeAllButCurrent()
{
    Buffer* current = _activeDocTab->currentBuffer();
    QList<Buffer*> buffers;
    for (DocTabView* tab : {_mainDocTab, _subDocTab})
        for (int i = 0; i < tab->count(); ++i)
            if (tab->bufferAt(i) != current) buffers.append(tab->bufferAt(i));
    closeBufferList(buffers);
}

void MainWindow::closeAllToLeft()
{
    QList<Buffer*> buffers;
    const int current = _activeDocTab->currentIndex();
    for (int i = 0; i < current; ++i) buffers.append(_activeDocTab->bufferAt(i));
    closeBufferList(buffers);
}

void MainWindow::closeAllToRight()
{
    QList<Buffer*> buffers;
    const int current = _activeDocTab->currentIndex();
    for (int i = current + 1; i < _activeDocTab->count(); ++i)
        buffers.append(_activeDocTab->bufferAt(i));
    closeBufferList(buffers);
}

void MainWindow::closeAllUnchanged()
{
    QList<Buffer*> buffers;
    for (DocTabView* tab : {_mainDocTab, _subDocTab})
        for (int i = 0; i < tab->count(); ++i) {
            Buffer* buffer = tab->bufferAt(i);
            if (buffer && !buffer->isDirty()) buffers.append(buffer);
        }
    closeBufferList(buffers);
}

void MainWindow::saveSessionFile()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Save Session"),
        QString(), tr("Session Files (*.xml);;All Files (*)"));
    if (path.isEmpty()) return;
    if (!_suppressSessionPersistence)
        saveSession();
    NppParameters& params = NppParameters::getInstance();
    if (!params.writeSession(path, params.getSession()))
        QMessageBox::warning(this, tr("Save Session"), tr("Cannot save session file."));
}

void MainWindow::loadSessionFile()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Load Session"),
        QString(), tr("Session Files (*.xml);;All Files (*)"));
    if (path.isEmpty()) return;

    QList<Buffer*> buffers;
    for (DocTabView* tab : {_mainDocTab, _subDocTab})
        for (int i = 0; i < tab->count(); ++i) buffers.append(tab->bufferAt(i));
    if (!closeBufferList(buffers)) return;

    NppParameters& params = NppParameters::getInstance();
    if (!params.loadSession(path)) {
        QMessageBox::warning(this, tr("Load Session"), tr("Invalid session file."));
        return;
    }
    restoreSession();
}

void MainWindow::openContainingFolder()
{
    Buffer* buffer = _activeDocTab->currentBuffer();
    if (buffer && !buffer->isUntitled())
        PlatformServices::openInFileManager(buffer->getFullPath(), true);
}

void MainWindow::openContainingTerminal()
{
    Buffer* buffer = _activeDocTab->currentBuffer();
    if (buffer && !buffer->isUntitled())
        PlatformServices::openTerminal(QFileInfo(buffer->getFullPath()).absolutePath());
}

void MainWindow::openDefaultViewer()
{
    Buffer* buffer = _activeDocTab->currentBuffer();
    if (buffer && !buffer->isUntitled())
        PlatformServices::openDefaultApplication(buffer->getFullPath());
}

void MainWindow::openFolderAsWorkspace()
{
    QString initial;
    Buffer* buffer = _activeDocTab->currentBuffer();
    if (buffer && !buffer->isUntitled())
        initial = QFileInfo(buffer->getFullPath()).absolutePath();
    const QString folder = QFileDialog::getExistingDirectory(this,
        tr("Open Folder as Workspace"), initial);
    if (folder.isEmpty()) return;
    _fileBrowserPanel->setRootPath(folder);
    _fileBrowserDock->show();
    _fileBrowserAction->setChecked(true);
}

void MainWindow::goToLine()
{
    ScintillaEditView* view = currentActiveView();
    if (!view) return;
    bool ok;
    int line = QInputDialog::getInt(this, tr("Go To Line"),
        tr("Line number:"), 1, 1, view->lines(), 1, &ok);
    if (ok) {
        view->setCursorPosition(line - 1, 0);
        view->ensureLineVisible(line - 1);
    }
}

void MainWindow::zoomIn()
{
    if (auto* v = currentActiveView()) v->zoomIn();
}

void MainWindow::zoomOut()
{
    if (auto* v = currentActiveView()) v->zoomOut();
}

void MainWindow::zoomRestore()
{
    if (auto* v = currentActiveView()) v->zoomTo(0);
}

void MainWindow::toggleWordWrap()
{
    if (auto* v = currentActiveView()) {
        Buffer* buffer = _activeDocTab->currentBuffer();
        if (buffer && buffer->isLargeFile()) {
            v->applyWordWrap(false);
            if (_wordWrapAction) _wordWrapAction->setChecked(false);
            return;
        }
        bool wrap = v->wrapMode() == WrapWord;
        v->setWrapMode(wrap ? WrapNone : WrapWord);
        if (_wordWrapAction) _wordWrapAction->setChecked(!wrap);
    }
}

void MainWindow::toggleWhitespace()
{
    if (auto* v = currentActiveView()) {
        bool vis = v->whitespaceVisibility() != WsInvisible;
        v->setWhitespaceVisibility(vis ? WsInvisible
                                       : WsVisible);
        if (_showWhitespaceAction) _showWhitespaceAction->setChecked(!vis);
    }
}

void MainWindow::toggleIndentGuide()
{
    // 对应原版 NppCommands 的处理：
    // 1. 同时作用于两个视图（主 + 副），原版 showIndentGuideLine 对 _mainEditView/_subEditView 均调用
    // 2. 以配置项 _indentGuideLineShow 为状态源，而不是读取当前视图的实际值（避免双视图状态不一致）
    ScintillaViewParams& svp = NppParameters::getInstance().getSVP();
    bool show = !svp._indentGuideLineShow;
    svp._indentGuideLineShow = show;

    _mainDocTab->editor()->applyShowIndentGuide(show);
    _subDocTab->editor()->applyShowIndentGuide(show);

    if (_showIndentAction) _showIndentAction->setChecked(show);
}

void MainWindow::find()
{
    FindReplaceDlg* findDlg = ensureFindReplaceDialog();

    ScintillaEditView* view = _activeDocTab->editor();
    if (view) {
        findDlg->setCurrentView(view);
        if (view->hasSelectedText())
            findDlg->setSearchText(view->selectedText());
    }
    findDlg->openFindTab();
}

void MainWindow::replace()
{
    FindReplaceDlg* findDlg = ensureFindReplaceDialog();

    ScintillaEditView* view = _activeDocTab->editor();
    if (view) {
        findDlg->setCurrentView(view);
        if (view->hasSelectedText())
            findDlg->setSearchText(view->selectedText());
    }
    findDlg->openReplaceTab();
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Notepad++ Qt"),
        tr("Notepad++ Qt v8.4.6\n\n"
           "A Qt port of Notepad++\n"
           "Based on Notepad++ v8.4.6"));
}

// ─── 视图菜单槽函数 ───────────────────────────────────────────────────────────

void MainWindow::toggleSplitView()
{
    if (_subDocTab->isVisible()) {
        hideSubView();
    } else {
        // 副视图为空时先新建文档
        if (_subDocTab->count() == 0)
            doNewBuffer(_subDocTab);
        showSubView();
    }
}

void MainWindow::rotateSplitView()
{
    Qt::Orientation newOri = (_splitter->orientation() == Qt::Horizontal)
                             ? Qt::Vertical : Qt::Horizontal;
    _splitter->setOrientation(newOri);
}

void MainWindow::moveToOtherView()
{
    DocTabView* source = _activeDocTab;
    Buffer* buf = source->currentBuffer();
    if (!buf) return;

    DocTabView* other = otherTab();

    if (!other->isVisible())
        showSubView();

    const int sourceIndex = source->indexOfBuffer(buf);
    source->removeTab(sourceIndex);

    if (other->indexOfBuffer(buf) == -1)
        other->addBuffer(buf);

    if (source->count() == 0) {
        if (source == _mainDocTab)
            doNewBuffer(source);
        else
            hideSubView();
    }

    setActiveTab(other);
    other->activateBuffer(buf);
}

void MainWindow::cloneToOtherView()
{
    Buffer* buf = _activeDocTab->currentBuffer();
    if (!buf || !buf->document()) return;

    DocTabView* other = otherTab();
    if (!other->isVisible()) showSubView();

    // 如果另一视图已显示该 buffer（原始或克隆），直接切换
    if (other->indexOfBuffer(buf) != -1) {
        setActiveTab(other);
        other->activateBuffer(buf);
        return;
    }

    other->addClone(buf, other->editor());
    setActiveTab(other);
}

// ─── 内部槽函数 ───────────────────────────────────────────────────────────────

void MainWindow::onBufferCloseRequested(Buffer* buf)
{
    DocTabView* srcTab = qobject_cast<DocTabView*>(sender());
    if (!srcTab || srcTab->indexOfBuffer(buf) == -1) {
        if (_activeDocTab && _activeDocTab->indexOfBuffer(buf) != -1)
            srcTab = _activeDocTab;
        else if (_mainDocTab->indexOfBuffer(buf) != -1)
            srcTab = _mainDocTab;
        else
            srcTab = _subDocTab;
    }

    int idx = srcTab->indexOfBuffer(buf);
    DocTabView* otherDocTab = (srcTab == _mainDocTab) ? _subDocTab : _mainDocTab;
    const bool remainsOpen = otherDocTab->indexOfBuffer(buf) >= 0;
    if (!remainsOpen && !checkBufferSave(buf))
        return;

    if (idx >= 0)
        srcTab->removeTab(idx);

    if (srcTab == _subDocTab && srcTab->count() == 0)
        hideSubView();
    else if (srcTab == _mainDocTab && srcTab->count() == 0)
        doNewBuffer(_mainDocTab);

    if (!remainsOpen) {
        if (!buf->isUntitled())
            rememberClosedFile(buf->getFullPath());
        unwatchBufferFile(buf);
        buf->clearBackupFile();
        MainFileManager.closeBuffer(buf);
    }
    updateActionStates();
}

void MainWindow::onTextChanged()
{
    ScintillaEditView* view = qobject_cast<ScintillaEditView*>(sender());
    if (!view) return;

    Buffer* buf = view == _mainDocTab->editor()
        ? _mainDocTab->currentBuffer()
        : view == _subDocTab->editor()
            ? _subDocTab->currentBuffer()
            : MainFileManager.findBufferByView(view);

    if (!buf) return;

    const bool wasDirty = buf->isDirty();
    buf->setTextDirty(view->isModified());
    if (wasDirty != buf->isDirty()) {
        _mainDocTab->updateTabTitle(buf);
        _subDocTab->updateTabTitle(buf);
        if (_activeDocTab->currentBuffer() == buf)
            updateWindowTitle(buf);
    }
    updateActionStates();
}

void MainWindow::onCurrentTabChanged(int /*index*/)
{
    // 确定是哪个 DocTabView 发出的信号
    DocTabView* senderTab = qobject_cast<DocTabView*>(sender());
    if (senderTab) _activeDocTab = senderTab;

    Buffer* buf = _activeDocTab->currentBuffer();
    updateWindowTitle(buf);
    if (buf)
        updateFindReplaceView();
    updateStatusBar();
    updateLangStatus();
    updateActionStates();
    syncDocumentMap();
    if (_funcListDock && _funcListDock->isVisible())
        _funcListPanel->updateForView(currentActiveView());
}

void MainWindow::onFocusChanged(QWidget* /*old*/, QWidget* now)
{
    if (!now) return;
    // 判断焦点落在哪个视图的子控件内
    QWidget* w = now;
    while (w) {
        if (w == _mainDocTab) { setActiveTab(_mainDocTab); return; }
        if (w == _subDocTab)  { setActiveTab(_subDocTab);  return; }
        w = w->parentWidget();
    }
}

// ─── 宏录制/回放 ─────────────────────────────────────────────────────────────

void MainWindow::startMacroRecording()
{
    ScintillaEditView* view = currentActiveView();
    if (!view || _isRecording) return;

    // 用当前视图创建临时宏对象（只用于录制）
    _recordingMacro = new EditorMacro(view);
    _recordingView = view;
    _recordingMacro->startRecording();
    // 等待停止录制时获取字符串并销毁
    _isRecording = true;

    _startRecordAction->setEnabled(false);
    _stopRecordAction->setEnabled(true);
    _playMacroAction->setEnabled(false);
    statusBar()->showMessage(tr("Macro recording started..."));
}

void MainWindow::stopMacroRecording()
{
    if (!_isRecording) return;
    if (_recordingMacro) {
        _recordingMacro->endRecording();
        _macroStr = _recordingMacro->save();
        delete _recordingMacro;
        _recordingMacro = nullptr;
        _recordingView = nullptr;
    }

    _isRecording = false;
    _startRecordAction->setEnabled(true);
    _stopRecordAction->setEnabled(false);
    _playMacroAction->setEnabled(!_macroStr.isEmpty());
    statusBar()->showMessage(tr("Macro recording stopped."), 2000);
}

void MainWindow::printDocument()
{
    ScintillaEditView* view = currentActiveView();
    if (!view) return;
    Buffer* buffer = _activeDocTab ? _activeDocTab->currentBuffer() : nullptr;
    const QString path =
        buffer && !buffer->isUntitled() ? buffer->getFullPath() : QString();
    Printer printer(
        NppParameters::getInstance().getNppGUI(), path);
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() == QDialog::Accepted)
        printer.printView(view);
}

void MainWindow::printDocumentNow()
{
    ScintillaEditView* view = currentActiveView();
    if (!view) return;
    Buffer* buffer = _activeDocTab ? _activeDocTab->currentBuffer() : nullptr;
    const QString path =
        buffer && !buffer->isUntitled() ? buffer->getFullPath() : QString();
    Printer printer(
        NppParameters::getInstance().getNppGUI(), path);
    printer.printView(view);
}

static QPair<int, int> selectedLines(ScintillaEditView* view)
{
    int start = 0, end = 0;
    view->getSelection(&start, &end);
    if (start == end) {
        int lastLine = qMax(0, view->lines() - 1);
        if (lastLine > 0
            && view->positionFromLineIndex(lastLine, 0) == view->length())
            --lastLine;
        return qMakePair(0, lastLine);
    }
    int firstLine = 0, firstIndex = 0, lastLine = 0, lastIndex = 0;
    view->lineIndexFromPosition(start, &firstLine, &firstIndex);
    view->lineIndexFromPosition(end, &lastLine, &lastIndex);
    if (lastIndex == 0 && lastLine > firstLine) --lastLine;
    return qMakePair(firstLine, lastLine);
}

static QString eolText(EolMode mode)
{
    if (mode == EolWindows) return "\r\n";
    if (mode == EolMac) return "\r";
    return "\n";
}

static void replaceLines(ScintillaEditView* view, int firstLine, int lastLine,
                         const QStringList& lines)
{
    const int start = view->positionFromLineIndex(firstLine, 0);
    const int end = lastLine + 1 < view->lines()
            ? view->positionFromLineIndex(lastLine + 1, 0) : view->length();
    const QString oldText = view->text(start, end);
    const QString eol = eolText(view->eolMode());
    const bool terminalEol = oldText.endsWith("\n") || oldText.endsWith("\r");
    QString replacement = lines.join(eol);
    if (terminalEol && !lines.isEmpty()) replacement += eol;
    const QByteArray utf8 = replacement.toUtf8();
    view->beginUndoAction();
    view->SendScintilla(SCI_SETTARGETRANGE, start, end);
    view->SendScintilla(SCI_REPLACETARGET, utf8.size(),
        reinterpret_cast<sptr_t>(utf8.constData()));
    view->endUndoAction();
}

void MainWindow::sortLines(int mode)
{
    ScintillaEditView* view = currentActiveView();
    if (!view || view->lines() < 2) return;
    const QPair<int, int> range = selectedLines(view);
    QStringList lines;
    for (int line = range.first; line <= range.second; ++line) {
        QString text = view->text(line);
        while (text.endsWith('\n') || text.endsWith('\r')) text.chop(1);
        lines.append(text);
    }
    int sortColumnStart = 0;
    int sortColumnLength = -1;
    if (view->SendScintilla(SCI_GETSELECTIONMODE)
        == SC_SEL_RECTANGLE) {
        const int anchor = static_cast<int>(
            view->SendScintilla(SCI_GETANCHOR));
        const int caret = static_cast<int>(
            view->SendScintilla(SCI_GETCURRENTPOS));
        const int anchorColumn = static_cast<int>(view->SendScintilla(
            SCI_GETCOLUMN, anchor));
        const int caretColumn = static_cast<int>(view->SendScintilla(
            SCI_GETCOLUMN, caret));
        sortColumnStart = qMin(anchorColumn, caretColumn);
        sortColumnLength = qAbs(anchorColumn - caretColumn);
    }
    auto sortKey = [&](const QString& line) {
        return sortColumnLength >= 0
            ? line.mid(sortColumnStart, sortColumnLength)
            : line;
    };
    if (mode == 4) {
        std::reverse(lines.begin(), lines.end());
    } else if (mode == 9) {
        std::shuffle(lines.begin(), lines.end(), *QRandomGenerator::global());
    } else {
        const bool descending =
            mode == 1 || mode == 3 || mode == 6 || mode == 8 || mode == 11;
        const bool insensitive = mode == 2 || mode == 3;
        const bool integerSort = mode == 5 || mode == 6;
        const bool decimalSort =
            mode == 7 || mode == 8 || mode == 10 || mode == 11;
        const bool decimalComma = mode == 10 || mode == 11;
        QCollator collator;
        collator.setCaseSensitivity(insensitive ? Qt::CaseInsensitive : Qt::CaseSensitive);
        std::stable_sort(lines.begin(), lines.end(), [&](const QString& left, const QString& right) {
            const QString leftKey = sortKey(left);
            const QString rightKey = sortKey(right);
            int result = 0;
            if (integerSort) {
                bool leftOk = false;
                bool rightOk = false;
                const qlonglong leftValue =
                    leftKey.trimmed().toLongLong(&leftOk);
                const qlonglong rightValue =
                    rightKey.trimmed().toLongLong(&rightOk);
                if (leftOk != rightOk)
                    result = leftOk ? -1 : 1;
                else if (leftOk)
                    result = leftValue < rightValue ? -1
                        : leftValue > rightValue ? 1 : 0;
                else
                    result = collator.compare(leftKey, rightKey);
            } else if (decimalSort) {
                bool leftOk = false;
                bool rightOk = false;
                QString normalizedLeft = leftKey.trimmed();
                QString normalizedRight = rightKey.trimmed();
                if (decimalComma) {
                    normalizedLeft.replace(',', '.');
                    normalizedRight.replace(',', '.');
                }
                const double leftValue = normalizedLeft.toDouble(&leftOk);
                const double rightValue = normalizedRight.toDouble(&rightOk);
                if (leftOk != rightOk)
                    result = leftOk ? -1 : 1;
                else if (leftOk)
                    result = leftValue < rightValue ? -1
                        : leftValue > rightValue ? 1 : 0;
                else
                    result = collator.compare(leftKey, rightKey);
            } else {
                result = collator.compare(leftKey, rightKey);
            }
            return descending ? result > 0 : result < 0;
        });
    }
    replaceLines(view, range.first, range.second, lines);
}

void MainWindow::transformLines(int mode)
{
    ScintillaEditView* view = currentActiveView();
    if (!view) return;
    const QPair<int, int> range = selectedLines(view);
    QStringList output;
    for (int line = range.first; line <= range.second; ++line) {
        QString text = view->text(line);
        while (text.endsWith('\n') || text.endsWith('\r')) text.chop(1);
        if (mode == 0 && text.isEmpty()) continue;
        if (mode == 1 && text.trimmed().isEmpty()) continue;
        const int tabSize = qMax(1, NppParameters::getInstance().getNppGUI()._tabSize);
        if (mode == 2) text = text.trimmed();
        if (mode == 3) text = text.replace('\t', QString(tabSize, ' '));
        if (mode == 4) text.remove(QRegularExpression("\\s+$"));
        if (mode == 5) text.remove(QRegularExpression("^\\s+"));
        if (mode == 6) {
            int count = 0;
            while (count < text.size() && text.at(count) == ' ') ++count;
            text.replace(0, count, QString(count / tabSize, '\t') +
                         QString(count % tabSize, ' '));
        }
        if (mode == 7) text.replace(QString(tabSize, ' '), "\t");
        output.append(text);
    }
    replaceLines(view, range.first, range.second, output);
}

void MainWindow::insertDateTime(int mode)
{
    ScintillaEditView* view = currentActiveView();
    if (!view) return;
    QString text;
    const QDateTime now = QDateTime::currentDateTime();
    if (mode == 0)
        text = QLocale().toString(now, QLocale::ShortFormat);
    else if (mode == 1)
        text = QLocale().toString(now, QLocale::LongFormat);
    else {
        const QString format =
            NppParameters::getInstance().getNppGUI()._dateTimeFormat;
        text = now.toString(format.isEmpty()
            ? QString("yyyy-MM-dd HH:mm:ss") : format);
    }
    view->insert(text);
}

void MainWindow::transformSelectionCase(int mode)
{
    ScintillaEditView* view = currentActiveView();
    if (!view || !view->hasSelectedText()) return;
    QString value = view->selectedText();
    if (mode == 0) {
        bool wordStart = true;
        for (int i = 0; i < value.size(); ++i) {
            if (value.at(i).isLetterOrNumber()) {
                value[i] = wordStart ? value.at(i).toUpper() : value.at(i).toLower();
                wordStart = false;
            } else {
                wordStart = true;
            }
        }
    } else if (mode == 1) {
        value = value.toLower();
        for (int i = 0; i < value.size(); ++i) {
            if (value.at(i).isLetter()) {
                value[i] = value.at(i).toUpper();
                break;
            }
        }
    } else {
        for (int i = 0; i < value.size(); ++i) {
            if (value.at(i).isLower()) value[i] = value.at(i).toUpper();
            else if (value.at(i).isUpper()) value[i] = value.at(i).toLower();
        }
    }
    view->replaceSelectedText(value);
}

void MainWindow::goToMatchingBrace()
{
    ScintillaEditView* view = currentActiveView();
    if (!view) return;
    Buffer* buffer = _activeDocTab->currentBuffer();
    if (buffer && buffer->isLargeFile())
        return;
    int position = view->getCurrentPos();
    auto isBrace = [view](int pos) {
        const int ch = static_cast<int>(view->SendScintilla(SCI_GETCHARAT, pos));
        return ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '{' || ch == '}';
    };
    if (!isBrace(position) && position > 0 && isBrace(position - 1)) --position;
    if (!isBrace(position)) return;
    const int match = static_cast<int>(
        view->SendScintilla(SCI_BRACEMATCH,
                            static_cast<unsigned long>(position), static_cast<long>(0)));
    if (match >= 0) view->setCurrentPos(match);
}

void MainWindow::jumpSearchMark(bool forward)
{
    ScintillaEditView* view = currentActiveView();
    if (!view) return;
    const int indicator = ScintillaEditView::FIND_MARK_INDICATOR;
    const int length = view->length();
    int position = view->getCurrentPos();
    if (forward) {
        position = qMin(length, position + 1);
        while (position < length) {
            if (view->SendScintilla(SCI_INDICATORVALUEAT,
                                    indicator, position) != 0) break;
            int next = static_cast<int>(view->SendScintilla(
                SCI_INDICATOREND, indicator, position));
            position = next > position ? next : position + 1;
        }
    } else {
        position = qMax(0, position - 1);
        while (position > 0) {
            if (view->SendScintilla(SCI_INDICATORVALUEAT,
                                    indicator, position) != 0) {
                position = static_cast<int>(view->SendScintilla(
                    SCI_INDICATORSTART, indicator, position));
                break;
            }
            int previous = static_cast<int>(view->SendScintilla(
                SCI_INDICATORSTART, indicator, position));
            position = previous < position ? qMax(0, previous - 1) : position - 1;
        }
    }
    if (position >= 0 && position < length &&
        view->SendScintilla(SCI_INDICATORVALUEAT,
                            indicator, position) != 0)
        view->setCurrentPos(position);
}

void MainWindow::clearSearchMarks()
{
    ScintillaEditView* view = currentActiveView();
    if (!view) return;
    view->SendScintilla(SCI_SETINDICATORCURRENT,
                        ScintillaEditView::FIND_MARK_INDICATOR);
    view->SendScintilla(SCI_INDICATORCLEARRANGE, 0, view->length());
}

void MainWindow::columnEditor()
{
    ScintillaEditView* view = currentActiveView();
    if (!view) return;

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Column Editor"));
    QFormLayout layout(&dialog);
    QComboBox mode;
    mode.setObjectName("columnModeCombo");
    mode.addItem(tr("Text"));
    mode.addItem(tr("Number"));
    QLineEdit textValue;
    textValue.setObjectName("columnTextValue");
    QSpinBox initial;
    initial.setObjectName("columnInitialValue");
    QSpinBox increment;
    increment.setObjectName("columnIncrementValue");
    QSpinBox repeat;
    repeat.setObjectName("columnRepeatValue");
    QComboBox format;
    format.setObjectName("columnFormatCombo");
    QCheckBox leadingZeros(tr("Leading zeros"));
    leadingZeros.setObjectName("columnLeadingZeros");
    initial.setRange(-1000000000, 1000000000);
    increment.setRange(-1000000, 1000000);
    increment.setValue(1);
    repeat.setRange(1, 1000000);
    repeat.setValue(1);
    format.addItem(tr("Decimal"), 10);
    format.addItem(tr("Hexadecimal"), 16);
    format.addItem(tr("Octal"), 8);
    format.addItem(tr("Binary"), 2);
    layout.addRow(tr("Mode:"), &mode);
    layout.addRow(tr("Text:"), &textValue);
    layout.addRow(tr("Initial number:"), &initial);
    layout.addRow(tr("Increase by:"), &increment);
    layout.addRow(tr("Repeat:"), &repeat);
    layout.addRow(tr("Format:"), &format);
    layout.addRow(QString(), &leadingZeros);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout.addRow(&buttons);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    auto updateMode = [&](int index) {
        textValue.setEnabled(index == 0);
        initial.setEnabled(index == 1);
        increment.setEnabled(index == 1);
        repeat.setEnabled(index == 1);
        format.setEnabled(index == 1);
        leadingZeros.setEnabled(index == 1);
    };
    connect(&mode, QOverload<int>::of(&QComboBox::currentIndexChanged), &dialog, updateMode);
    updateMode(0);
    if (dialog.exec() != QDialog::Accepted) return;

    int cursorLine = 0, column = 0;
    view->getCursorPosition(&cursorLine, &column);
    const QPair<int, int> range = selectedLines(view);
    const int base = format.currentData().toInt();
    int numberWidth = 0;
    if (mode.currentIndex() == 1 && leadingZeros.isChecked()) {
        for (int line = range.first; line <= range.second; ++line) {
            const qlonglong value = initial.value()
                + ((line - range.first) / repeat.value()) * increment.value();
            numberWidth = qMax(numberWidth, QString::number(value, base).size());
        }
    }
    view->beginUndoAction();
    for (int line = range.second; line >= range.first; --line) {
        QString value;
        if (mode.currentIndex() == 0) {
            value = textValue.text();
        } else {
            const qlonglong number = initial.value()
                + ((line - range.first) / repeat.value()) * increment.value();
            value = QString::number(number, base);
            if (base == 16)
                value = value.toUpper();
            if (leadingZeros.isChecked() && value.size() < numberWidth) {
                if (value.startsWith('-'))
                    value = QStringLiteral("-")
                        + value.mid(1).rightJustified(numberWidth - 1, '0');
                else
                    value = value.rightJustified(numberWidth, '0');
            }
        }
        int position = view->positionFromLineIndex(line, column);
        if (position < 0) {
            const int lineEnd = view->positionFromLineIndex(line, qMax(0, view->lineLength(line) - 1));
            position = lineEnd;
        }
        const QByteArray bytes = value.toUtf8();
        view->SendScintilla(SCI_INSERTTEXT, position,
            reinterpret_cast<sptr_t>(bytes.constData()));
    }
    view->endUndoAction();
}

void MainWindow::playMacro()
{
    if (_macroStr.isEmpty() || _isRecording) return;
    ScintillaEditView* view = currentActiveView();
    if (!view) return;

    EditorMacro temp(view);
    if (temp.load(_macroStr))
        temp.play();
}

void MainWindow::saveMacro()
{
    if (_macroStr.isEmpty()) return;
    QString path = QFileDialog::getSaveFileName(this,
        tr("Save Macro"), QString(), tr("Macro Files (*.macro);;All Files (*)"));
    if (path.isEmpty()) return;
    QFile f(path);
    if (f.open(QFile::WriteOnly | QFile::Text))
        f.write(_macroStr.toUtf8());
}

void MainWindow::loadMacro()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("Load Macro"), QString(), tr("Macro Files (*.macro);;All Files (*)"));
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QFile::ReadOnly | QFile::Text)) return;
    _macroStr = QString::fromUtf8(f.readAll());
    _playMacroAction->setEnabled(!_macroStr.isEmpty());
}

void MainWindow::toggleBookmark()
{
    if (auto* v = currentActiveView()) v->toggleBookmark();
}
void MainWindow::nextBookmark()
{
    if (auto* v = currentActiveView()) v->nextBookmark();
}
void MainWindow::prevBookmark()
{
    if (auto* v = currentActiveView()) v->prevBookmark();
}
void MainWindow::clearAllBookmarks()
{
    if (auto* v = currentActiveView()) v->clearAllBookmarks();
}

void MainWindow::onCursorPositionChanged(int line, int col)
{
    ScintillaEditView* view = qobject_cast<ScintillaEditView*>(sender());
    if (!view || view != currentActiveView()) return;

    // 同步更新 STATUSBAR_CUR_POS（含 Pos/Sel）和 STATUSBAR_DOC_SIZE（行数会随编辑变化）
    // 复用 updateStatusBar 中的逻辑，避免代码重复
    Q_UNUSED(line); Q_UNUSED(col);
    // 只更新频繁变化的两个部分，编码/EOL/INS 不需要每次都刷新
    const qintptr selStart =
        view->SendScintillaNpp(SCI_GETSELECTIONSTART);
    const qintptr selEnd =
        view->SendScintillaNpp(SCI_GETSELECTIONEND);
    int  curLine, curCol;
    view->getCursorPosition(&curLine, &curCol);
    QString posPart;
    if (selStart == selEnd) {
        const qintptr pos =
            view->SendScintillaNpp(SCI_GETCURRENTPOS);
        posPart  = QString("Pos : %1").arg(QLocale().toString((qlonglong)(pos + 1)));
    } else {
        const qintptr selChars = selEnd - selStart;
        const int selLineStart = static_cast<int>(view->SendScintillaNpp(
            SCI_LINEFROMPOSITION,
            static_cast<quintptr>(selStart)));
        const int selLineEnd = static_cast<int>(view->SendScintillaNpp(
            SCI_LINEFROMPOSITION,
            static_cast<quintptr>(selEnd)));
        posPart = QString("Sel : %1 | %2")
            .arg(QLocale().toString((qlonglong)selChars))
            .arg(selLineEnd - selLineStart + 1);
    }
    _posLabel->setText(
        QString("Ln : %1    Col : %2    %3")
            .arg(curLine + 1).arg(curCol + 1).arg(posPart));

    const qintptr docLen = view->documentLengthNpp();
    int  nLines = (int) view->SendScintilla(SCI_GETLINECOUNT);
    _docSizeLabel->setText(
        QString("length : %1    lines : %2")
            .arg(QLocale().toString((qlonglong)docLen))
            .arg(QLocale().toString((qlonglong)nLines)));
}

// ─── 备份定时器 ──────────────────────────────────────────────────────────────

void MainWindow::onBackupTimer()
{
    // 与原版 snapshot 模式一致：对每个脏 buffer 写入备份文件
    NppParameters& params = NppParameters::getInstance();
    if (!params.getNppGUI()._isSnapshotMode)
        return;

    QString backupDir = params.backupDirPath();
    QSet<Buffer*> visited;
    DocTabView* const originalActiveTab = _activeDocTab;

    auto backupTab = [&](DocTabView* tab) {
        const int originalIndex = tab->currentIndex();
        for (int i = 0; i < tab->count(); ++i) {
            Buffer* buf = tab->bufferAt(i);
            if (!buf || !buf->isDirty() ||
                buf->isLargeFile()) continue;
            if (visited.contains(buf)) continue;
            visited.insert(buf);
            ScintillaEditView* view = activateBufferView(buf, tab);
            if (!view)
                continue;

            // 首次备份时生成路径：filename@timestamp
            if (buf->getBackupFilePath().isEmpty()) {
                QString baseName = buf->isUntitled()
                    ? buf->getFileName()
                    : QFileInfo(buf->getFullPath()).fileName();
                QString ts = QString::number(QDateTime::currentSecsSinceEpoch());
                buf->setBackupFilePath(backupDir + "/" + baseName + "@" + ts);
            }

            QByteArray backupData;
            const QString backupText = view->text();
            bool encoded = TextFileCodec::encode(
                backupText, buf->getEncoding(), buf->hasBom(), &backupData);
            if (!encoded) {
                encoded = TextFileCodec::encode(
                    backupText, QStringLiteral("UTF-8"), true, &backupData);
            }
            if (encoded) {
                QSaveFile file(buf->getBackupFilePath());
                if (file.open(QFile::WriteOnly) &&
                    file.write(backupData) == backupData.size()) {
                    file.commit();
                }
            }
        }
        if (originalIndex >= 0 && originalIndex < tab->count())
            tab->setCurrentIndex(originalIndex);
    };

    backupTab(_mainDocTab);
    backupTab(_subDocTab);
    setActiveTab(originalActiveTab);
}

// ─── 关闭事件 ────────────────────────────────────────────────────────────────

void MainWindow::closeEvent(QCloseEvent* event)
{
    NppParameters& params = NppParameters::getInstance();
    NppGUI& gui = params.getNppGUI();

    if (!gui._isSnapshotMode) {
        QSet<Buffer*> checked;
        for (DocTabView* tab : {_mainDocTab, _subDocTab}) {
            for (int i = 0; tab && i < tab->count(); ++i) {
                Buffer* buffer = tab->bufferAt(i);
                if (!buffer || checked.contains(buffer))
                    continue;
                checked.insert(buffer);
                if (!checkBufferSave(buffer)) {
                    if (!_pendingPluginUpdatePlan.isEmpty() &&
                        !_pluginUpdaterStarted) {
                        QFile::remove(_pendingPluginUpdatePlan);
                        _pendingPluginUpdatePlan.clear();
                    }
                    event->ignore();
                    return;
                }
            }
        }
    }

    gui._isMaximized  = isMaximized();
    gui._windowState  = saveState();
    if (!isMaximized()) {
        QRect g = geometry();
        gui._appPos = g;
    }
    if (gui._isSnapshotMode)
        onBackupTimer();
    saveSession();
    params.writeNppGUI();

    if (!_pendingPluginUpdatePlan.isEmpty() &&
        !_pluginUpdaterStarted) {
        QString updaterError;
        if (!launchPendingPluginUpdater(&updaterError)) {
            QMessageBox::warning(
                this, tr("Plugins Admin"),
                tr("Could not start the plugin updater:\n%1")
                    .arg(updaterError));
            event->ignore();
            return;
        }
        _pluginUpdaterStarted = true;
    }

    event->accept();
}

// ─── IPluginHost ─────────────────────────────────────────────────────────────

QString MainWindow::currentFilePath() const
{
    Buffer* buf = _activeDocTab->currentBuffer();
    return (buf && !buf->isUntitled()) ? buf->getFullPath() : QString();
}

// ─── 文档地图 ────────────────────────────────────────────────────────────────

void MainWindow::setupDocumentMap()
{
    _docMapPanel = new DocumentMapPanel(this);

    _docMapDock = new QDockWidget(tr("Document Map"), this);
    _docMapDock->setObjectName("DocumentMapDock");
    _docMapDock->setWidget(_docMapPanel);
    _docMapDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, _docMapDock);
    _docMapDock->hide();

    connect(_docMapDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (_docMapAction) _docMapAction->setChecked(visible);
        if (visible)
            syncDocumentMap();
        else if (_documentMapBuffer && _docMapPanel) {
            _documentMapBuffer->setMapState(_docMapPanel->sessionState());
            _documentMapBuffer = nullptr;
        }
    });
}

void MainWindow::syncDocumentMap()
{
    if (!_docMapPanel || !_docMapDock || !_docMapDock->isVisible())
        return;

    if (_documentMapBuffer)
        _documentMapBuffer->setMapState(_docMapPanel->sessionState());

    _documentMapBuffer = _activeDocTab ? _activeDocTab->currentBuffer() : nullptr;
    _docMapPanel->syncWith(currentActiveView());
    if (_documentMapBuffer)
        _docMapPanel->restoreSessionState(_documentMapBuffer->mapState());
}

// ─── 函数列表 ────────────────────────────────────────────────────────────────

void MainWindow::setupFunctionList()
{
    _funcListPanel = new FunctionListPanel(this);

    _funcListDock = new QDockWidget(tr("Function List"), this);
    _funcListDock->setObjectName("FunctionListDock");
    _funcListDock->setWidget(_funcListPanel);
    _funcListDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, _funcListDock);
    _funcListDock->hide();

    connect(_funcListPanel, &FunctionListPanel::navigationRequested,
            this, [this](int line) {
                if (auto* v = currentActiveView()) {
                    v->setCursorPosition(line, 0);
                    v->ensureLineVisible(line);
                    v->setFocus();
                }
            });

    connect(_funcListDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (_funcListAction) _funcListAction->setChecked(visible);
        if (visible) _funcListPanel->updateForView(currentActiveView());
    });
}

// ─── Find All 结果面板 ────────────────────────────────────────────────────────
// 对应原版 Finder 停靠窗口，显示 Find All in Current Doc 的搜索结果

void MainWindow::setupAuxiliaryPanels()
{
    _documentList = new QListWidget(this);
    _documentListDock = new QDockWidget(tr("Document List"), this);
    _documentListDock->setObjectName("DocumentListDock");
    _documentListDock->setWidget(_documentList);
    addDockWidget(Qt::RightDockWidgetArea, _documentListDock);
    _documentListDock->hide();

    auto refreshDocuments = [this]() {
        _documentList->clear();
        QSet<Buffer*> visited;
        for (DocTabView* tab : {_mainDocTab, _subDocTab}) {
            for (int i = 0; tab && i < tab->count(); ++i) {
                Buffer* buffer = tab->bufferAt(i);
                if (!buffer || visited.contains(buffer))
                    continue;
                visited.insert(buffer);
                QListWidgetItem* item =
                    new QListWidgetItem(buffer->getTabLabel(), _documentList);
                item->setToolTip(buffer->getFullPath());
                item->setData(Qt::UserRole, QVariant::fromValue<quintptr>(
                    reinterpret_cast<quintptr>(buffer)));
            }
        }
    };
    connect(_documentListDock, &QDockWidget::visibilityChanged, this,
            [refreshDocuments](bool visible) {
                if (visible)
                    refreshDocuments();
            });
    connect(_mainDocTab, &DocTabView::currentChanged, this,
            [refreshDocuments](int) { refreshDocuments(); });
    connect(_subDocTab, &DocTabView::currentChanged, this,
            [refreshDocuments](int) { refreshDocuments(); });
    connect(_documentList, &QListWidget::itemActivated, this,
            [this](QListWidgetItem* item) {
        Buffer* buffer = reinterpret_cast<Buffer*>(
            item->data(Qt::UserRole).value<quintptr>());
        if (_mainDocTab->indexOfBuffer(buffer) >= 0) {
            setActiveTab(_mainDocTab);
            _mainDocTab->activateBuffer(buffer);
        } else if (_subDocTab->indexOfBuffer(buffer) >= 0) {
            setActiveTab(_subDocTab);
            _subDocTab->activateBuffer(buffer);
        }
    });

    for (int i = 0; i < 3; ++i) {
        ProjectPanel* panel = new ProjectPanel(i, this);
        _projectPanels[i] = panel;
        NativeLangSpeaker& speaker =
            NppParameters::getInstance().getNativeLangSpeaker();
        if (speaker.isLoaded())
            speaker.changeDlgLang(panel, "ProjectPanel");
        connect(panel, &ProjectPanel::fileActivated, this,
                [this](const QString& path) { doOpenFile(path, _activeDocTab); });
        connect(panel, &ProjectPanel::workspacePathChanged, this,
                [](int id, const QString& path) {
            NppParameters& parameters = NppParameters::getInstance();
            parameters.setWorkspaceFilePath(id, path);
            parameters.writeNppGUI();
        });
        connect(panel, &ProjectPanel::findInProjectsRequested, this,
                [this](int panelMask) {
            ensureFindReplaceDialog()->openFindInProjectsTab(panelMask);
        });
        const QString workspace =
            NppParameters::getInstance().workspaceFilePath(i);
        if (!workspace.isEmpty() && QFileInfo::exists(workspace))
            panel->loadWorkspace(workspace);

        QDockWidget* dock = new QDockWidget(tr("Project %1").arg(i + 1), this);
        dock->setObjectName(i == 0
            ? QStringLiteral("ProjectPanelsDock")
            : QStringLiteral("ProjectPanelsDock%1").arg(i + 1));
        dock->setWidget(panel);
        dock->setMinimumWidth(190);
        addDockWidget(Qt::LeftDockWidgetArea, dock);
        dock->hide();
        _projectPanelsDock[i] = dock;
    }
    resizeDocks({_projectPanelsDock[0]}, {207}, Qt::Horizontal);

    _clipboardHistory = new QListWidget(this);
    _clipboardHistory->setWordWrap(true);
    _clipboardDock = new QDockWidget(tr("Clipboard History"), this);
    _clipboardDock->setObjectName("ClipboardHistoryDock");
    _clipboardDock->setWidget(_clipboardHistory);
    addDockWidget(Qt::BottomDockWidgetArea, _clipboardDock);
    _clipboardDock->hide();
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, [this]() {
        const QString text = QApplication::clipboard()->text();
        if (text.isEmpty())
            return;
        if (_clipboardHistory->count() > 0 &&
            _clipboardHistory->item(0)->data(Qt::UserRole).toString() == text)
            return;
        QListWidgetItem* item = new QListWidgetItem(
            text.left(160).replace('\n', QChar(0x23CE)));
        item->setData(Qt::UserRole, text);
        _clipboardHistory->insertItem(0, item);
        while (_clipboardHistory->count() > 30)
            delete _clipboardHistory->takeItem(_clipboardHistory->count() - 1);
    });
    connect(_clipboardHistory, &QListWidget::itemActivated, this,
            [this](QListWidgetItem* item) {
        if (ScintillaEditView* view = currentActiveView())
            view->insert(item->data(Qt::UserRole).toString());
    });

    _characterList = new QListWidget(this);
    for (int code = 32; code <= 255; ++code) {
        const QChar character(code);
        QListWidgetItem* item = new QListWidgetItem(
            QString("%1   U+%2").arg(character)
                .arg(code, 4, 16, QChar('0')).toUpper(), _characterList);
        item->setData(Qt::UserRole, QString(character));
    }
    _characterDock = new QDockWidget(tr("Character Panel"), this);
    _characterDock->setObjectName("CharacterPanelDock");
    _characterDock->setWidget(_characterList);
    addDockWidget(Qt::RightDockWidgetArea, _characterDock);
    _characterDock->hide();
    connect(_characterList, &QListWidget::itemActivated, this,
            [this](QListWidgetItem* item) {
        if (ScintillaEditView* view = currentActiveView())
            view->insert(item->data(Qt::UserRole).toString());
    });
}

void MainWindow::setupFindResultPanel()
{
    _findResultView = new ScintillaEditView(this);
    _findResultView->setObjectName(QStringLiteral("findResultView"));
    _findResultView->setFont(QFont(QStringLiteral("Courier New"), 9));
    _findResultView->setBuiltinLanguage(QStringLiteral("searchResult"));
    _findResultView->setFolding(BoxedTreeFoldStyle);
    _findResultView->setMarginWidth(0, 0);
    _findResultView->setMarginWidth(1, 0);
    _findResultView->setReadOnly(true);

    _findResultDock = new QDockWidget(tr("Find Result"), this);
    _findResultDock->setObjectName("FindResultDock");
    _findResultDock->setWidget(_findResultView);
    _findResultDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, _findResultDock);
    _findResultDock->hide();

    // 双击结果行 → 跳转到对应文档位置。
    connect(_findResultView, &ScintillaEditBase::doubleClick, this,
            [this](Scintilla::Position, Scintilla::Position resultLine) {
        if (resultLine < 0 ||
            resultLine >= _findResultIndexByLine.size())
            return;
        const int resultIndex = _findResultIndexByLine.at(resultLine);
        if (resultIndex < 0 || resultIndex >= _lastFindResults.size())
            return;
        const FindAllResult& result = _lastFindResults.at(resultIndex);
        const qintptr matchStart = static_cast<qintptr>(result.matchStart);
        const QString filePath = result.filePath;
        const QString sourceName = result.sourceName;
        if (!filePath.isEmpty()) {
            doOpenFile(filePath, _activeDocTab);
        } else if (!sourceName.isEmpty()) {
            auto activateUntitled = [&](DocTabView* tab) {
                for (int i = 0; tab && i < tab->count(); ++i) {
                    Buffer* buf = tab->bufferAt(i);
                    if (buf && buf->isUntitled() && buf->getFileName() == sourceName) {
                        tab->activateBuffer(buf);
                        setActiveTab(tab);
                        return true;
                    }
                }
                return false;
            };
            activateUntitled(_mainDocTab) || activateUntitled(_subDocTab);
        }
        if (auto* v = currentActiveView()) {
            const int line = static_cast<int>(v->SendScintillaNpp(
                SCI_LINEFROMPOSITION,
                static_cast<quintptr>(matchStart)));
            v->setCursorPosition(line, 0);
            v->ensureLineVisible(line);
            v->setFocus();
        }
    });

    connectFindReplaceDialogSignals();
}

void MainWindow::connectFindReplaceDialogSignals()
{
    if (!_findReplaceDlg)
        return;

    connect(_findReplaceDlg, &FindReplaceDlg::findAllResultsReady,
            this, &MainWindow::onFindAllResults, Qt::UniqueConnection);
    connect(_findReplaceDlg, &FindReplaceDlg::findAllOpenedDocsRequested,
            this, &MainWindow::onFindAllOpenedDocsRequested, Qt::UniqueConnection);
    connect(_findReplaceDlg, &FindReplaceDlg::findInFilesRequested,
            this, &MainWindow::onFindInFilesRequested, Qt::UniqueConnection);
    connect(_findReplaceDlg, &FindReplaceDlg::replaceAllOpenedDocsRequested,
            this, &MainWindow::onReplaceAllOpenedDocsRequested, Qt::UniqueConnection);
    connect(_findReplaceDlg, &FindReplaceDlg::replaceInFilesRequested,
            this, &MainWindow::onReplaceInFilesRequested, Qt::UniqueConnection);
    connect(_findReplaceDlg, &FindReplaceDlg::findInProjectsRequested,
            this, &MainWindow::onFindInProjectsRequested, Qt::UniqueConnection);
    connect(_findReplaceDlg, &FindReplaceDlg::replaceInProjectsRequested,
            this, &MainWindow::onReplaceInProjectsRequested,
            Qt::UniqueConnection);
}

FindReplaceDlg* MainWindow::ensureFindReplaceDialog()
{
    if (!_findReplaceDlg) {
        _findReplaceDlg = new FindReplaceDlg(this);
        connectFindReplaceDialogSignals();
    }

    NativeLangSpeaker& speaker =
        NppParameters::getInstance().getNativeLangSpeaker();
    if (speaker.isLoaded())
        speaker.changeDlgLang(_findReplaceDlg, "FindReplace");

    return _findReplaceDlg;
}

void MainWindow::onFindAllResults(const QString& searchText,
                                  const QList<FindAllResult>& results)
{
    _lastFindResults = results;
    _findResultIndexByLine.clear();
    QStringList output;
    output << tr("Search \"%1\": %2 match(es) found")
                  .arg(searchText).arg(results.count());
    _findResultIndexByLine << -1;
    _findResultDock->setWindowTitle(tr("Find Result - \"%1\"").arg(searchText));

    QString currentSource;
    for (int resultIndex = 0; resultIndex < results.size(); ++resultIndex) {
        const FindAllResult& r = results.at(resultIndex);
        QString source = r.sourceName;
        if (source.isEmpty() && !r.filePath.isEmpty())
            source = QFileInfo(r.filePath).fileName();
        if (!source.isEmpty() && source != currentSource) {
            currentSource = source;
            QString groupLabel = source;
            if (!r.filePath.isEmpty() && r.filePath != source)
                groupLabel += QStringLiteral(" - ") + r.filePath;
            output << groupLabel;
            _findResultIndexByLine << -1;
        }
        output << QStringLiteral("  Line %1:\t%2")
                      .arg(r.lineNo + 1).arg(r.lineText);
        _findResultIndexByLine << resultIndex;
    }

    _findResultView->setReadOnly(false);
    _findResultView->setText(output.join(QStringLiteral("\n")));
    _findResultView->setReadOnly(true);
    const int base = SC_FOLDLEVELBASE;
    const int header = SC_FOLDLEVELHEADERFLAG;
    int currentGroupLine = -1;
    for (int line = 0; line < _findResultIndexByLine.size(); ++line) {
        const int resultIndex = _findResultIndexByLine.at(line);
        if (line == 0) {
            _findResultView->SendScintillaNpp(
                SCI_SETFOLDLEVEL, line, base | header);
        } else if (resultIndex < 0) {
            currentGroupLine = line;
            _findResultView->SendScintillaNpp(
                SCI_SETFOLDLEVEL,
                line, (base + 1) | header);
        } else {
            _findResultView->SendScintillaNpp(
                SCI_SETFOLDLEVEL,
                line, base + (currentGroupLine >= 0 ? 2 : 1));
            const FindAllResult& result = results.at(resultIndex);
            const QByteArray prefix =
                QStringLiteral("  Line %1:\t").arg(result.lineNo + 1).toUtf8();
            const qintptr lineStart = _findResultView->SendScintillaNpp(
                SCI_POSITIONFROMLINE, line);
            _findResultView->SendScintillaNpp(
                SCI_SETINDICATORCURRENT,
                ScintillaEditView::FIND_MARK_INDICATOR);
            _findResultView->SendScintillaNpp(
                SCI_INDICATORFILLRANGE,
                lineStart + prefix.size() + result.lineMatchStart,
                result.matchLen);
        }
    }
    _findResultDock->show();
    _findResultDock->raise();
}

void MainWindow::onFindAllOpenedDocsRequested(const QString& searchText,
                                              const FindOption& opt)
{
    QList<FindAllResult> allResults;
    QSet<Buffer*> visited;
    DocTabView* const originalActiveTab = _activeDocTab;
    QString scintillaSearchText = searchText;
    if (opt._isRegex && opt._dotMatchesNewline)
        scintillaSearchText.prepend("(?s)");

    auto scanTab = [&](DocTabView* tab) {
        const int originalIndex = tab->currentIndex();
        for (int i = 0; tab && i < tab->count(); ++i) {
            Buffer* buf = tab->bufferAt(i);
            if (!buf || buf->isBinary() || visited.contains(buf))
                continue;
            visited.insert(buf);

            ScintillaEditView* view = activateBufferView(buf, tab);
            if (!view)
                continue;

            qintptr originalStart = 0;
            qintptr originalEnd = 0;
            view->getSelectionNpp(&originalStart, &originalEnd);
            const int originalFirstVisible = view->firstVisibleLine();
            const int originalXOffset = static_cast<int>(view->SendScintilla(
                SCI_GETXOFFSET));
            const qintptr docLen = view->documentLengthNpp();
            view->setCurrentPositionNpp(0);
            bool found = view->findFirst(scintillaSearchText, opt._isRegex, opt._isMatchCase,
                                         opt._isWholeWord, false, true);
            while (found) {
                qintptr matchStart = 0;
                qintptr matchEnd = 0;
                view->getSelectionNpp(&matchStart, &matchEnd);
                if (matchStart >= docLen)
                    break;

                const int lineNo = static_cast<int>(view->SendScintillaNpp(
                    SCI_LINEFROMPOSITION,
                    static_cast<quintptr>(matchStart)));
                QString lineText = view->text(lineNo);
                while (!lineText.isEmpty() &&
                       (lineText.back() == '\n' || lineText.back() == '\r'))
                    lineText.chop(1);

                FindAllResult r;
                r.lineNo = lineNo;
                r.matchStart = matchStart;
                r.matchLen = matchEnd - matchStart;
                const qintptr lineStart = view->SendScintillaNpp(
                    SCI_POSITIONFROMLINE,
                    static_cast<quintptr>(lineNo));
                r.lineMatchStart = matchStart - lineStart;
                r.lineText = lineText;
                r.filePath = buf->isUntitled() ? QString() : buf->getFullPath();
                r.sourceName = buf->getFileName();
                allResults.append(r);

                found = view->findNext();
            }
            view->findFirst(QString(), false, false, false, false, true);
            view->SendScintillaNpp(
                SCI_SETSEL,
                static_cast<quintptr>(originalStart), originalEnd);
            view->SendScintilla(SCI_SETFIRSTVISIBLELINE,
                                static_cast<unsigned long>(originalFirstVisible));
            view->SendScintilla(SCI_SETXOFFSET,
                                static_cast<unsigned long>(originalXOffset));
        }
        if (originalIndex >= 0 && originalIndex < tab->count())
            tab->setCurrentIndex(originalIndex);
    };

    scanTab(_mainDocTab);
    scanTab(_subDocTab);
    setActiveTab(originalActiveTab);
    onFindAllResults(searchText, allResults);
}

void MainWindow::onFindInFilesRequested(const QString& searchText,
                                        const FindOption& opt,
                                        const QString& directory,
                                        const QString& filters,
                                        bool recursive,
                                        bool includeHidden)
{
    QList<FindAllResult> allResults;
    QDir root(directory);
    if (!root.exists()) {
        QMessageBox::warning(this, tr("Find in Files"),
                             tr("Directory does not exist:\n%1").arg(directory));
        onFindAllResults(searchText, allResults);
        return;
    }

    QStringList nameFilters;
    for (const QString& part : filters.split(QRegularExpression("[;\\s]+"), QString::SkipEmptyParts))
        nameFilters << part;
    if (nameFilters.isEmpty())
        nameFilters << "*.*";

    QDir::Filters dirFilters = QDir::Files | QDir::NoDotAndDotDot;
    if (includeHidden)
        dirFilters |= QDir::Hidden;
    QDirIterator::IteratorFlags flags = recursive
        ? QDirIterator::Subdirectories
        : QDirIterator::NoIteratorFlags;

    QDirIterator it(root.absolutePath(), nameFilters, dirFilters, flags);
    QStringList paths;
    while (it.hasNext())
        paths.append(it.next());
    QProgressDialog progress(
        tr("Searching files..."), tr("Cancel"), 0, paths.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(300);
    QSet<QString> visitedPaths;
    for (int pathIndex = 0; pathIndex < paths.size(); ++pathIndex) {
        if (progress.wasCanceled())
            break;
        progress.setValue(pathIndex);
        const QString path = paths.at(pathIndex);
        progress.setLabelText(tr("Searching %1").arg(path));
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        QString canonicalPath = QFileInfo(path).canonicalFilePath();
        if (canonicalPath.isEmpty())
            canonicalPath = QFileInfo(path).absoluteFilePath();
        if (visitedPaths.contains(canonicalPath))
            continue;
        visitedPaths.insert(canonicalPath);
        QFile file(path);
        if (!file.open(QFile::ReadOnly))
            continue;

        const QByteArray bytes = file.readAll();
        file.close();
        const DecodedTextFile decoded =
            TextFileCodec::decode(bytes, decodingOptionsForPath(path));
        if (!decoded.isValid || decoded.isBinary)
            continue;
        allResults.append(findAllInPlainText(decoded.text, searchText, opt, path,
                                             QFileInfo(path).fileName()));
    }
    progress.setValue(paths.size());

    onFindAllResults(searchText, allResults);
}

QStringList MainWindow::projectFiles(int panelMask) const
{
    QStringList files;
    for (int i = 0; i < 3; ++i) {
        if ((panelMask & (1 << i)) == 0 || !_projectPanels[i])
            continue;
        for (const QString& path : _projectPanels[i]->allFiles())
            if (!files.contains(path))
                files.append(path);
    }
    return files;
}

void MainWindow::onFindInProjectsRequested(const QString& searchText,
                                           const FindOption& opt,
                                           const QString& filters,
                                           int panelMask)
{
    QStringList nameFilters = filters.split(
        QRegularExpression(QStringLiteral("[;\\s]+")),
        QString::SkipEmptyParts);
    if (nameFilters.isEmpty())
        nameFilters << QStringLiteral("*.*");
    QList<FindAllResult> results;
    const QStringList files = projectFiles(panelMask);
    QProgressDialog progress(
        tr("Searching project files..."), tr("Cancel"), 0, files.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(300);
    for (int fileIndex = 0; fileIndex < files.size(); ++fileIndex) {
        if (progress.wasCanceled())
            break;
        progress.setValue(fileIndex);
        const QString path = files.at(fileIndex);
        progress.setLabelText(tr("Searching %1").arg(path));
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        if (!QDir::match(nameFilters, QFileInfo(path).fileName()))
            continue;
        QFile file(path);
        if (!file.open(QFile::ReadOnly))
            continue;
        const DecodedTextFile decoded =
            TextFileCodec::decode(file.readAll(), decodingOptionsForPath(path));
        if (!decoded.isValid || decoded.isBinary)
            continue;
        results.append(findAllInPlainText(
            decoded.text, searchText, opt, path, QFileInfo(path).fileName()));
    }
    progress.setValue(files.size());
    onFindAllResults(searchText, results);
}

// ─── 插件系统 ────────────────────────────────────────────────────────────────

void MainWindow::onReplaceAllOpenedDocsRequested(const QString& searchText,
                                                 const QString& replaceText,
                                                 const FindOption& opt)
{
    if (searchText.isEmpty())
        return;
    if (QMessageBox::question(this, tr("Replace All in Opened Documents"),
            tr("Replace all occurrences in every opened document?"))
        != QMessageBox::Yes)
        return;

    int replacementCount = 0;
    int documentCount = 0;
    QSet<Buffer*> visited;
    DocTabView* const originalActiveTab = _activeDocTab;
    QString scintillaSearchText = searchText;
    if (opt._isRegex && opt._dotMatchesNewline)
        scintillaSearchText.prepend("(?s)");
    auto replaceInTab = [&](DocTabView* tab) {
        const int originalIndex = tab->currentIndex();
        for (int i = 0; tab && i < tab->count(); ++i) {
            Buffer* buffer = tab->bufferAt(i);
            if (!buffer || buffer->isReadOnly() || buffer->isBinary() ||
                visited.contains(buffer))
                continue;
            visited.insert(buffer);
            ScintillaEditView* view = activateBufferView(buffer, tab);
            if (!view)
                continue;

            int count = 0;
            qintptr originalStart = 0;
            qintptr originalEnd = 0;
            view->getSelectionNpp(&originalStart, &originalEnd);
            view->beginUndoAction();
            view->setCurrentPositionNpp(0);
            bool found = view->findFirst(scintillaSearchText, opt._isRegex, opt._isMatchCase,
                                         opt._isWholeWord, false, true);
            while (found) {
                qintptr start = 0;
                qintptr end = 0;
                view->getSelectionNpp(&start, &end);
                const qintptr lengthBefore = view->documentLengthNpp();
                view->replace(replaceText);
                const qintptr lengthAfter = view->documentLengthNpp();
                ++count;
                if (start == end) {
                    if (start >= lengthBefore) {
                        found = false;
                    } else {
                        const qintptr shiftedOriginal =
                            start + (lengthAfter - lengthBefore);
                        const qintptr nextPosition = view->SendScintillaNpp(
                            SCI_POSITIONAFTER,
                            static_cast<quintptr>(shiftedOriginal));
                        found = findInViewFromPosition(
                            view, scintillaSearchText, opt, nextPosition);
                    }
                } else {
                    found = view->findNext();
                }
            }
            view->endUndoAction();
            const qintptr documentLength = view->documentLengthNpp();
            view->SendScintillaNpp(
                SCI_SETSEL,
                static_cast<quintptr>(qBound<qintptr>(
                    0, originalStart, documentLength)),
                qBound<qintptr>(0, originalEnd, documentLength));
            if (count > 0) {
                ++documentCount;
                replacementCount += count;
            }
        }
        if (originalIndex >= 0 && originalIndex < tab->count())
            tab->setCurrentIndex(originalIndex);
    };

    replaceInTab(_mainDocTab);
    replaceInTab(_subDocTab);
    setActiveTab(originalActiveTab);
    updateActionStates();
    statusBar()->showMessage(
        tr("%1 replacement(s) made in %2 document(s).")
            .arg(replacementCount).arg(documentCount), 5000);
}

void MainWindow::onReplaceInFilesRequested(const QString& searchText,
                                           const QString& replaceText,
                                           const FindOption& opt,
                                           const QString& directory,
                                           const QString& filters,
                                           bool recursive,
                                           bool includeHidden)
{
    if (searchText.isEmpty())
        return;
    QDir root(directory);
    if (!root.exists()) {
        QMessageBox::warning(this, tr("Replace in Files"),
                             tr("Directory does not exist:\n%1").arg(directory));
        return;
    }
    bool validExpression = true;
    ScintillaTextSearch::findAll(
        QString(), searchText, opt._isRegex, opt._isMatchCase,
        opt._isWholeWord, opt._dotMatchesNewline, &validExpression);
    if (!validExpression) {
        QMessageBox::warning(this, tr("Replace in Files"),
                             tr("Invalid regular expression."));
        return;
    }
    if (QMessageBox::question(this, tr("Replace in Files"),
            tr("Replace all occurrences in files under:\n%1").arg(root.absolutePath()))
        != QMessageBox::Yes)
        return;

    QStringList nameFilters;
    for (const QString& part :
         filters.split(QRegularExpression("[;\\s]+"), QString::SkipEmptyParts))
        nameFilters << part;
    if (nameFilters.isEmpty())
        nameFilters << "*.*";

    QDir::Filters dirFilters = QDir::Files | QDir::NoDotAndDotDot;
    if (includeHidden)
        dirFilters |= QDir::Hidden;
    const QDirIterator::IteratorFlags iteratorFlags = recursive
        ? QDirIterator::Subdirectories
        : QDirIterator::NoIteratorFlags;

    int replacementCount = 0;
    int changedFileCount = 0;
    int skippedDirtyCount = 0;
    int skippedReadOnlyCount = 0;
    int errorCount = 0;
    DocTabView* const originalActiveTab = _activeDocTab;
    const int originalMainIndex = _mainDocTab->currentIndex();
    const int originalSubIndex = _subDocTab->currentIndex();
    QSet<QString> visitedPaths;
    QDirIterator it(root.absolutePath(), nameFilters, dirFilters, iteratorFlags);
    while (it.hasNext()) {
        const QString path = it.next();
        QString canonicalPath = QFileInfo(path).canonicalFilePath();
        if (canonicalPath.isEmpty())
            canonicalPath = QFileInfo(path).absoluteFilePath();
        if (visitedPaths.contains(canonicalPath))
            continue;
        visitedPaths.insert(canonicalPath);
        Buffer* opened = MainFileManager.findBufferByPath(path);
        if (opened && opened->isDirty()) {
            ++skippedDirtyCount;
            continue;
        }
        if ((opened && opened->isReadOnly()) || !QFileInfo(path).isWritable()) {
            ++skippedReadOnlyCount;
            continue;
        }

        QFile input(path);
        if (!input.open(QFile::ReadOnly)) {
            ++errorCount;
            continue;
        }
        const QByteArray original = input.readAll();
        input.close();
        const DecodedTextFile decoded =
            TextFileCodec::decode(original, decodingOptionsForPath(path));
        if (!decoded.isValid || decoded.isBinary)
            continue;

        QString text = decoded.text;
        const int count = replacePlainText(text, searchText, replaceText, opt);
        if (count <= 0) {
            if (count < 0)
                ++errorCount;
            continue;
        }

        if (opened) {
            ScintillaEditView* view = activateBufferView(opened);
            if (!view) {
                ++errorCount;
                continue;
            }
            view->beginUndoAction();
            view->selectAll();
            view->replaceSelectedText(text);
            view->endUndoAction();
            if (!doSave(opened, path)) {
                ++errorCount;
                continue;
            }
        } else {
            QSaveFile output(path);
            if (!output.open(QFile::WriteOnly)) {
                ++errorCount;
                continue;
            }
            QByteArray encoded;
            QString encodeError;
            if (!TextFileCodec::encode(text, decoded.encoding, decoded.hasBom,
                                       &encoded, &encodeError) ||
                output.write(encoded) != encoded.size() || !output.commit()) {
                output.cancelWriting();
                ++errorCount;
                continue;
            }
        }
        replacementCount += count;
        ++changedFileCount;
    }

    if (originalMainIndex >= 0 && originalMainIndex < _mainDocTab->count())
        _mainDocTab->setCurrentIndex(originalMainIndex);
    if (originalSubIndex >= 0 && originalSubIndex < _subDocTab->count())
        _subDocTab->setCurrentIndex(originalSubIndex);
    setActiveTab(originalActiveTab);

    QMessageBox::information(this, tr("Replace in Files"),
        tr("%1 replacement(s) made in %2 file(s).\n"
           "Skipped dirty opened files: %3\n"
           "Skipped read-only files: %4\nErrors: %5")
            .arg(replacementCount).arg(changedFileCount)
            .arg(skippedDirtyCount).arg(skippedReadOnlyCount).arg(errorCount));
}

void MainWindow::onReplaceInProjectsRequested(const QString& searchText,
                                              const QString& replaceText,
                                              const FindOption& opt,
                                              const QString& filters,
                                              int panelMask)
{
    bool validExpression = true;
    ScintillaTextSearch::findAll(
        QString(), searchText, opt._isRegex, opt._isMatchCase,
        opt._isWholeWord, opt._dotMatchesNewline, &validExpression);
    if (!validExpression) {
        QMessageBox::warning(this, tr("Replace in Projects"),
                             tr("Invalid regular expression."));
        return;
    }
    const QStringList selectedFiles = projectFiles(panelMask);
    if (QMessageBox::question(
            this, tr("Replace in Projects"),
            tr("Replace all occurrences in %1 project file(s)?")
                .arg(selectedFiles.size())) != QMessageBox::Yes)
        return;

    QStringList nameFilters = filters.split(
        QRegularExpression(QStringLiteral("[;\\s]+")),
        QString::SkipEmptyParts);
    if (nameFilters.isEmpty())
        nameFilters << QStringLiteral("*.*");
    int replacements = 0;
    int changedFiles = 0;
    int skipped = 0;
    int errors = 0;
    DocTabView* const originalActiveTab = _activeDocTab;
    const int originalMainIndex = _mainDocTab->currentIndex();
    const int originalSubIndex = _subDocTab->currentIndex();
    for (const QString& path : selectedFiles) {
        if (!QDir::match(nameFilters, QFileInfo(path).fileName()))
            continue;
        Buffer* opened = MainFileManager.findBufferByPath(path);
        if ((opened && (opened->isDirty() || opened->isReadOnly())) ||
            !QFileInfo(path).isWritable()) {
            ++skipped;
            continue;
        }
        QFile input(path);
        if (!input.open(QFile::ReadOnly)) {
            ++errors;
            continue;
        }
        const DecodedTextFile decoded =
            TextFileCodec::decode(input.readAll(), decodingOptionsForPath(path));
        if (!decoded.isValid || decoded.isBinary)
            continue;
        QString changed = decoded.text;
        const int count =
            replacePlainText(changed, searchText, replaceText, opt);
        if (count <= 0) {
            if (count < 0)
                ++errors;
            continue;
        }
        if (opened) {
            ScintillaEditView* view = activateBufferView(opened);
            if (!view) {
                ++errors;
                continue;
            }
            view->beginUndoAction();
            view->selectAll();
            view->replaceSelectedText(changed);
            view->endUndoAction();
            if (!doSave(opened, path)) {
                ++errors;
                continue;
            }
        } else {
            QByteArray encoded;
            QString encodingError;
            if (!TextFileCodec::encode(changed, decoded.encoding, decoded.hasBom,
                                       &encoded, &encodingError)) {
                ++errors;
                continue;
            }
            QSaveFile output(path);
            if (!output.open(QFile::WriteOnly) ||
                output.write(encoded) != encoded.size() || !output.commit()) {
                output.cancelWriting();
                ++errors;
                continue;
            }
        }
        replacements += count;
        ++changedFiles;
    }
    if (originalMainIndex >= 0 && originalMainIndex < _mainDocTab->count())
        _mainDocTab->setCurrentIndex(originalMainIndex);
    if (originalSubIndex >= 0 && originalSubIndex < _subDocTab->count())
        _subDocTab->setCurrentIndex(originalSubIndex);
    setActiveTab(originalActiveTab);
    QMessageBox::information(
        this, tr("Replace in Projects"),
        tr("%1 replacement(s) made in %2 file(s).\n"
           "Skipped dirty/read-only files: %3\nErrors: %4")
            .arg(replacements).arg(changedFiles).arg(skipped).arg(errors));
}

void MainWindow::setupPluginSystem()
{
    const QString pluginDir =
        QDir(NppParameters::getInstance().getNppPath())
            .filePath(QStringLiteral("plugins"));
#ifdef Q_OS_WIN
    // Temporary compatibility-test filter. Remove it after the Win32 message
    // router supports the broader plugin corpus.
    QStringList win32PluginErrors;
    _win32PluginManager->loadPlugins(
        pluginDir, {QStringLiteral("mimeTools")}, &win32PluginErrors);
    for (const QString& error : win32PluginErrors)
        qWarning() << "Win32 plugin load failed:" << error;
#endif
#ifdef ENABLE_PLUGIN_SYSTEM
    _pluginManager = new PluginManager(this);
    _pluginManager->loadPlugins(pluginDir, this);
#endif
}

void MainWindow::showPluginAdmin()
{
    const QString pluginRoot =
        QDir(NppParameters::getInstance().getNppPath())
            .filePath(QStringLiteral("plugins"));
    PluginCatalog catalog = PluginCatalog::embedded();
    if (!catalog.isValid()) {
        QMessageBox::warning(
            this, tr("Plugins Admin"),
            tr("The built-in plugin list is invalid."));
        return;
    }

    PluginAdminModel model(
        pluginRoot, catalog,
        PluginVersion(QCoreApplication::applicationVersion()));
    PluginAdminDialog dialog(&model, this);
    dialog.applyLocalization(
        NppParameters::getInstance().getNativeLangSpeaker());
    if (dialog.exec() != QDialog::Accepted)
        return;
    if (!schedulePluginOperations(dialog.selectedOperations()))
        return;
    QTimer::singleShot(0, this, &QWidget::close);
}

bool MainWindow::schedulePluginOperations(
    const QVector<PluginOperation>& operations)
{
    if (operations.isEmpty())
        return false;

    PluginUpdatePlan plan;
    plan.applicationPath = QCoreApplication::applicationFilePath();
    plan.pluginRoot =
        QDir(NppParameters::getInstance().getNppPath())
            .filePath(QStringLiteral("plugins"));
    plan.operations = operations;

    const QString planPath =
        QDir(NppParameters::getInstance().getUserPath())
            .filePath(QStringLiteral(
                "plugins/Config/pending-plugin-update.json"));
    QString error;
    if (!plan.write(planPath, &error)) {
        QMessageBox::warning(
            this, tr("Plugins Admin"),
            tr("Could not create the plugin update plan:\n%1")
                .arg(error));
        return false;
    }
    _pendingPluginUpdatePlan = planPath;
    return true;
}

bool MainWindow::launchPendingPluginUpdater(QString* error)
{
    if (_pendingPluginUpdatePlan.isEmpty()) {
        if (error)
            *error = tr("No plugin update plan is pending.");
        return false;
    }

    QString updaterName = QStringLiteral("npp-plugin-updater");
#if defined(Q_OS_WIN)
    updaterName += QStringLiteral(".exe");
#endif
    const QString applicationDirectory =
        QCoreApplication::applicationDirPath();
    const QString updaterPath =
        QDir(applicationDirectory).filePath(updaterName);
    if (!QFileInfo::exists(updaterPath)) {
        if (error) {
            *error = tr("Updater executable was not found: %1")
                         .arg(QDir::toNativeSeparators(updaterPath));
        }
        return false;
    }
    QString planError;
    const PluginUpdatePlan plan = PluginUpdatePlan::read(
        _pendingPluginUpdatePlan, &planError);
    if (!planError.isEmpty()) {
        if (error)
            *error = planError;
        return false;
    }
    QString writableDirectory = plan.pluginRoot;
    while (!QFileInfo(writableDirectory).exists()) {
        const QString parent = QFileInfo(writableDirectory).absolutePath();
        if (parent == writableDirectory)
            break;
        writableDirectory = parent;
    }
    QTemporaryFile writeProbe(QDir(writableDirectory).filePath(
        QStringLiteral(".npp-plugin-write-test-XXXXXX")));
    const bool requiresElevation = !writeProbe.open();

    QStringList arguments = {
        QStringLiteral("--plan"), _pendingPluginUpdatePlan,
        QStringLiteral("--wait-pid"),
        QString::number(QCoreApplication::applicationPid())
    };
    QFile planFile(_pendingPluginUpdatePlan);
    if (!planFile.open(QFile::ReadOnly)) {
        if (error)
            *error = planFile.errorString();
        return false;
    }
    arguments.append(QStringLiteral("--plan-sha256"));
    arguments.append(QString::fromLatin1(
        QCryptographicHash::hash(planFile.readAll(),
                                 QCryptographicHash::Sha256).toHex()));
#if defined(Q_OS_WIN)
    if (requiresElevation) {
        arguments.append(QStringLiteral("--restart-unelevated"));
        return PlatformServices::startElevated(
            updaterPath, arguments, applicationDirectory, error);
    }
#else
    if (requiresElevation) {
        if (error) {
            *error = tr("The plugin directory is not writable: %1")
                         .arg(QDir::toNativeSeparators(plan.pluginRoot));
        }
        return false;
    }
#endif
    if (!QProcess::startDetached(
            updaterPath, arguments, applicationDirectory)) {
        if (error)
            *error = tr("The updater process could not be created.");
        return false;
    }
    return true;
}

void MainWindow::watchBufferFile(Buffer* buf)
{
    if (!_fileWatcher || !buf || buf->isUntitled())
        return;

    QFileInfo info(buf->getFullPath());
    QString path = info.canonicalFilePath();
    if (path.isEmpty())
        path = info.absoluteFilePath();
    if (path.isEmpty() || !info.exists())
        return;

    buf->setLastKnownModificationTime(info.lastModified());
    if (!_fileWatcher->files().contains(path))
        _fileWatcher->addPath(path);
    const QString directory = info.absolutePath();
    if (!_fileWatcher->directories().contains(directory))
        _fileWatcher->addPath(directory);
}

void MainWindow::unwatchBufferFile(Buffer* buf)
{
    if (!_fileWatcher || !buf || buf->isUntitled())
        return;

    QFileInfo info(buf->getFullPath());
    QString path = info.canonicalFilePath();
    if (path.isEmpty())
        path = info.absoluteFilePath();
    if (!path.isEmpty() && _fileWatcher->files().contains(path))
        _fileWatcher->removePath(path);
}

void MainWindow::onWatchedFileChanged(const QString& path)
{
    QFileInfo info(path);
    QString canonical = info.canonicalFilePath();
    if (canonical.isEmpty())
        canonical = info.absoluteFilePath();

    Buffer* buf = MainFileManager.findBufferByPath(path);
    if (!buf)
        return;

    if (_savingPaths.contains(canonical)) {
        buf->setLastKnownModificationTime(info.lastModified());
        if (info.exists() && !_fileWatcher->files().contains(canonical))
            _fileWatcher->addPath(canonical);
        return;
    }

    if (!info.exists()) {
        if (_dismissedMissingPaths.contains(canonical))
            return;
        QMessageBox message(QMessageBox::Warning, tr("File Deleted"),
            tr("File \"%1\" no longer exists on disk.")
                .arg(buf->getFileName()), QMessageBox::NoButton, this);
        QPushButton* saveButton = message.addButton(
            tr("Save"), QMessageBox::AcceptRole);
        QPushButton* closeButton = message.addButton(
            tr("Close"), QMessageBox::DestructiveRole);
        QPushButton* keepButton = message.addButton(
            tr("Keep Open"), QMessageBox::RejectRole);
        message.setDefaultButton(saveButton);
        message.exec();
        if (message.clickedButton() == saveButton) {
            doSave(buf, buf->getFullPath());
        } else if (message.clickedButton() == closeButton) {
            buf->setDirty(false);
            onBufferCloseRequested(buf);
        } else if (message.clickedButton() == keepButton) {
            _dismissedMissingPaths.insert(canonical);
            buf->setDirty(true);
            _mainDocTab->updateTabTitle(buf);
            _subDocTab->updateTabTitle(buf);
            updateWindowTitle(buf);
        }
        return;
    }
    _dismissedMissingPaths.remove(canonical);

    const bool readOnly =
        !info.isWritable() || buf->isCommandLineReadOnly() ||
        buf->isMonitoring();
    if (buf->isReadOnly() != readOnly) {
        buf->setReadOnly(readOnly);
        for (DocTabView* tab : {_mainDocTab, _subDocTab})
            if (tab->currentBuffer() == buf)
                tab->editor()->setReadOnly(readOnly);
        _mainDocTab->updateTabTitle(buf);
        _subDocTab->updateTabTitle(buf);
        statusBar()->showMessage(
            readOnly
                ? tr("File became read-only: %1").arg(buf->getFileName())
                : tr("File is writable again: %1").arg(buf->getFileName()),
            4000);
    }

    if (buf->lastKnownModificationTime().isValid() &&
        info.lastModified() <= buf->lastKnownModificationTime()) {
        if (!_fileWatcher->files().contains(canonical))
            _fileWatcher->addPath(canonical);
        return;
    }

    if (buf->isMonitoring()) {
        if (_mainDocTab->indexOfBuffer(buf) != -1) {
            setActiveTab(_mainDocTab);
            _mainDocTab->activateBuffer(buf);
        } else if (_subDocTab->indexOfBuffer(buf) != -1) {
            setActiveTab(_subDocTab);
            _subDocTab->activateBuffer(buf);
        }
        const bool wasReadOnly = buf->isReadOnly();
        buf->setReadOnly(false);
        for (DocTabView* tab : {_mainDocTab, _subDocTab})
            if (tab->currentBuffer() == buf)
                tab->editor()->setReadOnly(false);
        reloadFromDisk();
        buf->setReadOnly(wasReadOnly);
        for (DocTabView* tab : {_mainDocTab, _subDocTab})
            if (tab->currentBuffer() == buf)
                tab->editor()->setReadOnly(wasReadOnly);
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("File Modified"),
        tr("File \"%1\" has been modified by another program.\nReload from disk?")
            .arg(buf->getFileName()),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (_mainDocTab->indexOfBuffer(buf) != -1) {
            setActiveTab(_mainDocTab);
            _mainDocTab->activateBuffer(buf);
        } else if (_subDocTab->indexOfBuffer(buf) != -1) {
            setActiveTab(_subDocTab);
            _subDocTab->activateBuffer(buf);
        }
        reloadFromDisk();
    } else {
        buf->setLastKnownModificationTime(info.lastModified());
        if (!_fileWatcher->files().contains(canonical))
            _fileWatcher->addPath(canonical);
    }
}

void MainWindow::onWatchedDirectoryChanged(const QString& path)
{
    QDir directory(path);
    if (!directory.exists())
        return;

    const QVector<Buffer*> buffers = MainFileManager.buffers();
    for (Buffer* buffer : buffers) {
        if (!buffer || buffer->isUntitled())
            continue;
        const QFileInfo oldInfo(buffer->getFullPath());
        if (QDir::cleanPath(oldInfo.absolutePath())
                != QDir::cleanPath(directory.absolutePath())
            || oldInfo.exists()) {
            continue;
        }

        QStringList candidates;
        const QFileInfoList entries = directory.entryInfoList(
            QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
        for (const QFileInfo& candidate : entries) {
            if (buffer->sourceFileSize() >= 0
                && candidate.size() != buffer->sourceFileSize()) {
                continue;
            }
            const QDateTime known = buffer->lastKnownModificationTime();
            if (known.isValid()
                && qAbs(known.msecsTo(candidate.lastModified())) > 2000) {
                continue;
            }
            candidates.append(candidate.absoluteFilePath());
        }
        if (candidates.size() != 1) {
            onWatchedFileChanged(buffer->getFullPath());
            continue;
        }

        const QString newPath = candidates.first();
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this, tr("File Renamed"),
            tr("File \"%1\" appears to have been renamed to \"%2\".\n"
               "Update the opened document path?")
                .arg(buffer->getFileName(), QFileInfo(newPath).fileName()),
            QMessageBox::Yes | QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            onWatchedFileChanged(buffer->getFullPath());
            continue;
        }

        buffer->setFilePath(newPath);
        buffer->setLastKnownModificationTime(
            QFileInfo(newPath).lastModified());
        _mainDocTab->updateTabTitle(buffer);
        _subDocTab->updateTabTitle(buffer);
        updateWindowTitle(buffer);
        watchBufferFile(buffer);
    }
}

void MainWindow::pollWatchedFiles()
{
    const QVector<Buffer*> buffers = MainFileManager.buffers();
    for (Buffer* buffer : buffers) {
        if (!buffer || buffer->isUntitled())
            continue;
        const QFileInfo info(buffer->getFullPath());
        const bool expectedReadOnly =
            !info.isWritable() || buffer->isCommandLineReadOnly() ||
            buffer->isMonitoring();
        const bool permissionChanged =
            info.exists() && buffer->isReadOnly() != expectedReadOnly;
        const bool contentChanged =
            info.exists() && buffer->lastKnownModificationTime().isValid()
            && info.lastModified() > buffer->lastKnownModificationTime();
        if (!info.exists() || permissionChanged || contentChanged)
            onWatchedFileChanged(buffer->getFullPath());
    }
}

// ─── 最近文件 ────────────────────────────────────────────────────────────────

void MainWindow::addToRecentFiles(const QString& filePath)
{
    NppGUI& gui = NppParameters::getInstance().getNppGUI();
    gui._recentFileList.removeAll(filePath);
    gui._recentFileList.prepend(filePath);
    while (gui._recentFileList.size() > gui._nbMaxRecentFile)
        gui._recentFileList.removeLast();
    updateRecentFilesMenu();
}

void MainWindow::updateRecentFilesMenu()
{
    if (!_recentFilesMenu) return;
    _recentFilesMenu->clear();

    const QStringList& recent = NppParameters::getInstance().getNppGUI()._recentFileList;

    for (const QString& path : recent) {
        QAction* act = _recentFilesMenu->addAction(path);
        connect(act, &QAction::triggered, this, &MainWindow::openRecentFile);
    }

    if (!recent.isEmpty()) {
        _recentFilesMenu->addSeparator();
        QAction* clearAct = _recentFilesMenu->addAction(tr("Clear Recent Files"));
        clearAct->setObjectName("clearRecentFilesAction");
        connect(clearAct, &QAction::triggered, this, &MainWindow::clearRecentFiles);
        const QString translated =
            NppParameters::getInstance().getNativeLangSpeaker()
                .getNativeLangMenuString("clearRecentFilesAction");
        if (!translated.isEmpty())
            clearAct->setText(translated);
    }

    _recentFilesMenu->setEnabled(!recent.isEmpty());
}

void MainWindow::openRecentFile()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if (act) doOpenFile(act->text(), _activeDocTab);
}

void MainWindow::clearRecentFiles()
{
    NppParameters::getInstance().getNppGUI()._recentFileList.clear();
    updateRecentFilesMenu();
}

void MainWindow::rememberClosedFile(const QString& filePath)
{
    _closedFileHistory.remember(filePath);
    if (_restoreLastClosedAction)
        _restoreLastClosedAction->setEnabled(!_closedFileHistory.isEmpty());
}

void MainWindow::restoreLastClosedFile()
{
    for (;;) {
        const QString path = _closedFileHistory.takeNextExisting();
        if (path.isEmpty())
            break;
        if (doOpenFile(path, _activeDocTab))
            break;
    }
    if (_restoreLastClosedAction)
        _restoreLastClosedAction->setEnabled(!_closedFileHistory.isEmpty());
}

// ─── 会话 ────────────────────────────────────────────────────────────────────

void MainWindow::saveSession()
{
    NppParameters& params = NppParameters::getInstance();
    Session session;
    DocTabView* const originalActiveTab = _activeDocTab;

    if (_documentMapBuffer && _docMapPanel)
        _documentMapBuffer->setMapState(_docMapPanel->sessionState());

    // 主视图
    session._activeView = (_activeDocTab == _mainDocTab) ? 0 : 1;
    auto collectFiles = [this](DocTabView* tab,
                           std::vector<sessionFileInfo>& out) -> size_t
    {
        size_t activeOutputIndex = 0;
        const int originalIndex = tab->currentIndex();
        for (int i = 0; i < tab->count(); ++i) {
            Buffer* buf = tab->bufferAt(i);
            if (!buf) continue;
            // untitled 文件：只有存在备份路径时才记录（用于崩溃恢复）
            if (buf->isUntitled() && buf->getBackupFilePath().isEmpty()) continue;

            sessionFileInfo sfi(buf->isUntitled()
                ? buf->getFileName()      // "new 1" 等虚拟名
                : buf->getFullPath());
            ScintillaEditView* view = activateBufferView(buf, tab);
            const QString language = view ? view->lexerLanguage() : QString();
            sfi._langName = language.isEmpty() ? "Normal Text" : language;
            sfi._encoding = sessionEncodingForBuffer(buf);
            sfi._userReadOnly = buf->isReadOnly() ||
                                (view && view->isReadOnly());
            sfi._individualTabColour = buf->individualTabColour();
            const BufferMapState& mapState = buf->mapState();
            sfi._mapFirstVisibleDisplayLine =
                mapState.firstVisibleDisplayLine;
            sfi._mapFirstVisibleDocLine = mapState.firstVisibleDocLine;
            sfi._mapLastVisibleDocLine = mapState.lastVisibleDocLine;
            sfi._mapNbLine = mapState.lineCount;
            sfi._mapHigherPos = mapState.higherPosition;
            sfi._mapWidth = mapState.width;
            sfi._mapHeight = mapState.height;
            sfi._mapKByteInDoc = mapState.kBytesInDocument;
            sfi._mapWrapIndentMode = mapState.wrapIndentMode;
            sfi._mapIsWrap = mapState.isWrap;

            if (view) {
                sfi._startPos = view->SendScintillaNpp(
                    SCI_GETSELECTIONSTART);
                sfi._endPos = view->SendScintillaNpp(
                    SCI_GETSELECTIONEND);
                sfi._firstVisibleLine = view->firstVisibleLine();
                sfi._xOffset          = view->SendScintilla(
                    SCI_GETXOFFSET);
                sfi._scrollWidth      = view->SendScintilla(
                    SCI_GETSCROLLWIDTH);
                sfi._selMode          = view->SendScintilla(
                    SCI_GETSELECTIONMODE);

                int markedLine = (int)view->SendScintilla(
                    SCI_MARKERNEXT, 0UL, 1L << 1);
                while (markedLine >= 0) {
                    sfi._marks.push_back(static_cast<size_t>(markedLine));
                    markedLine = (int)view->SendScintilla(
                        SCI_MARKERNEXT,
                        static_cast<unsigned long>(markedLine + 1), 1L << 1);
                }
                if (!buf->isLargeFile()) {
                    for (int line = 0; line < view->lines(); ++line) {
                        const int level = (int)view->SendScintilla(
                            SCI_GETFOLDLEVEL,
                            static_cast<unsigned long>(line));
                        if ((level &
                             SC_FOLDLEVELHEADERFLAG) &&
                            !view->SendScintilla(
                                SCI_GETFOLDEXPANDED,
                                static_cast<unsigned long>(line))) {
                            sfi._foldStates.push_back(
                                static_cast<size_t>(line));
                        }
                    }
                }
            }
            if (!buf->isUntitled()) {
                QFileInfo fi(buf->getFullPath());
                if (fi.exists()) {
                    toSessionFileTime(
                        fi.lastModified(),
                        &sfi._originalFileLastModifTimestamp,
                        &sfi._originalFileLastModifTimestampHigh);
                }
            }
            // 记录备份文件路径（用于崩溃恢复）
            sfi._backupFilePath = buf->getBackupFilePath();
            if (i == tab->currentIndex())
                activeOutputIndex = out.size();
            out.push_back(std::move(sfi));
        }
        if (originalIndex >= 0 && originalIndex < tab->count())
            tab->setCurrentIndex(originalIndex);
        return activeOutputIndex;
    };

    session._activeMainIndex =
        collectFiles(_mainDocTab, session._mainViewFiles);
    if (_subDocTab->isVisible())
        session._activeSubIndex =
            collectFiles(_subDocTab, session._subViewFiles);
    if (_fileBrowserPanel && !_fileBrowserPanel->rootPath().isEmpty())
        session._fileBrowserRoots.push_back(_fileBrowserPanel->rootPath());
    if (_fileBrowserPanel)
        session._fileBrowserSelectedItem = _fileBrowserPanel->selectedPath();

    setActiveTab(originalActiveTab);

    params.getSession() = session;
    params.writeSession(session);
}

void MainWindow::restoreSession()
{
    const Session& session = NppParameters::getInstance().getSession();
    if (!session._subViewFiles.empty())
        showSubView();

    auto decodeSessionBackup = [](const QByteArray& data,
                                  const sessionFileInfo& sfi) {
        const bool hasUnicodeBom =
            data.startsWith("\xEF\xBB\xBF") ||
            data.startsWith("\xFF\xFE") ||
            data.startsWith("\xFE\xFF");
        const QString codec =
            EncodingMapper::codecNameForCodePage(sfi._encoding);
        if (codec.isEmpty() || hasUnicodeBom) {
            return TextFileCodec::decode(
                data, decodingOptionsForPath(sfi._fileName));
        }
        return TextFileCodec::decodeAs(data, codec);
    };

    auto restoreFile = [&](const sessionFileInfo& sfi, DocTabView* tab) {
        bool hasBackup = !sfi._backupFilePath.isEmpty() &&
                         QFile::exists(sfi._backupFilePath);
        bool fileExists = QFile::exists(sfi._fileName);

        Buffer* buf = nullptr;

        if (fileExists) {
            // 正常打开磁盘文件
            Buffer* existing = MainFileManager.findBufferByPath(sfi._fileName);
            if (existing && tab->indexOfBuffer(existing) == -1) {
                tab->addClone(existing, tab->editor());
                buf = existing;
            } else {
                const QString sessionCodec =
                    EncodingMapper::codecNameForCodePage(sfi._encoding);
                if (!doOpenFile(sfi._fileName, tab, sessionCodec)) return;
                buf = tab->currentBuffer();
            }

            // 有备份说明上次未保存，从备份恢复内容并标脏
            if (buf && hasBackup && !buf->isDirty() &&
                !buf->isLargeFile()) {
                QFile f(sfi._backupFilePath);
                if (f.open(QFile::ReadOnly)) {
                    const DecodedTextFile decoded =
                        decodeSessionBackup(f.readAll(), sfi);
                    f.close();
                    ScintillaEditView* backupView =
                        activateBufferView(buf, tab);
                    const bool readOnly = backupView && backupView->isReadOnly();
                    if (backupView)
                        backupView->setReadOnly(false);
                    if (backupView && decoded.isValid)
                        backupView->setText(decoded.text);
                    if (backupView)
                        backupView->SendScintilla(
                        SCI_SETSAVEPOINT); // 不算 undo
                    if (backupView)
                        backupView->setReadOnly(readOnly);
                    if (decoded.isValid) {
                        buf->setEncoding(decoded.encoding);
                        buf->setHasBom(decoded.hasBom);
                        buf->setUsesEncodingCookie(
                            decoded.usesEncodingCookie);
                        buf->setBinary(decoded.isBinary);
                        buf->setEolMode(decoded.eolMode);
                        buf->setDirty(true);
                        tab->updateTabTitle(buf);
                    }
                }
            }
        } else if (hasBackup) {
            // untitled 文件或已被删除的文件：从备份恢复为新 buffer
            buf = doNewBuffer(tab);
            if (!buf) return;
            const qint64 backupSize =
                QFileInfo(sfi._backupFilePath).size();
            if (!confirmHugeFileOpen(this, backupSize))
                return;
            buf->setSourceFileSize(backupSize);
            buf->setLargeFile(Buffer::isLargeFileSize(backupSize));
            const QString sessionCodec =
                EncodingMapper::codecNameForCodePage(sfi._encoding);
            QString loadError;
            ScintillaEditView* backupView = activateBufferView(buf, tab);
            if (backupView && MainFileManager.loadBufferContent(
                    buf, backupView,
                    decodingOptionsForPath(sfi._fileName), sessionCodec,
                    &loadError, sfi._backupFilePath)) {
                if (buf->document() !=
                    static_cast<qintptr>(backupView->document())) {
                    buf->releaseDocument();
                    captureBufferDocument(buf, backupView);
                }
                buf->setDirty(true);
                tab->updateTabTitle(buf);
            }
        } else {
            return; // 文件和备份都不存在，跳过
        }

        if (!buf) return;

        tab->setIndividualTabColour(buf, sfi._individualTabColour);
        BufferMapState mapState;
        mapState.firstVisibleDisplayLine =
            sfi._mapFirstVisibleDisplayLine;
        mapState.firstVisibleDocLine = sfi._mapFirstVisibleDocLine;
        mapState.lastVisibleDocLine = sfi._mapLastVisibleDocLine;
        mapState.lineCount = sfi._mapNbLine;
        mapState.higherPosition = sfi._mapHigherPos;
        mapState.width = sfi._mapWidth;
        mapState.height = sfi._mapHeight;
        mapState.kBytesInDocument = sfi._mapKByteInDoc;
        mapState.wrapIndentMode = sfi._mapWrapIndentMode;
        mapState.isWrap = sfi._mapIsWrap;
        buf->setMapState(mapState);

        // 恢复光标位置
        ScintillaEditView* view = activateBufferView(buf, tab);
        if (view) {
            const qintptr documentLength = view->documentLengthNpp();
            const qintptr selectionStart = qBound<qintptr>(
                0, static_cast<qintptr>(sfi._startPos), documentLength);
            const qintptr selectionEnd = qBound<qintptr>(
                0, static_cast<qintptr>(sfi._endPos), documentLength);
            view->SendScintilla(SCI_SETSELECTIONMODE,
                                static_cast<unsigned long>(sfi._selMode));
            view->SendScintillaNpp(
                SCI_SETSEL,
                static_cast<quintptr>(selectionStart), selectionEnd);
            view->SendScintilla(SCI_SETFIRSTVISIBLELINE,
                                static_cast<unsigned long>(sfi._firstVisibleLine));
            view->SendScintilla(SCI_SETXOFFSET,
                                static_cast<unsigned long>(sfi._xOffset));
            if (sfi._scrollWidth > 1) {
                view->SendScintilla(SCI_SETSCROLLWIDTH,
                                    static_cast<unsigned long>(sfi._scrollWidth));
            }
            for (size_t line : sfi._marks) {
                if (line < static_cast<size_t>(view->lines()))
                    view->markerAdd(static_cast<int>(line), 1);
            }
            for (size_t line : sfi._foldStates) {
                if (line >= static_cast<size_t>(view->lines()))
                    continue;
                const bool expanded = view->SendScintilla(
                    SCI_GETFOLDEXPANDED,
                    static_cast<unsigned long>(line));
                if (expanded)
                    view->foldLine(static_cast<int>(line));
            }
            view->setReadOnly(sfi._userReadOnly || buf->isReadOnly());
        }
        buf->setReadOnly(sfi._userReadOnly || buf->isReadOnly());
        const QDateTime sessionModificationTime = fromSessionFileTime(
            sfi._originalFileLastModifTimestamp,
            sfi._originalFileLastModifTimestampHigh);
        if (sessionModificationTime.isValid())
            buf->setLastKnownModificationTime(sessionModificationTime);
        // 记住备份路径，下次定时器继续写到同一个文件
        if (hasBackup && !buf->isLargeFile())
            buf->setBackupFilePath(sfi._backupFilePath);
    };

    for (const auto& sfi : session._mainViewFiles)
        restoreFile(sfi, _mainDocTab);

    for (const auto& sfi : session._subViewFiles)
        restoreFile(sfi, _subDocTab);

    if (_fileBrowserPanel && !session._fileBrowserRoots.empty())
        _fileBrowserPanel->setRootPath(session._fileBrowserRoots.front());
    if (_fileBrowserPanel && !session._fileBrowserSelectedItem.isEmpty())
        _fileBrowserPanel->setSelectedPath(
            session._fileBrowserSelectedItem);

    // 恢复活动 tab 索引（与原版一致）
    int activeMain = static_cast<int>(session._activeMainIndex);
    if (activeMain >= 0 && activeMain < _mainDocTab->count())
        _mainDocTab->setCurrentIndex(activeMain);
    else if (_mainDocTab->count() > 0)
        _mainDocTab->setCurrentIndex(_mainDocTab->count() - 1);

    int activeSub = static_cast<int>(session._activeSubIndex);
    if (activeSub >= 0 && activeSub < _subDocTab->count())
        _subDocTab->setCurrentIndex(activeSub);

    // 恢复活动视图
    if (session._activeView == 1 && _subDocTab->isVisible())
        setActiveTab(_subDocTab);
    else
        setActiveTab(_mainDocTab);
    syncDocumentMap();
}

// ─── 偏好设置 ────────────────────────────────────────────────────────────────

void MainWindow::applyPreferencesToView(ScintillaEditView* view)
{
    if (!view) return;
    const NppParameters& params = NppParameters::getInstance();
    const NppGUI& gui = params.getNppGUI();
    const ScintillaViewParams& svp = params.getSVP();

    Buffer* buffer = MainFileManager.findBufferByView(view);
    const bool largeFile = buffer && buffer->isLargeFile();
    view->setLargeFileMode(largeFile);
    view->setBorderEdge(svp._showBorderEdge, gui._darkModeEnabled);
    view->applyFont(gui._editorFontName, gui._editorFontSize);
    view->applyTabSettings(gui._tabSize, gui._tabReplacedBySpace);
    view->setAutoIndent(gui._autoIndent);
    view->applyWordWrap(!largeFile && svp._doWrap);
    view->applyAutoComplete(
        !largeFile && gui._autoCompleteEnable, gui._autoCompleteThreshold);
    view->applyShowWhitespace(svp._whiteSpaceShow);
    view->applyShowEol(svp._eolShow);
    view->applyShowIndentGuide(svp._indentGuideLineShow);
    applyScintillaShortcuts(view);
    view->refreshUrlHotspots();
    view->refreshXmlTagHighlight();
}

void MainWindow::applyPreferencesToAllViews()
{
    applyDarkMode();

    const ScintillaViewParams& svp =
        NppParameters::getInstance().getSVP();
    _mainDocTab->setEditorBorderWidth(svp._borderWidth);
    _subDocTab->setEditorBorderWidth(svp._borderWidth);

    applyPreferencesToView(_mainDocTab->editor());
    applyPreferencesToView(_subDocTab->editor());
    updateStatusBar();

    const NppGUI& gui = NppParameters::getInstance().getNppGUI();
    if (_backupTimer) {
        _backupTimer->setInterval(qMax(1000, gui._snapshotBackupTiming));
        if (gui._isSnapshotMode && !_backupTimer->isActive())
            _backupTimer->start();
        else if (!gui._isSnapshotMode && _backupTimer->isActive())
            _backupTimer->stop();
    }

    // 对应原版：偏好设置变更后重新应用界面语言翻译（支持运行时热切换）
    applyNativeLang();
}

void MainWindow::applyDarkMode()
{
    const bool dark = NppParameters::getInstance().getNppGUI()._darkModeEnabled;
    if (!dark) {
        qApp->setPalette(qApp->style()->standardPalette());
        qApp->setStyleSheet(QString());
        return;
    }

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(32, 32, 32));
    palette.setColor(QPalette::WindowText, QColor(224, 224, 224));
    palette.setColor(QPalette::Base, QColor(24, 24, 24));
    palette.setColor(QPalette::AlternateBase, QColor(38, 38, 38));
    palette.setColor(QPalette::ToolTipBase, QColor(48, 48, 48));
    palette.setColor(QPalette::ToolTipText, QColor(240, 240, 240));
    palette.setColor(QPalette::Text, QColor(224, 224, 224));
    palette.setColor(QPalette::Button, QColor(45, 45, 48));
    palette.setColor(QPalette::ButtonText, QColor(224, 224, 224));
    palette.setColor(QPalette::BrightText, Qt::white);
    palette.setColor(QPalette::Link, QColor(86, 156, 214));
    palette.setColor(QPalette::Highlight, QColor(38, 79, 120));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(112, 112, 112));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(112, 112, 112));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(112, 112, 112));
    palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(56, 56, 56));
    qApp->setPalette(palette);

    qApp->setStyleSheet(QStringLiteral(R"(
        QMainWindow, QDialog { background: #202020; color: #e0e0e0; }
        QMenuBar { background: #202020; color: #e0e0e0; border-bottom: 1px solid #3a3a3a; }
        QMenuBar::item { padding: 4px 7px; background: transparent; }
        QMenuBar::item:selected, QMenuBar::item:pressed { background: #3b3b3b; }
        QMenu { background: #252525; color: #e0e0e0; border: 1px solid #4a4a4a; }
        QMenu::item { padding: 4px 28px 4px 24px; }
        QMenu::item:selected { background: #264f78; color: white; }
        QMenu::item:disabled { color: #707070; }
        QMenu::separator { height: 1px; background: #454545; margin: 3px 6px; }
        QToolBar { background: #252525; border: 0; border-bottom: 1px solid #3a3a3a; spacing: 1px; padding: 2px; }
        QToolBar::separator { width: 1px; background: #4a4a4a; margin: 3px 4px; }
        QToolButton { background: transparent; border: 1px solid transparent; padding: 2px; }
        QToolButton:hover { background: #3a3a3a; border-color: #555555; }
        QToolButton:pressed, QToolButton:checked { background: #264f78; border-color: #3f78a8; }
        QStatusBar { background: #252525; color: #d8d8d8; border-top: 1px solid #3a3a3a; }
        QStatusBar::item { border: 0; }
        QDockWidget { color: #e0e0e0; }
        QDockWidget::title { background: #303030; border-bottom: 1px solid #484848; padding: 5px 6px; text-align: left; }
        QSplitter::handle { background: #3a3a3a; }
        QTabWidget::pane { border: 1px solid #454545; background: #202020; }
        QTabBar::tab { background: #303030; color: #cfcfcf; border: 1px solid #454545; border-bottom: 0; padding: 5px 9px; margin-right: 1px; }
        QTabBar::tab:selected { background: #202020; color: white; }
        QTabBar::tab:hover:!selected { background: #3a3a3a; }
        QListWidget, QTreeView, QTableWidget { background: #181818; color: #e0e0e0; alternate-background-color: #222222; border: 1px solid #484848; selection-background-color: #264f78; selection-color: white; }
        QListWidget::item { color: #e0e0e0; }
        QListWidget::item:selected { color: white; background: #264f78; }
        QHeaderView::section { background: #303030; color: #e0e0e0; border: 0; border-right: 1px solid #484848; border-bottom: 1px solid #484848; padding: 4px; }
        QLineEdit, QComboBox, QSpinBox, QKeySequenceEdit { background: #181818; color: #e0e0e0; border: 1px solid #555555; padding: 3px; selection-background-color: #264f78; }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QKeySequenceEdit:focus { border-color: #5b9bd5; }
        QComboBox QAbstractItemView { background: #252525; color: #e0e0e0; selection-background-color: #264f78; }
        QPushButton { background: #333333; color: #e0e0e0; border: 1px solid #5a5a5a; padding: 4px 12px; }
        QPushButton:hover { background: #404040; border-color: #707070; }
        QPushButton:pressed { background: #252525; }
        QPushButton:default { border-color: #5b9bd5; }
        QPushButton:disabled { color: #707070; background: #2a2a2a; }
        QGroupBox { border: 1px solid #484848; margin-top: 8px; padding-top: 6px; }
        QGroupBox::title { subcontrol-origin: margin; left: 7px; padding: 0 4px; color: #d8d8d8; }
        QAbstractScrollArea::corner { background: #202020; }
        QScrollBar:vertical, QScrollBar:horizontal { background: #202020; border: 0; }
        QScrollBar::handle { background: #555555; min-width: 18px; min-height: 18px; }
        QScrollBar::handle:hover { background: #686868; }
        QScrollBar::add-page, QScrollBar::sub-page { background: #202020; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { background: #202020; height: 0; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { background: #202020; width: 0; }
        QScrollBar::up-arrow, QScrollBar::down-arrow, QScrollBar::left-arrow, QScrollBar::right-arrow { image: none; width: 0; height: 0; }
        QToolTip { background: #303030; color: white; border: 1px solid #666666; }
    )"));
}

void MainWindow::showPreferences()
{
    // 如果对话框被语言切换销毁了，重新创建（使用 tr() 内建文本）
    if (!_preferenceDlg) {
        _preferenceDlg = new PreferenceDlg(this);
        connect(_preferenceDlg, &PreferenceDlg::settingsChanged,
                this, &MainWindow::applyPreferencesToAllViews);
    }
    // 对应原版 changeDlgLang()：用 NativeLangSpeaker 翻译对话框控件
    NppParameters& params = NppParameters::getInstance();
    NativeLangSpeaker& speaker = params.getNativeLangSpeaker();
    if (speaker.isLoaded())
        speaker.changeDlgLang(_preferenceDlg, "Preferences");

    _preferenceDlg->loadSettings();
    _preferenceDlg->exec();
    delete _preferenceDlg;
    _preferenceDlg = nullptr;
}

// ─── 界面创建 ────────────────────────────────────────────────────────────────

// 加载 BMP 图标（以左上角像素色为透明色）
static QIcon loadBmpIcon(const QString& path)
{
    QPixmap pm(path);
    if (pm.isNull()) return QIcon();
    QImage img = pm.toImage().convertToFormat(QImage::Format_ARGB32);
    QColor bg(img.pixel(0, 0));
    // 将背景色替换为透明
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            QColor c(img.pixel(x, y));
            if (c.red()   == bg.red()   &&
                c.green() == bg.green() &&
                c.blue()  == bg.blue())
                img.setPixel(x, y, qRgba(0, 0, 0, 0));
        }
    }
    return QIcon(QPixmap::fromImage(img));
}

void MainWindow::applyToolbarIcons()
{
    const ToolbarIconTheme theme = ToolbarIconTheme::fromUserDirectory(
        NppParameters::getInstance().getUserPath());
    const struct {
        const char* actionName;
        const char* iconId;
        const char* fallback;
    } icons[] = {
        {"newAction", "new", ":/icons/newFile.bmp"},
        {"openAction", "open", ":/icons/openFile.bmp"},
        {"saveAction", "save", ":/icons/saveFile.bmp"},
        {"saveAllAction", "save-all", ":/icons/saveAll.bmp"},
        {"closeAction", "close", ":/icons/closeFile.bmp"},
        {"closeAllAction", "close-all", ":/icons/closeAll.bmp"},
        {"printAction", "print", ":/icons/print.bmp"},
        {"cutAction", "cut", ":/icons/cut.bmp"},
        {"copyAction", "copy", ":/icons/copy.bmp"},
        {"pasteAction", "paste", ":/icons/paste.bmp"},
        {"undoAction", "undo", ":/icons/undo.bmp"},
        {"redoAction", "redo", ":/icons/redo.bmp"},
        {"findAction", "find", ":/icons/find.bmp"},
        {"replaceAction", "replace", ":/icons/findReplace.bmp"},
        {"zoomInAction", "zoom-in", ":/icons/zoomIn.bmp"},
        {"zoomOutAction", "zoom-out", ":/icons/zoomOut.bmp"},
        {"wordWrapAction", "word-wrap", ":/icons/wrap.bmp"},
        {"showAllCharactersAction", "all-chars", ":/icons/invisibleChar.bmp"},
        {"showIndentAction", "indent-guide", ":/icons/indentGuide.bmp"},
        {"userDefinedLanguageDialogAction", "udl-dlg", ":/icons/userDefineDlg_off.ico"},
        {"docMapAction", "doc-map", ":/icons/docMap.bmp"},
        {"documentListAction", "doc-list", ":/icons/docList.bmp"},
        {"funcListAction", "function-list", ":/icons/functionList.bmp"},
        {"fileBrowserAction", "folder-as-workspace", ":/icons/fileBrowser.bmp"},
        {"startRecordAction", "record", ":/icons/startRecord.bmp"},
        {"stopRecordAction", "stop-record", ":/icons/stopRecord.bmp"},
        {"playMacroAction", "playback", ":/icons/playRecord.bmp"},
        {"saveMacroAction", "save-macro", ":/icons/saveRecord.bmp"}
    };
    for (const auto& entry : icons) {
        QAction* action = findChild<QAction*>(
            QString::fromLatin1(entry.actionName));
        if (!action)
            continue;
        const QIcon fallback = *entry.fallback
            ? loadBmpIcon(QString::fromLatin1(entry.fallback)) : QIcon();
        action->setIcon(theme.icon(QString::fromLatin1(entry.iconId), fallback));
    }
}

void MainWindow::createActions()
{
    _newAction = new QAction(loadBmpIcon(":/icons/newFile.bmp"), tr("&New"), this);
    _newAction->setObjectName("newAction");
    _newAction->setShortcut(QKeySequence::New);
    connect(_newAction, &QAction::triggered, this, &MainWindow::newFile);

    _openAction = new QAction(loadBmpIcon(":/icons/openFile.bmp"), tr("&Open..."), this);
    _openAction->setObjectName("openAction");
    _openAction->setShortcut(QKeySequence::Open);
    connect(_openAction, &QAction::triggered, this,
            static_cast<void (MainWindow::*)()>(&MainWindow::openFile));

    _saveAction = new QAction(loadBmpIcon(":/icons/saveFile.bmp"), tr("&Save"), this);
    _saveAction->setObjectName("saveAction");
    _saveAction->setShortcut(QKeySequence::Save);
    connect(_saveAction, &QAction::triggered, this, &MainWindow::saveFile);

    _saveAsAction = new QAction(tr("Save &As..."), this);
    _saveAsAction->setObjectName("saveAsAction");
    _saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(_saveAsAction, &QAction::triggered, this, &MainWindow::saveFileAs);

    _saveAllAction = new QAction(loadBmpIcon(":/icons/saveAll.bmp"), tr("Sa&ve All"), this);
    _saveAllAction->setObjectName("saveAllAction");
    _saveAllAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_S);
    connect(_saveAllAction, &QAction::triggered, this, &MainWindow::saveAllFiles);

    _closeAction = new QAction(loadBmpIcon(":/icons/closeFile.bmp"), tr("&Close"), this);
    _closeAction->setObjectName("closeAction");
    _closeAction->setShortcut(Qt::CTRL | Qt::Key_W);
    connect(_closeAction, &QAction::triggered, this, &MainWindow::closeFile);

    _closeAllAction = new QAction(loadBmpIcon(":/icons/closeAll.bmp"), tr("Clos&e All"), this);
    _closeAllAction->setObjectName("closeAllAction");
    connect(_closeAllAction, &QAction::triggered, this, &MainWindow::closeAllFiles);

    _reloadAction = new QAction(tr("Re&load from Disk"), this);
    _reloadAction->setObjectName("reloadAction");
    connect(_reloadAction, &QAction::triggered, this, &MainWindow::reloadFromDisk);

    _exitAction = new QAction(tr("E&xit"), this);
    _exitAction->setObjectName("exitAction");
    _exitAction->setShortcut(QKeySequence::Quit);
    connect(_exitAction, &QAction::triggered, this, &QWidget::close);

    _undoAction = new QAction(loadBmpIcon(":/icons/undo.bmp"), tr("&Undo"), this);
    _undoAction->setObjectName("undoAction");
    _undoAction->setShortcut(QKeySequence::Undo);
    connect(_undoAction, &QAction::triggered, this, [this]() {
        if (auto* v = currentActiveView()) v->undo();
    });

    _redoAction = new QAction(loadBmpIcon(":/icons/redo.bmp"), tr("&Redo"), this);
    _redoAction->setObjectName("redoAction");
    _redoAction->setShortcut(QKeySequence::Redo);
    connect(_redoAction, &QAction::triggered, this, [this]() {
        if (auto* v = currentActiveView()) v->redo();
    });

    _cutAction = new QAction(loadBmpIcon(":/icons/cut.bmp"), tr("Cu&t"), this);
    _cutAction->setObjectName("cutAction");
    _cutAction->setShortcut(QKeySequence::Cut);
    connect(_cutAction, &QAction::triggered, this, [this]() {
        if (auto* v = currentActiveView()) v->cut();
    });

    _copyAction = new QAction(loadBmpIcon(":/icons/copy.bmp"), tr("&Copy"), this);
    _copyAction->setObjectName("copyAction");
    _copyAction->setShortcut(QKeySequence::Copy);
    connect(_copyAction, &QAction::triggered, this, [this]() {
        if (auto* v = currentActiveView()) v->copy();
    });

    _pasteAction = new QAction(loadBmpIcon(":/icons/paste.bmp"), tr("&Paste"), this);
    _pasteAction->setObjectName("pasteAction");
    _pasteAction->setShortcut(QKeySequence::Paste);
    connect(_pasteAction, &QAction::triggered, this, [this]() {
        if (auto* v = currentActiveView()) v->paste();
    });

    _selectAllAction = new QAction(tr("Select &All"), this);
    _selectAllAction->setObjectName("selectAllAction");
    _selectAllAction->setShortcut(QKeySequence::SelectAll);
    connect(_selectAllAction, &QAction::triggered, this, [this]() {
        if (auto* v = currentActiveView()) v->selectAll();
    });

    _deleteLineAction = new QAction(tr("&Delete Line"), this);
    _deleteLineAction->setObjectName("deleteLineAction");
    _deleteLineAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_K);
    connect(_deleteLineAction, &QAction::triggered, this, [this]() {
        if (auto* v = currentActiveView())
            v->SendScintilla(SCI_LINEDELETE);
    });

    _duplicateLineAction = new QAction(tr("D&uplicate Line"), this);
    _duplicateLineAction->setObjectName("duplicateLineAction");
    _duplicateLineAction->setShortcut(Qt::CTRL | Qt::Key_D);
    connect(_duplicateLineAction, &QAction::triggered, this, [this]() {
        if (auto* v = currentActiveView())
            v->SendScintilla(SCI_LINEDUPLICATE);
    });

    _moveLineUpAction = new QAction(tr("Move Line &Up"), this);
    _moveLineUpAction->setObjectName("moveLineUpAction");
    _moveLineUpAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_Up);
    connect(_moveLineUpAction, &QAction::triggered, this, [this]() {
        if (auto* v = currentActiveView())
            v->SendScintilla(SCI_MOVESELECTEDLINESUP);
    });

    _moveLineDownAction = new QAction(tr("Move Line Do&wn"), this);
    _moveLineDownAction->setObjectName("moveLineDownAction");
    _moveLineDownAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_Down);
    connect(_moveLineDownAction, &QAction::triggered, this, [this]() {
        if (auto* v = currentActiveView())
            v->SendScintilla(SCI_MOVESELECTEDLINESDOWN);
    });

    _toggleCommentAction = new QAction(tr("Toggle &Line Comment"), this);
    _toggleCommentAction->setObjectName("toggleCommentAction");
    _toggleCommentAction->setShortcut(Qt::CTRL | Qt::Key_Q);
    connect(_toggleCommentAction, &QAction::triggered, this, [this]() {
        if (auto* v = currentActiveView()) v->toggleLineComment();
    });

    _toUpperCaseAction = new QAction(tr("&UPPERCASE"), this);
    _toUpperCaseAction->setObjectName("toUpperCaseAction");
    _toUpperCaseAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_U);
    connect(_toUpperCaseAction, &QAction::triggered, this, [this]() {
        if (auto* v = currentActiveView())
            v->SendScintilla(SCI_UPPERCASE);
    });

    _toLowerCaseAction = new QAction(tr("&lowercase"), this);
    _toLowerCaseAction->setObjectName("toLowerCaseAction");
    _toLowerCaseAction->setShortcut(Qt::CTRL | Qt::Key_U);
    connect(_toLowerCaseAction, &QAction::triggered, this, [this]() {
        if (auto* v = currentActiveView())
            v->SendScintilla(SCI_LOWERCASE);
    });

    _findAction = new QAction(loadBmpIcon(":/icons/find.bmp"), tr("&Find..."), this);
    _findAction->setObjectName("findAction");
    _findAction->setShortcut(QKeySequence::Find);
    connect(_findAction, &QAction::triggered, this, &MainWindow::find);

    _replaceAction = new QAction(loadBmpIcon(":/icons/findReplace.bmp"), tr("&Replace..."), this);
    _replaceAction->setObjectName("replaceAction");
    _replaceAction->setShortcut(QKeySequence::Replace);
    connect(_replaceAction, &QAction::triggered, this, &MainWindow::replace);

    _goToLineAction = new QAction(tr("&Go to..."), this);
    _goToLineAction->setObjectName("goToLineAction");
    _goToLineAction->setShortcut(Qt::CTRL | Qt::Key_G);
    connect(_goToLineAction, &QAction::triggered, this, &MainWindow::goToLine);

    _toggleBookmarkAction = new QAction(tr("Toggle &Bookmark"), this);
    _toggleBookmarkAction->setObjectName("toggleBookmarkAction");
    _toggleBookmarkAction->setShortcut(Qt::CTRL | Qt::Key_F2);
    connect(_toggleBookmarkAction, &QAction::triggered, this, &MainWindow::toggleBookmark);

    _nextBookmarkAction = new QAction(tr("&Next Bookmark"), this);
    _nextBookmarkAction->setObjectName("nextBookmarkAction");
    _nextBookmarkAction->setShortcut(Qt::Key_F2);
    connect(_nextBookmarkAction, &QAction::triggered, this, &MainWindow::nextBookmark);

    _prevBookmarkAction = new QAction(tr("&Previous Bookmark"), this);
    _prevBookmarkAction->setObjectName("prevBookmarkAction");
    _prevBookmarkAction->setShortcut(Qt::SHIFT | Qt::Key_F2);
    connect(_prevBookmarkAction, &QAction::triggered, this, &MainWindow::prevBookmark);

    _clearBookmarksAction = new QAction(tr("Clear &All Bookmarks"), this);
    _clearBookmarksAction->setObjectName("clearBookmarksAction");
    connect(_clearBookmarksAction, &QAction::triggered, this, &MainWindow::clearAllBookmarks);

    // EOL 转换
    _eolWindowsAction = new QAction(tr("&Windows (CRLF)"), this);
    _eolWindowsAction->setObjectName("eolWindowsAction");
    _eolWindowsAction->setCheckable(true);
    connect(_eolWindowsAction, &QAction::triggered, this, [this]() {
        setBufferEolMode(_activeDocTab->currentBuffer(),
                         EolWindows, true);
    });

    _eolUnixAction = new QAction(tr("&Unix (LF)"), this);
    _eolUnixAction->setObjectName("eolUnixAction");
    _eolUnixAction->setCheckable(true);
    connect(_eolUnixAction, &QAction::triggered, this, [this]() {
        setBufferEolMode(_activeDocTab->currentBuffer(),
                         EolUnix, true);
    });

    _eolMacAction = new QAction(tr("&Mac (CR)"), this);
    _eolMacAction->setObjectName("eolMacAction");
    _eolMacAction->setCheckable(true);
    connect(_eolMacAction, &QAction::triggered, this, [this]() {
        setBufferEolMode(_activeDocTab->currentBuffer(),
                         EolMac, true);
    });

    // 设置
    _preferencesAction = new QAction(tr("&Preferences..."), this);
    _preferencesAction->setObjectName("preferencesAction");
    _preferencesAction->setShortcut(Qt::CTRL | Qt::Key_Comma);
    connect(_preferencesAction, &QAction::triggered, this, &MainWindow::showPreferences);

    // 面板
    _docMapAction = new QAction(loadBmpIcon(":/icons/docMap.bmp"), tr("&Document Map"), this);
    _docMapAction->setObjectName("docMapAction");
    _docMapAction->setCheckable(true);
    connect(_docMapAction, &QAction::triggered, this, [this]() {
        _docMapDock->setVisible(!_docMapDock->isVisible());
    });

    _funcListAction = new QAction(loadBmpIcon(":/icons/functionList.bmp"), tr("&Function List"), this);
    _funcListAction->setObjectName("funcListAction");
    _funcListAction->setCheckable(true);
    connect(_funcListAction, &QAction::triggered, this, [this]() {
        _funcListDock->setVisible(!_funcListDock->isVisible());
    });

    // 宏
    _startRecordAction = new QAction(loadBmpIcon(":/icons/startRecord.bmp"),
                                     tr("&Start Recording"), this);
    _startRecordAction->setObjectName("startRecordAction");
    _startRecordAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_R);
    connect(_startRecordAction, &QAction::triggered, this, &MainWindow::startMacroRecording);

    _stopRecordAction = new QAction(loadBmpIcon(":/icons/stopRecord.bmp"),
                                    tr("S&top Recording"), this);
    _stopRecordAction->setObjectName("stopRecordAction");
    _stopRecordAction->setEnabled(false);
    connect(_stopRecordAction, &QAction::triggered, this, &MainWindow::stopMacroRecording);

    _playMacroAction = new QAction(loadBmpIcon(":/icons/playRecord.bmp"),
                                   tr("&Playback"), this);
    _playMacroAction->setObjectName("playMacroAction");
    _playMacroAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_P);
    _playMacroAction->setEnabled(false);
    connect(_playMacroAction, &QAction::triggered, this, &MainWindow::playMacro);

    _saveMacroAction = new QAction(loadBmpIcon(":/icons/saveRecord.bmp"),
                                   tr("Sa&ve Macro..."), this);
    _saveMacroAction->setObjectName("saveMacroAction");
    connect(_saveMacroAction, &QAction::triggered, this, &MainWindow::saveMacro);

    _loadMacroAction = new QAction(tr("&Load Macro..."), this);
    _loadMacroAction->setObjectName("loadMacroAction");
    connect(_loadMacroAction, &QAction::triggered, this, &MainWindow::loadMacro);

    _splitViewAction = new QAction(tr("&Split View"), this);
    _splitViewAction->setObjectName("splitViewAction");
    _splitViewAction->setShortcut(Qt::Key_F8);
    _splitViewAction->setCheckable(true);
    connect(_splitViewAction, &QAction::triggered, this, &MainWindow::toggleSplitView);

    _rotateSplitAction = new QAction(tr("&Rotate Split View"), this);
    _rotateSplitAction->setObjectName("rotateSplitAction");
    _rotateSplitAction->setShortcut(Qt::SHIFT | Qt::Key_F8);
    connect(_rotateSplitAction, &QAction::triggered, this, &MainWindow::rotateSplitView);

    _moveToOtherAction = new QAction(tr("&Move to Other View"), this);
    _moveToOtherAction->setObjectName("moveToOtherAction");
    connect(_moveToOtherAction, &QAction::triggered, this, &MainWindow::moveToOtherView);

    _cloneToOtherAction = new QAction(tr("&Clone to Other View"), this);
    _cloneToOtherAction->setObjectName("cloneToOtherAction");
    connect(_cloneToOtherAction, &QAction::triggered, this, &MainWindow::cloneToOtherView);

    _fileBrowserAction = new QAction(loadBmpIcon(":/icons/fileBrowser.bmp"),
                                     tr("Folder as &Workspace"), this);
    _fileBrowserAction->setObjectName("fileBrowserAction");
    _fileBrowserAction->setShortcut(Qt::CTRL | Qt::ALT | Qt::Key_W);
    _fileBrowserAction->setCheckable(true);
    connect(_fileBrowserAction, &QAction::triggered, this, &MainWindow::toggleFileBrowser);

    _zoomInAction = new QAction(loadBmpIcon(":/icons/zoomIn.bmp"), tr("Zoom &In"), this);
    _zoomInAction->setObjectName("zoomInAction");
    _zoomInAction->setShortcut(Qt::CTRL | Qt::Key_Plus);
    connect(_zoomInAction, &QAction::triggered, this, &MainWindow::zoomIn);

    _zoomOutAction = new QAction(loadBmpIcon(":/icons/zoomOut.bmp"), tr("Zoom &Out"), this);
    _zoomOutAction->setObjectName("zoomOutAction");
    _zoomOutAction->setShortcut(Qt::CTRL | Qt::Key_Minus);
    connect(_zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);

    _zoomRestoreAction = new QAction(tr("Restore Default &Zoom"), this);
    _zoomRestoreAction->setObjectName("zoomRestoreAction");
    _zoomRestoreAction->setShortcut(Qt::CTRL | Qt::Key_0);
    connect(_zoomRestoreAction, &QAction::triggered, this, &MainWindow::zoomRestore);

    _wordWrapAction = new QAction(loadBmpIcon(":/icons/wrap.bmp"), tr("&Word Wrap"), this);
    _wordWrapAction->setObjectName("wordWrapAction");
    _wordWrapAction->setShortcut(Qt::Key_F5);
    _wordWrapAction->setCheckable(true);
    connect(_wordWrapAction, &QAction::triggered, this, &MainWindow::toggleWordWrap);

    _showWhitespaceAction = new QAction(loadBmpIcon(":/icons/invisibleChar.bmp"),
                                        tr("Show White Space and &TAB"), this);
    _showWhitespaceAction->setObjectName("showWhitespaceAction");
    _showWhitespaceAction->setCheckable(true);
    connect(_showWhitespaceAction, &QAction::triggered, this, &MainWindow::toggleWhitespace);

    _showIndentAction = new QAction(loadBmpIcon(":/icons/indentGuide.bmp"),
                                    tr("Show &Indent Guide"), this);
    _showIndentAction->setObjectName("showIndentAction");
    _showIndentAction->setCheckable(true);
    connect(_showIndentAction, &QAction::triggered, this, &MainWindow::toggleIndentGuide);

    _aboutAction = new QAction(tr("&About"), this);
    _aboutAction->setObjectName("aboutAction");
    connect(_aboutAction, &QAction::triggered, this, &MainWindow::about);
}

void MainWindow::registerNppCommandIds()
{
    for (const NppCommandMapping& mapping : nppCommandMappings()) {
        QAction* action = findChild<QAction*>(mapping.objectName);
        if (action)
            action->setProperty("nppCommandId", mapping.commandId);
    }
}

void MainWindow::applyConfiguredShortcuts()
{
    const auto& shortcuts =
        NppParameters::getInstance().getInternalCommandShortcuts();
    for (const InternalCommandShortcut& configured : shortcuts) {
        for (QAction* action : findChildren<QAction*>()) {
            if (action->property("nppCommandId").toInt() == configured.id) {
                action->setShortcut(configured.shortcut.toKeySequence());
                break;
            }
        }
    }
}

void MainWindow::applyScintillaShortcuts(ScintillaEditView* view)
{
    if (!view)
        return;
    for (const ScintillaKeyDef& configured :
         NppParameters::getInstance().getScintillaKeys()) {
        for (const ShortcutKey& shortcut : configured.shortcuts) {
            const int combined = shortcut.toKeySequence()[0];
            const int key = combined & ~Qt::KeyboardModifierMask;
            int modifiers = 0;
            if (combined & Qt::ShiftModifier) modifiers |= SCMOD_SHIFT;
            if (combined & Qt::ControlModifier) modifiers |= SCMOD_CTRL;
            if (combined & Qt::AltModifier) modifiers |= SCMOD_ALT;
            view->SendScintilla(SCI_ASSIGNCMDKEY,
                key | (modifiers << 16), configured.scintillaId);
        }
    }
}

bool MainWindow::executeNppCommand(int commandId)
{
    for (QAction* action : findChildren<QAction*>()) {
        if (action->property("nppCommandId").toInt() == commandId &&
            action->isEnabled()) {
            action->trigger();
            return true;
        }
    }
    return false;
}

void MainWindow::playConfiguredMacro(const MacroDef& macro)
{
    ScintillaEditView* view = currentActiveView();
    if (!view)
        return;
    for (const MacroAction& action : macro.actions) {
        if (action.type == 2) {
            executeNppCommand(action.wParam);
        } else if (action.type == 1) {
            const QByteArray parameter = action.sParam.toUtf8();
            view->SendScintillaNpp(
                action.message, static_cast<quintptr>(action.wParam),
                reinterpret_cast<qintptr>(parameter.constData()));
        } else if (action.type == 0) {
            view->SendScintillaNpp(
                action.message, static_cast<quintptr>(action.wParam),
                static_cast<qintptr>(action.lParam));
        } else if (action.type == 3 && _findReplaceDlg) {
            _findReplaceDlg->executeSavedMacroAction(
                action.message, action.lParam, action.sParam);
        }
    }
}

void MainWindow::rebuildConfiguredMacroMenu()
{
    if (!_macroMenu)
        return;
    for (QAction* action : _macroMenu->actions()) {
        if (action->property("configuredMacro").toBool()) {
            _macroMenu->removeAction(action);
            action->deleteLater();
        }
    }
    const QVector<MacroDef>& macros = NppParameters::getInstance().getMacros();
    if (macros.isEmpty())
        return;
    QAction* separator = _macroMenu->addSeparator();
    separator->setProperty("configuredMacro", true);
    for (const MacroDef& macro : macros) {
        QAction* action = _macroMenu->addAction(macro.name);
        action->setProperty("configuredMacro", true);
        ShortcutKey key;
        key.ctrl = macro.ctrl;
        key.alt = macro.alt;
        key.shift = macro.shift;
        key.key = macro.key;
        action->setShortcut(key.toKeySequence());
        connect(action, &QAction::triggered, this,
                [this, macro]() { playConfiguredMacro(macro); });
    }
}

void MainWindow::createMenus()
{
    auto addPlaceholder = [](QMenu* menu, const QString& text, const char* objectName = nullptr) {
        QAction* action = menu->addAction(text);
        action->setEnabled(false);
        if (objectName)
            action->setObjectName(objectName);
        return action;
    };
    auto addCommand = [this](QMenu* menu, const QString& text, const char* objectName,
                             const std::function<void()>& command) {
        QAction* action = menu->addAction(text);
        action->setObjectName(objectName);
        connect(action, &QAction::triggered, this, command);
        return action;
    };

    // ── File ─────────────────────────────────────────────────
    _fileMenu = menuBar()->addMenu(tr("&File"));
    _fileMenu->setObjectName("fileMenu");
    _fileMenu->addAction(_newAction);
    _fileMenu->addAction(_openAction);

    QMenu* openContainingMenu = _fileMenu->addMenu(tr("Open Containing &Folder"));
    openContainingMenu->setObjectName("openContainingFolderMenu");
    addCommand(openContainingMenu, tr("Explorer"), "openContainingExplorerAction",
               [this]() { openContainingFolder(); });
    addCommand(openContainingMenu, tr("cmd"), "openContainingCmdAction",
               [this]() { openContainingTerminal(); });
    openContainingMenu->addSeparator();
    addCommand(openContainingMenu, tr("Folder as Workspace"), "containingFolderAsWorkspaceAction",
               [this]() {
        Buffer* buffer = _activeDocTab->currentBuffer();
        if (!buffer || buffer->isUntitled()) return;
        _fileBrowserPanel->setRootPath(QFileInfo(buffer->getFullPath()).absolutePath());
        _fileBrowserDock->show();
    });

    addCommand(_fileMenu, tr("Open in &Default Viewer"), "openDefaultViewerAction",
               [this]() { openDefaultViewer(); });
    addCommand(_fileMenu, tr("Open Folder as &Workspace..."), "openFolderAsWorkspaceAction",
               [this]() { openFolderAsWorkspace(); });

    _recentFilesMenu = _fileMenu->addMenu(tr("Open &Recent"));
    _recentFilesMenu->setObjectName("recentFilesMenu");
    _recentFilesMenu->setEnabled(false);
    _restoreLastClosedAction =
        _fileMenu->addAction(tr("Restore Recent Closed File"));
    _restoreLastClosedAction->setObjectName("restoreLastClosedFileAction");
    _restoreLastClosedAction->setShortcut(
        QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T));
    _restoreLastClosedAction->setEnabled(false);
    connect(_restoreLastClosedAction, &QAction::triggered,
            this, &MainWindow::restoreLastClosedFile);
    _fileMenu->addAction(_reloadAction);
    _fileMenu->addSeparator();
    _fileMenu->addAction(_saveAction);
    _fileMenu->addAction(_saveAsAction);
    addCommand(_fileMenu, tr("Save a Cop&y As..."), "saveCopyAsAction",
               [this]() { saveCopyAs(); });
    _fileMenu->addAction(_saveAllAction);
    addCommand(_fileMenu, tr("&Rename..."), "renameFileAction",
               [this]() { renameCurrentFile(); });
    _fileMenu->addSeparator();
    _fileMenu->addAction(_closeAction);
    _fileMenu->addAction(_closeAllAction);

    QMenu* closeMultipleMenu = _fileMenu->addMenu(tr("Close &Multiple Documents"));
    closeMultipleMenu->setObjectName("closeMultipleMenu");
    addCommand(closeMultipleMenu, tr("Close All but Active Document"), "closeAllButCurrentAction",
               [this]() { closeAllButCurrent(); });
    addCommand(closeMultipleMenu, tr("Close All to the Left"), "closeAllToLeftAction",
               [this]() { closeAllToLeft(); });
    addCommand(closeMultipleMenu, tr("Close All to the Right"), "closeAllToRightAction",
               [this]() { closeAllToRight(); });
    addCommand(closeMultipleMenu, tr("Close All Unchanged"), "closeAllUnchangedAction",
               [this]() { closeAllUnchanged(); });

    addCommand(_fileMenu, tr("Move to Recycle &Bin"), "moveToRecycleBinAction",
               [this]() { moveCurrentFileToTrash(); });
    _fileMenu->addSeparator();
    addCommand(_fileMenu, tr("Load Sess&ion..."), "loadSessionAction",
               [this]() { loadSessionFile(); });
    addCommand(_fileMenu, tr("Save Sess&ion..."), "saveSessionAction",
               [this]() { saveSessionFile(); });
    _fileMenu->addSeparator();
    _printAction = _fileMenu->addAction(tr("&Print..."));
    _printAction->setObjectName("printAction");
    _printAction->setShortcut(QKeySequence::Print);
    connect(_printAction, &QAction::triggered, this, &MainWindow::printDocument);
    _printNowAction = _fileMenu->addAction(tr("Print No&w"));
    _printNowAction->setObjectName("printNowAction");
    connect(_printNowAction, &QAction::triggered, this, &MainWindow::printDocumentNow);
    _fileMenu->addSeparator();
    _fileMenu->addAction(_exitAction);

    // ── Edit ─────────────────────────────────────────────────
    _editMenu = menuBar()->addMenu(tr("&Edit"));
    _editMenu->setObjectName("editMenu");
    _editMenu->addAction(_undoAction);
    _editMenu->addAction(_redoAction);
    _editMenu->addSeparator();
    _editMenu->addAction(_cutAction);
    _editMenu->addAction(_copyAction);
    _editMenu->addAction(_pasteAction);
    QAction* deleteSelectionAction = addCommand(_editMenu, tr("&Delete"),
        "deleteSelectionAction", [this]() {
            if (auto* view = currentActiveView()) view->removeSelectedText();
        });
    deleteSelectionAction->setShortcut(QKeySequence::Delete);
    _editMenu->addAction(_selectAllAction);
    QAction* beginEndSelectAction = _editMenu->addAction(tr("Begin/End &Select"));
    beginEndSelectAction->setObjectName("beginEndSelectAction");
    beginEndSelectAction->setCheckable(true);
    connect(beginEndSelectAction, &QAction::toggled, this,
            [this, beginEndSelectAction](bool started) {
        ScintillaEditView* view = currentActiveView();
        if (!view) {
            beginEndSelectAction->setChecked(false);
            return;
        }
        if (started) {
            beginEndSelectAction->setProperty("selectionStart", view->getCurrentPos());
            statusBar()->showMessage(tr("Selection start set"));
        } else {
            const int start = beginEndSelectAction->property("selectionStart").toInt();
            view->SendScintilla(SCI_SETSEL, start, view->getCurrentPos());
            statusBar()->clearMessage();
        }
    });
    _editMenu->addSeparator();

    QMenu* insertMenu = _editMenu->addMenu(tr("&Insert"));
    insertMenu->setObjectName("insertMenu");
    addCommand(insertMenu, tr("Date Time (short)"), "insertDateTimeShortAction",
               [this]() { insertDateTime(0); });
    addCommand(insertMenu, tr("Date Time (long)"), "insertDateTimeLongAction",
               [this]() { insertDateTime(1); });
    addCommand(insertMenu, tr("Date Time (customized)"), "insertDateTimeCustomAction",
               [this]() { insertDateTime(2); });

    // Line Operations submenu
    QMenu* lineOpsMenu = _editMenu->addMenu(tr("&Line Operations"));
    lineOpsMenu->setObjectName("lineOpsMenu");
    lineOpsMenu->addAction(_duplicateLineAction);
    lineOpsMenu->addAction(_deleteLineAction);
    lineOpsMenu->addSeparator();
    lineOpsMenu->addAction(_moveLineUpAction);
    lineOpsMenu->addAction(_moveLineDownAction);
    lineOpsMenu->addSeparator();
    addCommand(lineOpsMenu, tr("Duplicate Current Line Selection"),
               "duplicateSelectionAction", [this]() {
        if (auto* view = currentActiveView()) {
            if (view->hasSelectedText()) {
                const QString selected = view->selectedText();
                const qintptr end = view->SendScintillaNpp(
                    SCI_GETSELECTIONEND);
                view->SendScintillaNpp(
                    SCI_INSERTTEXT, end,
                    reinterpret_cast<qintptr>(selected.toUtf8().constData()));
            } else {
                view->SendScintillaNpp(SCI_LINEDUPLICATE);
            }
        }
    });
    addCommand(lineOpsMenu, tr("Split Lines"), "splitLinesAction",
               [this]() { if (auto* view = currentActiveView())
                    view->SendScintillaNpp(SCI_LINESSPLIT, 0); });
    addCommand(lineOpsMenu, tr("Join Lines"), "joinLinesAction",
               [this]() { if (auto* view = currentActiveView())
                    view->SendScintillaNpp(SCI_LINESJOIN); });
    addCommand(lineOpsMenu, tr("Insert Blank Line Above"),
               "insertBlankLineAboveAction", [this]() {
        if (auto* view = currentActiveView()) {
            int line = 0, index = 0;
            view->getCursorPosition(&line, &index);
            view->insertAt(QStringLiteral("\n"), line, 0);
            view->setCursorPosition(line, 0);
        }
    });
    addCommand(lineOpsMenu, tr("Insert Blank Line Below"),
               "insertBlankLineBelowAction", [this]() {
        if (auto* view = currentActiveView()) {
            int line = 0, index = 0;
            view->getCursorPosition(&line, &index);
            view->insertAt(QStringLiteral("\n"), line, view->lineLength(line));
            view->setCursorPosition(line + 1, 0);
        }
    });
    lineOpsMenu->addSeparator();
    QAction* removeEmpty = lineOpsMenu->addAction(tr("Remove Empty Lines"));
    removeEmpty->setObjectName("removeEmptyLinesAction");
    connect(removeEmpty, &QAction::triggered, this, [this]() { transformLines(0); });
    QAction* removeBlank = lineOpsMenu->addAction(tr("Remove Empty Lines (Containing Blank characters)"));
    removeBlank->setObjectName("removeBlankLinesAction");
    connect(removeBlank, &QAction::triggered, this, [this]() { transformLines(1); });

    QMenu* sortMenu = lineOpsMenu->addMenu(tr("Sort Lines"));
    sortMenu->setObjectName("sortLinesMenu");
    const struct { const char* text; const char* objectName; int mode; } sortItems[] = {
        {"Lexicographically Ascending", "sortLexicographicAscendingAction", 0},
        {"Lexicographically Descending", "sortLexicographicDescendingAction", 1},
        {"Ascending Ignoring Case", "sortAscendingIgnoreCaseAction", 2},
        {"Descending Ignoring Case", "sortDescendingIgnoreCaseAction", 3},
        {"Reverse Line Order", "reverseLineOrderAction", 4},
        {"Integer Ascending", "sortIntegerAscendingAction", 5},
        {"Integer Descending", "sortIntegerDescendingAction", 6},
        {"Decimal (Dot) Ascending", "sortDecimalAscendingAction", 7},
        {"Decimal (Dot) Descending", "sortDecimalDescendingAction", 8},
        {"Decimal (Comma) Ascending", "sortDecimalCommaAscendingAction", 10},
        {"Decimal (Comma) Descending", "sortDecimalCommaDescendingAction", 11},
        {"Randomize Line Order", "sortRandomAction", 9}
    };
    for (const auto& item : sortItems) {
        QAction* action = sortMenu->addAction(tr(item.text));
        action->setObjectName(item.objectName);
        connect(action, &QAction::triggered, this, [this, item]() { sortLines(item.mode); });
        _documentActions.append(action);
    }

    // Comment/Uncomment submenu
    QMenu* commentMenu = _editMenu->addMenu(tr("Co&mment/Uncomment"));
    commentMenu->setObjectName("commentMenu");
    commentMenu->addAction(_toggleCommentAction);
    addCommand(commentMenu, tr("Block Comment"), "blockCommentAction",
               [this]() { if (auto* view = currentActiveView()) view->blockComment(); });
    addCommand(commentMenu, tr("Stream Comment"), "streamCommentAction",
               [this]() { if (auto* view = currentActiveView()) view->blockComment(); });
    addCommand(commentMenu, tr("Block Uncomment"), "blockUncommentAction",
               [this]() { if (auto* view = currentActiveView()) view->blockUncomment(); });

    // Convert Case submenu
    QMenu* caseMenu = _editMenu->addMenu(tr("Con&vert Case to"));
    caseMenu->setObjectName("caseMenu");
    caseMenu->addAction(_toUpperCaseAction);
    caseMenu->addAction(_toLowerCaseAction);
    addCommand(caseMenu, tr("Proper Case"), "properCaseAction",
               [this]() { transformSelectionCase(0); });
    addCommand(caseMenu, tr("Sentence case"), "sentenceCaseAction",
               [this]() { transformSelectionCase(1); });
    addCommand(caseMenu, tr("iNVERT cASE"), "invertCaseAction",
               [this]() { transformSelectionCase(2); });

    QMenu* blankOpsMenu = _editMenu->addMenu(tr("Blank Operations"));
    blankOpsMenu->setObjectName("blankOperationsMenu");
    addCommand(blankOpsMenu, tr("Trim Trailing Space"), "trimTrailingSpaceAction",
               [this]() { transformLines(4); });
    addCommand(blankOpsMenu, tr("Trim Leading Space"), "trimLeadingSpaceAction",
               [this]() { transformLines(5); });
    QAction* trimBoth = blankOpsMenu->addAction(tr("Trim Leading and Trailing Space"));
    trimBoth->setObjectName("trimBothAction");
    connect(trimBoth, &QAction::triggered, this, [this]() { transformLines(2); });
    QAction* tabToSpace = blankOpsMenu->addAction(tr("TAB to Space"));
    tabToSpace->setObjectName("tabToSpaceAction");
    connect(tabToSpace, &QAction::triggered, this, [this]() { transformLines(3); });
    addCommand(blankOpsMenu, tr("Space to TAB (Leading)"), "spaceToTabLeadingAction",
               [this]() { transformLines(6); });
    addCommand(blankOpsMenu, tr("Space to TAB (All)"), "spaceToTabAllAction",
               [this]() { transformLines(7); });

    QMenu* indentationMenu = _editMenu->addMenu(tr("Indentation"));
    indentationMenu->setObjectName("indentationMenu");
    addCommand(indentationMenu, tr("Increase Line Indent"),
               "increaseIndentAction", [this]() {
        if (auto* view = currentActiveView())
            view->SendScintillaNpp(SCI_TAB);
    });
    addCommand(indentationMenu, tr("Decrease Line Indent"),
               "decreaseIndentAction", [this]() {
        if (auto* view = currentActiveView())
            view->SendScintillaNpp(SCI_BACKTAB);
    });

    QMenu* copySpecialMenu = _editMenu->addMenu(tr("Copy to Clipboard"));
    copySpecialMenu->setObjectName("copySpecialMenu");
    auto copyBufferPath = [this](int part) {
        Buffer* buffer = _activeDocTab ? _activeDocTab->currentBuffer() : nullptr;
        if (!buffer)
            return;
        const QFileInfo info(buffer->getFullPath());
        QString value;
        if (part == 0) value = info.absoluteFilePath();
        else if (part == 1) value = info.fileName();
        else value = info.absolutePath();
        QApplication::clipboard()->setText(value);
    };
    addCommand(copySpecialMenu, tr("Current Full File Path"),
               "copyCurrentPathAction", [copyBufferPath]() { copyBufferPath(0); });
    addCommand(copySpecialMenu, tr("Current File Name"),
               "copyCurrentNameAction", [copyBufferPath]() { copyBufferPath(1); });
    addCommand(copySpecialMenu, tr("Current Directory Path"),
               "copyCurrentDirectoryAction",
               [copyBufferPath]() { copyBufferPath(2); });
    addCommand(copySpecialMenu, tr("Copy Styled Text"),
               "copyStyledTextAction",
               [this]() { if (auto* view = currentActiveView()) view->copy(); });

    QMenu* pasteSpecialMenu = _editMenu->addMenu(tr("Paste Special"));
    pasteSpecialMenu->setObjectName("pasteSpecialMenu");
    addCommand(pasteSpecialMenu, tr("Paste HTML Content"),
               "pasteHtmlAction", [this]() {
        if (auto* view = currentActiveView()) {
            const QMimeData* mime = QApplication::clipboard()->mimeData();
            view->insert(mime->hasHtml() ? mime->html() : mime->text());
        }
    });
    addCommand(pasteSpecialMenu, tr("Paste RTF Content"),
               "pasteRtfAction", [this]() {
        if (auto* view = currentActiveView()) {
            const QMimeData* mime = QApplication::clipboard()->mimeData();
            const QByteArray rtf = mime->data(QStringLiteral("text/rtf"));
            view->insert(rtf.isEmpty() ? mime->text()
                                       : QString::fromLocal8Bit(rtf));
        }
    });
    QAction* readOnlyAction = addCommand(
        _editMenu, tr("Set Read-Only"), "readOnlyAction", [this]() {
        if (auto* view = currentActiveView())
            view->setReadOnly(!view->isReadOnly());
    });
    readOnlyAction->setCheckable(true);

    QAction* columnModeTip = _editMenu->addAction(tr("Column Mode..."));
    columnModeTip->setObjectName("columnModeTipAction");
    connect(columnModeTip, &QAction::triggered, this, [this]() {
        QMessageBox::information(this, tr("Column Mode"),
            tr("Use Alt+mouse selection or Alt+Shift+arrow keys for rectangular selection."));
    });
    QAction* columnEditorAction = _editMenu->addAction(tr("Colum&n Editor..."));
    columnEditorAction->setObjectName("columnEditorAction");
    columnEditorAction->setShortcut(Qt::ALT | Qt::Key_C);
    connect(columnEditorAction, &QAction::triggered, this, &MainWindow::columnEditor);
    _documentActions << removeEmpty << removeBlank << trimBoth << tabToSpace
                     << columnModeTip << columnEditorAction;

    QMenu* autoCompletionMenu = _editMenu->addMenu(tr("Auto-Completion"));
    autoCompletionMenu->setObjectName("autoCompletionMenu");
    addCommand(autoCompletionMenu, tr("Function Completion"), "functionCompletionAction",
               [this]() {
                   Buffer* buffer = _activeDocTab->currentBuffer();
                   auto* view = currentActiveView();
                   if (view && (!buffer || !buffer->isLargeFile()))
                       view->autoCompleteFromAPIs();
               });
    addCommand(autoCompletionMenu, tr("Word Completion"), "wordCompletionAction",
               [this]() {
                   Buffer* buffer = _activeDocTab->currentBuffer();
                   auto* view = currentActiveView();
                   if (view && (!buffer || !buffer->isLargeFile()))
                       view->autoCompleteFromDocument();
               });
    addCommand(autoCompletionMenu, tr("Function Parameters Hint"), "functionParametersHintAction",
               [this]() {
                   Buffer* buffer = _activeDocTab->currentBuffer();
                   auto* view = currentActiveView();
                   if (view && (!buffer || !buffer->isLargeFile()))
                       view->callTip();
               });
    addCommand(autoCompletionMenu, tr("Path Completion"),
               "pathCompletionAction", [this]() {
        ScintillaEditView* view = currentActiveView();
        if (!view)
            return;
        Buffer* buffer = _activeDocTab
            ? _activeDocTab->currentBuffer() : nullptr;
        const QString directory = buffer && !buffer->isUntitled()
            ? QFileInfo(buffer->getFullPath()).absolutePath()
            : QDir::currentPath();
        QStringList entries = QDir(directory).entryList(
            QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
            QDir::Name | QDir::IgnoreCase);
        const QByteArray list = entries.join(QStringLiteral(" ")).toUtf8();
        view->SendScintillaNpp(
            SCI_AUTOCSHOW, 0,
            reinterpret_cast<qintptr>(list.constData()));
    });
    addCommand(autoCompletionMenu, tr("Previous Function Parameter Hint"),
               "previousFunctionParameterHintAction",
               [this]() { if (auto* view = currentActiveView()) view->callTip(); });
    addCommand(autoCompletionMenu, tr("Next Function Parameter Hint"),
               "nextFunctionParameterHintAction",
               [this]() { if (auto* view = currentActiveView()) view->callTip(); });

    // EOL Conversion submenu
    QMenu* eolEditMenu = _editMenu->addMenu(tr("&EOL Conversion"));
    eolEditMenu->setObjectName("eolEditMenu");
    eolEditMenu->addAction(_eolWindowsAction);
    eolEditMenu->addAction(_eolUnixAction);
    eolEditMenu->addAction(_eolMacAction);

    // ── Search ───────────────────────────────────────────────
    _searchMenu = menuBar()->addMenu(tr("&Search"));
    _searchMenu->setObjectName("searchMenu");
    _searchMenu->addAction(_findAction);
    QAction* findNextAction = _searchMenu->addAction(tr("Find &Next"));
    findNextAction->setObjectName("findNextAction");
    findNextAction->setShortcut(Qt::Key_F3);
    connect(findNextAction, &QAction::triggered, this, [this]() {
        FindReplaceDlg* findDlg = ensureFindReplaceDialog();
        if (auto* view = currentActiveView())
            findDlg->setCurrentView(view);
        findDlg->findNext(true);
    });
    QAction* findPrevAction = _searchMenu->addAction(tr("Find &Previous"));
    findPrevAction->setObjectName("findPreviousAction");
    findPrevAction->setShortcut(Qt::SHIFT | Qt::Key_F3);
    connect(findPrevAction, &QAction::triggered, this, [this]() {
        FindReplaceDlg* findDlg = ensureFindReplaceDialog();
        if (auto* view = currentActiveView())
            findDlg->setCurrentView(view);
        findDlg->findNext(false);
    });
    _searchMenu->addAction(_replaceAction);
    QAction* findInFilesAction = _searchMenu->addAction(tr("Find in Files..."));
    findInFilesAction->setObjectName("findInFilesAction");
    connect(findInFilesAction, &QAction::triggered, this, [this]() {
        FindReplaceDlg* findDlg = ensureFindReplaceDialog();
        if (auto* view = currentActiveView()) {
            findDlg->setCurrentView(view);
            if (view->hasSelectedText())
                findDlg->setSearchText(view->selectedText());
        }
        findDlg->openFindInFilesTab();
    });
    QAction* findAllOpenedDocsAction = _searchMenu->addAction(tr("Find All in All Opened Documents"));
    findAllOpenedDocsAction->setObjectName("findAllOpenedDocsAction");
    connect(findAllOpenedDocsAction, &QAction::triggered, this, [this]() {
        FindReplaceDlg* findDlg = ensureFindReplaceDialog();
        if (auto* view = currentActiveView())
            findDlg->setCurrentView(view);
        findDlg->findAllInOpenedDocs();
    });
    addCommand(_searchMenu, tr("Search on Internet"), "searchOnInternetAction",
               [this]() {
        ScintillaEditView* view = currentActiveView();
        if (!view)
            return;
        int line = 0;
        int index = 0;
        view->getCursorPosition(&line, &index);
        QString query = view->hasSelectedText()
            ? view->selectedText()
            : view->wordAtLineIndex(line, index);
        query = query.trimmed();
        if (query.isEmpty())
            return;
        const NppGUI& gui = NppParameters::getInstance().getNppGUI();
        QString pattern;
        switch (gui._searchEngineChoice) {
            case 0: pattern = gui._searchEngineCustom; break;
            case 1: pattern = "https://duckduckgo.com/?q=%s"; break;
            case 3: pattern = "https://www.bing.com/search?q=%s"; break;
            case 4: pattern = "https://search.yahoo.com/search?p=%s"; break;
            case 5: pattern = "https://stackoverflow.com/search?q=%s"; break;
            default: pattern = "https://www.google.com/search?q=%s"; break;
        }
        const QString encoded =
            QString::fromLatin1(QUrl::toPercentEncoding(query));
        if (pattern.contains("%s"))
            pattern.replace("%s", encoded);
        else
            pattern += encoded;
        QDesktopServices::openUrl(QUrl(pattern));
    });
    _searchMenu->addSeparator();
    _searchMenu->addAction(_goToLineAction);
    addCommand(_searchMenu, tr("Go to Matching Brace"), "goToMatchingBraceAction",
               [this]() { goToMatchingBrace(); });
    addCommand(_searchMenu, tr("Select to Matching Brace"),
               "selectToMatchingBraceAction", [this]() {
        if (auto* view = currentActiveView()) {
            const qintptr current = view->getCurrentPos();
            const qintptr match = view->SendScintillaNpp(
                SCI_BRACEMATCH, current);
            if (match >= 0)
                view->SendScintillaNpp(
                    SCI_SETSEL, current, match + 1);
        }
    });
    addCommand(_searchMenu, tr("Next Search Result"),
               "nextSearchResultAction", [this]() {
        if (!_findResultView || !_findResultDock->isVisible())
            return;
        int line = 0, index = 0;
        _findResultView->getCursorPosition(&line, &index);
        for (int candidate = line + 1;
             candidate < _findResultIndexByLine.size(); ++candidate) {
            if (_findResultIndexByLine.at(candidate) >= 0) {
                _findResultView->setCursorPosition(candidate, 0);
                return;
            }
        }
    });
    addCommand(_searchMenu, tr("Previous Search Result"),
               "previousSearchResultAction", [this]() {
        if (!_findResultView || !_findResultDock->isVisible())
            return;
        int line = 0, index = 0;
        _findResultView->getCursorPosition(&line, &index);
        for (int candidate = line - 1; candidate >= 0; --candidate) {
            if (_findResultIndexByLine.at(candidate) >= 0) {
                _findResultView->setCursorPosition(candidate, 0);
                return;
            }
        }
    });
    _searchMenu->addSeparator();

    // Bookmark submenu
    QMenu* bookmarkMenu = _searchMenu->addMenu(tr("&Bookmark"));
    bookmarkMenu->setObjectName("bookmarkMenu");
    bookmarkMenu->addAction(_toggleBookmarkAction);
    bookmarkMenu->addAction(_nextBookmarkAction);
    bookmarkMenu->addAction(_prevBookmarkAction);
    bookmarkMenu->addAction(_clearBookmarksAction);

    QMenu* markMenu = _searchMenu->addMenu(tr("&Mark"));
    markMenu->setObjectName("markMenu");
    QAction* markDialogAction = markMenu->addAction(tr("Mark..."));
    markDialogAction->setObjectName("markDialogAction");
    connect(markDialogAction, &QAction::triggered, this, [this]() {
        FindReplaceDlg* findDlg = ensureFindReplaceDialog();
        if (auto* view = currentActiveView())
            findDlg->setCurrentView(view);
        findDlg->openMarkTab();
    });
    addCommand(markMenu, tr("Jump Up"), "jumpUpMarkedAction",
               [this]() { jumpSearchMark(false); });
    addCommand(markMenu, tr("Jump Down"), "jumpDownMarkedAction",
               [this]() { jumpSearchMark(true); });
    addCommand(markMenu, tr("Clear All Marks"), "clearAllMarksAction",
               [this]() { clearSearchMarks(); });
    QMenu* styleTokenMenu = markMenu->addMenu(tr("Using Style"));
    styleTokenMenu->setObjectName("markUsingStyleMenu");
    static const char* const markStyleObjectNames[] = {
        "markStyle1Action", "markStyle2Action", "markStyle3Action",
        "markStyle4Action", "markStyle5Action"
    };
    for (int style = 1; style <= 5; ++style) {
        QAction* action = styleTokenMenu->addAction(
            tr("Style %1").arg(style));
        action->setObjectName(
            QString::fromLatin1(markStyleObjectNames[style - 1]));
        connect(action, &QAction::triggered, this, [this, style]() {
            ScintillaEditView* view = currentActiveView();
            if (!view || !view->hasSelectedText())
                return;
            const qintptr start = view->SendScintillaNpp(
                SCI_GETSELECTIONSTART);
            const qintptr end = view->SendScintillaNpp(
                SCI_GETSELECTIONEND);
            view->SendScintillaNpp(
                SCI_SETINDICATORCURRENT, 26 - style);
            view->SendScintillaNpp(
                SCI_INDICATORFILLRANGE,
                start, end - start);
        });
    }
    addCommand(styleTokenMenu, tr("Clear All Styles"),
               "clearStyleMarksAction", [this]() {
        if (auto* view = currentActiveView()) {
            const qintptr length = view->SendScintillaNpp(
                SCI_GETLENGTH);
            for (int style = 1; style <= 5; ++style) {
                view->SendScintillaNpp(
                    SCI_SETINDICATORCURRENT, 26 - style);
                view->SendScintillaNpp(
                    SCI_INDICATORCLEARRANGE, 0, length);
            }
        }
    });

    // ── View ─────────────────────────────────────────────────
    _viewMenu = menuBar()->addMenu(tr("&View"));
    _viewMenu->setObjectName("viewMenu");
    QAction* alwaysOnTopAction = _viewMenu->addAction(tr("Always on Top"));
    alwaysOnTopAction->setObjectName("alwaysOnTopAction");
    alwaysOnTopAction->setCheckable(true);
    connect(alwaysOnTopAction, &QAction::toggled, this, [this](bool enabled) {
        setWindowFlag(Qt::WindowStaysOnTopHint, enabled);
        show();
    });
    QAction* fullScreenAction = _viewMenu->addAction(tr("Toggle Full Screen Mode"));
    fullScreenAction->setObjectName("fullScreenAction");
    fullScreenAction->setCheckable(true);
    connect(fullScreenAction, &QAction::toggled, this, [this](bool enabled) {
        enabled ? showFullScreen() : showNormal();
    });
    QAction* distractionFreeAction =
        _viewMenu->addAction(tr("Distraction Free Mode"));
    distractionFreeAction->setObjectName("distractionFreeAction");
    distractionFreeAction->setCheckable(true);
    connect(distractionFreeAction, &QAction::toggled, this,
            [this](bool enabled) {
        menuBar()->setVisible(!enabled);
        statusBar()->setVisible(!enabled);
        for (QToolBar* bar : findChildren<QToolBar*>())
            bar->setVisible(!enabled);
        for (QDockWidget* dock : findChildren<QDockWidget*>())
            dock->setVisible(!enabled);
    });
    QAction* postItAction = _viewMenu->addAction(tr("Post-It"));
    postItAction->setObjectName("postItAction");
    postItAction->setCheckable(true);
    connect(postItAction, &QAction::toggled, this, [this](bool enabled) {
        menuBar()->setVisible(!enabled);
        statusBar()->setVisible(!enabled);
        if (_fileToolBar) _fileToolBar->setVisible(!enabled);
        setWindowFlag(Qt::FramelessWindowHint, enabled);
        setWindowFlag(Qt::WindowStaysOnTopHint, enabled);
        show();
    });
    _viewMenu->addSeparator();

    QAction* toolbarAction = _viewMenu->addAction(tr("Toolbar"));
    toolbarAction->setObjectName("toolbarAction");
    toolbarAction->setCheckable(true);
    toolbarAction->setChecked(NppParameters::getInstance().getNppGUI()._toolBarShow);
    connect(toolbarAction, &QAction::toggled, this, [this](bool visible) {
        if (_fileToolBar)
            _fileToolBar->setVisible(visible);
        NppParameters::getInstance().getNppGUI()._toolBarShow = visible;
    });

    QAction* statusBarAction = _viewMenu->addAction(tr("Status Bar"));
    statusBarAction->setObjectName("statusBarAction");
    statusBarAction->setCheckable(true);
    statusBarAction->setChecked(NppParameters::getInstance().getNppGUI()._statusBarShow);
    connect(statusBarAction, &QAction::toggled, this, [this](bool visible) {
        statusBar()->setVisible(visible);
        NppParameters::getInstance().getNppGUI()._statusBarShow = visible;
    });
    _viewMenu->addSeparator();

    // Show Symbol submenu
    QMenu* showSymMenu = _viewMenu->addMenu(tr("Show &Symbol"));
    showSymMenu->setObjectName("showSymMenu");
    showSymMenu->addAction(_showWhitespaceAction);
    QAction* showEolAction = showSymMenu->addAction(tr("Show End of Line"));
    showEolAction->setObjectName("showEolAction");
    showEolAction->setCheckable(true);
    connect(showEolAction, &QAction::toggled, this, [this](bool show) {
        if (auto* view = currentActiveView()) view->setEolVisibility(show);
    });
    QAction* showAllCharactersAction = showSymMenu->addAction(tr("Show All Characters"));
    showAllCharactersAction->setObjectName("showAllCharactersAction");
    showAllCharactersAction->setCheckable(true);
    connect(showAllCharactersAction, &QAction::toggled, this, [this](bool show) {
        if (auto* view = currentActiveView()) {
            view->setEolVisibility(show);
            view->setWhitespaceVisibility(show ? WsVisible
                                               : WsInvisible);
        }
    });
    showSymMenu->addAction(_showIndentAction);

    // Zoom submenu
    QMenu* zoomMenu = _viewMenu->addMenu(tr("&Zoom"));
    zoomMenu->setObjectName("zoomMenu");
    zoomMenu->addAction(_zoomInAction);
    zoomMenu->addAction(_zoomOutAction);
    zoomMenu->addAction(_zoomRestoreAction);

    // Move/Clone submenu
    QMenu* moveCloneMenu = _viewMenu->addMenu(tr("Move/Clone Current Document"));
    moveCloneMenu->setObjectName("moveCloneMenu");
    moveCloneMenu->addAction(_moveToOtherAction);
    moveCloneMenu->addAction(_cloneToOtherAction);

    _viewMenu->addAction(_wordWrapAction);
    addCommand(_viewMenu, tr("Hide Selected Lines"), "hideLinesAction",
               [this]() {
        if (auto* view = currentActiveView()) {
            const int start = view->SendScintillaNpp(
                SCI_LINEFROMPOSITION,
                view->SendScintillaNpp(SCI_GETSELECTIONSTART));
            const int end = view->SendScintillaNpp(
                SCI_LINEFROMPOSITION,
                view->SendScintillaNpp(SCI_GETSELECTIONEND));
            view->SendScintillaNpp(
                SCI_HIDELINES, start, end);
        }
    });
    addCommand(_viewMenu, tr("Show All Hidden Lines"), "showHiddenLinesAction",
               [this]() { if (auto* view = currentActiveView())
                    view->SendScintillaNpp(
                        SCI_SHOWLINES, 0, view->lines()); });
    addCommand(_viewMenu, tr("Text Direction RTL"), "textDirectionRtlAction",
               [this]() { if (auto* view = currentActiveView())
                    view->setLayoutDirection(Qt::RightToLeft); });
    addCommand(_viewMenu, tr("Text Direction LTR"), "textDirectionLtrAction",
               [this]() { if (auto* view = currentActiveView())
                    view->setLayoutDirection(Qt::LeftToRight); });
    addCommand(_viewMenu, tr("Document Summary"), "documentSummaryAction",
               [this]() {
        ScintillaEditView* view = currentActiveView();
        if (!view) return;
        const QString contents = view->text();
        const int words = contents.split(
            QRegularExpression(QStringLiteral("\\s+")),
            QString::SkipEmptyParts).size();
        QMessageBox::information(
            this, tr("Document Summary"),
            tr("Characters: %1\nWords: %2\nLines: %3")
                .arg(contents.size()).arg(words).arg(view->lines()));
    });
    QAction* monitoringAction = addCommand(
        _viewMenu, tr("Monitoring (tail -f)"), "monitoringAction",
        [this]() {
        Buffer* buffer = _activeDocTab
            ? _activeDocTab->currentBuffer() : nullptr;
        if (!buffer || buffer->isUntitled())
            return;
        const bool enabled = !buffer->isMonitoring();
        buffer->setMonitoring(enabled);
        const bool readOnly = enabled || buffer->isCommandLineReadOnly() ||
            !QFileInfo(buffer->getFullPath()).isWritable();
        buffer->setReadOnly(readOnly);
        for (DocTabView* tab : {_mainDocTab, _subDocTab})
            if (tab->currentBuffer() == buffer)
                tab->editor()->setReadOnly(readOnly);
        if (enabled) watchBufferFile(buffer);
    });
    monitoringAction->setCheckable(true);
    _viewMenu->addSeparator();
    _viewMenu->addAction(_splitViewAction);
    _viewMenu->addAction(_rotateSplitAction);
    QAction* syncVerticalAction = addCommand(
        _viewMenu, tr("Synchronize Vertical Scrolling"),
        "synchronizeVerticalAction", [this]() {
        QAction* action =
            findChild<QAction*>(QStringLiteral("synchronizeVerticalAction"));
        if (!action) return;
        const bool enabled = action->isChecked();
        for (QMetaObject::Connection& connection :
             _syncVerticalConnections)
            QObject::disconnect(connection);
        ScintillaEditView* mainView = _mainDocTab->editor();
        ScintillaEditView* subView = _subDocTab->editor();
        if (enabled && mainView && subView) {
            QScrollBar* first =
                mainView->verticalScrollBar();
            QScrollBar* second =
                subView->verticalScrollBar();
            _syncVerticalConnections[0] = connect(
                first, &QScrollBar::valueChanged,
                second, &QScrollBar::setValue);
            _syncVerticalConnections[1] = connect(
                second, &QScrollBar::valueChanged,
                first, &QScrollBar::setValue);
        }
    });
    syncVerticalAction->setCheckable(true);
    QAction* syncHorizontalAction = addCommand(
        _viewMenu, tr("Synchronize Horizontal Scrolling"),
        "synchronizeHorizontalAction", [this]() {
        QAction* action =
            findChild<QAction*>(QStringLiteral("synchronizeHorizontalAction"));
        if (!action) return;
        const bool enabled = action->isChecked();
        for (QMetaObject::Connection& connection :
             _syncHorizontalConnections)
            QObject::disconnect(connection);
        ScintillaEditView* mainView = _mainDocTab->editor();
        ScintillaEditView* subView = _subDocTab->editor();
        if (enabled && mainView && subView) {
            QScrollBar* first =
                mainView->horizontalScrollBar();
            QScrollBar* second =
                subView->horizontalScrollBar();
            _syncHorizontalConnections[0] = connect(
                first, &QScrollBar::valueChanged,
                second, &QScrollBar::setValue);
            _syncHorizontalConnections[1] = connect(
                second, &QScrollBar::valueChanged,
                first, &QScrollBar::setValue);
        }
    });
    syncHorizontalAction->setCheckable(true);
    QMenu* tabMenu = _viewMenu->addMenu(tr("Tab"));
    tabMenu->setObjectName("tabMenu");
    static const char* const activateTabObjectNames[] = {
        "activateTab1Action", "activateTab2Action", "activateTab3Action",
        "activateTab4Action", "activateTab5Action", "activateTab6Action",
        "activateTab7Action", "activateTab8Action", "activateTab9Action"
    };
    for (int tabNumber = 1; tabNumber <= 9; ++tabNumber) {
        QAction* tabAction = tabMenu->addAction(
            tr("Activate Tab %1").arg(tabNumber));
        tabAction->setObjectName(
            QString::fromLatin1(activateTabObjectNames[tabNumber - 1]));
        tabAction->setShortcut(
            QKeySequence(Qt::ALT | (Qt::Key_0 + tabNumber)));
        connect(tabAction, &QAction::triggered, this,
                [this, tabNumber]() {
            if (_activeDocTab && _activeDocTab->count() >= tabNumber)
                _activeDocTab->setCurrentIndex(tabNumber - 1);
        });
    }
    addCommand(tabMenu, tr("Move Tab Forward"), "moveTabForwardAction",
               [this]() {
        if (!_activeDocTab) return;
        const int from = _activeDocTab->currentIndex();
        if (from >= 0 && from + 1 < _activeDocTab->count())
            _activeDocTab->tabBar()->moveTab(from, from + 1);
    });
    addCommand(tabMenu, tr("Move Tab Backward"), "moveTabBackwardAction",
               [this]() {
        if (!_activeDocTab) return;
        const int from = _activeDocTab->currentIndex();
        if (from > 0)
            _activeDocTab->tabBar()->moveTab(from, from - 1);
    });
    QMenu* tabColorMenu = tabMenu->addMenu(tr("Apply Colour to Tab"));
    tabColorMenu->setObjectName("tabColorMenu");
    const QList<QColor> tabColors = {
        QColor(255, 128, 128), QColor(128, 255, 128),
        QColor(128, 192, 255), QColor(255, 192, 96),
        QColor(192, 128, 255)
    };
    static const char* const tabColorObjectNames[] = {
        "tabColor1Action", "tabColor2Action", "tabColor3Action",
        "tabColor4Action", "tabColor5Action"
    };
    for (int colorIndex = 0; colorIndex < tabColors.size(); ++colorIndex) {
        QAction* colorAction = tabColorMenu->addAction(
            tr("Colour %1").arg(colorIndex + 1));
        colorAction->setObjectName(
            QString::fromLatin1(tabColorObjectNames[colorIndex]));
        connect(colorAction, &QAction::triggered, this,
                [this, tabColors, colorIndex]() {
            if (_activeDocTab && _activeDocTab->currentIndex() >= 0)
                _activeDocTab->tabBar()->setTabTextColor(
                    _activeDocTab->currentIndex(),
                    tabColors.at(colorIndex));
        });
    }
    addCommand(tabColorMenu, tr("Remove Colour"), "removeTabColorAction",
               [this]() {
        if (_activeDocTab && _activeDocTab->currentIndex() >= 0)
            _activeDocTab->tabBar()->setTabTextColor(
                _activeDocTab->currentIndex(), palette().color(QPalette::Text));
    });
    _viewMenu->addSeparator();
    _viewMenu->addAction(_fileBrowserAction);
    _viewMenu->addAction(_docMapAction);
    _viewMenu->addAction(_funcListAction);
    addCommand(_viewMenu, tr("Document List"), "documentListAction", [this]() {
        _documentListDock->setVisible(!_documentListDock->isVisible());
    });
    QMenu* projectPanelsMenu = _viewMenu->addMenu(tr("Project Panels"));
    projectPanelsMenu->setObjectName("projectPanelsMenu");
    for (int panelIndex = 0; panelIndex < 3; ++panelIndex) {
        QAction* action = addCommand(projectPanelsMenu,
            tr("Project Panel %1").arg(panelIndex + 1),
            panelIndex == 0 ? "projectPanelsAction"
                            : QString("projectPanels%1Action").arg(panelIndex + 1).toLatin1().constData(),
            [this, panelIndex]() {
                QDockWidget* dock = _projectPanelsDock[panelIndex];
                dock->setVisible(!dock->isVisible());
                if (dock->isVisible())
                    resizeDocks({dock}, {207}, Qt::Horizontal);
            });
        action->setCheckable(true);
        connect(_projectPanelsDock[panelIndex], &QDockWidget::visibilityChanged,
                action, &QAction::setChecked);
    }
    addCommand(_viewMenu, tr("Clipboard History"), "clipboardHistoryAction", [this]() {
        _clipboardDock->setVisible(!_clipboardDock->isVisible());
    });
    addCommand(_viewMenu, tr("Character Panel"), "characterPanelAction", [this]() {
        _characterDock->setVisible(!_characterDock->isVisible());
    });
    _viewMenu->addSeparator();

    QMenu* foldMenu = _viewMenu->addMenu(tr("Fold All"));
    foldMenu->setObjectName("foldAllMenu");
    addCommand(foldMenu, tr("Fold Current Level"), "foldCurrentLevelAction",
               [this]() {
        if (auto* view = currentActiveView()) {
            int line = 0, index = 0;
            view->getCursorPosition(&line, &index);
            view->foldLine(line);
        }
    });
    addCommand(foldMenu, tr("Unfold Current Level"), "unfoldCurrentLevelAction",
               [this]() {
        if (auto* view = currentActiveView()) {
            int line = 0, index = 0;
            view->getCursorPosition(&line, &index);
            view->foldLine(line);
        }
    });
    addCommand(foldMenu, tr("Collapse All"), "collapseAllAction",
               [this]() { if (auto* view = currentActiveView())
                    view->SendScintilla(SCI_FOLDALL,
                                        SC_FOLDACTION_CONTRACT); });
    addCommand(foldMenu, tr("Uncollapse All"), "uncollapseAllAction",
               [this]() { if (auto* view = currentActiveView())
                    view->SendScintilla(SCI_FOLDALL,
                                        SC_FOLDACTION_EXPAND); });
    QMenu* foldLevelMenu = foldMenu->addMenu(tr("Collapse Level"));
    foldLevelMenu->setObjectName("foldLevelMenu");
    QMenu* unfoldLevelMenu = foldMenu->addMenu(tr("Uncollapse Level"));
    unfoldLevelMenu->setObjectName("unfoldLevelMenu");
    for (int level = 1; level <= 8; ++level) {
        auto addLevelAction = [this, level](
            QMenu* menu, bool expand, const QString& objectName) {
            QAction* action = menu->addAction(tr("Level %1").arg(level));
            action->setObjectName(objectName);
            connect(action, &QAction::triggered, this,
                    [this, level, expand]() {
                ScintillaEditView* view = currentActiveView();
                if (!view) return;
                for (int line = 0; line < view->lines(); ++line) {
                    const int foldLevel = view->SendScintillaNpp(
                        SCI_GETFOLDLEVEL, line);
                    if ((foldLevel & SC_FOLDLEVELNUMBERMASK) ==
                        SC_FOLDLEVELBASE + level - 1) {
                        view->SendScintillaNpp(
                            SCI_FOLDLINE, line,
                            expand ? SC_FOLDACTION_EXPAND
                                   : SC_FOLDACTION_CONTRACT);
                    }
                }
            });
        };
        addLevelAction(
            foldLevelMenu, false,
            QStringLiteral("collapseLevel%1Action").arg(level));
        addLevelAction(
            unfoldLevelMenu, true,
            QStringLiteral("uncollapseLevel%1Action").arg(level));
    }

    // ── Encoding ─────────────────────────────────────────────
    createEncodingMenu();

    // ── Language ─────────────────────────────────────────────
    _languageMenu = menuBar()->addMenu(tr("&Language"));
    _languageMenu->setObjectName("languageMenu");
    QAction* plainTextAct = _languageMenu->addAction(tr("None (Normal Text)"));
    plainTextAct->setObjectName("plainTextLanguageAction");
    connect(plainTextAct, &QAction::triggered, this, [this]() {
        if (auto* v = currentActiveView()) v->clearLexer();
    });

    auto addLang = [&](QMenu* menu, const QString& name, const QString& ext) {
        QAction* a = menu->addAction(name);
        a->setData(ext);
        connect(a, &QAction::triggered, this, [this, a]() {
            if (auto* v = currentActiveView()) {
                if (a->data().toString().isEmpty())
                    v->clearLexer();
                else
                    v->setLexerByExtension(a->data().toString());
            }
        });
    };

    _languageMenu->addSeparator();

    // A
    QMenu* langA = _languageMenu->addMenu("A");
    addLang(langA, "Ada",          "ada");
    addLang(langA, "ASP",          "asp");
    addLang(langA, "Assembly",     "asm");

    // B
    QMenu* langB = _languageMenu->addMenu("B");
    addLang(langB, "Batch",        "bat");
    addLang(langB, "Blitzbasic",   "bb");

    // C
    QMenu* langC = _languageMenu->addMenu("C");
    addLang(langC, "C",            "c");
    addLang(langC, "C#",           "cs");
    addLang(langC, "C++",          "cpp");
    addLang(langC, "CMake",        "cmake");
    addLang(langC, "CoffeeScript", "coffee");
    addLang(langC, "CSS",          "css");

    // D
    QMenu* langD = _languageMenu->addMenu("D");
    addLang(langD, "D",            "d");
    addLang(langD, "Diff",         "diff");

    // F
    QMenu* langF = _languageMenu->addMenu("F");
    addLang(langF, "Forth",        "forth");
    addLang(langF, "Fortran",      "f90");
    addLang(langF, "Freebasic",    "bi");

    // H
    QMenu* langH = _languageMenu->addMenu("H");
    addLang(langH, "Haskell",      "hs");
    addLang(langH, "HTML",         "html");

    // I-J
    QMenu* langIJ = _languageMenu->addMenu("I-J");
    addLang(langIJ, "INI file",   "ini");
    addLang(langIJ, "Java",       "java");
    addLang(langIJ, "JavaScript", "js");
    addLang(langIJ, "JSON",       "json");

    // L
    QMenu* langL = _languageMenu->addMenu("L");
    addLang(langL, "LaTeX",        "tex");
    addLang(langL, "LISP",         "lisp");
    addLang(langL, "Lua",          "lua");

    // M
    QMenu* langM = _languageMenu->addMenu("M");
    addLang(langM, "Makefile",     "makefile");
    addLang(langM, "Markdown",     "md");
    addLang(langM, "Matlab",       "m");

    // N
    QMenu* langN = _languageMenu->addMenu("N");
    addLang(langN, "NSIS",         "nsi");

    // P
    QMenu* langP = _languageMenu->addMenu("P");
    addLang(langP, "Pascal",       "pas");
    addLang(langP, "Perl",         "pl");
    addLang(langP, "PHP",          "php");
    addLang(langP, "PostScript",   "ps");
    addLang(langP, "PowerShell",   "ps1");
    addLang(langP, "Properties",   "properties");
    addLang(langP, "Python",       "py");

    // R
    QMenu* langR = _languageMenu->addMenu("R");
    addLang(langR, "R",            "r");
    addLang(langR, "Ruby",         "rb");
    addLang(langR, "Rust",         "rs");

    // S
    QMenu* langS = _languageMenu->addMenu("S");
    addLang(langS, "Shell",        "sh");
    addLang(langS, "Scheme",       "scm");
    addLang(langS, "SQL",          "sql");
    addLang(langS, "Swift",        "swift");

    // T
    QMenu* langT = _languageMenu->addMenu("T");
    addLang(langT, "TCL",          "tcl");
    addLang(langT, "TeX",          "tex");
    addLang(langT, "TypeScript",   "ts");

    // V
    QMenu* langV = _languageMenu->addMenu("V");
    addLang(langV, "Verilog",      "v");
    addLang(langV, "VHDL",         "vhd");
    addLang(langV, "Visual Basic", "vb");

    // X-Y
    QMenu* langXY = _languageMenu->addMenu("X-Y");
    addLang(langXY, "XML",          "xml");
    addLang(langXY, "YAML",         "yaml");

    const QVector<UserLangDesc>& userLangs = NppParameters::getInstance().getUserLangs();
    {
        _languageMenu->addSeparator();
        QMenu* userLangMenu = _languageMenu->addMenu(tr("User Defined Language"));
        userLangMenu->setObjectName("userDefinedLanguageMenu");
        for (const UserLangDesc& language : userLangs) {
            QAction* action = userLangMenu->addAction(language.name);
            action->setProperty("udlLanguage", true);
            connect(action, &QAction::triggered, this, [this, language]() {
                if (auto* view = currentActiveView()) view->setUserDefinedLanguage(language);
            });
        }
        userLangMenu->addSeparator();
        addCommand(userLangMenu, tr("Define your language..."),
                   "userDefinedLanguageDialogAction", [this, userLangMenu]() {
            QVector<UserLangDesc> editableLanguages =
                NppParameters::getInstance().getUserLangs();
            QDialog dialog(this);
            dialog.setWindowTitle(tr("User Defined Language"));
            dialog.resize(820, 620);
            QVBoxLayout layout(&dialog);
            QComboBox languageCombo;
            for (const UserLangDesc& language : editableLanguages)
                languageCombo.addItem(language.name);
            layout.addWidget(&languageCombo);
            QWidget managementBar;
            QHBoxLayout managementLayout(&managementBar);
            QPushButton newButton(tr("New"));
            QPushButton renameButton(tr("Rename"));
            QPushButton deleteButton(tr("Delete"));
            QPushButton importButton(tr("Import..."));
            QPushButton exportButton(tr("Export..."));
            managementLayout.addWidget(&newButton);
            managementLayout.addWidget(&renameButton);
            managementLayout.addWidget(&deleteButton);
            managementLayout.addStretch();
            managementLayout.addWidget(&importButton);
            managementLayout.addWidget(&exportButton);
            layout.addWidget(&managementBar);
            QTabWidget tabs;

            QWidget generalPage;
            QFormLayout generalLayout(&generalPage);
            QLineEdit extensions;
            QCheckBox caseSensitive(tr("Case sensitive"));
            QCheckBox foldComments(tr("Allow folding of comments"));
            QWidget prefixWidget;
            QGridLayout prefixLayout(&prefixWidget);
            QCheckBox* prefixChecks[8] = {};
            for (int i = 0; i < 8; ++i) {
                prefixChecks[i] = new QCheckBox(
                    tr("Keyword %1 supports prefixes").arg(i + 1),
                    &prefixWidget);
                prefixLayout.addWidget(prefixChecks[i], i / 2, i % 2);
            }
            generalLayout.addRow(tr("Extensions:"), &extensions);
            generalLayout.addRow(&caseSensitive);
            generalLayout.addRow(&foldComments);
            generalLayout.addRow(&prefixWidget);
            tabs.addTab(&generalPage, tr("General"));

            QTableWidget keywordTable(28, 2);
            keywordTable.setHorizontalHeaderLabels(
                {tr("Keyword list"), tr("Values")});
            keywordTable.horizontalHeader()->setStretchLastSection(true);
            const QStringList keywordNames = {
                "Comments", "Numbers, prefix1", "Numbers, prefix2",
                "Numbers, extras1", "Numbers, extras2",
                "Numbers, suffix1", "Numbers, suffix2", "Numbers, range",
                "Operators1", "Operators2", "Folders in code1, open",
                "Folders in code1, middle", "Folders in code1, close",
                "Folders in code2, open", "Folders in code2, middle",
                "Folders in code2, close", "Folders in comment, open",
                "Folders in comment, middle", "Folders in comment, close",
                "Keywords1", "Keywords2", "Keywords3", "Keywords4",
                "Keywords5", "Keywords6", "Keywords7", "Keywords8",
                "Delimiters"
            };
            for (int row = 0; row < keywordNames.size(); ++row) {
                QTableWidgetItem* name =
                    new QTableWidgetItem(keywordNames.at(row));
                name->setFlags(name->flags() & ~Qt::ItemIsEditable);
                keywordTable.setItem(row, 0, name);
            }
            tabs.addTab(&keywordTable, tr("Keywords"));

            QTableWidget styleTable;
            styleTable.setColumnCount(7);
            styleTable.setHorizontalHeaderLabels({
                tr("Style"), tr("Foreground"), tr("Background"),
                tr("Font"), tr("Size"), tr("Font style"), tr("Nesting")
            });
            styleTable.horizontalHeader()->setStretchLastSection(true);
            tabs.addTab(&styleTable, tr("Styles"));
            layout.addWidget(&tabs);

            auto populate = [&](int index) {
                if (index < 0 || index >= editableLanguages.size()) {
                    extensions.clear();
                    keywordTable.clearContents();
                    styleTable.setRowCount(0);
                    return;
                }
                const UserLangDesc& language =
                    editableLanguages.at(index);
                extensions.setText(language.exts.join(QStringLiteral(" ")));
                caseSensitive.setChecked(language.caseSensitive);
                foldComments.setChecked(language.foldComments);
                for (int i = 0; i < 8; ++i)
                    prefixChecks[i]->setChecked(language.prefixKeywords[i]);
                for (int i = 0; i < 28; ++i)
                    keywordTable.setItem(
                        i, 1, new QTableWidgetItem(language.keywordLists[i]));
                styleTable.setRowCount(language.styles.size());
                for (int row = 0; row < language.styles.size(); ++row) {
                    const WordsStyle& style = language.styles.at(row);
                    styleTable.setItem(row, 0,
                        new QTableWidgetItem(style.name));
                    styleTable.setItem(row, 1, new QTableWidgetItem(
                        style.hasFg ? style.fgColor.name().mid(1).toUpper()
                                    : QString()));
                    styleTable.setItem(row, 2, new QTableWidgetItem(
                        style.hasBg ? style.bgColor.name().mid(1).toUpper()
                                    : QString()));
                    styleTable.setItem(row, 3,
                        new QTableWidgetItem(style.fontName));
                    styleTable.setItem(row, 4,
                        new QTableWidgetItem(QString::number(style.fontSize)));
                    styleTable.setItem(row, 5,
                        new QTableWidgetItem(QString::number(style.fontStyle)));
                    styleTable.setItem(row, 6,
                        new QTableWidgetItem(QString::number(style.nesting)));
                }
            };
            connect(&languageCombo,
                    QOverload<int>::of(&QComboBox::currentIndexChanged),
                    &dialog, populate);
            auto reloadLanguages = [&](const QString& selectName) {
                editableLanguages =
                    NppParameters::getInstance().getUserLangs();
                languageCombo.blockSignals(true);
                languageCombo.clear();
                int selected = -1;
                for (int i = 0; i < editableLanguages.size(); ++i) {
                    languageCombo.addItem(editableLanguages.at(i).name);
                    if (editableLanguages.at(i).name == selectName)
                        selected = i;
                }
                languageCombo.blockSignals(false);
                languageCombo.setCurrentIndex(
                    selected >= 0 ? selected
                                  : (editableLanguages.isEmpty() ? -1 : 0));
                populate(languageCombo.currentIndex());
            };
            connect(&newButton, &QPushButton::clicked, &dialog, [&]() {
                bool ok = false;
                const QString name = QInputDialog::getText(
                    &dialog, tr("New User Defined Language"),
                    tr("Name:"), QLineEdit::Normal, QString(), &ok).trimmed();
                if (!ok || name.isEmpty())
                    return;
                if (!NppParameters::getInstance()
                         .createUserDefinedLanguage(name)) {
                    QMessageBox::warning(
                        &dialog, tr("User Defined Language"),
                        tr("The language could not be created. "
                           "Its name may already exist."));
                    return;
                }
                reloadLanguages(name);
            });
            connect(&renameButton, &QPushButton::clicked, &dialog, [&]() {
                const int index = languageCombo.currentIndex();
                if (index < 0 || index >= editableLanguages.size())
                    return;
                const UserLangDesc oldLanguage =
                    editableLanguages.at(index);
                bool ok = false;
                const QString name = QInputDialog::getText(
                    &dialog, tr("Rename User Defined Language"),
                    tr("Name:"), QLineEdit::Normal,
                    oldLanguage.name, &ok).trimmed();
                if (!ok || name.isEmpty() || name == oldLanguage.name)
                    return;
                if (!NppParameters::getInstance().renameUserDefinedLanguage(
                        oldLanguage.name, name,
                        oldLanguage.sourceFilePath)) {
                    QMessageBox::warning(
                        &dialog, tr("User Defined Language"),
                        tr("The language could not be renamed."));
                    return;
                }
                reloadLanguages(name);
            });
            connect(&deleteButton, &QPushButton::clicked, &dialog, [&]() {
                const int index = languageCombo.currentIndex();
                if (index < 0 || index >= editableLanguages.size())
                    return;
                const UserLangDesc language =
                    editableLanguages.at(index);
                if (QMessageBox::question(
                        &dialog, tr("Delete User Defined Language"),
                        tr("Delete \"%1\"?").arg(language.name)) !=
                    QMessageBox::Yes)
                    return;
                if (NppParameters::getInstance()
                        .deleteUserDefinedLanguage(
                            language.name, language.sourceFilePath))
                    reloadLanguages(QString());
            });
            connect(&importButton, &QPushButton::clicked, &dialog, [&]() {
                const QString path = QFileDialog::getOpenFileName(
                    &dialog, tr("Import User Defined Language"),
                    QString(), tr("XML files (*.xml)"));
                if (path.isEmpty())
                    return;
                if (!NppParameters::getInstance()
                         .importUserDefinedLanguages(path)) {
                    QMessageBox::warning(
                        &dialog, tr("User Defined Language"),
                        tr("No new language could be imported."));
                }
                reloadLanguages(QString());
            });
            connect(&exportButton, &QPushButton::clicked, &dialog, [&]() {
                const int index = languageCombo.currentIndex();
                if (index < 0 || index >= editableLanguages.size())
                    return;
                const QString path = QFileDialog::getSaveFileName(
                    &dialog, tr("Export User Defined Language"),
                    editableLanguages.at(index).name + QStringLiteral(".xml"),
                    tr("XML files (*.xml)"));
                if (!path.isEmpty() &&
                    !NppParameters::getInstance()
                         .exportUserDefinedLanguage(
                             editableLanguages.at(index).name, path)) {
                    QMessageBox::warning(
                        &dialog, tr("User Defined Language"),
                        tr("The language could not be exported."));
                }
            });
            populate(0);

            QDialogButtonBox buttons(
                QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
            layout.addWidget(&buttons);
            connect(&buttons, &QDialogButtonBox::accepted,
                    &dialog, &QDialog::accept);
            connect(&buttons, &QDialogButtonBox::rejected,
                    &dialog, &QDialog::reject);
            connect(&dialog, &QDialog::finished, this,
                    [this, userLangMenu](int) {
                for (QAction* action : userLangMenu->actions()) {
                    if (action->property("udlLanguage").toBool()) {
                        userLangMenu->removeAction(action);
                        action->deleteLater();
                    }
                }
                QAction* before = nullptr;
                for (QAction* action : userLangMenu->actions()) {
                    if (action->isSeparator() ||
                        action->objectName() ==
                            QStringLiteral("userDefinedLanguageDialogAction")) {
                        before = action;
                        break;
                    }
                }
                for (const UserLangDesc& language :
                     NppParameters::getInstance().getUserLangs()) {
                    QAction* action = new QAction(language.name, userLangMenu);
                    action->setProperty("udlLanguage", true);
                    userLangMenu->insertAction(before, action);
                    connect(action, &QAction::triggered, this,
                            [this, language]() {
                        if (auto* view = currentActiveView())
                            view->setUserDefinedLanguage(language);
                    });
                }
            });
            if (dialog.exec() != QDialog::Accepted)
                return;

            const int index = languageCombo.currentIndex();
            if (index < 0 || index >= editableLanguages.size())
                return;
            UserLangDesc language = editableLanguages.at(index);
            language.exts = extensions.text().toLower().split(
                QRegularExpression(QStringLiteral("\\s+")),
                QString::SkipEmptyParts);
            language.caseSensitive = caseSensitive.isChecked();
            language.foldComments = foldComments.isChecked();
            for (int i = 0; i < 8; ++i)
                language.prefixKeywords[i] = prefixChecks[i]->isChecked();
            for (int i = 0; i < 28; ++i)
                language.keywordLists[i] =
                    keywordTable.item(i, 1)->text().trimmed();
            for (int row = 0; row < language.styles.size(); ++row) {
                WordsStyle& style = language.styles[row];
                const QColor foreground(
                    QStringLiteral("#") + styleTable.item(row, 1)->text());
                const QColor background(
                    QStringLiteral("#") + styleTable.item(row, 2)->text());
                style.hasFg = foreground.isValid();
                style.hasBg = background.isValid();
                if (style.hasFg) style.fgColor = foreground;
                if (style.hasBg) style.bgColor = background;
                style.fontName = styleTable.item(row, 3)->text();
                style.fontSize = styleTable.item(row, 4)->text().toInt();
                style.fontStyle = styleTable.item(row, 5)->text().toInt();
                style.nesting = styleTable.item(row, 6)->text().toInt();
            }
            if (!NppParameters::getInstance()
                    .writeUserDefinedLanguage(language)) {
                QMessageBox::warning(
                    this, tr("User Defined Language"),
                    tr("Could not save userDefineLang.xml."));
                return;
            }
            if (auto* view = currentActiveView())
                view->setUserDefinedLanguage(language);
        });
    }

    // ── Settings ─────────────────────────────────────────────
    _settingsMenu = menuBar()->addMenu(tr("Se&ttings"));
    _settingsMenu->setObjectName("settingsMenu");
    _settingsMenu->addAction(_preferencesAction);
    addCommand(_settingsMenu, tr("Style Configurator..."), "styleConfiguratorAction",
               [this]() {
        NppParameters& params = NppParameters::getInstance();
        QVector<LexerStyler> lexers = params.getLexerStylers();
        QVector<WordsStyle> globals = params.getGlobalStyles();
        QDialog dialog(this);
        dialog.setWindowTitle(tr("Style Configurator"));
        dialog.resize(920, 620);
        QVBoxLayout layout(&dialog);
        QComboBox languageCombo;
        languageCombo.addItem(tr("Global Styles"));
        for (const LexerStyler& lexer : lexers)
            languageCombo.addItem(
                lexer.desc.isEmpty() ? lexer.name : lexer.desc);
        layout.addWidget(&languageCombo);
        QTableWidget styles;
        styles.setColumnCount(9);
        styles.setHorizontalHeaderLabels({
            tr("Style"), tr("Foreground"), tr("Background"), tr("Font"),
            tr("Size"), tr("Bold"), tr("Italic"), tr("Underline"),
            tr("User-defined keywords")
        });
        styles.horizontalHeader()->setSectionResizeMode(
            0, QHeaderView::Stretch);
        styles.horizontalHeader()->setSectionResizeMode(
            3, QHeaderView::Stretch);
        layout.addWidget(&styles);
        QLabel hint(tr("Double-click a colour cell to choose a colour. "
                       "Leave it empty to inherit the default."));
        layout.addWidget(&hint);

        int previousLanguage = 0;
        auto selectedStyles = [&]() -> QVector<WordsStyle>* {
            return previousLanguage == 0
                ? &globals : &lexers[previousLanguage - 1].styles;
        };
        auto saveTable = [&]() {
            QVector<WordsStyle>* values = selectedStyles();
            const int count = qMin(values->size(), styles.rowCount());
            for (int row = 0; row < count; ++row) {
                WordsStyle& style = (*values)[row];
                auto text = [&](int column) {
                    QTableWidgetItem* item = styles.item(row, column);
                    return item ? item->text().trimmed() : QString();
                };
                const QColor foreground(
                    QStringLiteral("#") + text(1));
                const QColor background(
                    QStringLiteral("#") + text(2));
                style.hasFg = !text(1).isEmpty() && foreground.isValid();
                style.hasBg = !text(2).isEmpty() && background.isValid();
                if (style.hasFg) style.fgColor = foreground;
                if (style.hasBg) style.bgColor = background;
                style.fontName = text(3);
                style.fontSize = text(4).toInt();
                style.fontStyle =
                    (styles.item(row, 5)->checkState() == Qt::Checked ? 1 : 0) |
                    (styles.item(row, 6)->checkState() == Qt::Checked ? 2 : 0) |
                    (styles.item(row, 7)->checkState() == Qt::Checked ? 4 : 0);
                style.userKeywords = text(8);
            }
        };
        auto populate = [&](int languageIndex) {
            previousLanguage = languageIndex;
            QVector<WordsStyle>* values = selectedStyles();
            styles.setRowCount(values->size());
            for (int row = 0; row < values->size(); ++row) {
                const WordsStyle& style = values->at(row);
                QTableWidgetItem* name = new QTableWidgetItem(style.name);
                name->setFlags(name->flags() & ~Qt::ItemIsEditable);
                styles.setItem(row, 0, name);
                styles.setItem(row, 1, new QTableWidgetItem(
                    style.hasFg
                        ? style.fgColor.name().mid(1).toUpper() : QString()));
                styles.setItem(row, 2, new QTableWidgetItem(
                    style.hasBg
                        ? style.bgColor.name().mid(1).toUpper() : QString()));
                styles.setItem(row, 3, new QTableWidgetItem(style.fontName));
                styles.setItem(row, 4, new QTableWidgetItem(
                    style.fontSize > 0 ? QString::number(style.fontSize)
                                       : QString()));
                for (int column = 5; column <= 7; ++column) {
                    QTableWidgetItem* check = new QTableWidgetItem;
                    check->setFlags(Qt::ItemIsEnabled |
                                    Qt::ItemIsUserCheckable);
                    check->setCheckState(
                        style.fontStyle & (1 << (column - 5))
                            ? Qt::Checked : Qt::Unchecked);
                    styles.setItem(row, column, check);
                }
                QTableWidgetItem* keywords =
                    new QTableWidgetItem(style.userKeywords);
                if (style.keywordClass.isEmpty())
                    keywords->setFlags(
                        keywords->flags() & ~Qt::ItemIsEditable);
                styles.setItem(row, 8, keywords);
            }
        };
        connect(&languageCombo,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                &dialog, [&](int index) {
            saveTable();
            populate(index);
        });
        connect(&styles, &QTableWidget::cellDoubleClicked, &dialog,
                [&](int row, int column) {
            if (column != 1 && column != 2)
                return;
            const QString current = styles.item(row, column)->text();
            const QColor color = QColorDialog::getColor(
                QColor(QStringLiteral("#") + current), &dialog);
            if (color.isValid())
                styles.item(row, column)->setText(
                    color.name().mid(1).toUpper());
        });
        populate(0);
        QDialogButtonBox buttons(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        layout.addWidget(&buttons);
        connect(&buttons, &QDialogButtonBox::accepted,
                &dialog, &QDialog::accept);
        connect(&buttons, &QDialogButtonBox::rejected,
                &dialog, &QDialog::reject);
        if (dialog.exec() != QDialog::Accepted)
            return;
        saveTable();
        params.getLexerStylers() = lexers;
        params.getGlobalStyles() = globals;
        if (!params.writeStylers()) {
            QMessageBox::warning(
                this, tr("Style Configurator"),
                tr("Could not save stylers.xml."));
            params.loadStylers();
            return;
        }
        applyPreferencesToAllViews();
        for (ScintillaEditView* view :
             findChildren<ScintillaEditView*>())
            view->reloadConfiguredStyles();
    });
    addCommand(_settingsMenu, tr("Shortcut Mapper..."), "shortcutMapperAction",
               [this]() {
        QList<QAction*> actions;
        std::function<void(QMenu*)> appendMenuActions =
            [&](QMenu* menu) {
            for (QAction* action : menu->actions()) {
                if (QMenu* subMenu = action->menu()) {
                    appendMenuActions(subMenu);
                } else if (!action->isSeparator()
                    && action->property("nppCommandId").isValid()
                    && !action->property("userCommandIndex").isValid()
                    && action->objectName() != "containingFolderAsWorkspaceAction"
                    && action->objectName() != "restoreLastClosedFileAction"
                    && !actions.contains(action)) {
                    actions.append(action);
                }
            }
        };
        for (QAction* topLevel : menuBar()->actions()) {
            if (QMenu* menu = topLevel->menu())
                appendMenuActions(menu);
        }
        ShortcutMapper mapper(actions, this);
        NativeLangSpeaker& speaker =
            NppParameters::getInstance().getNativeLangSpeaker();
        if (speaker.isLoaded())
            speaker.changeDlgLang(&mapper, "ShortcutMapper");
        mapper.exec();
        if (mapper.shortcutsChanged()) {
            rebuildConfiguredMacroMenu();
            applyPreferencesToAllViews();
        }
    });

    QMenu* importMenu = _settingsMenu->addMenu(tr("Import"));
    importMenu->setObjectName("importMenu");
    addPlaceholder(importMenu, tr("Import plugin(s)..."), "importPluginsAction");
    addCommand(importMenu, tr("Import style theme(s)..."), "importStyleThemesAction",
               [this]() {
        const QStringList files = QFileDialog::getOpenFileNames(
            this, tr("Import Style Themes"), QString(), tr("XML Files (*.xml)"));
        if (files.isEmpty())
            return;
        const QString targetDir =
            NppParameters::getInstance().getUserPath() + "/themes";
        QDir().mkpath(targetDir);
        int imported = 0;
        for (const QString& source : files) {
            const QString target = targetDir + "/" + QFileInfo(source).fileName();
            if (QFile::exists(target))
                QFile::remove(target);
            if (QFile::copy(source, target))
                ++imported;
        }
        statusBar()->showMessage(tr("%1 theme(s) imported.").arg(imported), 4000);
    });
    addCommand(_settingsMenu, tr("Edit Popup ContextMenu"), "editContextMenuAction",
               [this]() {
        doOpenFile(NppParameters::getInstance().getUserPath() + "/contextMenu.xml");
    });

    // ── Tools ────────────────────────────────────────────────
    QMenu* toolsMenu = menuBar()->addMenu(tr("T&ools"));
    toolsMenu->setObjectName("toolsMenu");
    QMenu* md5Menu = toolsMenu->addMenu(tr("MD5"));
    md5Menu->setObjectName("md5Menu");
    auto hashText = [this](QCryptographicHash::Algorithm algorithm,
                           const QString& title) {
        bool ok = false;
        const QString input = QInputDialog::getMultiLineText(
            this, title, tr("Text:"), QString(), &ok);
        if (!ok)
            return;
        const QString digest = QString::fromLatin1(
            QCryptographicHash::hash(input.toUtf8(), algorithm).toHex());
        QMessageBox::information(this, title, digest);
    };
    auto hashFiles = [this](QCryptographicHash::Algorithm algorithm,
                            const QString& title) {
        const QStringList files = QFileDialog::getOpenFileNames(this, title);
        if (files.isEmpty())
            return;
        QStringList output;
        for (const QString& path : files) {
            QFile file(path);
            if (!file.open(QFile::ReadOnly))
                continue;
            QCryptographicHash hash(algorithm);
            hash.addData(&file);
            output << QString("%1  %2")
                .arg(QString::fromLatin1(hash.result().toHex()),
                     QFileInfo(path).fileName());
        }
        QMessageBox::information(this, title, output.join('\n'));
    };
    auto hashToClipboard = [this](QCryptographicHash::Algorithm algorithm) {
        ScintillaEditView* view = currentActiveView();
        if (!view)
            return;
        const QString text = view->hasSelectedText()
            ? view->selectedText()
            : view->text();
        QApplication::clipboard()->setText(QString::fromLatin1(
            QCryptographicHash::hash(text.toUtf8(), algorithm).toHex()));
        statusBar()->showMessage(tr("Hash copied to clipboard."), 2500);
    };
    addCommand(md5Menu, tr("Generate..."), "md5GenerateAction",
               [hashText]() { hashText(QCryptographicHash::Md5, "MD5"); });
    addCommand(md5Menu, tr("Generate from files..."), "md5FromFilesAction",
               [hashFiles]() { hashFiles(QCryptographicHash::Md5, "MD5"); });
    addCommand(md5Menu, tr("Generate into clipboard"), "md5ToClipboardAction",
               [hashToClipboard]() { hashToClipboard(QCryptographicHash::Md5); });
    QMenu* shaMenu = toolsMenu->addMenu(tr("SHA-256"));
    shaMenu->setObjectName("sha256Menu");
    addCommand(shaMenu, tr("Generate..."), "sha256GenerateAction",
               [hashText]() { hashText(QCryptographicHash::Sha256, "SHA-256"); });
    addCommand(shaMenu, tr("Generate from files..."), "sha256FromFilesAction",
               [hashFiles]() { hashFiles(QCryptographicHash::Sha256, "SHA-256"); });
    addCommand(shaMenu, tr("Generate into clipboard"), "sha256ToClipboardAction",
               [hashToClipboard]() { hashToClipboard(QCryptographicHash::Sha256); });

    // ── Macro ────────────────────────────────────────────────
    _macroMenu = menuBar()->addMenu(tr("&Macro"));
    _macroMenu->setObjectName("macroMenu");
    _macroMenu->addAction(_startRecordAction);
    _macroMenu->addAction(_stopRecordAction);
    _macroMenu->addAction(_playMacroAction);
    _macroMenu->addSeparator();
    _macroMenu->addAction(_saveMacroAction);
    _macroMenu->addAction(_loadMacroAction);

    // ── Run ──────────────────────────────────────────────────
    QMenu* runMenu = menuBar()->addMenu(tr("&Run"));
    runMenu->setObjectName("runMenu");
    addCommand(runMenu, tr("Run..."), "runDialogAction", [this]() {
        bool ok = false;
        const QString command = QInputDialog::getText(
            this, tr("Run"), tr("Command:"), QLineEdit::Normal,
            QString(), &ok).trimmed();
        if (ok && !command.isEmpty() && !QProcess::startDetached(command))
            QMessageBox::warning(this, tr("Run"), tr("Could not start command."));
    });
    const QVector<UserCommandDef>& userCommands =
        NppParameters::getInstance().getUserCommands();
    if (!userCommands.isEmpty())
        runMenu->addSeparator();
    for (int commandIndex = 0; commandIndex < userCommands.size();
         ++commandIndex) {
        const UserCommandDef command = userCommands.at(commandIndex);
        QAction* action = runMenu->addAction(command.name);
        action->setProperty("userCommandIndex", commandIndex + 1);
        QString objectName = command.name;
        objectName.remove(QRegularExpression("[^A-Za-z0-9_]"));
        action->setObjectName("userCommand_" + objectName);
        int key = command.key;
        if (key >= 112 && key <= 123)
            key = Qt::Key_F1 + key - 112;
        if (key > 0) {
            int sequence = key;
            if (command.ctrl) sequence |= Qt::CTRL;
            if (command.alt) sequence |= Qt::ALT;
            if (command.shift) sequence |= Qt::SHIFT;
            action->setShortcut(QKeySequence(sequence));
        }
        connect(action, &QAction::triggered, this, [this, command]() {
            Buffer* buffer = _activeDocTab ? _activeDocTab->currentBuffer() : nullptr;
            ScintillaEditView* view = currentActiveView();
            const QString fullPath =
                buffer && !buffer->isUntitled() ? buffer->getFullPath() : QString();
            QFileInfo info(fullPath);
            int line = 0;
            int column = 0;
            QString currentWord;
            if (view) {
                view->getCursorPosition(&line, &column);
                currentWord = view->hasSelectedText()
                    ? view->selectedText()
                    : view->wordAtLineIndex(line, column);
            }
            QString expanded = command.command;
            const QMap<QString, QString> variables = {
                {"$(FULL_CURRENT_PATH)", fullPath},
                {"$(CURRENT_DIRECTORY)", info.absolutePath()},
                {"$(FILE_NAME)", info.fileName()},
                {"$(NAME_PART)", info.completeBaseName()},
                {"$(EXT_PART)", info.suffix()},
                {"$(CURRENT_WORD)", currentWord},
                {"$(CURRENT_LINE)", QString::number(line + 1)},
                {"$(CURRENT_COLUMN)", QString::number(column + 1)},
                {"$(NPP_DIRECTORY)", QCoreApplication::applicationDirPath()},
                {"$(NPP_FULL_FILE_PATH)", QCoreApplication::applicationFilePath()}
            };
            for (auto it = variables.constBegin(); it != variables.constEnd(); ++it)
                expanded.replace(it.key(), it.value(), Qt::CaseInsensitive);
            if (expanded.startsWith("http://", Qt::CaseInsensitive) ||
                expanded.startsWith("https://", Qt::CaseInsensitive)) {
                QDesktopServices::openUrl(QUrl(expanded));
            } else if (!QProcess::startDetached(expanded)) {
                QMessageBox::warning(this, tr("Run"),
                                     tr("Could not start command."));
            }
        });
    }

    // ── Plugins ──────────────────────────────────────────────
    _pluginsMenu = menuBar()->addMenu(tr("&Plugins"));
    _pluginsMenu->setObjectName("pluginsMenu");
    bool hasLoadedPlugins = false;
#ifdef ENABLE_PLUGIN_SYSTEM
    if (_pluginManager) {
        for (IPlugin* plugin : _pluginManager->plugins()) {
            QMenu* sub = _pluginsMenu->addMenu(plugin->getName());
            for (QAction* act : plugin->getMenuActions())
                sub->addAction(act);
            hasLoadedPlugins = true;
        }
    }
#endif
    if (!hasLoadedPlugins) {
        QAction* noPlugins = _pluginsMenu->addAction(tr("No plugins loaded"));
        noPlugins->setObjectName("noPluginsLoadedAction");
        noPlugins->setEnabled(false);
    }
    _pluginsMenu->addSeparator();
    addCommand(_pluginsMenu, tr("Plugins Admin..."),
               "pluginsAdminAction",
               [this]() { showPluginAdmin(); });
    addCommand(_pluginsMenu, tr("Open Plugins Folder"),
               "openPluginsFolderAction", []() {
        const QString pluginRoot =
            QDir(NppParameters::getInstance().getNppPath())
                .filePath(QStringLiteral("plugins"));
        QDir().mkpath(pluginRoot);
        QDesktopServices::openUrl(QUrl::fromLocalFile(pluginRoot));
    });

    // ── Window ───────────────────────────────────────────────
    QMenu* windowMenu = menuBar()->addMenu(tr("&Window"));
    windowMenu->setObjectName("windowMenu");
    addCommand(windowMenu, tr("Windows..."), "windowsDialogAction", [this]() {
        QDialog dlg(this);
        dlg.setWindowTitle(tr("Windows"));
        dlg.resize(520, 380);
        QVBoxLayout* layout = new QVBoxLayout(&dlg);
        QListWidget* list = new QListWidget(&dlg);
        QSet<Buffer*> visited;
        auto appendTab = [&](DocTabView* tab) {
            for (int i = 0; tab && i < tab->count(); ++i) {
                Buffer* buffer = tab->bufferAt(i);
                if (!buffer || visited.contains(buffer))
                    continue;
                visited.insert(buffer);
                QListWidgetItem* item = new QListWidgetItem(
                    buffer->isUntitled() ? buffer->getFileName()
                                         : buffer->getFullPath(), list);
                item->setData(Qt::UserRole, QVariant::fromValue<quintptr>(
                    reinterpret_cast<quintptr>(buffer)));
            }
        };
        appendTab(_mainDocTab);
        appendTab(_subDocTab);
        layout->addWidget(list);
        QDialogButtonBox* buttons = new QDialogButtonBox(
            QDialogButtonBox::Close, &dlg);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        connect(list, &QListWidget::itemActivated, &dlg,
                [this, &dlg](QListWidgetItem* item) {
            Buffer* buffer = reinterpret_cast<Buffer*>(
                item->data(Qt::UserRole).value<quintptr>());
            if (_mainDocTab->indexOfBuffer(buffer) >= 0) {
                setActiveTab(_mainDocTab);
                _mainDocTab->activateBuffer(buffer);
            } else if (_subDocTab->indexOfBuffer(buffer) >= 0) {
                setActiveTab(_subDocTab);
                _subDocTab->activateBuffer(buffer);
            }
            dlg.accept();
        });
        dlg.exec();
    });
    windowMenu->addSeparator();
    addCommand(windowMenu, tr("Sort by Name A to Z"), "sortByNameAscAction",
               [this]() { _activeDocTab->sortBuffersByName(true); });
    addCommand(windowMenu, tr("Sort by Name Z to A"), "sortByNameDescAction",
               [this]() { _activeDocTab->sortBuffersByName(false); });
    auto copyDocumentNames = [this](bool fullPath) {
        QStringList values;
        QSet<Buffer*> visited;
        for (DocTabView* tab : {_mainDocTab, _subDocTab}) {
            for (int i = 0; tab && i < tab->count(); ++i) {
                Buffer* buffer = tab->bufferAt(i);
                if (!buffer || visited.contains(buffer))
                    continue;
                visited.insert(buffer);
                values << (fullPath && !buffer->isUntitled()
                    ? buffer->getFullPath()
                    : buffer->getFileName());
            }
        }
        QApplication::clipboard()->setText(values.join('\n'));
    };
    addCommand(windowMenu, tr("Copy Name(s) to Clipboard"), "copyNamesAction",
               [copyDocumentNames]() { copyDocumentNames(false); });
    addCommand(windowMenu, tr("Copy Pathname(s) to Clipboard"), "copyPathsAction",
               [copyDocumentNames]() { copyDocumentNames(true); });

    // ── Help ─────────────────────────────────────────────────
    _helpMenu = menuBar()->addMenu(tr("&?"));
    _helpMenu->setObjectName("helpMenu");
    addCommand(_helpMenu, tr("Notepad++ Home"), "homeAction", []() {
        QDesktopServices::openUrl(QUrl("https://notepad-plus-plus.org/"));
    });
    addCommand(_helpMenu, tr("Online Documentation"), "onlineDocsAction", []() {
        QDesktopServices::openUrl(
            QUrl("https://npp-user-manual.org/docs/getting-started/"));
    });
    addCommand(_helpMenu, tr("Command Line Arguments"), "cmdArgsAction", []() {
        QDesktopServices::openUrl(
            QUrl("https://npp-user-manual.org/docs/command-prompt/"));
    });
    addCommand(_helpMenu, tr("Debug Info..."), "debugInfoAction", [this]() {
        const QString info = tr(
            "Notepad++ Qt\n"
            "Qt: %1\n"
            "Operating system: %2\n"
            "Application path: %3\n"
            "Configuration path: %4")
            .arg(qVersion(), QSysInfo::prettyProductName(),
                 QCoreApplication::applicationFilePath(),
                 NppParameters::getInstance().getUserPath());
        QApplication::clipboard()->setText(info);
        QMessageBox::information(this, tr("Debug Info"), info);
    });
    _helpMenu->addSeparator();
    _helpMenu->addAction(_aboutAction);

    menuBar()->setVisible(NppParameters::getInstance().getNppGUI()._menuBarShow);
}

// ── 动作状态同步 ──────────────────────────────────────────────────────────────

void MainWindow::initActionStates()
{
    // 从配置初始化 toggle 按钮的 checked 状态
    const ScintillaViewParams& svp = NppParameters::getInstance().getSVP();
    _wordWrapAction->setChecked(svp._doWrap);
    _showWhitespaceAction->setChecked(svp._whiteSpaceShow);
    _showIndentAction->setChecked(svp._indentGuideLineShow);
    _splitViewAction->setChecked(_subDocTab->isVisible());

    // 初始化各按钮 enabled 状态
    updateActionStates();
}

void MainWindow::updateActionStates()
{
    Buffer*            buf  = _activeDocTab ? _activeDocTab->currentBuffer() : nullptr;
    ScintillaEditView* view = currentActiveView();

    const bool hasBuf     = (buf  != nullptr);
    const bool isDirty    = hasBuf && buf->isDirty();
    const bool isUntitled = hasBuf && buf->isUntitled();
    const bool hasSel     = view  && view->hasSelectedText();

    // ── 文件动作 ──────────────────────────────────────────
    // 保存：仅当文件有未保存修改时可用（新建空文件不算修改）
    _saveAction->setEnabled(isDirty);

    // 保存所有：任意视图存在未保存文件
    auto anyDirty = [this]() {
        for (int i = 0; i < _mainDocTab->count(); ++i) {
            Buffer* b = _mainDocTab->bufferAt(i);
            if (b && (b->isDirty() || b->isUntitled())) return true;
        }
        for (int i = 0; i < _subDocTab->count(); ++i) {
            Buffer* b = _subDocTab->bufferAt(i);
            if (b && (b->isDirty() || b->isUntitled())) return true;
        }
        return false;
    };
    _saveAllAction->setEnabled(anyDirty());

    // 关闭 / 关闭所有 / 重新加载
    _closeAction->setEnabled(hasBuf);
    _closeAllAction->setEnabled((_mainDocTab->count() + _subDocTab->count()) > 0);
    _reloadAction->setEnabled(hasBuf && !isUntitled);
    _printAction->setEnabled(hasBuf);
    _printNowAction->setEnabled(hasBuf);

    // ── 编辑动作 ──────────────────────────────────────────
    _undoAction->setEnabled(view && view->isUndoAvailable());
    _redoAction->setEnabled(view && view->isRedoAvailable());
    _cutAction->setEnabled(hasSel);
    _copyAction->setEnabled(hasSel);
    _pasteAction->setEnabled(hasBuf);
    _selectAllAction->setEnabled(hasBuf);
    _deleteLineAction->setEnabled(hasBuf);
    _duplicateLineAction->setEnabled(hasBuf);
    _moveLineUpAction->setEnabled(hasBuf);
    _moveLineDownAction->setEnabled(hasBuf);
    _toggleCommentAction->setEnabled(hasBuf);
    _toUpperCaseAction->setEnabled(hasBuf);
    _toLowerCaseAction->setEnabled(hasBuf);
    for (QAction* action : _documentActions)
        action->setEnabled(hasBuf);

    // ── 搜索动作 ──────────────────────────────────────────
    _findAction->setEnabled(hasBuf);
    _replaceAction->setEnabled(hasBuf);
    _goToLineAction->setEnabled(hasBuf);
    _toggleBookmarkAction->setEnabled(hasBuf);
    _nextBookmarkAction->setEnabled(hasBuf);
    _prevBookmarkAction->setEnabled(hasBuf);
    _clearBookmarksAction->setEnabled(hasBuf);

    // ── 视图动作 ──────────────────────────────────────────
    _zoomInAction->setEnabled(hasBuf);
    _zoomOutAction->setEnabled(hasBuf);
    _zoomRestoreAction->setEnabled(hasBuf);
    _moveToOtherAction->setEnabled(hasBuf);
    _cloneToOtherAction->setEnabled(hasBuf);

    // ── 宏动作（录制中不可再开始） ────────────────────────
    _startRecordAction->setEnabled(hasBuf && !_isRecording);
    // stopRecord/playMacro 由录制逻辑单独控制，不在此处覆盖

    // ── EOL / 编码 ────────────────────────────────────────
    Buffer* currentBuffer = _activeDocTab ? _activeDocTab->currentBuffer() : nullptr;
    if (_wordWrapAction) {
        const bool wrapped = view && currentBuffer &&
            !currentBuffer->isLargeFile() &&
            view->wrapMode() != WrapNone;
        _wordWrapAction->setChecked(wrapped);
    }
    const bool writableBuffer = hasBuf && currentBuffer &&
                                !currentBuffer->isReadOnly();
    _eolWindowsAction->setEnabled(writableBuffer);
    _eolUnixAction->setEnabled(writableBuffer);
    _eolMacAction->setEnabled(writableBuffer);
    if (view) {
        _eolWindowsAction->setChecked(view->eolMode() == EolWindows);
        _eolUnixAction->setChecked(view->eolMode() == EolUnix);
        _eolMacAction->setChecked(view->eolMode() == EolMac);
    }
    if (_encodingMenu) {
        _encodingMenu->setEnabled(writableBuffer);
        for (QAction* action : _encodingMenu->findChildren<QAction*>()) {
            if (!action->property("encodingSelector").toBool())
                continue;
            const QString encoding =
                action->property("encodingName").toString();
            const bool bom = action->property("encodingBom").toBool();
            action->setChecked(currentBuffer &&
                TextFileCodec::normalizedEncoding(currentBuffer->getEncoding()) ==
                    encoding &&
                currentBuffer->hasBom() == bom);
        }
    }
}

// ── 编码操作 ──────────────────────────────────────────────────────────────────

// "以...编码重新解释" — 仅改变保存时使用的编码，不修改内存中的文本内容
void MainWindow::encodeIn(const QString& codec, bool hasBom)
{
    Buffer* buf = _activeDocTab->currentBuffer();
    if (!buf || buf->isReadOnly()) return;
    const QString normalized = TextFileCodec::normalizedEncoding(codec);
    if (buf->getEncoding() == normalized && buf->hasBom() == hasBom)
        return;
    buf->setEncoding(normalized);
    buf->setHasBom(hasBom);
    buf->setUsesEncodingCookie(
        !hasBom && normalized != "UTF-16LE" && normalized != "UTF-16BE");
    buf->setMetadataDirty(true);
    _mainDocTab->updateTabTitle(buf);
    _subDocTab->updateTabTitle(buf);
    updateWindowTitle(buf);
    updateStatusBar();
}

// "转换为..." — 改变编码并标记文件已修改（下次保存时以新编码写入）
void MainWindow::convertTo(const QString& codec, bool hasBom)
{
    Buffer* buf = _activeDocTab->currentBuffer();
    if (!buf || buf->isReadOnly()) return;
    const QString normalized = TextFileCodec::normalizedEncoding(codec);
    if (buf->getEncoding() == normalized && buf->hasBom() == hasBom)
        return;
    buf->setEncoding(normalized);
    buf->setHasBom(hasBom);
    buf->setUsesEncodingCookie(
        !hasBom && normalized != "UTF-16LE" && normalized != "UTF-16BE");
    buf->setMetadataDirty(true);
    _mainDocTab->updateTabTitle(buf);
    _subDocTab->updateTabTitle(buf);
    updateWindowTitle(buf);
    updateStatusBar();
}

void MainWindow::reinterpretAs(const QString& codec)
{
    Buffer* buf = _activeDocTab->currentBuffer();
    ScintillaEditView* activeView = _activeDocTab->editor();
    if (!buf || !activeView)
        return;

    const QString normalized = TextFileCodec::normalizedEncoding(codec);
    if (buf->isUntitled()) {
        encodeIn(normalized, false);
        return;
    }
    if (buf->getEncoding() == normalized && !buf->hasBom()) {
        updateActionStates();
        return;
    }

    if (buf->isDirty()) {
        const auto answer = QMessageBox::question(
            this, tr("Encoding"),
            tr("The file must be saved before it can be reinterpreted. Save it now?"),
            QMessageBox::Save | QMessageBox::Cancel, QMessageBox::Save);
        if (answer != QMessageBox::Save ||
            !doSave(buf, buf->getFullPath())) {
            updateActionStates();
            return;
        }
    }

    const qintptr selectionStart = activeView->SendScintillaNpp(
        SCI_GETSELECTIONSTART, 0, 0);
    const qintptr selectionEnd = activeView->SendScintillaNpp(
        SCI_GETSELECTIONEND, 0, 0);
    const int firstVisibleLine = activeView->firstVisibleLine();
    const bool readOnly = buf->isReadOnly();

    for (DocTabView* tab : {_mainDocTab, _subDocTab})
        if (tab->currentBuffer() == buf)
            tab->editor()->setReadOnly(false);

    QString loadError;
    if (!MainFileManager.loadBufferContent(
            buf, activeView, decodingOptionsForPath(buf->getFullPath()),
            normalized, &loadError)) {
        for (DocTabView* tab : {_mainDocTab, _subDocTab})
            if (tab->currentBuffer() == buf)
                tab->editor()->setReadOnly(readOnly);
        QMessageBox::warning(
            this, tr("Encoding"),
            tr("The file cannot be decoded as %1.\n\n%2")
                .arg(normalized, loadError));
        updateActionStates();
        return;
    }

    const EolMode eolMode =
        toScintillaEol(buf->getEolMode(), activeView->eolMode());
    if (buf->document() != static_cast<qintptr>(activeView->document())) {
        buf->releaseDocument();
        captureBufferDocument(buf, activeView);
    }
    for (DocTabView* tab : {_mainDocTab, _subDocTab}) {
        if (tab->currentBuffer() != buf)
            continue;
        ScintillaEditView* view = tab->editor();
        if (view != activeView)
            view->setDocument(buf->document());
        view->setLargeFileMode(buf->isLargeFile());
        view->setEolMode(eolMode);
        view->setLexerForFile(buf->getFullPath());
        applyPreferencesToView(view);
        view->SendScintilla(SCI_SETSAVEPOINT);
        view->setReadOnly(readOnly);
    }

    const qintptr documentLength = activeView->documentLengthNpp();
    activeView->SendScintillaNpp(
        SCI_SETSEL,
        static_cast<quintptr>(qMin(selectionStart, documentLength)),
        qMin(selectionEnd, documentLength));
    activeView->setFirstVisibleLine(firstVisibleLine);

    buf->setDirty(false);
    _mainDocTab->updateTabTitle(buf);
    _subDocTab->updateTabTitle(buf);
    updateWindowTitle(buf);
    updateStatusBar();
    updateActionStates();
}

void MainWindow::createEncodingMenu()
{
    _encodingMenu = menuBar()->addMenu(tr("&Encoding"));
    _encodingMenu->setObjectName("encodingMenu");

    QActionGroup* encodingGroup = new QActionGroup(_encodingMenu);
    encodingGroup->setExclusive(true);

    // 辅助：构造 "以...编码" 动作
    auto makeEncodeIn = [&](const QString& label,
                             const QString& codec, bool bom)
    {
        QAction* a = _encodingMenu->addAction(label);
        const QString normalized = TextFileCodec::normalizedEncoding(codec);
        a->setCheckable(true);
        a->setActionGroup(encodingGroup);
        a->setProperty("encodingSelector", true);
        a->setProperty("encodingName", normalized);
        a->setProperty("encodingBom", bom);
        connect(a, &QAction::triggered, this,
                [this, codec, bom]() { encodeIn(codec, bom); });
    };

    // 辅助：构造 "转换为..." 动作
    auto makeConvertTo = [&](QMenu* menu,
                              const QString& label,
                              const QString& codec, bool bom)
    {
        QAction* a = menu->addAction(label);
        connect(a, &QAction::triggered, this,
                [this, codec, bom]() { convertTo(codec, bom); });
    };

    // ── 以...编码（Encode in）────────────────────────────────
    makeEncodeIn(tr("Encode in ANSI"),                  "windows-1252", false);
    makeEncodeIn(tr("Encode in UTF-8"),                 "UTF-8",        false);
    makeEncodeIn(tr("Encode in UTF-8 with BOM"),        "UTF-8",        true);
    makeEncodeIn(tr("Encode in UTF-16 BE BOM"),         "UTF-16BE",     true);
    makeEncodeIn(tr("Encode in UTF-16 LE BOM"),         "UTF-16LE",     true);

    _encodingMenu->addSeparator();

    // ── 转换为（Convert to）──────────────────────────────────
    QMenu* convertMenu = _encodingMenu->addMenu(tr("Convert to"));
    makeConvertTo(convertMenu, tr("ANSI"),            "windows-1252", false);
    makeConvertTo(convertMenu, tr("UTF-8"),           "UTF-8",        false);
    makeConvertTo(convertMenu, tr("UTF-8 with BOM"),  "UTF-8",        true);
    makeConvertTo(convertMenu, tr("UTF-16 BE BOM"),   "UTF-16BE",     true);
    makeConvertTo(convertMenu, tr("UTF-16 LE BOM"),   "UTF-16LE",     true);

    _encodingMenu->addSeparator();

    // ── 字符集（Character sets）──────────────────────────────
    QMenu* charsets = _encodingMenu->addMenu(tr("Character sets"));

    auto addCharset = [&](QMenu* sub, const QString& label,
                           const QString& codec)
    {
        QAction* a = sub->addAction(label);
        const QString normalized = TextFileCodec::normalizedEncoding(codec);
        a->setCheckable(true);
        a->setActionGroup(encodingGroup);
        a->setProperty("encodingSelector", true);
        a->setProperty("encodingName", normalized);
        a->setProperty("encodingBom", false);
        connect(a, &QAction::triggered, this,
                [this, codec]() { reinterpretAs(codec); });
    };

    // Arabic
    {
        QMenu* m = charsets->addMenu(tr("Arabic"));
        addCharset(m, "1256 (Windows)",     "windows-1256");
        addCharset(m, "ISO-8859-6",         "ISO-8859-6");
    }
    // Baltic
    {
        QMenu* m = charsets->addMenu(tr("Baltic"));
        addCharset(m, "1257 (Windows)",     "windows-1257");
        addCharset(m, "ISO-8859-4",         "ISO-8859-4");
        addCharset(m, "ISO-8859-13",        "ISO-8859-13");
    }
    // Celtic
    {
        QMenu* m = charsets->addMenu(tr("Celtic"));
        addCharset(m, "ISO-8859-14",        "ISO-8859-14");
    }
    // Central European
    {
        QMenu* m = charsets->addMenu(tr("Central European"));
        addCharset(m, "1250 (Windows)",     "windows-1250");
        addCharset(m, "ISO-8859-2",         "ISO-8859-2");
    }
    // Chinese
    {
        QMenu* m = charsets->addMenu(tr("Chinese"));
        addCharset(m, "GB18030 (Simplified)",  "GB18030");
        addCharset(m, "Big5 (Traditional)",    "Big5");
    }
    // Cyrillic
    {
        QMenu* m = charsets->addMenu(tr("Cyrillic"));
        addCharset(m, "1251 (Windows)",     "windows-1251");
        addCharset(m, "ISO-8859-5",         "ISO-8859-5");
        addCharset(m, "KOI8-R",             "KOI8-R");
        addCharset(m, "KOI8-U",             "KOI8-U");
    }
    // Greek
    {
        QMenu* m = charsets->addMenu(tr("Greek"));
        addCharset(m, "1253 (Windows)",     "windows-1253");
        addCharset(m, "ISO-8859-7",         "ISO-8859-7");
    }
    // Hebrew
    {
        QMenu* m = charsets->addMenu(tr("Hebrew"));
        addCharset(m, "1255 (Windows)",     "windows-1255");
        addCharset(m, "ISO-8859-8",         "ISO-8859-8");
    }
    // Japanese
    {
        QMenu* m = charsets->addMenu(tr("Japanese"));
        addCharset(m, "Shift-JIS",          "Shift-JIS");
        addCharset(m, "EUC-JP",             "EUC-JP");
    }
    // Korean
    {
        QMenu* m = charsets->addMenu(tr("Korean"));
        addCharset(m, "EUC-KR",             "EUC-KR");
        addCharset(m, "949 (Windows)",       "windows-949");
    }
    // Nordic
    {
        QMenu* m = charsets->addMenu(tr("Nordic"));
        addCharset(m, "ISO-8859-10",        "ISO-8859-10");
    }
    // Romanian
    {
        QMenu* m = charsets->addMenu(tr("Romanian"));
        addCharset(m, "ISO-8859-16",        "ISO-8859-16");
    }
    // Thai
    {
        QMenu* m = charsets->addMenu(tr("Thai"));
        addCharset(m, "874 (Windows)",      "windows-874");
        addCharset(m, "TIS-620",            "TIS-620");
    }
    // Turkish
    {
        QMenu* m = charsets->addMenu(tr("Turkish"));
        addCharset(m, "1254 (Windows)",     "windows-1254");
        addCharset(m, "ISO-8859-9",         "ISO-8859-9");
    }
    // Vietnamese
    {
        QMenu* m = charsets->addMenu(tr("Vietnamese"));
        addCharset(m, "1258 (Windows)",     "windows-1258");
    }
    // Western European
    {
        QMenu* m = charsets->addMenu(tr("Western European"));
        addCharset(m, "ISO-8859-1 (Latin-1)",  "ISO-8859-1");
        addCharset(m, "ISO-8859-15 (Latin-9)", "ISO-8859-15");
    }
}

void MainWindow::createToolBars()
{
    applyToolbarIcons();
    _fileToolBar = addToolBar(tr("Standard"));
    _fileToolBar->setObjectName("StandardToolBar");
    _fileToolBar->setIconSize(QSize(16, 16));

    // 文件操作
    _fileToolBar->addAction(_newAction);
    _fileToolBar->addAction(_openAction);
    _fileToolBar->addAction(_saveAction);
    _fileToolBar->addAction(_saveAllAction);
    _fileToolBar->addAction(_closeAction);
    _fileToolBar->addAction(_closeAllAction);
    _fileToolBar->addAction(_printAction);
    _fileToolBar->addSeparator();

    // 编辑操作
    _fileToolBar->addAction(_cutAction);
    _fileToolBar->addAction(_copyAction);
    _fileToolBar->addAction(_pasteAction);
    _fileToolBar->addSeparator();
    _fileToolBar->addAction(_undoAction);
    _fileToolBar->addAction(_redoAction);
    _fileToolBar->addSeparator();

    // 搜索
    _fileToolBar->addAction(_findAction);
    _fileToolBar->addAction(_replaceAction);
    _fileToolBar->addSeparator();

    // 缩放
    _fileToolBar->addAction(_zoomInAction);
    _fileToolBar->addAction(_zoomOutAction);
    _fileToolBar->addSeparator();

    // 视图
    _fileToolBar->addAction(_wordWrapAction);
    if (QAction* showAll = findChild<QAction*>("showAllCharactersAction"))
        _fileToolBar->addAction(showAll);
    _fileToolBar->addAction(_showIndentAction);
    _fileToolBar->addSeparator();

    // 语言定义
    if (QAction* udl = findChild<QAction*>("userDefinedLanguageDialogAction")) {
        if (!udl->icon().isNull())
            _fileToolBar->addAction(udl);
    }

    // 面板
    _fileToolBar->addAction(_docMapAction);
    if (QAction* docList = findChild<QAction*>("documentListAction"))
        _fileToolBar->addAction(docList);
    _fileToolBar->addAction(_funcListAction);
    _fileToolBar->addAction(_fileBrowserAction);
    _fileToolBar->addSeparator();
    _fileToolBar->addAction(_startRecordAction);
    _fileToolBar->addAction(_stopRecordAction);
    _fileToolBar->addAction(_playMacroAction);
    _fileToolBar->addAction(_saveMacroAction);

    _fileToolBar->setVisible(NppParameters::getInstance().getNppGUI()._toolBarShow);
}

void MainWindow::createStatusBar()
{
    // 对应原版状态栏 6 个部分（顺序与原版一致）：
    // DOC_TYPE | DOC_SIZE | CUR_POS | EOF_FORMAT | UNICODE_TYPE | TYPING_MODE
    // 原版状态栏无临时消息区域，不显示 "Ready"

    auto makeSep = [this]() {
        QLabel* sep = new QLabel(" | ", this);
        sep->setStyleSheet("color: #aaa;");
        return sep;
    };

    _docTypeLabel  = new QLabel(tr("Normal Text"), this);
    _docSizeLabel  = new QLabel("length : 0    lines : 1", this);
    _posLabel      = new QLabel("Ln : 1    Col : 1    Pos : 1", this);
    _eolLabel      = new QLabel("Windows (CR LF)", this);
    _encodingLabel = new QLabel("UTF-8", this);
    _insertLabel   = new QLabel("INS", this);

    for (QLabel* l : {_docTypeLabel, _docSizeLabel, _posLabel,
                      _eolLabel, _encodingLabel, _insertLabel})
        l->setContentsMargins(4, 0, 4, 0);

    _docSizeLabel->setMinimumWidth(200);
    _posLabel->setMinimumWidth(210);
    _eolLabel->setMinimumWidth(150);
    _encodingLabel->setMinimumWidth(145);
    _insertLabel->setMinimumWidth(42);

    // DOC_TYPE 在最左侧，自动填充剩余空间（对应原版状态栏第 0 分区）
    statusBar()->addWidget(_docTypeLabel, 1);

    // 其余分区固定在右侧（addPermanentWidget）
    statusBar()->addPermanentWidget(makeSep());
    statusBar()->addPermanentWidget(_docSizeLabel);
    statusBar()->addPermanentWidget(makeSep());
    statusBar()->addPermanentWidget(_posLabel);
    statusBar()->addPermanentWidget(makeSep());
    statusBar()->addPermanentWidget(_eolLabel);
    statusBar()->addPermanentWidget(makeSep());
    statusBar()->addPermanentWidget(_encodingLabel);
    statusBar()->addPermanentWidget(makeSep());
    statusBar()->addPermanentWidget(_insertLabel);
    statusBar()->setVisible(NppParameters::getInstance().getNppGUI()._statusBarShow);
}

// ── applyNativeLang() ────────────────────────────────────────────────────────
// 对应原版 _nativeLangSpeaker.changeMenuLang(_mainMenuHandle)
// 纯 NativeLangSpeaker + XML 机制，无 QTranslator：
//   changeMenuLang() → 立即覆盖菜单/动作文本（热切换）
//   切换语言时重建对话框，使 tr() 内建文本在英文模式下生效

void MainWindow::applyNativeLang()
{
    NppParameters& params = NppParameters::getInstance();
    NativeLangSpeaker& speaker = params.getNativeLangSpeaker();

    // 重建已缓存的对话框，使其下次打开时以 tr() 内建文本重新构造
    // （切换回英文时需要，因为 NativeLangSpeaker 不会恢复 tr() 文本）
    if (_preferenceDlg && !_preferenceDlg->isVisible()) {
        delete _preferenceDlg;
        _preferenceDlg = nullptr;
    }
    if (_findReplaceDlg && !_findReplaceDlg->isVisible()) {
        delete _findReplaceDlg;
        _findReplaceDlg = nullptr;
    }

    if (!speaker.isLoaded()) {
        // 英文模式：恢复首次调用 changeMenuLang() 前保存的原始 tr() 文本
        speaker.restoreOriginalLang(this);
        updateRecentFilesMenu();
        return;
    }

    // 对应原版 changeMenuLang()：按 objectName 覆盖菜单和动作文本
    speaker.changeMenuLang(this);
    updateRecentFilesMenu();

    if (_preferenceDlg && _preferenceDlg->isVisible())
        speaker.changeDlgLang(_preferenceDlg, "Preferences");

    // 如果查找对话框正在显示，立即应用语言翻译
    if (_findReplaceDlg && _findReplaceDlg->isVisible())
        speaker.changeDlgLang(_findReplaceDlg, "FindReplace");
}
