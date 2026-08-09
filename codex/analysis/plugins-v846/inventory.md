# Notepad++ v8.4.6 插件调查索引（仅 x86）

## 范围与状态约定

- 数据源：`resources/pluginList/windows/pl.x86.json`（清单版本 `1.5.4`，架构字段 `32`）。
- 本索引仅覆盖 x86 清单中的 `169` 个插件；不合并、推断或代表 x64/ARM64 清单。
- 一行对应一个原始 `folder-name`，顺序与 JSON 一致。版本、下载地址和主页均直接取自 x86 条目。
- 调查状态、源码状态和兼容等级已由各字母批次的首轮静态调查回填。
- `U` 表示没有取得足以对清单版本定级的源码证据，不能解释为不兼容；本轮未对
  无源码插件进行反汇编。

## 统计

| 项目 | 数量 |
| --- | ---: |
| x86 JSON 条目 | 169 |
| 唯一 `folder-name` | 169 |
| 重复 `folder-name` | 0 |
| 首版完成 | 169 |
| 等级 A / B / C / D / U | 13 / 25 / 25 / 1 / 105 |
| 重要度 高 / 中 / 低 | 20 / 94 / 55 |

## 插件清单

| # | folder-name | display-name | x86 版本 | repository | homepage | 调查状态 | 源码状态 | 重要度 | 等级 |
| ---: | --- | --- | --- | --- | --- | --- | --- | :---: | :---: |
| 1 | 3P | 3P - Progress Programmers Pal | 1.8.7 | https://github.com/jcaillon/3P/releases/download/v1.8.7/3P.zip | https://jcaillon.github.io/3P/ | 首版完成 | 有，对应版本 | 低 | C |
| 2 | ActiveX | ActiveX Plugin | 1.1.8.7 | https://sourceforge.net/projects/nppactivexplugin/files/bin/ActiveX_Unicode_1_1_8_7.zip | https://sourceforge.net/projects/nppactivexplugin/ | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 3 | AnalysePlugin | AnalysePlugin | 1.13.49.0 | https://sourceforge.net/projects/analyseplugin/files/binaries/v01.13-R49/AnalysePlugin-v01.13-R49-x86.zip | https://sourceforge.net/projects/analyseplugin | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 4 | nppAutoDetectIndent | Auto Detect Indention | 2.3 | https://github.com/Chocobo1/nppAutoDetectIndent/releases/download/2.3/x86.zip | https://github.com/Chocobo1/nppAutoDetectIndent | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 5 | AutoCodepage | AutoCodepage | 1.2.4 | https://sourceforge.net/projects/autocodepage/files/v1.2.4/plugin/x86/AutoCodepage_v1.2.4_UNI.zip | https://sourceforge.net/projects/autocodepage | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 6 | AutoEolFormat | AutoEolFormat | 1.0.2 | https://sourceforge.net/projects/autoeolformat/files/v1.0.2/plugin/x86/AutoEolFormat_v1.0.2_UNI.zip | https://sourceforge.net/projects/autoeolformat | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 7 | NppScripts | Automation Scripts | 2.0.0.0 | https://github.com/oleg-shilo/scripts.npp/releases/download/v2.0.0.0/NppScripts.x86.zip | https://github.com/oleg-shilo | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 8 | AutoSave | AutoSave | 1.6.1.0 | https://github.com/francostellari/NppPlugins/raw/main/AutoSave/AutoSave_dll_1v61_x32.zip | https://github.com/francostellari/NppPlugins | 首版完成 | 未找到对应版本/证据不足 | 高 | U |
| 9 | BetterMultiSelection | BetterMultiSelection | 1.5 | https://github.com/dail8859/BetterMultiSelection/releases/download/v1.5/BetterMultiSelection_v1.5.zip | https://github.com/dail8859/BetterMultiSelection | 首版完成 | 有，对应版本 | 中 | A |
| 10 | BigFiles | BigFiles - Open Very Large Files | 0.1.3 | https://github.com/superolmo/BigFiles/releases/download/v0.1.3.x86/BigFiles.zip | https://github.com/superolmo/BigFiles | 首版完成 | 有，对应版本 | 高 | B |
| 11 | BookmarksDook | Bookmarks@Dook | 2.3.3.0 | https://github.com/Dook1/Bookmarks-Dook/releases/download/23332b/BookmarksDook.32.2.3.3.zip | https://github.com/Dook1/Bookmarks-Dook/issues | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 12 | BracketsCheck | BracketsCheck | 1.2.2 | https://github.com/niccord/BracketsCheck/releases/download/v1.2.2/BracketsCheck_1-2-2_x86.zip | https://github.com/niccord/BracketsCheck/ | 首版完成 | 有，对应版本 | 中 | A |
| 13 | CADdyTools | CADdyTools | 1.1.3.7 | https://github.com/MarioRosi/CADdyTools/releases/download/1.1.3.7/CADdyTools_v1137_x86.zip | https://github.com/MarioRosi/CADdyTools | 首版完成 | 有，对应版本 | 低 | C |
| 14 | CodeAlignmentNpp | Code Alignment | 14.1.107 | https://github.com/cpmcgrath/codealignment/releases/download/v14.1/CodeAlignmentNpp_v14.1_x86.zip | https://github.com/cpmcgrath/codealignment | 首版完成 | 有，对应版本 | 中 | A |
| 15 | CodeStats | Code::Stats | 1.0.1 | https://github.com/p0358/notepadpp-CodeStats/releases/download/v1.0.1/notepadpp-CodeStats_x86.zip | https://github.com/p0358/notepadpp-CodeStats | 首版完成 | 有，对应版本 | 低 | B |
| 16 | ColumnTools | ColumnTools | 1.4.4.1 | https://github.com/vinsworldcom/nppColumnTools/releases/download/1.4.4.1/ColumnTools-v1.4.4.1-Win32.zip | https://github.com/vinsworldcom/nppColumnTools | 首版完成 | 有，对应版本 | 中 | C |
| 17 | Comment Wrap | Comment Wrap | 1.0.0.5 | https://sourceforge.net/projects/kered13-notepad-plugins/files/Comment%20Wrap%20Win32%20v1.0.0.5.zip | https://sourceforge.net/projects/kered13-notepad-plugins/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 18 | ComparePlugin | Compare | 2.0.2 | https://github.com/pnedev/compare-plugin/releases/download/v2.0.2/ComparePlugin_v2.0.2_x86.zip | https://github.com/pnedev/compare-plugin | 首版完成 | 有，对应版本 | 高 | C |
| 19 | ComparePlus | ComparePlus | 1.0.0 | https://github.com/pnedev/comparePlus/releases/download/cp_1.0.0/ComparePlus_1.0.0_x86.zip | https://github.com/pnedev/comparePlus | 首版完成 | 有，对应版本 | 高 | C |
| 20 | CSScriptNpp | CS-Script - C# Intellisense | 2.0.2.0 | https://github.com/oleg-shilo/cs-script.npp/releases/download/v2.0.2.0/CSScriptNpp.2.0.2.0.x86.zip | https://github.com/oleg-shilo/cs-script.npp | 首版完成 | 有，对应版本 | 低 | C |
| 21 | CSVLint | CSV Lint | 0.4.5.4 | https://github.com/BdR76/CSVLint/releases/download/0.4.5.4/CSVLint_x86.zip | https://github.com/BdR76/CSVLint/ | 首版完成 | 有，对应版本 | 中 | C |
| 22 | CsvQuery | CsvQuery | 1.2.9 | https://github.com/jokedst/CsvQuery/releases/download/v1.2.9/CsvQuery-v1.2.9-x86.zip | https://github.com/jokedst/CsvQuery | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 23 | _CustomizeToolbar | Customize Toolbar | 5.3 | https://sourceforge.net/projects/npp-customize/files/Customize%20Toolbar%20v5.3/CustomizeToolbar_5_3_Win32_UNI.zip | https://sourceforge.net/projects/npp-customize | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 24 | CustomLineNumbers | CustomLineNumbers | 1.1.7 | https://sourceforge.net/projects/customlinenumbers/files/v1.1.7/plugin/x86/CustomLineNumbers_v1.1.7_UNI.zip | https://sourceforge.net/projects/customlinenumbers | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 25 | DarkTheme | Dark Theme Mode | 1.0 | https://github.com/MyTDT-Mysoft/DarkTheme-Npp-Plugin/releases/download/v1.0/DarkTheme.zip | https://github.com/MyTDT-Mysoft/DarkTheme-Npp-Plugin | 首版完成 | 有，对应版本 | 低 | D |
| 26 | dbgpPlugin | DBGp | 0.0.13.27 | https://sourceforge.net/projects/npp-plugins/files/DBGP%20Plugin/DBGP%20Plugin%20v0.13%20beta/DBGpPlugin_0_13b_dll.zip | https://sourceforge.net/projects/npp-plugins/files/DBGP%20Plugin/ | 首版完成 | 未找到对应版本 | 低 | U |
| 27 | DiscordRPC | Discord Rich Presence | 1.4.262.1 | https://github.com/Zukaritasu/notepadpp_rpc/releases/download/v1.4/DiscordRPC_v1.4_x86.zip | https://github.com/Zukaritasu/notepadpp_rpc | 首版完成 | 有，对应版本 | 低 | A |
| 28 | ColorPicker | Don Rowlett Color Picker | 2.3 | https://sourceforge.net/projects/npp-plugins/files/ColorPicker/Color%20Picker%20v.2.3/ColorPicker_230_dll.zip | https://sourceforge.net/projects/npp-plugins/files/ColorPicker/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 29 | DoxyIt | DoxyIt | 0.4.4 | https://github.com/dail8859/DoxyIt/releases/download/v0.4.4/DoxyIt_v0.4.4.zip | https://github.com/dail8859/DoxyIt | 首版完成 | 有，对应版本 | 中 | B |
| 30 | DSpellCheck | DSpellCheck | 1.4.24 | https://github.com/Predelnik/DSpellCheck/releases/download/v1.4.24/DSpellCheck_x86.zip | https://github.com/Predelnik/DSpellCheck | 首版完成 | 有，对应版本 | 高 | C |
| 31 | NppEditorConfig | EditorConfig | 0.4.0 | https://github.com/editorconfig/editorconfig-notepad-plus-plus/releases/download/v0.4.0/NppEditorConfig-040-x86.zip | https://github.com/editorconfig/editorconfig-notepad-plus-plus | 首版完成 | 未找到对应版本/证据不足 | 高 | U |
| 32 | ElasticTabstops | ElasticTabstops | 1.3.1 | https://github.com/dail8859/ElasticTabstops/releases/download/v1.3.1/ElasticTabstops_v1.3.1.zip | https://github.com/dail8859/ElasticTabstops | 首版完成 | 有，对应版本 | 中 | B |
| 33 | EnhanceAnyLexer | EnhanceAnyLexer | 1.1.3 | https://github.com/Ekopalypse/EnhanceAnyLexer/releases/download/v1.1.3/EnhanceAnyLexer_x86_PluginAdmin.zip | https://github.com/Ekopalypse/EnhanceAnyLexer | 首版完成 | 有，对应版本 | 中 | C |
| 34 | ERPHelper | ERP Helper | 1.1.2 | https://github.com/swhitley/ERPHelper/releases/download/v1.1.2/ERPHelper_x86.zip | https://github.com/swhitley/erphelper | 首版完成 | 有，对应版本 | 低 | C |
| 35 | Explorer | Explorer | 1.9.5.0 | https://github.com/oviradoi/npp-explorer-plugin/releases/download/v1.9.5/Explorer.zip | https://github.com/oviradoi/npp-explorer-plugin | 首版完成 | 有，对应版本 | 高 | C |
| 36 | PlanetCNCNpp32 | Expression calculator | 3001.21.1123.1 | https://github.com/PlanetCNC/PlanetCNCNpp/releases/download/release/PlanetCNCNpp32.zip | https://planet-cnc.com/notepad-plugin/ | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 37 | ExtSettings | ExtSettings | 1.2.1 | https://sourceforge.net/projects/extsettings/files/v1.2.1/plugin/x86/ExtSettings_v1.2.1_UNI.zip | https://sourceforge.net/projects/extsettings | 首版完成 | 未找到对应版本 | 中 | U |
| 38 | NPPFSIPlugin | F# Interactive | 0.1.1 | https://github.com/downloads/ppv/NPPFSIPlugin/NPPFSIPlugin.zip | https://github.com/ppv/NPPFSIPlugin | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 39 | FallingBricks | Falling Bricks | 1.1.0.0 | https://downloads.sourceforge.net/project/npp-plugins/FallingBricks/FallingBricks%201.1%20UNI/fallingbricks_v1.1_unicode_dll.zip | https://sourceforge.net/projects/npp-plugins/files/FallingBricks/ | 首版完成 | 未找到对应版本 | 低 | U |
| 40 | FileSwitcher | File Switcher | 1.0.3.0 | https://downloads.sourceforge.net/project/npp-plugins/File%20Switcher/FileSwitcher%201.0.3.0/FileSwitcher1030_UNI.zip | https://github.com/bruderstein/FileSwitcher | 首版完成 | 未找到对应版本 | 中 | U |
| 41 | FingerText | FingerText | 0.5.60 | https://downloads.sourceforge.net/project/fingertext/Alpha%20Releases/FingerText%20-%200.5.60.zip | https://sourceforge.net/projects/fingertext/ | 首版完成 | 未找到对应版本 | 中 | U |
| 42 | FWDataViz | Fixed-width Data Visualizer | 2.6.1.0 | https://github.com/shriprem/FWDataViz/releases/download/v2.6.1.0/FWDataViz_x86.zip | https://github.com/shriprem/FWDataViz | 首版完成 | 有，对应版本 | 中 | C |
| 43 | GedcomLexer | GEDCOM Lexer | 0.5.0.170 | https://sourceforge.net/projects/gedcomlexer/files/GedcomLexer-0.5.0-r170/GedcomLexer-0.5.0-r170-x86.zip | https://sourceforge.net/projects/gedcomlexer/ | 首版完成 | 未找到对应版本 | 低 | U |
| 44 | GitSCM | GitSCM | 1.4.7.1 | https://github.com/vinsworldcom/nppGitSCM/releases/download/1.4.7.1/GitSCM-v1.4.7.1-x86.zip | https://github.com/vinsworldcom/nppGitSCM | 首版完成 | 有，对应版本 | 中 | C |
| 45 | GmodLua | Gmod Lua Lexer | 1.5 | https://sourceforge.net/projects/npp-plugins/files/Gmod%20Lua%20Highlighter/Gmod%20Lua%20v1.5/NppGmodLuaPlugin-v1.5.zip | https://code.google.com/p/npp-gmod-lua/ | 首版完成 | 未找到对应版本 | 低 | U |
| 46 | GOnpp | GOnpp | 1.2.0.0 | https://sourceforge.net/projects/gonpp/files/GOnpp_1.2_UNI.zip | https://github.com/tike/GOnpp | 首版完成 | 未找到对应版本 | 低 | U |
| 47 | GotoLineCol | GotoLineCol | 2.4.2.0 | https://github.com/shriprem/Goto-Line-Col-NPP-Plugin/releases/download/v2.4.2.0/GotoLineCol_x86.zip | https://github.com/shriprem/Goto-Line-Col-NPP-Plugin | 首版完成 | 有，对应版本 | 中 | B |
| 48 | GrepBugsPluginNpp | GrepBugs | 1.0.0 | https://github.com/foospidy/GrepBugsPluginNotepadPlusPlus/releases/download/v1.0/GrepBugsPluginNpp.zip | https://grepbugs.com/plugins | 首版完成 | 有，对应版本 | 低 | B |
| 49 | HexEditor | HEX-Editor | 0.9.12.0 | https://github.com/chcg/NPP_HexEdit/releases/download/0.9.12/HexEditor_0.9.12_Win32.zip | https://github.com/chcg/NPP_HexEdit | 首版完成 | 有，对应版本 | 高 | C |
| 50 | HTMLTag_unicode | HTML Tag | 1.3.5.0 | https://bitbucket.org/rdipardo/htmltag/downloads/HTMLTag_v135.zip | https://bitbucket.org/rdipardo/htmltag/ | 首版完成 | 未找到对应版本 | 中 | U |
| 51 | HugeFiles | HugeFiles | 0.1.1.0 | https://github.com/molsonkiko/HugeFiles/releases/download/v0.1.1.0/Release_x86.zip | https://github.com/molsonkiko/HugeFiles/ | 首版完成 | 有，对应版本 | 高 | B |
| 52 | ImgTag | ImgTag | 2.0.1 | https://sourceforge.net/projects/imgtag/files/ImgTag_binary_unicode_2.0.1.zip | https://sourceforge.net/projects/imgtag/ | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 53 | IndentByFold | Indent By Fold | 0.7.3 | https://github.com/ffes/indentbyfold/releases/download/v0.7.3/IndentByFold-073-x32.zip | https://github.com/ffes/indentbyfold/ | 首版完成 | 有，对应版本 | 中 | C |
| 54 | iTimeTrack | iTimeTrack | 3.0.0 | https://github.com/itimetrack/itimetrack-notepadpp/releases/download/3.0.0/itimetrack-notepadpp-bin-3.0.0.zip | https://itimetrack.com | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 55 | NppJavaPlugin | Java Plugin | 0.4.0 | https://github.com/dominikcebula/npp-java-plugin/releases/download/v0.4.0/NppJavaPlugin_v0.4.0_x86.zip | https://github.com/dominikcebula/npp-java-plugin | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 56 | JsMapParser.NppPlugin | JavaScript Map Parser | 4.2 | https://github.com/megaboich/js-map-parser/releases/download/4.2/JsMapParser_NppPlugin_4_2_x86.zip | https://github.com/megaboich/js-map-parser/ | 首版完成 | 有，对应版本 | 低 | C |
| 57 | jN | jN Notepad++ Plugin | 2.2.185.8 | https://github.com/sieukrem/jn-npp-plugin/releases/download/2.2.185.8/jN_2.2.185.8_x86.zip | https://github.com/sieukrem/jn-npp-plugin/wiki | 首版完成 | 有，对应版本 | 中 | C |
| 58 | JSFunctionViewer | JSFunctionViewer | 1.1.0 | https://github.com/davidsover/nppJSFunctionViewer/releases/download/v1.1.0/JSFunctionViewer_x86.zip | https://github.com/davidsover/nppJSFunctionViewer | 首版完成 | 有，对应版本 | 中 | C |
| 59 | JSLintNpp | JSLint | 0.8.3.119 | https://downloads.sourceforge.net/project/jslintnpp/0.8.3/JSLintNPP.0.8.3.zip | https://sourceforge.net/projects/jslintnpp/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 60 | JsonTools | JSON Tools | 3.2.0 | https://github.com/molsonkiko/JsonToolsNppPlugin/releases/download/v3.2.0/Release_x86.zip | https://github.com/molsonkiko/JsonToolsNppPlugin | 首版完成 | 有，对应版本 | 高 | B |
| 61 | NPPJSONViewer | JSON Viewer | 1.41 | https://github.com/kapilratnani/JSON-Viewer/releases/download/v1.41/NPPJSONViewer_Win32.zip | https://github.com/kapilratnani/JSON-Viewer | 首版完成 | 有，对应版本 | 高 | B |
| 62 | JSMinNPP | JSTool | 1.2205.0 | https://sourceforge.net/projects/jsminnpp/files/Uni/JSToolNPP.1.2205.0.uni.32.zip | https://github.com/sunjw/jstoolnpp | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 63 | LanguageHelp | LanguageHelp | 1.7.5.0 | https://github.com/francostellari/NppPlugins/raw/main/LanguageHelp/LanguageHelp_dll_1v75_x32.zip | https://github.com/francostellari/NppPlugins | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 64 | lexamples | lexamples | 1.0.0.0 | https://sourceforge.net/projects/lexamples/files/v1.0.0/lexamples_1_0_0.zip | https://sourceforge.net/projects/lexamples | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 65 | LightExplorer | Light Explorer | 2.0.0.0 | https://downloads.sourceforge.net/project/npp-plugins/LightExplorer/LightExplorer%202.0%20UNICODE/LightExplorer_2_0_dll.zip | https://sourceforge.net/projects/npp-plugins/files/LightExplorer/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 66 | Linefilter3 | Linefilter3 | 1.0.0.0 | https://www.seelisoft.net/Linefilter3/Linefilter3_x86.zip | https://www.seelisoft.net/Linefilter3/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 67 | Linter | Linter | 0.1.0.0 | https://github.com/deadem/notepad-pp-linter/raw/v0.1.0.0/bin/x32/linter.zip | https://github.com/deadem/notepad-pp-linter | 首版完成 | 有，对应版本 | 中 | B |
| 68 | LocationNavigate | Location Navigate | 0.4.8.1 | https://sourceforge.net/projects/locationnav/files/LocationNavigate_v0.4.8.1_x86.zip | https://sourceforge.net/projects/locationnav/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 69 | LuaScript | LuaScript | 0.11 | https://github.com/dail8859/LuaScript/releases/download/v0.11/LuaScript_v0.11.zip | https://github.com/dail8859/LuaScript | 首版完成 | 有，对应版本 | 中 | C |
| 70 | NppMarkdownPanel | Markdown Panel | 0.6.2 | https://github.com/mohzy83/NppMarkdownPanel/releases/download/0.6.2/NppMarkdownPanel-0.6.2.0-x86.zip | https://github.com/mohzy83/NppMarkdownPanel | 首版完成 | 未找到对应版本/证据不足 | 高 | U |
| 71 | MarkdownViewerPlusPlus | MarkdownViewer++ | 0.8.2 | https://github.com/nea/MarkdownViewerPlusPlus/releases/download/0.8.2/MarkdownViewerPlusPlus-0.8.2-x86.zip | https://nea.github.io/MarkdownViewerPlusPlus/ | 首版完成 | 有，对应版本 | 高 | C |
| 72 | MenuIcons | MenuIcons | 1.2.5 | https://github.com/francostellari/NppPlugins/raw/main/MenuIcons/MenuIcons_dll_1v25_x32.zip | https://github.com/francostellari/NppPlugins | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 73 | Merge files in one | Merge files in one | 1.2.0.0 | https://github.com/gurikbal/Merge-files-in-one/releases/download/1.2.0.0/Merge.files.in.one_x86.zip | https://github.com/gurikbal/Merge-files-in-one | 首版完成 | 有，对应版本 | 中 | B |
| 74 | mimeTools | Mime tools | 2.8 | https://github.com/npp-plugins/mimetools/releases/download/v2.8/mimetools.v2.8.zip | https://github.com/npp-plugins/mimetools | 首版完成 | 有，对应版本 | 高 | A |
| 75 | MultiClipboard | MultiClipboard | 2.1.0.0 | https://downloads.sourceforge.net/project/npp-plugins/MultiClipboard/MultiClipboard%202.1%20unicode/MultiClipboard_2.1_unicode_dll.zip | https://sourceforge.net/projects/npp-plugins/files/MultiClipboard/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 76 | MusicPlaye_1.0.11x86r | MusicPlayer | 1.0.0.3 | https://github.com/gallettube/MusicPlayer/releases/download/1.0.11/MusicPlaye_1.0.11x86r.dll.zip | https://sourceforge.net/projects/nppmusicplayer | 首版完成 | 有，对应版本 | 低 | B |
| 77 | MZC8051 | MZC8051 | 0.0.1 | https://github.com/Jiangshan00001/npp_MZC8051/releases/download/0.0.1/MZC8051_x86.zip | https://github.com/Jiangshan00001/npp_MZC8051 | 首版完成 | 有，对应版本 | 低 | B |
| 78 | NativeLang | NativeLang | 1.1.0.0 | https://downloads.sourceforge.net/sourceforge/npp-plugins/NativeLang_1_2_dll.zip | https://sourceforge.net/projects/npp-plugins/files/NativeLang/ | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 79 | NavigateTo | NavigateTo | 1.12.7.0 | https://github.com/young-developer/nppNavigateTo/releases/download/v.1.12.7/NavigateTo_v.1.12.7_v142_x86.zip | https://github.com/young-developer/nppNavigateTo | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 80 | NewFileBrowser | NewFileBrowser | 0.1.3 | https://sourceforge.net/projects/locationnav/files/NewFileBrowser_v0.1.3.zip | https://sourceforge.net/projects/locationnav/ | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 81 | NppBplistPlugin | Notepad++ bplist plugin | 1.3.0.0 | https://github.com/azerg/NppBplistPlugin/releases/download/1.3.0.0/NppBplistPlugin_x86.zip | https://github.com/azerg/NppBplistPlugin | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 82 | NppPluginDemo | Notepad++ Plugin Demo | 4.2 | https://github.com/npp-plugins/plugindemo/releases/download/v4.2/pluginDemo.v4.2.bin.zip | https://npp-user-manual.org/docs/plugins/ | 首版完成 | 有，tag v4.2/真实 x64 DLL 已验证 | 中 | B |
| 83 | NppPluginTemplate | Notepad++ Plugin Template | 4.2 | https://github.com/npp-plugins/plugintemplate/releases/download/v4.2/pluginTemplate.v4.2.bin.zip | https://npp-user-manual.org/docs/plugins/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 84 | NotepadStarterPlugin | NotepadStarterPlugin | 2.3.3.0 | https://github.com/lygstate/NotepadStarter/releases/download/2.3.3.0/NotepadStarter_2.3.3.0_Win32.zip | https://github.com/lygstate/NotepadStarter/ | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 85 | nppConverter | Npp Converter | 4.4.0 | https://github.com/npp-plugins/converter/releases/download/v4.4/nppConvert.v4.4.zip | https://github.com/npp-plugins/converter/ | 首版完成 | 有，tag v4.4/真实 x64 DLL 已验证 | 高 | B |
| 86 | nppRandomStringGenerator | npp Random String Generator | 1.4.0 | https://github.com/cmbsolutions/nppRandomStringGenerator/releases/download/v1.4.0/nppRandomStringGenerator.1.4.0.x86.zip | https://github.com/cmbsolutions/nppRandomStringGenerator | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 87 | NppXmlTreeviewPlugin | Npp Xml Treeview | 2.0.0 | https://github.com/joaoasrosa/nppxmltreeview/releases/download/v2.0.0/NppXMLTreeViewPlugin_x86.zip | https://github.com/joaoasrosa/nppxmltreeview/ | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 88 | NppAutoIndent | NppAutoIndent | 1.2.0.0 | https://downloads.sourceforge.net/sourceforge/npp-plugins/NppAutoIndent_1_2_dll.zip | https://sourceforge.net/projects/npp-plugins/files/NppAutoIndent/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 89 | NppCalc | NppCalc | 1.5 | https://sourceforge.net/projects/nppcalc/files/nppcalc_1.5_bin.zip | https://sourceforge.net/projects/nppcalc/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 90 | nppcrypt | NppCrypt | 1.0.1.6 | https://github.com/jeanpaulrichter/nppcrypt/releases/download/1.0.1.6/nppcrypt_1.0.1.6_x86.zip | https://github.com/jeanpaulrichter/nppcrypt | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 91 | NppEventExec | NppEventExec | 0.9.0 | https://github.com/MIvanchev/NppEventExec/releases/download/v0.9.0/NppEventExec-plugin-x86-0.9.0.zip | https://github.com/MIvanchev/NppEventExec | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 92 | NppExec | NppExec | 0.8.2 | https://github.com/d0vgan/nppexec/releases/download/v082/NppExec_082_dll.zip | https://github.com/d0vgan/nppexec | 首版完成 | 未找到对应版本/证据不足 | 高 | U |
| 93 | NppExport | NppExport | 0.4.0.0 | https://github.com/chcg/NPP_ExportPlugin/releases/download/0.4.0/NppExport_0.4.0_Win32.zip | https://github.com/chcg/NPP_ExportPlugin | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 94 | NppFavorites | NppFavorites | 1.0.0.1 | https://github.com/heldersepu/nppfavorites/releases/download/1.0.0.1.21/NppFavorites_1.0.0.1.21_x86.zip | https://github.com/heldersepu/nppfavorites | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 95 | NppFTP | NppFTP | 0.29.10 | https://github.com/ashkulz/NppFTP/releases/download/v0.29.10/NppFTP-x86.zip | https://ashkulz.github.io/NppFTP/ | 首版完成 | 未找到对应版本/证据不足 | 高 | U |
| 96 | NppGist | NppGist | 1.5.1.35 | https://github.com/KvanTTT/NppGist/releases/download/1.5.1/NppGist-x86-1.5.1.35.zip | https://github.com/KvanTTT/NppGist | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 97 | NppGTags | NppGTags | 5.0.0 | https://github.com/pnedev/nppgtags/releases/download/v5.0.0/NppGTags_v5.0.0_x86.zip | https://github.com/pnedev/nppgtags | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 98 | NppHasher | NppHash | 1.0 | https://download.tuxfamily.org/nppplugins/NppHashMaker/NppHashMaker.v1.0.zip | https://github.com/npp-plugins/hasher | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 99 | NppJumpList | NppJumpList | 1.2.2 | https://github.com/chcg/JumpList/releases/download/1.2.2.10/NppJumpList_1.2.2.10_Win32.zip | https://sourceforge.net/projects/nppjumplist/ | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 100 | NppMenuSearch | NppMenuSearch | 0.9.6 | https://sourceforge.net/projects/nppmenusearch/files/v0.9.6/NppMenuSearch_v0.9.6_x86.zip | https://github.com/peter-frentrup/NppMenuSearch | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 101 | NppDocShare | NppNetNote | 0.1.0.0 | https://github.com/chcg/NppDocShare/releases/download/0.1.13/NppDocShare_0.1.13_Win32.zip | https://sourceforge.net/projects/npp-plugins/files/NppDocShare/ | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 102 | NppPluginOpenHost | NppPluginOpenHost | 1.1.0.0 | https://github.com/jejemorg/NppPluginOpenHost/raw/main/bin/NppPluginOpenHost.zip | https://github.com/jejemorg/NppPluginOpenHost/ | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 103 | NppQrCode32 | NppQrCode | 0.0.0.1 | https://github.com/vladk1973/NppQrCode/releases/download/v0.0.0.1/NppQrCode-0.0.0.1-x32.zip | https://github.com/vladk1973/NppQrCode | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 104 | NppRegExTractorPlugin | NppRegExTractor | 2.1.0 | https://github.com/viper3400/NppRegExTractor/releases/download/2.1.0/NppRegExTractor_2.1.0_BUILD_6_x86.zip | https://github.com/viper3400/RegExTractor/wiki/de_userdocumentation | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 105 | NppTags | NppTags | 0.9.1 | https://github.com/ffes/npptags/releases/download/v0.9.1/NppTags-091-x32.zip | https://www.fesevur.com/npptags | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 106 | NppTextViz | NppTextViz | 0.4.2 | https://github.com/KubaDee/NppTextViz/releases/download/v0.4.2/NppTextViz_x86_v0.4.2.zip | https://github.com/KubaDee/NppTextViz | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 107 | NppUISpy | NppUISpy | 1.0.4 | https://github.com/dinkumoil/NppUISpy/releases/download/v1.0.4/NppUISpy_v1.0.4_UNI.zip | https://github.com/dinkumoil/NppUISpy | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 108 | nppplugin_ofis2 | Open File In Solution | 3.0.1 | https://github.com/incrediblejr/nppplugins/releases/download/v3.0.1/nppplugin_ofis2_x86.zip | https://www.incrediblejunior.com/npp_plugins/ | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 109 | NWScript-Npp | NWScript Tools | 1.0.3.1950 | https://github.com/Leonard-The-Wise/NWScript-Npp/releases/download/v1.0.3/nwscript-npp.v1.0.3-x86.zip | https://github.com/Leonard-The-Wise/NWScript-Npp | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 110 | OpenSelection | OpenSelection | 1.1.3.0 | https://github.com/francostellari/NppPlugins/raw/main/OpenSelection/OpenSelection_dll_1v13_x32.zip | https://github.com/francostellari/NppPlugins | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 111 | Papyrus | Papyrus Script Lexer | 0.4.0.27 | https://github.com/blu3mania/npp-papyrus/releases/download/v0.4.0/PapyrusPlugin-v0.4.0-x86.zip | https://github.com/blu3mania/npp-papyrus | 首版完成 | 有，对应版本 | 低 | C |
| 112 | ccc | PHP Autocompletion | 1.4.1 | https://github.com/StanDog/npp-phpautocompletion/raw/master/RELEASES/ccc_1.4.1.zip | https://github.com/StanDog/npp-phpautocompletion | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 113 | PlantUmlViewer | PlantUML Viewer | 1.3.0.7 | https://github.com/Fruchtzwerg94/PlantUmlViewer/releases/download/1.3.0.7/PlantUmlViewer_v1.3.0.7_x86.zip | https://github.com/Fruchtzwerg94/PlantUmlViewer | 首版完成 | 有，对应版本 | 中 | C |
| 114 | PoorMansTSqlFormatterNppPlugin | Poor Man's T-Sql Formatter | 1.6.13.31502 | https://github.com/TaoK/PoorMansTSqlFormatter/releases/download/1.6.13/SqlFormatterNppPlugin.1.6.13.zip | http://architectshack.com/PoorMansTSqlFormatter.ashx | 首版完成 | 有，对应版本 | 中 | A |
| 115 | pork2sausage | Pork to Sausage | 2.2 | https://github.com/npp-plugins/pork2sausage/releases/download/v2.2/pork2sausage.2.2.bin.zip | https://github.com/npp-plugins/pork2sausage | 首版完成 | 有，对应版本 | 中 | B |
| 116 | PreviewHTML | Preview HTML | 1.3.2.0 | https://fossil.2of4.net/npp_preview/zip/PreviewHTML32.zip%3Fname%3D%26uuid%3Dv1.3.2.0-32 | https://fossil.2of4.net/npp_preview | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 117 | PyNPP | PyNPP | 1.2 | https://github.com/mpcabd/PyNPP/releases/download/v1.2/PyNPP.dll.zip | https://mpcabd.xyz/notepad-plugin-to-run-python-scripts/ | 首版完成 | 有，对应版本 | 中 | A |
| 118 | Python Indent | Python Indent | 1.0.0.4 | https://sourceforge.net/projects/kered13-notepad-plugins/files/Python%20Indent%20Win32%20v1.0.0.4.zip | https://sourceforge.net/projects/kered13-notepad-plugins/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 119 | PythonScript | PythonScript | 2.0.0.0 | https://github.com/bruderstein/PythonScript/releases/download/v2.0.0/PythonScript_Full_2.0.0.0_PluginAdmin.zip | https://github.com/bruderstein/PythonScript | 首版完成 | 有，对应版本 | 高 | C |
| 120 | NppQCP | Quick Color Picker + | 2.0 | https://s3-ap-southeast-1.amazonaws.com/nppqcp/nppqcp-2.0.zip | https://github.com/nulled666/nppqcp/ | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 121 | QuickOpenPlugin | QuickOpenPlugin | 1.1 | https://downloads.sourceforge.net/project/quickopenplugin/QuickOpenPlugin%20V1.2.zip | https://sourceforge.net/projects/quickopenplugin/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 122 | QuickText | QuickText | 0.2.5.1 | https://github.com/vinsworldcom/nppQuickText/releases/download/0.2.5.1/QuickText-v0.2.5.1-Win32.zip | https://github.com/vinsworldcom/nppQuickText | 首版完成 | 有，对应版本 | 中 | B |
| 123 | RandomValuesNppPlugin | Random Values | 0.2.1 | https://github.com/BdR76/RandomValuesNPP/releases/download/0.2.1/RandomValuesNppPlugin_x86.zip | https://github.com/BdR76/RandomValuesNPP/ | 首版完成 | 有，对应版本 | 中 | B |
| 124 | rdmd-en-x86 | RDMD for Notepad++ (English) | 0.1.0.2 | https://gitlab.com/dokutoku/rdmd-for-npp/uploads/16bb4134bb134a94042e75115ba03511/rdmd-en-x86.zip | https://gitlab.com/dokutoku/rdmd-for-npp | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 125 | rdmd-ja-x86 | RDMD for Notepad++ (Japanese) | 0.1.0.2 | https://gitlab.com/dokutoku/rdmd-for-npp/uploads/782a51c58fc5cf815e206239a22379f5/rdmd-ja-x86.zip | https://gitlab.com/dokutoku/rdmd-for-npp | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 126 | RegexTrainer | Regex Trainer | 1.0.0 | https://github.com/ahmoylaw/RegexTrainer-Descriptions/raw/master/Release/RegexTrainer.zip | https://github.com/ahmoylaw/RegexTrainer-Descriptions | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 127 | Remove Duplicate Lines | Remove Duplicate Lines | 1.3.0.0 | https://github.com/gurikbal/Remove_dup_lines/releases/download/1.3.0.2/Remove_dup_lines_x86.zip | https://github.com/gurikbal/Remove_dup_lines | 首版完成 | 有，对应版本 | 中 | A |
| 128 | RestApiToText | RestApiToText | 1.3.1.0 | https://github.com/eljefe7000/RestApiToText/raw/master/Release/v1.3.1.0/RestApiToText.zip | https://github.com/eljefe7000/RestApiToText | 首版完成 | 有，对应版本 | 中 | B |
| 129 | qkNppReverseLines | Reverse Lines | 1.0.0.0 | https://github.com/querykuma/qkNppReverseLines/releases/download/v1.0.0.0/qkNppReverseLinesPlugin_v1.0.0.0_npp7.7_x86.zip | https://github.com/querykuma/qkNppReverseLines | 首版完成 | 有，对应版本 | 中 | A |
| 130 | RunMe | RunMe | 1.4.1.0 | https://github.com/francostellari/NppPlugins/raw/main/RunMe/RunMe_dll_1v41_x32.zip | https://github.com/francostellari/NppPlugins | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 131 | NppSaveAsAdmin | Save as admin | 1.0.211 | https://github.com/Hsilgos/nppsaveasadmin/releases/download/1.0.211/NppSaveAsAdmin_1.0.211_x86.zip | https://github.com/Hsilgos/nppsaveasadmin | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 132 | SecurePad | SecurePad | 2.4 | https://github.com/DominicTobias/SecurePad/releases/download/v2.4/SecurePad_v2.4_Win32.zip | https://github.com/DominicTobias/SecurePad | 首版完成 | 有，对应版本 | 中 | A |
| 133 | selectNLaunch | Select N' Launch | 2.1 | https://github.com/npp-plugins/selectnlaunch/releases/download/v2.1/selectNLaunch.v2.1.bin.zip | https://github.com/npp-plugins/selectnlaunch | 首版完成 | 有，对应版本 | 低 | A |
| 134 | SelectToClipboard | Select to Clipboard | 1.0.3 | https://github.com/KubaDee/SelectToClipboard/releases/download/v1.0.3/SelectToClipboard_x86_v1.0.3.zip | https://github.com/KubaDee/SelectToClipboard | 首版完成 | 有，对应版本 | 低 | B |
| 135 | SelectQuotedText | SelectQuotedText | 1.0.0 | https://github.com/ffes/selectquotedtext/releases/download/v1.0.0/SelectQuotedText-100-x32.zip | https://www.fesevur.com/selectquotedtext | 首版完成 | 有，对应版本 | 中 | A |
| 136 | SessionMgr | Session Manager | 1.4.4 | https://github.com/chcg/npp-session-manager/releases/download/v1.4.4/SessionMgr_v1.4.4_x86.zip | https://mfoster.com/npp/SessionMgr.html | 首版完成 | 有，对应版本 | 中 | B |
| 137 | SherloXplorer | SherloXplorer | 0.3 | https://downloads.sourceforge.net/project/sourcecookifier/other%20plugins/SherloXplorer.v0.3.0.bin.zip | https://sourceforge.net/projects/sourcecookifier/files/other%20plugins/ | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 138 | ShtirlitzNppPlugin | Shtirlitz | 1.1.2 | https://github.com/shtirlitz-dev/notepadpp-plugin/raw/master/32bit/ShtirlitzNppPlugin.zip | https://vk.com/wall203102356_293 | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 139 | NppSnippets | Snippets | 1.7.0 | https://github.com/ffes/nppsnippets/releases/download/v1.7.0/NppSnippets-170-x32.zip | https://www.fesevur.com/nppsnippets | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 140 | nppplugin_solutionhub | Solution Hub | 3.0.1 | https://github.com/incrediblejr/nppplugins/releases/download/v3.0.1/nppplugin_solutionhub_x86.zip | https://www.incrediblejunior.com/npp_plugins/ | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 141 | nppplugin_solutionhub_ui | Solution Hub UI | 3.0.1 | https://github.com/incrediblejr/nppplugins/releases/download/v3.0.1/nppplugin_solutionhub_ui_x86.zip | https://www.incrediblejunior.com/npp_plugins/ | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 142 | nppplugin_solutiontools | Solution Tools | 3.0.1 | https://github.com/incrediblejr/nppplugins/releases/download/v3.0.1/nppplugin_solutiontools_x86.zip | https://www.incrediblejunior.com/npp_plugins/ | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 143 | SourceCookifier | Source Cookifier | 0.7.3 | https://downloads.sourceforge.net/project/sourcecookifier/0.7.3/SourceCookifier.v0.7.3.bin.zip | https://sourceforge.net/projects/sourcecookifier/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 144 | SpeechPlugin | SpeechPlugin | 0.4.0.0 | https://github.com/chcg/SpeechPlugin/releases/download/v0.4.0/SpeechPlugin_v0.4.0_Win32.zip | https://github.com/chcg/SpeechPlugin | 首版完成 | 有，对应版本 | 低 | A |
| 145 | SpellChecker | Spell-Checker | 1.3.3.0 | https://downloads.sourceforge.net/sourceforge/npp-plugins/SpellChecker_1_3_3_UNI_dll.zip | https://sourceforge.net/projects/npp-plugins/files/Spell-Checker/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 146 | SQLinFormNpp | SQLinForm | 5.3.35 | https://www.sqlinform.com/npp/SQLinFormNpp_5.3.35.zip | https://www.sqlinform.com | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 147 | SurroundSelection | SurroundSelection | 1.4.1 | https://github.com/dail8859/SurroundSelection/releases/download/v1.4.1/SurroundSelection_v1.4.1.zip | https://github.com/dail8859/SurroundSelection | 首版完成 | 有，对应版本 | 中 | B |
| 148 | TagLEET | TagLEET | 1.3.2.0 | https://sourceforge.net/projects/tagleet/files/v1.3.2/TagLEET_1.3.2.0.zip | https://sourceforge.net/projects/tagleet/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 149 | TagsView | TagsView | 1.0.3 | https://downloads.sourceforge.net/project/tagsview/TagsView%20for%20Notepad%2B%2B/TagsView_Npp_03beta.zip | https://sourceforge.net/projects/tagsview/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 150 | TakeNotes | TakeNotes | 1.2.3.0 | https://github.com/francostellari/NppPlugins/raw/main/TakeNotes/TakeNotes_dll_1v23_x32.zip | https://github.com/francostellari/NppPlugins | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 151 | NppTaskList | Task List | 2.4 | https://github.com/Megabyteceer/npp-task-list/releases/download/v2.4.0/NppTaskList_v2.4.0_Win32.zip | https://code.google.com/p/npp-task-list/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 152 | NppTextFX | TextFX Characters | 0.2.6 | https://downloads.sourceforge.net/project/npp-plugins/TextFX/TextFX%20v0.26/TextFX.v0.26.unicode.bin.zip | https://sourceforge.net/projects/npp-plugins/files/TextFX/ | 首版完成 | 未找到对应版本/证据不足 | 高 | U |
| 153 | NppTFS | TFS Work Item | 1.0 | https://downloads.sourceforge.net/project/npptfs/NppTFS.zip | https://sourceforge.net/projects/npptfs | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 154 | Tidy2 | Tidy2 | 0.2 | https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/npp-tidy2/Tidy2_0.2.zip | https://code.google.com/p/npp-tidy2/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 155 | NppToolBucket | ToolBucket | 1.10.6622.41336 | https://phdesign.com.au/assets/files/NppToolBucket-1.10.zip | https://phdesign.com.au/npptoolbucket/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 156 | TopMost | TopMost | 1.4.0.0 | https://github.com/francostellari/NppPlugins/raw/main/TopMost/TopMost_dll_1v40_x32.zip | https://github.com/francostellari/NppPlugins | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 157 | nppplugin_svn | Tortoise SVN | 3.0.1 | https://github.com/incrediblejr/nppplugins/releases/download/v3.0.1/nppplugin_svn_x86.zip | https://www.incrediblejunior.com/npp_plugins/ | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 158 | Translate | Translate | 3.1.1.0 | https://sourceforge.net/projects/npptranslate/files/bin/Translate_3.1.1.0.zip | https://sourceforge.net/projects/npptranslate/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 159 | urlPlugin | URL Encode/Decode Plugin | 1.2.0.0 | https://github.com/SinghRajenM/nppURLPlugin/releases/download/1.2.0.0/urlPlugin_x86.zip | https://github.com/SinghRajenM/nppURLPlugin | 首版完成 | 有，对应版本 | 中 | B |
| 160 | VisualStudioLineCopy | Visual Studio Line Copy | 1.0.0.2 | https://sourceforge.net/projects/notepad-visualstudiolinecopy/files/VisualStudioLineCopy%20Win32%20v1.0.0.2.zip | https://sourceforge.net/projects/notepad-visualstudiolinecopy/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 161 | WakaTime | WakaTime | 4.2.3 | https://github.com/wakatime/notepadpp-wakatime/releases/download/4.2.3/WakaTime-4.2.3-x86.zip | https://github.com/wakatime/notepadpp-wakatime | 首版完成 | 有，对应版本 | 中 | B |
| 162 | WebEdit | WebEdit | 2.1 | https://master.dl.sourceforge.net/project/npp-plugins/WebEdit/WebEdit%202.1/WebEdit.v2.1.zip | https://sourceforge.net/projects/npp-plugins/files/WebEdit/WebEdit%202.1/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 163 | WindowManager | Window Manager | 1.2.2.0 | https://downloads.sourceforge.net/sourceforge/npp-plugins/WindowManager_1_2_2_UNI_dll.zip | https://sourceforge.net/projects/npp-plugins/files/WindowManager/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 164 | WLangLexer | WLangLexer | 4.1.0.16 | https://downloads.sourceforge.net/project/wdscript/Syntax%20Highlighting/WDScript%202.5%20Notepad%2B%2B%20Syntax%20File/NPP.Plugin.WLangLexer.v4.1.0.16-wd16038f.zip | https://sourceforge.net/projects/wdscript/ | 首版完成 | 未找到对应版本/证据不足 | 低 | U |
| 165 | XBrackets | XBrackets Lite | 1.3.1 | https://github.com/d0vgan/npp-XBracketsLite/releases/download/v131/XBrackets_v131_dll.zip | https://github.com/d0vgan/npp-XBracketsLite | 首版完成 | 有，对应版本 | 中 | C |
| 166 | XMLTools | XML Tools | 3.1.1.13 | https://github.com/morbac/xmltools/releases/download/3.1.1.13/XMLTools-3.1.1.13-x86.zip | https://github.com/morbac/xmltools | 首版完成 | 有，对应版本 | 高 | B |
| 167 | XPatherizerNPP | XPatherizerNPP | 2.10 | https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/xpatherizernpp/XPatherizerNPP-2.10.zip | https://code.google.com/p/xpatherizernpp/ | 首版完成 | 未找到对应版本/证据不足 | 中 | U |
| 168 | ZenCoding-Python | Zen Coding - Python | 0.7.0.1 | https://downloads.sourceforge.net/project/npppythonscript/ZenCoding-Python/ZenCoding-Python-0.7.0.1a.zip | https://github.com/bruderstein/ZenCoding-Python | 首版完成 | 有，对应版本 | 中 | C |
| 169 | zoomdisabler | Zoom Disabler | 1.2.0 | https://github.com/StanDog/npp-zoomdisabler/raw/master/RELEASES/zoomdisabler_1.2.0.zip | https://github.com/StanDog/npp-zoomdisabler | 首版完成 | 未找到对应版本/证据不足 | 低 | U |

## 可复现验证

在仓库根目录运行：

```bash
python3 - <<'PY'
import collections
import json
from pathlib import Path

data = json.loads(Path('resources/pluginList/windows/pl.x86.json').read_text())
names = [item['folder-name'] for item in data['npp-plugins']]
duplicates = {name: count for name, count in collections.Counter(names).items() if count > 1}
print({'entries': len(names), 'unique': len(set(names)), 'duplicates': duplicates})
assert len(names) == 169
assert len(set(names)) == 169
assert not duplicates
PY
```

预期输出：

```text
{'entries': 169, 'unique': 169, 'duplicates': {}}
```
