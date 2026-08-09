// Parameters.h - 配置参数管理
// 移植自: v8.4.6:PowerEditor/src/Parameters.h
// 使用 Qt 类型替代 Windows 原生类型

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <QString>
#include <QRect>
#include <QKeySequence>
#include <QByteArray>
#include <QStringList>
#include <QColor>
#include <QVector>
#include <vector>
#include <cstdint>
#include "localization.h"
#include "MISC/ConfigPathResolver.h"

// ── 语言描述符（对应原版 langs.xml <Language> 元素） ─────────────────────────

struct LangDesc {
    static constexpr int KeywordSetCount = 9;
    QString     name;           // 语言名，如 "cpp"、"python"
    QStringList exts;           // 文件扩展名列表（小写，不含点）
    QString     commentLine;    // 行注释，如 "//"
    QString     commentStart;   // 块注释起，如 "/*"
    QString     commentEnd;     // 块注释止，如 "*/"
    QString     keywords[KeywordSetCount];
};

// ── 样式描述符（对应原版 stylers.xml <WordsStyle> 元素） ─────────────────────

struct WordsStyle {
    int     styleID   = 0;
    int     xmlStyleID = -1;
    QString name;
    QColor  fgColor;            // 前景色
    QColor  bgColor;            // 背景色
    QString fontName;
    QString keywordClass;
    QString userKeywords;
    int     fontStyle = 0;      // 0=normal,1=bold,2=italic,4=underline
    int     fontSize  = 0;      // 0 = 使用编辑器默认字号
    bool    hasFg     = false;
    bool    hasBg     = false;
    int     nesting   = 0;
};

// 单个词法分析器的完整样式集
struct LexerStyler {
    QString             name;   // 如 "cpp"
    QString             desc;   // 如 "C++"
    QVector<WordsStyle> styles;

    const WordsStyle* getStyle(int styleID) const {
        for (const auto& s : styles)
            if (s.styleID == styleID) return &s;
        return nullptr;
    }
};

struct UserLangDesc {
    QString name;
    QString sourceFilePath;
    QStringList exts;
    bool caseSensitive = true;
    QString lineComment;
    QString blockCommentStart;
    QString blockCommentEnd;
    QString operators;
    QStringList keywords[8];
    QString keywordLists[28];
    bool prefixKeywords[8] = {
        false, false, false, false, false, false, false, false
    };
    bool foldCompact = true;
    bool foldComments = false;
    QVector<WordsStyle> styles;
};

// ── 宏动作（对应原版 shortcuts.xml <Action> 元素） ───────────────────────────

struct MacroAction {
    int     type    = 0;    // 0=SCI msg,1=string SCI msg,2=N++ cmd,3=saved search
    int     message = 0;    // Scintilla 消息 or 命令 ID
    int     wParam  = 0;
    int     lParam  = 0;
    QString sParam;
};

// 宏定义（对应原版 shortcuts.xml <Macro> 元素）
struct MacroDef {
    QString             name;
    bool                ctrl  = false;
    bool                alt   = false;
    bool                shift = false;
    int                 key   = 0;    // Qt::Key
    QVector<MacroAction> actions;
};

struct UserCommandDef {
    QString name;
    QString command;
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    int key = 0;
};

struct ShortcutKey {
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    int key = 0;

    QKeySequence toKeySequence() const;
    static ShortcutKey fromKeySequence(const QKeySequence& sequence);
};

struct InternalCommandShortcut {
    int id = 0;
    ShortcutKey shortcut;
};

struct ScintillaKeyDef {
    int scintillaId = 0;
    int menuCommandId = 0;
    QVector<ShortcutKey> shortcuts;
};

// ── 枚举（与原版一致） ────────────────────────────────────────────────────────

enum EolType {
    EolType_windows = 0,
    EolType_macos   = 1,
    EolType_unix    = 2,
    EolType_unknown = 3
};

enum UniMode {
    uni8Bit   = 0,
    uniUTF8   = 1,
    uni16BE   = 2,
    uni16LE   = 3,
    uniCookie = 4,
    uni7Bit   = 5,
    uniEnd
};

// ── 光标/视口位置（对应原版 Position） ───────────────────────────────────────

struct Position {
    intptr_t _firstVisibleLine = 0;
    intptr_t _startPos         = 0;
    intptr_t _endPos           = 0;
    intptr_t _xOffset          = 0;
    intptr_t _selMode          = 0;
    intptr_t _scrollWidth      = 1;
    intptr_t _offset           = 0;
    intptr_t _wrapCount        = 0;
};

// ── 会话文件信息（对应原版 sessionFileInfo） ──────────────────────────────────

struct sessionFileInfo : public Position {
    QString  _fileName;
    QString  _langName;
    int      _encoding                          = -1;
    bool     _userReadOnly                      = false;
    QString  _backupFilePath;
    qint64   _originalFileLastModifTimestamp    = 0;
    qint64   _originalFileLastModifTimestampHigh = 0;
    int      _individualTabColour              = -1;
    // DocumentMap 相关（与原版格式一致）
    int      _mapFirstVisibleDisplayLine       = -1;
    int      _mapFirstVisibleDocLine           = -1;
    int      _mapLastVisibleDocLine            = -1;
    int      _mapNbLine                        = -1;
    int      _mapHigherPos                     = -1;
    int      _mapWidth                         = -1;
    int      _mapHeight                        = -1;
    qint64   _mapKByteInDoc                    = -1;
    int      _mapWrapIndentMode                = -1;
    bool     _mapIsWrap                        = false;
    std::vector<size_t> _marks;
    std::vector<size_t> _foldStates;

    sessionFileInfo() = default;
    explicit sessionFileInfo(const QString& fn) : _fileName(fn) {}
};

// ── 会话（对应原版 Session） ──────────────────────────────────────────────────

struct Session {
    size_t _activeView      = 0;
    size_t _activeMainIndex = 0;
    size_t _activeSubIndex  = 0;
    std::vector<sessionFileInfo> _mainViewFiles;
    std::vector<sessionFileInfo> _subViewFiles;
    std::vector<QString> _fileBrowserRoots;
    QString _fileBrowserSelectedItem;
};

// ── Scintilla 视图参数（对应原版 ScintillaViewParams） ───────────────────────

struct ScintillaViewParams {
    bool _lineNumberMarginShow    = true;
    bool _bookMarkMarginShow      = true;
    bool _foldMarginShow          = true;
    bool _indentGuideLineShow     = true;
    bool _currentLineHilitingShow = true;
    bool _wrapSymbolShow          = false;
    bool _doWrap                  = false;
    bool _folding                 = true;
    int  _lineWrapMethod          = 0;
    bool _scrollBeyondLastLine    = false;
    bool _showBorderEdge          = true;
    int  _borderWidth             = 2;
    bool _edgeShow                = false;
    int  _edgeNbColumn            = 80;
    int  _zoom                    = 0;
    int  _zoom2                   = 0;
    bool _whiteSpaceShow          = false;
    bool _eolShow                 = false;
};

// ── 主 GUI 配置（对应原版 NppGUI） ────────────────────────────────────────────

struct NppGUI {
    // 工具栏 / 状态栏 / 菜单栏可见性
    bool _toolBarShow   = true;
    bool _statusBarShow = true;
    bool _menuBarShow   = true;

    // Tab 栏行为（与 config.xml GUIConfig name="TabBar" 一致）
    bool _tabDragAndDrop     = true;
    bool _tabDrawTopBar      = true;
    bool _tabDrawInactiveTab = true;
    bool _tabCloseButton     = true;
    bool _tabDbclkToClose    = false;
    bool _tabVertical        = false;
    bool _tabMultiLine       = false;
    bool _tabHide            = false;
    bool _tabQuitOnEmpty     = false;
    int  _tabIconSetNumber   = 0;

    // Tab 缩进设置（GUIConfig name="TabSetting"）
    int  _tabSize            = 4;
    bool _tabReplacedBySpace = false;

    // 窗口位置/状态（GUIConfig name="AppPosition"）
    QRect      _appPos      = QRect(10, 10, 1024, 768);
    bool       _isMaximized = false;
    QByteArray _windowState; // QMainWindow::saveState() 字节流，base64 存储
    int _dockingLeftWidth = 200;
    int _dockingRightWidth = 200;
    int _dockingTopHeight = 200;
    int _dockingBottomHeight = 200;

    // 查找窗口位置（GUIConfig name="FindWindowPosition"，以 left/top/right/bottom 存储）
    int  _findWinLeft   = 100;
    int  _findWinTop    = 100;
    int  _findWinRight  = 500;
    int  _findWinBottom = 400;

    // 最近文件（History 元素）
    int         _nbMaxRecentFile        = 10;
    bool        _putRecentFileInSubMenu = false;
    QStringList _recentFileList;

    // 会话恢复（GUIConfig name="RememberLastSession"）
    bool _rememberLastSession = true;

    // 新建文档默认（GUIConfig name="NewDocDefaultSettings"）
    int _newDocDefaultFormat   = 2; // EolType_unix
    int _newDocDefaultEncoding = 0;
    int _newDocDefaultLang     = 0;
    bool _openAnsiAsUtf8       = true;
    bool _detectEncoding       = true;

    // 备份（GUIConfig name="Backup"）
    int  _backup               = 0;
    bool _useBackupDir         = false;
    QString _backupDir;
    bool _isSnapshotMode       = true;
    int  _snapshotBackupTiming = 7000;

    // 文件自动检测（GUIConfig name="Auto-detection"）
    int  _fileAutoDetection = 1;
    bool _checkHistoryFiles = false;

    // 自动完成（GUIConfig name="auto-completion"）
    int  _autocAction        = 0;
    int  _autocFromNbChar    = 3;
    bool _autocIgnoreNumbers = true;
    bool _funcParams         = true;
    bool _backSlashIsEscapeCharacterForSql = true;

    // 分割视图方向（GUIConfig name="ScintillaViewsSplitter"）
    bool _isVerticalSplit = false;

    // ── Qt 移植扩展（存于 config.xml 的 Qt 专用 GUIConfig 段） ──────────────

    // 编辑器字体（原版存于 stylers.xml，Qt 版简化存于 config.xml）
    QString _editorFontName = "Consolas";
    int     _editorFontSize = 10;

    // 编辑器行为
    bool _autoIndent = true;
    bool _doWordWrap = false;

    // 自动完成详细
    bool _autoCompleteEnable    = true;
    int  _autoCompleteThreshold = 3;

    // 显示设置（原版由 ScintillaViewParams 管理，此处为 Preferences 对话框冗余副本）
    bool _showWhitespace = false;
    bool _showEol        = false;
    bool _restoreSession = true;
    bool _darkModeEnabled = false;

    int _openSaveDir = 0;
    QString _defaultDirPath;

    bool _smartHighlight = true;
    bool _smartHighlightMatchCase = false;
    bool _smartHighlightWholeWord = true;
    bool _smartHighlightUseFindSettings = false;
    bool _smartHighlightAnotherView = false;

    bool _printLineNumber = true;
    int _printOption = 3;
    QString _printHeaderLeft;
    QString _printHeaderMiddle;
    QString _printHeaderRight;
    QString _printFooterLeft;
    QString _printFooterMiddle;
    QString _printFooterRight;

    int _multiInstSetting = 0;
    QString _dateTimeFormat = "yyyy-MM-dd HH:mm:ss";
    bool _dateTimeReverseDefaultOrder = false;

    int _leftmostDelimiter = 40;
    int _rightmostDelimiter = 41;
    bool _delimiterSelectionOnEntireDocument = false;

    int _urlMode = 2;
    QString _uriCustomizedSchemes =
        "svn:// cvs:// git:// imap:// irc:// irc6:// ircs:// ldap:// "
        "ldaps:// news: telnet:// gopher:// ssh:// sftp:// smb:// skype: "
        "snmp:// spotify: steam:// sms: slack:// chrome:// bitcoin:";
    bool _enableTagsMatchHighlight = true;
    bool _enableTagAttrsHighlight = true;
    bool _highlightNonHtmlZone = false;
    bool _fillFindFieldWithSelected = true;
    bool _fillFindFieldSelectCaret = true;
    bool _monospacedFontFindDlg = false;
    bool _findDlgAlwaysVisible = false;
    bool _confirmReplaceInAllOpenDocs = true;
    bool _replaceStopsWithoutFindingNext = false;
    bool _showOnlyOneEntryPerFoundLine = true;
    bool _autoInsertParentheses = false;
    bool _autoInsertBrackets = false;
    bool _autoInsertCurlyBrackets = false;
    bool _autoInsertQuotes = false;
    bool _autoInsertDoubleQuotes = false;
    bool _autoInsertHtmlXmlTag = false;
    int _searchEngineChoice = 2;
    QString _searchEngineCustom;

};

struct FindHistoryState {
    int nbMaxPath = 10;
    int nbMaxFilter = 10;
    int nbMaxFind = 10;
    int nbMaxReplace = 10;

    bool matchWord = false;
    bool matchCase = false;
    bool wrap = true;
    bool directionDown = true;
    bool fifRecursive = true;
    bool fifInHiddenFolder = false;
    bool fifFilterFollowsDoc = false;
    bool fifFolderFollowsDoc = false;
    bool fifProjectPanel1 = false;
    bool fifProjectPanel2 = false;
    bool fifProjectPanel3 = false;
    int searchMode = 0;
    int transparencyMode = 1;
    int transparency = 150;
    bool dotMatchesNewline = false;
    bool isSearch2ButtonsMode = false;

    QStringList paths;
    QStringList filters;
    QStringList finds;
    QStringList replaces;
};

// ── NppParameters 单例（对应原版 NppParameters） ─────────────────────────────

class TiXmlElement;
class TiXmlNode;

class NppParameters
{
public:
    static NppParameters& getInstance();

    // 加载所有配置文件（config.xml, session.xml）
    bool load();

    // 保存 config.xml
    bool writeNppGUI();
    bool writeFindHistory();

    // 保存 session.xml
    bool writeSession(const Session& session);
    bool writeSession(const QString& filePath, const Session& session);
    bool loadSession(const QString& filePath);

    // 访问器
    NppGUI&              getNppGUI()        { return _nppGUI; }
    const NppGUI&        getNppGUI()  const { return _nppGUI; }
    FindHistoryState&       getFindHistory()       { return _findHistory; }
    const FindHistoryState& getFindHistory() const { return _findHistory; }
    const QString& workspaceFilePath(int panel) const;
    void setWorkspaceFilePath(int panel, const QString& path);
    ScintillaViewParams& getSVP()           { return _svp; }
    const ScintillaViewParams& getSVP() const { return _svp; }
    Session&             getSession()       { return _session; }
    const Session&       getSession() const { return _session; }

    QString getNppPath()  const { return _nppPath;  }
    QString getUserPath() const { return _userPath; }
    QString getConfigFilePath() const { return configFilePath(); }
    ConfigPathSource configPathSource() const { return _configPathSource; }
    QString configPathError() const { return _configPathError; }
    bool setUserPathOverride(const QString& path);
    void setStartupLocalizationFile(const QString& fileName);

    // 界面语言（与原版一致，通过 nativeLang.xml 文件标识）
    QString getNativeLang() const;
    void    setNativeLang(const QString& lang);

    // NativeLangSpeaker（对应原版 Notepad_plus::_nativeLangSpeaker）
    NativeLangSpeaker&       getNativeLangSpeaker()       { return _nativeLangSpeaker; }
    const NativeLangSpeaker& getNativeLangSpeaker() const { return _nativeLangSpeaker; }
    // 重新加载语言文件（切换语言时调用）
    void reloadNativeLang();

    // langs.xml ── 语言/扩展名映射
    bool loadLangs();
    const LangDesc* getLangDescByExt(const QString& ext)  const;
    const LangDesc* getLangDescByName(const QString& name) const;
    const QVector<LangDesc>& getLangDescs() const { return _langDescs; }

    // stylers.xml ── 颜色主题
    bool loadStylers();
    bool writeStylers();
    const LexerStyler* getLexerStyler(const QString& nppLexerName) const;
    QVector<LexerStyler>& getLexerStylers() { return _lexerStylers; }
    const QVector<LexerStyler>& getLexerStylers() const { return _lexerStylers; }
    QVector<WordsStyle>& getGlobalStyles() { return _globalStyles; }
    const QVector<WordsStyle>& getGlobalStyles() const { return _globalStyles; }

    // userDefineLang.xml - user-defined languages (read-only compatibility).
    bool loadUserDefinedLanguages();
    bool writeUserDefinedLanguage(const UserLangDesc& language);
    bool createUserDefinedLanguage(const QString& name);
    bool deleteUserDefinedLanguage(const QString& name,
                                   const QString& sourceFilePath = QString());
    bool renameUserDefinedLanguage(const QString& oldName,
                                   const QString& newName,
                                   const QString& sourceFilePath = QString());
    bool importUserDefinedLanguages(const QString& filePath);
    bool exportUserDefinedLanguage(const QString& name,
                                   const QString& destinationPath) const;
    const QVector<UserLangDesc>& getUserLangs() const { return _userLangs; }
    const UserLangDesc* getUserLangByName(const QString& name) const;
    const UserLangDesc* getUserLangByExt(const QString& ext) const;

    // shortcuts.xml ── 宏
    bool loadShortcuts();
    bool writeShortcuts();
    QVector<MacroDef>& getMacros() { return _macros; }
    const QVector<MacroDef>& getMacros() const { return _macros; }
    QVector<UserCommandDef>& getUserCommands() { return _userCommands; }
    const QVector<UserCommandDef>& getUserCommands() const { return _userCommands; }
    QVector<InternalCommandShortcut>& getInternalCommandShortcuts()
        { return _internalCommandShortcuts; }
    const QVector<InternalCommandShortcut>& getInternalCommandShortcuts() const
        { return _internalCommandShortcuts; }
    QVector<ScintillaKeyDef>& getScintillaKeys()
        { return _scintillaKeys; }
    const QVector<ScintillaKeyDef>& getScintillaKeys() const
        { return _scintillaKeys; }
    void setInternalCommandShortcut(int id, const QKeySequence& sequence);

    // 备份目录路径
    QString backupDirPath() const { return _userPath + "/backup"; }

private:
    NppParameters();
    ~NppParameters() = default;
    NppParameters(const NppParameters&) = delete;
    NppParameters& operator=(const NppParameters&) = delete;

    void initDefaultPaths();
    bool ensureConfigDir();
    void ensureDefaultXmlFiles() const;
    void migrateFromQSettings(); // 从旧版 QSettings 一次性迁移

    bool loadConfig();
    QString configFilePath()    const;
    QString sessionFilePath()   const;
    QString nativeLangFilePath() const;
    QString qtStateFilePath() const;
    QString langsFilePath()     const;
    QString stylersFilePath()   const;
    QString shortcutsFilePath() const;
    QString contextMenuFilePath() const;
    QString toolbarIconsFilePath() const;
    QString userDefineLangFilePath() const;

    void feedGUIConfig(const TiXmlElement* el);
    void loadFindHistory(TiXmlNode* root);

    bool writeConfigXml(const QString& filePath);
    void loadQtState();
    bool writeQtState() const;
    bool writeSessionXml(const QString& filePath, const Session& session);

    NppGUI              _nppGUI;
    FindHistoryState    _findHistory;
    ScintillaViewParams _svp;
    Session             _session;
    NativeLangSpeaker   _nativeLangSpeaker;

    QString _nppPath;  // exe 目录
    QString _userPath; // 配置目录
    ConfigPathSource _configPathSource = ConfigPathSource::PlatformDefault;
    QString _configPathError;
    QString _startupLocalizationFile;

    QVector<LangDesc>    _langDescs;
    QVector<LexerStyler> _lexerStylers;
    QVector<WordsStyle>  _globalStyles;
    QVector<UserLangDesc> _userLangs;
    QVector<MacroDef>    _macros;
    QVector<UserCommandDef> _userCommands;
    QVector<InternalCommandShortcut> _internalCommandShortcuts;
    QVector<ScintillaKeyDef> _scintillaKeys;
    QString _workspaceFilePaths[3];
};

#endif // PARAMETERS_H
