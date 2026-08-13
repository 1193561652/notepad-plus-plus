#pragma once

#include <QString>
#include <QVector>

struct PluginArtifact
{
    QString folderName;
    QString binaryPath;
};

class PluginArtifactResolver
{
public:
    static QString platformDirectoryName();
    static QString librarySuffix();
    static QString binaryPath(const QString& pluginRoot,
                              const QString& folderName);
    static QString crossPlatformBinaryPath(const QString& pluginRoot,
                                           const QString& folderName);
    static QVector<PluginArtifact> discover(const QString& pluginRoot);
    static QVector<PluginArtifact> discoverCrossPlatform(
        const QString& pluginRoot);
};
