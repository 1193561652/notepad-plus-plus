#include "../src/MISC/AutoCompletionParser.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

static bool require(bool condition, const char* message)
{
    if (!condition)
        qCritical("%s", message);
    return condition;
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir directory;
    if (!require(directory.isValid(), "temporary directory"))
        return 1;
    QDir root(directory.path());
    root.mkpath(QStringLiteral("autoCompletion"));
    QFile xml(root.filePath(QStringLiteral("autoCompletion/cpp.xml")));
    if (!xml.open(QIODevice::WriteOnly | QIODevice::Text))
        return 1;
    xml.write(
        "<NotepadPlus><AutoComplete language=\"C++\">"
        "<KeyWord name=\"alignas\"/>"
        "<KeyWord name=\"printf\" func=\"yes\">"
        "<Overload retVal=\"int\"><Param name=\"format\"/>"
        "<Param name=\"...\"/></Overload></KeyWord>"
        "</AutoComplete></NotepadPlus>");
    xml.close();

    const QVector<AutoCompletionEntry> entries =
        AutoCompletionParser::loadLanguage(
            QStringLiteral("cpp"), directory.path(), QString());
    if (!require(entries.size() == 2, "entry count") ||
        !require(entries.at(0).apiText() == QStringLiteral("alignas"),
                 "keyword") ||
        !require(entries.at(1).apiText() ==
                     QStringLiteral("printf(format, ...)"),
                 "function signature"))
        return 1;
    return 0;
}
