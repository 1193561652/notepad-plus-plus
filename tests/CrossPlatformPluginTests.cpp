#include <QCoreApplication>
#include <QFileInfo>
#include <QStringList>

#include <cstdlib>
#include <iostream>

#include "MISC/PluginsManager/PluginHostServices.h"
#include "MISC/PluginsManager/PluginManager.h"

namespace {

class TestHostServices final : public PluginHostServices
{
public:
    PluginHostEnvironment environment() const override
    {
        PluginHostEnvironment value;
#if defined(Q_OS_WIN)
        value.systemType = NPP_PLUGIN_SYSTEM_WINDOWS;
#elif defined(Q_OS_MAC)
        value.systemType = NPP_PLUGIN_SYSTEM_MACOS;
#else
        value.systemType = NPP_PLUGIN_SYSTEM_LINUX;
#endif
        value.cpuArchitecture = NPP_PLUGIN_CPU_X64;
        value.systemName = QStringLiteral("Test System");
        value.systemVersion = QStringLiteral("1.0");
        value.applicationName = QStringLiteral("Notepad++");
        value.applicationVersion = QStringLiteral("8.4.6");
        return value;
    }

    QString pluginHomePath() const override
        { return QStringLiteral("/plugins"); }
    QString pluginConfigPath() const override
        { return QStringLiteral("/config"); }
    QString currentFilePath() const override
        { return QStringLiteral("current.txt"); }
    bool openFile(const QString& path) override
        { openedPath = path; return true; }
    bool isDarkModeEnabled() const override { return false; }
    QMainWindow* mainWindow() const override { return nullptr; }
    ScintillaEditView* currentView() const override { return nullptr; }
    bool executeMenuCommand(int commandId) override
        { executedCommand=commandId;return true; }
    bool saveCurrentFileAs(const QString&, bool) override { return false; }
    bool saveCurrentSession(const QString&) override { return false; }
    bool loadSession(const QString&) override { return false; }
    bool setCurrentLanguageType(int) override { return false; }
    quintptr currentBufferId() const override { return 0; }
    QString pathForBuffer(quintptr bufferId) const override
        { return bufferId == 42 ? QStringLiteral("buffer.txt") : QString(); }
    int positionForBuffer(quintptr, int) const override { return -1; }
    int openFileCount(int) const override { return 0; }
    int currentDocumentIndex(int) const override { return -1; }
    bool activateDocument(int, int) override { return false; }
    int currentLine() const override { return -1; }
    int bufferEncoding(quintptr) const override { return -1; }
    bool setBufferEncoding(quintptr, int) override { return false; }
    void setStatusBarText(int, const QString&) override {}
    bool addToolbarCommand(int) override { return false; }
    QByteArray currentDocumentBytes() const override
        { return document; }
    bool replaceCurrentDocument(const QByteArray& data) override
        { document = data; return true; }
    QByteArray currentSelectionBytes(qint64* start, qint64* end) const override
    {
        if (start) *start = 1;
        if (end) *end = 4;
        return document.mid(1, 3);
    }
    bool replaceCurrentSelection(const QByteArray& data) override
        { selectionReplacement = data; return true; }
    bool setCurrentSelection(qint64 start, qint64 end) override
        { selectionStart = start; selectionEnd = end; return true; }
    bool createDocument(const QByteArray& data) override
        { createdDocument = data; return true; }
    int currentViewIndex() const override { return 1; }
    QString clipboardText() const override { return QStringLiteral("clip"); }
    bool setClipboardText(const QString& text) override
        { clipboard = text; return true; }
    QByteArray viewDocumentBytes(int view) const override
        { requestedView = view; return QByteArray("view-document"); }
    bool showBufferInView(quintptr bufferId, int view) override
        { shownBuffer = bufferId; shownView = view; return true; }
    void clearCompareMarks(int view) override { clearedView = view; }
    bool addCompareMark(int view, qint64 line, quint32 kind) override
        { markedView = view; markedLine = line; markedKind = kind; return true; }
    qint64 firstVisibleLine(int view) const override
        { requestedTopView = view; return 12; }
    bool setFirstVisibleLine(int view, qint64 line) override
        { scrolledView = view; scrolledLine = line; return true; }
    bool gotoLine(int view, qint64 line) override
        { gotoView = view; gotoTarget = line; return true; }
    qintptr sendScintilla(int view, quint32 message,
                          quintptr wParam, qintptr lParam) override
        { sciView=view;sciMessage=message;sciWParam=wParam;sciLParam=lParam;return 99; }
    bool saveCurrentFile() override { saved=true;return true; }

    QString openedPath;
    mutable QByteArray document = QByteArray("document");
    QByteArray selectionReplacement;
    qint64 selectionStart = 0;
    qint64 selectionEnd = 0;
    QByteArray createdDocument;
    QString clipboard;
    mutable int requestedView = -1;
    quintptr shownBuffer = 0;
    int shownView = -1;
    int clearedView = -1;
    int markedView = -1;
    qint64 markedLine = -1;
    quint32 markedKind = 0;
    mutable int requestedTopView = -1;
    int scrolledView = -1;
    qint64 scrolledLine = -1;
    int gotoView = -1;
    qint64 gotoTarget = -1;
    int sciView=-1;quint32 sciMessage=0;quintptr sciWParam=0;qintptr sciLParam=0;
    bool saved=false;int executedCommand=0;
};

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    TestHostServices host;
    PluginManager manager(&host);
    QString error;
    require(manager.loadPlugin(
        QString::fromUtf8(NPP_CROSS_PLATFORM_TEST_PLUGIN),
        QStringLiteral("CrossPlatformAbiTest"), &error),
        qPrintable(error));
    require(manager.loadedPluginCount() == 1, "plugin count mismatch");
    require(manager.loadedPluginNames()
        == QStringList{QStringLiteral("Cross-platform ABI test")},
        "plugin name mismatch");
    require(manager.loadedPluginFunctionCount(0) == 1,
            "function count mismatch");
    require(manager.loadedPluginFunctionName(0, 0)
        == QStringLiteral("Run ABI test command"),
        "function name mismatch");
    require(manager.isLoadedPluginFunctionInitiallyChecked(0, 0),
            "initial checked state was not preserved");
    require(manager.sendPluginMessage(0, 1) == 1,
            "host environment was not received");
    require(manager.sendPluginMessage(0, 2) == 1,
            "current file callback failed");
    require(manager.sendPluginMessage(0, 5) != NPP_PLUGIN_SYSTEM_UNKNOWN,
            "system type was not transmitted");

    manager.notifyReady();
    manager.notifyReady();
    require(manager.sendPluginMessage(0, 3) == 1,
            "READY notification was not sent exactly once");
    require(manager.executePluginCommand(0, 0, &error), qPrintable(error));
    require(manager.sendPluginMessage(0, 4) == 1,
            "plugin command did not run");
    require(manager.sendPluginMessage(0, 7) == 1,
            "extended ABI callbacks failed");
    manager.notifyPlugins(NPP_PLUGIN_NOTIFICATION_TEXT_MODIFIED,
                          42, 1, 5, 3, 7, 0, QByteArray("abc"));
    require(manager.sendPluginMessage(0, 8) == 1,
            "extended notification payload failed");
    require(host.openedPath == QStringLiteral("abi-plugin-open.txt"),
            "plugin open-file callback did not reach host services");
    require(host.document == QByteArray("updated"),
            "binary-safe document replacement did not reach host services");
    require(host.createdDocument == QByteArray("new"),
            "document creation did not reach host services");
    require(host.clipboard == QStringLiteral("updated clipboard"),
            "clipboard callback did not reach host services");
    require(host.selectionStart == 2 && host.selectionEnd == 5,
            "selection callback did not reach host services");
    require(host.requestedView == 0 && host.shownBuffer == 42 && host.shownView == 1,
            "view document/buffer callbacks did not reach host services");
    require(host.clearedView == 1 && host.markedView == 0
            && host.markedLine == 7 && host.markedKind == 2,
            "compare marker callbacks did not reach host services");
    require(host.requestedTopView == 0 && host.scrolledView == 1
            && host.scrolledLine == 12 && host.gotoView == 0 && host.gotoTarget == 7,
            "view navigation callbacks did not reach host services");
    require(host.sciView==1&&host.sciMessage==2006&&host.sciWParam==2
            &&host.sciLParam==3&&host.saved&&host.executedCommand==41007,
            "deep plugin host callbacks did not reach host services");
    manager.unloadAll();
    require(manager.loadedPluginCount() == 0, "plugin did not unload");
    return 0;
}
