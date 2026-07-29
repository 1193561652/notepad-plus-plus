# 命令行兼容验证

验证日期：2026-07-28

## 自动化

- Debug 全量构建：通过。
- CTest：`19/19` 通过。
- 新增 `command-line-options-tests`：
  - 布尔、字符串和数值参数；
  - 相对路径和配置目录绝对化；
  - 递归通配符；
  - `-notepadStyleCmdline`；
  - 本地化代码映射；
  - 单实例 JSON 序列化往返。

## 真实进程

验证使用隔离目录：

`./build/cli-runtime-validation/`

1. 使用独立 `-settingsDir` 和 `-export=functionList` 打开测试 C++ 文件。
2. 进程退出码为 0。
3. `<source>.result.json` 存在，包含 `alpha`、`beta` 两个条目。
4. 启动首实例后，以相同配置目录再次调用并携带 `-n2 -c3`。
5. 第二个进程通过 IPC 返回，退出码为 0，未形成第二个长期运行实例。

## 构建修复

单实例 IPC 新增 Qt Network 依赖。CMake 现在通过 `$<TARGET_FILE:Qt5::Network>`
把与当前 Debug/Release 配置匹配的运行库复制到程序输出目录，避免硬编码 Qt 路径。
