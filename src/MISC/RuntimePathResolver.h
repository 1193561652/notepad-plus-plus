#pragma once

#include <QString>
#include <QStringList>

// Centralizes the distinction between the original portable Windows layout
// and an FHS-style Unix installation.  Application-relative paths remain the
// first lookup location so Windows packages and portable/build trees keep the
// original Notepad++ behavior.
class RuntimePathResolver
{
public:
    static QStringList resourceRoots(
        const QString& applicationDirectory,
        const QString& installedDataDirectory = QString());
    static QString resourceDirectory(
        const QString& applicationDirectory, const QString& relativePath,
        const QString& installedDataDirectory = QString());
    static QString resourceFile(
        const QString& applicationDirectory, const QString& relativePath,
        const QString& installedDataDirectory = QString());

    static QString userDataDirectory();
    static QString userPluginDirectory();
    static QString installedPluginDirectory(
        const QString& applicationDirectory);
    static QStringList pluginRoots(const QString& applicationDirectory,
                                   bool portableMode = false);
    static QString writablePluginRoot(const QString& applicationDirectory,
                                      bool portableMode = false);
    static QString pluginUpdaterPath(const QString& applicationDirectory);

private:
    static QString normalized(const QString& path);
    static void appendUnique(QStringList& paths, const QString& path);
};
