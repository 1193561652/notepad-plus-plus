# 2026-07-28 P1 命令与文件关联收尾

## 修改范围

- 集中并扩充 v8.4.6 command ID 映射。
- 恢复 Windows 文件关联管理页和注册表适配。
- 增加纯逻辑自动化测试和待授权运行验证清单。

## 行为变化

- shortcuts.xml、Shortcut Mapper 和宏 type 2 可定位更多已实现动作。
- Word Wrap 使用 44022，Restore Zoom 使用 44033，Reverse Line Order 使用
  42083。
- Windows 管理员进程可添加/移除 Notepad++ 文件关联；原默认关联保存在
  `Notepad++_backup` 并在移除时恢复。
- 非管理员只读展示关联列表并提示以管理员身份重启。

## 平台边界

- 注册表代码只存在于 `PlatformServices` 的 Windows 分支。
- macOS/Linux 不伪造 Windows ProgID 行为，使用系统默认应用设置。
- 插件实现仍不属于近期范围；命令表仅保留原版插件导入动作 ID。

## 验证

- `cmake --build build --parallel 4`：通过。
- `ctest --test-dir build --output-on-failure`：21/21 通过。
- 未启动主程序，未写入注册表，未进行截图、真实 SMB 或物理打印验证。

后续获得用户授权后已完成 UI 捕获、P1 运行捕获和临时文件关联真实写入/恢复；
CTest 仍为 21/21。详情见
`codex/validation/2026-07-28-p1-runtime/README.md`。
