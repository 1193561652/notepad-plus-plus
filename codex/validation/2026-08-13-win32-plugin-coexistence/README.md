# Win32 插件共存与重启验证

## 固定兼容集合

测试从 `third_party/win32-plugins` 固定官方包经 `npp-plugin-updater` 安装，并同时启用
27 个已审核插件。集合覆盖文本转换、WinForms 插件、Dock、Hook、Scintilla direct
调用、配置插件、SessionMgr、编码、EOL 和缩进策略插件。

## 测试流程

1. 清空隔离配置目录，写入 `pluginsEnabled.xml` 和插件测试配置。
2. 启动完整 `MainWindow`，确认 27 个插件全部注册且加载错误为空。
3. 打开测试文件并验证 EditorConfig、AutoEolFormat、AutoCodepage 和
   nppAutoDetectIndent 的实际结果。
4. 通过 SessionMgr 所需宿主消息保存 session XML。
5. 调用真实窗口关闭路径，发送退出协商和 `NPPN_SHUTDOWN`，随后逆序卸载 DLL。
6. 使用同一配置目录再次启动，接受并核验正常会话恢复，确认加载日志含两个完整会话。

## 结果

- 首次启动与第二次启动均正常退出。
- `plugin-load-in-progress.json` 没有残留。
- 两轮均加载 27 个插件且失败数为 0。
- 首轮发现并修复的 `AutoCodepage + ElasticTabstops` 空通知文本崩溃已由最小组合和
  全量组合共同覆盖。

