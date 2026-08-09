# 原版结构对齐验证

日期：2026-08-09

## 静态检查

- CMake 和活跃源码/测试中不再引用本批次迁移前路径。
- Preferences 文件名大小写与 v8.4.6 一致，可用于大小写敏感文件系统。
- `WinControls/DockingWnd` 只保留 `DockingManager`。
- Win32 插件消息常量和解析仍只存在于 `src/Win32PluginSystem`；MainWindow 暴露的
 宿主服务使用平台中性命名。
- `git diff --check` 通过。

## 构建与测试

```text
cmake --build build --config Release -j 4
ctest --test-dir build --output-on-failure
```

- Release 全目标构建通过。
- CTest 37/37 通过，耗时 40.95 秒。
- UI 100%/150%、亮色/暗色、`-noPlugin`、插件恢复和注册回滚均通过。
- v8.4.6 配置、快捷键、语言、样式、右键菜单和 UDL 语料全部通过。
- Buffer、Lexilla、Boost.Regex 和大文件模式回归通过。
