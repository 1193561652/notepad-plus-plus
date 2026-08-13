#include "PluginArtifactResolver.h"

#include <QDir>
#include <QFileInfo>

QString PluginArtifactResolver::platformDirectoryName()
{
#if defined(Q_OS_WIN)
    return QString();
#elif defined(Q_OS_MAC)
    return QStringLiteral("macos");
#else
    return QStringLiteral("linux");
#endif
}

QString PluginArtifactResolver::librarySuffix()
{
#if defined(Q_OS_WIN)
    return QStringLiteral(".dll");
#elif defined(Q_OS_MAC)
    return QStringLiteral(".dylib");
#else
    return QStringLiteral(".so");
#endif
}

QString PluginArtifactResolver::binaryPath(const QString& pluginRoot,
                                           const QString& folderName)
{
    QDir pluginDirectory(QDir(pluginRoot).filePath(folderName));
    const QString platform = platformDirectoryName();
    if (!platform.isEmpty())
        pluginDirectory = QDir(pluginDirectory.filePath(platform));
    return QDir::cleanPath(
        pluginDirectory.filePath(folderName + librarySuffix()));
}

QString PluginArtifactResolver::crossPlatformBinaryPath(
    const QString& pluginRoot, const QString& folderName)
{
#if defined(Q_OS_WIN)
    const QString platform = QStringLiteral("windows");
#elif defined(Q_OS_MAC)
    const QString platform = QStringLiteral("macos");
#else
    const QString platform = QStringLiteral("linux");
#endif
    return QDir::cleanPath(QDir(pluginRoot).filePath(
        QStringLiteral("%1/cross-platform/%2/%1%3")
            .arg(folderName, platform, librarySuffix())));
}

QVector<PluginArtifact> PluginArtifactResolver::discover(
    const QString& pluginRoot)
{
    QVector<PluginArtifact> artifacts;
    const QDir root(pluginRoot);
    const QFileInfoList directories =
        root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot,
                           QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo& directory : directories) {
        if (directory.isSymLink())
            continue;
        const QString folderName = directory.fileName();
        if (folderName.compare(QStringLiteral("Config"),
                               Qt::CaseInsensitive) == 0) {
            continue;
        }
        const QString path = binaryPath(pluginRoot, folderName);
        const QFileInfo binary(path);
        if (!binary.isFile() || binary.isSymLink())
            continue;
        PluginArtifact artifact;
        artifact.folderName = folderName;
        artifact.binaryPath = path;
        artifacts.append(artifact);
    }
    return artifacts;
}

QVector<PluginArtifact> PluginArtifactResolver::discoverCrossPlatform(
    const QString& pluginRoot)
{
    QVector<PluginArtifact> artifacts;
    const QDir root(pluginRoot);
    const QFileInfoList directories =
        root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot,
                           QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo& directory : directories) {
        if (directory.isSymLink()
            || directory.fileName().compare(
                QStringLiteral("Config"), Qt::CaseInsensitive) == 0) {
            continue;
        }
        const QString folderName = directory.fileName();
        const QString primaryPath = binaryPath(pluginRoot, folderName);
        const QFileInfo primaryBinary(primaryPath);
        if (primaryBinary.isFile() && !primaryBinary.isSymLink()) {
            artifacts.push_back({folderName, primaryPath});
            continue;
        }
        const QString compatibilityPath = crossPlatformBinaryPath(
            pluginRoot, folderName);
        const QFileInfo compatibilityBinary(compatibilityPath);
        if (compatibilityBinary.isFile() && !compatibilityBinary.isSymLink())
            artifacts.push_back({folderName, compatibilityPath});
    }
    return artifacts;
}
