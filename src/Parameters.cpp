// Parameters.cpp - 配置参数管理实现
// 移植自: v8.4.6:PowerEditor/src/Parameters.cpp

#include "Parameters.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSettings>
#include <QXmlStreamWriter>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QRegExp>
#include "TinyXml/tinyxml.h"

static int windowsVirtualKeyToQt(int key)
{
    if (key >= '0' && key <= '9') return Qt::Key_0 + key - '0';
    if (key >= 'A' && key <= 'Z') return Qt::Key_A + key - 'A';
    if (key >= 112 && key <= 135) return Qt::Key_F1 + key - 112;
    switch (key) {
        case 8: return Qt::Key_Backspace;
        case 9: return Qt::Key_Tab;
        case 13: return Qt::Key_Return;
        case 27: return Qt::Key_Escape;
        case 32: return Qt::Key_Space;
        case 33: return Qt::Key_PageUp;
        case 34: return Qt::Key_PageDown;
        case 35: return Qt::Key_End;
        case 36: return Qt::Key_Home;
        case 37: return Qt::Key_Left;
        case 38: return Qt::Key_Up;
        case 39: return Qt::Key_Right;
        case 40: return Qt::Key_Down;
        case 45: return Qt::Key_Insert;
        case 46: return Qt::Key_Delete;
        default: return key;
    }
}

static int qtKeyToWindowsVirtualKey(int key)
{
    key &= ~Qt::KeyboardModifierMask;
    if (key >= Qt::Key_0 && key <= Qt::Key_9) return '0' + key - Qt::Key_0;
    if (key >= Qt::Key_A && key <= Qt::Key_Z) return 'A' + key - Qt::Key_A;
    if (key >= Qt::Key_F1 && key <= Qt::Key_F24) return 112 + key - Qt::Key_F1;
    switch (key) {
        case Qt::Key_Backspace: return 8;
        case Qt::Key_Tab: return 9;
        case Qt::Key_Return:
        case Qt::Key_Enter: return 13;
        case Qt::Key_Escape: return 27;
        case Qt::Key_Space: return 32;
        case Qt::Key_PageUp: return 33;
        case Qt::Key_PageDown: return 34;
        case Qt::Key_End: return 35;
        case Qt::Key_Home: return 36;
        case Qt::Key_Left: return 37;
        case Qt::Key_Up: return 38;
        case Qt::Key_Right: return 39;
        case Qt::Key_Down: return 40;
        case Qt::Key_Insert: return 45;
        case Qt::Key_Delete: return 46;
        default: return key;
    }
}

QKeySequence ShortcutKey::toKeySequence() const
{
    int value = windowsVirtualKeyToQt(key);
    if (ctrl) value |= Qt::CTRL;
    if (alt) value |= Qt::ALT;
    if (shift) value |= Qt::SHIFT;
    return key == 0 ? QKeySequence() : QKeySequence(value);
}

ShortcutKey ShortcutKey::fromKeySequence(const QKeySequence& sequence)
{
    ShortcutKey result;
    if (sequence.isEmpty())
        return result;
    const int value = sequence[0];
    result.ctrl = (value & Qt::CTRL) != 0;
    result.alt = (value & Qt::ALT) != 0;
    result.shift = (value & Qt::SHIFT) != 0;
    result.key = qtKeyToWindowsVirtualKey(value);
    return result;
}

// ── TiXml → QString 辅助函数 ─────────────────────────────────────────────────

static inline QString txAttr(const TiXmlElement* el, const wchar_t* name)
{
    const wchar_t* v = el ? el->Attribute(name) : nullptr;
    return v ? QString::fromWCharArray(v) : QString();
}

static inline QString txText(const TiXmlNode* node)
{
    const TiXmlNode* child = node ? node->FirstChild() : nullptr;
    const wchar_t* v = child ? child->Value() : nullptr;
    return v ? QString::fromWCharArray(v) : QString();
}

// ── TiXml 文件加载辅助：用 Qt 读取（正确处理 UTF-8），再交给 TiXml 解析 ─────────
// 原因：TiXml 的 LoadFile 依赖系统代码页（GBK），中文路径会被错误解码。
// 用 Qt 读为 UTF-8 QString → wstring → TiXml::Parse，绕过代码页问题。
static bool loadTiXmlDoc(TiXmlDocument& doc, const QString& path)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly))
        return false;
    QByteArray data = file.readAll();
    file.close();
    // Qt 正确解析 UTF-8（包括中文字符）→ wstring → TiXml 解析
    std::wstring wContent = QString::fromUtf8(data).toStdWString();
    doc.Parse(wContent.c_str(), 0);
    return !doc.Error();
}

// ── XML 辅助（parseBool / parseInt 在 loadStylers 等之前定义） ────────────────

static bool parseBool(const QString& val, bool def = false)
{
    if (val.isNull()) return def;
    return (val == "yes" || val == "1" || val == "true");
}

static int parseInt(const QString& val, int def = 0)
{
    if (val.isNull()) return def;
    bool ok;
    int v = val.toInt(&ok);
    return ok ? v : def;
}

// ── 单例 ─────────────────────────────────────────────────────────────────────

NppParameters& NppParameters::getInstance()
{
    static NppParameters instance;
    return instance;
}

const QString& NppParameters::workspaceFilePath(int panel) const
{
    static const QString empty;
    return panel >= 0 && panel < 3 ? _workspaceFilePaths[panel] : empty;
}

void NppParameters::setWorkspaceFilePath(int panel, const QString& path)
{
    if (panel >= 0 && panel < 3)
        _workspaceFilePaths[panel] = path;
}

NppParameters::NppParameters()
{
    initDefaultPaths();
}

void NppParameters::initDefaultPaths()
{
    _nppPath = QDir::cleanPath(QApplication::applicationDirPath());
    const ConfigPathResolution resolution =
        ConfigPathResolver::resolve(_nppPath);
    _userPath = resolution.directory;
    _configPathSource = resolution.source;
    _configPathError = resolution.error;
}

bool NppParameters::setUserPathOverride(const QString& path)
{
    if (path.trimmed().isEmpty())
        return true;

    const ConfigPathResolution resolution =
        ConfigPathResolver::resolve(_nppPath, path);
    if (!resolution.isValid()) {
        _configPathError = resolution.error;
        return false;
    }

    _userPath = resolution.directory;
    _configPathSource = resolution.source;
    _configPathError.clear();
    return true;
}

void NppParameters::setStartupLocalizationFile(const QString& fileName)
{
    _startupLocalizationFile = QFileInfo(fileName).fileName();
}

bool NppParameters::ensureConfigDir()
{
    _configPathError.clear();
    if (!ConfigPathResolver::prepareDirectory(
            _userPath, _configPathSource != ConfigPathSource::CommandLine,
            &_configPathError)) {
        return false;
    }

    QDir dir(_userPath);

    // Keep the Notepad++ directory shape available early so later features can
    // read/write compatible resources without each creating its own ad hoc path.
    const QStringList childDirs = {
        "backup",
        "plugins",
        "themes",
        "autoCompletion",
        "localization",
        "nativeLang",
        "userDefineLangs",
        "toolbarIcons",
        "functionList"
    };
    for (const QString& child : childDirs) {
        if (!dir.mkpath(child)) {
            _configPathError =
                QStringLiteral("Could not create settings subdirectory: %1")
                    .arg(QDir::toNativeSeparators(dir.filePath(child)));
            return false;
        }
    }
    return true;
}

static void copyResourceIfMissing(const QString& resourcePath, const QString& targetPath)
{
    if (!QFile::exists(targetPath)) {
        QDir().mkpath(QFileInfo(targetPath).absolutePath());
        QFile::copy(resourcePath, targetPath);
    }
    QFile::setPermissions(targetPath,
        QFileDevice::ReadOwner | QFileDevice::WriteOwner |
        QFileDevice::ReadUser | QFileDevice::WriteUser |
        QFileDevice::ReadGroup | QFileDevice::ReadOther);
}

void NppParameters::ensureDefaultXmlFiles() const
{
    copyResourceIfMissing(":/config.model.xml", configFilePath());
    copyResourceIfMissing(":/langs.model.xml", langsFilePath());
    copyResourceIfMissing(":/stylers.model.xml", stylersFilePath());
    copyResourceIfMissing(":/shortcuts.xml", shortcutsFilePath());
    copyResourceIfMissing(":/contextMenu.xml", contextMenuFilePath());
    copyResourceIfMissing(":/toolbarIcons.xml", toolbarIconsFilePath());
    copyResourceIfMissing(":/userDefineLang.xml", userDefineLangFilePath());
}

QString NppParameters::configFilePath()    const { return _userPath + "/config.xml";    }
QString NppParameters::sessionFilePath()   const { return _userPath + "/session.xml";   }
QString NppParameters::nativeLangFilePath() const { return _userPath + "/nativeLang.xml"; }
QString NppParameters::qtStateFilePath() const { return _userPath + "/qtState.ini"; }
QString NppParameters::langsFilePath()     const { return _userPath + "/langs.xml";     }
QString NppParameters::stylersFilePath()   const { return _userPath + "/stylers.xml";   }
QString NppParameters::shortcutsFilePath() const { return _userPath + "/shortcuts.xml"; }
QString NppParameters::contextMenuFilePath() const { return _userPath + "/contextMenu.xml"; }
QString NppParameters::toolbarIconsFilePath() const { return _userPath + "/toolbarIcons.xml"; }
QString NppParameters::userDefineLangFilePath() const { return _userPath + "/userDefineLang.xml"; }

// ── 界面语言（与原版一致，通过 nativeLang.xml 标识） ────────────────────────

// 将原版 nativeLang.xml 的 filename 属性映射到 Qt locale 代码
static QString filenameToLang(const QString& filename)
{
    // 原版 localization 文件名 → Qt locale
    static const struct { const char* file; const char* lang; } map[] = {
        { "chineseSimplified.xml",  "zh_CN" },
        { "chineseTraditional.xml", "zh_TW" },
        { "japanese.xml",           "ja"    },
        { "korean.xml",             "ko"    },
        { "french.xml",             "fr"    },
        { "german.xml",             "de"    },
        { "spanish.xml",            "es"    },
        { "russian.xml",            "ru"    },
        { "portuguese.xml",         "pt"    },
        { "italian.xml",            "it"    },
        { "dutch.xml",              "nl"    },
        { "polish.xml",             "pl"    },
        { "turkish.xml",            "tr"    },
        { "arabic.xml",             "ar"    },
    };
    for (auto& e : map)
        if (filename.compare(e.file, Qt::CaseInsensitive) == 0)
            return e.lang;
    return QString();
}

static QString langToFilename(const QString& lang)
{
    if (lang == "zh_CN" || lang == "chineseSimplified")
        return "chineseSimplified.xml";
    return QString();
}

QString NppParameters::getNativeLang() const
{
    QString path = nativeLangFilePath();
    if (!QFile::exists(path))
        return "en";

    TiXmlDocument doc;
    if (!loadTiXmlDoc(doc, path))
        return "en";

    TiXmlNode* root = doc.FirstChild(L"NotepadPlus");
    if (!root) return "en";

    TiXmlElement* el = root->FirstChildElement(L"Native-Langue");
    if (!el) return "en";

    // 优先读我们自己写的 lang 属性
    const wchar_t* langW = el->Attribute(L"lang");
    QString lang = langW ? QString::fromWCharArray(langW) : QString();
    if (!lang.isEmpty())
        return lang;

    // 兼容原版格式：通过 filename 属性推断
    const wchar_t* fnW = el->Attribute(L"filename");
    QString filename = fnW ? QString::fromWCharArray(fnW) : QString();
    lang = filenameToLang(filename);
    return lang.isEmpty() ? "en" : lang;
}

void NppParameters::setNativeLang(const QString& lang)
{
    ensureConfigDir();
    if (lang.isEmpty() || lang == "en") {
        // 英文为默认，不需要文件
        QFile::remove(nativeLangFilePath());
        return;
    }

    const QString filename = langToFilename(lang);
    if (filename.isEmpty())
        return;

    QString sourcePath = ":/localization/" + filename;
    const QString userLocalization =
        _userPath + "/localization/" + filename;
    const QString appLocalization =
        _nppPath + "/localization/" + filename;
    QString installedLocalization;
#ifdef Q_OS_WIN
    const QString programFiles = qEnvironmentVariable("ProgramFiles");
    if (!programFiles.isEmpty()) {
        installedLocalization =
            QDir(programFiles).filePath(
                "Notepad++/localization/" + filename);
    }
#endif
    if (QFile::exists(userLocalization))
        sourcePath = userLocalization;
    else if (QFile::exists(appLocalization))
        sourcePath = appLocalization;
    else if (!installedLocalization.isEmpty()
             && QFile::exists(installedLocalization))
        sourcePath = installedLocalization;

    QFile source(sourcePath);
    if (!source.open(QFile::ReadOnly))
        return;
    const QByteArray languageXml = source.readAll();
    source.close();

    QSaveFile target(nativeLangFilePath());
    if (!target.open(QFile::WriteOnly))
        return;
    if (target.write(languageXml) != languageXml.size()) {
        target.cancelWriting();
        return;
    }
    target.commit();
}

// ── reloadNativeLang() ────────────────────────────────────────────────────────
// 对应原版：Notepad_plus 构造时加载语言文件到 _nativeLangSpeaker
// Qt 版：在 load() 后调用，或切换语言后调用（运行时热切换）

void NppParameters::reloadNativeLang()
{
    if (!_startupLocalizationFile.isEmpty()) {
        const QStringList candidates = {
            _userPath + "/localization/" + _startupLocalizationFile,
            _nppPath + "/localization/" + _startupLocalizationFile,
            QStringLiteral(":/localization/") + _startupLocalizationFile
        };
        for (const QString& candidate : candidates) {
            if (QFile::exists(candidate)) {
                _nativeLangSpeaker.init(candidate);
                return;
            }
        }
    }

    const QString lang = getNativeLang();
    QString xmlPath;

    // 语言 → XML 文件名映射（与原版目录结构一致）
    if (lang == "zh_CN" || lang == "chineseSimplified") {
        xmlPath = ":/nativeLang/chineseSimplified.xml";
    }
    // 其他语言：优先查找用户配置目录，再查资源
    if (xmlPath.isEmpty() && !lang.isEmpty() && lang != "en") {
        QString userFile = _nppPath + "/nativeLang/" + lang + ".xml";
        if (QFile::exists(userFile))
            xmlPath = userFile;
    }

    _nativeLangSpeaker.init(xmlPath); // 空路径 = 英文，使用内建 tr() 文本
}

// ── 加载全部配置 ──────────────────────────────────────────────────────────────

bool NppParameters::load()
{
    if (!ensureConfigDir())
        return false;

    // Older Qt builds wrote a short <Native-Langue lang="..."/> marker.
    // Replace that incompatible stub with the complete original language pack.
    if (QFile::exists(nativeLangFilePath())) {
        TiXmlDocument nativeDoc;
        if (loadTiXmlDoc(nativeDoc, nativeLangFilePath())) {
            TiXmlElement* root =
                nativeDoc.FirstChildElement(L"NotepadPlus");
            TiXmlElement* nativeLang = root
                ? root->FirstChildElement(L"Native-Langue")
                : nullptr;
            const QString legacyLang = txAttr(nativeLang, L"lang");
            if (!legacyLang.isEmpty()
                && txAttr(nativeLang, L"filename").isEmpty()) {
                setNativeLang(legacyLang);
            }
        }
    }

    bool configExisted = QFile::exists(configFilePath());
    ensureDefaultXmlFiles();
    loadConfig();   // 失败时使用内置默认值
    loadQtState();

    // 从旧版 QSettings 迁移（仅当 config.xml 不存在时执行一次）
    if (!configExisted)
        migrateFromQSettings();

    loadSession(sessionFilePath());
    loadLangs();
    loadStylers();
    loadUserDefinedLanguages();
    loadShortcuts();

    // 确保备份目录存在
    QDir().mkpath(backupDirPath());

    return true;
}

static QString xmlText(const TiXmlElement* element)
{
    if (!element || !element->FirstChild() || !element->FirstChild()->Value())
        return QString();
    return QString::fromWCharArray(element->FirstChild()->Value());
}

static QString xmlAttr(const TiXmlElement* element, const wchar_t* name)
{
    if (!element)
        return QString();
    const wchar_t* value = element->Attribute(name);
    return value ? QString::fromWCharArray(value) : QString();
}

static QColor parseUdlColor(const QString& value)
{
    if (value.size() != 6)
        return QColor();
    bool ok = false;
    const uint rgb = value.toUInt(&ok, 16);
    return ok ? QColor((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff) : QColor();
}

static int udlKeywordListIndex(const QString& name)
{
    static const QMap<QString, int> names = {
        {QStringLiteral("Comment"), 0},
        {QStringLiteral("Comments"), 0},
        {QStringLiteral("Numbers, prefix1"), 1},
        {QStringLiteral("Numbers, prefixes"), 2},
        {QStringLiteral("Numbers, prefix2"), 2},
        {QStringLiteral("Numbers, extras1"), 3},
        {QStringLiteral("Numbers, extras with prefixes"), 4},
        {QStringLiteral("Numbers, extras2"), 4},
        {QStringLiteral("Numbers, suffix1"), 5},
        {QStringLiteral("Numbers, suffixes"), 6},
        {QStringLiteral("Numbers, suffix2"), 6},
        {QStringLiteral("Numbers, additional"), 7},
        {QStringLiteral("Numbers, range"), 7},
        {QStringLiteral("Operators"), 8},
        {QStringLiteral("Operators1"), 8},
        {QStringLiteral("Operators2"), 9},
        {QStringLiteral("Folder+"), 10},
        {QStringLiteral("Folders in code1, open"), 10},
        {QStringLiteral("Folders in code1, middle"), 11},
        {QStringLiteral("Folder-"), 12},
        {QStringLiteral("Folders in code1, close"), 12},
        {QStringLiteral("Folders in code2, open"), 13},
        {QStringLiteral("Folders in code2, middle"), 14},
        {QStringLiteral("Folders in code2, close"), 15},
        {QStringLiteral("Folders in comment, open"), 16},
        {QStringLiteral("Folders in comment, middle"), 17},
        {QStringLiteral("Folders in comment, close"), 18},
        {QStringLiteral("Delimiters"), 27}
    };
    const auto mapped = names.constFind(name);
    if (mapped != names.constEnd())
        return mapped.value();
    QRegularExpressionMatch match =
        QRegularExpression(QStringLiteral("^(?:Words|Keywords)([1-8])$"))
            .match(name);
    return match.hasMatch() ? 18 + match.captured(1).toInt() : -1;
}

static int udlStyleId(const QString& name, int fallback)
{
    static const QMap<QString, int> legacy = {
        {QStringLiteral("DEFAULT"), 0},
        {QStringLiteral("COMMENT"), 1},
        {QStringLiteral("COMMENT LINE"), 2},
        {QStringLiteral("NUMBER"), 3},
        {QStringLiteral("KEYWORD1"), 4},
        {QStringLiteral("KEYWORD2"), 5},
        {QStringLiteral("KEYWORD3"), 6},
        {QStringLiteral("KEYWORD4"), 7},
        {QStringLiteral("KEYWORD5"), 8},
        {QStringLiteral("KEYWORD6"), 9},
        {QStringLiteral("KEYWORD7"), 10},
        {QStringLiteral("KEYWORD8"), 11},
        {QStringLiteral("OPERATOR"), 12},
        {QStringLiteral("FOLDEROPEN"), 13},
        {QStringLiteral("FOLDERCLOSE"), 13}
    };
    return legacy.value(name.toUpper(), fallback);
}

bool NppParameters::loadUserDefinedLanguages()
{
    _userLangs.clear();
    const auto loadFile = [this](const QString& filePath) {
    TiXmlDocument doc;
    if (!loadTiXmlDoc(doc, filePath))
        return false;

    TiXmlNode* root = doc.FirstChild(L"NotepadPlus");
    if (!root)
        return false;

    for (TiXmlElement* lang = root->FirstChildElement(L"UserLang"); lang;
         lang = lang->NextSiblingElement(L"UserLang")) {
        UserLangDesc desc;
        desc.name = xmlAttr(lang, L"name");
        desc.sourceFilePath = filePath;
        desc.exts = xmlAttr(lang, L"ext").toLower().split(QRegExp("\\s+"), QString::SkipEmptyParts);

        if (TiXmlElement* settings = lang->FirstChildElement(L"Settings")) {
            if (TiXmlElement* global = settings->FirstChildElement(L"Global")) {
                const QString sensitive = xmlAttr(global, L"caseSensitive");
                const QString ignored = xmlAttr(global, L"caseIgnored");
                if (!sensitive.isEmpty()) desc.caseSensitive = sensitive.compare("yes", Qt::CaseInsensitive) == 0;
                if (!ignored.isEmpty()) desc.caseSensitive = ignored.compare("yes", Qt::CaseInsensitive) != 0;
            }
            if (TiXmlElement* prefix = settings->FirstChildElement(L"Prefix")) {
                for (int i = 0; i < 8; ++i) {
                    const QString attribute =
                        QStringLiteral("words%1").arg(i + 1);
                    desc.prefixKeywords[i] =
                        xmlAttr(prefix, attribute.toStdWString().c_str())
                            .compare(QStringLiteral("yes"),
                                     Qt::CaseInsensitive) == 0;
                }
            }
            if (TiXmlElement* folding =
                    settings->FirstChildElement(L"FoldingInComment")) {
                desc.foldComments =
                    xmlAttr(folding, L"fold")
                        .compare(QStringLiteral("yes"),
                                 Qt::CaseInsensitive) == 0;
            }
        }

        if (TiXmlElement* lists = lang->FirstChildElement(L"KeywordLists")) {
            for (TiXmlElement* item = lists->FirstChildElement(L"Keywords"); item;
                 item = item->NextSiblingElement(L"Keywords")) {
                const QString name = xmlAttr(item, L"name");
                const QString value = xmlText(item).trimmed();
                const int keywordList = udlKeywordListIndex(name);
                if (keywordList >= 0)
                    desc.keywordLists[keywordList] = value;
                if (name == "Operators") {
                    desc.operators = value;
                } else if (name == "Comment") {
                    const QRegularExpression marker("(?:^|\\s)([012])([^\\s]+)");
                    QRegularExpressionMatchIterator it = marker.globalMatch(value);
                    while (it.hasNext()) {
                        const QRegularExpressionMatch match = it.next();
                        if (match.captured(1) == "0" && desc.lineComment.isEmpty()) desc.lineComment = match.captured(2);
                        else if (match.captured(1) == "1") desc.blockCommentStart = match.captured(2);
                        else if (match.captured(1) == "2") desc.blockCommentEnd = match.captured(2);
                    }
                } else if (name.startsWith("Words")
                           || name.startsWith("Keywords")) {
                    bool ok = false;
                    const int offset =
                        name.startsWith("Words") ? 5 : 8;
                    const int index = name.mid(offset).toInt(&ok) - 1;
                    if (ok && index >= 0 && index < 8)
                        desc.keywords[index] = value.split(QRegExp("\\s+"), QString::SkipEmptyParts);
                }
            }
        }

        if (TiXmlElement* styles = lang->FirstChildElement(L"Styles")) {
            for (TiXmlElement* item = styles->FirstChildElement(L"WordsStyle"); item;
                 item = item->NextSiblingElement(L"WordsStyle")) {
                WordsStyle style;
                style.name = xmlAttr(item, L"name");
                style.xmlStyleID = xmlAttr(item, L"styleID").toInt();
                style.styleID = udlStyleId(
                    style.name, style.xmlStyleID);
                style.fgColor = parseUdlColor(xmlAttr(item, L"fgColor"));
                style.bgColor = parseUdlColor(xmlAttr(item, L"bgColor"));
                style.hasFg = style.fgColor.isValid();
                style.hasBg = style.bgColor.isValid();
                style.fontName = xmlAttr(item, L"fontName");
                style.fontStyle = xmlAttr(item, L"fontStyle").toInt();
                style.fontSize = xmlAttr(item, L"fontSize").toInt();
                style.nesting = xmlAttr(item, L"nesting").toInt();
                desc.styles.append(style);
            }
        }
        if (!desc.name.isEmpty())
            _userLangs.append(desc);
    }
    return true;
    };

    bool loaded = loadFile(userDefineLangFilePath());
    const QDir userLangDirectory(_userPath + QStringLiteral("/userDefineLangs"));
    const QFileInfoList userLangFiles = userLangDirectory.entryInfoList(
        QStringList() << QStringLiteral("*.xml"), QDir::Files,
        QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo& file : userLangFiles)
        loaded = loadFile(file.absoluteFilePath()) || loaded;
    return loaded;
}

bool NppParameters::writeUserDefinedLanguage(const UserLangDesc& language)
{
    const QString sourceFilePath = language.sourceFilePath.isEmpty()
        ? userDefineLangFilePath()
        : language.sourceFilePath;
    TiXmlDocument doc;
    if (!loadTiXmlDoc(doc, sourceFilePath))
        return false;
    TiXmlElement* root = doc.FirstChildElement(L"NotepadPlus");
    if (!root)
        return false;
    TiXmlElement* target = nullptr;
    for (TiXmlElement* item = root->FirstChildElement(L"UserLang"); item;
         item = item->NextSiblingElement(L"UserLang")) {
        if (xmlAttr(item, L"name") == language.name) {
            target = item;
            break;
        }
    }
    if (!target)
        return false;

    target->SetAttribute(
        L"ext", language.exts.join(QStringLiteral(" "))
                    .toStdWString().c_str());
    auto ensureChild = [](TiXmlElement* parent, const wchar_t* name) {
        TiXmlElement* child = parent->FirstChildElement(name);
        if (!child) {
            child = new TiXmlElement(name);
            parent->LinkEndChild(child);
        }
        return child;
    };
    TiXmlElement* settings = ensureChild(target, L"Settings");
    TiXmlElement* global = ensureChild(settings, L"Global");
    global->SetAttribute(
        L"caseIgnored", language.caseSensitive ? L"no" : L"yes");
    TiXmlElement* prefix = ensureChild(settings, L"Prefix");
    for (int i = 0; i < 8; ++i) {
        const std::wstring attribute =
            QStringLiteral("words%1").arg(i + 1).toStdWString();
        prefix->SetAttribute(
            attribute.c_str(), language.prefixKeywords[i] ? L"yes" : L"no");
    }
    TiXmlElement* folding =
        ensureChild(settings, L"FoldingInComment");
    folding->SetAttribute(
        L"fold", language.foldComments ? L"yes" : L"no");

    TiXmlElement* lists = ensureChild(target, L"KeywordLists");
    const QString preferredNames[28] = {
        QStringLiteral("Comments"),
        QStringLiteral("Numbers, prefix1"),
        QStringLiteral("Numbers, prefix2"),
        QStringLiteral("Numbers, extras1"),
        QStringLiteral("Numbers, extras2"),
        QStringLiteral("Numbers, suffix1"),
        QStringLiteral("Numbers, suffix2"),
        QStringLiteral("Numbers, range"),
        QStringLiteral("Operators1"),
        QStringLiteral("Operators2"),
        QStringLiteral("Folders in code1, open"),
        QStringLiteral("Folders in code1, middle"),
        QStringLiteral("Folders in code1, close"),
        QStringLiteral("Folders in code2, open"),
        QStringLiteral("Folders in code2, middle"),
        QStringLiteral("Folders in code2, close"),
        QStringLiteral("Folders in comment, open"),
        QStringLiteral("Folders in comment, middle"),
        QStringLiteral("Folders in comment, close"),
        QStringLiteral("Keywords1"),
        QStringLiteral("Keywords2"),
        QStringLiteral("Keywords3"),
        QStringLiteral("Keywords4"),
        QStringLiteral("Keywords5"),
        QStringLiteral("Keywords6"),
        QStringLiteral("Keywords7"),
        QStringLiteral("Keywords8"),
        QStringLiteral("Delimiters")
    };
    for (int index = 0; index < 28; ++index) {
        TiXmlElement* element = nullptr;
        for (TiXmlElement* item = lists->FirstChildElement(L"Keywords");
             item; item = item->NextSiblingElement(L"Keywords")) {
            if (udlKeywordListIndex(xmlAttr(item, L"name")) == index) {
                element = item;
                break;
            }
        }
        if (!element && language.keywordLists[index].isEmpty())
            continue;
        if (!element) {
            element = new TiXmlElement(L"Keywords");
            element->SetAttribute(
                L"name", preferredNames[index].toStdWString().c_str());
            lists->LinkEndChild(element);
        }
        while (element->FirstChild())
            element->RemoveChild(element->FirstChild());
        element->LinkEndChild(new TiXmlText(
            language.keywordLists[index].toStdWString().c_str()));
    }

    TiXmlElement* styles = ensureChild(target, L"Styles");
    for (const WordsStyle& style : language.styles) {
        TiXmlElement* element = nullptr;
        for (TiXmlElement* item = styles->FirstChildElement(L"WordsStyle");
             item; item = item->NextSiblingElement(L"WordsStyle")) {
            if (xmlAttr(item, L"name") == style.name) {
                element = item;
                break;
            }
        }
        if (!element) {
            element = new TiXmlElement(L"WordsStyle");
            element->SetAttribute(
                L"name", style.name.toStdWString().c_str());
            styles->LinkEndChild(element);
        }
        element->SetAttribute(
            L"styleID",
            style.xmlStyleID >= 0 ? style.xmlStyleID : style.styleID);
        if (style.hasFg)
            element->SetAttribute(
                L"fgColor",
                style.fgColor.name(QColor::HexRgb).mid(1).toUpper()
                    .toStdWString().c_str());
        if (style.hasBg)
            element->SetAttribute(
                L"bgColor",
                style.bgColor.name(QColor::HexRgb).mid(1).toUpper()
                    .toStdWString().c_str());
        element->SetAttribute(
            L"fontName", style.fontName.toStdWString().c_str());
        element->SetAttribute(L"fontStyle", style.fontStyle);
        element->SetAttribute(L"fontSize", style.fontSize);
        element->SetAttribute(L"nesting", style.nesting);
    }

    const bool saved =
        doc.SaveFile(sourceFilePath.toStdWString().c_str());
    if (saved)
        loadUserDefinedLanguages();
    return saved;
}

const UserLangDesc* NppParameters::getUserLangByName(const QString& name) const
{
    for (const UserLangDesc& lang : _userLangs)
        if (lang.name.compare(name, Qt::CaseInsensitive) == 0) return &lang;
    return nullptr;
}

const UserLangDesc* NppParameters::getUserLangByExt(const QString& ext) const
{
    QString normalized = ext.toLower();
    if (normalized.startsWith('.')) normalized.remove(0, 1);
    for (const UserLangDesc& lang : _userLangs)
        if (lang.exts.contains(normalized)) return &lang;
    return nullptr;
}

// ── 颜色解析辅助 ─────────────────────────────────────────────────────────────

static QColor parseHexColor(const QString& hex)
{
    if (hex.isEmpty()) return QColor();
    bool ok = false;
    // 原版格式：6位十六进制 RRGGBB（无 # 前缀）
    uint rgb = hex.toUInt(&ok, 16);
    if (!ok) return QColor();
    return QColor((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

// ── langs.xml ─────────────────────────────────────────────────────────────────

bool NppParameters::loadLangs()
{
    _langDescs.clear();

    // 与原版一致：优先用户目录，回退到 exe 目录的 langs.model.xml
    QString path = langsFilePath();
    if (!QFile::exists(path))
        path = _nppPath + "/langs.model.xml";
    if (!QFile::exists(path))
        path = ":/langs.model.xml";
    if (!QFile::exists(path))
        return false;

    TiXmlDocument doc;
    if (!loadTiXmlDoc(doc, path)) return false;

    TiXmlNode* root = doc.FirstChild(L"NotepadPlus");
    if (!root) return false;
    TiXmlNode* langs = root->FirstChildElement(L"Languages");
    if (!langs) return false;

    for (TiXmlNode* n = langs->FirstChildElement(L"Language"); n;
         n = n->NextSibling(L"Language")) {
        TiXmlElement* el = n->ToElement();
        if (!el) continue;
        LangDesc cur;
        cur.name         = txAttr(el, L"name").toLower();
        cur.commentLine  = txAttr(el, L"commentLine");
        cur.commentStart = txAttr(el, L"commentStart");
        cur.commentEnd   = txAttr(el, L"commentEnd");
        QString extStr   = txAttr(el, L"ext");
        for (const QString& e : extStr.split(' ', QString::SkipEmptyParts))
            cur.exts << e.toLower();
        // Keywords children
        for (TiXmlNode* kw = n->FirstChildElement(L"Keywords"); kw;
             kw = kw->NextSibling(L"Keywords")) {
            TiXmlElement* ke = kw->ToElement();
            if (!ke) continue;
            QString kwName = txAttr(ke, L"name");
            TiXmlNode* tc = ke->FirstChild();
            QString text = tc ? QString::fromWCharArray(tc->Value()).trimmed() : QString();
            if      (kwName == "instre1") cur.keywords[0] = text;
            else if (kwName == "instre2") cur.keywords[1] = text;
            else if (kwName == "type1")   cur.keywords[2] = text;
            else if (kwName == "type2")   cur.keywords[3] = text;
        }
        if (!cur.name.isEmpty())
            _langDescs.append(cur);
    }
    return !_langDescs.isEmpty();
}

const LangDesc* NppParameters::getLangDescByExt(const QString& ext) const
{
    QString e = ext.toLower();
    for (const LangDesc& d : _langDescs)
        if (d.exts.contains(e)) return &d;
    return nullptr;
}

const LangDesc* NppParameters::getLangDescByName(const QString& name) const
{
    QString n = name.toLower();
    for (const LangDesc& d : _langDescs)
        if (d.name == n) return &d;
    return nullptr;
}

// ── stylers.xml ───────────────────────────────────────────────────────────────

bool NppParameters::loadStylers()
{
    _lexerStylers.clear();
    _globalStyles.clear();

    QString path = stylersFilePath();
    if (!QFile::exists(path))
        path = _nppPath + "/stylers.model.xml";
    if (!QFile::exists(path))
        path = ":/stylers.model.xml";   // Qt 资源回退（内嵌于 exe）
    if (!QFile::exists(path))
        return false;

    TiXmlDocument doc;
    if (!loadTiXmlDoc(doc, path)) return false;

    TiXmlNode* root = doc.FirstChild(L"NotepadPlus");
    if (!root) return false;

    TiXmlNode* stylers = root->FirstChildElement(L"LexerStyles");
    if (stylers) {
        for (TiXmlNode* ln = stylers->FirstChildElement(L"LexerType"); ln;
             ln = ln->NextSibling(L"LexerType")) {
            TiXmlElement* le = ln->ToElement();
            if (!le) continue;
            LexerStyler curLexer;
            curLexer.name = txAttr(le, L"name").toLower();
            curLexer.desc = txAttr(le, L"desc");
            for (TiXmlNode* wn = ln->FirstChildElement(L"WordsStyle"); wn;
                 wn = wn->NextSibling(L"WordsStyle")) {
                TiXmlElement* we = wn->ToElement();
                if (!we) continue;
                WordsStyle s;
                s.styleID   = parseInt(txAttr(we, L"styleID"), 0);
                s.name      = txAttr(we, L"name");
                s.fontName  = txAttr(we, L"fontName");
                s.fontStyle = parseInt(txAttr(we, L"fontStyle"), 0);
                QString fs  = txAttr(we, L"fontSize");
                s.fontSize  = fs.isEmpty() ? 0 : fs.toInt();
                QString fg  = txAttr(we, L"fgColor");
                QString bg  = txAttr(we, L"bgColor");
                if (!fg.isEmpty()) { s.fgColor = parseHexColor(fg); s.hasFg = s.fgColor.isValid(); }
                if (!bg.isEmpty()) { s.bgColor = parseHexColor(bg); s.hasBg = s.bgColor.isValid(); }
                curLexer.styles.append(s);
            }
            if (!curLexer.name.isEmpty())
                _lexerStylers.append(curLexer);
        }
    }

    TiXmlNode* globalStyles = root->FirstChildElement(L"GlobalStyles");
    if (globalStyles) {
        for (TiXmlNode* wn = globalStyles->FirstChildElement(L"WidgetStyle"); wn;
             wn = wn->NextSibling(L"WidgetStyle")) {
            TiXmlElement* we = wn->ToElement();
            if (!we) continue;
            WordsStyle s;
            s.styleID   = parseInt(txAttr(we, L"styleID"), 0);
            s.name      = txAttr(we, L"name");
            s.fontName  = txAttr(we, L"fontName");
            s.fontStyle = parseInt(txAttr(we, L"fontStyle"), 0);
            QString fs  = txAttr(we, L"fontSize");
            s.fontSize  = fs.isEmpty() ? 0 : fs.toInt();
            QString fg  = txAttr(we, L"fgColor");
            QString bg  = txAttr(we, L"bgColor");
            if (!fg.isEmpty()) { s.fgColor = parseHexColor(fg); s.hasFg = s.fgColor.isValid(); }
            if (!bg.isEmpty()) { s.bgColor = parseHexColor(bg); s.hasBg = s.bgColor.isValid(); }
            _globalStyles.append(s);
        }
    }

    return !_lexerStylers.isEmpty();
}

const LexerStyler* NppParameters::getLexerStyler(const QString& nppLexerName) const
{
    QString n = nppLexerName.toLower();
    for (const LexerStyler& ls : _lexerStylers)
        if (ls.name == n) return &ls;
    return nullptr;
}

// ── shortcuts.xml ─────────────────────────────────────────────────────────────

bool NppParameters::loadShortcuts()
{
    _macros.clear();
    _userCommands.clear();
    _internalCommandShortcuts.clear();
    _scintillaKeys.clear();

    QString path = shortcutsFilePath();
    if (!QFile::exists(path))
        path = _nppPath + "/shortcuts.xml";
    if (!QFile::exists(path))
        path = ":/shortcuts.xml";
    if (!QFile::exists(path))
        return false;  // 无 shortcuts.xml 则无自定义宏，不是错误

    TiXmlDocument doc;
    if (!loadTiXmlDoc(doc, path)) return false;

    TiXmlNode* root = doc.FirstChild(L"NotepadPlus");
    if (!root) return false;
    auto readShortcut = [](TiXmlElement* element) {
        ShortcutKey key;
        key.ctrl = txAttr(element, L"Ctrl") == "yes";
        key.alt = txAttr(element, L"Alt") == "yes";
        key.shift = txAttr(element, L"Shift") == "yes";
        key.key = parseInt(txAttr(element, L"Key"), 0);
        return key;
    };
    TiXmlNode* internal = root->FirstChildElement(L"InternalCommands");
    for (TiXmlNode* node = internal
             ? internal->FirstChildElement(L"Shortcut") : nullptr;
         node; node = node->NextSiblingElement(L"Shortcut")) {
        TiXmlElement* element = node->ToElement();
        InternalCommandShortcut command;
        command.id = parseInt(txAttr(element, L"id"), 0);
        command.shortcut = readShortcut(element);
        if (command.id != 0)
            _internalCommandShortcuts.append(command);
    }
    TiXmlNode* scintilla = root->FirstChildElement(L"ScintillaKeys");
    for (TiXmlNode* node = scintilla
             ? scintilla->FirstChildElement(L"ScintKey") : nullptr;
         node; node = node->NextSiblingElement(L"ScintKey")) {
        TiXmlElement* element = node->ToElement();
        ScintillaKeyDef command;
        command.scintillaId = parseInt(txAttr(element, L"ScintID"), 0);
        command.menuCommandId = parseInt(txAttr(element, L"menuCmdID"), 0);
        command.shortcuts.append(readShortcut(element));
        for (TiXmlNode* next = node->FirstChildElement(L"NextKey"); next;
             next = next->NextSiblingElement(L"NextKey"))
            command.shortcuts.append(readShortcut(next->ToElement()));
        if (command.scintillaId != 0)
            _scintillaKeys.append(command);
    }
    TiXmlNode* macros = root->FirstChildElement(L"Macros");

    for (TiXmlNode* mn = macros ? macros->FirstChildElement(L"Macro") : nullptr;
         mn; mn = mn->NextSibling(L"Macro")) {
        TiXmlElement* me = mn->ToElement();
        if (!me) continue;
        MacroDef cur;
        cur.name  = txAttr(me, L"name");
        cur.ctrl  = txAttr(me, L"Ctrl")  == "yes";
        cur.alt   = txAttr(me, L"Alt")   == "yes";
        cur.shift = txAttr(me, L"Shift") == "yes";
        cur.key   = parseInt(txAttr(me, L"Key"), 0);
        for (TiXmlNode* an = mn->FirstChildElement(L"Action"); an;
             an = an->NextSibling(L"Action")) {
            TiXmlElement* ae = an->ToElement();
            if (!ae) continue;
            MacroAction act;
            act.type    = parseInt(txAttr(ae, L"type"),    0);
            act.message = parseInt(txAttr(ae, L"message"), 0);
            act.wParam  = parseInt(txAttr(ae, L"wParam"),  0);
            act.lParam  = parseInt(txAttr(ae, L"lParam"),  0);
            act.sParam  = txAttr(ae, L"sParam");
            cur.actions.append(act);
        }
        if (!cur.name.isEmpty())
            _macros.append(cur);
    }

    TiXmlNode* commands = root->FirstChildElement(L"UserDefinedCommands");
    for (TiXmlNode* cn = commands ? commands->FirstChildElement(L"Command") : nullptr;
         cn; cn = cn->NextSibling(L"Command")) {
        TiXmlElement* element = cn->ToElement();
        if (!element)
            continue;
        UserCommandDef command;
        command.name = txAttr(element, L"name");
        command.command = txText(element);
        command.ctrl = txAttr(element, L"Ctrl") == "yes";
        command.alt = txAttr(element, L"Alt") == "yes";
        command.shift = txAttr(element, L"Shift") == "yes";
        command.key = parseInt(txAttr(element, L"Key"), 0);
        if (!command.name.isEmpty() && !command.command.isEmpty())
            _userCommands.append(command);
    }
    return true;
}

bool NppParameters::writeShortcuts()
{
    if (!ensureConfigDir())
        return false;
    ensureDefaultXmlFiles();

    TiXmlDocument doc;
    if (!loadTiXmlDoc(doc, shortcutsFilePath())) {
        doc.Clear();
        doc.LinkEndChild(new TiXmlDeclaration(L"1.0", L"UTF-8", L""));
        doc.LinkEndChild(new TiXmlElement(L"NotepadPlus"));
    }

    TiXmlElement* root = doc.FirstChildElement(L"NotepadPlus");
    if (!root) {
        root = new TiXmlElement(L"NotepadPlus");
        doc.LinkEndChild(root);
    }

    auto writeShortcut = [](TiXmlElement* element, const ShortcutKey& key) {
        element->SetAttribute(L"Ctrl", key.ctrl ? L"yes" : L"no");
        element->SetAttribute(L"Alt", key.alt ? L"yes" : L"no");
        element->SetAttribute(L"Shift", key.shift ? L"yes" : L"no");
        element->SetAttribute(L"Key", key.key);
    };
    TiXmlElement* internal = root->FirstChildElement(L"InternalCommands");
    if (!internal && !_internalCommandShortcuts.isEmpty()) {
        internal = new TiXmlElement(L"InternalCommands");
        root->LinkEndChild(internal);
    }
    for (const InternalCommandShortcut& command : _internalCommandShortcuts) {
        TiXmlElement* shortcut = nullptr;
        for (TiXmlNode* node = internal->FirstChildElement(L"Shortcut");
             node; node = node->NextSiblingElement(L"Shortcut")) {
            TiXmlElement* candidate = node->ToElement();
            if (parseInt(txAttr(candidate, L"id"), 0) == command.id) {
                shortcut = candidate;
                break;
            }
        }
        if (!shortcut) {
            shortcut = new TiXmlElement(L"Shortcut");
            shortcut->SetAttribute(L"id", command.id);
            internal->LinkEndChild(shortcut);
        }
        writeShortcut(shortcut, command.shortcut);
    }

    TiXmlElement* scintilla = root->FirstChildElement(L"ScintillaKeys");
    if (!scintilla && !_scintillaKeys.isEmpty()) {
        scintilla = new TiXmlElement(L"ScintillaKeys");
        root->LinkEndChild(scintilla);
    }
    for (const ScintillaKeyDef& command : _scintillaKeys) {
        TiXmlElement* key = nullptr;
        for (TiXmlNode* node = scintilla->FirstChildElement(L"ScintKey");
             node; node = node->NextSiblingElement(L"ScintKey")) {
            TiXmlElement* candidate = node->ToElement();
            if (parseInt(txAttr(candidate, L"ScintID"), 0) ==
                    command.scintillaId &&
                parseInt(txAttr(candidate, L"menuCmdID"), 0) ==
                    command.menuCommandId) {
                key = candidate;
                break;
            }
        }
        if (!key) {
            key = new TiXmlElement(L"ScintKey");
            key->SetAttribute(L"ScintID", command.scintillaId);
            key->SetAttribute(L"menuCmdID", command.menuCommandId);
            scintilla->LinkEndChild(key);
        }
        if (!command.shortcuts.isEmpty())
            writeShortcut(key, command.shortcuts.first());
        for (TiXmlNode* node = key->FirstChildElement(L"NextKey"); node; ) {
            TiXmlNode* next = node->NextSiblingElement(L"NextKey");
            key->RemoveChild(node);
            node = next;
        }
        for (int i = 1; i < command.shortcuts.size(); ++i) {
            TiXmlElement* next = new TiXmlElement(L"NextKey");
            writeShortcut(next, command.shortcuts.at(i));
            key->LinkEndChild(next);
        }
    }

    TiXmlElement* macros = root->FirstChildElement(L"Macros");
    if (!macros && !_macros.isEmpty()) {
        macros = new TiXmlElement(L"Macros");
        root->LinkEndChild(macros);
    }
    for (const MacroDef& m : _macros) {
        TiXmlElement* macro = nullptr;
        for (TiXmlNode* node = macros->FirstChildElement(L"Macro"); node;
             node = node->NextSiblingElement(L"Macro")) {
            TiXmlElement* candidate = node->ToElement();
            if (txAttr(candidate, L"name") == m.name) {
                macro = candidate;
                break;
            }
        }
        const bool isNew = !macro;
        if (isNew)
            macro = new TiXmlElement(L"Macro");
        macro->SetAttribute(L"name",  m.name.toStdWString().c_str());
        macro->SetAttribute(L"Ctrl",  m.ctrl  ? L"yes" : L"no");
        macro->SetAttribute(L"Alt",   m.alt   ? L"yes" : L"no");
        macro->SetAttribute(L"Shift", m.shift ? L"yes" : L"no");
        macro->SetAttribute(L"Key",   m.key);
        if (isNew) {
            for (const MacroAction& a : m.actions) {
                TiXmlElement* action = new TiXmlElement(L"Action");
                action->SetAttribute(L"type",    a.type);
                action->SetAttribute(L"message", a.message);
                action->SetAttribute(L"wParam",  a.wParam);
                action->SetAttribute(L"lParam",  a.lParam);
                action->SetAttribute(
                    L"sParam", a.sParam.toStdWString().c_str());
                macro->LinkEndChild(action);
            }
            macros->LinkEndChild(macro);
        }
    }

    TiXmlElement* userCommands =
        root->FirstChildElement(L"UserDefinedCommands");
    if (!userCommands && !_userCommands.isEmpty()) {
        userCommands = new TiXmlElement(L"UserDefinedCommands");
        root->LinkEndChild(userCommands);
    }
    for (const UserCommandDef& command : _userCommands) {
        TiXmlElement* element = nullptr;
        for (TiXmlNode* node =
                 userCommands->FirstChildElement(L"Command");
             node; node = node->NextSiblingElement(L"Command")) {
            TiXmlElement* candidate = node->ToElement();
            if (txAttr(candidate, L"name") == command.name) {
                element = candidate;
                break;
            }
        }
        if (!element) {
            element = new TiXmlElement(L"Command");
            element->SetAttribute(
                L"name", command.name.toStdWString().c_str());
            element->LinkEndChild(
                new TiXmlText(command.command.toStdWString().c_str()));
            userCommands->LinkEndChild(element);
        }
        element->SetAttribute(L"Ctrl", command.ctrl ? L"yes" : L"no");
        element->SetAttribute(L"Alt", command.alt ? L"yes" : L"no");
        element->SetAttribute(L"Shift", command.shift ? L"yes" : L"no");
        element->SetAttribute(L"Key", command.key);
    }

    return doc.SaveFile(shortcutsFilePath().toStdWString().c_str());
}

void NppParameters::setInternalCommandShortcut(
    int id, const QKeySequence& sequence)
{
    for (InternalCommandShortcut& command : _internalCommandShortcuts) {
        if (command.id == id) {
            command.shortcut = ShortcutKey::fromKeySequence(sequence);
            return;
        }
    }
    InternalCommandShortcut command;
    command.id = id;
    command.shortcut = ShortcutKey::fromKeySequence(sequence);
    _internalCommandShortcuts.append(command);
}

void NppParameters::migrateFromQSettings()
{
    // 优先尝试从旧版 config.xml 迁移界面语言（路径切换前的配置）
    // Windows: %LOCALAPPDATA%\Notepad++ Qt Project\Notepad++ Qt\config.xml
    QString localAppData = qEnvironmentVariable("LOCALAPPDATA");
    QString oldConfigPath = localAppData + "/Notepad++ Qt Project/Notepad++ Qt/config.xml";
    if (QFile::exists(oldConfigPath)) {
        TiXmlDocument doc;
        if (loadTiXmlDoc(doc, oldConfigPath)) {
            TiXmlNode* root = doc.FirstChild(L"NotepadPlus");
            if (root) {
                TiXmlNode* guiConfigs = root->FirstChildElement(L"GUIConfigs");
                if (guiConfigs) {
                    for (TiXmlNode* n = guiConfigs->FirstChildElement(L"GUIConfig"); n;
                         n = n->NextSibling(L"GUIConfig")) {
                        TiXmlElement* el = n->ToElement();
                        if (!el) continue;
                        QString name = txAttr(el, L"name");
                        if (name == "UILanguage") {
                            QString lang = txAttr(el, L"lang");
                            if (lang.isEmpty())
                                lang = txText(n).trimmed();
                            setNativeLang(lang);
                        }
                    }
                }
            }
        }
        writeNppGUI();
        return; // 迁移完成，无需读 QSettings
    }

    QSettings s("NotepadPlusPlusQt", "NotepadPlusPlusQt");
    if (s.allKeys().isEmpty())
        return; // 无旧配置，无需迁移

    // 界面语言
    if (s.contains("UI/language"))
        setNativeLang(s.value("UI/language", "en").toString());

    // 编辑器设置
    if (s.contains("Editor/fontFamily"))
        _nppGUI._editorFontName = s.value("Editor/fontFamily", "Consolas").toString();
    if (s.contains("Editor/fontSize"))
        _nppGUI._editorFontSize = s.value("Editor/fontSize", 10).toInt();
    if (s.contains("Editor/tabWidth"))
        _nppGUI._tabSize = s.value("Editor/tabWidth", 4).toInt();
    if (s.contains("Editor/useSpaces"))
        _nppGUI._tabReplacedBySpace = s.value("Editor/useSpaces", false).toBool();
    if (s.contains("Editor/autoIndent"))
        _nppGUI._autoIndent = s.value("Editor/autoIndent", true).toBool();
    if (s.contains("Editor/wordWrap"))
        _svp._doWrap = s.value("Editor/wordWrap", false).toBool();
    if (s.contains("Editor/autoComplete"))
        _nppGUI._autoCompleteEnable = s.value("Editor/autoComplete", true).toBool();
    if (s.contains("Editor/autoCompleteThreshold"))
        _nppGUI._autoCompleteThreshold = s.value("Editor/autoCompleteThreshold", 3).toInt();

    // 显示设置
    if (s.contains("Display/showWhitespace"))
        _svp._whiteSpaceShow = s.value("Display/showWhitespace", false).toBool();
    if (s.contains("Display/showEol"))
        _svp._eolShow = s.value("Display/showEol", false).toBool();

    // 会话设置
    if (s.contains("Session/restore"))
        _nppGUI._rememberLastSession = s.value("Session/restore", true).toBool();

    // 最近文件
    if (s.contains("recentFiles"))
        _nppGUI._recentFileList = s.value("recentFiles").toStringList();

    // 窗口状态
    if (s.contains("Window/geometry")) {
        // restoreGeometry 数据无法直接映射到 QRect，跳过，让程序用默认值
    }
    if (s.contains("Window/state"))
        _nppGUI._windowState = s.value("Window/state").toByteArray();

    // 迁移完成后立即写入 config.xml 持久化
    writeNppGUI();
}

// ── 读取 config.xml ───────────────────────────────────────────────────────────

bool NppParameters::loadConfig()
{
    TiXmlDocument doc;
    if (!loadTiXmlDoc(doc, configFilePath()))
        return false;

    TiXmlNode* root = doc.FirstChild(L"NotepadPlus");
    if (!root) return false;

    // GUIConfig elements are under GUIConfigs
    TiXmlNode* guiConfigs = root->FirstChildElement(L"GUIConfigs");
    if (guiConfigs) {
        for (TiXmlNode* n = guiConfigs->FirstChildElement(L"GUIConfig");
             n; n = n->NextSibling(L"GUIConfig")) {
            TiXmlElement* el = n->ToElement();
            if (el) feedGUIConfig(el);
        }
    }

    // History
    TiXmlNode* histNode = root->FirstChildElement(L"History");
    if (histNode) {
        TiXmlElement* hist = histNode->ToElement();
        if (hist) {
            _nppGUI._nbMaxRecentFile        = parseInt(txAttr(hist, L"nbMaxFile"), 10);
            _nppGUI._putRecentFileInSubMenu = parseBool(txAttr(hist, L"inSubMenu"), false);
            _nppGUI._recentFileList.clear();
            for (TiXmlNode* fn = hist->FirstChildElement(L"File"); fn;
                 fn = fn->NextSibling(L"File")) {
                TiXmlElement* fe = fn->ToElement();
                if (!fe) continue;
                QString name = txAttr(fe, L"filename");
                if (!name.isEmpty() && _nppGUI._recentFileList.size() < _nppGUI._nbMaxRecentFile)
                    _nppGUI._recentFileList.append(name);
            }
        }
    }

    TiXmlNode* projectPanels = root->FirstChildElement(L"ProjectPanels");
    for (TiXmlNode* node = projectPanels
             ? projectPanels->FirstChildElement(L"ProjectPanel") : nullptr;
         node; node = node->NextSiblingElement(L"ProjectPanel")) {
        TiXmlElement* panel = node->ToElement();
        const int id = parseInt(txAttr(panel, L"id"), -1);
        if (id >= 0 && id < 3)
            _workspaceFilePaths[id] = txAttr(panel, L"workSpaceFile");
    }

    loadFindHistory(root);
    return true;
}

void NppParameters::loadFindHistory(TiXmlNode* root)
{
    if (!root) return;

    TiXmlNode* node = root->FirstChildElement(L"FindHistory");
    if (!node) return;
    TiXmlElement* el = node->ToElement();
    if (!el) return;

    _findHistory.nbMaxPath = parseInt(txAttr(el, L"nbMaxFindHistoryPath"), 10);
    _findHistory.nbMaxFilter = parseInt(txAttr(el, L"nbMaxFindHistoryFilter"), 10);
    _findHistory.nbMaxFind = parseInt(txAttr(el, L"nbMaxFindHistoryFind"), 10);
    _findHistory.nbMaxReplace = parseInt(txAttr(el, L"nbMaxFindHistoryReplace"), 10);

    _findHistory.matchWord = (txAttr(el, L"matchWord") == "yes");
    _findHistory.matchCase = (txAttr(el, L"matchCase") == "yes");
    _findHistory.wrap = (txAttr(el, L"wrap") != "no");
    _findHistory.directionDown = (txAttr(el, L"directionDown") != "no");
    _findHistory.fifRecursive = (txAttr(el, L"fifRecuisive") != "no");
    _findHistory.fifInHiddenFolder = (txAttr(el, L"fifInHiddenFolder") == "yes");
    _findHistory.fifFilterFollowsDoc = (txAttr(el, L"fifFilterFollowsDoc") == "yes");
    _findHistory.fifFolderFollowsDoc = (txAttr(el, L"fifFolderFollowsDoc") == "yes");
    _findHistory.fifProjectPanel1 = (txAttr(el, L"fifProjectPanel1") == "yes");
    _findHistory.fifProjectPanel2 = (txAttr(el, L"fifProjectPanel2") == "yes");
    _findHistory.fifProjectPanel3 = (txAttr(el, L"fifProjectPanel3") == "yes");
    _findHistory.searchMode = parseInt(txAttr(el, L"searchMode"), 0);
    _findHistory.transparencyMode = parseInt(txAttr(el, L"transparencyMode"), 1);
    _findHistory.transparency = parseInt(txAttr(el, L"transparency"), 150);
    _findHistory.dotMatchesNewline = (txAttr(el, L"dotMatchesNewline") == "yes");
    _findHistory.isSearch2ButtonsMode = (txAttr(el, L"isSearch2ButtonsMode") == "yes");

    auto loadList = [](TiXmlNode* parent, const wchar_t* tag, int maxItems) {
        QStringList list;
        for (TiXmlNode* n = parent->FirstChildElement(tag);
             n && list.size() < maxItems;
             n = n->NextSibling(tag)) {
            TiXmlElement* item = n->ToElement();
            if (!item) continue;
            QString name = txAttr(item, L"name");
            if (!name.isEmpty())
                list.append(name);
        }
        return list;
    };

    _findHistory.paths = loadList(node, L"Path", _findHistory.nbMaxPath);
    _findHistory.filters = loadList(node, L"Filter", _findHistory.nbMaxFilter);
    _findHistory.finds = loadList(node, L"Find", _findHistory.nbMaxFind);
    _findHistory.replaces = loadList(node, L"Replace", _findHistory.nbMaxReplace);
}

// 分发各 GUIConfig 配置项（对应原版 feedGUIParameters）
void NppParameters::feedGUIConfig(const TiXmlElement* el)
{
    QString name = txAttr(el, L"name");
    auto attr = [&](const wchar_t* key) { return txAttr(el, key); };
    QString content = txText(el).trimmed();

    if (name == "ToolBar") {
        _nppGUI._toolBarShow = parseBool(attr(L"visible"), true);
    }
    else if (name == "StatusBar") {
        _nppGUI._statusBarShow = (content != "hide");
    }
    else if (name == "MenuBar") {
        _nppGUI._menuBarShow = (content != "hide");
    }
    else if (name == "TabBar") {
        _nppGUI._tabDragAndDrop     = parseBool(attr(L"dragAndDrop"),      true);
        _nppGUI._tabDrawTopBar      = parseBool(attr(L"drawTopBar"),       true);
        _nppGUI._tabDrawInactiveTab = parseBool(attr(L"drawInactiveTab"),  true);
        _nppGUI._tabCloseButton     = parseBool(attr(L"closeButton"),      true);
        _nppGUI._tabDbclkToClose    = parseBool(attr(L"doubleClick2Close"),false);
        _nppGUI._tabVertical        = parseBool(attr(L"vertical"),         false);
        _nppGUI._tabMultiLine       = parseBool(attr(L"multiLine"),        false);
        _nppGUI._tabHide            = parseBool(attr(L"hide"),             false);
        _nppGUI._tabQuitOnEmpty     = parseBool(attr(L"quitOnEmpty"),      false);
        _nppGUI._tabIconSetNumber   = parseInt(attr(L"iconSetNumber"),     0);
    }
    else if (name == "TabSetting") {
        _nppGUI._tabSize            = parseInt(attr(L"size"),           4);
        _nppGUI._tabReplacedBySpace = parseBool(attr(L"replaceBySpace"),false);
    }
    else if (name == "AppPosition") {
        int x = parseInt(attr(L"x"),      10);
        int y = parseInt(attr(L"y"),      10);
        int w = parseInt(attr(L"width"),  1024);
        int h = parseInt(attr(L"height"), 768);
        _nppGUI._appPos      = QRect(x, y, w, h);
        _nppGUI._isMaximized = parseBool(attr(L"isMaximized"), false);
    }
    else if (name == "FindWindowPosition") {
        _nppGUI._findWinLeft   = parseInt(attr(L"left"),   100);
        _nppGUI._findWinTop    = parseInt(attr(L"top"),    100);
        _nppGUI._findWinRight  = parseInt(attr(L"right"),  500);
        _nppGUI._findWinBottom = parseInt(attr(L"bottom"), 400);
    }
    else if (name == "RememberLastSession") {
        _nppGUI._rememberLastSession = (content == "yes" || content == "1");
    }
    else if (name == "NewDocDefaultSettings") {
        _nppGUI._newDocDefaultFormat   = parseInt(attr(L"format"),   2);
        _nppGUI._newDocDefaultEncoding = parseInt(attr(L"encoding"), 0);
        _nppGUI._newDocDefaultLang     = parseInt(attr(L"lang"),     0);
        _nppGUI._openAnsiAsUtf8 =
            parseBool(attr(L"openAnsiAsUTF8"), true);
    }
    else if (name == "DetectEncoding") {
        _nppGUI._detectEncoding =
            (content == "yes" || content == "1");
    }
    else if (name == "Backup") {
        _nppGUI._backup               = parseInt(attr(L"action"),               0);
        _nppGUI._useBackupDir         = parseBool(attr(L"useCustumDir"),        false);
        _nppGUI._backupDir            = attr(L"dir");
        _nppGUI._isSnapshotMode       = parseBool(attr(L"isSnapshotMode"),      true);
        _nppGUI._snapshotBackupTiming = parseInt(attr(L"snapshotBackupTiming"), 7000);
    }
    else if (name == "Auto-detection") {
        _nppGUI._fileAutoDetection =
            (content == "yes" || content == "yesOld" || content == "auto") ? 1 : 0;
    }
    else if (name == "CheckHistoryFiles") {
        _nppGUI._checkHistoryFiles = (content == "yes");
    }
    else if (name == "ScintillaViewsSplitter") {
        _nppGUI._isVerticalSplit = (content == "vertical");
    }
    else if (name == "ScintillaPrimaryView") {
        _svp._lineNumberMarginShow    = (attr(L"lineNumberMargin")        != "hide");
        _svp._bookMarkMarginShow      = (attr(L"bookMarkMargin")          != "hide");
        _svp._indentGuideLineShow     = (attr(L"indentGuideLine")         != "hide");
        const QString currentLine = attr(L"currentLineHilitingShow");
        _svp._currentLineHilitingShow = currentLine.isEmpty()
            ? (parseInt(attr(L"currentLineIndicator"), 1) != 0 ||
               parseInt(attr(L"currentLineFrameWidth"), 0) > 0)
            : (currentLine != "hide");
        _svp._wrapSymbolShow          = (attr(L"wrapSymbolShow")           == "show");
        _svp._doWrap                  = parseBool(attr(L"Wrap"),           false);
        const QString edge = attr(L"edge");
        _svp._edgeShow = parseBool(edge, false);
        const QString edgeColumn = attr(L"edgeNbColumn");
        if (!edgeColumn.isEmpty()) {
            _svp._edgeNbColumn = parseInt(edgeColumn, 80);
        } else {
            const QStringList columns =
                attr(L"edgeMultiColumnPos").split(
                    QRegExp("[,; ]+"), QString::SkipEmptyParts);
            _svp._edgeNbColumn =
                columns.isEmpty() ? 80 : parseInt(columns.first(), 80);
        }
        _svp._zoom                    = parseInt(attr(L"zoom"),            0);
        _svp._zoom2                   = parseInt(attr(L"zoom2"),           0);
        _svp._whiteSpaceShow          = (attr(L"whiteSpaceShow") == "show");
        _svp._eolShow                 = (attr(L"eolShow")        == "show");
        _svp._scrollBeyondLastLine    = parseBool(attr(L"scrollBeyondLastLine"), false);
    }
    else if (name == "auto-completion") {
        _nppGUI._autocAction        = parseInt(attr(L"autoCAction"),       0);
        _nppGUI._autocFromNbChar    = parseInt(attr(L"triggerFromNbChar"), 3);
        _nppGUI._autocIgnoreNumbers = parseBool(attr(L"autoCIgnoreNumbers"), true);
        _nppGUI._funcParams         = parseBool(attr(L"funcParams"),         true);
    }
    else if (name == "openSaveDir") {
        _nppGUI._openSaveDir = parseInt(attr(L"value"), 0);
        _nppGUI._defaultDirPath = attr(L"defaultDirPath");
    }
    else if (name == "SmartHighLight") {
        _nppGUI._smartHighlight = (content != "no");
        _nppGUI._smartHighlightMatchCase = parseBool(attr(L"matchCase"), false);
        _nppGUI._smartHighlightWholeWord = parseBool(attr(L"wholeWordOnly"), true);
        _nppGUI._smartHighlightUseFindSettings =
            parseBool(attr(L"useFindSettings"), false);
        _nppGUI._smartHighlightAnotherView =
            parseBool(attr(L"onAnotherView"), false);
    }
    else if (name == "Print") {
        _nppGUI._printLineNumber = parseBool(attr(L"lineNumber"), true);
        _nppGUI._printOption = parseInt(attr(L"printOption"), 3);
        _nppGUI._printHeaderLeft = attr(L"headerLeft");
        _nppGUI._printHeaderMiddle = attr(L"headerMiddle");
        _nppGUI._printHeaderRight = attr(L"headerRight");
        _nppGUI._printFooterLeft = attr(L"footerLeft");
        _nppGUI._printFooterMiddle = attr(L"footerMiddle");
        _nppGUI._printFooterRight = attr(L"footerRight");
    }
    else if (name == "multiInst") {
        _nppGUI._multiInstSetting = parseInt(attr(L"setting"), 0);
    }
    else if (name == "DateTime" || name == "insertDateTime") {
        QString format = attr(L"customizedFormat");
        if (!format.isEmpty())
            _nppGUI._dateTimeFormat = format;
        _nppGUI._dateTimeReverseDefaultOrder =
            parseBool(attr(L"reverseDefaultOrder"), false);
    }
    else if (name == "delimiterSelection") {
        _nppGUI._leftmostDelimiter = parseInt(attr(L"leftmostDelimiter"), 40);
        _nppGUI._rightmostDelimiter = parseInt(attr(L"rightmostDelimiter"), 41);
        _nppGUI._delimiterSelectionOnEntireDocument =
            parseBool(attr(L"delimiterSelectionOnEntireDocument"), false);
    }
    else if (name == "URL") {
        _nppGUI._urlMode = parseInt(content, 2);
    }
    else if (name == "uriCustomizedSchemes") {
        _nppGUI._uriCustomizedSchemes = content;
    }
    else if (name == "TagsMatchHighLight") {
        _nppGUI._enableTagsMatchHighlight = (content != "no");
        _nppGUI._enableTagAttrsHighlight =
            parseBool(attr(L"TagAttrHighLight"), true);
        _nppGUI._highlightNonHtmlZone =
            parseBool(attr(L"HighLightNonHtmlZone"), false);
    }
    else if (name == "auto-insert") {
        _nppGUI._autoInsertParentheses =
            parseBool(attr(L"parentheses"), false);
        _nppGUI._autoInsertBrackets =
            parseBool(attr(L"brackets"), false);
        _nppGUI._autoInsertCurlyBrackets =
            parseBool(attr(L"curlyBrackets"), false);
        _nppGUI._autoInsertQuotes = parseBool(attr(L"quotes"), false);
        _nppGUI._autoInsertDoubleQuotes =
            parseBool(attr(L"doubleQuotes"), false);
        _nppGUI._autoInsertHtmlXmlTag =
            parseBool(attr(L"htmlXmlTag"), false);
    }
    else if (name == "searchEngine") {
        _nppGUI._searchEngineChoice = parseInt(attr(L"searchEngineChoice"), 2);
        _nppGUI._searchEngineCustom = attr(L"searchEngineCustom");
    }
    // ── Qt 移植扩展配置 ────────────────────────────────────────────────────────
    else if (name == "EditorFont") {
        QString fn = attr(L"fontName");
        _nppGUI._editorFontName = fn.isEmpty() ? "Consolas" : fn;
        _nppGUI._editorFontSize = parseInt(attr(L"fontSize"), 10);
    }
    else if (name == "EditorSettings") {
        _nppGUI._autoIndent             = parseBool(attr(L"autoIndent"),         true);
        _nppGUI._autoCompleteEnable     = parseBool(attr(L"autoCompleteEnable"), true);
        _nppGUI._autoCompleteThreshold  = parseInt(attr(L"autoCompleteFrom"),    3);
        _nppGUI._showWhitespace         = parseBool(attr(L"showWhitespace"),     false);
        _nppGUI._showEol                = parseBool(attr(L"showEol"),            false);
        _nppGUI._restoreSession         = parseBool(attr(L"restoreSession"),     true);
        _nppGUI._darkModeEnabled        = parseBool(attr(L"darkMode"),           false);
    }
    else if (name == "WindowState") {
        if (!content.isEmpty())
            _nppGUI._windowState = QByteArray::fromBase64(content.toLatin1());
    }
}

// ── 读取 session.xml ──────────────────────────────────────────────────────────

static void parseSessionView(TiXmlNode* viewNode,
                              std::vector<sessionFileInfo>& files,
                              size_t& activeIndex)
{
    TiXmlElement* viewEl = viewNode ? viewNode->ToElement() : nullptr;
    if (!viewEl) return;

    activeIndex = static_cast<size_t>(
        parseInt(txAttr(viewEl, L"activeIndex"), 0));

    for (TiXmlNode* fn = viewNode->FirstChildElement(L"File"); fn;
         fn = fn->NextSibling(L"File")) {
        TiXmlElement* fe = fn->ToElement();
        if (!fe) continue;

        sessionFileInfo sfi;
        sfi._fileName         = txAttr(fe, L"filename");
        sfi._langName         = txAttr(fe, L"lang");
        sfi._encoding         = parseInt(txAttr(fe, L"encoding"), -1);
        sfi._userReadOnly     = (txAttr(fe, L"userReadOnly") == "yes");
        sfi._backupFilePath   = txAttr(fe, L"backupFilePath");
        sfi._originalFileLastModifTimestamp =
            txAttr(fe, L"originalFileLastModifTimestamp").toLongLong();
        sfi._originalFileLastModifTimestampHigh =
            txAttr(fe, L"originalFileLastModifTimestampHigh").toLongLong();
        {
            QString v = txAttr(fe, L"tabColourId");
            sfi._individualTabColour = v.isEmpty() ? -1 : v.toInt();
        }

        // DocumentMap 位置
        {
            QString v = txAttr(fe, L"mapFirstVisibleDisplayLine");
            sfi._mapFirstVisibleDisplayLine = v.isEmpty() ? -1 : v.toInt();
        }
        {
            QString v = txAttr(fe, L"mapFirstVisibleDocLine");
            sfi._mapFirstVisibleDocLine = v.isEmpty() ? -1 : v.toInt();
        }
        {
            QString v = txAttr(fe, L"mapLastVisibleDocLine");
            sfi._mapLastVisibleDocLine = v.isEmpty() ? -1 : v.toInt();
        }
        {
            QString v = txAttr(fe, L"mapNbLine");
            sfi._mapNbLine = v.isEmpty() ? -1 : v.toInt();
        }
        {
            QString v = txAttr(fe, L"mapHigherPos");
            sfi._mapHigherPos = v.isEmpty() ? -1 : v.toInt();
        }
        {
            QString v = txAttr(fe, L"mapWidth");
            sfi._mapWidth = v.isEmpty() ? -1 : v.toInt();
        }
        {
            QString v = txAttr(fe, L"mapHeight");
            sfi._mapHeight = v.isEmpty() ? -1 : v.toInt();
        }
        {
            QString v = txAttr(fe, L"mapKByteInDoc");
            sfi._mapKByteInDoc = v.isEmpty() ? -1 : v.toLongLong();
        }
        {
            QString v = txAttr(fe, L"mapWrapIndentMode");
            sfi._mapWrapIndentMode = v.isEmpty() ? -1 : v.toInt();
        }
        sfi._mapIsWrap = (txAttr(fe, L"mapIsWrap") == "yes");

        sfi._firstVisibleLine = txAttr(fe, L"firstVisibleLine").toLongLong();
        sfi._startPos    = txAttr(fe, L"startPos").toLongLong();
        sfi._endPos      = txAttr(fe, L"endPos").toLongLong();
        sfi._xOffset     = txAttr(fe, L"xOffset").toLongLong();
        sfi._selMode     = txAttr(fe, L"selMode").toLongLong();
        sfi._scrollWidth = txAttr(fe, L"scrollWidth").toLongLong();
        sfi._offset      = txAttr(fe, L"offset").toLongLong();
        sfi._wrapCount   = txAttr(fe, L"wrapCount").toLongLong();

        // Mark / Fold 子元素
        for (TiXmlNode* mn = fn->FirstChildElement(L"Mark"); mn;
             mn = mn->NextSibling(L"Mark")) {
            TiXmlElement* me = mn->ToElement();
            if (me) {
                sfi._marks.push_back(static_cast<size_t>(
                    txAttr(me, L"line").toULongLong()));
            }
        }
        for (TiXmlNode* fdn = fn->FirstChildElement(L"Fold"); fdn;
             fdn = fdn->NextSibling(L"Fold")) {
            TiXmlElement* fde = fdn->ToElement();
            if (fde) {
                sfi._foldStates.push_back(static_cast<size_t>(
                    txAttr(fde, L"line").toULongLong()));
            }
        }

        if (!sfi._fileName.isEmpty())
            files.push_back(std::move(sfi));
    }
}

bool NppParameters::loadSession(const QString& path)
{
    if (!QFile::exists(path))
        return false;

    _session = Session();

    TiXmlDocument doc;
    if (!loadTiXmlDoc(doc, path))
        return false;

    TiXmlNode* root = doc.FirstChild(L"NotepadPlus");
    if (!root) return false;

    TiXmlNode* sessionNode = root->FirstChildElement(L"Session");
    if (!sessionNode) return false;

    TiXmlElement* sessionEl = sessionNode->ToElement();
    if (sessionEl) {
        _session._activeView = static_cast<size_t>(
            parseInt(txAttr(sessionEl, L"activeView"), 0));
    }

    TiXmlNode* mainView = sessionNode->FirstChildElement(L"mainView");
    if (mainView)
        parseSessionView(mainView, _session._mainViewFiles, _session._activeMainIndex);

    TiXmlNode* subView = sessionNode->FirstChildElement(L"subView");
    if (subView)
        parseSessionView(subView, _session._subViewFiles, _session._activeSubIndex);

    TiXmlNode* fbNode = sessionNode->FirstChildElement(L"FileBrowser");
    if (fbNode) {
        TiXmlElement* fbEl = fbNode->ToElement();
        if (fbEl)
            _session._fileBrowserSelectedItem =
                txAttr(fbEl, L"latestSelectedItem");
        for (TiXmlNode* rn = fbNode->FirstChildElement(L"root"); rn;
             rn = rn->NextSibling(L"root")) {
            TiXmlElement* re = rn->ToElement();
            if (!re) continue;
            QString folder = txAttr(re, L"foldername");
            if (!folder.isEmpty())
                _session._fileBrowserRoots.push_back(folder);
        }
    }

    return true;
}

// ── 写入 config.xml / session.xml（使用 TiXml，非 ASCII 字符输出为 &#xHHHH; 纯 ASCII）
// 原因：QXmlStreamWriter 写入原始 UTF-8 字节；原版用 _wfopen+fgetws (GBK) 读取会乱码。
// TiXml 的 EncodeString 将所有非 ASCII wchar_t 转为纯 ASCII 数字字符引用，两版均可正确读取。
// ─────────────────────────────────────────────────────────────────────────────

// 将整数转为 wstring，供 TiXml SetAttribute 使用（intptr_t/qint64 在 64 位均为 long long）
static inline std::wstring iw(long long v) { return std::to_wstring(v); }

// bool → "yes"/"no" wstring literal
static inline const wchar_t* bw(bool v) { return v ? L"yes" : L"no"; }

// 创建 GUIConfig 元素并附加到 parent
static TiXmlElement* makeCfgEl(TiXmlElement* parent, const wchar_t* cfgName)
{
    TiXmlElement* found = nullptr;
    for (TiXmlNode* n = parent->FirstChildElement(L"GUIConfig"); n; ) {
        TiXmlNode* next = n->NextSiblingElement(L"GUIConfig");
        TiXmlElement* el = n->ToElement();
        if (txAttr(el, L"name") == QString::fromWCharArray(cfgName)) {
            if (!found)
                found = el;
            else
                parent->RemoveChild(n);
        }
        n = next;
    }

    if (found) {
        for (TiXmlNode* child = found->FirstChild(); child; ) {
            TiXmlNode* next = child->NextSibling();
            if (child->ToText())
                found->RemoveChild(child);
            child = next;
        }
        found->SetAttribute(L"name", cfgName);
        return found;
    }

    TiXmlElement* el = new TiXmlElement(L"GUIConfig");
    el->SetAttribute(L"name", cfgName);
    parent->LinkEndChild(el);
    return el;
}

static TiXmlElement* findCfgEl(TiXmlElement* parent, const wchar_t* cfgName)
{
    for (TiXmlNode* node = parent
             ? parent->FirstChildElement(L"GUIConfig") : nullptr;
         node; node = node->NextSiblingElement(L"GUIConfig")) {
        TiXmlElement* element = node->ToElement();
        if (txAttr(element, L"name") == QString::fromWCharArray(cfgName))
            return element;
    }
    return nullptr;
}

static void removeFirstChildElement(TiXmlElement* parent, const wchar_t* childName)
{
    if (!parent)
        return;
    TiXmlElement* child = parent->FirstChildElement(childName);
    if (child)
        parent->RemoveChild(child);
}

static void removeGuiConfig(TiXmlElement* guiConfigs,
                            const wchar_t* configName)
{
    if (!guiConfigs)
        return;
    for (TiXmlNode* node = guiConfigs->FirstChildElement(L"GUIConfig");
         node; ) {
        TiXmlNode* next = node->NextSiblingElement(L"GUIConfig");
        TiXmlElement* element = node->ToElement();
        if (txAttr(element, L"name")
            == QString::fromWCharArray(configName)) {
            guiConfigs->RemoveChild(node);
        }
        node = next;
    }
}

void NppParameters::loadQtState()
{
    if (!QFile::exists(qtStateFilePath()))
        return;
    QSettings state(qtStateFilePath(), QSettings::IniFormat);
    state.beginGroup("Editor");
    _nppGUI._editorFontName =
        state.value("fontName", _nppGUI._editorFontName).toString();
    _nppGUI._editorFontSize =
        state.value("fontSize", _nppGUI._editorFontSize).toInt();
    _nppGUI._autoIndent =
        state.value("autoIndent", _nppGUI._autoIndent).toBool();
    _nppGUI._autoCompleteEnable =
        state.value("autoCompleteEnable",
                    _nppGUI._autoCompleteEnable).toBool();
    _nppGUI._autoCompleteThreshold =
        state.value("autoCompleteFrom",
                    _nppGUI._autoCompleteThreshold).toInt();
    _nppGUI._showWhitespace =
        state.value("showWhitespace", _nppGUI._showWhitespace).toBool();
    _nppGUI._showEol =
        state.value("showEol", _nppGUI._showEol).toBool();
    _nppGUI._restoreSession =
        state.value("restoreSession", _nppGUI._restoreSession).toBool();
    _nppGUI._darkModeEnabled =
        state.value("darkMode", _nppGUI._darkModeEnabled).toBool();
    state.endGroup();
    _nppGUI._windowState =
        state.value("Window/state", _nppGUI._windowState).toByteArray();
}

bool NppParameters::writeQtState() const
{
    QSettings state(qtStateFilePath(), QSettings::IniFormat);
    state.beginGroup("Editor");
    state.setValue("fontName", _nppGUI._editorFontName);
    state.setValue("fontSize", _nppGUI._editorFontSize);
    state.setValue("autoIndent", _nppGUI._autoIndent);
    state.setValue("autoCompleteEnable", _nppGUI._autoCompleteEnable);
    state.setValue("autoCompleteFrom", _nppGUI._autoCompleteThreshold);
    state.setValue("showWhitespace", _nppGUI._showWhitespace);
    state.setValue("showEol", _nppGUI._showEol);
    state.setValue("restoreSession", _nppGUI._restoreSession);
    state.setValue("darkMode", _nppGUI._darkModeEnabled);
    state.endGroup();
    state.setValue("Window/state", _nppGUI._windowState);
    state.sync();
    return state.status() == QSettings::NoError;
}

bool NppParameters::writeNppGUI()
{
    if (!ensureConfigDir())
        return false;
    return writeConfigXml(configFilePath()) && writeQtState();
}

bool NppParameters::writeFindHistory()
{
    if (!ensureConfigDir())
        return false;
    ensureDefaultXmlFiles();

    TiXmlDocument doc;
    if (!loadTiXmlDoc(doc, configFilePath())) {
        TiXmlDeclaration* decl = new TiXmlDeclaration(L"1.0", L"Windows-1252", L"yes");
        doc.LinkEndChild(decl);
        TiXmlElement* root = new TiXmlElement(L"NotepadPlus");
        doc.LinkEndChild(root);
    }

    TiXmlElement* root = doc.FirstChildElement(L"NotepadPlus");
    if (!root) {
        root = new TiXmlElement(L"NotepadPlus");
        doc.LinkEndChild(root);
    }

    auto yn = [](bool v) { return v ? L"yes" : L"no"; };
    auto replaceList = [](TiXmlElement* parent, const wchar_t* tag,
                          const QStringList& values, int maxItems) {
        bool hadEmptyPlaceholder = false;
        for (TiXmlNode* node = parent->FirstChildElement(tag); node; ) {
            TiXmlNode* next = node->NextSiblingElement(tag);
            TiXmlElement* item = node->ToElement();
            hadEmptyPlaceholder = hadEmptyPlaceholder ||
                (item && txAttr(item, L"name").isEmpty());
            parent->RemoveChild(node);
            node = next;
        }

        int count = 0;
        for (const QString& value : values) {
            if (value.isEmpty()) continue;
            if (count >= maxItems) break;
            TiXmlElement* item = new TiXmlElement(tag);
            item->SetAttribute(L"name", value.toStdWString().c_str());
            parent->LinkEndChild(item);
            ++count;
        }
        if (count == 0 && hadEmptyPlaceholder) {
            TiXmlElement* item = new TiXmlElement(tag);
            item->SetAttribute(L"name", L"");
            parent->LinkEndChild(item);
        }
    };

    TiXmlElement* findHistory = root->FirstChildElement(L"FindHistory");
    if (!findHistory)
        findHistory = new TiXmlElement(L"FindHistory");
    findHistory->SetAttribute(L"nbMaxFindHistoryPath", _findHistory.nbMaxPath);
    findHistory->SetAttribute(L"nbMaxFindHistoryFilter", _findHistory.nbMaxFilter);
    findHistory->SetAttribute(L"nbMaxFindHistoryFind", _findHistory.nbMaxFind);
    findHistory->SetAttribute(L"nbMaxFindHistoryReplace", _findHistory.nbMaxReplace);
    findHistory->SetAttribute(L"matchWord", yn(_findHistory.matchWord));
    findHistory->SetAttribute(L"matchCase", yn(_findHistory.matchCase));
    findHistory->SetAttribute(L"wrap", yn(_findHistory.wrap));
    findHistory->SetAttribute(L"directionDown", yn(_findHistory.directionDown));
    findHistory->SetAttribute(L"fifRecuisive", yn(_findHistory.fifRecursive));
    findHistory->SetAttribute(L"fifInHiddenFolder", yn(_findHistory.fifInHiddenFolder));
    findHistory->SetAttribute(L"fifFilterFollowsDoc", yn(_findHistory.fifFilterFollowsDoc));
    findHistory->SetAttribute(L"fifFolderFollowsDoc", yn(_findHistory.fifFolderFollowsDoc));
    if (findHistory->Attribute(L"fifProjectPanel1") ||
        _findHistory.fifProjectPanel1)
        findHistory->SetAttribute(
            L"fifProjectPanel1", yn(_findHistory.fifProjectPanel1));
    if (findHistory->Attribute(L"fifProjectPanel2") ||
        _findHistory.fifProjectPanel2)
        findHistory->SetAttribute(
            L"fifProjectPanel2", yn(_findHistory.fifProjectPanel2));
    if (findHistory->Attribute(L"fifProjectPanel3") ||
        _findHistory.fifProjectPanel3)
        findHistory->SetAttribute(
            L"fifProjectPanel3", yn(_findHistory.fifProjectPanel3));
    findHistory->SetAttribute(L"searchMode", _findHistory.searchMode);
    findHistory->SetAttribute(L"transparencyMode", _findHistory.transparencyMode);
    findHistory->SetAttribute(L"transparency", _findHistory.transparency);
    findHistory->SetAttribute(L"dotMatchesNewline", yn(_findHistory.dotMatchesNewline));
    findHistory->SetAttribute(L"isSearch2ButtonsMode", yn(_findHistory.isSearch2ButtonsMode));

    replaceList(findHistory, L"Path", _findHistory.paths, _findHistory.nbMaxPath);
    replaceList(findHistory, L"Filter", _findHistory.filters, _findHistory.nbMaxFilter);
    replaceList(findHistory, L"Find", _findHistory.finds, _findHistory.nbMaxFind);
    replaceList(findHistory, L"Replace", _findHistory.replaces, _findHistory.nbMaxReplace);

    if (!findHistory->Parent())
        root->LinkEndChild(findHistory);
    return doc.SaveFile(configFilePath().toStdWString().c_str());
}

bool NppParameters::writeConfigXml(const QString& filePath)
{
    TiXmlDocument doc;
    if (!loadTiXmlDoc(doc, filePath)) {
        doc.Clear();
        doc.LinkEndChild(new TiXmlDeclaration(L"1.0", L"UTF-8", L""));
        doc.LinkEndChild(new TiXmlElement(L"NotepadPlus"));
    }

    TiXmlElement* root = doc.FirstChildElement(L"NotepadPlus");
    if (!root) {
        root = new TiXmlElement(L"NotepadPlus");
        doc.LinkEndChild(root);
    }

    TiXmlElement* guiConfigs = root->FirstChildElement(L"GUIConfigs");
    if (!guiConfigs) {
        guiConfigs = new TiXmlElement(L"GUIConfigs");
        root->LinkEndChild(guiConfigs);
    }

    // ToolBar
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"ToolBar");
        el->SetAttribute(L"visible", bw(_nppGUI._toolBarShow));
        el->LinkEndChild(new TiXmlText(L"standard"));
    }
    // StatusBar
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"StatusBar");
        el->LinkEndChild(new TiXmlText(_nppGUI._statusBarShow ? L"show" : L"hide"));
    }
    // TabBar
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"TabBar");
        el->SetAttribute(L"dragAndDrop",       bw(_nppGUI._tabDragAndDrop));
        el->SetAttribute(L"drawTopBar",        bw(_nppGUI._tabDrawTopBar));
        el->SetAttribute(L"drawInactiveTab",   bw(_nppGUI._tabDrawInactiveTab));
        el->SetAttribute(L"reduce",            L"yes");
        el->SetAttribute(L"closeButton",       bw(_nppGUI._tabCloseButton));
        el->SetAttribute(L"doubleClick2Close", bw(_nppGUI._tabDbclkToClose));
        el->SetAttribute(L"vertical",          bw(_nppGUI._tabVertical));
        el->SetAttribute(L"multiLine",         bw(_nppGUI._tabMultiLine));
        el->SetAttribute(L"hide",              bw(_nppGUI._tabHide));
        el->SetAttribute(L"quitOnEmpty",       bw(_nppGUI._tabQuitOnEmpty));
    }
    // ScintillaViewsSplitter
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"ScintillaViewsSplitter");
        el->LinkEndChild(new TiXmlText(_nppGUI._isVerticalSplit ? L"vertical" : L"horizontal"));
    }
    // TabSetting
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"TabSetting");
        el->SetAttribute(L"replaceBySpace", bw(_nppGUI._tabReplacedBySpace));
        el->SetAttribute(L"size",           _nppGUI._tabSize);
    }
    // AppPosition
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"AppPosition");
        el->SetAttribute(L"x",           _nppGUI._appPos.x());
        el->SetAttribute(L"y",           _nppGUI._appPos.y());
        el->SetAttribute(L"width",       _nppGUI._appPos.width());
        el->SetAttribute(L"height",      _nppGUI._appPos.height());
        el->SetAttribute(L"isMaximized", bw(_nppGUI._isMaximized));
    }
    // RememberLastSession
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"RememberLastSession");
        el->LinkEndChild(new TiXmlText(_nppGUI._rememberLastSession ? L"yes" : L"no"));
    }
    // NewDocDefaultSettings
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"NewDocDefaultSettings");
        el->SetAttribute(L"format",   _nppGUI._newDocDefaultFormat);
        el->SetAttribute(L"encoding", _nppGUI._newDocDefaultEncoding);
        el->SetAttribute(L"lang",     _nppGUI._newDocDefaultLang);
        el->SetAttribute(L"openAnsiAsUTF8", bw(_nppGUI._openAnsiAsUtf8));
    }
    // Character-set detection
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"DetectEncoding");
        el->LinkEndChild(new TiXmlText(
            _nppGUI._detectEncoding ? L"yes" : L"no"));
    }
    // Backup
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"Backup");
        el->SetAttribute(L"action",               _nppGUI._backup);
        el->SetAttribute(L"useCustumDir",          bw(_nppGUI._useBackupDir));
        el->SetAttribute(L"dir",
                         _nppGUI._backupDir.toStdWString().c_str());
        el->SetAttribute(L"isSnapshotMode",        bw(_nppGUI._isSnapshotMode));
        el->SetAttribute(L"snapshotBackupTiming",  _nppGUI._snapshotBackupTiming);
    }
    // Auto-detection
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"Auto-detection");
        el->LinkEndChild(new TiXmlText(_nppGUI._fileAutoDetection ? L"yes" : L"no"));
    }
    // ScintillaPrimaryView
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"ScintillaPrimaryView");
        el->SetAttribute(L"lineNumberMargin",        _svp._lineNumberMarginShow    ? L"show" : L"hide");
        el->SetAttribute(L"bookMarkMargin",          _svp._bookMarkMarginShow      ? L"show" : L"hide");
        el->SetAttribute(L"indentGuideLine",         _svp._indentGuideLineShow     ? L"show" : L"hide");
        if (el->Attribute(L"currentLineHilitingShow")) {
            el->SetAttribute(L"currentLineHilitingShow",
                             _svp._currentLineHilitingShow ? L"show" : L"hide");
        }
        if (el->Attribute(L"currentLineIndicator")) {
            el->SetAttribute(L"currentLineIndicator",
                             _svp._currentLineHilitingShow ? 1 : 0);
        }
        el->SetAttribute(L"wrapSymbolShow",          _svp._wrapSymbolShow          ? L"show" : L"hide");
        el->SetAttribute(L"Wrap",                    bw(_svp._doWrap));
        if (el->Attribute(L"edge"))
            el->SetAttribute(L"edge", bw(_svp._edgeShow));
        if (el->Attribute(L"edgeNbColumn"))
            el->SetAttribute(L"edgeNbColumn", _svp._edgeNbColumn);
        el->SetAttribute(L"zoom",                    _svp._zoom);
        el->SetAttribute(L"zoom2",                   _svp._zoom2);
        el->SetAttribute(L"whiteSpaceShow",          _svp._whiteSpaceShow ? L"show" : L"hide");
        el->SetAttribute(L"eolShow",                 _svp._eolShow        ? L"show" : L"hide");
        el->SetAttribute(L"scrollBeyondLastLine",    bw(_svp._scrollBeyondLastLine));
    }
    // auto-completion
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"auto-completion");
        el->SetAttribute(L"autoCAction",        _nppGUI._autocAction);
        el->SetAttribute(L"triggerFromNbChar",  _nppGUI._autocFromNbChar);
        el->SetAttribute(L"autoCIgnoreNumbers", bw(_nppGUI._autocIgnoreNumbers));
        el->SetAttribute(L"funcParams",         bw(_nppGUI._funcParams));
    }
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"openSaveDir");
        el->SetAttribute(L"value", _nppGUI._openSaveDir);
        el->SetAttribute(L"defaultDirPath",
                         _nppGUI._defaultDirPath.toStdWString().c_str());
    }
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"SmartHighLight");
        el->SetAttribute(L"matchCase", bw(_nppGUI._smartHighlightMatchCase));
        el->SetAttribute(L"wholeWordOnly", bw(_nppGUI._smartHighlightWholeWord));
        el->SetAttribute(L"useFindSettings",
                         bw(_nppGUI._smartHighlightUseFindSettings));
        el->SetAttribute(L"onAnotherView",
                         bw(_nppGUI._smartHighlightAnotherView));
        el->LinkEndChild(new TiXmlText(
            _nppGUI._smartHighlight ? L"yes" : L"no"));
    }
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"Print");
        el->SetAttribute(L"lineNumber", bw(_nppGUI._printLineNumber));
        el->SetAttribute(L"printOption", _nppGUI._printOption);
        el->SetAttribute(L"headerLeft",
                         _nppGUI._printHeaderLeft.toStdWString().c_str());
        el->SetAttribute(L"headerMiddle",
                         _nppGUI._printHeaderMiddle.toStdWString().c_str());
        el->SetAttribute(L"headerRight",
                         _nppGUI._printHeaderRight.toStdWString().c_str());
        el->SetAttribute(L"footerLeft",
                         _nppGUI._printFooterLeft.toStdWString().c_str());
        el->SetAttribute(L"footerMiddle",
                         _nppGUI._printFooterMiddle.toStdWString().c_str());
        el->SetAttribute(L"footerRight",
                         _nppGUI._printFooterRight.toStdWString().c_str());
    }
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"multiInst");
        el->SetAttribute(L"setting", _nppGUI._multiInstSetting);
    }
    {
        TiXmlElement* el = nullptr;
        for (TiXmlNode* node = guiConfigs->FirstChildElement(L"GUIConfig");
             node; node = node->NextSiblingElement(L"GUIConfig")) {
            TiXmlElement* candidate = node->ToElement();
            const QString name = txAttr(candidate, L"name");
            if (name == "insertDateTime" || name == "DateTime") {
                el = candidate;
                break;
            }
        }
        if (el) {
            el->SetAttribute(L"customizedFormat",
                             _nppGUI._dateTimeFormat.toStdWString().c_str());
            el->SetAttribute(L"reverseDefaultOrder",
                             bw(_nppGUI._dateTimeReverseDefaultOrder));
        }
    }
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"delimiterSelection");
        el->SetAttribute(L"leftmostDelimiter", _nppGUI._leftmostDelimiter);
        el->SetAttribute(L"rightmostDelimiter", _nppGUI._rightmostDelimiter);
        el->SetAttribute(L"delimiterSelectionOnEntireDocument",
                         bw(_nppGUI._delimiterSelectionOnEntireDocument));
    }
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"URL");
        el->LinkEndChild(new TiXmlText(
            QString::number(_nppGUI._urlMode).toStdWString().c_str()));
    }
    {
        TiXmlElement* el = findCfgEl(guiConfigs, L"uriCustomizedSchemes");
        if (el) {
            makeCfgEl(guiConfigs, L"uriCustomizedSchemes")->LinkEndChild(
                new TiXmlText(
                    _nppGUI._uriCustomizedSchemes.toStdWString().c_str()));
        }
    }
    {
        TiXmlElement* el = findCfgEl(guiConfigs, L"TagsMatchHighLight");
        if (el || !_nppGUI._enableTagsMatchHighlight ||
            !_nppGUI._enableTagAttrsHighlight ||
            _nppGUI._highlightNonHtmlZone) {
            el = makeCfgEl(guiConfigs, L"TagsMatchHighLight");
            el->SetAttribute(L"TagAttrHighLight",
                             bw(_nppGUI._enableTagAttrsHighlight));
            el->SetAttribute(L"HighLightNonHtmlZone",
                             bw(_nppGUI._highlightNonHtmlZone));
            el->LinkEndChild(new TiXmlText(
                _nppGUI._enableTagsMatchHighlight ? L"yes" : L"no"));
        }
    }
    {
        TiXmlElement* el = findCfgEl(guiConfigs, L"auto-insert");
        if (el || _nppGUI._autoInsertParentheses ||
            _nppGUI._autoInsertBrackets ||
            _nppGUI._autoInsertCurlyBrackets ||
            _nppGUI._autoInsertQuotes ||
            _nppGUI._autoInsertDoubleQuotes ||
            _nppGUI._autoInsertHtmlXmlTag) {
            el = makeCfgEl(guiConfigs, L"auto-insert");
            el->SetAttribute(L"parentheses",
                             bw(_nppGUI._autoInsertParentheses));
            el->SetAttribute(L"brackets", bw(_nppGUI._autoInsertBrackets));
            el->SetAttribute(L"curlyBrackets",
                             bw(_nppGUI._autoInsertCurlyBrackets));
            el->SetAttribute(L"quotes", bw(_nppGUI._autoInsertQuotes));
            el->SetAttribute(L"doubleQuotes",
                             bw(_nppGUI._autoInsertDoubleQuotes));
            el->SetAttribute(L"htmlXmlTag",
                             bw(_nppGUI._autoInsertHtmlXmlTag));
        }
    }
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"searchEngine");
        el->SetAttribute(L"searchEngineChoice", _nppGUI._searchEngineChoice);
        el->SetAttribute(L"searchEngineCustom",
                         _nppGUI._searchEngineCustom.toStdWString().c_str());
    }
    // MenuBar
    {
        TiXmlElement* el = makeCfgEl(guiConfigs, L"MenuBar");
        el->LinkEndChild(new TiXmlText(_nppGUI._menuBarShow ? L"show" : L"hide"));
    }
    // stylerTheme
    {
        TiXmlElement* el = nullptr;
        for (TiXmlNode* node = guiConfigs->FirstChildElement(L"GUIConfig");
             node; node = node->NextSiblingElement(L"GUIConfig")) {
            TiXmlElement* candidate = node->ToElement();
            if (txAttr(candidate, L"name") == "stylerTheme") {
                el = candidate;
                break;
            }
        }
        if (!el) {
            el = makeCfgEl(guiConfigs, L"stylerTheme");
            el->SetAttribute(L"path",
                             stylersFilePath().toStdWString().c_str());
        }
    }
    // Qt-only state belongs in qtState.ini, never in original config.xml.
    // Remove nodes written by earlier Qt builds after their values were loaded.
    removeGuiConfig(guiConfigs, L"EditorFont");
    removeGuiConfig(guiConfigs, L"EditorSettings");
    removeGuiConfig(guiConfigs, L"WindowState");

    // History（最近打开文件，filename 可能含中文，TiXml 自动编码为 &#xHHHH;）
    TiXmlElement* hist = root->FirstChildElement(L"History");
    TiXmlElement* projectPanels = root->FirstChildElement(L"ProjectPanels");
    const bool hasWorkspacePath =
        !_workspaceFilePaths[0].isEmpty() ||
        !_workspaceFilePaths[1].isEmpty() ||
        !_workspaceFilePaths[2].isEmpty();
    if (!projectPanels && hasWorkspacePath) {
        projectPanels = new TiXmlElement(L"ProjectPanels");
        root->LinkEndChild(projectPanels);
    }
    for (int id = 0; projectPanels && id < 3; ++id) {
        TiXmlElement* panel = nullptr;
        for (TiXmlNode* node = projectPanels->FirstChildElement(L"ProjectPanel");
             node; node = node->NextSiblingElement(L"ProjectPanel")) {
            TiXmlElement* candidate = node->ToElement();
            if (parseInt(txAttr(candidate, L"id"), -1) == id) {
                panel = candidate;
                break;
            }
        }
        if (!panel) {
            panel = new TiXmlElement(L"ProjectPanel");
            panel->SetAttribute(L"id", id);
            projectPanels->LinkEndChild(panel);
        }
        panel->SetAttribute(L"workSpaceFile",
                            _workspaceFilePaths[id].toStdWString().c_str());
    }

    if (!hist)
        hist = new TiXmlElement(L"History");
    hist->SetAttribute(L"nbMaxFile",    _nppGUI._nbMaxRecentFile);
    hist->SetAttribute(L"inSubMenu",    bw(_nppGUI._putRecentFileInSubMenu));
    hist->SetAttribute(L"customLength", L"-1");
    std::vector<TiXmlElement*> existingHistoryFiles;
    for (TiXmlNode* node = hist->FirstChildElement(L"File"); node;
         node = node->NextSiblingElement(L"File")) {
        existingHistoryFiles.push_back(node->ToElement());
    }
    std::vector<bool> usedHistoryFiles(existingHistoryFiles.size(), false);
    for (const QString& fn : _nppGUI._recentFileList) {
        TiXmlElement* fEl = nullptr;
        for (size_t i = 0; i < existingHistoryFiles.size(); ++i) {
            if (!usedHistoryFiles[i] &&
                txAttr(existingHistoryFiles[i], L"filename") == fn) {
                usedHistoryFiles[i] = true;
                fEl = existingHistoryFiles[i];
                break;
            }
        }
        if (!fEl) {
            fEl = new TiXmlElement(L"File");
            hist->LinkEndChild(fEl);
        }
        fEl->SetAttribute(L"filename", fn.toStdWString().c_str());
    }
    for (size_t i = 0; i < existingHistoryFiles.size(); ++i) {
        if (!usedHistoryFiles[i])
            hist->RemoveChild(existingHistoryFiles[i]);
    }
    if (!hist->Parent())
        root->LinkEndChild(hist);

    return doc.SaveFile(filePath.toStdWString().c_str());
}

// ── 写入 session.xml ──────────────────────────────────────────────────────────

bool NppParameters::writeSession(const Session& session)
{
    if (!ensureConfigDir())
        return false;
    return writeSession(sessionFilePath(), session);
}

bool NppParameters::writeSession(const QString& filePath, const Session& session)
{
    if (filePath.isEmpty())
        return false;
    return writeSessionXml(filePath, session);
}

static void updateSessionFileElement(TiXmlElement* fEl,
                                     const sessionFileInfo& sfi,
                                     bool isNew)
{
    auto setOptional = [fEl, isNew](const wchar_t* name,
                                    long long value,
                                    long long defaultValue) {
        if (isNew || fEl->Attribute(name) || value != defaultValue)
            fEl->SetAttribute(name, iw(value).c_str());
    };

    fEl->SetAttribute(L"firstVisibleLine", iw(sfi._firstVisibleLine).c_str());
    fEl->SetAttribute(L"xOffset",          iw(sfi._xOffset).c_str());
    fEl->SetAttribute(L"scrollWidth",      iw(sfi._scrollWidth).c_str());
    fEl->SetAttribute(L"startPos",         iw(sfi._startPos).c_str());
    fEl->SetAttribute(L"endPos",           iw(sfi._endPos).c_str());
    fEl->SetAttribute(L"selMode",          iw(sfi._selMode).c_str());
    setOptional(L"offset", sfi._offset, 0);
    setOptional(L"wrapCount", sfi._wrapCount, 0);

    fEl->SetAttribute(L"lang", sfi._langName.toStdWString().c_str());
    fEl->SetAttribute(L"encoding", sfi._encoding);
    if (isNew || fEl->Attribute(L"userReadOnly") || sfi._userReadOnly)
        fEl->SetAttribute(L"userReadOnly", bw(sfi._userReadOnly));
    fEl->SetAttribute(L"filename", sfi._fileName.toStdWString().c_str());
    fEl->SetAttribute(L"backupFilePath",
                      sfi._backupFilePath.toStdWString().c_str());
    fEl->SetAttribute(L"originalFileLastModifTimestamp",
                      iw(sfi._originalFileLastModifTimestamp).c_str());
    fEl->SetAttribute(L"originalFileLastModifTimestampHigh",
                      iw(sfi._originalFileLastModifTimestampHigh).c_str());
    setOptional(L"tabColourId", sfi._individualTabColour, -1);

    setOptional(L"mapFirstVisibleDisplayLine",
                sfi._mapFirstVisibleDisplayLine, -1);
    setOptional(L"mapFirstVisibleDocLine", sfi._mapFirstVisibleDocLine, -1);
    setOptional(L"mapLastVisibleDocLine", sfi._mapLastVisibleDocLine, -1);
    setOptional(L"mapNbLine", sfi._mapNbLine, -1);
    setOptional(L"mapHigherPos", sfi._mapHigherPos, -1);
    setOptional(L"mapWidth", sfi._mapWidth, -1);
    setOptional(L"mapHeight", sfi._mapHeight, -1);
    setOptional(L"mapKByteInDoc", sfi._mapKByteInDoc, -1);
    setOptional(L"mapWrapIndentMode", sfi._mapWrapIndentMode, -1);
    if (isNew || fEl->Attribute(L"mapIsWrap") || sfi._mapIsWrap)
        fEl->SetAttribute(L"mapIsWrap", bw(sfi._mapIsWrap));

    std::vector<size_t> existingMarks;
    for (TiXmlNode* node = fEl->FirstChildElement(L"Mark"); node;
         node = node->NextSiblingElement(L"Mark")) {
        existingMarks.push_back(static_cast<size_t>(
            txAttr(node->ToElement(), L"line").toULongLong()));
    }
    std::vector<size_t> existingFolds;
    for (TiXmlNode* node = fEl->FirstChildElement(L"Fold"); node;
         node = node->NextSiblingElement(L"Fold")) {
        existingFolds.push_back(static_cast<size_t>(
            txAttr(node->ToElement(), L"line").toULongLong()));
    }

    if (existingMarks != sfi._marks || existingFolds != sfi._foldStates) {
        for (TiXmlNode* node = fEl->FirstChildElement(); node; ) {
            TiXmlNode* next = node->NextSiblingElement();
            const QString name = QString::fromWCharArray(node->Value());
            if (name == "Mark" || name == "Fold")
                fEl->RemoveChild(node);
            node = next;
        }
        for (size_t line : sfi._marks) {
            TiXmlElement* mark = new TiXmlElement(L"Mark");
            mark->SetAttribute(L"line", static_cast<int>(line));
            fEl->LinkEndChild(mark);
        }
        for (size_t line : sfi._foldStates) {
            TiXmlElement* fold = new TiXmlElement(L"Fold");
            fold->SetAttribute(L"line", static_cast<int>(line));
            fEl->LinkEndChild(fold);
        }
    }
}

static void updateSessionView(TiXmlElement* sessionElement,
                              const wchar_t* viewTag,
                              const std::vector<sessionFileInfo>& files,
                              size_t activeIndex)
{
    TiXmlElement* view = sessionElement->FirstChildElement(viewTag);
    if (!view) {
        view = new TiXmlElement(viewTag);
        sessionElement->LinkEndChild(view);
    }
    view->SetAttribute(L"activeIndex", static_cast<int>(activeIndex));

    std::vector<TiXmlElement*> existingFiles;
    for (TiXmlNode* node = view->FirstChildElement(L"File"); node;
         node = node->NextSiblingElement(L"File")) {
        existingFiles.push_back(node->ToElement());
    }
    std::vector<bool> used(existingFiles.size(), false);

    for (const sessionFileInfo& file : files) {
        TiXmlElement* element = nullptr;
        for (size_t i = 0; i < existingFiles.size(); ++i) {
            if (!used[i] &&
                txAttr(existingFiles[i], L"filename") == file._fileName) {
                used[i] = true;
                element = existingFiles[i];
                break;
            }
        }

        const bool isNew = element == nullptr;
        if (isNew) {
            element = new TiXmlElement(L"File");
            view->LinkEndChild(element);
        }
        updateSessionFileElement(element, file, isNew);
    }

    for (size_t i = 0; i < existingFiles.size(); ++i) {
        if (!used[i])
            view->RemoveChild(existingFiles[i]);
    }
}

bool NppParameters::writeSessionXml(const QString& filePath, const Session& session)
{
    TiXmlDocument doc;
    if (!loadTiXmlDoc(doc, filePath)) {
        doc.Clear();
        doc.LinkEndChild(new TiXmlDeclaration(L"1.0", L"UTF-8", L""));
        doc.LinkEndChild(new TiXmlElement(L"NotepadPlus"));
    }

    TiXmlElement* root = doc.FirstChildElement(L"NotepadPlus");
    if (!root) {
        root = new TiXmlElement(L"NotepadPlus");
        doc.LinkEndChild(root);
    }

    TiXmlElement* sessionEl = root->FirstChildElement(L"Session");
    if (!sessionEl) {
        sessionEl = new TiXmlElement(L"Session");
        root->LinkEndChild(sessionEl);
    }
    sessionEl->SetAttribute(L"activeView", static_cast<int>(session._activeView));

    updateSessionView(sessionEl, L"mainView",
                      session._mainViewFiles, session._activeMainIndex);
    updateSessionView(sessionEl, L"subView",
                      session._subViewFiles, session._activeSubIndex);

    if (!session._fileBrowserRoots.empty()
        || !session._fileBrowserSelectedItem.isEmpty()) {
        TiXmlElement* fb = sessionEl->FirstChildElement(L"FileBrowser");
        if (!fb) {
            fb = new TiXmlElement(L"FileBrowser");
            sessionEl->LinkEndChild(fb);
        }
        fb->SetAttribute(L"latestSelectedItem",
            session._fileBrowserSelectedItem.toStdWString().c_str());
        for (TiXmlNode* node = fb->FirstChildElement(L"root"); node; ) {
            TiXmlNode* next = node->NextSiblingElement(L"root");
            fb->RemoveChild(node);
            node = next;
        }
        for (const QString& r : session._fileBrowserRoots) {
            TiXmlElement* rEl = new TiXmlElement(L"root");
            rEl->SetAttribute(L"foldername", r.toStdWString().c_str());
            fb->LinkEndChild(rEl);
        }
    }

    return doc.SaveFile(filePath.toStdWString().c_str());
}
