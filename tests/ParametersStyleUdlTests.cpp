#include "Parameters.h"

#include <QApplication>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QTemporaryDir>

static bool expect(bool condition, const char* message)
{
    if (!condition)
        qCritical("%s", message);
    return condition;
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QTemporaryDir temporary;
    if (!expect(temporary.isValid(), "temporary settings directory"))
        return 1;
    const QString userPath = temporary.path() + QStringLiteral("/settings");
    QDir().mkpath(userPath);
    if (!QFile::copy(
            QStringLiteral(NPP_TEST_STYLERS_SOURCE),
            userPath + QStringLiteral("/stylers.xml")) ||
        !QFile::copy(
            QStringLiteral(NPP_TEST_UDL_SOURCE),
            userPath + QStringLiteral("/userDefineLang.xml")))
        return 1;

    QFile styleFile(userPath + QStringLiteral("/stylers.xml"));
    QDomDocument styleDocument;
    if (!styleFile.open(QIODevice::ReadOnly) ||
        !styleDocument.setContent(&styleFile))
        return 1;
    styleFile.close();
    styleDocument.documentElement().setAttribute(
        QStringLiteral("preserveMe"), QStringLiteral("yes"));
    if (!styleFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return 1;
    styleFile.write(styleDocument.toByteArray(2));
    styleFile.close();

    NppParameters& parameters = NppParameters::getInstance();
    if (!parameters.setUserPathOverride(userPath) || !parameters.load())
        return 1;
    const LangDesc* baan = parameters.getLangDescByName(
        QStringLiteral("baanc"));
    if (!expect(baan && !baan->keywords[8].isEmpty(),
                "langs.xml type7 keyword group"))
        return 1;
    QVector<LexerStyler>& lexers = parameters.getLexerStylers();
    if (!expect(!lexers.isEmpty() && !lexers.first().styles.isEmpty(),
                "loaded lexer styles"))
        return 1;
    lexers.first().styles.first().hasFg = true;
    lexers.first().styles.first().fgColor = QColor(18, 52, 86);
    bool userKeywordsSet = false;
    for (LexerStyler& lexer : lexers) {
        for (WordsStyle& style : lexer.styles) {
            if (!style.keywordClass.isEmpty()) {
                style.userKeywords = QStringLiteral("codex_custom_keyword");
                userKeywordsSet = true;
                break;
            }
        }
        if (userKeywordsSet) break;
    }
    if (!expect(userKeywordsSet, "keyword-capable style"))
        return 1;
    if (!expect(parameters.writeStylers(), "write stylers"))
        return 1;

    QDomDocument writtenStyles;
    if (!styleFile.open(QIODevice::ReadOnly) ||
        !writtenStyles.setContent(&styleFile))
        return 1;
    if (!expect(
            writtenStyles.documentElement().attribute(
                QStringLiteral("preserveMe")) == QStringLiteral("yes"),
            "unknown stylers attribute must survive"))
        return 1;
    if (!expect(
            writtenStyles.toString().contains(
                QStringLiteral("userDefine=\"codex_custom_keyword\"")),
            "user-defined keywords must be written"))
        return 1;

    if (!expect(parameters.createUserDefinedLanguage(
                    QStringLiteral("Codex Test")),
                "create UDL") ||
        !expect(parameters.renameUserDefinedLanguage(
                    QStringLiteral("Codex Test"),
                    QStringLiteral("Codex Renamed")),
                "rename UDL"))
        return 1;
    const QString exported = temporary.path() + QStringLiteral("/export.xml");
    if (!expect(parameters.exportUserDefinedLanguage(
                    QStringLiteral("Codex Renamed"), exported),
                "export UDL") ||
        !expect(parameters.deleteUserDefinedLanguage(
                    QStringLiteral("Codex Renamed")),
                "delete UDL") ||
        !expect(parameters.importUserDefinedLanguages(exported),
                "import UDL") ||
        !expect(parameters.getUserLangByName(
                    QStringLiteral("Codex Renamed")) != nullptr,
                "imported UDL must load"))
        return 1;
    return 0;
}
