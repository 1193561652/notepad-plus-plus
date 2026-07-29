# Notepad++ Qt 移植版功能实现与原版差异报告

报告日期：2026-07-22

## 1. 报告范围

本报告对照以下代码：

- 原版：`v8.4.6:`，版本 8.4.6。
- 移植版：`./`，Qt5、C++14、QScintilla。
- Qt 参考项目：`sample/notepadppqt2/` 仅作为只读参考，不计入移植版完成度。

报告依据当前代码、阶段 0 至阶段 7 变更记录、代码级 UI 审计和本地构建结果。状态定义：

| 状态 | 含义 |
| --- | --- |
| 已实现 | 已形成日常可用闭环，仍可能存在少量原版细节差异 |
| 基础实现 | 核心路径可用，但原版高级选项、边界或配置联动不完整 |
| 部分实现 | 仅覆盖功能子集，不能视为原版等价实现 |
| 占位 | UI 或接口存在，但动作被禁用或没有实际业务逻辑 |
| 缺失 | 当前主线未发现对应实现 |

## 2. 总体结论

Qt 移植版已经不是空壳，已具备可启动、可编辑、可打开保存、可管理多标签、可恢复基础会话、可搜索、可打印的编辑器主线。配置、界面、搜索、停靠面板、宏、UDL 和插件边界均已有代码入口。

当前最适合的定位是：**核心文本编辑流程可用，Notepad++ 8.4.6 的高阶功能和精细兼容仍未完成**。

主要成熟区域：

- 文件新建、打开、保存、另存为、保存全部、关闭和重新加载。
- 基础文本编辑、常用行操作、EOL 与编码菜单、多标签和双视图基础行为。
- 当前文档查找替换、Find All、所有打开文档查找、Find in Files 基础搜索。
- 配置目录和主要 XML 资源初始化、基础配置读写、会话和查找历史。
- 工具栏、状态栏、主要菜单、文档地图、函数列表、文件浏览器和基础深色模式。
- 打印、宏录制回放、列编辑、常用排序和轻量 UDL。

主要未完成区域：

- 原版全部菜单命令、完整偏好设置页面和配置驱动 UI。
- 原版完整编码检测、大文件处理、文件删除/重命名和复杂外部变更场景。
- Replace in Files、Replace All in Opened Documents、Find in Projects 和 Boost.Regex 兼容。
- 完整 UDL 编辑器、完整宏 XML/命令映射、完整打印偏好。
- 原版插件 ABI、`NPPM_*` 消息、插件 docking 和 Plugin Admin。

## 3. 文件、Buffer、标签与会话

| 功能点 | 移植版状态 | 移植版实现 | 与原版差异 |
| --- | --- | --- | --- |
| 新建文件 | 已实现 | 创建 Buffer 和编辑视图，并应用默认 EOL、编码 | 默认语言、全部新文档选项和原版状态联动不完整 |
| 打开文件 | 基础实现 | 原始字节读取，检测 BOM、UTF-8、locale fallback 和 EOL | 原版还有 cookie、uchardet、代码页策略、大文件和更完整错误恢复 |
| 保存/另存为 | 基础实现 | 按 Buffer 编码写出，处理 BOM，校验写入长度 | 原版编码模式、备份、只读文件、权限及冲突处理更完整 |
| 保存全部 | 基础实现 | 遍历主/副视图文档并保存 | 原版的失败汇总、只读和复杂交互更完整 |
| 重新加载 | 基础实现 | 重新读取字节并重新检测 BOM、编码、EOL | 原版有更完整的延迟 reload、状态保持和外部变更策略 |
| 关闭/关闭全部 | 基础实现 | 主副视图 Buffer 去重，处理 clone 和保存检查 | Close All but Current、左侧、右侧、未修改文档仍是占位 |
| 最近文件 | 已实现 | `config.xml` History 读取、菜单刷新、清空 | 原版的路径检查、恢复最后关闭文档和更多历史策略未完整实现 |
| 多标签 | 基础实现 | `DocTabView` 管理 Buffer 和视图切换 | 标签右键菜单、颜色、锁定、批量排序和完整拖放行为不足 |
| 双视图 | 基础实现 | 主/副视图显示、旋转、移动、clone | 原版基于 Scintilla Document 引用计数；Qt 版所有权模型更轻量 |
| 会话保存恢复 | 基础实现 | 主副视图、活动文档、选择、滚动、编码和 clone 基础恢复 | 折叠、书签、只读、标签颜色、Document Map 等字段未完整恢复 |
| 快照备份 | 基础实现 | 按配置定时备份，主副视图 Buffer 去重 | 原版备份命名、恢复、清理和异常退出边界更完整 |
| 外部文件变更 | 基础实现 | `QFileSystemWatcher` 检测修改并提示 reload | 删除、重命名、权限变化、批量事件和平台差异仍需增强 |
| 文件重命名/删除 | 占位 | 菜单入口存在并禁用 | 原版有完整 rename、delete/recycle bin 行为 |
| 打印 | 基础实现 | `QsciPrinter`、打印对话框、直接打印 | 原版页眉页脚、颜色、行号、缩放和打印偏好未对齐 |

## 4. 编辑器与文本处理

| 功能点 | 移植版状态 | 移植版实现 | 与原版差异 |
| --- | --- | --- | --- |
| 基础编辑 | 已实现 | QScintilla 提供输入、选择、撤销重做、剪切复制粘贴 | 底层由 QScintilla/Qt 事件替代原版 Win32 Scintilla 消息链 |
| 行删除/复制/移动 | 已实现 | Scintilla 命令直接执行 | 原版还有更多行连接、拆分、上下移空行等命令 |
| 大小写转换 | 部分实现 | UPPERCASE、lowercase | Proper Case、Sentence case、Invert Case 等仍是占位 |
| 行注释 | 基础实现 | 读取 `langs.xml` 注释符并切换行注释 | Block、Stream、Block Uncomment 仍是占位；语言边界需继续核验 |
| 书签 | 基础实现 | 切换、上一个、下一个、清空 | 原版标记样式、复制/剪切/删除标记行功能更多 |
| EOL | 基础实现 | 检测并显示 CRLF/LF/CR，可执行转换 | 混合 EOL、状态传播和异常文件行为未完全对齐 |
| 编码菜单 | 基础实现 | UTF 系列及多地区字符集菜单，支持 Encode in/Convert to 基础路径 | 原版代码页映射和编码检测更完整，部分转换语义仍需专项验证 |
| 标准语法高亮 | 基础实现 | 扩展名映射到 QScintilla lexer，读取 `langs.xml`/`stylers.xml` | QScintilla lexer 与原版 Lexilla 版本、属性和 style ID 不完全相同 |
| UDL | 基础实现 | 只读解析 `userDefineLang.xml`，支持扩展名、关键字、注释、数字、操作符和基础样式 | 无完整 UDL 设计器；折叠、嵌套、跨区间多行状态和全部版本语义未对齐 |
| 自动完成 | 部分实现 | 支持配置开关、触发字符数和当前文档词补全基础 | 原版函数参数提示、语言 API 文件、排序和忽略规则更完整；菜单入口仍占位 |
| Smart Highlight | 基础实现 | 使用 indicator 高亮选择文本匹配 | 原版对大文档、语言、选区、样式和刷新条件控制更细 |
| 列模式 | 基础实现 | QScintilla 矩形选择能力，提供操作提示 | 原版列模式交互细节未全面验证 |
| 列编辑器 | 基础实现 | 对行范围同列插入文本或递增数字，一次撤销 | 原版支持前导零、重复、不同进制和更丰富格式 |
| 排序 | 部分实现 | 字典升降序、忽略大小写升降序、反转行序 | 数值、小数、随机、矩形选区列范围排序未实现 |
| 空白处理 | 部分实现 | 删除空行/空白行、首尾修剪、TAB 转空格 | 单独前导/尾随修剪、Space to TAB 等入口仍占位 |
| 宏 | 基础实现 | `QsciMacro` 录制、停止、回放和 `.macro` 导入导出 | 未完整记录 Notepad++ 菜单命令，未完全映射原版 `shortcuts.xml` Macro Action |

## 5. 查找、替换与结果面板

| 功能点 | 移植版状态 | 移植版实现 | 与原版差异 |
| --- | --- | --- | --- |
| Find Next/Previous | 已实现 | 当前文档前后查找、选项和历史 | 环绕、零长度匹配和全部正则边界仍需验证 |
| 当前文档 Replace/Replace All | 基础实现 | 查找替换对话框执行当前视图操作 | 选区、反向、零长度正则和原版提示语义未完全对齐 |
| Count | 基础实现 | 当前文档匹配计数 | 原版复杂搜索模式与状态联动更完整 |
| Mark | 基础实现 | indicator 标记当前文档匹配 | Jump Up/Down、Clear All Marks 及标记行处理入口仍占位 |
| Find All 当前文档 | 基础实现 | 输出行号、匹配位置、文本片段到 Find Result | 原版是可折叠、分组、带样式 segment 的 Finder 文档 |
| Find All 打开文档 | 基础实现 | 主副视图按 Buffer 去重，支持未保存文档和结果跳转 | 原版结果分组、统计、取消和文档生命周期联动更完整 |
| Find in Files | 基础实现 | 目录、过滤器、递归、隐藏文件和结果跳转 | 缺少取消、进度、二进制跳过、大文件分块和完整编码检测 |
| Replace All 打开文档 | 缺失 | 无实际替换逻辑 | 原版支持全部打开文档替换 |
| Replace in Files | 缺失 | 无实际替换逻辑 | 原版支持备份、过滤、替换统计和文件写回 |
| Find in Projects | 占位 | 项目面板和搜索入口未形成可用闭环 | 原版支持三个 Project Panel 的限定搜索 |
| 搜索历史 | 基础实现 | `FindHistory` XML 读取写回，恢复 Find/Replace/Filter/Path 和选项 | 历史上限、全部 UI 状态和原版细节仍需逐项验证 |
| 正则表达式 | 部分实现 | 当前视图依赖 QScintilla，文件搜索使用 `QRegularExpression` | 原版使用 Boost.Regex；语法、转义、替换表达式和边界不保证兼容 |

## 6. UI、视图与偏好设置

| 功能点 | 移植版状态 | 移植版实现 | 与原版差异 |
| --- | --- | --- | --- |
| 顶层菜单 | 基础实现 | File/Edit/Search/View/Encoding/Language/Settings/Tools/Macro/Run/Plugins/Window/? 顺序已对齐 | 大量低频命令仅显示为禁用占位 |
| 工具栏 | 基础实现 | 常用文件、编辑、搜索、视图、宏和面板动作 | 原版 `toolbarIcons.xml` 自定义图标及多套图标策略未完整接入 |
| 状态栏 | 基础实现 | 文档类型、长度/行数、光标、EOL、编码、INS/OVR | 文案、宽度、刷新时机和点击交互未全面对齐 |
| 标签栏 | 基础实现 | 文档标题、脏标记、切换、关闭、主副视图 | 原版标签配置、上下文菜单、颜色和拖放细节更多 |
| 文件浏览器 | 基础实现 | 文件系统树和双击打开 | 原版 Folder as Workspace 的历史、过滤和多根目录行为更完整 |
| 文档地图 | 基础实现 | 停靠面板与当前编辑视图同步 | 原版快照渲染、视口框和会话字段更完整 |
| 函数列表 | 部分实现 | 停靠面板和基础解析/跳转入口 | 原版 parser XML、语言覆盖和规则系统更完整 |
| 项目面板 | 占位 | View 菜单有禁用入口 | 原版有三个独立 Project Panel |
| Document List | 占位 | View 菜单有禁用入口 | 原版支持垂直文档列表、排序和激活 |
| 深色模式 | 基础实现 | 偏好开关、Qt palette 和基础 stylesheet | 原版有完整 DarkMode 色彩、图标、原生控件和系统主题联动 |
| 本地化 | 部分实现 | `NativeLangSpeaker` 加载资源/用户语言并应用菜单 | 原版 XML 覆盖面更广；Qt 动态控件和全部对话框翻译未完整核验 |
| 偏好设置 | 部分实现 | General、Editing、Dark Mode、Margins、New Document、Recent Files、Language、Backup、Auto-Completion、Misc 基础页面 | Default Directory、File Association、Highlighting、Print、Searching、Multi-Instance、Delimiter、Cloud/Link、Search Engine 仍为 stub |
| UI 一致性验证 | 部分完成 | 已完成代码级菜单和结构审计 | 尚未完成原版与移植版逐窗口截图、尺寸、字体和交互回归测试 |

## 7. 配置和兼容资源

| 配置项 | 移植版状态 | 当前行为 | 与原版差异/风险 |
| --- | --- | --- | --- |
| 配置目录结构 | 已实现 | 创建 backup、plugins、themes、autoCompletion、localization、nativeLang、userDefineLangs、toolbarIcons | 平台路径和便携模式边界仍需多平台验证 |
| `config.xml` | 基础实现 | 读取 GUI、视图、历史、备份、自动完成等子集；保守更新支持节点 | 尚未逐节点证明完全读写兼容；Qt 扩展字段需要持续隔离 |
| `session.xml` | 基础实现 | 读写主副视图、文件和部分位置/备份字段 | 原版全部 Session 字段未完全应用 |
| `langs.xml` | 基础实现 | 读取扩展名、注释和关键字 | 原版全部语言属性与 lexer 配置未完整消费 |
| `stylers.xml` | 基础实现 | 读取 lexer/global style 并应用主要字体颜色 | 主题切换、全部 style 和透明/系统色行为未完整对齐 |
| `shortcuts.xml` | 部分实现 | 读取/保守写回 Macros，保留其他节点 | 主菜单快捷键、Scintilla key map 和宏命令尚未完整绑定 |
| `contextMenu.xml` | 资源基础 | 默认文件可复制和保留 | 编辑器右键菜单尚未完整由该文件驱动 |
| `toolbarIcons.xml` | 资源基础 | 默认文件可复制和保留 | 工具栏仍主要使用 qrc 固定图标 |
| `userDefineLang.xml` | 基础实现 | 只读加载并应用轻量 UDL，不写回、不升级 | 完整 UDL 编辑器和全部 XML 语义未实现 |
| localization 目录 | 部分实现 | 保留目录并支持基础 native language 加载 | 原版全部语言文件发现和切换规则未完整覆盖 |

完全兼容约束下最需要注意的是：**“文件能够被读取”不等于“文件已完全兼容”**。当前实现对支持节点采取局部更新和保留策略，但仍需使用包含未知节点、插件节点、自定义主题和旧版本字段的真实配置样本做逐文件往返测试。

## 8. 插件体系

| 功能点 | 移植版状态 | 移植版实现 | 与原版差异 |
| --- | --- | --- | --- |
| 插件构建开关 | 已实现 | `ENABLE_PLUGIN_SYSTEM` 默认关闭，可显式启用 | 符合插件不是近期目标的约束 |
| 动态库发现 | 基础实现 | Windows `.dll`、Linux `.so`、macOS `.dylib`；扫描根目录和一级插件目录 | 原版有更复杂的插件目录、禁用列表、兼容性和安全检查 |
| Qt 插件接口 | 接口预留 | `IPlugin`、`IPluginHost`、create/destroy C 导出和 QAction 菜单 | 这是新的 Qt 接口，不是原版 ABI |
| 生命周期 | 基础实现 | init、cleanup、destroy、卸载动态库，路径去重 | 异常隔离、版本协商和崩溃插件处理不足 |
| 原版 `NPPM_*` | 缺失 | 无消息兼容层 | 原版插件无法依赖现有 ABI 直接运行 |
| 插件通知/快捷键 | 缺失 | 未实现原版通知和 FuncItem/ShortcutKey 模型 | 插件事件覆盖明显不足 |
| 插件 docking | 缺失 | 未实现原版 docking ABI | Qt 插件只能自行使用有限宿主接口 |
| Plugin Admin | 缺失 | 无管理、下载、更新和兼容列表 | 按当前计划后置 |

因此，当前只能表述为“保留跨平台插件扩展接口”，不能表述为“兼容 Notepad++ 插件”。

## 9. 关键逻辑差异

| 领域 | 原版逻辑 | Qt 移植版逻辑 | 影响 |
| --- | --- | --- | --- |
| 应用入口 | Win32 `WinMain`、窗口消息循环 | `QApplication` 和 Qt event loop | 行为应映射到 Qt 事件，不应机械翻译 Win32 API |
| 命令分发 | 命令 ID、`NppBigSwitch`、窗口消息 | `QAction`、signal/slot、lambda/slot | 尚缺统一完整的原版命令 ID 到 Qt 动作映射表 |
| 文档模型 | Buffer 与 Scintilla Document 分离并引用计数 | Buffer 与 `ScintillaEditView` 关联更紧 | clone、关闭、撤销栈和生命周期边界需要持续验证 |
| 文件 I/O | 自有编码读写器、uchardet、代码页模型 | Qt 字节读取、`QTextCodec`、QString | 非 UTF 编码、字节位置和无损往返风险更高 |
| XML | 原版参数系统消费大量节点 | `NppParameters` 消费支持子集并保守写回 | 完全兼容尚需逐节点和真实样本往返证明 |
| 搜索 | Finder 文档、Boost.Regex、后台/多范围逻辑 | QScintilla 搜索、`QRegularExpression`、QList 结果 | 正则语法、结果层次、性能和取消机制不同 |
| UI | Win32 resource、原生控件、DockingManager | Qt Widgets、布局、`QDockWidget` | 结构可接近，但像素、主题和交互不会天然一致 |
| 插件 | 固定 ABI、窗口句柄、`NPPM_*` 消息 | Qt C++ 接口和 QObject/QAction | 二进制和行为均不兼容，必须单独设计适配层 |

## 10. 当前占位和缺失功能摘要

仍为明确占位的主要入口包括：

- 文件：Open Containing Folder、Default Viewer、Folder as Workspace、Save Copy As、Rename、批量关闭子项、Recycle Bin、手动 Load/Save Session。
- 编辑：日期时间插入、块/流注释、更多大小写转换、部分空白转换、显式自动完成菜单。
- 搜索：Matching Brace、Mark 跳转/清空、Find in Projects。
- 视图：Always on Top、Full Screen、Post-It、Document List、Project Panels、Clipboard History、Character Panel、折叠菜单。
- 设置和工具：Style Configurator、Shortcut Mapper、Context Menu 编辑、主题/插件导入、MD5/SHA 工具。
- Run、Window 扩展命令和部分 Help 链接。

完全缺失或未形成业务闭环的主要功能包括：

- Replace in Files、Replace All in Opened Documents。
- 原版项目面板、Document List 和 Clipboard History。
- 完整 UDL 设计器和完整 Shortcut Mapper。
- 原版插件 ABI、Plugin Admin、插件通知和 docking。
- 原版全部打印设置、搜索设置及多个偏好设置页面。

## 11. 验证状态与报告限制

- 当前主线已通过 CMake 配置和 Debug 构建：`[100%] Built target notepadpp-qt`。
- Windows GUI 已完成隐藏窗口启动冒烟测试，程序可稳定进入事件循环。
- 阶段 4 完成的是代码级 UI 结构审计，不是逐像素截图对比。
- 当前没有覆盖全部功能的自动化回归测试，因此“已实现/基础实现”表示代码路径存在并形成基础闭环，不代表已经证明与原版完全等价。

## 12. 后续建议

1. 为 `config.xml`、`session.xml`、`shortcuts.xml`、UDL 和主题建立真实样本往返测试，优先证明不丢节点、不改结构。
2. 建立原版命令 ID、菜单动作、快捷键和 Qt slot 的统一映射表，逐步消除 disabled 占位。
3. 补齐 Replace in Files、Replace All in Opened Documents 和 Find in Projects，再做搜索体系专项回归。
4. 对文件编码、外部文件变更、大文件和 clone Buffer 生命周期建立自动化测试。
5. 单独开展 UI 截图和交互一致性阶段，覆盖常见 DPI、字体、浅色/深色和主副视图布局。
6. 插件继续维持接口边界；在 ABI 方案明确前不实现 Plugin Admin。
