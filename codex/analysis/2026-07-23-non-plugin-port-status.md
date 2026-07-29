# 非插件移植状态报告

日期：2026-07-23

## 结论

Qt 主线已经消除主菜单中除“Import plugin(s)”外的禁用占位项，并为原先 9 个空白偏好页提供了实际 UI 和配置读写。日常文件编辑、搜索替换、会话、面板、工具和运行命令已形成可操作闭环。

这不等于 Notepad++ 8.4.6 所有非插件行为完全等价。当前应描述为“非插件主流程和可见入口已补齐，复杂兼容语义仍需专项收敛”。

## 功能状态表

| 领域 | 当前实现 | 与原版主要差异 |
| --- | --- | --- |
| 文件管理 | 新建、打开、保存、另存、另存副本、重命名、回收站、批量关闭、外部修改/删除处理 | 只读权限提升、超大文件和全部编码探测策略仍较简化 |
| 会话 | 主/副视图、选择、滚动、备份、编码、书签、折叠、手工加载/保存 | 标签颜色、Document Map 全字段和部分只读状态未完全应用 |
| 搜索替换 | 当前文档、所有打开文档、目录文件、项目页入口、标记与结果跳转 | Boost.Regex 语法、后台取消、进度和超大目录性能不等价 |
| 编辑 | 行操作、注释、大小写、日期、空白、列编辑、补全、括号和折叠 | 数值/列范围排序、完整自动完成 API 和全部原版命令仍有差异 |
| 停靠面板 | Folder Workspace、Document Map、Function List、Document List、3 Project Panels、Clipboard、Character | Project workspace XML、Function List parser 规则和 Document Map 像素行为不完整 |
| 偏好设置 | 19 页均有 UI；原版主要 GUIConfig 字段兼容读写 | 文件关联只跳转系统设置；打印渲染和部分选项只完成配置层 |
| 配置 | config/session/langs/stylers/shortcuts/contextMenu/UDL 基础兼容；未知节点保留 | 完整节点语义仍需真实配置语料持续往返测试 |
| 工具与 Run | MD5、SHA-256、自由 Run、UserDefinedCommands、窗口管理、帮助 | InternalCommands/ScintillaKeys 完整快捷键持久化尚未完成 |
| UI | 原版顶级菜单结构、主要工具栏/状态栏、全屏/置顶/Post-It | 仍需原版和 Qt 版逐窗口截图与 DPI/主题交互回归 |
| 插件 | 仅保留隔离接口和动态库边界 | 按本轮要求不实施原版 ABI、Plugin Admin、通知和 docking |

## 配置兼容证据

`Parameters.cpp::makeCfgEl()` 现在优先复用已有 `GUIConfig` 元素，只清理子内容并更新受管属性。未被 Qt 识别的属性继续留在原元素上，未被管理的节点不删除。

默认 XML 从 qrc 复制后会显式增加用户写权限。该修复是设置能够持久化的必要条件。

便携模式往返测试验证未知属性和未知 GUIConfig 节点均保留，程序正常启动与关闭退出码为 `0`。

## 后续优先级

1. 建立原版/Qt 版 UI 自动截图矩阵，覆盖浅色、深色、100%/150% DPI 和主副视图。
2. 建立 config/session/shortcuts/contextMenu 的语料化往返测试。
3. 专项完成 Shortcut ID 映射、打印渲染、workspace 文件和命令行参数。
4. 使用原版测试用例验证正则、编码、EOL、克隆文档和大文件边界。
