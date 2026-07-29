#include "FileAssociationModel.h"

#include <QRegularExpression>

const QVector<FileAssociationCategory>& fileAssociationCategories()
{
    static const QVector<FileAssociationCategory> categories = {
        {"Notepad", {".txt", ".log"}},
        {"ms ini/inf", {".ini", ".inf"}},
        {"c, c++, objc", {
            ".h", ".hh", ".hpp", ".hxx", ".c", ".cpp", ".cxx", ".cc",
            ".m", ".mm", ".vcxproj", ".vcproj", ".props", ".vsprops",
            ".manifest"
        }},
        {"java, c#, pascal", {".java", ".cs", ".pas", ".pp", ".inc"}},
        {"web script", {
            ".html", ".htm", ".shtml", ".shtm", ".hta", ".asp", ".aspx",
            ".css", ".js", ".json", ".jsm", ".jsp", ".php", ".php3",
            ".php4", ".php5", ".phps", ".phpt", ".phtml", ".xml", ".xhtml",
            ".xht", ".xul", ".kml", ".xaml", ".xsml"
        }},
        {"public script", {
            ".sh", ".bsh", ".bash", ".bat", ".cmd", ".nsi", ".nsh", ".lua",
            ".pl", ".pm", ".py"
        }},
        {"property script", {".rc", ".as", ".mx", ".vb", ".vbs"}},
        {"fortran, TeX, SQL", {
            ".f", ".for", ".f90", ".f95", ".f2k", ".tex", ".sql"
        }},
        {"misc", {".nfo", ".mak"}},
        {"customize", {}}
    };
    return categories;
}

QString normalizeFileAssociationExtension(const QString& extension)
{
    QString normalized = extension.trimmed().toLower();
    if (normalized.isEmpty())
        return {};
    if (!normalized.startsWith('.'))
        normalized.prepend('.');
    static const QRegularExpression valid(QStringLiteral("^\\.[a-z0-9_+-]{1,17}$"));
    return valid.match(normalized).hasMatch() ? normalized : QString();
}
