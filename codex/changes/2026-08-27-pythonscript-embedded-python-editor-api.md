# PythonScript 嵌入式 Python 与编辑器 API

日期：2026-08-27

## 结果

- `PythonScript-qt` 从外部进程/临时文件桥切换为插件进程内嵌 Python 3。
- `editor`、`editor1`、`editor2` 直接通过跨平台宿主 ABI v1 发送 Scintilla 消息。
- 从原版 `Scintilla.iface` 生成 743 个方法、2,306 个原始值及 775 个 SCI/SCN 标识，
  生成结果由 CTest 校验。
- 为字符串、颜色、cells、文本范围、styled text、搜索和 string-result 参数增加平台无关
  适配；仅含原生绘图句柄的 `formatRange` 明确返回不支持。
- 进程内测试覆盖脚本发现、两个编辑视图、修改/读取/搜索、常量及编辑通知回调。

## 边界

编辑器消息面已补齐，但宿主 ABI v1 当前只转发文本修改和 UI 更新通知；`notepad` 对象也
只能提供 ABI 已暴露的文件、Buffer、菜单、状态栏和基础对话框操作。扩展这些能力时应在
共享 ABI 中增加平台无关服务，不向插件传递 Qt 对象或平台窗口句柄。

插件实现和构建说明见同级仓库 `../PythonScript/qt/README.md`；总体状态见
`codex/index/high-priority-plugin-ports.md`。
