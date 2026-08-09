# 高、中优先级插件兼容批次

## 结论

本批次以 Notepad++ v8.4.6 插件清单及对应版本源码为基线，未修改任何插件 DLL。

已通过现有 Win32PluginSystem 有限适配层接入：

| 插件 | 版本 | 结果 | 主要宿主能力 |
|---|---:|---|---|
| GotoLineCol | 2.4.2.0 | 已接入 | Dock、Buffer 编码、版本/主题/命令行、常用定位消息 |
| RandomValuesNppPlugin | 0.2.1 | 已接入 | 工具栏、配置目录、`SCI_REPLACESEL`/换行 |
| Merge files in one | 1.2.0.0 | 已接入 | 当前文件名、新建文档、文本读写 |
| SelectToClipboard | 1.0.3 | 已接入 | `SCN_UPDATEUI`、选区/行选区、代码页和剪贴板所需数据 |
| urlPlugin | 1.2.0.0 | 已接入 | 主题函数、工具栏、选区读写、新建文档 |

上述官方 x64 ZIP 已固定到 `third_party/win32-plugins`，由项目 updater 安装并校验 SHA-256；加载器仍只加载审核白名单。

## 宿主增量

- `Win32PluginManager` 将主、副永久 Scintilla 编辑器的 `NotificationData` 逐字段转换为插件 ABI 的 `SCNotification`，并使用对应代理 HWND 作为 `hwndFrom`。
- 主窗口适配器补齐当前 Buffer、路径、打开文件数、文档索引/激活、编码、状态栏、版本、主题颜色、插件目录和命令行消息。
- 编辑器适配器补齐上述 5 个插件源码实际使用的同步 `SCI_*`；仍不开放 `SCI_GETDIRECTFUNCTION` / `SCI_GETDIRECTPOINTER`。
- `NPPM_GETENABLETHEMETEXTUREFUNC` 返回系统 `uxtheme.dll` 的原生函数地址，避免在 Qt 业务层模拟 Win32 主题 API。

## 本批次跳过

| 插件 | 版本 | 原因 |
|---|---:|---|
| DoxyIt | 0.4.4 | 使用 Scintilla direct function/pointer，并安装键盘 Hook |
| SurroundSelection | 1.4.1 | 使用 Scintilla direct function/pointer，并安装键盘 Hook |
| ElasticTabstops | 1.3.1 | 使用 Scintilla direct function/pointer |
| XMLTools | 3.1.1.13 | 在 `NPPN_READY` 安装线程键盘 Hook；真实 DLL 运行矩阵出现消息循环卡死 |
| SessionMgr | 1.4.4 | 依赖完整 Session XML、文档事件和插件间消息宿主服务 |
| Linter | 0.1.0.0 | 依赖高频 SCN 管线、外部进程生命周期和配置服务 |
| WakaTime | 4.2.3 | 依赖网络/CLI、认证、隐私配置和外部进程生命周期 |

这些插件不是判定为永久不兼容；它们需要单独决策 direct-call/Hook、完整 Session Host Services 或外部进程与隐私边界。按照项目约束，本批次不为它们扩大架构。

## 验证

- 项目 updater 对新增 5 个官方包完成安装、哈希和收据验证。
- `ui-localization-runtime-100` 验证真实 DLL 加载、插件名、命令表及零加载错误。
- `ui-plugin-registration-rollback` 增加 `SCN_MODIFIED` / `SCN_UPDATEUI` 代理 HWND 与通知转发验证。
- XMLTools 隔离复现：启用时测试在菜单注册后卡死，禁用后同一完整矩阵通过。
