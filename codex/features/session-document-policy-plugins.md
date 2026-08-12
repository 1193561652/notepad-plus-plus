# Session 与文档策略插件

## SessionMgr

SessionMgr 1.4.4 使用官方原始 DLL。会话列表、设置、自动保存和原生对话框仍由
插件拥有；Qt 宿主只提供与 v8.4.6 对应的 session XML 保存/加载、Buffer 位置、
状态栏、文件生命周期通知和主程序目录服务。

## AutoCodepage 与 AutoEolFormat

两个插件按独立 INI 文件匹配扩展名，并通过 `NPPM_MENUCOMMAND` 请求宿主执行原版
编码或 EOL 命令。Qt 编码菜单动作保留原版命令 ID；EOL 使用 45001 至 45003。
打开文件时必须在 `NPPN_FILEOPENED` 后发送 `NPPN_BUFFERACTIVATED`，否则插件会因
仍处于加载屏蔽状态而忽略当前文档。

## nppAutoDetectIndent

插件通过 Scintilla direct API 扫描最多 5000 行，使用缩进、文本范围和 Tab 设置
消息。2.3 基于 Scintilla 5，不能走旧插件的 32 位 `TextRange` 翻译路径。当前
适配器在插件回调范围内选择对应结构 ABI，回调结束后恢复旧 ABI 默认值。

## 测试入口

- `win32-session-manager`
- `win32-document-policy-plugins`
- `win32-plugin-corpus-managed-install`

