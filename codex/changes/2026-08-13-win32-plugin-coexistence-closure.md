# Win32 插件联合回归收口

## 范围

- 同时加载 27 个已经确认兼容的官方未修改 Windows x64 插件 DLL。
- 验证首次启动、基础文档操作、正常关闭、会话恢复和第二次启动。
- `PoorMansTSqlFormatterNppPlugin` 仍因 CLR 加载错误 1114 排除在兼容集合外。

## 问题与修复

联合测试在 `AutoCodepage + ElasticTabstops` 打开 `.acp` 文件时复现
`0xC0000005`。ElasticTabstops v1.3.1 官方源码在处理带
`SC_MOD_INSERTTEXT/SC_MOD_DELETETEXT` 的 `SCN_MODIFIED` 时会读取
`notify->text`；Qt 版编码重解释路径曾转发空指针。

`Win32PluginManager::notifyScintilla()` 现在为这类缺失文本的修改通知提供长度匹配、
调用期间稳定的零填充缓冲区，恢复原版 Scintilla 通知要求的可读指针契约。插件 DLL
没有修改，也没有增加插件专属业务实现。

## 自动验证

- `win32-plugin-coexistence-first-start`：27 个插件联合加载、EditorConfig、EOL、
  codepage、自动缩进、SessionMgr、真实窗口关闭和逆序 DLL 卸载。
- `win32-plugin-coexistence-restart`：复用配置目录，恢复首次运行保存的会话，确认第二个
  完整加载日志会话并再次正常关闭。
- 插件专项测试：`16/16` 通过。
- Debug 完整构建成功，完整 CTest：`46/46` 通过。

