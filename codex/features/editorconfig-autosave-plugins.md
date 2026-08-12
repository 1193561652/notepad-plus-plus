# EditorConfig 与 AutoSave 插件功能

## NppEditorConfig 0.4.0

| 功能 | 原版插件 | Qt 宿主状态 |
| --- | --- | --- |
| 项目配置发现 | 从当前文件向父目录查找 `.editorconfig` | 已由官方 DLL 验证 |
| 缩进 | `indent_style`、`indent_size`、`tab_width` | 已验证 |
| 行尾 | `end_of_line` | 已验证 LF |
| 保存处理 | `trim_trailing_whitespace`、`insert_final_newline` | 已验证 |
| 语言映射 | `NPPM_SETCURRENTLANGTYPE` | 宿主接口已存在，未扩展本轮样例矩阵 |
| 命令 | Reload、Show settings、About | Reload 已验证；其余 UI 可加载，未做文本像素对照 |

官方 v0.4.0 DLL 保持未修改。适配只补标准 `NPPM_*`、`NPPN_*` 和 `SCI_*`
宿主能力，没有在 Qt 主程序重写 EditorConfig 解析逻辑。

## AutoSave 1.6.1.0

| 功能 | 原版插件 | Qt 宿主状态 |
| --- | --- | --- |
| 配置 | 独立 `plugins/Config/AutoSave.ini` | 已验证读取和 Options 回显 |
| 手动时间戳副本 | 保存当前编辑 Buffer 到带时间戳副本 | 已验证内容、原文件和 dirty 状态 |
| 失焦触发 | 自装主窗口 WndProc，应用失焦时保存 | 真实前台切换与覆盖保存已验证 |
| 定时触发 | 按分钟 Timer 保存 | 最小 1 分钟周期、磁盘内容和 dirty 状态已验证 |
| 命名文件策略 | 忽略、覆盖、同目录 autorecover | 覆盖所需命令和副本接口已接入；完整 UI 矩阵待交互验证 |
| 未命名文件策略 | 忽略、询问路径、静默目录、autorecover | `NPPM_SAVECURRENTFILEAS` 已接入；文件对话框矩阵待交互验证 |

AutoSave 官方仓库不提供对应源码。本轮未反汇编 DLL；使用官方包、公开功能说明、
PE 导入、配置 UI 与受控消息记录确认行为。关键接口 `NPPM_SAVECURRENTFILEAS`
保持原版 `asCopy` 语义。
