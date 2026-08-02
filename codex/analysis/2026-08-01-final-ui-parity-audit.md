# 原版与 Qt 版最终 UI 对照

日期：2026-08-01

## 结论

> 状态更新：本文件最初记录的是第一次最终对照。其后已完成 Preferences、
> Shortcut Mapper、Project Panel、Find 外框、工具栏/状态栏/标签和编辑器边栏收口。
> 当前结论以本节更新、`codex/changes/2026-08-01-preferences-shortcut-project-parity.md`
>、`codex/changes/2026-08-01-preferences-margin-ui-finalization.md` 和
> `codex/changes/2026-08-01-editor-border-edge.md` 为准。

本轮已完成当前 Windows 环境中的最终 UI 对照。原版基线固定为
Notepad++ v8.4.6，Qt 版固定为本次 Scintilla 5 / Lexilla 迁移后的主线。

- 主窗口基本结构、顶级菜单顺序、编辑区、状态栏分区和标准工具栏已经接近原版。
- Find 的五个页面已固定到原版客户区基线并完成剩余布局收口。
- Preferences 的 19 个分类、页面职责、主要分组、客户区和单一 Close 已按 v8.4.6 重建。
- Shortcut Mapper 已恢复五页、过滤/冲突/修改工作流；Project 1/2/3 已恢复独立 Dock 与 Workspace/Edit。
- Qt 浅色/深色和 100%/150% DPI 均无文字重叠、截断或不可读控件。
- 编辑器恢复 `borderEdge` 和 `borderWidth`：浅色为原版双线 3D 下沉客户区边框，
  深色为单线边框，默认保留 2px 外部间距；首选项及配置写回均可用。
- Windows 100% DPI 的客户区、控件层级和工作流已达到当前像素对照基线；
  Qt 与 Win32 的字体栅格和系统控件绘制仍不承诺截图逐字节相同。

## 验证矩阵

| 程序 | 主题 | DPI | 覆盖 |
| --- | --- | --- | --- |
| 原版 v8.4.6 | 浅色 | 当前系统 100% | 主窗口、Project Panel、Shortcut Mapper、5 个 Find 页、19 个 Preferences 页 |
| 原版 v8.4.6 | 深色 | 当前系统 100% | 主窗口、5 个 Find 页、19 个 Preferences 页 |
| Qt 版 | 浅色 | 100% / 150% | 主窗口、Project Panel、Shortcut Mapper、5 个 Find 页、19 个 Preferences 页 |
| Qt 版 | 深色 | 100% / 150% | 同上 |

原版 150% 需要改变 Windows 桌面缩放并重新登录或切换显示会话，本轮没有修改
用户系统显示设置。Qt 150% 使用 `QT_SCALE_FACTOR=1.5` 隔离验证。

## 对照结果

| 区域 | 已对齐 | 仍存在的差异 | 结论 |
| --- | --- | --- | --- |
| 主窗口 | 顶级菜单顺序、编辑区、标签栏、工具栏和状态栏基本结构 | 工具栏图标数量/禁用态、状态栏分区宽度、标签细节不同 | 高度接近，非像素一致 |
| 窗口标题 | `new 1 - Notepad++` | 无已知启动标题差异 | 本轮已修复 |
| Find | 5 个标签、主要控件、固定客户区 `573x336` | 字体栅格和系统边框存在平台差异 | 当前基线完成 |
| Preferences 外框 | 19 个分类；原版客户区和 Qt 均约 `830x372` | Qt 使用滚动页承载较长内容 | 尺寸和导航对齐 |
| Preferences 命令 | 单一 Close/关闭 | 原版设置多为即时生效；Qt 在关闭时集中写回 | 本轮修复可见命令，仍有时机差异 |
| Preferences 内容 | 19 页按 v8.4.6 职责和主要分组重排 | 少量系统控件度量随 Qt 主题变化 | 当前基线完成 |
| Shortcut Mapper | 5 页、过滤、冲突区、Modify/Clear/Delete/Close | 插件页内容受插件 ABI 延期影响 | 非插件工作流完成 |
| Project Panel | 三个独立 Dock、Workspace/Edit、207 px 目标宽度 | Dock 装饰由 Qt 平台主题绘制 | 当前基线完成 |
| 深色模式 | 主窗口、Find、Preferences 均可读 | 原版对话框偏中灰，Qt 偏深黑；边框、选中态和滚动条样式不同 | 主题完整，色值非原版复刻 |
| DPI | Qt 100%/150% 无重叠和截断 | 当前桌面未采集原版真实 150% | Qt 验证通过，原版高 DPI 留作环境验证 |

## 本轮修复

1. 新建和打开 Buffer 后立即刷新活动窗口标题，恢复原版标题格式。
2. Preferences 底部由 `OK / Apply / Cancel` 改为原版单一 Close 命令。
3. 补充简体中文 `btnPrefsClose=关闭`，并加入自动断言。
4. 改进 Win32 UI 捕获脚本：真实按键、原版命令 ID、菜单窗口过滤、
   Preferences 列表消息选择、模态命令异步投递和初始化等待。

## 后续 UI 开发范围

上述开发项均已完成。剩余 UI 工作只包括需要指定桌面环境才能确认的原版真实
150% DPI 对照，以及 Qt/Win32 字体栅格、系统边框等平台绘制差异；它们不再作为
未实现功能记录。
