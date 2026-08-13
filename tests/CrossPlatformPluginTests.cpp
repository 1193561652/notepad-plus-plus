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
    bool executeMenuCommand(int) override { return false; }
    bool saveCurrentFileAs(const QString&, bool) override { return false; }
    bool saveCurrentSession(const QString&) override { return false; }
    bool loadSession(const QString&) override { return false; }
    bool setCurrentLanguageType(int) override { return false; }
    quintptr currentBufferId() const override { return 0; }
    QString pathForBuffer(quintptr) const override { return QString(); }
    int positionForBuffer(quintptr, int) const override { return -1; }
    int openFileCount(int) const override { return 0; }
    int currentDocumentIndex(int) const override { return -1; }
    bool activateDocument(int, int) override { return false; }
    int currentLine() const override { return -1; }
    int bufferEncoding(quintptr) const override { return -1; }
    bool setBufferEncoding(quintptr, int) override { return false; }
    void setStatusBarText(int, const QString&) override {}
    bool addToolbarCommand(int) override { return false; }

    QString openedPath;
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
    require(host.openedPath == QStringLiteral("abi-plugin-open.txt"),
            "plugin open-file callback did not reach host services");
    manager.unloadAll();
    require(manager.loadedPluginCount() == 0, "plugin did not unload");
    return 0;
}
