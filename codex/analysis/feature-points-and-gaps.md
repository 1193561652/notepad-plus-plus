# 功能点与功能差异清单

分析日期：2026-07-22

## 说明

状态含义：

- 已实现：Qt 版已有可见实现，但仍需验证原版一致性。
- 部分实现：Qt 版已有入口或子集，但缺少原版完整行为。
- 占位：Qt 版有 UI、类或接口，但实际功能不足。
- 缺失：Qt 版未见对应实现。
- 待核验：需要继续读完整调用链。

## 文件类功能

| 功能点 | Qt 状态 | 差异说明 |
| --- | --- | --- |
| 新建文件 | 已实现/待核验 | Qt 版有 `newFile()` / `doNewBuffer()`；默认编码、EOL、语言需对齐原版配置 |
| 打开文件 | 部分实现 | Qt 版有打开流程；原版有编码检测、EOL 检测、大文件、备份恢复、语言检测 |
| 保存文件 | 部分实现 | Qt 版有编码写出；原版保存状态、备份删除、文件时间戳和编码模式更完整 |
| 另存为 | 部分实现 | Qt 版有入口；原版支持 Save Copy As、路径策略、文件名冲突和状态同步 |
| 保存全部 | 部分实现 | Qt 版有入口；原版有确认、失败处理、多视图状态 |
| 关闭文件 | 部分实现 | Qt 版有关闭询问；原版有多种批量关闭模式和备份处理 |
| Reload from Disk | 部分实现 | 原版支持延迟 reload、外部变更检测和提示策略 |
| 删除/重命名文件 | 缺失 | 原版 `fileDelete()`、`fileRename()` 已实现 |
| 打印 | 缺失 | 原版有 `Printer.*` 和 File 菜单打印命令 |
| 恢复最近关闭文件 | 缺失 | 原版有 `IDM_FILE_RESTORELASTCLOSEDFILE` |

## 编辑器类功能

| 功能点 | Qt 状态 | 差异说明 |
| --- | --- | --- |
| 基础文本编辑 | 已实现 | QScintilla 支撑，仍需验证命令行为 |
| Undo/Redo/Cut/Copy/Paste | 已实现/待核验 | Qt 版直接调用 QScintilla API |
| 行操作 | 部分实现 | Qt 版有删除/复制/上下移动；原版行操作更多 |
| 大小写转换 | 部分实现 | Qt 版仅 upper/lower；原版含 proper/sentence/invert/random 等 |
| 注释/取消注释 | 部分实现 | Qt 版有 toggle line comment；原版有 block/stream/comment set 多模式 |
| 书签 | 部分实现 | Qt 版有基本书签；原版标记、跳转、标记行操作更完整 |
| 语法高亮 | 部分实现 | Qt 版按 QScintilla lexer；UDL 与 Lexilla 映射未完整 |
| 自动完成 | 部分实现 | Qt 版有入口；原版 `AutoCompletion` 功能更完整 |
| Smart Highlight | 部分实现 | Qt 版有 indicator；原版有更多状态/样式联动 |
| 列编辑 | 缺失 | 原版有 `columnEditor.*` |
| 宏录制/回放 | 占位 | Qt 版动作和字符串占位，原版与 Scintilla/shortcuts 深度集成 |

## UI 与视图

| 功能点 | Qt 状态 | 差异说明 |
| --- | --- | --- |
| 菜单栏 | 部分实现 | Qt 版核心子集，原版命令极多 |
| 工具栏 | 部分实现 | Qt 版固定 qrc 图标，原版可配置 toolbar icons |
| 状态栏 | 部分实现 | Qt 版有多 label，需验证内容和刷新时机 |
| 标签页 | 部分实现 | Qt 版基础标签，原版上下文菜单/拖拽/样式更完整 |
| 双视图 | 部分实现 | Qt 版有主/副视图入口，Document 共享语义待核验 |
| 文档地图 | 部分实现 | Qt 版有面板，渲染/同步与原版不同 |
| 函数列表 | 部分实现 | Qt 版有面板，parser 完整性待核验 |
| 文件浏览器 | 部分实现 | Qt 版有面板，原版功能更多 |
| 项目面板 | 缺失 | 原版支持 3 个 ProjectPanel |
| 文档列表/垂直文件切换 | 缺失 | 原版有 DocumentList/VerticalFileSwitcher |
| 深色模式 | 占位/缺失 | 原版有完整 Windows dark mode |

## 搜索类功能

| 功能点 | Qt 状态 | 差异说明 |
| --- | --- | --- |
| Find Next | 部分实现 | Qt 版当前文档基础搜索 |
| Replace | 部分实现 | Qt 版有当前文档替换 |
| Replace All | 部分实现 | 范围、正则和选区语义需核验 |
| Count | 部分实现 | 当前文档范围 |
| Find All Current Doc | 部分实现 | Qt 版用简单结果信号 |
| Find All Opened Docs | 占位 | Qt 版按钮禁用 |
| Find in Files | 占位 | UI 存在，完整逻辑缺失 |
| Find in Projects | 占位/缺失 | 原版有项目面板搜索 |
| Mark All | 部分实现 | 多样式、复制/删除标记行不完整 |
| 搜索历史 | 部分实现/待核验 | Qt 版 combo 历史与原版 XML find history 未完整对齐 |

## 配置与资源

| 功能点 | Qt 状态 | 差异说明 |
| --- | --- | --- |
| config.xml | 部分实现，高风险 | 读写子集，未知节点保留待核验 |
| session.xml | 部分实现 | 结构相似，恢复细节待核验 |
| langs.xml | 部分实现 | Qt 版读取语言扩展和注释信息 |
| stylers.xml | 部分实现 | 样式映射需验证 |
| shortcuts.xml | 部分实现 | 宏和快捷键完整行为未对齐 |
| contextMenu.xml | 缺失/待核验 | 未见完整入口 |
| toolbarIcons.xml | 缺失/待核验 | Qt 版使用 qrc 图标 |
| userDefineLang.xml | 缺失 | UDL 未完整实现 |
| localization | 部分实现 | Qt 版 `NativeLangSpeaker` 和 Qt qm 并存 |

## 插件

| 功能点 | Qt 状态 | 差异说明 |
| --- | --- | --- |
| 插件接口 | 接口预留 | Qt `IPlugin` 不是原版 ABI |
| 插件加载 | 部分实现/待核验 | Qt 使用 `QLibrary` 思路，动态库后缀策略需核验 |
| 插件菜单动作 | 部分实现 | Qt 插件返回 `QAction*`，原版是 `FuncItem` 命令 |
| `NPPM_*` 消息 | 缺失 | 原版插件核心能力 |
| Plugin Admin | 缺失 | 原版有 JSON 管理和兼容列表 |
| 插件 docking | 缺失 | 原版 docking 与插件窗口句柄绑定 |

