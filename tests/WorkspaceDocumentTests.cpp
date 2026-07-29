#include "WinControls/ProjectPanel/WorkspaceDocument.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <cstdio>

static bool check(bool condition, const char* message)
{
    if (!condition)
        std::fprintf(stderr, "FAILED: %s\n", message);
    return condition;
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temporary;
    bool ok = check(temporary.isValid(), "Temporary directory is unavailable");
    QDir root(temporary.path());
    root.mkpath(QStringLiteral("src"));
    QFile source(root.filePath(QStringLiteral("src/main.cpp")));
    ok &= check(source.open(QFile::WriteOnly) &&
                source.write("int main() {}") > 0,
                "Could not create workspace member");
    source.close();

    const QString inputPath = root.filePath(QStringLiteral("sample.workspace"));
    QFile input(inputPath);
    ok &= check(input.open(QFile::WriteOnly | QFile::Text),
                "Could not create workspace XML");
    input.write(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<NotepadPlus><Project name=\"App\"><Folder name=\"Source\">"
        "<File name=\"src/main.cpp\"/></Folder></Project></NotepadPlus>");
    input.close();

    WorkspaceDocument document;
    QString error;
    ok &= check(document.load(inputPath, &error),
                qPrintable(QStringLiteral("Workspace load failed: %1").arg(error)));
    const QString expected =
        QFileInfo(root.filePath(QStringLiteral("src/main.cpp"))).absoluteFilePath();
    ok &= check(document.projects().size() == 1 &&
                document.projects().first().children.size() == 1 &&
                document.allFiles() == QStringList{expected},
                "Workspace hierarchy or relative path resolution is wrong");

    const QString outputPath =
        root.filePath(QStringLiteral("roundtrip.workspace"));
    ok &= check(document.save(outputPath, &error),
                qPrintable(QStringLiteral("Workspace save failed: %1").arg(error)));
    WorkspaceDocument roundTrip;
    const bool roundTripLoaded = roundTrip.load(outputPath, &error);
    if (!roundTripLoaded || roundTrip.allFiles() != QStringList{expected}) {
        std::fprintf(stderr, "DETAIL: load=%d error=%s expected=%s actual=%s\n",
                     roundTripLoaded, qPrintable(error), qPrintable(expected),
                     qPrintable(roundTrip.allFiles().join("|")));
    }
    ok &= check(roundTripLoaded &&
                roundTrip.allFiles() == QStringList{expected},
                "Workspace round trip changed project members");
    QFile output(outputPath);
    const bool outputOpened = output.open(QFile::ReadOnly);
    const QByteArray outputBytes = outputOpened ? output.readAll() : QByteArray();
    if (!outputBytes.contains("src/main.cpp"))
        std::fprintf(stderr, "DETAIL XML: %s\n", outputBytes.constData());
    ok &= check(outputOpened &&
                (outputBytes.contains("src/main.cpp") ||
                 outputBytes.contains("src\\main.cpp")),
                "Workspace save did not preserve a relative member path");
    return ok ? 0 : 1;
}
