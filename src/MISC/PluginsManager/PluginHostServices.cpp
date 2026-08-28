#include "MISC/PluginsManager/PluginHostServices.h"

#include <QCoreApplication>
#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QDir>
#include <QSysInfo>

#include "MainWindow.h"
#include "Parameters.h"
#include "CrossPlatformPluginSystem/PluginInterface.h"
#include "ScintillaComponent/ScintillaEditView.h"

namespace {

int systemType()
{
#if defined(Q_OS_WIN)
    return NPP_PLUGIN_SYSTEM_WINDOWS;
#elif defined(Q_OS_LINUX)
    return NPP_PLUGIN_SYSTEM_LINUX;
#elif defined(Q_OS_MAC)
    return NPP_PLUGIN_SYSTEM_MACOS;
#else
    return NPP_PLUGIN_SYSTEM_UNKNOWN;
#endif
}

int cpuArchitecture()
{
    const QString architecture = QSysInfo::currentCpuArchitecture().toLower();
    if (architecture == QStringLiteral("x86_64")
        || architecture == QStringLiteral("amd64"))
        return NPP_PLUGIN_CPU_X64;
    if (architecture == QStringLiteral("i386")
        || architecture == QStringLiteral("i686")
        || architecture == QStringLiteral("x86"))
        return NPP_PLUGIN_CPU_X86;
    if (architecture == QStringLiteral("arm64")
        || architecture == QStringLiteral("aarch64"))
        return NPP_PLUGIN_CPU_ARM64;
    return NPP_PLUGIN_CPU_UNKNOWN;
}

} // namespace

MainWindowPluginHostServices::MainWindowPluginHostServices(
    MainWindow* mainWindow)
    : _mainWindow(mainWindow)
{
}

PluginHostEnvironment MainWindowPluginHostServices::environment() const
{
    PluginHostEnvironment value;
    value.systemType = systemType();
    value.cpuArchitecture = cpuArchitecture();
    value.systemName = QSysInfo::prettyProductName();
    value.systemVersion = QSysInfo::productVersion();
    if (value.systemName.isEmpty())
        value.systemName = QSysInfo::kernelType();
    if (value.systemVersion.isEmpty())
        value.systemVersion = QSysInfo::kernelVersion();
    value.applicationName = QCoreApplication::applicationName();
    value.applicationVersion = QCoreApplication::applicationVersion();
    return value;
}

QString MainWindowPluginHostServices::pluginHomePath() const
{
    return QDir(NppParameters::getInstance().getNppPath())
        .filePath(QStringLiteral("plugins"));
}

QString MainWindowPluginHostServices::pluginConfigPath() const
{
    const QString path = QDir(NppParameters::getInstance().getUserPath())
        .filePath(QStringLiteral("plugins/Config"));
    QDir().mkpath(path);
    return path;
}

QString MainWindowPluginHostServices::currentFilePath() const
{
    return _mainWindow ? _mainWindow->currentPathForPlugin() : QString();
}

bool MainWindowPluginHostServices::openFile(const QString& path)
{
    return _mainWindow && _mainWindow->openFileForPlugin(path);
}

bool MainWindowPluginHostServices::isDarkModeEnabled() const
{
    return NppParameters::getInstance().getNppGUI()._darkModeEnabled;
}

QMainWindow* MainWindowPluginHostServices::mainWindow() const
{
    return _mainWindow;
}

ScintillaEditView* MainWindowPluginHostServices::currentView() const
{
    return _mainWindow ? _mainWindow->currentView() : nullptr;
}

bool MainWindowPluginHostServices::executeMenuCommand(int commandId)
{
    return _mainWindow && _mainWindow->executePluginMenuCommand(commandId);
}

bool MainWindowPluginHostServices::saveCurrentFileAs(
    const QString& path, bool asCopy)
{
    return _mainWindow
        && _mainWindow->saveCurrentFileAsForPlugin(path, asCopy);
}

bool MainWindowPluginHostServices::saveCurrentSession(const QString& path)
{
    return _mainWindow && _mainWindow->saveCurrentSessionForPlugin(path);
}

bool MainWindowPluginHostServices::loadSession(const QString& path)
{
    return _mainWindow && _mainWindow->loadSessionForPlugin(path);
}

bool MainWindowPluginHostServices::setCurrentLanguageType(int languageType)
{
    return _mainWindow
        && _mainWindow->setCurrentLanguageTypeFromPlugin(languageType);
}

quintptr MainWindowPluginHostServices::currentBufferId() const
{
    return _mainWindow ? _mainWindow->currentBufferIdForPlugin() : 0;
}

QString MainWindowPluginHostServices::pathForBuffer(quintptr bufferId) const
{
    return _mainWindow
        ? _mainWindow->pathForPluginBuffer(bufferId) : QString();
}

int MainWindowPluginHostServices::positionForBuffer(
    quintptr bufferId, int priorityView) const
{
    return _mainWindow
        ? _mainWindow->positionForPluginBuffer(bufferId, priorityView) : -1;
}

int MainWindowPluginHostServices::openFileCount(int scope) const
{
    return _mainWindow ? _mainWindow->openFileCountForPlugin(scope) : 0;
}

int MainWindowPluginHostServices::currentDocumentIndex(int view) const
{
    return _mainWindow
        ? _mainWindow->currentDocumentIndexForPlugin(view) : -1;
}

quintptr MainWindowPluginHostServices::bufferIdAt(int view, int index) const
{
    return _mainWindow ? _mainWindow->bufferIdAtForPlugin(view, index) : 0;
}

int MainWindowPluginHostServices::currentLanguageType() const
{
    return _mainWindow ? _mainWindow->currentLanguageTypeForPlugin() : 0;
}

bool MainWindowPluginHostServices::activateDocument(int view, int index)
{
    return _mainWindow
        && _mainWindow->activateDocumentForPlugin(view, index);
}

int MainWindowPluginHostServices::currentLine() const
{
    return _mainWindow ? _mainWindow->currentLineForPlugin() : -1;
}

int MainWindowPluginHostServices::bufferEncoding(quintptr bufferId) const
{
    return _mainWindow ? _mainWindow->bufferEncodingForPlugin(bufferId) : -1;
}

bool MainWindowPluginHostServices::setBufferEncoding(
    quintptr bufferId, int encoding)
{
    return _mainWindow
        && _mainWindow->setBufferEncodingForPlugin(bufferId, encoding);
}

void MainWindowPluginHostServices::setStatusBarText(
    int section, const QString& text)
{
    if (_mainWindow)
        _mainWindow->setPluginStatusBarText(section, text);
}

bool MainWindowPluginHostServices::addToolbarCommand(int commandId)
{
    return _mainWindow && _mainWindow->addPluginToolbarCommand(commandId);
}

QByteArray MainWindowPluginHostServices::currentDocumentBytes() const
{
    ScintillaEditView* view = currentView();
    if (!view)
        return QByteArray();
    const sptr_t length = view->execute(SCI_GETLENGTH);
    QByteArray data(static_cast<int>(length) + 1, '\0');
    view->execute(SCI_GETTEXT, length + 1,
                  reinterpret_cast<sptr_t>(data.data()));
    data.truncate(static_cast<int>(length));
    return data;
}

bool MainWindowPluginHostServices::replaceCurrentDocument(const QByteArray& data)
{
    ScintillaEditView* view = currentView();
    if (!view || view->isReadOnly())
        return false;
    view->execute(SCI_BEGINUNDOACTION);
    view->execute(SCI_CLEARALL);
    if (!data.isEmpty()) {
        view->execute(SCI_ADDTEXT, static_cast<uptr_t>(data.size()),
                      reinterpret_cast<sptr_t>(data.constData()));
    }
    view->execute(SCI_ENDUNDOACTION);
    return true;
}

QByteArray MainWindowPluginHostServices::currentSelectionBytes(
    qint64* start, qint64* end) const
{
    ScintillaEditView* view = currentView();
    if (!view)
        return QByteArray();
    const sptr_t begin = view->execute(SCI_GETSELECTIONSTART);
    const sptr_t finish = view->execute(SCI_GETSELECTIONEND);
    if (start)
        *start = begin;
    if (end)
        *end = finish;
    QByteArray data(static_cast<int>(qMax<sptr_t>(0, finish - begin)) + 1, '\0');
    Sci_TextRange range{};
    range.chrg.cpMin = begin;
    range.chrg.cpMax = finish;
    range.lpstrText = data.data();
    view->execute(SCI_GETTEXTRANGE, 0, reinterpret_cast<sptr_t>(&range));
    data.truncate(static_cast<int>(qMax<sptr_t>(0, finish - begin)));
    return data;
}

bool MainWindowPluginHostServices::replaceCurrentSelection(const QByteArray& data)
{
    ScintillaEditView* view = currentView();
    if (!view || view->isReadOnly())
        return false;
    const sptr_t start = view->execute(SCI_GETSELECTIONSTART);
    const sptr_t end = view->execute(SCI_GETSELECTIONEND);
    view->execute(SCI_SETTARGETRANGE, static_cast<uptr_t>(start), end);
    view->execute(SCI_REPLACETARGET, static_cast<uptr_t>(data.size()),
                  reinterpret_cast<sptr_t>(data.constData()));
    view->execute(SCI_SETSEL, static_cast<uptr_t>(start),
                  start + static_cast<sptr_t>(data.size()));
    return true;
}

bool MainWindowPluginHostServices::setCurrentSelection(qint64 start, qint64 end)
{
    ScintillaEditView* view = currentView();
    if (!view || start < 0 || end < 0)
        return false;
    view->execute(SCI_SETSEL, static_cast<uptr_t>(start), static_cast<sptr_t>(end));
    return true;
}

bool MainWindowPluginHostServices::createDocument(const QByteArray& data)
{
    return _mainWindow && _mainWindow->createDocumentForPlugin(data);
}

int MainWindowPluginHostServices::currentViewIndex() const
{
    return _mainWindow ? _mainWindow->currentViewIndexForPlugin() : -1;
}

QString MainWindowPluginHostServices::clipboardText() const
{
    return QApplication::clipboard()->text();
}

bool MainWindowPluginHostServices::setClipboardText(const QString& text)
{
    QApplication::clipboard()->setText(text);
    return true;
}

QByteArray MainWindowPluginHostServices::viewDocumentBytes(int view) const
{
    ScintillaEditView* editor = _mainWindow ? _mainWindow->pluginView(view) : nullptr;
    if (!editor) return {};
    const sptr_t length = editor->execute(SCI_GETLENGTH);
    QByteArray data(static_cast<int>(length) + 1, '\0');
    editor->execute(SCI_GETTEXT, length + 1, reinterpret_cast<sptr_t>(data.data()));
    data.truncate(static_cast<int>(length));
    return data;
}

bool MainWindowPluginHostServices::showBufferInView(quintptr bufferId, int view)
{
    return _mainWindow && _mainWindow->showPluginBufferInView(bufferId, view);
}

void MainWindowPluginHostServices::clearCompareMarks(int view)
{
    ScintillaEditView* editor = _mainWindow ? _mainWindow->pluginView(view) : nullptr;
    if (!editor) return;
    for (int marker = 10; marker <= 13; ++marker)
        editor->execute(SCI_MARKERDELETEALL, marker);
}

bool MainWindowPluginHostServices::addCompareMark(int view, qint64 line, quint32 kind)
{
    ScintillaEditView* editor = _mainWindow ? _mainWindow->pluginView(view) : nullptr;
    if (!editor || line < 0 || line >= editor->lines() || kind > 3)
        return false;
    const int marker = 10 + static_cast<int>(kind);
    const QColor colors[] = {QColor(210,245,218), QColor(255,220,220),
                             QColor(255,242,188), QColor(215,228,255)};
    editor->execute(SCI_MARKERDEFINE, marker, SC_MARK_BACKGROUND);
    editor->setMarkerBackgroundColor(colors[kind], marker);
    editor->execute(SCI_MARKERADD, static_cast<uptr_t>(line), marker);
    return true;
}

qint64 MainWindowPluginHostServices::firstVisibleLine(int view) const
{
    ScintillaEditView* editor = _mainWindow ? _mainWindow->pluginView(view) : nullptr;
    return editor ? editor->execute(SCI_GETFIRSTVISIBLELINE) : -1;
}

bool MainWindowPluginHostServices::setFirstVisibleLine(int view, qint64 line)
{
    ScintillaEditView* editor = _mainWindow ? _mainWindow->pluginView(view) : nullptr;
    if (!editor || line < 0) return false;
    editor->execute(SCI_SETFIRSTVISIBLELINE, static_cast<uptr_t>(line));
    return true;
}

bool MainWindowPluginHostServices::gotoLine(int view, qint64 line)
{
    ScintillaEditView* editor = _mainWindow ? _mainWindow->pluginView(view) : nullptr;
    if (!editor || line < 0 || line >= editor->lines()) return false;
    editor->execute(SCI_GOTOLINE, static_cast<uptr_t>(line));
    editor->setFocus();
    return true;
}

qintptr MainWindowPluginHostServices::sendScintilla(
    int view, quint32 message, quintptr wParam, qintptr lParam)
{
    ScintillaEditView* editor = _mainWindow ? _mainWindow->pluginView(view) : nullptr;
    return editor ? editor->execute(message, static_cast<uptr_t>(wParam),
                                    static_cast<sptr_t>(lParam)) : 0;
}

bool MainWindowPluginHostServices::saveCurrentFile()
{
    return _mainWindow && _mainWindow->saveCurrentFileForPlugin();
}
