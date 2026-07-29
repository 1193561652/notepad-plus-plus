#include "ScintillaTextSearch.h"

#include "../../third_party/boostregex/BoostRegexSearch.h"

#include <QByteArray>
#include <Qsci/qsciscintilla.h>

namespace {

int searchFlags(bool regex, bool matchCase, bool wholeWord,
                bool dotMatchesNewline)
{
    unsigned int flags = 0;
    if (matchCase)
        flags |= QsciScintillaBase::SCFIND_MATCHCASE;
    if (wholeWord)
        flags |= QsciScintillaBase::SCFIND_WHOLEWORD;
    if (regex) {
        flags |= QsciScintillaBase::SCFIND_REGEXP;
        flags |= SCFIND_REGEXP_EMPTYMATCH_ALL;
        flags |= SCFIND_REGEXP_SKIPCRLFASONE;
        if (dotMatchesNewline)
            flags |= SCFIND_REGEXP_DOTMATCHESNL;
    }
    return static_cast<int>(flags);
}

void prepare(QsciScintilla& editor, const QString& text, int flags)
{
    editor.setUtf8(true);
    editor.setText(text);
    editor.SendScintilla(QsciScintillaBase::SCI_SETSEARCHFLAGS, flags);
}

long findTarget(QsciScintilla& editor, const QByteArray& pattern,
                long start, long end)
{
    editor.SendScintilla(QsciScintillaBase::SCI_SETTARGETSTART, start);
    editor.SendScintilla(QsciScintillaBase::SCI_SETTARGETEND, end);
    return editor.SendScintilla(QsciScintillaBase::SCI_SEARCHINTARGET,
                                pattern.size(), pattern.constData());
}

long nextPosition(QsciScintilla& editor, long matchStart, long matchEnd,
                  long documentLength)
{
    if (matchEnd > matchStart)
        return matchEnd;
    if (matchStart >= documentLength)
        return documentLength + 1;
    return editor.SendScintilla(QsciScintillaBase::SCI_POSITIONAFTER,
                                matchStart);
}

} // namespace

QList<ScintillaTextMatch> ScintillaTextSearch::findAll(
    const QString& text, const QString& searchText,
    bool regex, bool matchCase, bool wholeWord,
    bool dotMatchesNewline, bool* valid)
{
    QList<ScintillaTextMatch> matches;
    if (valid)
        *valid = true;
    if (searchText.isEmpty())
        return matches;

    QsciScintilla editor;
    prepare(editor, text, searchFlags(regex, matchCase, wholeWord,
                                      dotMatchesNewline));
    const QByteArray pattern = searchText.toUtf8();
    const long documentLength =
        editor.SendScintilla(QsciScintillaBase::SCI_GETLENGTH);
    long position = 0;
    while (position <= documentLength) {
        const long found = findTarget(editor, pattern, position,
                                      documentLength);
        if (found < 0) {
            if (found < -1 && valid)
                *valid = false;
            break;
        }
        const long end =
            editor.SendScintilla(QsciScintillaBase::SCI_GETTARGETEND);
        const int line = static_cast<int>(editor.SendScintilla(
            QsciScintillaBase::SCI_LINEFROMPOSITION, found));

        ScintillaTextMatch match;
        match.byteStart = static_cast<int>(found);
        match.byteLength = static_cast<int>(end - found);
        match.line = line;
        const long lineStart = editor.SendScintilla(
            QsciScintillaBase::SCI_POSITIONFROMLINE, line);
        match.byteColumn = static_cast<int>(found - lineStart);
        match.lineText = editor.text(line);
        while (!match.lineText.isEmpty()
               && (match.lineText.endsWith('\n')
                   || match.lineText.endsWith('\r')))
            match.lineText.chop(1);
        matches.append(match);
        position = nextPosition(editor, found, end, documentLength);
    }
    return matches;
}

int ScintillaTextSearch::replaceAll(
    QString& text, const QString& searchText, const QString& replacement,
    bool regex, bool matchCase, bool wholeWord, bool dotMatchesNewline)
{
    if (searchText.isEmpty())
        return 0;

    QsciScintilla editor;
    prepare(editor, text, searchFlags(regex, matchCase, wholeWord,
                                      dotMatchesNewline));
    const QByteArray pattern = searchText.toUtf8();
    const QByteArray replacementBytes = replacement.toUtf8();
    long documentLength =
        editor.SendScintilla(QsciScintillaBase::SCI_GETLENGTH);
    long position = 0;
    int count = 0;
    while (position <= documentLength) {
        const long found = findTarget(editor, pattern, position,
                                      documentLength);
        if (found < -1)
            return -1;
        if (found < 0)
            break;

        const long matchEnd =
            editor.SendScintilla(QsciScintillaBase::SCI_GETTARGETEND);
        const bool emptyMatch = matchEnd == found;
        const long replacementLength = editor.SendScintilla(
            regex ? QsciScintillaBase::SCI_REPLACETARGETRE
                  : QsciScintillaBase::SCI_REPLACETARGET,
            replacementBytes.size(), replacementBytes.constData());
        documentLength += replacementLength - (matchEnd - found);
        ++count;
        const long replacementEnd = found + replacementLength;
        position = emptyMatch
            ? nextPosition(editor, replacementEnd, replacementEnd,
                           documentLength)
            : replacementEnd;
    }
    text = editor.text();
    return count;
}

bool ScintillaTextSearch::replaceWholeMatch(
    QString& text, const QString& searchText, const QString& replacement,
    bool regex, bool matchCase, bool wholeWord, bool dotMatchesNewline)
{
    QsciScintilla editor;
    prepare(editor, text, searchFlags(regex, matchCase, wholeWord,
                                      dotMatchesNewline));
    const QByteArray pattern = searchText.toUtf8();
    const long documentLength =
        editor.SendScintilla(QsciScintillaBase::SCI_GETLENGTH);
    const long found = findTarget(editor, pattern, 0, documentLength);
    const long end = found >= 0
        ? editor.SendScintilla(QsciScintillaBase::SCI_GETTARGETEND)
        : -1;
    if (found != 0 || end != documentLength)
        return false;

    const QByteArray replacementBytes = replacement.toUtf8();
    editor.SendScintilla(
        regex ? QsciScintillaBase::SCI_REPLACETARGETRE
              : QsciScintillaBase::SCI_REPLACETARGET,
        replacementBytes.size(), replacementBytes.constData());
    text = editor.text();
    return true;
}
