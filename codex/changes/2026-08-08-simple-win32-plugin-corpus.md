# 前 8 个简单 Win32 插件适配与验证

## 范围

本轮以 mimeTools 2.8 的真实 DLL 接入方式为基线，处理推荐列表中的前 8 个插件。所有包均来自 v8.4.6 x64 插件列表指向的官方 Release，并通过 `npp-plugin-updater` 完成下载、SHA-256 校验、ZIP/平台预检和事务安装。

## 结果

| 插件 | 安装 | 加载/功能 | 结论 |
|---|---|---|---|
| Reverse Lines 1.0.0.0 | 通过 | 选区和全文反转通过 | 已适配 |
| Remove Duplicate Lines 1.3.0.0 | 通过 | 保留空行的选区去重通过 | 已适配 |
| SelectQuotedText 1.0.0 | 通过 | 光标所在单词选择和 `Alt+'` 通过 | 已适配 |
| BracketsCheck 1.2.2 | 通过 | 全文括号检查、结果对话框、配置目录和菜单分隔符通过 | 核心功能已适配；旧式 `HMENU` 勾选回写待决策 |
| Poor Man's T-SQL Formatter 1.6.13 | 通过 | `LoadLibraryW` 返回 1114 | 按规则跳过；需要 CLR 2.0/4.0 进程激活策略决策 |
| SecurePad 2.4 | 通过 | 选区及全文加密/解密、原生密钥对话框通过 | 已适配 |
| Code Alignment 14.1 | 通过 | 等号对齐、配置/语言/扩展名查询和快捷键通过 | 已适配 |
| BetterMultiSelection 1.5 | 通过 | 未加入加载白名单 | 按规则跳过；依赖全局 Hook、通知链和多选输入状态 |

## 宿主增量

- `Win32MainWindowAdapter` 支持 `NPPM_GETPLUGINSCONFIGDIR`、`NPPM_GETCURRENTLANGTYPE`、`NPPM_GETEXTPART` 和 `NPPM_SETMENUITEMCHECK`。
- `Win32EditorMessageAdapter` 增加上述 6 个插件实际使用的同步 `SCI_*` 白名单，仍由 Scintilla 5 原样执行，不在 adapter 重写编辑逻辑。
- `Win32PluginManager` 为 `FuncItem` 分配稳定 command ID，读取初始勾选状态和快捷键；空函数指针按原版 ABI 解释为菜单分隔符。
- `MainWindow` 仅加载已经审计为同步消息型的插件目录；BetterMultiSelection 和 Poor Man's T-SQL Formatter 均明确排除，后者的 1114 错误来自隔离加载探测。

## 可重复验证

- 8 个官方 ZIP 缓存在 `third_party/win32-plugins/<folder>/<version>/x64/`，配置阶段逐包校验固定 SHA-256。
- `tests/Win32PluginCorpusInstallPlan.json.in` 通过项目 updater 安装 mimeTools 和本轮 8 个插件，并核对每个 `.npp-package.json` 回执。
- `UiParityCapture` 使用真实 DLL 菜单命令验证文本、选区、快捷键、主窗口消息、Scintilla 消息和原生模态对话框。
- Windows 浅色/深色、100%/150% 四组运行测试及安装 fixture 已全部通过。
- 人工验证确认 Reverse Lines、Remove Duplicate Lines、SelectQuotedText、BracketsCheck、SecurePad 和 Code Alignment 的主视图操作正常；副视图重复操作尚未人工执行，已有自动化覆盖。

## 保留差异

BracketsCheck 使用 `GetMenu`/`CheckMenuItem` 直接操作原版主窗口 `HMENU`，而不是发送 `NPPM_SETMENUITEMCHECK`。Qt 主窗口没有可供插件直接修改且与 `QMenuBar` 同步的原生菜单；强行挂载 `HMENU` 会改变现有窗口结构，因此本轮不做架构扩展。默认配置下菜单状态与功能正常，持久化为未勾选后重新勾选的 Qt 视觉回写记录为后续决策项。
