#include "MISC/ClosedFileHistory.h"

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
    require(file.open(QFile::WriteOnly), "could not create history test file");
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temporary;
    require(temporary.isValid(), "temporary directory is invalid");
    QDir root(temporary.path());
    const QString first = root.filePath(QStringLiteral("first.txt"));
    const QString second = root.filePath(QStringLiteral("second.txt"));
    const QString third = root.filePath(QStringLiteral("third.txt"));
    touch(first);
    touch(second);
    touch(third);

    ClosedFileHistory history(2);
    history.remember(first);
    history.remember(second);
    history.remember(first);
    require(history.size() == 2, "duplicate path was not moved in place");
    require(history.takeNextExisting() == QFileInfo(first).absoluteFilePath(),
            "history is not LIFO");

    history.remember(second);
    history.remember(third);
    history.remember(first);
    require(history.size() == 2, "history capacity was not enforced");
    QFile::remove(first);
    require(history.takeNextExisting() == QFileInfo(third).absoluteFilePath(),
            "missing file was not skipped");
    require(history.isEmpty(), "history was not consumed");

    std::cout << "Closed file history tests passed." << std::endl;
    return 0;
}
