#include "RuntimePathResolver.h"

#include "RuntimePathConfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

QString RuntimePathResolver::normalized(const QString& path)
{
    if (path.trimmed().isEmpty())
        return QString();
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

void RuntimePathResolver::appendUnique(QStringList& paths,
                                       const QString& path)
{
    const QString candidate = normalized(path);
    if (candidate.isEmpty())
        return;
    for (const QString& existing : paths) {
        if (existing.compare(candidate,
#if defined(Q_OS_WIN)
                             Qt::CaseInsensitive
#else
                             Qt::CaseSensitive
#endif
                             ) == 0) {
            return;
        }
    }
    paths.append(candidate);
}

QStringList RuntimePathResolver::resourceRoots(
    const QString& applicationDirectory,
    const QString& installedDataDirectory)
{
    QStringList result;
    appendUnique(result, applicationDirectory);
#if !defined(Q_OS_WIN)
    const QDir application(applicationDirectory);
    appendUnique(result,
                 application.filePath(
                     QStringLiteral(NPP_RELATIVE_DATA_DIRECTORY)));
    appendUnique(result, installedDataDirectory.isEmpty()
                             ? QStringLiteral(NPP_COMPILED_DATA_DIRECTORY)
                             : installedDataDirectory);
#else
    Q_UNUSED(installedDataDirectory)
#endif
    return result;
}

QString RuntimePathResolver::resourceDirectory(
    const QString& applicationDirectory, const QString& relativePath,
    const QString& installedDataDirectory)
{
    const QStringList roots = resourceRoots(
        applicationDirectory, installedDataDirectory);
    for (const QString& root : roots) {
        const QString candidate = QDir(root).filePath(relativePath);
        if (QFileInfo(candidate).isDir())
            return normalized(candidate);
    }
    return roots.isEmpty()
        ? QString() : normalized(QDir(roots.constFirst()).filePath(relativePath));
}

QString RuntimePathResolver::resourceFile(
    const QString& applicationDirectory, const QString& relativePath,
    const QString& installedDataDirectory)
{
    const QStringList roots = resourceRoots(
        applicationDirectory, installedDataDirectory);
    for (const QString& root : roots) {
        const QString candidate = QDir(root).filePath(relativePath);
        if (QFileInfo(candidate).isFile())
            return normalized(candidate);
    }
    return QString();
}

QString RuntimePathResolver::userDataDirectory()
{
#if defined(Q_OS_WIN)
    return QCoreApplication::applicationDirPath();
#else
    QString base = QStandardPaths::writableLocation(
        QStandardPaths::GenericDataLocation);
    if (base.isEmpty()) {
        const QString xdgDataHome = qEnvironmentVariable("XDG_DATA_HOME");
        base = xdgDataHome.isEmpty()
            ? QDir::home().filePath(QStringLiteral(".local/share"))
            : xdgDataHome;
    }
    return normalized(QDir(base).filePath(
        QStringLiteral("notepad-plus-plus-qt")));
#endif
}

QString RuntimePathResolver::userPluginDirectory()
{
#if defined(Q_OS_WIN)
    return normalized(QDir(QCoreApplication::applicationDirPath())
                          .filePath(QStringLiteral("plugins")));
#else
    return normalized(QDir(userDataDirectory())
                          .filePath(QStringLiteral("plugins")));
#endif
}

QString RuntimePathResolver::installedPluginDirectory(
    const QString& applicationDirectory)
{
#if defined(Q_OS_WIN)
    return normalized(QDir(applicationDirectory)
                          .filePath(QStringLiteral("plugins")));
#else
    const QString relocatable = normalized(QDir(applicationDirectory).filePath(
        QStringLiteral(NPP_RELATIVE_PLUGIN_DIRECTORY)));
    if (QFileInfo(relocatable).isDir())
        return relocatable;
    return normalized(QStringLiteral(NPP_COMPILED_PLUGIN_DIRECTORY));
#endif
}

QStringList RuntimePathResolver::pluginRoots(
    const QString& applicationDirectory, bool portableMode)
{
    QStringList result;
#if defined(Q_OS_WIN)
    Q_UNUSED(portableMode)
    appendUnique(result, QDir(applicationDirectory)
                             .filePath(QStringLiteral("plugins")));
#else
    if (portableMode) {
        appendUnique(result, QDir(applicationDirectory)
                                 .filePath(QStringLiteral("plugins")));
        return result;
    }
    appendUnique(result, userPluginDirectory());
    appendUnique(result, QDir(applicationDirectory)
                             .filePath(QStringLiteral("plugins")));
    appendUnique(result, installedPluginDirectory(applicationDirectory));
#endif
    return result;
}

QString RuntimePathResolver::writablePluginRoot(
    const QString& applicationDirectory, bool portableMode)
{
#if defined(Q_OS_WIN)
    Q_UNUSED(portableMode)
    return normalized(QDir(applicationDirectory)
                          .filePath(QStringLiteral("plugins")));
#else
    if (portableMode) {
        return normalized(QDir(applicationDirectory)
                              .filePath(QStringLiteral("plugins")));
    }
    return userPluginDirectory();
#endif
}

QString RuntimePathResolver::pluginUpdaterPath(
    const QString& applicationDirectory)
{
    QString updaterName = QStringLiteral("npp-plugin-updater");
#if defined(Q_OS_WIN)
    updaterName += QStringLiteral(".exe");
    return normalized(QDir(applicationDirectory).filePath(updaterName));
#else
    const QString applicationRelative = normalized(
        QDir(applicationDirectory).filePath(updaterName));
    if (QFileInfo(applicationRelative).isFile())
        return applicationRelative;
    const QString relocatable = normalized(QDir(applicationDirectory).filePath(
        QStringLiteral(NPP_RELATIVE_LIBEXEC_DIRECTORY) + QLatin1Char('/')
        + updaterName));
    if (QFileInfo(relocatable).isFile())
        return relocatable;
    return normalized(QDir(QStringLiteral(NPP_COMPILED_LIBEXEC_DIRECTORY))
                          .filePath(updaterName));
#endif
}
