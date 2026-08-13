# 2026-08-13 高优先级插件深层行为

## 宿主能力

- 跨平台 ABI v1 采用尾部追加，新增 Scintilla 消息、Buffer 路径、保存和菜单命令回调。
- 新增文件打开/保存/关闭、Buffer 激活、语言变化、文本修改、UI 更新和深色模式通知码。
- Scintilla 通知保留位置、长度、修改类型、更新标记和回调期间有效的 UTF-8 文本。
- ABI 测试覆盖新增回调和通知负载；宿主完整 CTest `47/47` 通过。

## 插件行为

- DSpellCheck：实时波浪线、防抖重检和用户词典恢复。
- Markdown：事件刷新、滚动同步和常用扩展块语义。
- Explorer：`Favorites.dat` 读写兼容和安全上下文菜单。
- NppExec：串行动作、异步等待、错误导航及常用 NPP/SCI/SEL/CON 命令。
- NppFTP：FIFO 队列、缓存绑定、保存上传和 SFTP 私钥认证。
- NPPTextFX2：补充原版高频转换、数制和插入命令。
- PythonScript：状态化编辑器模型及 Notepad 文件操作请求。

八插件独立 CTest 合计 `13/13` 通过。原版专属第三方运行时和 x86-only 功能另行记录，
不以不完整仿真实现冒充完全等价。
