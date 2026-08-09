#include "WinControls/FunctionList/functionParser.h"

#include <QCoreApplication>
#include <QDir>
#include <QDomDocument>
#include <QFile>

#include <cstdlib>
#include <iostream>

#ifndef NPP_FUNCTION_LIST_SOURCE
#error NPP_FUNCTION_LIST_SOURCE is required
#endif

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}

bool contains(const QList<FunctionListEntry>& entries, const QString& name)
{
    for (const FunctionListEntry& entry : entries)
        if (entry.name.contains(name))
            return true;
    return false;
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    const QString directory =
        QDir::cleanPath(QStringLiteral(NPP_FUNCTION_LIST_SOURCE));
    const QStringList parserFiles =
        QDir(directory).entryList({QStringLiteral("*.xml")}, QDir::Files);
    require(parserFiles.size() == 34,
            "original v8.4.6 function list corpus is incomplete");
    for (const QString& parserFile : parserFiles) {
        QFile file(QDir(directory).filePath(parserFile));
        QDomDocument document;
        require(file.open(QIODevice::ReadOnly) && document.setContent(&file),
                "function list XML is invalid");
    }

    const QList<FunctionListEntry> python = FunctionListParser::parse(
        QStringLiteral(
            "# def ignored():\n"
            "def top_level(value):\n"
            "    return value\n"
            "class Example:\n"
            "    def method(self):\n"
            "        pass\n"),
        QStringLiteral("Python"), {directory});
    require(contains(python, QStringLiteral("top_level")),
            "Python top-level function was not parsed");
    require(contains(python, QStringLiteral("Example")),
            "Python class was not parsed");
    require(contains(python, QStringLiteral("method")),
            "Python class method was not parsed");
    require(!contains(python, QStringLiteral("ignored")),
            "commented Python function was parsed");

    const QList<FunctionListEntry> cpp = FunctionListParser::parse(
        QStringLiteral(
            "class Example {\n"
            "public:\n"
            "    void method() {}\n"
            "};\n"
            "void topLevel() {}\n"),
        QStringLiteral("C++"), {directory});
    require(contains(cpp, QStringLiteral("Example")),
            "C++ class was not parsed");
    require(contains(cpp, QStringLiteral("method")),
            "C++ class method was not parsed");
    require(contains(cpp, QStringLiteral("topLevel")),
            "C++ top-level function was not parsed");

    require(FunctionListParser::parserFileName(QStringLiteral("C++")) ==
            QStringLiteral("cpp.xml"),
            "C++ parser mapping is incorrect");
    require(FunctionListParser::parse(
                QStringLiteral("anything"), QStringLiteral("unknown"),
                {directory}).isEmpty(),
            "unknown language should not select a parser");

    std::cout << "Function list parser tests passed." << std::endl;
    return 0;
}
