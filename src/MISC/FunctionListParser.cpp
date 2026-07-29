#include "FunctionListParser.h"

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QRegularExpression>

#include <algorithm>

namespace {

struct FunctionRule
{
    QString mainExpression;
    QStringList nameExpressions;
};

struct ClassRule
{
    QString mainExpression;
    QStringList nameExpressions;
    QList<FunctionRule> functions;
    QString openSymbol;
    QString closeSymbol;
};

QRegularExpression parserRegex(const QString& expression)
{
    return QRegularExpression(
        expression,
        QRegularExpression::MultilineOption |
            QRegularExpression::DotMatchesEverythingOption |
            QRegularExpression::UseUnicodePropertiesOption);
}

QStringList expressionChain(const QDomElement& parent)
{
    QStringList expressions;
    for (QDomElement element = parent.firstChildElement();
         !element.isNull(); element = element.nextSiblingElement()) {
        if (element.hasAttribute(QStringLiteral("expr")))
            expressions.append(element.attribute(QStringLiteral("expr")));
        expressions.append(expressionChain(element));
    }
    return expressions;
}

QList<FunctionRule> functionRules(const QDomElement& parent)
{
    QList<FunctionRule> rules;
    for (QDomElement element = parent.firstChildElement(QStringLiteral("function"));
         !element.isNull();
         element = element.nextSiblingElement(QStringLiteral("function"))) {
        FunctionRule rule;
        rule.mainExpression = element.attribute(QStringLiteral("mainExpr"));
        const QDomElement names =
            element.firstChildElement(QStringLiteral("functionName"));
        rule.nameExpressions = expressionChain(names);
        if (!rule.mainExpression.isEmpty())
            rules.append(rule);
    }
    return rules;
}

QString extractedName(QString value, const QStringList& expressions)
{
    for (const QString& expression : expressions) {
        const QRegularExpression regex = parserRegex(expression);
        if (!regex.isValid())
            return QString();
        const QRegularExpressionMatch match = regex.match(value);
        if (!match.hasMatch())
            return QString();
        value = match.captured(0);
    }
    return value.trimmed();
}

int lineForOffset(const QString& text, int offset)
{
    return text.left(qMax(0, offset)).count(QLatin1Char('\n'));
}

QString literalSymbol(QString symbol)
{
    if (symbol.size() == 2 && symbol.front() == QLatin1Char('\\'))
        symbol.remove(0, 1);
    return symbol;
}

int scopedEnd(const QString& text, int start, int initialEnd,
              const QString& rawOpen, const QString& rawClose)
{
    const QString open = literalSymbol(rawOpen);
    const QString close = literalSymbol(rawClose);
    if (open.isEmpty() || close.isEmpty())
        return initialEnd;
    const int firstOpen = text.indexOf(open, start);
    if (firstOpen < 0)
        return initialEnd;
    int depth = 0;
    for (int position = firstOpen; position < text.size();) {
        if (text.midRef(position, open.size()) == open) {
            ++depth;
            position += open.size();
        } else if (text.midRef(position, close.size()) == close) {
            --depth;
            position += close.size();
            if (depth == 0)
                return position;
        } else {
            ++position;
        }
    }
    return initialEnd;
}

void appendMatches(QList<FunctionListEntry>* entries, const QString& fullText,
                   const QString& scopeText, int scopeOffset,
                   const FunctionRule& rule, const QString& prefix = QString())
{
    const QRegularExpression regex = parserRegex(rule.mainExpression);
    if (!regex.isValid())
        return;
    QRegularExpressionMatchIterator matches = regex.globalMatch(scopeText);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        QString name = extractedName(match.captured(0), rule.nameExpressions);
        if (name.isEmpty())
            name = match.captured(0).trimmed();
        if (name.isEmpty())
            continue;
        if (!prefix.isEmpty())
            name = prefix + QStringLiteral("::") + name;
        entries->append(
            {name, lineForOffset(fullText, scopeOffset + match.capturedStart())});
    }
}

} // namespace

QString FunctionListParser::parserFileName(const QString& language)
{
    const QString key = language.trimmed().toLower();
    static const QMap<QString, QString> mappings = {
        {QStringLiteral("c++"), QStringLiteral("cpp.xml")},
        {QStringLiteral("c"), QStringLiteral("c.xml")},
        {QStringLiteral("c#"), QStringLiteral("cs.xml")},
        {QStringLiteral("ada"), QStringLiteral("ada.xml")},
        {QStringLiteral("autoit"), QStringLiteral("autoit.xml")},
        {QStringLiteral("baanc"), QStringLiteral("baanc.xml")},
        {QStringLiteral("python"), QStringLiteral("python.xml")},
        {QStringLiteral("java"), QStringLiteral("java.xml")},
        {QStringLiteral("javascript"), QStringLiteral("javascript.js.xml")},
        {QStringLiteral("typescript"), QStringLiteral("typescript.xml")},
        {QStringLiteral("bash"), QStringLiteral("bash.xml")},
        {QStringLiteral("batch"), QStringLiteral("batch.xml")},
        {QStringLiteral("lua"), QStringLiteral("lua.xml")},
        {QStringLiteral("ruby"), QStringLiteral("ruby.xml")},
        {QStringLiteral("sql"), QStringLiteral("sql.xml")},
        {QStringLiteral("xml"), QStringLiteral("xml.xml")},
        {QStringLiteral("php"), QStringLiteral("php.xml")},
        {QStringLiteral("perl"), QStringLiteral("perl.xml")},
        {QStringLiteral("powershell"), QStringLiteral("powershell.xml")},
        {QStringLiteral("vhdl"), QStringLiteral("vhdl.xml")},
        {QStringLiteral("fortran"), QStringLiteral("fortran.xml")},
        {QStringLiteral("fortran77"), QStringLiteral("fortran77.xml")},
        {QStringLiteral("haskell"), QStringLiteral("haskell.xml")},
        {QStringLiteral("assembly"), QStringLiteral("asm.xml")},
        {QStringLiteral("asm"), QStringLiteral("asm.xml")},
        {QStringLiteral("ini"), QStringLiteral("ini.xml")},
        {QStringLiteral("cobol"), QStringLiteral("cobol.xml")},
        {QStringLiteral("cobol (free)"), QStringLiteral("cobol-free.xml")},
        {QStringLiteral("inno setup"), QStringLiteral("inno.xml")},
        {QStringLiteral("krl"), QStringLiteral("krl.xml")},
        {QStringLiteral("nsis"), QStringLiteral("nsis.xml")},
        {QStringLiteral("rust"), QStringLiteral("rust.xml")},
        {QStringLiteral("sinumerik"), QStringLiteral("sinumerik.xml")},
        {QStringLiteral("universe basic"), QStringLiteral("universe_basic.xml")}
    };
    return mappings.value(key);
}

QList<FunctionListEntry> FunctionListParser::parse(
    const QString& text, const QString& language,
    const QStringList& searchDirectories)
{
    const QString fileName = parserFileName(language);
    if (fileName.isEmpty())
        return {};

    QString parserPath;
    for (const QString& directory : searchDirectories) {
        const QString candidate = QDir(directory).filePath(fileName);
        if (QFileInfo(candidate).isFile()) {
            parserPath = candidate;
            break;
        }
    }
    if (parserPath.isEmpty())
        return {};

    QFile file(parserPath);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QDomDocument document;
    if (!document.setContent(&file))
        return {};
    const QDomElement parser =
        document.documentElement()
            .firstChildElement(QStringLiteral("functionList"))
            .firstChildElement(QStringLiteral("parser"));
    if (parser.isNull())
        return {};

    QString source = text;
    const QString commentExpression =
        parser.attribute(QStringLiteral("commentExpr"));
    if (!commentExpression.isEmpty()) {
        const QRegularExpression comments = parserRegex(commentExpression);
        if (comments.isValid()) {
            QRegularExpressionMatchIterator matches =
                comments.globalMatch(source);
            while (matches.hasNext()) {
                const QRegularExpressionMatch match = matches.next();
                for (int i = match.capturedStart(); i < match.capturedEnd(); ++i) {
                    if (source.at(i) != QLatin1Char('\n') &&
                        source.at(i) != QLatin1Char('\r'))
                        source[i] = QLatin1Char(' ');
                }
            }
        }
    }

    QList<FunctionListEntry> entries;
    for (const FunctionRule& rule : functionRules(parser))
        appendMatches(&entries, source, source, 0, rule);

    for (QDomElement element =
             parser.firstChildElement(QStringLiteral("classRange"));
         !element.isNull();
         element = element.nextSiblingElement(QStringLiteral("classRange"))) {
        ClassRule rule;
        rule.mainExpression = element.attribute(QStringLiteral("mainExpr"));
        rule.nameExpressions = expressionChain(
            element.firstChildElement(QStringLiteral("className")));
        rule.functions = functionRules(element);
        rule.openSymbol = element.attribute(QStringLiteral("openSymbole"));
        rule.closeSymbol = element.attribute(QStringLiteral("closeSymbole"));
        const QRegularExpression classRegex = parserRegex(rule.mainExpression);
        if (!classRegex.isValid())
            continue;
        QRegularExpressionMatchIterator classes = classRegex.globalMatch(source);
        while (classes.hasNext()) {
            const QRegularExpressionMatch match = classes.next();
            const int classEnd = scopedEnd(
                source, match.capturedStart(), match.capturedEnd(),
                rule.openSymbol, rule.closeSymbol);
            const QString classText = source.mid(
                match.capturedStart(), classEnd - match.capturedStart());
            const QString className =
                extractedName(classText, rule.nameExpressions);
            if (!className.isEmpty()) {
                entries.append(
                    {className, lineForOffset(source, match.capturedStart())});
            }
            for (const FunctionRule& function : rule.functions) {
                appendMatches(&entries, source, classText,
                              match.capturedStart(), function, className);
            }
        }
    }

    std::sort(entries.begin(), entries.end(),
              [](const FunctionListEntry& left,
                 const FunctionListEntry& right) {
        return left.line < right.line ||
            (left.line == right.line && left.name < right.name);
    });
    return entries;
}
