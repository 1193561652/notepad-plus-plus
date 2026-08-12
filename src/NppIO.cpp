// Qt port implementation split along the original Notepad++ controller boundaries.
// 移植自: v8.4.6:PowerEditor/src/

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

// File, buffer, session, encoding, and persistence operations.

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

void MainWindow::applyFileCommandLineState(
    Buffer* buffer, const CommandLineOptions& options)
{
    if (!buffer)
        return;
    ScintillaEditView* view = activateBufferView(buffer);
    if (!view)
        return;

    const QString previousLanguage = view->lexerLanguage();
    if (!options.userDefinedLanguage.isEmpty()) {
        const UserLangDesc* language =
            NppParameters::getInstance().getUserLangByName(
                options.userDefinedLanguage);
        if (language)
            view->setUserDefinedLanguage(*language);
    } else if (!options.language.isEmpty()) {
        view->setLexerByName(options.language);
    }
    if (view->lexerLanguage() != previousLanguage)
        notifyCurrentLanguageChanged();

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
        setBufferReadOnly(buffer, true);
    }
    if (options.monitorFiles) {
        buffer->setMonitoring(true);
        setBufferReadOnly(buffer, true);
        watchBufferFile(buffer);
    }
    updateLangStatus();
    updateStatusBar();
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

    _openingBuffer = true;
    targetTab->addBuffer(buf);
    _openingBuffer = false;
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

#ifdef Q_OS_WIN
    if (_win32PluginManager)
        _win32PluginManager->notifyFileBeforeLoad();
#endif

    Buffer* buf = MainFileManager.loadBuffer(filePath);
    if (!buf) {
#ifdef Q_OS_WIN
        if (_win32PluginManager)
            _win32PluginManager->notifyFileLoadFailed(0);
#endif
        QMessageBox::warning(this, tr("Open Failed"),
            tr("Cannot open file:\n%1").arg(filePath));
        return false;
    }

#ifdef Q_OS_WIN
    if (_win32PluginManager)
        _win32PluginManager->notifyFileBeforeOpen(
            reinterpret_cast<quintptr>(buf));
#endif

    ScintillaEditView* view = targetTab->editor();
    view->createStandardDocument();
    buf->setView(view);

    QString loadError;
    if (!MainFileManager.loadBufferContent(
            buf, view, decodingOptionsForPath(filePath), forcedEncoding,
            &loadError)) {
#ifdef Q_OS_WIN
        if (_win32PluginManager)
            _win32PluginManager->notifyFileLoadFailed(
                reinterpret_cast<quintptr>(buf));
#endif
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
#ifdef Q_OS_WIN
    if (_win32PluginManager)
        _win32PluginManager->notifyFileOpened(
            reinterpret_cast<quintptr>(buf));
    if (_win32PluginManager)
        _win32PluginManager->notifyBufferActivated(
            reinterpret_cast<quintptr>(buf));
#endif
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
#ifdef Q_OS_WIN
    if (_win32PluginManager)
        _win32PluginManager->notifyFileBeforeSave(
            reinterpret_cast<quintptr>(buf));
#endif
    if (!MainFileManager.saveBuffer(buf, filePath, &errorMessage)) {
        if (!canonical.isEmpty())
            _savingPaths.remove(canonical);
        QMessageBox::warning(this, tr("Save Failed"),
            tr("Cannot save file:\n%1\n\n%2").arg(filePath, errorMessage));
        return false;
    }
#ifdef Q_OS_WIN
    if (_win32PluginManager)
        _win32PluginManager->notifyFileSaved(
            reinterpret_cast<quintptr>(buf));
#endif
    if (!canonical.isEmpty())
        _savingPaths.remove(canonical);

    // 保存成功后删除对应备份文件（与原版一致）
    buf->clearBackupFile();

    setBufferReadOnly(buf, !QFileInfo(filePath).isWritable());
    activeBufferView->SendScintilla(SCI_SETSAVEPOINT);
    const QString previousLanguage = activeBufferView->lexerLanguage();
    activeBufferView->setLexerForFile(filePath);
    for (DocTabView* tab : {_mainDocTab, _subDocTab}) {
        if (tab->currentBuffer() == buf)
            tab->editor()->setReadOnly(buf->isReadOnly());
    }
    if (activeBufferView->lexerLanguage() != previousLanguage)
        notifyCurrentLanguageChanged();

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

bool MainWindow::saveCurrentFileAsForPlugin(
    const QString& path, bool asCopy)
{
    Buffer* buffer = _activeDocTab ? _activeDocTab->currentBuffer() : nullptr;
    if (!buffer || path.isEmpty())
        return false;
    if (!asCopy)
        return doSave(buffer, path);
    if (!activateBufferView(buffer)
        || !MainFileManager.saveBufferCopy(buffer, path)) {
        return false;
    }
    statusBar()->showMessage(tr("Copy saved"), 2000);
    return true;
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
        const quintptr bufferId = reinterpret_cast<quintptr>(buffer);
#ifdef Q_OS_WIN
        if (_win32PluginManager)
            _win32PluginManager->notifyFileBeforeClose(bufferId);
#endif
        if (!buffer->isUntitled())
            rememberClosedFile(buffer->getFullPath());
        removeFromTab(_mainDocTab, buffer);
        removeFromTab(_subDocTab, buffer);
        unwatchBufferFile(buffer);
        buffer->clearBackupFile();
        MainFileManager.closeBuffer(buffer);
#ifdef Q_OS_WIN
        if (_win32PluginManager)
            _win32PluginManager->notifyFileClosed(bufferId);
#endif
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
    setBufferReadOnly(buf, !QFileInfo(path).isWritable());
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
    if (ScintillaEditView* view = activateBufferView(buffer)) {
        const QString previousLanguage = view->lexerLanguage();
        view->setLexerForFile(newPath);
        if (view->lexerLanguage() != previousLanguage)
            notifyCurrentLanguageChanged();
    }
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

bool MainWindow::saveCurrentSessionForPlugin(const QString& path)
{
    if (path.isEmpty())
        return false;
    saveSession();
    return NppParameters::getInstance().writeSession(path,
        NppParameters::getInstance().getSession());
}

bool MainWindow::loadSessionForPlugin(const QString& path)
{
    if (path.isEmpty())
        return false;
    NppParameters& params = NppParameters::getInstance();
    if (!params.loadSession(path))
        return false;
    restoreSession();
    return true;
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
    _dockingManager.showDockableDlg(_fileBrowserPanel, true);
    _fileBrowserAction->setChecked(true);
}

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
        setBufferReadOnly(buf, readOnly);
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
        }
        setBufferReadOnly(buf, sfi._userReadOnly || buf->isReadOnly());
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
