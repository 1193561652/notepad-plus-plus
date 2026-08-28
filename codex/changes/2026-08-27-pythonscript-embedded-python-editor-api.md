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
- 跨平台 ABI v1 以尾部扩展方式增加另存为、会话、Buffer 枚举/激活、语言、编码、
  文档索引和分区状态栏服务，旧 v1 插件仍可按原结构长度加载。
- `notepad` 已覆盖对应的可移植文件/Buffer 操作，并使用原版 `NOTIFICATION` 数值提供
  文件生命周期、Buffer、语言及深色模式回调；控制台的 show/hide/clear 也已实际接入。

## 边界

编辑器消息面和常用的可移植 `notepad` 操作已补齐。原版依赖 HWND、Win32 消息、原生
菜单句柄、动态命令 ID/Marker 分配或隐藏 Scintilla 窗口的接口不进入公共 ABI；后续如需
增加宿主能力，仍只追加平台无关服务，不向插件传递 Qt 对象或平台窗口句柄。

插件实现和构建说明见同级仓库 `../PythonScript/qt/README.md`；总体状态见
`codex/index/high-priority-plugin-ports.md`。
