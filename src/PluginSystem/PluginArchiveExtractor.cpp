#include "PluginArchiveExtractor.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>

namespace {

bool runProcess(const QString& program, const QStringList& arguments,
                QByteArray* standardOutput, QString* error)
{
    QProcess process;
    process.start(program, arguments, QIODevice::ReadOnly);
    if (!process.waitForStarted(10000)) {
        if (error)
            *error = process.errorString();
        return false;
    }
    if (!process.waitForFinished(120000)) {
        process.kill();
        process.waitForFinished();
        if (error)
            *error = QStringLiteral("Archive tool timed out.");
        return false;
    }
    if (standardOutput)
        *standardOutput = process.readAllStandardOutput();
    if (process.exitStatus() != QProcess::NormalExit ||
        process.exitCode() != 0) {
        if (error) {
            *error = QString::fromLocal8Bit(process.readAllStandardError())
                         .trimmed();
            if (error->isEmpty())
                *error = QStringLiteral("Archive tool failed.");
        }
        return false;
    }
    return true;
}

#if defined(Q_OS_WIN)
const char kListScript[] =
    "Add-Type -AssemblyName System.IO.Compression.FileSystem; "
    "$a=[IO.Compression.ZipFile]::OpenRead($args[0]); "
    "try {$a.Entries | ForEach-Object {$_.FullName}} finally {$a.Dispose()}";
const char kExtractScript[] =
    "Add-Type -AssemblyName System.IO.Compression.FileSystem; "
    "[IO.Compression.ZipFile]::ExtractToDirectory($args[0],$args[1])";
#endif

bool containsSymbolicLink(const QDir& directory, QString* linkPath)
{
    const QFileInfoList entries = directory.entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden |
            QDir::System,
        QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo& info : entries) {
        if (info.isSymLink()) {
            if (linkPath)
                *linkPath = info.absoluteFilePath();
            return true;
        }
        if (info.isDir() &&
            containsSymbolicLink(QDir(info.absoluteFilePath()), linkPath)) {
            return true;
        }
    }
    return false;
}

} // namespace

bool PluginArchiveExtractor::validateEntry(const QString& entry,
                                           QString* error)
{
    const QString normalized =
        QDir::fromNativeSeparators(entry).trimmed();
    if (normalized.isEmpty())
        return true;
    if (normalized.startsWith(QLatin1Char('/')) ||
        normalized.contains(QLatin1Char(':'))) {
        if (error)
            *error = QStringLiteral("Archive contains an absolute path: %1")
                         .arg(entry);
        return false;
    }
    const QStringList parts =
        normalized.split(QLatin1Char('/'), QString::SkipEmptyParts);
    for (const QString& part : parts) {
        if (part == QStringLiteral("..")) {
            if (error)
                *error = QStringLiteral(
                    "Archive contains a parent-directory entry: %1")
                             .arg(entry);
            return false;
        }
    }
    return true;
}

bool PluginArchiveExtractor::extractZip(const QString& archivePath,
                                        const QString& destination,
                                        QString* error)
{
    QDir destinationDir(destination);
    if ((!destinationDir.exists() &&
         !QDir().mkpath(destinationDir.absolutePath())) ||
        !QFileInfo(archivePath).isFile()) {
        if (error)
            *error = QStringLiteral("Archive or destination is unavailable.");
        return false;
    }

    QByteArray listing;
#if defined(Q_OS_WIN)
    const QString program = QStringLiteral("powershell.exe");
    const QStringList listArguments = {
        QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
        QStringLiteral("-Command"), QString::fromLatin1(kListScript),
        archivePath
    };
#else
    const QString program = QStringLiteral("unzip");
    const QStringList listArguments = {
        QStringLiteral("-Z1"), archivePath
    };
#endif
    if (!runProcess(program, listArguments, &listing, error))
        return false;

    const QStringList entries =
        QString::fromUtf8(listing).split(QLatin1Char('\n'),
                                         QString::SkipEmptyParts);
    if (entries.isEmpty()) {
        if (error)
            *error = QStringLiteral("Plugin archive is empty.");
        return false;
    }
    for (const QString& entry : entries) {
        if (!validateEntry(entry.trimmed(), error))
            return false;
    }

#if defined(Q_OS_WIN)
    const QStringList extractArguments = {
        QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
        QStringLiteral("-Command"), QString::fromLatin1(kExtractScript),
        archivePath, destinationDir.absolutePath()
    };
#else
    const QStringList extractArguments = {
        QStringLiteral("-qq"), archivePath,
        QStringLiteral("-d"), destinationDir.absolutePath()
    };
#endif
    if (!runProcess(program, extractArguments, nullptr, error))
        return false;

    QString linkPath;
    if (containsSymbolicLink(destinationDir, &linkPath)) {
        if (error) {
            *error = QStringLiteral(
                "Plugin archive contains a symbolic link: %1")
                         .arg(linkPath);
        }
        return false;
    }
    return true;
}
