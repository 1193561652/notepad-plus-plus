# 2026-08-08 mimeTools 2.8 完整兼容

## 范围

完成官方 `mimeTools 2.8 x64` 在 Windows Qt 移植版中的用户可用兼容：加载、菜单、
所有命令、主/副永久视图、空选区和 About。

## 实现

- `Win32PluginManager` 公开 DLL `FuncItem` 的名称和分隔符信息；`MainWindow` 在插件
  加载后将命令插入 Plugins 菜单，保留原顺序和分隔符。
- 自动测试通过实际 `QAction` 触发 13 个转换，而非直接调用管理器测试入口。
- URL 命令保留真实 DLL 调用。仅其 `SCI_GETSELTEXT` 的长度查询按 DLL 所假定的 NUL
  长度返回，防止 URL 源码的临时输出缓冲区少分配 2 字节。
- SAML Decode 和 About 分别替代官方 DLL 的两个 Qt 宿主崩溃点。SAML 使用 raw-DEFLATE
  zlib 解压，沿用原插件的 200000 字节限制、错误提示和 target/selection 语义；About
  使用非模态 Qt 对话框显示官方资源信息。

## 验证

- CMake Debug 构建 `ui-parity-capture` 成功。
- CTest `mimetools-managed-install` 成功：固定官方 ZIP 通过 updater 安装。
- CTest `ui-localization-runtime-100` 成功：菜单表、全部 13 个转换、空选区、About，
  以及主/副视图 Base64 路径均通过。

## 仍不代表的范围

- 本次只完成 mimeTools 2.8 x64；不因此宣布其他 Windows 插件兼容。
- `mimeTools` 文件夹白名单仍是当前真实语料隔离措施。移除白名单需先完成通用安全
  加载、消息白名单和逐插件验证。
