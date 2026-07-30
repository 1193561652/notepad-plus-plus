# 非插件差异 1–6 收敛记录

日期：2026-07-30

## 范围

本轮针对与 Notepad++ v8.4.6 比较后确认的六组差异实施收敛：

1. 内置语言菜单与 Scintilla/Lexilla lexer 映射；
2. 非插件命令入口及原版命令 ID；
3. `stylers.xml` Style Configurator；
4. 搜索标记、Tab/View 与高级编辑操作；
5. `autoCompletion/*.xml` API 和函数签名；
6. UDL 设计器与 Finder。

## 实现

- `ScintillaEditView` 使用 v8.4.6 `_langNameInfoArray` 对应关系覆盖语言菜单
  中全部内置语言。QScintilla 没有专用包装类时使用通用 `QsciLexer` 包装，
  仍由打包的 Lexilla lexer 执行着色，不回退到 Qt 正则表达式。
- 命令注册表加入拆分/合并行、缩进、路径复制、特殊粘贴、匹配括号选择、
  五组样式标记、查找结果导航、隐藏行、同步滚动、Tab 1–9、Tab 移动和颜色等
  原版命令 ID。View 同时补充无干扰模式、文档摘要、文件监视和按层折叠。
- Style Configurator 可编辑全局及每种 lexer 的前景色、背景色、字体、字号、
  粗体、斜体和下划线。写回只更新已有 `LexerType/WordsStyle` 和
  `GlobalStyles/WidgetStyle` 的受管属性，未知 XML 内容不删除。
- 自动完成优先读取用户配置目录的 `autoCompletion/<language>.xml`，再读取
  程序目录；支持普通关键字、函数、重载和参数签名，并保留 lexer 内置关键字
  及当前文档补全。
- UDL 菜单在没有已有语言时仍提供设计器。设计器支持新建、重命名、删除、
  导入、单语言导出以及原有的设置、28 组关键字和样式编辑。
- Finder 改为只读 `ScintillaEditView`，使用 `searchResult/errorlist` lexer，
  具有搜索命中指示器、按来源折叠和双击导航，不再使用 `QListWidget` 模拟。

## 兼容边界

- 保持现有业务调用关系，没有替换既有字符串转换等内部函数调用。
- `stylers.xml`、`userDefineLang.xml` 的结构不升级；导入和导出使用原版
  `NotepadPlus/UserLang` 结构。
- 新代码仅使用标准 C++、Qt 和现有 QScintilla 接口，没有新增 Windows/Linux
  分支或 Windows 专用行为。

## 验证

- `AutoCompletionParserTests`：关键字和函数参数签名。
- `ParametersStyleUdlTests`：样式未知属性保留以及 UDL 新建、重命名、导出、
  删除、导入闭环。
- `LargeFileModeTests`：全部菜单内置语言都有 lexer 名称映射。
- `NppCommandRegistryTests`：新增命令的唯一性、入口存在性和关键原版 ID。
- 完整构建和 `ctest --output-on-failure` 在提交前执行。
