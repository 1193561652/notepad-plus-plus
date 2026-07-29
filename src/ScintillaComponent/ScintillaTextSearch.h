#pragma once

#include <QList>
#include <QString>

struct ScintillaTextMatch
{
    int byteStart = 0;
    int byteLength = 0;
    int line = 0;
    int byteColumn = 0;
    QString lineText;
};

class ScintillaTextSearch
{
public:
    static QList<ScintillaTextMatch> findAll(
        const QString& text, const QString& searchText,
        bool regex, bool matchCase, bool wholeWord,
        bool dotMatchesNewline, bool* valid = nullptr);

    static int replaceAll(
        QString& text, const QString& searchText, const QString& replacement,
        bool regex, bool matchCase, bool wholeWord,
        bool dotMatchesNewline);

    static bool replaceWholeMatch(
        QString& text, const QString& searchText, const QString& replacement,
        bool regex, bool matchCase, bool wholeWord,
        bool dotMatchesNewline);
};
