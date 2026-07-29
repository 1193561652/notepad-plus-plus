# Notepad++ Qt 配置模块索引

## 文件

- `src/Parameters.h`
- `src/Parameters.cpp`
- `src/TinyXml/`
- `resources/config.model.xml`
- `resources/stylers.model.xml`
- `resources/langs.model.xml`
- `resources/shortcuts.xml`
- `resources/contextMenu.xml`
- `resources/toolbarIcons.xml`
- `resources/userDefineLang.xml`
- `resources/nativeLang/chineseSimplified.xml`

## 主要类

- `NppParameters`：全局配置单例。
- `NativeLangSpeaker`：本地化文本读取和应用。

## 主要数据结构

- `NppGUI`
- `ScintillaViewParams`
- `Session`
- `sessionFileInfo`
- `LangDesc`
- `LexerStyler`
- `WordsStyle`
- `MacroDef`
- `MacroAction`

## 主要方法

- `NppParameters::load()`
- `NppParameters::loadConfig()`
- `NppParameters::loadFindHistory()`
- `NppParameters::loadSession()`
- `NppParameters::loadLangs()`
- `NppParameters::loadStylers()`
- `NppParameters::loadShortcuts()`
- `NppParameters::writeNppGUI()`
- `NppParameters::writeFindHistory()`
- `NppParameters::writeSession()`
- `NppParameters::writeShortcuts()`

## 缓存结论

配置系统已经具备 Notepad++ 风格 XML 文件入口，但完全读写兼容仍需逐节点核验。后续修改配置前必须先对照原版 `Parameters`。

阶段一补充：默认 XML 模型已进入 qrc；首次缺失时会被复制到用户配置目录。`loadLangs()` 和 `loadShortcuts()` 已具备资源回退。配置目录初始化会创建 `backup/plugins/themes/autoCompletion/localization/nativeLang/userDefineLangs/toolbarIcons` 等兼容目录。`writeConfigXml()` 和 `writeShortcuts()` 已改为尽量保留当前未支持的原版 XML 结构。

阶段五补充：`FindHistory` 已纳入 `config.xml` 读写；新建文档默认 EOL/编码和快照备份配置已从“可解析”进入“实际影响行为”的状态。

## 本地化索引

- `NativeLangSpeaker::init()` 读取 `<Menu><Main>` 下全部同名 `Entries` 和 `Commands` 分组，语言文件可以按顶级菜单、子菜单或功能域拆分。
- Qt 菜单和动作通过稳定 `objectName` 对应 XML 的 `menuId` / `objectName`，可本地化的静态项不得省略该标识。
- `MainWindow::applyNativeLang()` 必须在菜单、工具栏和全部停靠面板创建后首次调用，否则英文原文缓存和面板标题翻译不完整。
- 运行时重建的最近文件菜单由 `updateRecentFilesMenu()` 单独读取 `clearRecentFilesAction`，确保中英文热切换正确。
- 简体中文资源现覆盖主窗口静态菜单、动作、停靠面板和标准工具栏；用户命令、最近文件路径、UDL 名称等用户数据不翻译。
- 偏好设置通过 `changeDlgLang("Preferences")` 翻译 19 个页面；索引项使用 `objectName_index`，数值后缀使用 `objectName_suffix`。
- 偏好设置实例关闭后销毁，下次打开时按当前配置语言重新构建，避免语言热切换后的旧文本残留。
