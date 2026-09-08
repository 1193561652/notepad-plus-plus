// MainWindow.cpp - 主窗口实现
// 移植自: v8.4.6:PowerEditor/src/

#include "MainWindow.h"
#include "WinControls/UiResourceLoader.h"
#include "MISC/QtCompat.h"
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
#include "WinControls/PluginsAdmin/PluginAdminDialog.h"
#include "WinControls/PluginsAdmin/PluginAdminModel.h"
#include "MISC/PluginsManager/PluginCatalog.h"
#include "MISC/PluginsManager/PluginEnablementConfig.h"
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
#include <QContextMenuEvent>
#include <QMouseEvent>
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
#ifdef Q_OS_WIN
    if (buf && _win32PluginManager)
        _win32PluginManager->notifyBufferActivated(
            reinterpret_cast<quintptr>(buf));
#endif
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


void MainWindow::setupFileBrowser()
{
    _fileBrowserPanel = new FileBrowserPanel(this);
    DockingData data;
    data.hClient = _fileBrowserPanel;
    data.pszName = tr("Folder as Workspace");
    data.uMask |= DWS_ICONTAB;
    data.hIconTab = NppUiResources::panelIcon(
        NppUiResources::PanelIcon::FileBrowser,
        NppParameters::getInstance().getNppGUI()._darkModeEnabled,
        NppParameters::getInstance().getNppGUI()._toolBarStatus != TB_STANDARD);
    data.objectName = QStringLiteral("FileBrowserDock");
    data.allowedAreas = Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea;
    _fileBrowserDock = _dockingManager.createDockableDlg(
        data, CONT_LEFT, false);

    connect(_fileBrowserPanel, &FileBrowserPanel::fileActivated,
            this, [this](const QString& path) {
                doOpenFile(path, _activeDocTab);
                activateWindow();
            });
    connect(_fileBrowserPanel, &FileBrowserPanel::locateCurrentFileRequested,
            this, [this]() {
        if (Buffer* buffer = _activeDocTab->currentBuffer())
            _fileBrowserPanel->setSelectedPath(buffer->getFullPath());
    });

    connect(_fileBrowserDock, &QDockWidget::visibilityChanged,
            this, [this](bool visible) {
                if (_fileBrowserAction)
                    _fileBrowserAction->setChecked(visible);
            });
}

void MainWindow::toggleFileBrowser()
{
    _dockingManager.toggleDockableDlg(_fileBrowserPanel);
}

void MainWindow::updateFindReplaceView()
{
    if (!_findReplaceDlg) return;
    // 使用当前标签页实际显示的视图（可能是克隆视图）
    ScintillaEditView* view = _activeDocTab->editor();
    if (view)
        _findReplaceDlg->setCurrentView(view);
}


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













































































// ─── 关闭事件 ────────────────────────────────────────────────────────────────

void MainWindow::closeEvent(QCloseEvent* event)
{
#ifdef Q_OS_WIN
    if (_win32PluginManager && !_shutdownNotificationPending) {
        _win32PluginManager->notifyBeforeShutdown();
        _shutdownNotificationPending = true;
    }
    const auto cancelPluginShutdown = [this]() {
        if (_win32PluginManager && _shutdownNotificationPending)
            _win32PluginManager->notifyCancelShutdown();
        _shutdownNotificationPending = false;
    };
#endif
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
#ifdef Q_OS_WIN
                    cancelPluginShutdown();
#endif
                    event->ignore();
                    return;
                }
            }
        }
    }

    gui._isMaximized  = isMaximized();
    gui._windowState  = _dockingManager.saveState();
    gui._dockingLeftWidth =
        _dockingManager.getDockedContSize(CONT_LEFT);
    gui._dockingRightWidth =
        _dockingManager.getDockedContSize(CONT_RIGHT);
    gui._dockingTopHeight =
        _dockingManager.getDockedContSize(CONT_TOP);
    gui._dockingBottomHeight =
        _dockingManager.getDockedContSize(CONT_BOTTOM);
    if (!isMaximized()) {
        gui._appPos = frameGeometry();
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
#ifdef Q_OS_WIN
            cancelPluginShutdown();
#endif
            event->ignore();
            return;
        }
        _pluginUpdaterStarted = true;
    }

    event->accept();
}

// ─── Plugin host operations ─────────────────────────────────────────────────

QString MainWindow::currentFilePath() const
{
    Buffer* buf = _activeDocTab->currentBuffer();
    return (buf && !buf->isUntitled()) ? buf->getFullPath() : QString();
}

int MainWindow::positionForPluginBuffer(quintptr bufferId,
                                        int priorityView) const
{
    const DocTabView* views[] = {_mainDocTab, _subDocTab};
    const int first = priorityView == 1 ? 1 : 0;
    for (int pass = 0; pass < 2; ++pass) {
        const int view = pass == 0 ? first : 1 - first;
        const DocTabView* tab = views[view];
        for (int index = 0; tab && index < tab->count(); ++index) {
            if (reinterpret_cast<quintptr>(tab->bufferAt(index)) == bufferId)
                return (view << 30) | index;
        }
    }
    return -1;
}

QString MainWindow::currentPathForPlugin() const
{
    Buffer* buf = _activeDocTab ? _activeDocTab->currentBuffer() : nullptr;
    return buf ? buf->getFullPath() : QString();
}

bool MainWindow::executePluginMenuCommand(int commandId)
{
    return executeNppCommand(commandId);
}

bool MainWindow::openFileForPlugin(const QString& path)
{
    return !path.isEmpty() && doOpenFile(path, _activeDocTab);
}

bool MainWindow::setCurrentLanguageTypeFromPlugin(int languageType)
{
    ScintillaEditView* view = currentActiveView();
    if (!view)
        return false;
    const QString previousLanguage = view->lexerLanguage();
    switch (languageType) {
        case 0: view->setBuiltinLanguage(QStringLiteral("normal")); break;
        case 1: view->setBuiltinLanguage(QStringLiteral("php")); break;
        case 2: view->setBuiltinLanguage(QStringLiteral("c")); break;
        case 3: view->setBuiltinLanguage(QStringLiteral("cpp")); break;
        case 4: view->setBuiltinLanguage(QStringLiteral("csharp")); break;
        case 6: view->setBuiltinLanguage(QStringLiteral("java")); break;
        case 8: view->setBuiltinLanguage(QStringLiteral("html")); break;
        case 9: view->setBuiltinLanguage(QStringLiteral("xml")); break;
        case 19:
        case 58: view->setBuiltinLanguage(QStringLiteral("javascript")); break;
        case 22: view->setBuiltinLanguage(QStringLiteral("python")); break;
        case 57: view->setBuiltinLanguage(QStringLiteral("json")); break;
        default: return false;
    }
    updateStatusBar();
    if (view->lexerLanguage() != previousLanguage)
        notifyCurrentLanguageChanged();
    return true;
}

quintptr MainWindow::currentBufferIdForPlugin() const
{
    Buffer* buffer = _activeDocTab ? _activeDocTab->currentBuffer() : nullptr;
    return reinterpret_cast<quintptr>(buffer);
}

QString MainWindow::pathForPluginBuffer(quintptr bufferId) const
{
    Buffer* requested = reinterpret_cast<Buffer*>(bufferId);
    for (Buffer* buffer : MainFileManager.buffers()) {
        if (buffer == requested)
            return buffer->getFullPath();
    }
    return QString();
}

int MainWindow::openFileCountForPlugin(int scope) const
{
    if (scope == 1)
        return _mainDocTab ? _mainDocTab->count() : 0;
    if (scope == 2)
        return _subDocTab ? _subDocTab->count() : 0;
    return (_mainDocTab ? _mainDocTab->count() : 0)
        + (_subDocTab ? _subDocTab->count() : 0);
}

int MainWindow::currentDocumentIndexForPlugin(int view) const
{
    const DocTabView* tab = view == SUB_VIEW ? _subDocTab : _mainDocTab;
    return tab ? tab->currentIndex() : -1;
}

quintptr MainWindow::bufferIdAtForPlugin(int view, int index) const
{
    const DocTabView* tab = view == SUB_VIEW ? _subDocTab : _mainDocTab;
    return tab && index >= 0 && index < tab->count()
        ? reinterpret_cast<quintptr>(tab->bufferAt(index)) : 0;
}

int MainWindow::currentLanguageTypeForPlugin() const
{
    const ScintillaEditView* view = currentActiveView();
    if (!view)
        return 0;
    const QString language = view->lexerLanguage().toLower();
    if (language == QStringLiteral("php")) return 1;
    if (language == QStringLiteral("c")) return 2;
    if (language == QStringLiteral("cpp")) return 3;
    if (language == QStringLiteral("csharp")) return 4;
    if (language == QStringLiteral("java")) return 6;
    if (language == QStringLiteral("html")) return 8;
    if (language == QStringLiteral("xml")) return 9;
    if (language == QStringLiteral("javascript")) return 19;
    if (language == QStringLiteral("python")) return 22;
    if (language == QStringLiteral("json")) return 57;
    return 0;
}

bool MainWindow::activateDocumentForPlugin(int view, int index)
{
    DocTabView* tab = view == SUB_VIEW ? _subDocTab : _mainDocTab;
    if (!tab || index < 0 || index >= tab->count())
        return false;
    if (tab == _subDocTab && !tab->isVisible())
        showSubView();
    setActiveTab(tab);
    tab->setCurrentIndex(index);
    return true;
}

int MainWindow::currentLineForPlugin() const
{
    ScintillaEditView* view = currentActiveView();
    return view ? static_cast<int>(view->SendScintillaNpp(
        SCI_LINEFROMPOSITION, view->SendScintillaNpp(SCI_GETCURRENTPOS))) : -1;
}

int MainWindow::bufferEncodingForPlugin(quintptr bufferId) const
{
    Buffer* requested = reinterpret_cast<Buffer*>(bufferId);
    for (Buffer* buffer : MainFileManager.buffers()) {
        if (buffer != requested)
            continue;
        const QString encoding = buffer->getEncoding();
        if (encoding.compare(QStringLiteral("UTF-8"), Qt::CaseInsensitive) == 0)
            return buffer->hasBom() ? 1 : 4;
        if (encoding.compare(QStringLiteral("UTF-16BE"), Qt::CaseInsensitive) == 0)
            return 2;
        if (encoding.compare(QStringLiteral("UTF-16LE"), Qt::CaseInsensitive) == 0)
            return 3;
        return 0;
    }
    return -1;
}

bool MainWindow::setBufferEncodingForPlugin(
    quintptr bufferId, int encoding)
{
    Buffer* requested = reinterpret_cast<Buffer*>(bufferId);
    for (Buffer* buffer : MainFileManager.buffers()) {
        if (buffer != requested)
            continue;
        if (!buffer->isUntitled() || buffer->isDirty())
            return false;
        switch (encoding) {
            case 0:
                buffer->setEncoding(QStringLiteral("windows-1252"));
                buffer->setHasBom(false);
                break;
            case 1:
                buffer->setEncoding(QStringLiteral("UTF-8"));
                buffer->setHasBom(true);
                break;
            case 2:
                buffer->setEncoding(QStringLiteral("UTF-16BE"));
                buffer->setHasBom(true);
                break;
            case 3:
                buffer->setEncoding(QStringLiteral("UTF-16LE"));
                buffer->setHasBom(true);
                break;
            case 4:
                buffer->setEncoding(QStringLiteral("UTF-8"));
                buffer->setHasBom(false);
                break;
            default:
                return false;
        }
        updateStatusBar();
        return true;
    }
    return false;
}

void MainWindow::setPluginStatusBarText(
    int section, const QString& text)
{
    QLabel* labels[] = {_docTypeLabel, _docSizeLabel, _posLabel,
                        _eolLabel, _encodingLabel, _insertLabel};
    if (section >= 0 && section < 6 && labels[section])
        labels[section]->setText(text);
}

bool MainWindow::addPluginToolbarCommand(int commandId)
{
    if (!_fileToolBar)
        return false;
    for (QAction* action : findChildren<QAction*>()) {
        if (action->property("win32PluginCommandId").toInt() != commandId)
            continue;
        if (!_fileToolBar->actions().contains(action))
            _fileToolBar->addAction(action);
        return true;
    }
    return false;
}

bool MainWindow::createDocumentForPlugin(const QByteArray& data)
{
    if (!doNewBuffer(_activeDocTab))
        return false;
    ScintillaEditView* view = currentActiveView();
    if (!view)
        return false;
    view->execute(SCI_CLEARALL);
    if (!data.isEmpty()) {
        view->execute(SCI_ADDTEXT, static_cast<uptr_t>(data.size()),
                      reinterpret_cast<sptr_t>(data.constData()));
    }
    return true;
}

int MainWindow::currentViewIndexForPlugin() const
{
    return _activeDocTab == _subDocTab ? SUB_VIEW : MAIN_VIEW;
}

ScintillaEditView* MainWindow::pluginView(int view) const
{
    if (view != MAIN_VIEW && view != SUB_VIEW)
        return nullptr;
    return view == SUB_VIEW
        ? (_subDocTab ? _subDocTab->editor() : nullptr)
        : (_mainDocTab ? _mainDocTab->editor() : nullptr);
}

bool MainWindow::showPluginBufferInView(quintptr bufferId, int view)
{
    if (view != MAIN_VIEW && view != SUB_VIEW)
        return false;
    Buffer* requested = reinterpret_cast<Buffer*>(bufferId);
    if (!MainFileManager.buffers().contains(requested))
        return false;
    DocTabView* target = view == SUB_VIEW ? _subDocTab : _mainDocTab;
    if (!target)
        return false;
    if (target == _subDocTab && !target->isVisible())
        showSubView();
    if (target->indexOfBuffer(requested) < 0)
        target->addClone(requested, target->editor());
    target->activateBuffer(requested);
    return true;
}


void MainWindow::setBufferReadOnly(Buffer* buffer, bool readOnly)
{
    if (!buffer)
        return;
    const bool changed = buffer->isReadOnly() != readOnly;
    buffer->setReadOnly(readOnly);
    for (DocTabView* tab : {_mainDocTab, _subDocTab}) {
        if (tab && tab->currentBuffer() == buffer)
            tab->editor()->setReadOnly(readOnly);
    }
#ifdef Q_OS_WIN
    if (changed && _win32PluginManager) {
        _win32PluginManager->notifyReadOnlyChanged(
            reinterpret_cast<quintptr>(buffer), readOnly, buffer->isDirty());
    }
#else
    Q_UNUSED(changed)
#endif
}

// ─── 文档地图 ────────────────────────────────────────────────────────────────

void MainWindow::setupDocumentMap()
{
    _docMapPanel = new DocumentMapPanel(this);
    DockingData data;
    data.hClient = _docMapPanel;
    data.pszName = tr("Document Map");
    data.uMask |= DWS_ICONTAB;
    data.hIconTab = NppUiResources::panelIcon(
        NppUiResources::PanelIcon::DocumentMap,
        NppParameters::getInstance().getNppGUI()._darkModeEnabled,
        NppParameters::getInstance().getNppGUI()._toolBarStatus != TB_STANDARD);
    data.objectName = QStringLiteral("DocumentMapDock");
    data.allowedAreas = Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea;
    _docMapDock = _dockingManager.createDockableDlg(
        data, CONT_RIGHT, false);

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
    DockingData data;
    data.hClient = _funcListPanel;
    data.pszName = tr("Function List");
    data.uMask |= DWS_ICONTAB;
    data.hIconTab = NppUiResources::panelIcon(
        NppUiResources::PanelIcon::FunctionList,
        NppParameters::getInstance().getNppGUI()._darkModeEnabled,
        NppParameters::getInstance().getNppGUI()._toolBarStatus != TB_STANDARD);
    data.objectName = QStringLiteral("FunctionListDock");
    data.allowedAreas = Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea;
    _funcListDock = _dockingManager.createDockableDlg(
        data, CONT_RIGHT, false);

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
    DockingData documentListData;
    documentListData.hClient = _documentList;
    documentListData.pszName = tr("Document List");
    documentListData.uMask |= DWS_ICONTAB;
    documentListData.hIconTab = NppUiResources::panelIcon(
        NppUiResources::PanelIcon::DocumentList,
        NppParameters::getInstance().getNppGUI()._darkModeEnabled,
        NppParameters::getInstance().getNppGUI()._toolBarStatus != TB_STANDARD);
    documentListData.objectName = QStringLiteral("DocumentListDock");
    _documentListDock = _dockingManager.createDockableDlg(
        documentListData, CONT_RIGHT, false);

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
                const NppGUI& gui = NppParameters::getInstance().getNppGUI();
                const auto state = buffer->isMonitoring()
                    ? NppUiResources::DocumentState::Monitoring
                    : buffer->isReadOnly()
                        ? NppUiResources::DocumentState::ReadOnly
                        : buffer->isDirty()
                            ? NppUiResources::DocumentState::Modified
                            : NppUiResources::DocumentState::Saved;
                item->setIcon(NppUiResources::documentIcon(
                    state, gui._darkModeEnabled, gui._tabIconSetNumber == 1));
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

        DockingData projectData;
        projectData.hClient = panel;
        projectData.pszName = tr("Project %1").arg(i + 1);
        projectData.uMask |= DWS_ICONTAB;
        projectData.hIconTab = NppUiResources::panelIcon(
            NppUiResources::PanelIcon::Project,
            NppParameters::getInstance().getNppGUI()._darkModeEnabled,
            NppParameters::getInstance().getNppGUI()._toolBarStatus != TB_STANDARD);
        projectData.objectName = i == 0
            ? QStringLiteral("ProjectPanelsDock")
            : QStringLiteral("ProjectPanelsDock%1").arg(i + 1);
        projectData.minimumWidth = 190;
        _projectPanelsDock[i] = _dockingManager.createDockableDlg(
            projectData, CONT_LEFT, false);
    }
    _dockingManager.setDockedContSize(CONT_LEFT, 207);

    _clipboardHistory = new QListWidget(this);
    _clipboardHistory->setWordWrap(true);
    DockingData clipboardData;
    clipboardData.hClient = _clipboardHistory;
    clipboardData.pszName = tr("Clipboard History");
    clipboardData.uMask |= DWS_ICONTAB;
    clipboardData.hIconTab = NppUiResources::panelIcon(
        NppUiResources::PanelIcon::Clipboard,
        NppParameters::getInstance().getNppGUI()._darkModeEnabled,
        NppParameters::getInstance().getNppGUI()._toolBarStatus != TB_STANDARD);
    clipboardData.objectName = QStringLiteral("ClipboardHistoryDock");
    _clipboardDock = _dockingManager.createDockableDlg(
        clipboardData, CONT_BOTTOM, false);
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
    DockingData characterData;
    characterData.hClient = _characterList;
    characterData.pszName = tr("Character Panel");
    characterData.uMask |= DWS_ICONTAB;
    characterData.hIconTab = NppUiResources::panelIcon(
        NppUiResources::PanelIcon::Character,
        NppParameters::getInstance().getNppGUI()._darkModeEnabled,
        NppParameters::getInstance().getNppGUI()._toolBarStatus != TB_STANDARD);
    characterData.objectName = QStringLiteral("CharacterPanelDock");
    _characterDock = _dockingManager.createDockableDlg(
        characterData, CONT_RIGHT, false);
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

    DockingData findResultData;
    findResultData.hClient = _findResultView;
    findResultData.pszName = tr("Find Result");
    findResultData.uMask |= DWS_ICONTAB;
    findResultData.hIconTab = NppUiResources::panelIcon(
        NppUiResources::PanelIcon::FindResult, false, false);
    findResultData.objectName = QStringLiteral("FindResultDock");
    findResultData.allowedAreas =
        Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea;
    _findResultDock = _dockingManager.createDockableDlg(
        findResultData, CONT_BOTTOM, false);

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






// ─── 插件系统 ────────────────────────────────────────────────────────────────




void MainWindow::setupPluginSystem()
{
    NppParameters& parameters = NppParameters::getInstance();
    const QStringList pluginDirectories = parameters.getPluginSearchPaths();
#ifdef Q_OS_WIN
    const QString pluginDir = pluginDirectories.isEmpty()
        ? QDir(parameters.getNppPath()).filePath(QStringLiteral("plugins"))
        : pluginDirectories.constFirst();
#endif
    PluginEnablementConfig enablement(
        PluginEnablementConfig::filePathForConfigDirectory(
            parameters.getUserPath()));
    QString enablementError;
    if (!enablement.load(&enablementError)) {
        qWarning() << "Plugin enablement config could not be loaded:"
                   << enablementError;
    }
    QStringList enabledPluginFolders = enablement.enabledPlugins();
#ifdef ENABLE_PLUGIN_SYSTEM
    _pluginManager = new PluginManager(_pluginHostServices, this);
    QStringList crossPlatformErrors;
    QStringList crossPlatformEnabledFolders = enabledPluginFolders;
    for (const QString& directory : pluginDirectories) {
        _pluginManager->loadPlugins(
            directory, crossPlatformEnabledFolders, &crossPlatformErrors);
        const QStringList loadedFolders = _pluginManager->loadedPluginFolders();
        for (int index = crossPlatformEnabledFolders.size() - 1;
             index >= 0; --index) {
            for (const QString& loadedFolder : loadedFolders) {
                if (loadedFolder.compare(crossPlatformEnabledFolders[index],
                                         Qt::CaseInsensitive) == 0) {
                    crossPlatformEnabledFolders.removeAt(index);
                    break;
                }
            }
        }
    }
    for (const QString& error : crossPlatformErrors)
        qInfo() << "Cross-platform plugin skipped:" << error;
    populateCrossPlatformPluginMenu();

    const QStringList loadedCrossPlatformFolders =
        _pluginManager->loadedPluginFolders();
    QSet<QString> crossPlatformFolders;
    for (const QString& folder : loadedCrossPlatformFolders)
        crossPlatformFolders.insert(folder);
    for (int index = enabledPluginFolders.size() - 1; index >= 0; --index) {
        bool loadedByCrossPlatformAbi = false;
        for (const QString& folder : crossPlatformFolders) {
            if (folder.compare(enabledPluginFolders[index],
                               Qt::CaseInsensitive) == 0) {
                loadedByCrossPlatformAbi = true;
                break;
            }
        }
        if (loadedByCrossPlatformAbi)
            enabledPluginFolders.removeAt(index);
    }
#endif
    connect(_mainDocTab->editor(), &ScintillaEditBase::notify, this,
            [this](Scintilla::NotificationData* notification) {
#ifdef ENABLE_PLUGIN_SYSTEM
        if (_pluginManager && notification) {
            const quint32 code = static_cast<quint32>(notification->nmhdr.code);
            if (code == SCN_MODIFIED || code == SCN_UPDATEUI || code == SCN_ZOOM) {
                const QByteArray text = notification->text && notification->length > 0
                    ? QByteArray(notification->text,
                                 static_cast<int>(notification->length))
                    : QByteArray();
                _pluginManager->notifyPlugins(
                    code == SCN_MODIFIED
                        ? NPP_PLUGIN_NOTIFICATION_TEXT_MODIFIED
                        : (code == SCN_ZOOM ? NPP_PLUGIN_NOTIFICATION_ZOOM : NPP_PLUGIN_NOTIFICATION_UPDATE_UI),
                    currentBufferIdForPlugin(), MAIN_VIEW,
                    notification->position, notification->length,
                    static_cast<quint32>(notification->modificationType),
                    static_cast<quint32>(notification->updated), text, notification->linesAdded);
            }
        }
#endif
#ifdef Q_OS_WIN
        if (_win32PluginManager && notification)
            _win32PluginManager->notifyScintilla(*notification, true);
#endif
    });
    connect(_subDocTab->editor(), &ScintillaEditBase::notify, this,
            [this](Scintilla::NotificationData* notification) {
#ifdef ENABLE_PLUGIN_SYSTEM
        if (_pluginManager && notification) {
            const quint32 code = static_cast<quint32>(notification->nmhdr.code);
            if (code == SCN_MODIFIED || code == SCN_UPDATEUI || code == SCN_ZOOM) {
                const QByteArray text = notification->text && notification->length > 0
                    ? QByteArray(notification->text,
                                 static_cast<int>(notification->length))
                    : QByteArray();
                _pluginManager->notifyPlugins(
                    code == SCN_MODIFIED
                        ? NPP_PLUGIN_NOTIFICATION_TEXT_MODIFIED
                        : (code == SCN_ZOOM ? NPP_PLUGIN_NOTIFICATION_ZOOM : NPP_PLUGIN_NOTIFICATION_UPDATE_UI),
                    currentBufferIdForPlugin(), SUB_VIEW,
                    notification->position, notification->length,
                    static_cast<quint32>(notification->modificationType),
                    static_cast<quint32>(notification->updated), text, notification->linesAdded);
            }
        }
#endif
#ifdef Q_OS_WIN
        if (_win32PluginManager && notification)
            _win32PluginManager->notifyScintilla(*notification, false);
#endif
    });
#ifdef Q_OS_WIN
    QStringList win32PluginErrors;
#ifdef NPP_PLUGIN_REGISTRATION_TEST
    const QString testPluginFilter =
        qEnvironmentVariable("NPP_QT_TEST_WIN32_PLUGIN_FILTER");
    if (!testPluginFilter.isEmpty()) {
        enabledPluginFolders = testPluginFilter.split(
            QLatin1Char('|'), NppQtCompat::SkipEmptyParts);
    }
#endif
    if (!enabledPluginFolders.isEmpty()) {
        _win32PluginManager->loadPlugins(
            pluginDir, enabledPluginFolders,
            &win32PluginErrors);
    }
    setProperty("win32PluginLoadErrors", win32PluginErrors);
    for (const QString& error : win32PluginErrors)
        qWarning() << "Win32 plugin load failed:" << error;
    qInfo() << "Loaded Win32 plugins:"
            << _win32PluginManager->loadedPluginNames();
    populateWin32PluginMenu();
    const QString recoveredPlugin =
        _win32PluginManager->recoveredPluginFolder();
    if (!recoveredPlugin.isEmpty()) {
        QTimer::singleShot(0, this, [this, recoveredPlugin]() {
            QMessageBox* notice = new QMessageBox(
                QMessageBox::Warning, tr("Plugin load recovery"),
                tr("The previous launch stopped while loading plugin '%1'.\n\n"
                   "The plugin was skipped for this launch. Update or uninstall "
                   "it before trying again.").arg(recoveredPlugin),
                QMessageBox::Ok, this);
            notice->setObjectName(
                QStringLiteral("pluginLoadRecoveryNotice"));
            notice->setAttribute(Qt::WA_DeleteOnClose);
            notice->setWindowModality(Qt::NonModal);
            notice->show();
        });
    }
#endif
}

void MainWindow::populateCrossPlatformPluginMenu()
{
    if (!_pluginsMenu || !_pluginManager)
        return;
    QAction* placeholder = findChild<QAction*>(
        QStringLiteral("noPluginsLoadedAction"));
    if (placeholder) {
        _pluginsMenu->removeAction(placeholder);
        delete placeholder;
    }

    QAction* insertionPoint = findChild<QAction*>(
        QStringLiteral("pluginsAdminAction"));
    for (int pluginIndex = 0;
         pluginIndex < _pluginManager->loadedPluginCount(); ++pluginIndex) {
        QMenu* pluginMenu = new QMenu(
            _pluginManager->loadedPluginNames().value(pluginIndex),
            _pluginsMenu);
        pluginMenu->setObjectName(
            QStringLiteral("crossPlatformPluginMenu_%1").arg(pluginIndex));
        for (int functionIndex = 0;
             functionIndex < _pluginManager->loadedPluginFunctionCount(
                 pluginIndex); ++functionIndex) {
            if (_pluginManager->isLoadedPluginFunctionSeparator(pluginIndex, functionIndex)) {
                pluginMenu->addSeparator();
                continue;
            }
            QAction* action = pluginMenu->addAction(
                _pluginManager->loadedPluginFunctionName(
                    pluginIndex, functionIndex));
            action->setCheckable(
                _pluginManager->isLoadedPluginFunctionCheckable(
                    pluginIndex, functionIndex));
            action->setChecked(
                _pluginManager->isLoadedPluginFunctionInitiallyChecked(
                    pluginIndex, functionIndex));
            action->setShortcut(
                _pluginManager->loadedPluginFunctionShortcut(
                    pluginIndex, functionIndex));
            const auto syncState = [this, action, pluginIndex, functionIndex]() {
                action->setCheckable(_pluginManager->isLoadedPluginFunctionCheckable(pluginIndex, functionIndex));
                action->setChecked(_pluginManager->isLoadedPluginFunctionInitiallyChecked(pluginIndex, functionIndex));
            };
            connect(pluginMenu, &QMenu::aboutToShow, action, syncState);
            connect(action, &QAction::triggered, this,
                    [this, pluginIndex, functionIndex, syncState]() {
                QString error;
                if (!_pluginManager->executePluginCommand(
                        pluginIndex, functionIndex, &error)
                    && !error.isEmpty()) {
                    QMessageBox::warning(
                        this, tr("Plugins"), error);
                }
                syncState();
            });
        }
        _pluginsMenu->insertMenu(insertionPoint, pluginMenu);
    }
}

#ifdef Q_OS_WIN
void MainWindow::populateWin32PluginMenu()
{
    if (!_pluginsMenu || !_win32PluginManager)
        return;

    QAction* placeholder = findChild<QAction*>(
        QStringLiteral("noPluginsLoadedAction"));
    if (placeholder) {
        _pluginsMenu->removeAction(placeholder);
        delete placeholder;
    }

    QAction* const insertionPoint = findChild<QAction*>(
        QStringLiteral("pluginsAdminAction"));
    for (int pluginIndex = 0;
         pluginIndex < _win32PluginManager->loadedPluginCount();
         ++pluginIndex) {
        const QString pluginName =
            _win32PluginManager->loadedPluginNames().value(pluginIndex);
        if (pluginName.isEmpty())
            continue;

        QMenu* pluginMenu = new QMenu(pluginName, _pluginsMenu);
        pluginMenu->setObjectName(
            QStringLiteral("win32PluginMenu_%1").arg(pluginIndex));
        const int functionCount =
            _win32PluginManager->loadedPluginFunctionCount(pluginIndex);
        for (int functionIndex = 0; functionIndex < functionCount;
             ++functionIndex) {
            if (_win32PluginManager->isLoadedPluginFunctionSeparator(
                    pluginIndex, functionIndex)) {
                pluginMenu->addSeparator();
                continue;
            }

            const QString functionName =
                _win32PluginManager->loadedPluginFunctionName(
                    pluginIndex, functionIndex);
            if (functionName.isEmpty())
                continue;
            QAction* action = pluginMenu->addAction(functionName);
            action->setObjectName(QStringLiteral("win32PluginAction_%1_%2")
                                      .arg(pluginIndex)
                                      .arg(functionIndex));
            action->setProperty(
                "win32PluginCommandId",
                _win32PluginManager->loadedPluginFunctionCommandId(
                    pluginIndex, functionIndex));
            const bool initiallyChecked =
                _win32PluginManager->isLoadedPluginFunctionInitiallyChecked(
                    pluginIndex, functionIndex);
            if (initiallyChecked) {
                action->setCheckable(true);
                action->setChecked(true);
            }
            const QKeySequence shortcut =
                _win32PluginManager->loadedPluginFunctionShortcut(
                    pluginIndex, functionIndex);
            if (!shortcut.isEmpty())
                action->setShortcut(shortcut);
            connect(action, &QAction::triggered, this,
                    [this, action, pluginIndex, functionIndex]() {
                const bool checkedBefore = _win32PluginManager
                    ->isLoadedPluginFunctionInitiallyChecked(
                        pluginIndex, functionIndex);
                QString error;
                if (!_win32PluginManager->executePluginCommand(
                        pluginIndex, functionIndex, &error)) {
                    QMessageBox::warning(this, tr("Plugin command failed"),
                                         error);
                    return;
                }
                const bool checked = _win32PluginManager
                    ->isLoadedPluginFunctionInitiallyChecked(
                        pluginIndex, functionIndex);
                if (checked != checkedBefore) {
                    action->setCheckable(true);
                    action->setChecked(checked);
                }
            });
        }
        _pluginsMenu->insertMenu(insertionPoint, pluginMenu);
    }
}
#endif

void MainWindow::showPluginAdmin()
{
    if (_pluginAdminDlg) {
        _pluginAdminDlg->show();
        _pluginAdminDlg->raise();
        _pluginAdminDlg->activateWindow();
        return;
    }

    const QString pluginRoot =
        NppParameters::getInstance().getWritablePluginPath();
    QDir().mkpath(pluginRoot);
    PluginCatalog catalog = PluginCatalog::embedded();
    if (!catalog.isValid()) {
        QMessageBox::warning(
            this, tr("Plugins Admin"),
            tr("The built-in plugin list is invalid."));
        return;
    }

    _pluginAdminModel = new PluginAdminModel(
        pluginRoot, catalog,
        PluginVersion(QCoreApplication::applicationVersion()),
        PluginEnablementConfig::filePathForConfigDirectory(
            NppParameters::getInstance().getUserPath()));
    _pluginAdminDlg = new PluginAdminDialog(_pluginAdminModel, this);
    _pluginAdminDlg->setAttribute(Qt::WA_DeleteOnClose);
    _pluginAdminDlg->setWindowModality(Qt::NonModal);
    _pluginAdminDlg->setModal(false);
    _pluginAdminDlg->applyLocalization(
        NppParameters::getInstance().getNativeLangSpeaker());
    connect(_pluginAdminDlg, &QDialog::accepted, this, [this]() {
        if (_pluginAdminDlg && schedulePluginOperations(
                _pluginAdminDlg->selectedOperations()))
            QTimer::singleShot(0, this, &QWidget::close);
    });
    connect(_pluginAdminDlg, &QObject::destroyed, this, [this]() {
        _pluginAdminDlg = nullptr;
        delete _pluginAdminModel;
        _pluginAdminModel = nullptr;
    });
    _pluginAdminDlg->show();
}

bool MainWindow::schedulePluginOperations(
    const QVector<PluginOperation>& operations)
{
    if (operations.isEmpty())
        return false;

    PluginUpdatePlan plan;
    plan.applicationPath = QCoreApplication::applicationFilePath();
    plan.pluginRoot = NppParameters::getInstance().getWritablePluginPath();
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

    const QString applicationDirectory =
        NppParameters::getInstance().getNppPath();
    const QString updaterPath =
        NppParameters::getInstance().getPluginUpdaterPath();
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
    applyToolbarIcons();
    applyPanelIcons();

    const ScintillaViewParams& svp =
        NppParameters::getInstance().getSVP();
    _mainDocTab->setEditorBorderWidth(svp._borderWidth);
    _subDocTab->setEditorBorderWidth(svp._borderWidth);
    _mainDocTab->refreshTabIcons();
    _subDocTab->refreshTabIcons();

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

void MainWindow::applyPanelIcons()
{
    const NppGUI& gui = NppParameters::getInstance().getNppGUI();
    const bool alternate = gui._toolBarStatus != TB_STANDARD;
    const auto setIcon = [this, &gui, alternate](
            QWidget* client, NppUiResources::PanelIcon panel) {
        if (QDockWidget* dock = _dockingManager.dockForClient(client))
            dock->setWindowIcon(NppUiResources::panelIcon(
                panel, gui._darkModeEnabled, alternate));
    };
    setIcon(_docMapPanel, NppUiResources::PanelIcon::DocumentMap);
    setIcon(_documentList, NppUiResources::PanelIcon::DocumentList);
    setIcon(_funcListPanel, NppUiResources::PanelIcon::FunctionList);
    setIcon(_fileBrowserPanel, NppUiResources::PanelIcon::FileBrowser);
    for (ProjectPanel* panel : _projectPanels)
        setIcon(panel, NppUiResources::PanelIcon::Project);
    setIcon(_clipboardHistory, NppUiResources::PanelIcon::Clipboard);
    setIcon(_characterList, NppUiResources::PanelIcon::Character);
    setIcon(_findResultView, NppUiResources::PanelIcon::FindResult);

    if (_fileBrowserPanel)
        _fileBrowserPanel->refreshResources(gui._darkModeEnabled);
    if (_funcListPanel)
        _funcListPanel->refreshResources(gui._darkModeEnabled);
    for (ProjectPanel* panel : _projectPanels) {
        if (panel)
            panel->refreshResources();
    }
}

void MainWindow::applyDarkMode()
{
    const bool dark = NppParameters::getInstance().getNppGUI()._darkModeEnabled;
    const bool changed = _hasAppliedDarkMode && _appliedDarkMode != dark;
    _hasAppliedDarkMode = true;
    _appliedDarkMode = dark;
    const auto notifyDarkModeChanged = [this, changed]() {
#ifdef ENABLE_PLUGIN_SYSTEM
        if (changed && _pluginManager)
            _pluginManager->notifyPlugins(
                NPP_PLUGIN_NOTIFICATION_DARK_MODE_CHANGED);
#endif
#ifdef Q_OS_WIN
        if (changed && _win32PluginManager)
            _win32PluginManager->notifyDarkModeChanged();
#elif !defined(ENABLE_PLUGIN_SYSTEM)
        Q_UNUSED(changed)
#endif
    };
    if (!dark) {
        qApp->setPalette(qApp->style()->standardPalette());
        qApp->setStyleSheet(QString());
        notifyDarkModeChanged();
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
    notifyDarkModeChanged();
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

    _docTypeLabel->setObjectName(QStringLiteral("statusDocType"));
    _docSizeLabel->setObjectName(QStringLiteral("statusDocSize"));
    _posLabel->setObjectName(QStringLiteral("statusCursorPosition"));
    _eolLabel->setObjectName(QStringLiteral("statusEolFormat"));
    _encodingLabel->setObjectName(QStringLiteral("statusEncoding"));
    _insertLabel->setObjectName(QStringLiteral("statusTypingMode"));

    for (QLabel* l : {_docTypeLabel, _docSizeLabel, _posLabel,
                      _eolLabel, _encodingLabel, _insertLabel}) {
        l->setContentsMargins(4, 0, 4, 0);
        l->installEventFilter(this);
    }

    for (QLabel* interactive : {_docTypeLabel, _docSizeLabel, _posLabel,
                                _eolLabel, _encodingLabel, _insertLabel}) {
        interactive->setCursor(Qt::PointingHandCursor);
    }
    _docTypeLabel->setToolTip(tr("Double-click or right-click to select a language"));
    _docSizeLabel->setToolTip(tr("Double-click to show document summary"));
    _posLabel->setToolTip(tr("Double-click to go to a line"));
    _eolLabel->setToolTip(tr("Double-click or right-click to convert line endings"));
    _encodingLabel->setToolTip(tr("Double-click or right-click to select encoding"));
    _insertLabel->setToolTip(tr("Click to switch between insert and overwrite mode"));

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

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    QLabel* label = qobject_cast<QLabel*>(watched);
    const bool isStatusLabel = label
        && (label == _docTypeLabel || label == _docSizeLabel
            || label == _posLabel || label == _eolLabel
            || label == _encodingLabel || label == _insertLabel);
    if (!isStatusLabel)
        return QMainWindow::eventFilter(watched, event);

    auto popupStatusMenu = [this, label](const QPoint& localPosition) {
        QMenu* menu = nullptr;
        if (label == _docTypeLabel) {
            menu = _languageMenu;
        } else if (label == _eolLabel && _editMenu) {
            menu = _editMenu->findChild<QMenu*>(
                QStringLiteral("eolEditMenu"));
        } else if (label == _encodingLabel) {
            menu = _encodingMenu;
        }
        if (menu) {
            updateActionStates();
            menu->popup(label->mapToGlobal(localPosition));
            return true;
        }
        return false;
    };

    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (label == _insertLabel
            && mouseEvent->button() == Qt::LeftButton) {
            if (ScintillaEditView* view = currentActiveView()) {
                const bool overtype = view->SendScintilla(SCI_GETOVERTYPE) != 0;
                view->SendScintilla(SCI_SETOVERTYPE, !overtype);
                _insertLabel->setText(overtype ? QStringLiteral("INS")
                                               : QStringLiteral("OVR"));
                view->setFocus(Qt::MouseFocusReason);
            }
            return true;
        }
    } else if (event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() != Qt::LeftButton)
            return false;
        if (label == _posLabel) {
            goToLine();
            return true;
        }
        if (label == _docSizeLabel) {
            if (QAction* summary = findChild<QAction*>(
                    QStringLiteral("documentSummaryAction"))) {
                summary->trigger();
            }
            return true;
        }
        return popupStatusMenu(mouseEvent->pos());
    } else if (event->type() == QEvent::ContextMenu) {
        QContextMenuEvent* contextEvent =
            static_cast<QContextMenuEvent*>(event);
        return popupStatusMenu(contextEvent->pos());
    }

    return QMainWindow::eventFilter(watched, event);
}
