#include "MISC/ConfigPathResolver.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Notepad++"));
    app.setOrganizationName(QStringLiteral("Notepad++"));

    QTemporaryDir temporary;
    require(temporary.isValid(), "temporary directory is invalid");
    QDir root(temporary.path());
    require(root.mkpath(QStringLiteral("application")), "application dir failed");
    require(root.mkpath(QStringLiteral("explicit")), "explicit dir failed");

    const QString application = root.filePath(QStringLiteral("application"));
    const QString explicitPath = root.filePath(QStringLiteral("explicit"));

#if defined(Q_OS_WIN)
    const QString expectedDefault =
        QDir(qEnvironmentVariable("APPDATA")).filePath(QStringLiteral("Notepad++"));
    require(QDir::cleanPath(ConfigPathResolver::platformDefaultDirectory()) ==
            QDir::cleanPath(expectedDefault),
            "Windows default must remain %APPDATA%/Notepad++ compatible");
#endif

    ConfigPathResolution resolved =
        ConfigPathResolver::resolve(application, explicitPath);
    require(resolved.isValid() &&
            resolved.source == ConfigPathSource::CommandLine,
            "command-line directory must have highest priority");
    require(resolved.directory == QDir(explicitPath).canonicalPath(),
            "command-line directory was not canonicalized");

    touch(root.filePath(QStringLiteral("application/doLocalConf.xml")));
    resolved = ConfigPathResolver::resolve(application);
    require(resolved.isValid() &&
            resolved.source == ConfigPathSource::Portable,
            "portable marker was not detected");
    require(resolved.directory == QDir(application).canonicalPath(),
            "portable settings directory is incorrect");

    resolved = ConfigPathResolver::resolve(
        application, root.filePath(QStringLiteral("missing")));
    require(!resolved.isValid() &&
            resolved.source == ConfigPathSource::CommandLine,
            "missing explicit directory must be rejected");

    const QString filePath = root.filePath(QStringLiteral("not-a-directory"));
    touch(filePath);
    resolved = ConfigPathResolver::resolve(application, filePath);
    require(!resolved.isValid(), "file used as settings dir must be rejected");

    const QString creatable = root.filePath(QStringLiteral("new/settings"));
    QString error;
    require(ConfigPathResolver::prepareDirectory(creatable, true, &error),
            "default settings directory could not be created");
    require(QFileInfo(creatable).isDir(),
            "prepared settings directory does not exist");

    std::cout << "Config path resolver tests passed." << std::endl;
    return 0;
}
