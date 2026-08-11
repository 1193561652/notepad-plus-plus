# 无白名单 Win32 插件加载验证

## 目标

验证移除 Windows 插件审核白名单后，不兼容插件是否只表现为功能降级，而不会导致
主程序崩溃或影响其他功能。

## 方法

- 临时将 `MainWindow::setupPluginSystem()` 改为无过滤扫描全部已安装插件。
- 使用项目管理安装语料和隔离配置运行 `ui-localization-runtime-100`。
- CTest 单项超时设为 35 秒，加载过程由 `PluginLoadJournal` 记录。
- 验证完成后恢复审核白名单，未把不安全配置保留在主线。

## 结果

- 18 个 DLL 完成注册。
- `PoorMansTSqlFormatterNppPlugin` 在 `LoadLibraryW` 返回错误 1114 后被正常记录为失败，
  加载器继续处理后续插件。
- `XMLTools` 被记录为加载成功；随后进程发生段错误，CTest 失败。该顺序只能说明
  相关性，不能单独完成插件归因。
- 崩溃发生在加载会话标记完成后，没有留下 `plugin-load-in-progress.json`，因此当前
  恢复机制不能把回调期或插件后台行为故障归因到具体插件。

## 逐插件及组合归因

后续增加了仅在 `ui-parity-capture` 测试目标生效的过滤覆盖，所有插件均使用完整
`MainWindow` 初始化、菜单、READY、通知和析构路径，在独立进程与独立配置目录运行。

| 结果 | 插件 |
|---|---|
| READY 阶段卡死 | `BetterMultiSelection` |
| 可控加载失败 | `PoorMansTSqlFormatterNppPlugin`，`LoadLibraryW` 错误 1114 |
| 单独运行无崩溃/卡死 | 其余 17 项，包括 `XMLTools` |

关键组合结果：

- `BetterMultiSelection + XMLTools`：稳定复现访问冲突，退出码 `0xC0000005`
  （十进制 `-1073741819`）。
- `BetterMultiSelection + BracketsCheck`：保持 READY 阶段卡死，没有转为访问冲突。
- 排除 `BetterMultiSelection`、加载其余全部插件：正常退出到预期测试断言，没有崩溃。
- `XMLTools` 单独完成加载、READY、编辑器通知和卸载，没有崩溃。

因此，初始段错误不能归因为“XMLTools 单独崩溃”。当前确认的独立故障插件是
`BetterMultiSelection`；访问冲突是它与 `XMLTools` 同时运行时的组合故障。

## 结论

“移除白名单后，不兼容插件只降级功能”的期望在当前同进程 Win32 ABI 兼容层上不成立。
静态导出检查和注册回滚只能处理可控失败，不能隔离 `DllMain`、通知回调、Hook、插件
窗口过程、后台线程中的访问冲突或死锁。

继续推进该目标前至少需要单独设计：

1. 把加载、通知和命令回调都纳入插件归因日志，使崩溃后可自动隔离嫌疑插件。
2. 为未知插件增加辅助进程预检和超时；该预检只能降低风险，不能证明进程内运行安全。
3. 对 Hook、窗口子类化、direct function 和后台线程插件保留能力隔离或明确拒绝。
4. 若要求首次运行也绝不影响主程序，需要进程外插件宿主；这不能自然完整兼容原版
   HWND、指针和 Scintilla direct-call ABI，必须作为新架构决策。
