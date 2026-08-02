#include "MISC/PluginsManager/updater/PluginArchiveExtractor.h"
#include "MISC/PluginsManager/PluginArtifactResolver.h"
#include "MISC/PluginsManager/updater/PluginUpdateExecutor.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QTemporaryDir>
#include <QUrl>

#include <iostream>

namespace {

int failures = 0;

void check(bool condition, const char* message)
{
    if (condition)
        return;
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
}

void append16(QByteArray* bytes, quint16 value)
{
    bytes->append(static_cast<char>(value & 0xff));
    bytes->append(static_cast<char>((value >> 8) & 0xff));
}

void append32(QByteArray* bytes, quint32 value)
{
    append16(bytes, static_cast<quint16>(value & 0xffff));
    append16(bytes, static_cast<quint16>((value >> 16) & 0xffff));
}

quint32 crc32(const QByteArray& data)
{
    quint32 crc = 0xffffffffu;
    for (unsigned char byte : data) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

bool writeStoredZip(const QString& path,
                    const QVector<QPair<QString, QByteArray>>& entries)
{
    struct CentralEntry
    {
        QByteArray name;
        QByteArray data;
        quint32 crc = 0;
        quint32 offset = 0;
    };
    QVector<CentralEntry> centralEntries;
    QByteArray bytes;
    for (const auto& source : entries) {
        CentralEntry entry;
        entry.name = source.first.toUtf8();
        entry.data = source.second;
        entry.crc = crc32(entry.data);
        entry.offset = static_cast<quint32>(bytes.size());

        append32(&bytes, 0x04034b50);
        append16(&bytes, 20);
        append16(&bytes, 0);
        append16(&bytes, 0);
        append16(&bytes, 0);
        append16(&bytes, 0);
        append32(&bytes, entry.crc);
        append32(&bytes, static_cast<quint32>(entry.data.size()));
        append32(&bytes, static_cast<quint32>(entry.data.size()));
        append16(&bytes, static_cast<quint16>(entry.name.size()));
        append16(&bytes, 0);
        bytes.append(entry.name);
        bytes.append(entry.data);
        centralEntries.append(entry);
    }

    const quint32 centralOffset = static_cast<quint32>(bytes.size());
    for (const CentralEntry& entry : centralEntries) {
        append32(&bytes, 0x02014b50);
        append16(&bytes, 20);
        append16(&bytes, 20);
        append16(&bytes, 0);
        append16(&bytes, 0);
        append16(&bytes, 0);
        append16(&bytes, 0);
        append32(&bytes, entry.crc);
        append32(&bytes, static_cast<quint32>(entry.data.size()));
        append32(&bytes, static_cast<quint32>(entry.data.size()));
        append16(&bytes, static_cast<quint16>(entry.name.size()));
        append16(&bytes, 0);
        append16(&bytes, 0);
        append16(&bytes, 0);
        append16(&bytes, 0);
        append32(&bytes, 0);
        append32(&bytes, entry.offset);
        bytes.append(entry.name);
    }
    const quint32 centralSize =
        static_cast<quint32>(bytes.size()) - centralOffset;
    append32(&bytes, 0x06054b50);
    append16(&bytes, 0);
    append16(&bytes, 0);
    append16(&bytes, static_cast<quint16>(centralEntries.size()));
    append16(&bytes, static_cast<quint16>(centralEntries.size()));
    append32(&bytes, centralSize);
    append32(&bytes, centralOffset);
    append16(&bytes, 0);

    QFile file(path);
    return file.open(QFile::WriteOnly) && file.write(bytes) == bytes.size();
}

QString binaryEntry(const QString& folderName)
{
    const QString platform =
        PluginArtifactResolver::platformDirectoryName();
    const QString fileName =
        folderName + PluginArtifactResolver::librarySuffix();
    return platform.isEmpty() ? fileName
                              : platform + QLatin1Char('/') + fileName;
}

QString sha256(const QString& path)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly))
        return QString();
    return QString::fromLatin1(
        QCryptographicHash::hash(file.readAll(),
                                 QCryptographicHash::Sha256).toHex());
}

PluginOperation packageOperation(PluginOperationType type,
                                 const QString& folderName,
                                 const QString& version,
                                 const QString& package)
{
    PluginOperation operation;
    operation.type = type;
    operation.folderName = folderName;
    operation.version = version;
    operation.repository = QUrl::fromLocalFile(package).toString();
    operation.packageSha256 = sha256(package);
    return operation;
}

QByteArray readFile(const QString& path)
{
    QFile file(path);
    return file.open(QFile::ReadOnly) ? file.readAll() : QByteArray();
}

void testInstallUpdateRemoveAndPreflight()
{
    QTemporaryDir temporary;
    check(temporary.isValid(), "temporary updater root should exist");
    const QString pluginRoot =
        QDir(temporary.path()).filePath(QStringLiteral("plugins"));
    QDir().mkpath(pluginRoot);
    const QString v1Package =
        QDir(temporary.path()).filePath(QStringLiteral("sample-v1.zip"));
    const QString v2Package =
        QDir(temporary.path()).filePath(QStringLiteral("sample-v2.zip"));
    const QString brokenPackage =
        QDir(temporary.path()).filePath(QStringLiteral("broken.zip"));
    check(writeStoredZip(v1Package,
                         {{binaryEntry(QStringLiteral("Sample")), "v1"},
                          {QStringLiteral("readme.txt"), "readme"}}),
          "v1 package should be created");
    check(writeStoredZip(v2Package,
                         {{binaryEntry(QStringLiteral("Sample")), "v2"}}),
          "v2 package should be created");
    check(writeStoredZip(brokenPackage,
                         {{QStringLiteral("readme.txt"), "broken"}}),
          "broken package should be created");

    PluginUpdatePlan plan;
    plan.applicationPath = QCoreApplication::applicationFilePath();
    plan.pluginRoot = pluginRoot;
    plan.operations.append(packageOperation(
        PluginOperationType::Install, QStringLiteral("Sample"),
        QStringLiteral("1.0"), v1Package));
    QString error;
    const bool installed = PluginUpdateExecutor::apply(plan, &error);
    if (!installed)
        std::cerr << "Install error: " << error.toStdString() << '\n';
    check(installed, "local package install should succeed");
    const QString binary = PluginArtifactResolver::binaryPath(
        pluginRoot, QStringLiteral("Sample"));
    check(readFile(binary) == QByteArray("v1"),
          "install should place the platform binary");
    check(QFileInfo(QDir(pluginRoot).filePath(
              QStringLiteral("Sample/.npp-package.json"))).isFile(),
          "install should write a package receipt");

    plan.operations.clear();
    plan.operations.append(packageOperation(
        PluginOperationType::Update, QStringLiteral("Sample"),
        QStringLiteral("2.0"), v2Package));
    const bool updated = PluginUpdateExecutor::apply(plan, &error);
    if (!updated)
        std::cerr << "Update error: " << error.toStdString() << '\n';
    check(updated, "local package update should succeed");
    check(readFile(binary) == QByteArray("v2"),
          "update should replace the platform binary");

    plan.operations.clear();
    plan.operations.append(packageOperation(
        PluginOperationType::Update, QStringLiteral("Sample"),
        QStringLiteral("3.0"), v2Package));
    plan.operations.append(packageOperation(
        PluginOperationType::Install, QStringLiteral("Broken"),
        QStringLiteral("1.0"), brokenPackage));
    check(!PluginUpdateExecutor::apply(plan, &error),
          "batch preflight should reject a package without its binary");
    check(readFile(binary) == QByteArray("v2"),
          "failed batch preflight must not change existing plugins");

    plan.operations.clear();
    PluginOperation remove;
    remove.type = PluginOperationType::Remove;
    remove.folderName = QStringLiteral("Sample");
    plan.operations.append(remove);
    check(PluginUpdateExecutor::apply(plan, &error),
          "plugin removal should succeed");
    check(!QFileInfo::exists(QDir(pluginRoot).filePath(
              QStringLiteral("Sample"))),
          "plugin removal should remove the complete plugin directory");
}

void testHashAndTraversalFailuresLeaveStateUntouched()
{
    QTemporaryDir temporary;
    check(temporary.isValid(), "temporary security root should exist");
    const QString pluginRoot =
        QDir(temporary.path()).filePath(QStringLiteral("plugins"));
    const QString existing = QDir(pluginRoot).filePath(
        QStringLiteral("Sample/keep.txt"));
    QDir().mkpath(QFileInfo(existing).absolutePath());
    QFile keep(existing);
    keep.open(QFile::WriteOnly);
    keep.write("keep");
    keep.close();

    const QString package =
        QDir(temporary.path()).filePath(QStringLiteral("sample.zip"));
    writeStoredZip(package,
                   {{binaryEntry(QStringLiteral("Sample")), "new"}});
    PluginUpdatePlan plan;
    plan.applicationPath = QCoreApplication::applicationFilePath();
    plan.pluginRoot = pluginRoot;
    PluginOperation update = packageOperation(
        PluginOperationType::Update, QStringLiteral("Sample"),
        QStringLiteral("2.0"), package);
    update.packageSha256 = QString(64, QLatin1Char('0'));
    plan.operations.append(update);
    QString error;
    check(!PluginUpdateExecutor::apply(plan, &error),
          "wrong package hash should be rejected");
    check(readFile(existing) == QByteArray("keep"),
          "wrong hash must leave the old plugin untouched");

    const QString malicious =
        QDir(temporary.path()).filePath(QStringLiteral("traversal.zip"));
    writeStoredZip(malicious,
                   {{QStringLiteral("../escape.txt"), "escape"}});
    const QString destination =
        QDir(temporary.path()).filePath(QStringLiteral("extract"));
    check(!PluginArchiveExtractor::extractZip(
              malicious, destination, &error),
          "archive traversal should be rejected before extraction");
    check(!QFileInfo::exists(QDir(temporary.path()).filePath(
              QStringLiteral("escape.txt"))),
          "archive traversal must not write outside the destination");
}

void testScheduledPlanHashRejectsTampering()
{
    QTemporaryDir temporary;
    check(temporary.isValid(), "temporary plan root should exist");
    PluginUpdatePlan plan;
    plan.applicationPath = QCoreApplication::applicationFilePath();
    plan.pluginRoot =
        QDir(temporary.path()).filePath(QStringLiteral("plugins"));
    PluginOperation remove;
    remove.type = PluginOperationType::Remove;
    remove.folderName = QStringLiteral("Sample");
    plan.operations.append(remove);
    const QString planPath =
        QDir(temporary.path()).filePath(QStringLiteral("plan.json"));
    QString error;
    check(plan.write(planPath, &error),
          "tamper test plan should be written");

    QProcess updater;
    updater.start(QString::fromLocal8Bit(NPP_PLUGIN_UPDATER_PATH),
                  {QStringLiteral("--plan"), planPath,
                   QStringLiteral("--plan-sha256"),
                   QString(64, QLatin1Char('0'))});
    check(updater.waitForFinished(30000),
          "updater tamper test should finish");
    check(updater.exitStatus() == QProcess::NormalExit &&
              updater.exitCode() == 4,
          "updater should reject a changed scheduled plan");
    check(QFileInfo::exists(planPath),
          "rejected update plan should remain available for diagnosis");
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    testInstallUpdateRemoveAndPreflight();
    testHashAndTraversalFailuresLeaveStateUntouched();
    testScheduledPlanHashRejectsTampering();
    if (failures == 0)
        std::cout << "Plugin updater tests passed\n";
    return failures == 0 ? 0 : 1;
}
