# 阶段四：原版 UI 与 Qt 移植版 UI 一致性检验

执行日期：2026-07-22

## 检验方式

- 静态对照原版 `v8.4.6:PowerEditor/src/Notepad_plus.rc` 中 `IDR_M30_MENU` 菜单资源。
- 静态对照 Qt 版 `./src/MainWindow.cpp` 中 `createMenus()`、`createToolBars()`、`createStatusBar()` 和 dock 初始化。
- 未启动 GUI 做视觉截图检验；本阶段完成的是代码级 UI 结构一致性审计。
- 审计过程中顺手修正了顶层菜单顺序问题，并移除了非原版的顶层 `Format` 菜单。

## 顶层菜单一致性

| 原版顺序 | Qt 版当前状态 | 一致性 | 说明 |
| --- | --- | --- | --- |
| File | File | 高 | 主要入口已出现，复杂项禁用占位 |
| Edit | Edit | 中高 | 常用入口与主要分组已出现，部分高级子菜单缺失 |
| Search | Search | 中 | 常用入口已出现，Style/Jump/Copy Styled 等高级组缺失 |
| View | View | 中 | 主要视图入口已出现，Tab/Fold Level/浏览器预览等仍缺 |
| Encoding | Encoding | 中高 | 结构存在，字符集覆盖仍不完整 |
| Language | Language | 中 | 静态常用语言分组，未完全按原版动态/完整列表 |
| Settings | Settings | 中 | Preferences 可用，其余入口禁用占位 |
| Tools | Tools | 低中 | MD5/SHA-256 骨架已补齐，逻辑禁用 |
| Macro | Macro | 中 | 入口存在，完整宏行为后续验证 |
| Run | Run | 低中 | Run... 骨架已补齐，逻辑禁用 |
| Plugins | Plugins | 中 | 插件边界存在，无插件时显示禁用占位 |
| Window | Window | 低中 | Window/Sort/Copy 入口占位 |
| ? | ? | 中 | About 可用，其余帮助入口占位 |

结论：顶层菜单顺序当前已与原版一致。Qt 版仍保留大量 disabled 占位，适合作为后续功能补齐清单。

## 菜单内部分组一致性

| 菜单 | 已覆盖 | 仍缺失或不足 |
| --- | --- | --- |
| File | New/Open/Reload/Save/Save As/Save All/Close/Close All/Open Containing/Close Multiple/Session/Print | Open Recent 行为需继续对齐；Print、Rename、Save Copy、删除文件、Session 文件导入导出为占位 |
| Edit | Undo/Redo/Cut/Copy/Paste/Delete/Select All/Insert/Line Operations/Comment/Case/Blank/Auto-Completion/EOL | Copy to Clipboard、Indent、Paste Special、On Selection、Column Mode、Read-only、完整排序/去重仍缺 |
| Search | Find/Replace/Go To/Bookmark/Mark/Find in Files 占位 | Search Results Window、Style groups、Jump groups、Copy Styled Text、Find characters in range 等缺 |
| View | Always on Top/Full Screen/Post-It/Show Symbol/Zoom/Move Clone/Word Wrap/Split/Panel/Fold | View in browser、Tab 子菜单、Focus other view、Hide Lines、Fold/Unfold Level、Sync Scroll、RTL/LTR、Monitoring 缺 |
| Encoding | Encode in、Convert to、Character sets | 字符集未完整覆盖原版 OEM/Eastern/North European 等细项 |
| Language | 常用语言按字母分组 | 原版完整语言、UDL 菜单、UDL 目录/站点入口缺 |
| Settings | Preferences、Style Configurator、Shortcut Mapper、Import、Edit Popup ContextMenu | 除 Preferences 外均占位 |
| Tools | MD5/SHA-256 骨架 | 全部占位 |
| Macro | Start/Stop/Play/Save/Load | Run a Macro Multiple Times、宏列表、快捷键映射未完整 |
| Plugins | 插件菜单边界 | Plugin Admin、插件命令动态兼容后置 |
| Window | Windows、Sort、Copy | 文档列表弹窗和动态 MRU 列表缺 |
| ? | Home、Docs、Command line、Debug、About | 在线入口和 Debug Info 占位 |

## 工具栏一致性

| 项 | 当前状态 | 差距 |
| --- | --- | --- |
| 标准工具栏 | 已有 New/Open/Save/SaveAll/Close/CloseAll/Cut/Copy/Paste/Undo/Redo/Find/Replace/Zoom/Wrap/Symbol/Macro/Panel | 顺序接近原版，但缺 Print、UDL、同步滚动、Document List、Monitoring 等按钮 |
| 图标来源 | 使用 qrc 内原版风格位图 | 尚未由 `toolbarIcons.xml` 驱动 |
| 可见性 | 从 `config.xml` 初始化，可在 View 菜单切换 | 原版更细的工具栏图标集、大小和 dark mode 策略未完成 |

## 状态栏一致性

| 项 | 当前状态 | 差距 |
| --- | --- | --- |
| 分区 | DOC_TYPE、DOC_SIZE、CUR_POS、EOF_FORMAT、UNICODE_TYPE、TYPING_MODE 六区已存在 | 视觉宽度、原版精确分区比例未截图验证 |
| 动态刷新 | 光标、选择、长度、行数、EOL、编码、INS/OVR 已有刷新 | Unicode 字节位置与原版完全一致性仍需专项验证 |
| 可见性 | 从 `config.xml` 初始化，可在 View 菜单切换 | 配置写回依赖关闭时保存 |

## 标签栏与主编辑区一致性

| 项 | 当前状态 | 差距 |
| --- | --- | --- |
| 标签栏 | 有 saved/unsaved 图标、关闭按钮、当前标签顶部高亮、双击空白新建 | 原版标签右键菜单、颜色、锁定/只读/监控图标仍缺 |
| 双视图 | Split、Rotate、Move、Clone 已有入口 | 原版新实例、跨实例行为缺；副视图视觉细节未截图验证 |
| 编辑区 | QScintilla、行号、符号边距、折叠、书签、缩进线、样式基础存在 | 右键菜单未由 `contextMenu.xml` 驱动；折叠层级和高级标记 UI 不完整 |

## 对话框与停靠面板一致性

| 项 | 当前状态 | 差距 |
| --- | --- | --- |
| Find/Replace | 已有主要对话框骨架和当前文档搜索入口 | Find in Files、Mark、透明度、完整历史/选项仍需后续 |
| Preferences | 已有多个设置页面 | 原版所有页和控件仍未完全覆盖 |
| Folder as Workspace | 已有 dock 和文件激活入口 | 工具按钮、会话目录、右键操作需要继续补 |
| Document Map | 已有 dock 和同步入口 | 视觉比例、滚动联动需人工检验 |
| Function List | 已有 dock 和跳转入口 | 解析完整度和配置驱动待后续 |
| Find Result | 已有 dock | 与原版 Finder 多文件格式和命令仍有差距 |
| Document List/Project/Clipboard/Character | 菜单占位 | dock UI 未实现 |

## UI 已存在但逻辑缺失

- File：Open Containing Folder、Open Default Viewer、Open Folder as Workspace、Save Copy As、Rename、Close Multiple、Move to Recycle Bin、Load/Save Session、Print。
- Edit：Insert、Copy to Clipboard、Indent、完整 Line Operations、高级 Comment、完整 Case、Blank Operations、Paste Special、On Selection、Column Mode、Read-only。
- Search：Find Next/Previous、Find in Files、Search Results Window、Style/Jump/Copy Styled、Mark、Find characters in range。
- View：Always on Top、Full Screen、Post-It、View in Browser、Tab 管理、Fold/Unfold Levels、Sync Scroll、RTL/LTR、Monitoring。
- Settings/Tools/Run/Window/Help 中多数入口。

## UI 仍缺失

- 编辑区右键菜单未实现 `contextMenu.xml` 驱动。
- Tab 右键菜单和 Document List 动态菜单缺失。
- Tools 顶层虽已出现，但 MD5/SHA-256 相关对话框缺失。
- Language 菜单未完全动态使用 `langs.xml` 和 UDL。
- Plugin Admin UI 缺失。
- 原版深色模式相关 UI 缺失。

## 阶段结论

阶段四完成后，Qt 版已经具备可进行后续功能补齐的主 UI 骨架。当前一致性评价：

- 顶层菜单：高。
- 常用菜单入口：中高。
- 高级菜单入口：中，主要以 disabled 占位为主。
- 工具栏：中。
- 状态栏：中高。
- 标签栏/编辑区：中。
- 对话框/停靠面板：中。

下一阶段应优先从“UI 已存在且已有部分逻辑”的功能开始，例如查找替换当前文档、Find All 当前文档、查找历史、EOL/编码转换、备份/外部变更检测。
