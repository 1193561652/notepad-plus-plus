#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

struct CommandLineOptions
{
    bool showHelp = false;
    bool multiInstance = false;
    bool noPlugin = false;
    bool readOnly = false;
    bool noSession = false;
    bool noTabBar = false;
    bool systemTray = false;
    bool showLoadingTime = false;
    bool alwaysOnTop = false;
    bool openSession = false;
    bool recursive = false;
    bool exportFunctionList = false;
    bool quickPrint = false;
    bool notepadStyle = false;
    bool openFoldersAsWorkspace = false;
    bool monitorFiles = false;

    qint64 line = -1;
    qint64 column = -1;
    qint64 position = -1;
    int windowX = 0;
    int windowY = 0;
    bool hasWindowX = false;
    bool hasWindowY = false;

    QString language;
    QString localizationCode;
    QString userDefinedLanguage;
    QString pluginMessage;
    QString settingsDirectory;
    QString titleAddition;
    QString quote;
    int quoteType = -1;
    int ghostTypingSpeed = -1;
    QStringList paths;
    QStringList unknownOptions;

    bool hasWindowPosition() const { return hasWindowX && hasWindowY; }
    bool hasFiles() const { return !paths.isEmpty(); }

    QJsonObject toJson() const;
    static CommandLineOptions fromJson(const QJsonObject& object);
};

class CommandLineParser
{
public:
    static CommandLineOptions parse(const QStringList& arguments,
                                    const QString& workingDirectory);
    static QString helpText();
    static QString localizationFileName(const QString& code);
};
