#include "WinControls/PluginsAdmin/PluginAdminModel.h"
#include "MISC/PluginsManager/PluginArtifactResolver.h"
#include "MISC/PluginsManager/PluginCatalog.h"
#include "MISC/PluginsManager/PluginEnablementConfig.h"
#include "MISC/PluginsManager/PluginUpdatePlan.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

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

QByteArray catalogJson(const QString& hostRange = QStringLiteral("[8.4,8.5]"))
{
    return QStringLiteral(R"json(
{
  "version": "test",
  "npp-plugins": [
    {
      "folder-name": "Sample",
      "display-name": "Sample Plugin",
      "author": "Tester",
      "description": "Test plugin",
      "id": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
      "version": "2.0",
      "npp-compatible-versions": "%1",
      "old-versions-compatibility": "[1.0,1.9][8.0,8.4.6]",
      "repository": "https://example.invalid/Sample.zip",
      "homepage": "https://example.invalid/"
    }
  ]
}
)json").arg(hostRange).toUtf8();
}

void testVersionsAndCatalog()
{
    check(PluginVersion(QStringLiteral("8.4.6")) ==
              PluginVersion(QStringLiteral("8.4.6.0")),
          "versions should normalize to four parts");
    check(PluginVersion(QStringLiteral("8.4.7")) >
              PluginVersion(QStringLiteral("8.4.6.9")),
          "version comparison should be numeric");
    check(!PluginVersion(QStringLiteral("8.x")).isValid(),
          "non-numeric versions should be invalid");

    QString error;
    const PluginCatalog catalog =
        PluginCatalog::fromJson(catalogJson(), &error);
    check(catalog.isValid() && error.isEmpty(),
          "original-shaped catalog should parse");
    check(catalog.entries().size() == 1,
          "catalog should contain one plugin");
    const PluginCatalogEntry& entry = catalog.entries().first();
    check(entry.supportsHost(PluginVersion(QStringLiteral("8.4.6"))),
          "host interval should be inclusive");
    check(entry.supportsInstalledVersion(
              PluginVersion(QStringLiteral("1.5")),
              PluginVersion(QStringLiteral("8.4.6"))),
          "old plugin compatibility mapping should parse");
    check(!entry.supportsInstalledVersion(
              PluginVersion(QStringLiteral("1.5")),
              PluginVersion(QStringLiteral("8.5"))),
          "old plugin compatibility host range should be enforced");
    check(PluginCatalog::embedded().isValid(),
          "embedded placeholder catalog should remain valid");

    const QByteArray catalogWithBadEntry = QByteArray(R"json(
{
  "version": "test",
  "npp-plugins": [
    {"folder-name": "Broken"},
    {
      "folder-name": "Good",
      "display-name": "Good",
      "id": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
      "version": "1.0",
      "repository": "https://example.invalid/Good.zip"
    }
  ]
}
)json");
    const PluginCatalog tolerant =
        PluginCatalog::fromJson(catalogWithBadEntry, &error);
    check(tolerant.isValid() && tolerant.entries().size() == 1,
          "a malformed plugin entry should be ignored like v8.4.6");
}

void testBundledWindowsCatalogs()
{
    struct CatalogExpectation
    {
        const char* resource;
        int entries;
    };
    const CatalogExpectation expectations[] = {
        {":/pluginList/windows/pl.x86.json", 169},
        {":/pluginList/windows/pl.x64.json", 134},
        {":/pluginList/windows/pl.arm64.json", 20}
    };
    for (const CatalogExpectation& expectation : expectations) {
        QFile file(QString::fromLatin1(expectation.resource));
        check(file.open(QFile::ReadOnly),
              "bundled Windows catalog should be readable");
        QString error;
        const PluginCatalog catalog =
            PluginCatalog::fromJson(file.readAll(), &error);
        check(catalog.isValid() && error.isEmpty(),
              "bundled Windows catalog should parse");
        check(catalog.version() == QStringLiteral("1.5.4"),
              "bundled catalog should retain its original version");
        check(catalog.entries().size() == expectation.entries,
              "bundled catalog should retain every original entry");
    }
}

void writeReceipt(const QString& root, const QString& version)
{
    const QString path =
        PluginAdminModel::installReceiptPath(root, QStringLiteral("Sample"));
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    file.open(QFile::WriteOnly);
    QJsonObject object;
    object.insert(QStringLiteral("version"), version);
    file.write(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

void testPluginEnablementConfig()
{
    QTemporaryDir temporary;
    check(temporary.isValid(), "temporary config directory should exist");
    const QString path =
        PluginEnablementConfig::filePathForConfigDirectory(temporary.path());

    PluginEnablementConfig missing(path);
    QString error;
    check(missing.load(&error) && error.isEmpty(),
          "a missing enablement file should be an empty valid config");
    check(missing.enabledPlugins().isEmpty()
              && !missing.isEnabled(QStringLiteral("Sample")),
          "plugins without explicit configuration should be disabled");
    check(missing.setEnabled(QStringLiteral("Sample"), true)
              && missing.setEnabled(QStringLiteral("Disabled Plugin"), false)
              && missing.save(&error),
          "plugin enablement should save atomically");

    PluginEnablementConfig loaded(path);
    check(loaded.load(&error) && error.isEmpty(),
          "saved plugin enablement XML should load");
    check(loaded.isEnabled(QStringLiteral("sample"))
              && !loaded.isEnabled(QStringLiteral("Disabled Plugin"))
              && loaded.enabledPlugins()
                     == QStringList{QStringLiteral("Sample")},
          "enablement should be case-insensitive and explicit");

    QFile malformed(path);
    check(malformed.open(QFile::WriteOnly | QFile::Truncate),
          "malformed enablement fixture should open");
    malformed.write("<NotepadPlus><Plugins>");
    malformed.close();
    PluginEnablementConfig invalid(path);
    check(!invalid.load(&error) && invalid.enabledPlugins().isEmpty(),
          "malformed XML should disable every plugin");
}

void testArtifactsAndModel()
{
    QTemporaryDir temporary;
    check(temporary.isValid(), "temporary plugin root should exist");
    const QString expected = PluginArtifactResolver::binaryPath(
        temporary.path(), QStringLiteral("Sample"));
#if defined(Q_OS_WIN)
    check(expected.endsWith(QStringLiteral("/Sample/Sample.dll")),
          "Windows should retain original plugin layout");
#elif defined(Q_OS_MAC)
    check(expected.endsWith(QStringLiteral("/Sample/macos/Sample.dylib")),
          "macOS should use the platform directory");
#else
    check(expected.endsWith(QStringLiteral("/Sample/linux/Sample.so")),
          "Linux should use the platform directory");
#endif
    QDir().mkpath(QFileInfo(expected).absolutePath());
    QFile binary(expected);
    binary.open(QFile::WriteOnly);
    binary.write("test");
    binary.close();
    writeReceipt(temporary.path(), QStringLiteral("1.5"));

    QString error;
    const PluginCatalog catalog =
        PluginCatalog::fromJson(catalogJson(), &error);
    PluginAdminModel model(
        temporary.path(), catalog,
        PluginVersion(QStringLiteral("8.4.6")));
    check(model.installedItems().size() == 1,
          "installed plugin should be discovered");
    check(model.updateItems().size() == 1,
          "older managed plugin should have an update");
    check(model.availableItems().isEmpty(),
          "installed plugin should not also be available");
    check(model.incompatibleItems().isEmpty(),
          "mapped old plugin should be compatible");
    check(!model.installedItems().first().enabled,
          "an installed plugin without configuration should be disabled");

    const QString enablementPath =
        PluginEnablementConfig::filePathForConfigDirectory(temporary.path());
    PluginEnablementConfig enablement(enablementPath);
    check(enablement.setEnabled(QStringLiteral("Sample"), true)
              && enablement.save(&error),
          "model enablement fixture should save");
    PluginAdminModel enabledModel(
        temporary.path(), catalog,
        PluginVersion(QStringLiteral("8.4.6")), enablementPath);
    check(enabledModel.installedItems().first().enabled,
          "the model should expose explicitly enabled plugins");
    check(enabledModel.setPluginEnabled(
              QStringLiteral("Sample"), false, &error),
          "the model should persist enablement changes");
    PluginEnablementConfig disabled(enablementPath);
    check(disabled.load(&error)
              && !disabled.isEnabled(QStringLiteral("Sample")),
          "model changes should round-trip through XML");

    const PluginCatalog incompatibleCatalog =
        PluginCatalog::fromJson(
            catalogJson(QStringLiteral("[9.0,]")), &error);
    writeReceipt(temporary.path(), QStringLiteral("2.0"));
    PluginAdminModel incompatible(
        temporary.path(), incompatibleCatalog,
        PluginVersion(QStringLiteral("8.4.6")));
    check(incompatible.incompatibleItems().size() == 1,
          "installed exact version outside host range should be incompatible");

    writeReceipt(temporary.path(), QStringLiteral("1.5"));
    PluginAdminModel incompatibleOld(
        temporary.path(), catalog,
        PluginVersion(QStringLiteral("8.5")));
    check(incompatibleOld.incompatibleItems().size() == 1,
          "an incompatible old plugin should remain visible");
    check(incompatibleOld.updateItems().size() == 1,
          "an incompatible old plugin should offer a compatible update");
}

void testCrossPlatformArtifactDiscovery()
{
    QTemporaryDir temporary;
    check(temporary.isValid(),
          "cross-platform artifact root should exist");
    const QString binaryPath =
        PluginArtifactResolver::crossPlatformBinaryPath(
            temporary.path(), QStringLiteral("PortableSample"));
    QDir().mkpath(QFileInfo(binaryPath).absolutePath());
    QFile binary(binaryPath);
    check(binary.open(QFile::WriteOnly),
          "cross-platform test binary should open");
    binary.write("test");
    binary.close();

    const QVector<PluginArtifact> artifacts =
        PluginArtifactResolver::discoverCrossPlatform(temporary.path());
    check(artifacts.size() == 1
              && artifacts.first().folderName
                     == QStringLiteral("PortableSample")
              && artifacts.first().binaryPath == binaryPath,
          "ABI v1 platform directory should be discovered");

    PluginAdminModel model(
        temporary.path(), PluginCatalog(),
        PluginVersion(QStringLiteral("8.4.6")));
    check(model.installedItems().size() == 1
              && model.installedItems().first().folderName
                     == QStringLiteral("PortableSample"),
          "Plugins Admin should list an ABI v1-only plugin");
}

void testUpdatePlan()
{
    QTemporaryDir temporary;
    PluginUpdatePlan plan;
    plan.applicationPath = QStringLiteral("/opt/notepad++/notepad++");
    plan.pluginRoot = QStringLiteral("/opt/notepad++/plugins");
    PluginOperation operation;
    operation.type = PluginOperationType::Install;
    operation.folderName = QStringLiteral("Sample");
    operation.version = QStringLiteral("2.0");
    operation.repository = QStringLiteral("https://example.invalid/Sample.zip");
    operation.packageSha256 =
        QStringLiteral("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    plan.operations.append(operation);
    QString error;
    check(plan.isValid(&error), "valid update plan should pass");

    const QString path =
        QDir(temporary.path()).filePath(QStringLiteral("plan.json"));
    check(plan.write(path, &error), "update plan should be written");
    const PluginUpdatePlan loaded = PluginUpdatePlan::read(path, &error);
    check(error.isEmpty() && loaded.operations.size() == 1,
          "update plan should round-trip");
    check(loaded.operations.first().folderName == QStringLiteral("Sample"),
          "round-tripped plan should preserve folder");

    plan.operations[0].folderName = QStringLiteral("../escape");
    check(!plan.isValid(&error),
          "update plan should reject path traversal");
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    testVersionsAndCatalog();
    testBundledWindowsCatalogs();
    testPluginEnablementConfig();
    testArtifactsAndModel();
    testCrossPlatformArtifactDiscovery();
    testUpdatePlan();
    if (failures == 0)
        std::cout << "Plugin admin tests passed\n";
    return failures == 0 ? 0 : 1;
}
