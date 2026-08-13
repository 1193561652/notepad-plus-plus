#include "MISC/PluginsManager/PluginHostServices.h"

#include <QCoreApplication>
#include <QDir>
#include <QSysInfo>

#include "MainWindow.h"
#include "Parameters.h"
#include "CrossPlatformPluginSystem/PluginInterface.h"

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
