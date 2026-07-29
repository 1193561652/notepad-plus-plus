# 2026-07-28 命令行兼容缓存

- v8.4.6 参数全集已由 `CommandLineOptions` 统一建模。
- `main.cpp` 负责配置加载前覆盖和 `QLocalServer` 单实例边界。
- `MainWindow::applyCommandLineInvocation()` 同时处理首次启动和后续实例转发。
- `-nosession` 同时禁止默认会话读取与退出写回。
- `-settingsDir` 参与本地服务名，隔离配置目录也隔离实例组。
- `-l`/`-udl`、`-n`/`-c`/`-p`、`-ro`/`-monitor` 为逐文件行为。
- 退出型 `-quickPrint` 和 `-export=functionList` 强制独立、无会话、隐藏运行。
- Debug 构建成功，CTest `19/19`；真实导出和双进程 IPC 通过。
- 详细说明见 `codex/features/command-line-compatibility.md`。
