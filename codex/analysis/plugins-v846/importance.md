# Notepad++ v8.4.6 x86 插件重要度首评

## 1. 目的和边界

本文件为 `pl.x86.json` 的 169 个插件增加“重要度”评价，用于决定兼容层验证、
新 API 设计和源码移植的先后顺序。重要度与 A/B/C/D/U 技术兼容等级相互独立：
重要插件可能难以兼容，容易兼容的插件也可能价值较低。

本轮没有可靠、统一的安装量数据，因此不把 GitHub star、单个帖子或仍在官方清单
中直接等同于用户数量。社区材料只作为部分重点插件的定性信号；其余项目按功能的
通用性、替代成本、是否填补编辑器能力缺口和受众范围进行首评。功能判断的置信度
统一保守标为“低”，后续应由匿名安装统计、用户调查或社区维护者反馈校正。

## 2. 评级标准

| 等级 | 含义 | 移植用途 |
| --- | --- | --- |
| 高 | 广泛、关键或替代成本高，且通常存在社区信号或明显通用需求 | 优先进入兼容验证或新 API 路线 |
| 中 | 对一类常见工作流有价值，但受众、替代方案或证据有限 | 在公共宿主能力形成后分批处理 |
| 低 | 窄领域、旧平台耦合、娱乐/装饰性功能或容易由外部工具替代 | 默认不驱动宿主设计，按社区需求处理 |

依据代码：`C1` 为 Notepad++ 社区具体工作流推荐；`C2` 为多份社区/插件推荐材料；
`F` 为本项目依据功能通用性和替代成本作出的判断。社区证据只提升置信度，不自动
决定等级。

## 3. 汇总

| 重要度 | 数量 | A | B | C | D | U |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 高 | 20 | 1 | 5 | 7 | 0 | 7 |
| 中 | 94 | 9 | 13 | 12 | 0 | 60 |
| 低 | 55 | 3 | 5 | 6 | 1 | 40 |

优先决策时应先看“高重要度 × B/C”：B 类适合作为有限兼容层验证样本，C 类用于
提炼新 API 和源码移植需求。高重要度 U 类应优先补源码、使用量和 Windows 行为
证据，而不是直接假设兼容或不兼容。

## 4. 逐项评价

| folder-name | 插件 | 重要度 | 置信度 | 依据 | 兼容等级 | 简要理由 |
| --- | --- | :---: | :---: | --- | :---: | --- |
| 3P | 3P - Progress Programmers Pal | 低 | 低 | F | C | 面向较窄场景、旧平台集成或可替代功能；Designed to help writing OpenEdge ABL (formerly known as Progress 4GL) code. |
| ActiveX | ActiveX Plugin | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Allows you to control Notepad++ via ActiveX. |
| AnalysePlugin | AnalysePlugin | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；AnalysePlugin will help you to search for more than one search pattern at a time. |
| nppAutoDetectIndent | Auto Detect Indention | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Detects indention (tab or spaces) and auto adjust Tab key on-the-fly. |
| AutoCodepage | AutoCodepage | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Automatically sets a document's code page to your needs on loading or renaming a document… |
| AutoEolFormat | AutoEolFormat | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；A plugin to automatically set a document's EOL (End Of Line) format to your needs on load… |
| NppScripts | Automation Scripts | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；C# based automation. |
| AutoSave | AutoSave | 高 | 中 | F | U | 自动保存属于广泛用户可感知的数据保护能力。 |
| BetterMultiSelection | BetterMultiSelection | 中 | 低 | F | A | 覆盖一定用户群或可复用工作流；Provides better cursor movements when using multiple selections. |
| BigFiles | BigFiles - Open Very Large Files | 高 | 中 | F | B | 弥补超大文本读取这一编辑器核心能力缺口。 |
| BookmarksDook | Bookmarks@Dook | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Bookmarks panel |
| BracketsCheck | BracketsCheck | 中 | 低 | F | A | 覆盖一定用户群或可复用工作流；Helps to check if brackets in your file are balanced. |
| CADdyTools | CADdyTools | 低 | 低 | F | C | 面向较窄场景、旧平台集成或可替代功能；Notepad++-Plugin for manipulate CADdy-formated coordinate- and measure-textfiles |
| CodeAlignmentNpp | Code Alignment | 中 | 低 | F | A | 覆盖一定用户群或可复用工作流；Code alignment helps you present your code beautifully, enhancing clarity and readability. |
| CodeStats | Code::Stats | 低 | 低 | F | B | 面向较窄场景、旧平台集成或可替代功能；Write code, level up, show off! |
| ColumnTools | ColumnTools | 中 | 低 | F | C | 覆盖一定用户群或可复用工作流；Notepad++ Highlight Current Column and Horizontal Ruler |
| Comment Wrap | Comment Wrap | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Wraps comments as you type. |
| ComparePlugin | Compare | 高 | 高 | C2 | C | 文件比较是跨行业通用工作流，且有明确社区使用信号。 |
| ComparePlus | ComparePlus | 高 | 高 | C1+C2 | C | 高级文件比较是社区反复推荐的日常开发工作流。 |
| CSScriptNpp | CS-Script - C# Intellisense | 低 | 低 | F | C | 面向较窄场景、旧平台集成或可替代功能；CS-Script integration. |
| CSVLint | CSV Lint | 中 | 低 | F | C | 覆盖一定用户群或可复用工作流；Syntax highlighting and quality control for csv and fixed width data, detect column and d… |
| CsvQuery | CsvQuery | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Enables SQL queries against CSV files. |
| _CustomizeToolbar | Customize Toolbar | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Allows the toolbar to be fully customised by the user, and includes twenty-six additional… |
| CustomLineNumbers | CustomLineNumbers | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Displays line numbers as hex numbers. |
| DarkTheme | Dark Theme Mode | 低 | 低 | F | D | 面向较窄场景、旧平台集成或可替代功能；Invert colors of default white themes independent of style configuration |
| dbgpPlugin | DBGp | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；PHP debugger (XDebug) which talks with the DBGP protocol. |
| DiscordRPC | Discord Rich Presence | 低 | 低 | F | A | 面向较窄场景、旧平台集成或可替代功能；Shows in discord the file that is currently being edited in Notepad++. |
| ColorPicker | Don Rowlett Color Picker | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Developer tool for selecting color codes in various formats. |
| DoxyIt | DoxyIt | 中 | 低 | F | B | 覆盖一定用户群或可复用工作流；Supports creating Doxygen comments. |
| DSpellCheck | DSpellCheck | 高 | 中 | F | C | 拼写检查适用于文本、文档和代码注释，覆盖面广。 |
| NppEditorConfig | EditorConfig | 高 | 中 | F | U | EditorConfig 是跨编辑器项目规范，直接影响团队一致性。 |
| ElasticTabstops | ElasticTabstops | 中 | 低 | F | B | 覆盖一定用户群或可复用工作流；Support for Elastic Tabstops. |
| EnhanceAnyLexer | EnhanceAnyLexer | 中 | 低 | F | C | 覆盖一定用户群或可复用工作流；Adds more styling, colors only, options to any Lexer |
| ERPHelper | ERP Helper | 低 | 低 | F | C | 面向较窄场景、旧平台集成或可替代功能；A set of utilities for developing ERP integrations, includes XSL transformation and Workd… |
| Explorer | Explorer | 高 | 中 | F | C | 文件浏览是编辑器项目工作流中的通用能力。 |
| PlanetCNCNpp32 | Expression calculator | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Expression calculator plugin for Notepad++ |
| ExtSettings | ExtSettings | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Set various Scintilla settings which are not available via Notepad++ preferences dialog. |
| NPPFSIPlugin | F# Interactive | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Hosts F# Interactive inside Notepad++. |
| FallingBricks | Falling Bricks | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Simple tetris-like game to play from within Notepad++. |
| FileSwitcher | File Switcher | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Allows you to switch the active buffer using just the keyboard. |
| FingerText | FingerText | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；A Snippet plugin with support for multiple hotspots. |
| FWDataViz | Fixed-width Data Visualizer | 中 | 低 | F | C | 覆盖一定用户群或可复用工作流；Fixed Width Data Visualizer adds Excel-like features for fixed-width data files in Notepa… |
| GedcomLexer | GEDCOM Lexer | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；View and edit GEDCOM files with syntax highlighting of: level, xref id, tag, pointer, val… |
| GitSCM | GitSCM | 中 | 低 | F | C | 覆盖一定用户群或可复用工作流；N++ Gui for already installed Git SCM for Windows. |
| GmodLua | Gmod Lua Lexer | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；A Garry's Mod 10 lua syntax highlighter plugin. |
| GOnpp | GOnpp | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Assists you writing Go-programs. |
| GotoLineCol | GotoLineCol | 中 | 低 | F | B | 覆盖一定用户群或可复用工作流；A plugin to navigate to a specified line and (byte-based or character-based) column posit… |
| GrepBugsPluginNpp | GrepBugs | 低 | 低 | F | B | 面向较窄场景、旧平台集成或可替代功能；Downloads the latest regular expressions from GrepBugs.com and grep for matches in all op… |
| HexEditor | HEX-Editor | 高 | 高 | C2 | C | 二进制查看与编辑是明确且难以由普通文本功能替代的能力。 |
| HTMLTag_unicode | HTML Tag | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Provides three core functions: - HTML and XML tag jumping, like the built-in brace matchi… |
| HugeFiles | HugeFiles | 高 | 中 | F | B | 分块读取超大文件解决日志和数据文件的常见痛点。 |
| ImgTag | ImgTag | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Insert IMG tags, in your HTML document, using the Open File dialog box to select image fi… |
| IndentByFold | Indent By Fold | 中 | 低 | F | C | 覆盖一定用户群或可复用工作流；Indent using Fold points Note: Disable Notepad++'s Auto Indent in Settings - Preferences… |
| iTimeTrack | iTimeTrack | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Automated time tracking tool for programmers. |
| NppJavaPlugin | Java Plugin | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Allows Java Code Compilation and Execution directly from Notepad++. |
| JsMapParser.NppPlugin | JavaScript Map Parser | 低 | 低 | F | C | 面向较窄场景、旧平台集成或可替代功能；Provides better JavaScript support. |
| jN | jN Notepad++ Plugin | 中 | 低 | F | C | 覆盖一定用户群或可复用工作流；Allows you to extend Notepad++ by using JavaScript. |
| JSFunctionViewer | JSFunctionViewer | 中 | 低 | F | C | 覆盖一定用户群或可复用工作流；Provides an easier way to view and/or navigate to functions from function calls. |
| JSLintNpp | JSLint | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Allows running JSLint (The JavaScript Code Quality Tool) against open JavaScript files (m… |
| JsonTools | JSON Tools | 高 | 高 | C1 | B | JSON 格式化、校验、查询和树视图是常见开发工作流。 |
| NPPJSONViewer | JSON Viewer | 高 | 中 | F | B | JSON 树形查看是常见数据检查需求。 |
| JSMinNPP | JSTool | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Javascript plugin. |
| LanguageHelp | LanguageHelp | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Allows loading a language specific help file (CHM, HLP, PDF) and search for the keyword u… |
| lexamples | lexamples | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；External lexer package with improved lexers for Makefiles and MIB/ASN.1 files. |
| LightExplorer | Light Explorer | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Allows documents to be opened from a dockable file explorer that is very light weight and… |
| Linefilter3 | Linefilter3 | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Allows filtering for a given text and display the matching lines in a new window. |
| Linter | Linter | 中 | 低 | F | B | 覆盖一定用户群或可复用工作流；Allows realtime code check against any checkstyle-compatible linter: jshint, eslint, jscs… |
| LocationNavigate | Location Navigate | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Navigate between your last edit/view points. |
| LuaScript | LuaScript | 中 | 低 | F | C | 覆盖一定用户群或可复用工作流；Adds Lua scripting capabilities. |
| NppMarkdownPanel | Markdown Panel | 高 | 中 | F | U | Markdown 预览是跨平台版本应具备的常用文档能力。 |
| MarkdownViewerPlusPlus | MarkdownViewer++ | 高 | 中 | F | C | Markdown 实时预览覆盖广泛的文档工作流。 |
| MenuIcons | MenuIcons | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Allows adding icons to both main and context menu. |
| Merge files in one | Merge files in one | 中 | 低 | F | B | 覆盖一定用户群或可复用工作流；Copy lines from multiple files into one. |
| mimeTools | Mime tools | 高 | 中 | F | A | 编码/解码属于通用文本处理能力，并长期随官方发行生态提供。 |
| MultiClipboard | MultiClipboard | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Implements multiple (10) text buffers that are filled via copying and/or cutting of text. |
| MusicPlaye_1.0.11x86r | MusicPlayer | 低 | 低 | F | B | 面向较窄场景、旧平台集成或可替代功能；Open and play music files. |
| MZC8051 | MZC8051 | 低 | 低 | F | B | 面向较窄场景、旧平台集成或可替代功能；a 8051 c compiler plugin within notepad++. |
| NativeLang | NativeLang | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Helper plugin that allows other plugins to translate their menus and dialogs. |
| NavigateTo | NavigateTo | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Do you have more then 10 open tabs? |
| NewFileBrowser | NewFileBrowser | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Define 20 new file's initial text and have an inner web browser which can run current file. |
| NppBplistPlugin | Notepad++ bplist plugin | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Supports viewing/editing binary plist files. |
| NppPluginDemo | Notepad++ Plugin Demo | 中 | 低 | F | B | v4.2 源码与真实 x64 DLL 已验证；基础命令、Dock 和行跳转可兼容，宽会话/通知示例不扩展。 |
| NppPluginTemplate | Notepad++ Plugin Template | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Template for making plugin development as easy and simple as possible. |
| NotepadStarterPlugin | NotepadStarterPlugin | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；When it is installed as a Notepad++ plugin or running NotepadStarter.exe in the Notepad++… |
| nppConverter | Npp Converter | 高 | 中 | F | B | v4.4 源码与真实 x64 DLL 已验证；ASCII/Hex、配置和 Conversion Panel 可兼容。 |
| nppRandomStringGenerator | npp Random String Generator | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Generates random strings with configurable output. |
| NppXmlTreeviewPlugin | Npp Xml Treeview | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Treeview visualization for XML files. |
| NppAutoIndent | NppAutoIndent | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Smart indentation for C-style languages, such as C/C++, PHP, and Java. |
| NppCalc | NppCalc | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Evaluate expressions in Notepad++. |
| nppcrypt | NppCrypt | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Encryption/decryption with various block ciphers, hash-algorithms, random-characters, enc… |
| NppEventExec | NppEventExec | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Allows automatically executing NppExec scripts on Notepad++ events. |
| NppExec | NppExec | 高 | 高 | C1+C2 | U | 命令、脚本和输出捕获是社区常用的通用自动化入口。 |
| NppExport | NppExport | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；True WYSIWYG exporter. |
| NppFavorites | NppFavorites | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Favorites plugin. |
| NppFTP | NppFTP | 高 | 中 | F | U | 远程文件传输仍是 Web 和运维用户的重要工作流。 |
| NppGist | NppGist | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Allows working with GitHub Gist (create, edit, remove, rename). |
| NppGTags | NppGTags | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Front-end to GNU Global source code tagging system (GTags). |
| NppHasher | NppHash | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Computes the hash of selected text. |
| NppJumpList | NppJumpList | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Adds Windows 7 jump list support. |
| NppMenuSearch | NppMenuSearch | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Adds a text field to the toolbar for searching menu items and preference dialog options. |
| NppDocShare | NppNetNote | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Allows the same document to be edited in real time on two different computers. |
| NppPluginOpenHost | NppPluginOpenHost | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Allow to open Host file on Windows |
| NppQrCode32 | NppQrCode | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Creates QR-Code from selected text. |
| NppRegExTractorPlugin | NppRegExTractor | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Search one or more regular expression in one or more different files and get XML search r… |
| NppTags | NppTags | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；NppTags is a Universal Ctags plug-in to browse through your sources easily and lets you j… |
| NppTextViz | NppTextViz | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Hide or show lines to help analyse larger files - logs for example. |
| NppUISpy | NppUISpy | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Determine the menu command ID's of Notepad++ menu items and toolbar buttons. |
| nppplugin_ofis2 | Open File In Solution | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Lets you index specific folders and possible specific types of resources (XML, CPP, PY fi… |
| NWScript-Npp | NWScript Tools | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；View, edit and compile Bioware's NWScript files with this plugin. |
| OpenSelection | OpenSelection | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Open files based on the selected text. |
| Papyrus | Papyrus Script Lexer | 低 | 低 | F | C | 面向较窄场景、旧平台集成或可替代功能；View and edit Papyrus Script files used by Bethesda games with syntax highlighting, funct… |
| ccc | PHP Autocompletion | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Implements code completion for custom PHP classes. |
| PlantUmlViewer | PlantUML Viewer | 中 | 低 | F | C | 覆盖一定用户群或可复用工作流；A Notepad++ plugin to generate view and export PlantUML diagrams. |
| PoorMansTSqlFormatterNppPlugin | Poor Man's T-Sql Formatter | 中 | 低 | F | A | 覆盖一定用户群或可复用工作流；Simple SQL formatter performing full multi-batch T-SQL formatting (individual statements,… |
| pork2sausage | Pork to Sausage | 中 | 低 | F | B | 覆盖一定用户群或可复用工作流；Pass any selected text to any command line program as input and take the output (the resu… |
| PreviewHTML | Preview HTML | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Preview HTML files inside Notepad++ (or in a floating window) without having to save them… |
| PyNPP | PyNPP | 中 | 低 | F | A | 覆盖一定用户群或可复用工作流；Allows writing Python scripts and run them from Notepad++ without having to open a comman… |
| Python Indent | Python Indent | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Python auto-indent plugin. |
| PythonScript | PythonScript | 高 | 高 | C1+C2 | C | 脚本化编辑器是社区常用的高扩展性自动化能力。 |
| NppQCP | Quick Color Picker + | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；HEX/RGB/RGBA/HSL/HSLA color code highlighter. |
| QuickOpenPlugin | QuickOpenPlugin | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Mimics the "open selected file" in PSPad. |
| QuickText | QuickText | 中 | 低 | F | B | 覆盖一定用户群或可复用工作流；Quick text substitution, including multi-field inputs. |
| RandomValuesNppPlugin | Random Values | 中 | 低 | F | B | 覆盖一定用户群或可复用工作流；Random values generator for passwords or test data. |
| rdmd-en-x86 | RDMD for Notepad++ (English) | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Runs rdmd in Notepad++ (English). |
| rdmd-ja-x86 | RDMD for Notepad++ (Japanese) | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Runs rdmd in Notepad++ (Japanese). |
| RegexTrainer | Regex Trainer | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Regex Trainer (based on net framework 4) that supports a complex regular expression. |
| Remove Duplicate Lines | Remove Duplicate Lines | 中 | 低 | F | A | 覆盖一定用户群或可复用工作流；Remove duplicate lines without removing empty lines. |
| RestApiToText | RestApiToText | 中 | 低 | F | B | 覆盖一定用户群或可复用工作流；Make REST API calls using content from an editor tab, then see the results in a new tab. |
| qkNppReverseLines | Reverse Lines | 中 | 低 | F | A | 覆盖一定用户群或可复用工作流；Reverse lines in the selection or document. |
| RunMe | RunMe | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Execute the currently open file, based on its shell association. |
| NppSaveAsAdmin | Save as admin | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Allows saving file as administrator with Windows UAC prompt. |
| SecurePad | SecurePad | 中 | 低 | F | A | 覆盖一定用户群或可复用工作流；Encrypt/decrypt whole documents or selected text with your own key. |
| selectNLaunch | Select N' Launch | 低 | 低 | F | A | 面向较窄场景、旧平台集成或可替代功能；Get the selected text, save it as file with the extension you customized in the system te… |
| SelectToClipboard | Select to Clipboard | 低 | 低 | F | B | 面向较窄场景、旧平台集成或可替代功能；Auto copy selected text to clipboard. |
| SelectQuotedText | SelectQuotedText | 中 | 低 | F | A | 覆盖一定用户群或可复用工作流；Select the text in quotes (aka a string) based on the Scintilla lexers in Notepad++. |
| SessionMgr | Session Manager | 中 | 低 | F | B | 覆盖一定用户群或可复用工作流；Session manager. |
| SherloXplorer | SherloXplorer | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Explorer-like functionality (requires .NET 2.0). |
| ShtirlitzNppPlugin | Shtirlitz | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Adds menu listing decoding styles. |
| NppSnippets | Snippets | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Adds the possibility to add code snippets to the current document by selecting them from… |
| nppplugin_solutionhub | Solution Hub | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Base requirement for several plugins from incfred. |
| nppplugin_solutionhub_ui | Solution Hub UI | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Basic UI to create and setup solutions used by the SolutionHub. |
| nppplugin_solutiontools | Solution Tools | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Configurable priority-based fileswitching (most commonly used when switching between .h a… |
| SourceCookifier | Source Cookifier | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Use Exuberant Ctags to parse either only the currently activated source file or multiple… |
| SpeechPlugin | SpeechPlugin | 低 | 低 | F | A | 面向较窄场景、旧平台集成或可替代功能；No kidding, Notepad++ speaks now. |
| SpellChecker | Spell-Checker | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Correct your typos in your language. |
| SQLinFormNpp | SQLinForm | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Formats plain SQL, SQL embedded in program code, SQL snippets, and SQL statements with sy… |
| SurroundSelection | SurroundSelection | 中 | 低 | F | B | 覆盖一定用户群或可复用工作流；Automatically surround the selection in quotes/brackets/parenthesis/etc. |
| TagLEET | TagLEET | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Ctags browser. |
| TagsView | TagsView | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Provides user interface for ctags parsed results. |
| TakeNotes | TakeNotes | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Helps people who like to use Notepad++ for jotting quick notes. |
| NppTaskList | Task List | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Automatically scans the open document and adds all "TODO:*" items to your task list, a wi… |
| NppTextFX | TextFX Characters | 高 | 中 | F | U | 聚合大量通用文本变换，历史使用面和替代成本较高。 |
| NppTFS | TFS Work Item | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Attaches opened (in the current tab) file to the TFS work item. |
| Tidy2 | Tidy2 | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；HTML Tidy with support for HTML5. |
| NppToolBucket | ToolBucket | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Requires .NET 3.5 Multi-line search and replace dialog. |
| TopMost | TopMost | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Allows setting the main Notepad++ window as a topmost window so it can stay on top of oth… |
| nppplugin_svn | Tortoise SVN | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Main operations for SVN, with a concept of a root solution directory. |
| Translate | Translate | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Provides quick translation of selected text to your language of choice. |
| urlPlugin | URL Encode/Decode Plugin | 中 | 低 | F | B | 覆盖一定用户群或可复用工作流；Hopefully a decent URL Encoder and Decoder plug-in for Notepad++ which helps to make deve… |
| VisualStudioLineCopy | Visual Studio Line Copy | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Adds two commands to Notepad++ CopyAllowLine and CutAllowLine, which adds Visual Studio s… |
| WakaTime | WakaTime | 中 | 低 | F | B | 覆盖一定用户群或可复用工作流；Automatic time tracking and metrics generated from your programming activity. |
| WebEdit | WebEdit | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；WebEdit is another attempt to integrate a user-configurable code template collection into… |
| WindowManager | Window Manager | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Gives a short overview of open documents. |
| WLangLexer | WLangLexer | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Lexer for the WDScript language: WDScript and Linux portability of WDScript project. |
| XBrackets | XBrackets Lite | 中 | 低 | F | C | 覆盖一定用户群或可复用工作流；Allows autocompletion of brackets ([{""}]) Inserts corresponding right bracket when the l… |
| XMLTools | XML Tools | 高 | 高 | C1+C2 | B | XML 格式化、校验和查询是社区反复推荐的通用开发能力。 |
| XPatherizerNPP | XPatherizerNPP | 中 | 低 | F | U | 覆盖一定用户群或可复用工作流；Analyze multiple XPath queries with reverse lookup. |
| ZenCoding-Python | Zen Coding - Python | 中 | 低 | F | C | 覆盖一定用户群或可复用工作流；Implementation of the Zen Coding method by Sergey Chikuyonok. |
| zoomdisabler | Zoom Disabler | 低 | 低 | F | U | 面向较窄场景、旧平台集成或可替代功能；Tired of zooming your document everytime you just want to scroll but accidentally still h… |

## 5. 社区证据

- `C1`：[Notepad++ Community：插件生产力讨论](https://community.notepad-plus-plus.org/topic/27434/)，明确提到 ComparePlus、NppExec、PythonScript、XMLTools 和 JsonTools 等实际工作流。
- `C2`：[Notepad++ Community：Must-Have plugins](https://community.notepad-plus-plus.org/topic/21830/must-have-plugins-what-are-yours/14)、[插件推荐汇总](https://www.notepadplus.com.cn/en/plugins.html)，用于确认 Compare、HexEditor、PythonScript、NppExec、XMLTools 等具有持续社区可见度。
- 官方 `nppPluginList` 只证明插件进入官方管理清单，不代表安装量或重要度；本轮不把清单存在性当作流行度排名。

## 6. 后续校正项

- [ ] 若插件管理器未来可在用户明确同意后收集匿名安装/启用统计，按架构和版本形成真实使用量。
- [ ] 在 Notepad++ 社区发布候选清单，收集“日常必需、偶尔使用、可替代、不再使用”反馈。
- [ ] 对高重要度 U 类优先联系维护者或查找精确版本源码。
- [ ] Windows x86 实测后，将可加载、可执行核心命令和稳定性结果加入重要度决策。
- [ ] 每次调整都记录依据日期，避免把当前判断永久固化为产品承诺。
