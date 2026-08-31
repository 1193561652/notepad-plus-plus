#ifndef AUTOCOMPLETIONPARSER_H
#define AUTOCOMPLETIONPARSER_H

#include <QString>
#include <QStringList>
#include <QVector>

struct AutoCompletionEntry {
    QString name;
    QString returnValue;
    QStringList parameters;

    QString apiText() const;
};

class AutoCompletionParser
{
public:
    static QVector<AutoCompletionEntry> loadLanguage(
        const QString& languageName,
        const QString& userPath,
        const QString& sharedAutoCompletionPath);
    static QVector<AutoCompletionEntry> parseFile(const QString& filePath);
};

#endif
