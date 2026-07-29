#pragma once

#include <QList>
#include <QString>
#include <QStringList>

struct FunctionListEntry
{
    QString name;
    int line = 0;
};

class FunctionListParser
{
public:
    static QList<FunctionListEntry> parse(const QString& text,
                                          const QString& language,
                                          const QStringList& searchDirectories);
    static QString parserFileName(const QString& language);
};
