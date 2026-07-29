# 2026-07-28 P1 待授权运行验证

状态：已获得授权并完成可在当前环境执行的运行验证。结果见
`codex/validation/2026-07-28-p1-runtime/README.md`。

## 文件关联

1. 以普通权限打开偏好设置，确认列表只读且显示管理员提示。
2. 以管理员权限启动，在自定义分类添加测试扩展名，确认关联目标、图标和打开命令。
3. 对已有默认关联的测试扩展名执行添加再移除，确认原 ProgID 被恢复。
4. 清理全部测试扩展名，确认不遗留用户文件关联。

该组已使用随机临时扩展名完成，原 ProgID 已恢复，扫描确认无测试键残留。

## 命令与快捷键

1. 打开 Shortcut Mapper，确认新增覆盖动作可见且 ID 与 v8.4.6 一致。
2. 临时修改 Word Wrap、Restore Zoom、Reverse Line Order 与自动完成快捷键，
   重启后确认 shortcuts.xml 写回和重新加载。
3. 使用宏 type 2 分别触发上述命令，确认 QAction 路由正确。

## 环境相关遗留

1. 使用真实 UNC/SMB 路径验证文件变化轮询、重命名和断线恢复。
2. 使用物理打印机验证页眉页脚、页码、边距、行号与颜色模式。
3. 对偏好设置、Shortcut Mapper、UDL、Finder 和各停靠面板进行多 DPI、
   本地化、深色模式及像素级原版对照。

`notepadpp-qt.exe`、`ui-parity-capture.exe` 和
`p1-runtime-capture.exe` 均已启动验证。真实 UNC/SMB、物理打印机和用户主观
UI 对照仍受环境或人工确认限制。
