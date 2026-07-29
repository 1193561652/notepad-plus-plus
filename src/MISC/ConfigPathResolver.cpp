#include "ConfigPathResolver.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryFile>

QString ConfigPathResolver::normalizedAbsolutePath(const QString& path)
{
    QString expanded = QDir::fromNativeSeparators(path.trimmed());
    if (expanded == QStringLiteral("~")) {
        expanded = QDir::homePath();
    } else if (expanded.startsWith(QStringLiteral("~/"))) {
        expanded = QDir::home().filePath(expanded.mid(2));
    }

    QFileInfo info(expanded);
    const QString absolute = QDir::cleanPath(info.absoluteFilePath());
    const QString canonical = QFileInfo(absolute).canonicalFilePath();
    return canonical.isEmpty() ? absolute : QDir::cleanPath(canonical);
}

QString ConfigPathResolver::platformDefaultDirectory()
{
#if defined(Q_OS_WIN)
    const QString appData = qEnvironmentVariable("APPDATA");
    if (!appData.isEmpty())
        return QDir(appData).filePath(QStringLiteral("Notepad++"));
    return QDir::home().filePath(QStringLiteral("AppData/Roaming/Notepad++"));
#else
    QString directory =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (!directory.isEmpty())
        return QDir::cleanPath(directory);

#if defined(Q_OS_MAC)
    return QDir::home().filePath(
        QStringLiteral("Library/Preferences/Notepad++"));
#else
    const QString configHome = qEnvironmentVariable("XDG_CONFIG_HOME");
    if (!configHome.isEmpty())
        return QDir(configHome).filePath(QStringLiteral("Notepad++"));
    return QDir::home().filePath(QStringLiteral(".config/Notepad++"));
#endif

    return QDir::home().filePath(
        QStringLiteral(".config/") + QCoreApplication::applicationName());
#endif
}

ConfigPathResolution ConfigPathResolver::resolve(
    const QString& applicationDirectory, const QString& commandLineDirectory)
{
    ConfigPathResolution result;
    const QString appDirectory = normalizedAbsolutePath(applicationDirectory);

    if (!commandLineDirectory.trimmed().isEmpty()) {
        result.source = ConfigPathSource::CommandLine;
        result.directory = normalizedAbsolutePath(commandLineDirectory);
        if (!prepareDirectory(result.directory, false, &result.error))
            result.directory.clear();
        return result;
    }

    const QString portableMarker =
        QDir(appDirectory).filePath(QStringLiteral("doLocalConf.xml"));
    if (QFileInfo(portableMarker).isFile()) {
        result.source = ConfigPathSource::Portable;
        result.directory = appDirectory;
        return result;
    }

    result.source = ConfigPathSource::PlatformDefault;
    result.directory = normalizedAbsolutePath(platformDefaultDirectory());
    return result;
}

bool ConfigPathResolver::prepareDirectory(const QString& directory,
                                          bool allowCreate, QString* error)
{
    const QString path = normalizedAbsolutePath(directory);
    QFileInfo info(path);
    if (info.exists() && !info.isDir()) {
        if (error)
            *error = QStringLiteral("The settings path is not a directory: %1")
                         .arg(QDir::toNativeSeparators(path));
        return false;
    }

    if (!info.exists()) {
        if (!allowCreate || !QDir().mkpath(path)) {
            if (error) {
                *error = QStringLiteral("The settings directory does not exist "
                                       "or cannot be created: %1")
                             .arg(QDir::toNativeSeparators(path));
            }
            return false;
        }
    }

    QTemporaryFile probe(
        QDir(path).filePath(QStringLiteral(".nppqt-write-test-XXXXXX")));
    probe.setAutoRemove(true);
    if (!probe.open()) {
        if (error) {
            *error = QStringLiteral("The settings directory is not writable: %1")
                         .arg(QDir::toNativeSeparators(path));
        }
        return false;
    }
    return true;
}
