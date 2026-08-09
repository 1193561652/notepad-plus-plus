#include "MISC/RegExt/FileAssociationModel.h"

#include <QSet>

#include <iostream>

namespace
{
bool expect(bool condition, const char* message)
{
    if (!condition)
        std::cerr << "FAILED: " << message << '\n';
    return condition;
}
}

int main()
{
    bool ok = true;
    const QVector<FileAssociationCategory>& categories =
        fileAssociationCategories();
    ok &= expect(categories.size() == 10,
                 "v8.4.6 file association categories must be preserved");
    ok &= expect(categories.first().name == QStringLiteral("Notepad"),
                 "first category must match v8.4.6");
    ok &= expect(categories.last().extensions.isEmpty(),
                 "customize category must accept user input");

    QSet<QString> extensions;
    for (const FileAssociationCategory& category : categories) {
        for (const QString& extension : category.extensions) {
            ok &= expect(extension == normalizeFileAssociationExtension(extension),
                         "built-in extension must be normalized");
            ok &= expect(!extensions.contains(extension),
                         "built-in extensions must not be duplicated");
            extensions.insert(extension);
        }
    }
    ok &= expect(extensions.contains(QStringLiteral(".txt")),
                 "Notepad .txt association must be present");
    ok &= expect(extensions.contains(QStringLiteral(".vcxproj")),
                 "C/C++ project association must be present");
    ok &= expect(normalizeFileAssociationExtension(QStringLiteral(" CPP ")) ==
                     QStringLiteral(".cpp"),
                 "custom extensions must be trimmed, lower-cased and dotted");
    ok &= expect(normalizeFileAssociationExtension(QStringLiteral("../txt")).isEmpty(),
                 "paths must not be accepted as extensions");
    ok &= expect(normalizeFileAssociationExtension(QStringLiteral("*.txt")).isEmpty(),
                 "wildcards must not be accepted as extensions");
    return ok ? 0 : 1;
}
