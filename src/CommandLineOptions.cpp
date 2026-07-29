#include "CommandLineOptions.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonArray>

namespace {

QString unquote(QString value)
{
    if (value.size() >= 2 && value.front() == QLatin1Char('"') &&
        value.back() == QLatin1Char('"')) {
        value = value.mid(1, value.size() - 2);
    }
    return value;
}

bool takeFlag(QStringList* values, const QString& flag)
{
    const int index = values->indexOf(flag);
    if (index < 0)
        return false;
    values->removeAt(index);
    return true;
}

QString takePrefix(QStringList* values, const QString& prefix)
{
    for (int i = 0; i < values->size(); ++i) {
        if (values->at(i).startsWith(prefix)) {
            const QString result = values->at(i).mid(prefix.size());
            values->removeAt(i);
            return unquote(result);
        }
    }
    return QString();
}

QString takeNamedValue(QStringList* values, const QStringList& names)
{
    for (int i = 0; i < values->size(); ++i) {
        for (const QString& name : names) {
            const QString value = values->at(i);
            if (value == name) {
                values->removeAt(i);
                if (i >= values->size())
                    return QStringLiteral("");
                return unquote(values->takeAt(i));
            }
            const QString prefix = name + QLatin1Char('=');
            if (value.startsWith(prefix)) {
                values->removeAt(i);
                return unquote(value.mid(prefix.size()));
            }
        }
    }
    return QString();
}

qint64 takeNumber(QStringList* values, QChar option, bool* present = nullptr)
{
    if (present)
        *present = false;
    const QString prefix = QStringLiteral("-") + option;
    for (int i = 0; i < values->size(); ++i) {
        const QString value = values->at(i);
        if (!value.startsWith(prefix) || value.size() < 2)
            continue;
        bool ok = false;
        const qint64 number = value.mid(2).toLongLong(&ok);
        values->removeAt(i);
        if (present)
            *present = true;
        return ok ? number : 0;
    }
    return -1;
}

QString absolutePath(const QString& path, const QString& workingDirectory)
{
    QString expanded = QDir::fromNativeSeparators(path);
    if (expanded == QStringLiteral("~")) {
        expanded = QDir::homePath();
    } else if (expanded.startsWith(QStringLiteral("~/"))) {
        expanded = QDir::home().filePath(expanded.mid(2));
    }

    QFileInfo info(expanded);
    if (info.isRelative())
        info.setFile(QDir(workingDirectory), expanded);
    return QDir::cleanPath(info.absoluteFilePath());
}

QStringList expandPath(const QString& rawPath, const QString& workingDirectory,
                       bool recursive)
{
    const QString path = absolutePath(rawPath, workingDirectory);
    if (!path.contains(QLatin1Char('*')) && !path.contains(QLatin1Char('?')))
        return QStringList(path);

    const QFileInfo patternInfo(path);
    const QString root = patternInfo.absolutePath();
    const QString pattern = patternInfo.fileName();
    QStringList result;
    QDirIterator it(root, QStringList(pattern), QDir::Files,
                    recursive ? QDirIterator::Subdirectories
                              : QDirIterator::NoIteratorFlags);
    while (it.hasNext())
        result.append(QDir::cleanPath(it.next()));
    result.sort(Qt::CaseInsensitive);
    return result;
}

QJsonArray stringArray(const QStringList& values)
{
    QJsonArray result;
    for (const QString& value : values)
        result.append(value);
    return result;
}

QStringList jsonStrings(const QJsonValue& value)
{
    QStringList result;
    for (const QJsonValue& item : value.toArray())
        result.append(item.toString());
    return result;
}

} // namespace

CommandLineOptions CommandLineParser::parse(
    const QStringList& arguments, const QString& workingDirectory)
{
    CommandLineOptions result;
    QStringList values = arguments;
    if (!values.isEmpty())
        values.removeFirst();

    result.notepadStyle = takeFlag(&values, QStringLiteral("-notepadStyleCmdline"));
    result.quickPrint = takeFlag(&values, QStringLiteral("-quickPrint"));
    if (!values.isEmpty() &&
        values.first().compare(QStringLiteral("/p"), Qt::CaseInsensitive) == 0) {
        result.quickPrint = true;
        values.removeFirst();
    }

    result.showHelp = takeFlag(&values, QStringLiteral("--help"));
    result.multiInstance = takeFlag(&values, QStringLiteral("-multiInst"));
    result.noPlugin = takeFlag(&values, QStringLiteral("-noPlugin"));
    result.readOnly = takeFlag(&values, QStringLiteral("-ro"));
    result.noSession = takeFlag(&values, QStringLiteral("-nosession"));
    result.noTabBar = takeFlag(&values, QStringLiteral("-notabbar"));
    result.systemTray = takeFlag(&values, QStringLiteral("-systemtray"));
    result.showLoadingTime = takeFlag(&values, QStringLiteral("-loadingTime"));
    result.alwaysOnTop = takeFlag(&values, QStringLiteral("-alwaysOnTop"));
    result.openSession = takeFlag(&values, QStringLiteral("-openSession"));
    result.recursive = takeFlag(&values, QStringLiteral("-r"));
    result.exportFunctionList =
        takeFlag(&values, QStringLiteral("-export=functionList"));
    result.openFoldersAsWorkspace =
        takeFlag(&values, QStringLiteral("-openFoldersAsWorkspace"));
    result.monitorFiles = takeFlag(&values, QStringLiteral("-monitor"));

    result.language = takePrefix(&values, QStringLiteral("-l"));
    result.localizationCode = takePrefix(&values, QStringLiteral("-L"));
    result.userDefinedLanguage =
        takePrefix(&values, QStringLiteral("-udl="));
    result.pluginMessage =
        takePrefix(&values, QStringLiteral("-pluginMessage="));
    result.settingsDirectory = takeNamedValue(
        &values, {QStringLiteral("-settingsDir"),
                  QStringLiteral("--settings-dir")});
    result.titleAddition =
        takePrefix(&values, QStringLiteral("-titleAdd="));

    result.quote = takePrefix(&values, QStringLiteral("-qn="));
    if (!result.quote.isNull())
        result.quoteType = 0;
    else {
        result.quote = takePrefix(&values, QStringLiteral("-qt="));
        if (!result.quote.isNull())
            result.quoteType = 1;
        else {
            result.quote = takePrefix(&values, QStringLiteral("-qf="));
            if (!result.quote.isNull()) {
                result.quoteType = 2;
                result.quote = absolutePath(result.quote, workingDirectory);
            }
        }
    }
    const QString speed = takePrefix(&values, QStringLiteral("-qSpeed"));
    if (!speed.isNull()) {
        bool ok = false;
        const int parsed = speed.toInt(&ok);
        if (ok && parsed >= 1 && parsed <= 3)
            result.ghostTypingSpeed = parsed;
    }

    result.line = takeNumber(&values, QLatin1Char('n'));
    result.column = takeNumber(&values, QLatin1Char('c'));
    result.position = takeNumber(&values, QLatin1Char('p'));
    bool xPresent = false;
    bool yPresent = false;
    result.windowX = static_cast<int>(
        takeNumber(&values, QLatin1Char('x'), &xPresent));
    result.windowY = static_cast<int>(
        takeNumber(&values, QLatin1Char('y'), &yPresent));
    result.hasWindowX = xPresent;
    result.hasWindowY = yPresent;

    if (result.notepadStyle && !values.isEmpty()) {
        QString path = values.join(QLatin1Char(' '));
        if (QFileInfo(path).suffix().isEmpty())
            path += QStringLiteral(".txt");
        values = QStringList(path);
        result.multiInstance = true;
        result.noSession = true;
        result.noTabBar = true;
    }

    if (result.quickPrint || result.exportFunctionList) {
        result.multiInstance = true;
        result.noSession = true;
    }

    for (const QString& value : values) {
        if (value.startsWith(QLatin1Char('-'))) {
            result.unknownOptions.append(value);
        }
        result.paths.append(expandPath(value, workingDirectory, result.recursive));
    }
    if (!result.settingsDirectory.isEmpty())
        result.settingsDirectory =
            absolutePath(result.settingsDirectory, workingDirectory);
    return result;
}

QString CommandLineParser::helpText()
{
    return QStringLiteral(
        "Usage:\n\n"
        "notepad++ [--help] [-multiInst] [-noPlugin] [-lLanguage] "
        "[-udl=\"My UDL Name\"] [-LlangCode] [-nLineNumber] "
        "[-cColumnNumber] [-pPosition] [-xLeftPos] [-yTopPos] [-monitor] "
        "[-nosession] [-notabbar] [-ro] [-systemtray] [-loadingTime] "
        "[-alwaysOnTop] [-openSession] [-r] [-qn=\"Easter egg name\" | "
        "-qt=\"text\" | -qf=\"quote file\"] [-qSpeed1|2|3] [-quickPrint] "
        "[-settingsDir=\"settings directory\"] [--settings-dir path] "
        "[-openFoldersAsWorkspace] "
        "[-titleAdd=\"title text\"] [filePath]\n\n"
        "--help: Show this help.\n"
        "-multiInst: Launch another instance.\n"
        "-noPlugin: Do not load plugins.\n"
        "-l: Apply the named built-in language.\n"
        "-udl=: Apply a User Defined Language.\n"
        "-L: Apply the indicated UI localization for this launch.\n"
        "-n/-c/-p: Go to line, column, or UTF-8 document position.\n"
        "-x/-y: Set the initial window position.\n"
        "-monitor: Open files with monitoring enabled.\n"
        "-nosession: Do not load or save the previous session.\n"
        "-notabbar: Hide the document tab bar.\n"
        "-ro: Open command-line files read-only.\n"
        "-systemtray: Start hidden in the system tray.\n"
        "-loadingTime: Display startup time.\n"
        "-alwaysOnTop: Keep the window above other windows.\n"
        "-openSession: Treat the single file argument as a session file.\n"
        "-r: Expand wildcard file arguments recursively.\n"
        "-quickPrint: Print argument files, then exit.\n"
        "-settingsDir=/--settings-dir: Override the settings directory.\n"
        "-openFoldersAsWorkspace: Open directory arguments as workspace.\n"
        "-titleAdd=: Append text to the title bar.");
}

QString CommandLineParser::localizationFileName(const QString& rawCode)
{
    const QString code = rawCode.toLower().replace(QLatin1Char('_'), QLatin1Char('-'));
    if (code == QStringLiteral("pt-br")) return QStringLiteral("brazilian_portuguese.xml");
    if (code == QStringLiteral("zh") || code == QStringLiteral("zh-cn"))
        return QStringLiteral("chineseSimplified.xml");
    if (code == QStringLiteral("zh-tw") || code == QStringLiteral("zh-hk") ||
        code == QStringLiteral("zh-sg"))
        return QStringLiteral("taiwaneseMandarin.xml");
    if (code == QStringLiteral("sr-cyrl-ba") || code == QStringLiteral("sr-cyrl-sp"))
        return QStringLiteral("serbianCyrillic.xml");
    if (code == QStringLiteral("es-ar")) return QStringLiteral("spanish_ar.xml");
    if (code == QStringLiteral("uz-cyrl-uz")) return QStringLiteral("uzbekCyrillic.xml");
    if (code == QStringLiteral("tg-cyrl-tj")) return QStringLiteral("tajikCyrillic.xml");

    static const char* const mappings[][2] = {
        {"af","afrikaans"}, {"sq","albanian"}, {"ar","arabic"},
        {"an","aragonese"}, {"az","azerbaijani"}, {"eu","basque"},
        {"be","belarusian"}, {"bn","bengali"}, {"bs","bosnian"},
        {"br-fr","breton"}, {"bg","bulgarian"}, {"ca","catalan"},
        {"co","corsican"}, {"hr","croatian"}, {"cs","czech"},
        {"da","danish"}, {"nl","dutch"}, {"eo","esperanto"},
        {"et","estonian"}, {"fa","farsi"}, {"fi","finnish"},
        {"fr","french"}, {"fur","friulian"}, {"gl","galician"},
        {"ka","georgian"}, {"de","german"}, {"el","greek"},
        {"gu","gujarati"}, {"he","hebrew"}, {"hi","hindi"},
        {"hu","hungarian"}, {"id","indonesian"}, {"it","italian"},
        {"ja","japanese"}, {"kn","kannada"}, {"kk","kazakh"},
        {"ko","korean"}, {"ku","kurdish"}, {"ky","kyrgyz"},
        {"lv","latvian"}, {"lt","lithuanian"}, {"lb","luxembourgish"},
        {"mk","macedonian"}, {"ms","malay"}, {"mr","marathi"},
        {"mn","mongolian"}, {"no","norwegian"}, {"nb","norwegian"},
        {"nn","nynorsk"}, {"oc","occitan"}, {"pl","polish"},
        {"pt","portuguese"}, {"pa","punjabi"}, {"ro","romanian"},
        {"ru","russian"}, {"sc","sardinian"}, {"sr","serbian"},
        {"si","sinhala"}, {"sk","slovak"}, {"sl","slovenian"},
        {"es","spanish"}, {"sv","swedish"}, {"tl","tagalog"},
        {"ta","tamil"}, {"tt","tatar"}, {"te","telugu"}, {"th","thai"},
        {"tr","turkish"}, {"uk","ukrainian"}, {"ur","urdu"},
        {"ug-cn","uyghur"}, {"uz","uzbek"}, {"vec","venetian"},
        {"vi","vietnamese"}, {"cy-gb","welsh"}, {"zu","zulu"},
        {"ne","nepali"}, {"nep","nepali"}, {"oc-aranes","aranese"},
        {"exy","extremaduran"}, {"keb","kabyle"}, {"lij","ligurian"},
        {"ga","irish"}, {"sgs","samogitian"}, {"yue","hongKongCantonese"},
        {"ab","abkhazian"}, {"abk","abkhazian"}
    };
    const QString base = code.section(QLatin1Char('-'), 0, 0);
    for (const auto& mapping : mappings) {
        if (code == QLatin1String(mapping[0]) ||
            base == QLatin1String(mapping[0])) {
            return QLatin1String(mapping[1]) + QStringLiteral(".xml");
        }
    }
    return QString();
}

QJsonObject CommandLineOptions::toJson() const
{
    QJsonObject o;
#define WRITE_BOOL(name) o.insert(QStringLiteral(#name), name)
    WRITE_BOOL(showHelp); WRITE_BOOL(multiInstance); WRITE_BOOL(noPlugin);
    WRITE_BOOL(readOnly); WRITE_BOOL(noSession); WRITE_BOOL(noTabBar);
    WRITE_BOOL(systemTray); WRITE_BOOL(showLoadingTime); WRITE_BOOL(alwaysOnTop);
    WRITE_BOOL(openSession); WRITE_BOOL(recursive); WRITE_BOOL(exportFunctionList);
    WRITE_BOOL(quickPrint); WRITE_BOOL(notepadStyle);
    WRITE_BOOL(openFoldersAsWorkspace); WRITE_BOOL(monitorFiles);
    WRITE_BOOL(hasWindowX); WRITE_BOOL(hasWindowY);
#undef WRITE_BOOL
    o.insert(QStringLiteral("line"), static_cast<double>(line));
    o.insert(QStringLiteral("column"), static_cast<double>(column));
    o.insert(QStringLiteral("position"), static_cast<double>(position));
    o.insert(QStringLiteral("windowX"), windowX);
    o.insert(QStringLiteral("windowY"), windowY);
    o.insert(QStringLiteral("language"), language);
    o.insert(QStringLiteral("localizationCode"), localizationCode);
    o.insert(QStringLiteral("userDefinedLanguage"), userDefinedLanguage);
    o.insert(QStringLiteral("pluginMessage"), pluginMessage);
    o.insert(QStringLiteral("settingsDirectory"), settingsDirectory);
    o.insert(QStringLiteral("titleAddition"), titleAddition);
    o.insert(QStringLiteral("quote"), quote);
    o.insert(QStringLiteral("quoteType"), quoteType);
    o.insert(QStringLiteral("ghostTypingSpeed"), ghostTypingSpeed);
    o.insert(QStringLiteral("paths"), stringArray(paths));
    o.insert(QStringLiteral("unknownOptions"), stringArray(unknownOptions));
    return o;
}

CommandLineOptions CommandLineOptions::fromJson(const QJsonObject& o)
{
    CommandLineOptions r;
#define READ_BOOL(name) r.name = o.value(QStringLiteral(#name)).toBool()
    READ_BOOL(showHelp); READ_BOOL(multiInstance); READ_BOOL(noPlugin);
    READ_BOOL(readOnly); READ_BOOL(noSession); READ_BOOL(noTabBar);
    READ_BOOL(systemTray); READ_BOOL(showLoadingTime); READ_BOOL(alwaysOnTop);
    READ_BOOL(openSession); READ_BOOL(recursive); READ_BOOL(exportFunctionList);
    READ_BOOL(quickPrint); READ_BOOL(notepadStyle);
    READ_BOOL(openFoldersAsWorkspace); READ_BOOL(monitorFiles);
    READ_BOOL(hasWindowX); READ_BOOL(hasWindowY);
#undef READ_BOOL
    r.line = static_cast<qint64>(o.value(QStringLiteral("line")).toDouble(-1));
    r.column = static_cast<qint64>(o.value(QStringLiteral("column")).toDouble(-1));
    r.position = static_cast<qint64>(o.value(QStringLiteral("position")).toDouble(-1));
    r.windowX = o.value(QStringLiteral("windowX")).toInt();
    r.windowY = o.value(QStringLiteral("windowY")).toInt();
    r.language = o.value(QStringLiteral("language")).toString();
    r.localizationCode = o.value(QStringLiteral("localizationCode")).toString();
    r.userDefinedLanguage =
        o.value(QStringLiteral("userDefinedLanguage")).toString();
    r.pluginMessage = o.value(QStringLiteral("pluginMessage")).toString();
    r.settingsDirectory =
        o.value(QStringLiteral("settingsDirectory")).toString();
    r.titleAddition = o.value(QStringLiteral("titleAddition")).toString();
    r.quote = o.value(QStringLiteral("quote")).toString();
    r.quoteType = o.value(QStringLiteral("quoteType")).toInt(-1);
    r.ghostTypingSpeed =
        o.value(QStringLiteral("ghostTypingSpeed")).toInt(-1);
    r.paths = jsonStrings(o.value(QStringLiteral("paths")));
    r.unknownOptions =
        jsonStrings(o.value(QStringLiteral("unknownOptions")));
    return r;
}
