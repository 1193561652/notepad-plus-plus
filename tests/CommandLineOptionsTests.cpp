#include "CommandLineOptions.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonObject>
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
    file.write("test");
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temporary;
    require(temporary.isValid(), "temporary directory is invalid");
    QDir root(temporary.path());
    root.mkpath(QStringLiteral("nested"));
    touch(root.filePath(QStringLiteral("one.cpp")));
    touch(root.filePath(QStringLiteral("nested/two.cpp")));
    touch(root.filePath(QStringLiteral("nested/skip.txt")));

    const CommandLineOptions parsed = CommandLineParser::parse({
        QStringLiteral("notepad++"),
        QStringLiteral("-multiInst"),
        QStringLiteral("-noPlugin"),
        QStringLiteral("-nosession"),
        QStringLiteral("-notabbar"),
        QStringLiteral("-ro"),
        QStringLiteral("-monitor"),
        QStringLiteral("-alwaysOnTop"),
        QStringLiteral("-r"),
        QStringLiteral("-lpython"),
        QStringLiteral("-udl=My UDL"),
        QStringLiteral("-Lzh-cn"),
        QStringLiteral("-n12"),
        QStringLiteral("-c7"),
        QStringLiteral("-p300"),
        QStringLiteral("-x40"),
        QStringLiteral("-y50"),
        QStringLiteral("-settingsDir=settings"),
        QStringLiteral("-titleAdd=Portable"),
        QStringLiteral("-pluginMessage=hello"),
        QStringLiteral("*.cpp")
    }, temporary.path());

    require(parsed.multiInstance && parsed.noPlugin && parsed.noSession,
            "boolean options were not parsed");
    require(parsed.noTabBar && parsed.readOnly && parsed.monitorFiles,
            "file behavior options were not parsed");
    require(parsed.alwaysOnTop && parsed.recursive,
            "window/recursive options were not parsed");
    require(parsed.language == QStringLiteral("python"),
            "language was not parsed");
    require(parsed.userDefinedLanguage == QStringLiteral("My UDL"),
            "UDL was not parsed");
    require(parsed.localizationCode == QStringLiteral("zh-cn"),
            "localization was not parsed");
    require(parsed.line == 12 && parsed.column == 7 && parsed.position == 300,
            "document positions were not parsed");
    require(parsed.hasWindowPosition() && parsed.windowX == 40 &&
            parsed.windowY == 50, "window position was not parsed");
    require(parsed.paths.size() == 2,
            "recursive wildcard expansion did not find both files");
    require(parsed.settingsDirectory ==
            root.filePath(QStringLiteral("settings")),
            "settings directory was not made absolute");

    const CommandLineOptions portableAlias = CommandLineParser::parse({
        QStringLiteral("notepad++"),
        QStringLiteral("--settings-dir"),
        QStringLiteral("portable settings")
    }, temporary.path());
    require(portableAlias.settingsDirectory ==
            root.filePath(QStringLiteral("portable settings")),
            "cross-platform settings directory alias was not parsed");

    const CommandLineOptions homeSettings = CommandLineParser::parse({
        QStringLiteral("notepad++"),
        QStringLiteral("--settings-dir=~/npp-settings")
    }, temporary.path());
    require(homeSettings.settingsDirectory ==
            QDir::home().filePath(QStringLiteral("npp-settings")),
            "home-relative settings directory was not expanded");

    const CommandLineOptions roundTrip =
        CommandLineOptions::fromJson(parsed.toJson());
    require(roundTrip.paths == parsed.paths &&
            roundTrip.userDefinedLanguage == parsed.userDefinedLanguage &&
            roundTrip.position == parsed.position,
            "IPC JSON round trip changed command-line state");

    const CommandLineOptions notepadStyle = CommandLineParser::parse({
        QStringLiteral("notepad++"),
        QStringLiteral("-notepadStyleCmdline"),
        QStringLiteral("a"),
        QStringLiteral("file")
    }, temporary.path());
    require(notepadStyle.paths.size() == 1 &&
            notepadStyle.paths.first().endsWith(QStringLiteral("a file.txt")),
            "Notepad-style command line was not joined");
    require(notepadStyle.multiInstance && notepadStyle.noSession &&
            notepadStyle.noTabBar,
            "Notepad-style overrides were not applied");

    require(CommandLineParser::localizationFileName(QStringLiteral("pt-BR")) ==
            QStringLiteral("brazilian_portuguese.xml"),
            "Brazilian Portuguese localization mapping failed");
    require(CommandLineParser::localizationFileName(QStringLiteral("fr-CA")) ==
            QStringLiteral("french.xml"),
            "regional localization fallback failed");

    std::cout << "Command-line option tests passed." << std::endl;
    return 0;
}
