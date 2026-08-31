#include "MISC/RuntimePathResolver.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}

void touch(const QString& path)
{
    QFile file(path);
    require(file.open(QFile::WriteOnly), "could not create test file");
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    QTemporaryDir temporary;
    require(temporary.isValid(), "temporary directory is invalid");

    QDir root(temporary.path());
    require(root.mkpath(QStringLiteral("bin/localization")),
            "portable localization directory creation failed");
    require(root.mkpath(QStringLiteral("share/functionList")),
            "installed function list directory creation failed");
    const QString appDir = root.filePath(QStringLiteral("bin"));
    const QString dataDir = root.filePath(QStringLiteral("share"));

    require(RuntimePathResolver::resourceDirectory(
                appDir, QStringLiteral("localization"), dataDir)
                == QDir(appDir).filePath(QStringLiteral("localization")),
            "application-relative resources must keep Windows/portable priority");
    require(RuntimePathResolver::resourceDirectory(
                appDir, QStringLiteral("functionList"), dataDir)
                == QDir(dataDir).filePath(QStringLiteral("functionList")),
            "installed shared data must be used when portable data is absent");

    touch(QDir(dataDir).filePath(QStringLiteral("nativeLang.xml")));
    require(RuntimePathResolver::resourceFile(
                appDir, QStringLiteral("nativeLang.xml"), dataDir)
                == QDir(dataDir).filePath(QStringLiteral("nativeLang.xml")),
            "installed shared resource file was not resolved");

    const QStringList roots = RuntimePathResolver::resourceRoots(appDir, dataDir);
    require(!roots.isEmpty() && roots.constFirst() == appDir,
            "application directory must remain the first resource root");

    const QString portablePluginRoot =
        QDir(appDir).filePath(QStringLiteral("plugins"));
    require(RuntimePathResolver::pluginRoots(appDir, true)
                == QStringList{portablePluginRoot},
            "portable plugin lookup must stay application-relative");
    require(RuntimePathResolver::writablePluginRoot(appDir, true)
                == portablePluginRoot,
            "portable plugin writes must stay application-relative");

    std::cout << "Runtime path resolver tests passed." << std::endl;
    return 0;
}
