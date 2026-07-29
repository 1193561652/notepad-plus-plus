#include <QApplication>
#include <QByteArray>
#include <Qsci/qsciscintilla.h>

#include "../src/ScintillaComponent/ScintillaTextSearch.h"

#include <iostream>

namespace {

bool expect(bool condition, const char* message)
{
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}

long search(QsciScintilla& editor, const QByteArray& pattern,
            long start, long end, int extraFlags = 0)
{
    editor.SendScintilla(QsciScintillaBase::SCI_SETSEARCHFLAGS,
                         QsciScintillaBase::SCFIND_REGEXP | extraFlags);
    editor.SendScintilla(QsciScintillaBase::SCI_SETTARGETSTART, start);
    editor.SendScintilla(QsciScintillaBase::SCI_SETTARGETEND, end);
    return editor.SendScintilla(QsciScintillaBase::SCI_SEARCHINTARGET,
                                pattern.size(), pattern.constData());
}

} // namespace

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QsciScintilla editor;
    editor.setUtf8(true);
    editor.setText(QStringLiteral("foo bar\r\nfoobar\nalpha\nbeta"));

    const long documentLength =
        editor.SendScintilla(QsciScintillaBase::SCI_GETLENGTH);

    bool ok = true;
    const long lookbehind = search(editor, QByteArray("(?<=foo)bar"),
                                   0, documentLength);
    ok &= expect(lookbehind == 12,
                 "Boost lookbehind should find 'bar' in 'foobar'");

    const long newline = search(editor, QByteArray("\\R"),
                                0, documentLength);
    ok &= expect(newline == 7, "Boost \\R should match CRLF");
    ok &= expect(editor.SendScintilla(
                     QsciScintillaBase::SCI_GETTARGETEND) == 9,
                 "Boost \\R should consume CRLF as one match");

    const long capture = search(editor, QByteArray("(alpha)\\R(beta)"),
                                0, documentLength);
    ok &= expect(capture >= 0, "capturing expression should match");
    if (capture >= 0) {
        const QByteArray replacement("$2/$1");
        editor.SendScintilla(QsciScintillaBase::SCI_REPLACETARGETRE,
                             replacement.size(), replacement.constData());
        ok &= expect(editor.text().endsWith(QStringLiteral("beta/alpha")),
                     "Boost replacement should expand capture groups");
    }

    QString plainText = QStringLiteral("foo\r\nbar foo\nbar");
    const QList<ScintillaTextMatch> matches =
        ScintillaTextSearch::findAll(
            plainText, QStringLiteral("(?<=foo)\\Rbar"),
            true, true, false, false);
    ok &= expect(matches.size() == 2,
                 "plain-text routes should use Boost.Regex semantics");
    const int replaced = ScintillaTextSearch::replaceAll(
        plainText, QStringLiteral("(foo)\\R(bar)"),
        QStringLiteral("$2/$1"), true, true, false, false);
    ok &= expect(replaced == 2
                 && plainText == QStringLiteral("bar/foo bar/foo"),
                 "plain-text replacement should use Boost capture syntax");

    const QList<ScintillaTextMatch> emptyMatches =
        ScintillaTextSearch::findAll(
            QStringLiteral("ab"), QStringLiteral("(?=.)"),
            true, true, false, false);
    ok &= expect(emptyMatches.size() == 2,
                 "Boost empty matches should advance by character");
    const QList<ScintillaTextMatch> dotAllMatches =
        ScintillaTextSearch::findAll(
            QStringLiteral("a\nb"), QStringLiteral("a.b"),
            true, true, false, true);
    ok &= expect(dotAllMatches.size() == 1,
                 "dot-matches-newline should use the Scintilla extension flag");
    const QList<ScintillaTextMatch> utf8ColumnMatches =
        ScintillaTextSearch::findAll(
            QString::fromUtf8("\xC3\xA9 foo"), QStringLiteral("foo"),
            false, true, false, false);
    ok &= expect(
        utf8ColumnMatches.size() == 1
            && utf8ColumnMatches.first().byteColumn == 3,
        "Finder match columns should preserve UTF-8 byte offsets");

    if (ok)
        std::cout << "Boost.Regex Scintilla integration passed\n";
    return ok ? 0 : 1;
}
