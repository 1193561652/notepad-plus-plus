# 插件清单 submodule

## 决策

- 官方 `notepad-plus-plus/nppPluginList` 的 `master` 和 tags 保留在用户 fork 中。
- fork 新增 orphan 分支 `qt-catalog`，只保存 Windows、Linux、macOS 各架构 JSON，
  不保存或编译原版 DLL、验证器和 CI 工程。
- 主项目通过 `third_party/nppPluginList` submodule 固定清单 commit；运行时不联网
  更新清单，也不为内嵌 JSON 增加独立签名。

## 构建映射

- Windows：`catalog/windows/pl.x86.json`、`pl.x64.json`、`pl.arm64.json`。
- Linux：`catalog/linux/pl.x86.json`、`pl.x64.json`、`pl.arm64.json`。
- macOS：`catalog/macos/pl.x64.json`、`pl.arm64.json`。
- CMake 根据目标平台、指针宽度和处理器选择一份文件，通过生成的 QRC 统一映射到
  `:/pluginList/catalog.json`。`NPP_PLUGIN_CATALOG_FILE` 可用于发布测试或定制构建。

## 兼容和安全

- Windows JSON 初始内容固定为 v8.4.6 对应的 nppPluginList v1.5.4，避免本次结构
  调整改变可用插件集合。
- Linux/macOS 初始清单为空，后续插件发布只需更新 `qt-catalog` 并在主项目推进
  submodule commit。
- 清单不从网络更新；插件 ZIP 仍执行 SHA-256、路径、符号链接、平台二进制和事务
  预检。

## 验证

- CMake 配置输出实际选中的 JSON 路径。
- `plugin-admin-tests` 读取统一资源路径并验证目标平台清单。
- `plugin-investigation-tests` 改为读取 submodule 中固定的 Windows x86 v1.5.4 清单。
