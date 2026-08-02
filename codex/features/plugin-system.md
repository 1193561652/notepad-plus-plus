# 插件系统功能索引

## 范围

插件系统分为两个独立范围：

- 插件包管理：清单、已安装状态、安装、更新、卸载和退出后更新器，当前已实现。
- 插件加载运行：规划和 v8.4.6 x86 静态调查已完成；原版 Windows ABI 与未来
  跨平台 ABI 当前仍是隔离的预留接口。

完整 ABI 决策和插件分级见 `codex/guides/plugin-system-porting-guide.md`。

## 管理入口

- `src/MISC/PluginsManager/PluginCatalog.*`：解析 v8.4.6 插件清单、版本区间和旧版兼容映射。
- `src/MISC/PluginsManager/PluginArtifactResolver.*`：统一管理器与加载器的平台动态库路径。
- `src/WinControls/PluginsAdmin/PluginAdminModel.*`：Available、Updates、Installed、Incompatible 分类。
- `src/WinControls/PluginsAdmin/PluginAdminDialog.*`：四页、搜索、描述、批量选择和退出确认。
- `src/MISC/PluginsManager/PluginUpdatePlan.*`：持久化并验证退出后操作计划。
- `src/MISC/PluginsManager/updater/PluginArchiveExtractor.*`：ZIP 列表、目录穿越和符号链接防护。
- `src/MISC/PluginsManager/updater/PluginUpdateExecutor.*`：下载、SHA-256、平台二进制预检和事务提交/回滚。
- `src/MISC/PluginsManager/updater/PluginUpdaterMain.cpp`：等待主进程退出、执行计划并重启。
- `MainWindow::showPluginAdmin()` / `schedulePluginOperations()` /
  `launchPendingPluginUpdater()`：主程序协调入口。

## Windows 原版插件兼容入口

- `src/Win32PluginSystem/Win32PluginManager.*` 只在 Windows 构建。
- `Win32MainWindowAdapter`、`Win32MainEditorAdapter` 和
  `Win32SubEditorAdapter` 分别固化主窗口、主编辑器和副编辑器的 Qt 对象及
  稳定 HWND，作为后续 `NPPM_*` / `SCI_*` 消息适配边界。
- 主窗口适配器使用 Qt 主窗口真实 HWND；主/副编辑器适配器使用管理器在主窗口
  `(0,0)` 创建的两个隐藏 `1×1` Win32 子窗口，并分别映射永久主/副编辑器对象。
- 两个代理窗口通过 `GWLP_USERDATA` 分发到主/副 editor adapter；支持的 `SCI_*`
  由 adapter 原样调用 Scintilla 5 消息入口，未知消息回落到 `DefWindowProcW`。
- 主窗口 adapter 通过 Win32 subclass 处理 `NPPM_GETCURRENTSCINTILLA`，根据 Qt
  主窗口当前活动的永久编辑器写回 `MAIN_VIEW/SUB_VIEW`。
- `Win32PluginManager::loadPlugin()` 已按 v8.4.6 六导出 ABI 加载 DLL，并将三个
  适配 HWND 组成 `NppData` 后调用插件 `setInfo()`。
- `Win32PluginManager::loadPlugins()` 扫描 Plugins Admin/updater 共用的
  `NppPath/plugins`；当前测试调用传入 `mimeTools` 白名单，只加载该文件夹。
- Windows x64 CTest 先用 `npp-plugin-updater` 按标准计划安装已校验的官方
  mimeTools ZIP，再验证安装收据、插件名 `MIME Tools` 和非空命令表。人工集成测试
  已通过同一 updater 从官方 Release 实际下载并安装。
- mimeTools v2.8 使用的九个 `SCI_*` 已全部加入白名单。真实 DLL 的 Base64 Encode
  在主/副视图通过，URL Encode 在主视图通过，覆盖 selection、target、字节缓冲和
  replacement 语义。
- `MainWindow` 完成主/副 `DocTabView` 创建后立即构造管理类，并传入主窗口与
  两个永久 `ScintillaEditView`。
- 管理类保存 Qt 窗口引用并提供相应 `HWND`，供后续原版六导出、消息路由和 Dock
  兼容层使用。
- 当前 Qt 版与原版一样拥有永久的主/副编辑器，插件可长期保存这两个稳定 HWND；
  标签切换只更换其 Scintilla document pointer。

## 当前行为

- Windows 内置 `nppPluginList/v1.5.4` 的 x86、x64、ARM64 JSON，并按进程架构选择。
- Linux/macOS 在没有本平台原生包清单前使用有效空清单，避免误装 Windows DLL。
- 路径保持既定兼容规则：Windows `plugins/<name>/<name>.dll`，Linux
  `plugins/<name>/linux/<name>.so`，macOS `plugins/<name>/macos/<name>.dylib`。
- 安装、更新和卸载都在主程序正常退出后执行，完成后重启，与 v8.4.6 工作流一致。
- 所有非卸载包先完成下载、SHA-256、ZIP 路径、符号链接和目标平台二进制检查；
  全部预检成功后才修改插件目录。
- 提交在插件根目录内使用 staging/backup 原子换位；批处理中途失败会逆序恢复旧目录。
- 成功安装写入 `.npp-package.json`；Windows 手工安装 DLL 可回退读取文件版本。
- Windows 程序目录不可写时使用 UAC 启动更新器，完成后使用桌面 shell 的普通用户
  token 重启主程序，避免主程序继承管理员权限。
- Linux/macOS 不自动调用系统提权工具；不可写目录会在退出前明确提示。

## 安全边界

- 更新计划拒绝空计划、重复/越界目录、`Config` 目录、非法 URL 和非法 SHA-256。
- 主程序把计划内容 SHA-256 随更新器命令行传递；更新器一次读取并在解析前校验，
  防止退出/UAC 交接期间替换计划文件。
- 远程下载遵循 Qt 代理/TLS，限制为 HTTP(S)；自动化测试使用 `file:` 本地包。
- ZIP 在解压前检查绝对路径与 `..`，解压后拒绝符号链接。
- 包必须包含当前平台约定位置和名称的动态库，否则整个批次不提交。
- 当前按已确认方案只校验包 SHA-256，不实现插件列表、更新器或动态库签名校验。

## 构建边界

- Plugin Admin 和 `npp-plugin-updater` 始终构建，不依赖 `ENABLE_PLUGIN_SYSTEM`。
- `ENABLE_PLUGIN_SYSTEM` 默认关闭，只控制实验性 Qt/C++ `IPlugin` 运行接口。
- 当前 `IPlugin` 不是冻结 ABI，也不等同于 Notepad++ 原版插件兼容。

## 验证

- `plugin-admin-tests`：版本、清单条目、兼容区间、平台路径、发现、收据和计划验证。
- `plugin-updater-tests`：真实 ZIP 安装/更新/卸载、错误哈希、缺失平台二进制、
  批次预检不改旧插件和目录穿越拒绝。
- 更新器子进程测试覆盖计划哈希不匹配时拒绝执行并保留诊断文件。
- Windows 构建已验证 PowerShell ZIP 实际执行路径。
- `plugin-investigation-tests`：核对 v8.4.6 x86 清单与 169 项 inventory、importance、
  六个字母批次、兼容等级和 API/依赖统一矩阵。
- 真实公网下载、UAC 交互确认和非 Windows 原生插件包仍属于发布环境/人工矩阵，
  不是代码逻辑缺口。

## 仍延期的插件工作

- 安全加载基础：PE 架构、六导出、依赖预检、加载日志、异常恢复和注册回滚。
- Windows v8.4.6 原版插件 ABI 兼容层。
- 跨平台稳定插件 ABI 及 SDK。
- Linux/macOS 原生插件生态清单、签名、公证和发布策略。
