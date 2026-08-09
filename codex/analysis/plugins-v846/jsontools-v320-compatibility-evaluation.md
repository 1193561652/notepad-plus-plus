# JsonTools 3.2.0 兼容性评估

> 2026-08-09 实施更新：本文第 5-7 节记录的是实施前缺口。所列 4 个 `NPPM_*`、
> 3 个 `SCI_*` 和 2 个通知现已实现，官方 x64 DLL 已完成加载、格式化/压缩及首个
> WinForms Tree Dock 自动回归。上游代码有意把功能索引 `4` 写入 `tTbData::dlgID`；
> `NPPN_TBMODIFICATION` 刷新的是用于菜单勾选的宿主 command ID，不应把两者合并。
> Settings、RemesPath 查询/赋值、JSON Lines、YAML、树节点跳转和 4 MB 阈值现已纳入
> 真实 DLL 自动矩阵。唯一剩余边界是 v3.2.0 `Run tests` 中无保护的作者绝对测试路径。

## 1. 结论

JsonTools 3.2.0 继续维持 Windows 原 DLL 兼容等级 **B**，但已经具备进入实现阶段的条件。
它不要求修改现有插件架构，也不要求模拟原版控件树；当前 JSON Viewer 已验证的
`Win32PluginDockAdapter`、`Win32NativeDockHost` 和永久主/副编辑器代理可以直接复用。

近期推荐路线是在 Windows 上加载官方 `JsonTools.dll`，仅补充目标插件实际使用的窄
`NPPM_*`、`SCI_*` 和通知集合。Linux/macOS 不能加载该 DLL；跨平台版本需要在未来
稳定插件 ABI 上迁移解析、RemesPath 和 Qt UI，不能把 .NET Framework 或 WinForms
纳入通用 ABI。

## 2. 基线和证据

| 项目 | 结果 |
| --- | --- |
| Notepad++ 基线 | v8.4.6 插件清单 |
| 插件 | JsonTools 3.2.0，官方 tag `v3.2.0`，commit `7d9a94388adb8733959780d4a4d061c479a2a1d5` |
| x86 包 | `Release_x86.zip`，SHA-256 `8371a76462adb1afd231a3f655a16e66a0097ae799178d4a5a2388ae51c052fd` |
| x64 包 | `Release_x64.zip`，SHA-256 `884704a9dc046616831a8df6013114100472e4a7b7a4bb62e40309b286b33a0d` |
| x64 DLL | `JsonTools.dll` 487424 bytes，SHA-256 `d1a70506637f06e33b8d1c6e65770ff1da729a8f6032a0f62143d61581efc462` |
| 许可证 | Apache-2.0 |

两个 ZIP 与项目内 v8.4.6 x86/x64 清单的哈希完全一致。x64 包只包含一个
`JsonTools.dll`，没有插件私有依赖 DLL。

## 3. 二进制和运行时

- C# 工程目标为 `.NET Framework 4.0`，分别提供 x86/x64 target。
- UI 使用 `System.Windows.Forms` 和 `System.Drawing`。
- NppPlugin.NET 的 DllExport/Mono.Cecil 构建桥生成六个标准 C 导出：
  `isUnicode`、`setInfo`、`getFuncsArray`、`messageProc`、`getName`、`beNotified`。
- x64 DLL 为 AMD64 PE32+；CLR header 为 2.5，运行时标记为 `v4.0.30319`，
  `ILONLY=0`，导入 `mscoree.dll!_CorDllMain`。
- 本机已安装 .NET Framework 4.8，因此满足 v4 程序集运行时前提。JsonTools 不需要
  Poor Man's T-SQL Formatter 所涉及的 CLR 2.0 兼容激活策略。
- CLR 在进程内全局启动，不能随单个插件卸载。首次 `LoadLibraryW`、托管导出进入、
  未处理托管异常和关闭时 WinForms 对象回收仍必须通过真实 Qt 进程验证。

## 4. v3.2.0 功能面

插件公开 10 个菜单项，其中 2 个是分隔符：文档、格式化、压缩、JSON Tree、设置、
JSON Lines、JSON 转 YAML 和内置测试。

主要行为包括：

- 宽松 JSON 解析和 lint 错误报告；
- pretty-print、compress、JSON Lines 和 YAML 输出；
- WinForms JSON Tree Dock，树节点跳转到编辑器行；
- RemesPath 查询、赋值修改、查询结果新建文档和 JSON-to-CSV 对话框；
- 4 MB 默认完整树阈值，超过阈值只创建根的直接子节点；用户可确认强制加载完整树。

源码中 `JsonGrepper` 含网络和多线程实现，但 v3.2.0 菜单/UI 没有调用它，因此不属于
本版本运行兼容的必需范围。

## 5. 精确 API 缺口

### 已有能力

| 接口 | 当前状态 |
| --- | --- |
| `NPPM_GETCURRENTSCINTILLA` | 已支持 |
| `NPPM_GETPLUGINSCONFIGDIR` | 已支持 |
| `NPPM_SETCURRENTLANGTYPE` | 已支持 |
| `NPPM_DMMREGASDCKDLG` | 已支持并经 JSON Viewer 验证 |
| `NPPM_SETMENUITEMCHECK` | 已支持 |
| `SCI_GETLENGTH/GETTEXT/SETTEXT` | 已支持 |
| `NPPN_SHUTDOWN` | 已发送 |

### 必须补充

| 接口 | 使用场景 | 风险 |
| --- | --- | --- |
| `NPPM_GETFULLCURRENTPATH` | 解析缓存、树与当前文档关联 | 低 |
| `NPPM_GETFILENAME` | `.jsonl` 扩展名判断 | 低 |
| `NPPM_MENUCOMMAND(IDM_FILE_NEW)` | lint、YAML、查询和 CSV 结果 | 中；必须走现有 `MainWindow::newFile()` 语义 |
| `NPPM_DOOPEN` | lint 报告后重新激活原文件 | 中；需保持路径和现有 Buffer 复用语义 |
| `SCI_APPENDTEXT` | 新文档结果、CSV、测试输出 | 低 |
| `SCI_GOTOLINE` | 树节点导航 | 低 |
| `SCI_GOTOPOS` | append 后移动光标 | 低 |
| `NPPN_TBMODIFICATION` | 把宿主分配的 command ID 刷回托管 `FuncItems` | 高；未发送会使 Dock `dlgID` 和菜单勾选仍使用索引 4 |
| `NPPN_FILEBEFORECLOSE` | 清除按文件名缓存的解析树 | 中；需在关闭 Buffer 前发送 |

`NPPM_GETCURRENTDIRECTORY` 和 `NPPM_GETFULLPATHFROMBUFFERID` 存在于通用网关，但当前
v3.2.0 可达业务路径没有调用，首轮不应加入白名单。`SCI_GETDIRECTFUNCTION/POINTER`
也不在本插件范围内。

路径消息还有一个实现细节：JsonTools 以 `wParam=0` 和预分配 `StringBuilder` 调用
`NPPM_GETFULLCURRENTPATH/NPPM_GETFILENAME`，不能套用当前要求 `wParam` 为容量的
`copyPluginString()`；适配器需要按原消息契约写入 `MAX_PATH` 宽字符缓冲区。

## 6. 结构和行为风险

1. **命令 ID 时序**：加载器写入 `FuncItem::_cmdID` 后必须发送
   `NPPN_TBMODIFICATION`。JsonTools 会在通知中执行 `RefreshItems()`，之后才可打开
   Dock 或调用 `NPPM_SETMENUITEMCHECK`。
2. **托管边界**：C++ 侧插件回滚能处理导出缺失和正常返回失败，不能假设所有未处理
   CLR 异常都能被宿主捕获。现有启动恢复日志仍是进程异常退出后的主要恢复手段。
3. **Dock 生命周期**：`TreeViewer.Handle` 是 WinForms 原生 HWND，可由现有 native
   dock host 承载；必须验证关闭、浮动、重新停靠、多次打开和退出时先分离 HWND，
   再释放 Dock，最后 `FreeLibrary`。
4. **窗口层级**：预期与 JSON Viewer 相同，只提升承载 Dock 所需的 Qt 窗口，不提升
   编辑器、splitter 或 central widget。WinForms 的设置、消息框和 CSV 对话框是独立
   原生顶层窗口，不应改造成 Qt 窗口来伪装兼容。
5. **双视图上游行为**：v3.2.0 的 `Npp.editor` 在首次使用时缓存一个 Scintilla HWND，
   后续不会自动切换。原版 DLL 也具有该行为；兼容层不应偷偷改变插件内部语义，
   但回归报告必须记录它。
6. **大文档**：插件先全量 `GETTEXT` 到托管字符串再解析，树阈值只限制树展开，不限制
   全文复制。需要测量 4 MB 边界和更大文档的 UI 阻塞、托管内存峰值及 undo 行为。
7. **已知 v3.2.0 行为**：`AppendTextAndMoveCursor` 按 .NET 字符数移动，和 UTF-8 字节
   位置并不总是一致；这是上游版本行为，首轮兼容不得擅自修正。
8. **全文读取边界**：业务代码把 `SCI_GETLENGTH` 的结果原样作为 `SCI_GETTEXT` 缓冲区
   长度，而不是显式增加 NUL 空间。适配器应保持 Scintilla 消息原义，是否造成末字节
   截断必须在原版和 Qt 版使用同一语料对照，不能由宿主私自补偿。

## 7. 实施状态

1. **静态接入（完成）**：把官方 x64 包加入固定测试语料和安装计划；核对收据、六导出、版本和
   架构。PE 架构/依赖通用预检仍按既有决策延期，不借 JsonTools 扩大范围。
2. **窄接口增量（完成）**：实现本报告列出的 4 个 NPPM、3 个 SCI 和 2 个通知；每项增加适配器
   单元/集成断言。
3. **真实 DLL 冒烟（完成）**：只对白名单中的 `JsonTools` 启用，验证 CLR4 加载、10 个菜单项、
   pretty-print、compress、JSON Lines、YAML、settings 和 shutdown。
4. **Dock 与功能矩阵（核心完成）**：验证树节点跳转、RemesPath 查询/赋值、结果新文档、CSV、
   partial/full tree、close/float/redock/reopen 和多 Dock。
5. **可靠性收口（完成）**：验证异常启动恢复、失败注册回滚、`-noPlugin`、大 JSON、Unicode、
   主/副视图、关闭文档和程序退出。

## 8. 决策

- **Windows 近期方案**：直接兼容并运行官方 JsonTools 3.2.0 DLL。
- **是否需要大架构改动**：不需要；新增能力均应放在 `src/Win32PluginSystem`，文档/文件
  操作调用现有 `MainWindow` 和 Buffer 入口。
- **是否现在重写 Qt 版 JsonTools**：不建议。先用真实 DLL 完成 Windows 行为基线，再在
  跨平台 ABI 冻结后决定 parser/RemesPath 源码迁移。
- **验证状态**：官方 CLR4 DLL 已在 Qt 主进程运行，自动矩阵覆盖核心命令、WinForms Dock、
  查询/赋值、YAML、JSON Lines、树跳转和阈值；完整 CTest 37/37 通过。
