# P0 配置兼容与 UI 一致性验证

执行日期：2026-07-26

## 结论

本轮 P0 已完成：

- 建立真实 XML 语料的读取、写回和语义保持测试。
- 修复 `config.xml`、`session.xml` 和查找历史写回时丢失未知结构的问题。
- 使用 Qt 写回后的隔离配置成功启动 Notepad++ v8.4.6 原版。
- 建立原版 UI 截图基线和 Qt 版 100% / 150% 缩放截图矩阵。
- 修正主窗口/查找窗口标题、Windows UI 字体和关键对话框初始比例。

## 配置语料

`ConfigCorpusTests.cpp` 直接调用 `NppParameters` 的真实加载和写入入口，并在
临时目录中注入未知根属性、未知子属性、未知节点、未知 File 属性和嵌套状态。
比较使用 QDom 语义树，忽略格式化空白和注释，但严格比较元素顺序、属性和值。

当前 CTest 包含 14 项配置语料：

| 来源 | 覆盖文件 |
| --- | --- |
| v8.4.6 仓库语料 | config、shortcuts、langs、stylers、contextMenu、UDL |
| 当前用户隔离语料 | config、session、shortcuts |
| v7.8.1 诊断语料 | config、shortcuts、langs、stylers、contextMenu |

结果：14/14 通过。

## 写回修复

- 已知 `GUIConfig` 元素改为原位更新，不再清空未知属性和未知子节点。
- `FindHistory` 只替换受管历史节点，保留未知属性、未知节点和空占位结构。
- 最近文件与 Session File 按文件名复用原元素，保留未知状态。
- Session 主/副视图、书签、折叠和 FileBrowser 根节点均改为保守原位更新。
- 不再无条件新增 `stylerTheme`、`insertDateTime`、edge 等原配置中不存在的字段。
- 对齐 v8.4.6 的 `insertDateTime`、current-line indicator 和 edge 字段语义。

## 原版回读

Qt 版实际写回
`./build/p0-ui-audit/appdata-qt/Notepad++/`
后，使用 v8.4.6 原版的 `-settingsDir` 指向该目录。原版成功显示主窗口，
未弹出 XML 或配置加载错误。证据：

- `build/p0-ui-audit/original-readback-final/main.png`

## UI 验证矩阵

原版基线包含主窗口、5 个查找页和 19 个首选项页。Qt 版新增
`ui-parity-capture` 进程内采集目标，避免桌面前台焦点影响，并生成：

- `build/p0-ui-audit/qt-final-100/`：28 个文件。
- `build/p0-ui-audit/qt-final-150/`：28 个文件。

Qt 元数据：

| 项目 | 100% | 150% |
| --- | --- | --- |
| UI 字体 | Segoe UI 9 pt | Segoe UI 9 pt |
| 主窗口标题 | `*new 1 - Notepad++` | 同左 |
| 查找标题 / 页数 | `Find` / 5 | `Find` / 5 |
| 首选项标题 / 页数 | `Preferences` / 19 | 同左 |
| 首选项逻辑尺寸 | 830 x 372 | 830 x 372 |

150% 下未发现文字互相覆盖；较长首选项页由滚动区承载。

## 已确认差异

P0 的目标是完成验证、修复基础呈现错误并形成可重复证据，不代表像素级完全等价。
以下差异仍存在，应按相应功能范围继续移植：

| 区域 | 当前差异 |
| --- | --- |
| 主工具栏 | Qt 版动作和图标数量少于原版，尚未完全由原版配置驱动 |
| Preferences / General | Qt 控件分组与原版不同，缺少 localization、工具栏图标样式等控件；Editor Font 仍位于该页 |
| Preferences 按钮 | Qt 使用 OK / Apply / Cancel，v8.4.6 原版使用 Close |
| Find 布局 | 功能页齐全，控件间距和窗口高度仍非像素级一致 |
| 原版高 DPI | 已建立 100% 原版基线；原版 150% 自动截图仍需在可控桌面会话中补采 |

这些差异不再属于“未检查”状态，已经进入本地功能差异清单。

