# Session 与文档策略插件兼容

## 范围

- SessionMgr 1.4.4
- AutoCodepage 1.2.4
- AutoEolFormat 1.0.2
- nppAutoDetectIndent 2.3

四个 Windows x64 官方 DLL 均保持未修改，通过插件管理更新器安装。测试包的
SHA-256 与 v8.4.6 插件清单一致。

## 宿主接口

- SessionMgr：补齐 `NPPM_SAVECURRENTSESSION`、`NPPM_LOADSESSION`、
  `NPPM_GETPOSFROMBUFFERID` 与 `NPPM_GETNPPDIRECTORY`。
- AutoCodepage：恢复 v8.4.6 编码菜单命令 ID，修正 `NPPM_GETEXTPART`
  为不含点号的扩展名语义。
- AutoEolFormat：复用 45001/45002/45003 EOL 命令，并修正打开文件通知顺序为
  `FILEBEFORELOAD -> FILEBEFOREOPEN -> FILEOPENED -> BUFFERACTIVATED`。
- nppAutoDetectIndent：补齐标准缩进 SCI 消息，并按插件 ABI 在 Scintilla 4
  旧 `TextRange` 与 Scintilla 5 原生 `Sci_TextRange` 之间选择。

这些行为位于通用 Win32 插件适配层和既有宿主服务中，插件业务逻辑仍由原 DLL
实现。

## 验证

- SessionMgr：加载、8 项命令注册、指定路径 session XML 保存、应用目录查询、
  Buffer 到视图位置映射。
- AutoCodepage：按 `.acp` 扩展名自动切换到 Windows-1251。
- AutoEolFormat：按 `.eolpolicy` 扩展名把 CRLF 转为 LF。
- nppAutoDetectIndent：读取真实文档并选择 Tab 缩进策略。
- 组合回归和完整 CTest：44/44 通过。

