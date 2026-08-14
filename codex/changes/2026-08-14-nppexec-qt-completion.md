# NppExec-qt 基础功能补全

## 基线与结构

- 以原版 NppExec `v082` 为行为和结构基线。
- 新增 `ScriptRepository`，对应原版 `CNppScriptList` 的独立所有权。
- 新增 `ScintillaMessageCall`，在插件执行层管理同步消息参数和指针生命周期。
- 未扩展宿主 ABI，也未向公共 ABI 暴露 Qt 对象或 Win32 窗口句柄。

## 完成内容

- 兼容读取和写入原版 `npes_saved.txt` 的 `::script-name` 格式。
- 保存前生成 `npes_saved.txt.bak`，执行对话框支持选择、保存、删除命名脚本。
- `NPP_EXEC` 可调用命名脚本，并提供 `ARGC`、`ARGV[0]`、`ARGV[n]` 变量。
- `SCI_SENDMSG` 支持整数、字符串、十六进制字节以及 `@` 指针回写形式。
- 同步调用后更新 `MSG_RESULT`、`MSG_WPARAM`、`MSG_LPARAM`。

## 保留边界

原版 `NPP_SENDMSG` 可以发送任意 Win32 主窗口消息并携带私有结构指针。这类接口
无法形成稳定的平台无关 ABI，因此 Qt 插件继续通过已有的文件、菜单、Buffer 和
Scintilla 服务完成等价行为，不复制任意原生窗口消息入口。

## 验证

- NppExec-qt CTest：2/2 通过。
- 宿主全量 CTest：47/47 通过。
- 将新 DLL 安装到宿主插件目录后，以隔离配置启动；实例运行稳定，无启动崩溃。
