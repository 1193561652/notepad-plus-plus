# 阶段一：配置 XML 与资源兼容基础

执行日期：2026-07-22

## 本次范围

本次执行阶段一的第一步：配置 XML 与资源兼容基础。重点是建立默认 XML 资源来源、配置目录骨架和节点级索引。业务功能未展开实现。

## 原版默认 XML 文件

原版 `v8.4.6:PowerEditor/src/` 中与配置基础相关的默认 XML：

| 文件 | 用途 | Qt 版本次处理 |
| --- | --- | --- |
| `config.4zipPackage.xml` | 默认 `config.xml` 模板 | 已复制为 `resources/config.model.xml` 并加入 qrc |
| `langs.model.xml` | 语言、扩展名、注释符、关键字 | 已复制到 `./resources/` 并加入 qrc |
| `stylers.model.xml` | lexer 样式和全局样式 | Qt 版已有资源 |
| `shortcuts.xml` | 内部命令、宏、用户命令、插件命令、Scintilla keys | 已复制到 `resources/` 并加入 qrc |
| `contextMenu.xml` | Scintilla 右键菜单结构 | 已复制到 `resources/` 并加入 qrc |
| `toolbarIcons.xml` | 工具栏图标自定义配置 | 已复制到 `resources/` 并加入 qrc |
| `userDefineLang.xml` | 默认用户自定义语言 | 已复制到 `resources/` 并加入 qrc |

## 关键节点索引

### config.xml

原版默认模板关键结构：

- `NotepadPlus/GUIConfigs`
- `GUIConfig name="ToolBar"`
- `GUIConfig name="StatusBar"`
- `GUIConfig name="TabBar"`
- `GUIConfig name="ScintillaViewsSplitter"`
- `GUIConfig name="UserDefineDlg"`
- `GUIConfig name="TabSetting"`
- `GUIConfig name="AppPosition"`
- `GUIConfig name="Auto-detection"`
- `GUIConfig name="CheckHistoryFiles"`
- `GUIConfig name="RememberLastSession"`
- `GUIConfig name="DetectEncoding"`
- `GUIConfig name="NewDocDefaultSettings"`
- `GUIConfig name="Print"`
- `GUIConfig name="Backup"`
- `GUIConfig name="auto-completion"`
- `GUIConfig name="MenuBar"`
- `GUIConfig name="ScintillaPrimaryView"`
- `GUIConfig name="DockingManager"`
- `NotepadPlus/History`

Qt 版当前已读取/写入其中一部分。完全读写兼容仍需要解决未知节点和未知属性保留问题。

### session.xml

Qt 版当前结构与原版方向一致：

- `NotepadPlus/Session`
- `Session@activeView`
- `mainView@activeIndex`
- `subView@activeIndex`
- `File@filename`
- `File@lang`
- `File@encoding`
- `File@userReadOnly`
- `File@backupFilePath`
- `File@originalFileLastModifTimestamp`
- `File@originalFileLastModifTimestampHigh`
- `File@tabColourId`
- `File@firstVisibleLine`
- `File@startPos`
- `File@endPos`
- `File@xOffset`
- `File@scrollWidth`
- `File@selMode`
- `File@offset`
- `File@wrapCount`
- `Mark@line`
- `Fold@line`
- `FileBrowser`

### langs.xml / langs.model.xml

关键结构：

- `NotepadPlus/Languages`
- `Language@name`
- `Language@ext`
- `Language@commentLine`
- `Language@commentStart`
- `Language@commentEnd`
- `Keywords@name`

Qt 版 `NppParameters::loadLangs()` 已读取语言名、扩展名、注释符和 `instre1/instre2/type1/type2` 关键字。

### stylers.xml / stylers.model.xml

关键结构：

- `NotepadPlus/LexerStyles`
- `LexerType@name`
- `LexerType@desc`
- `WordsStyle@styleID`
- `WordsStyle@name`
- `WordsStyle@fgColor`
- `WordsStyle@bgColor`
- `WordsStyle@fontName`
- `WordsStyle@fontStyle`
- `WordsStyle@fontSize`
- `NotepadPlus/GlobalStyles`
- `WidgetStyle@styleID`

Qt 版 `NppParameters::loadStylers()` 已读取 lexer 样式和全局样式。

### shortcuts.xml

关键结构：

- `NotepadPlus/InternalCommands`
- `NotepadPlus/Macros`
- `Macro@name`
- `Macro@Ctrl`
- `Macro@Alt`
- `Macro@Shift`
- `Macro@Key`
- `Action@type`
- `Action@message`
- `Action@wParam`
- `Action@lParam`
- `Action@sParam`
- `NotepadPlus/UserDefinedCommands`
- `NotepadPlus/PluginCommands`
- `NotepadPlus/ScintillaKeys`

Qt 版当前主要读取 `Macros`。`InternalCommands`、`UserDefinedCommands`、`PluginCommands`、`ScintillaKeys` 仍需后续接入或至少保持写回。

### contextMenu.xml

关键结构：

- `NotepadPlus/ScintillaContextMenu`
- `Item@MenuEntryName`
- `Item@MenuItemName`
- `Item@id`
- `Item@FolderName`
- `Item@TranslateID`
- `Item@PluginEntryName`
- `Item@PluginCommandItemName`

Qt 版当前尚未完整接入右键菜单配置，但默认文件已内嵌为后续读取来源。

### toolbarIcons.xml

关键结构：

- `NotepadPlus/ToolBarIcons`
- `ToolBarIcons@icoFolderName`

Qt 版当前工具栏使用 qrc 固定图标，后续需要设计 Qt 图标映射策略。

### userDefineLang.xml

关键结构：

- `NotepadPlus/UserLang`
- `UserLang@name`
- `UserLang@ext`
- `Settings`
- `KeywordLists`
- `Styles`
- `WordsStyle`

Qt 版当前尚未实现 UDL，本文件仅作为默认资源和后续兼容基础。

## 本次代码改动

- `./resources/resources.qrc`
  - 新增 `config.model.xml`
  - 新增 `langs.model.xml`
  - 新增 `shortcuts.xml`
  - 新增 `contextMenu.xml`
  - 新增 `toolbarIcons.xml`
  - 新增 `userDefineLang.xml`

- `./src/Parameters.cpp`
  - `ensureConfigDir()` 现在同时创建配置子目录：
    - `backup`
    - `plugins`
    - `themes`
    - `autoCompletion`
    - `localization`
    - `nativeLang`
    - `userDefineLangs`
    - `toolbarIcons`
  - `loadLangs()` 在用户目录和 exe 目录都不存在语言文件时，回退到 `:/langs.model.xml`。
  - `loadShortcuts()` 在用户目录和 exe 目录都不存在快捷键文件时，回退到 `:/shortcuts.xml`。
  - 新增默认 XML 补齐逻辑：用户配置目录缺少文件时，从 Qt 资源复制默认 XML。
  - `writeConfigXml()` 改为基于现有 `config.xml` 更新已支持的 `GUIConfig` 和 `History`，尽量保留未知节点与未知属性。
  - `writeShortcuts()` 改为只替换 `Macros` 节点，保留 `InternalCommands`、`UserDefinedCommands`、`PluginCommands`、`ScintillaKeys` 等原版结构。

## 验证

已执行：

```bash
cmake --build .
```

工作目录：

```text
build
```

结果：

- 构建成功。
- qrc 自动重新生成。
- `Parameters.cpp` 重新编译。
- `notepadpp-qt.exe` 链接成功。

备注：直接执行 `mingw32-make` 失败，因为当前 shell PATH 中没有该命令；`cmake --build .` 成功使用现有构建配置完成构建。

## 后续建议

阶段一基础范围已完成。后续进入具体功能时仍需继续处理：

1. 为 `contextMenu.xml`、`toolbarIcons.xml`、`userDefineLang.xml` 增加正式解析入口。
2. 将 `shortcuts.xml` 的 `InternalCommands`、`UserDefinedCommands`、`PluginCommands`、`ScintillaKeys` 逐步绑定到命令系统。
3. 针对 `config.xml` 做更严格的节点级读写往返测试。
4. 在会话、菜单、右键菜单、工具栏和 UDL 功能阶段继续扩展对应 XML 的业务使用。
