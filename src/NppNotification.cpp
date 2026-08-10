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

// Editor, tab, focus, and lifecycle notifications.

void MainWindow::connectTabView(DocTabView* tab)
{
    connect(tab, &DocTabView::bufferCloseRequested,
            this, &MainWindow::onBufferCloseRequested);
    connect(tab, &DocTabView::currentChanged,
            this, &MainWindow::onCurrentTabChanged);
    connect(tab, &DocTabView::newTabRequested,
            this, [this, tab]() { doNewBuffer(tab); });
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

    const quintptr bufferId = reinterpret_cast<quintptr>(buf);
#ifdef Q_OS_WIN
    if (_win32PluginManager)
        _win32PluginManager->notifyFileBeforeClose(bufferId);
#endif

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
#ifdef Q_OS_WIN
        if (_win32PluginManager)
            _win32PluginManager->notifyFileClosed(bufferId);
#endif
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
#ifdef Q_OS_WIN
    if (buf && _win32PluginManager)
        _win32PluginManager->notifyBufferActivated(
            reinterpret_cast<quintptr>(buf));
#endif
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

void MainWindow::notifyCurrentLanguageChanged()
{
#ifdef Q_OS_WIN
    Buffer* buffer = _activeDocTab ? _activeDocTab->currentBuffer() : nullptr;
    if (buffer && _win32PluginManager) {
        _win32PluginManager->notifyLanguageChanged(
            reinterpret_cast<quintptr>(buffer));
    }
#endif
}
