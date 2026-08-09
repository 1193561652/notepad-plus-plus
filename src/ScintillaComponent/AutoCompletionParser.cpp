#include "AutoCompletionParser.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QSet>
#include <QXmlStreamReader>

QString AutoCompletionEntry::apiText() const
{
    if (parameters.isEmpty())
        return name;
    return QStringLiteral("%1(%2)").arg(name, parameters.join(QStringLiteral(", ")));
}

QVector<AutoCompletionEntry> AutoCompletionParser::loadLanguage(
    const QString& languageName,
    const QString& userPath,
    const QString& applicationPath)
{
    QStringList baseNames;
    baseNames << languageName.toLower();
    static const QMap<QString, QString> aliases = {
        {QStringLiteral("c"), QStringLiteral("cpp")},
        {QStringLiteral("cs"), QStringLiteral("csharp")},
        {QStringLiteral("javascript"), QStringLiteral("javascript")},
        {QStringLiteral("typescript"), QStringLiteral("typescript")},
        {QStringLiteral("visual basic"), QStringLiteral("vb")}
    };
    const QString alias = aliases.value(languageName.toLower());
    if (!alias.isEmpty() && !baseNames.contains(alias))
        baseNames << alias;

    const QStringList roots = {
        QDir(userPath).filePath(QStringLiteral("autoCompletion")),
        QDir(applicationPath).filePath(QStringLiteral("autoCompletion"))
    };
    for (const QString& root : roots) {
        for (const QString& baseName : baseNames) {
            const QString filePath =
                QDir(root).filePath(baseName + QStringLiteral(".xml"));
            if (QFileInfo(filePath).isFile())
                return parseFile(filePath);
        }
    }
    return {};
}

QVector<AutoCompletionEntry> AutoCompletionParser::parseFile(
    const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QVector<AutoCompletionEntry> entries;
    QSet<QString> seen;
    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement() ||
            xml.name().compare(QStringLiteral("KeyWord"),
                               Qt::CaseInsensitive) != 0)
            continue;

        AutoCompletionEntry entry;
        entry.name = xml.attributes().value(QStringLiteral("name")).toString();
        const bool function =
            xml.attributes().value(QStringLiteral("func")).toString()
                .compare(QStringLiteral("yes"), Qt::CaseInsensitive) == 0;
        while (function && xml.readNextStartElement()) {
            if (xml.name().compare(QStringLiteral("Overload"),
                                   Qt::CaseInsensitive) != 0) {
                xml.skipCurrentElement();
                continue;
            }
            entry.returnValue =
                xml.attributes().value(QStringLiteral("retVal")).toString();
            while (xml.readNextStartElement()) {
                if (xml.name().compare(QStringLiteral("Param"),
                                       Qt::CaseInsensitive) == 0) {
                    entry.parameters << xml.attributes()
                        .value(QStringLiteral("name")).toString();
                    xml.skipCurrentElement();
                } else {
                    xml.skipCurrentElement();
                }
            }
            // Keep overloads as separate completion signatures.
            const QString signature = entry.apiText();
            if (!entry.name.isEmpty() && !seen.contains(signature)) {
                entries.append(entry);
                seen.insert(signature);
            }
            entry.parameters.clear();
            entry.returnValue.clear();
        }
        if (!function && !entry.name.isEmpty() && !seen.contains(entry.name)) {
            entries.append(entry);
            seen.insert(entry.name);
        }
    }
    return xml.hasError() ? QVector<AutoCompletionEntry>() : entries;
}
