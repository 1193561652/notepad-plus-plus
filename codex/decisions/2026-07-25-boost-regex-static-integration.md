# Boost.Regex 静态集成决策

日期：2026-07-25

## 决策

- 保持与 Notepad++ v8.4.6 相同的结构关系：Scintilla 通过
  `SCI_OWNREGEX` 的工厂接口使用 `BoostRegExSearch.cxx`。
- QScintilla、其内嵌 Scintilla、Notepad++ Boost.Regex 适配层和精简
  Boost 1.78 头文件编译为一个静态库，再由 Qt 主程序链接。
- 不链接系统 Boost.Regex 动态库，不修改只读参考目录
  `third_party/qscintilla/`。
- Windows 使用 `.a` 或 `.lib`，Linux/macOS 使用平台原生静态归档；
  CMake 负责发现 Qt qmake 与匹配的构建工具。

## 兼容层

Notepad++ v8.4.6 的适配器面向较新的 Scintilla。主线副本只做以下薄适配：

- `Scintilla::Internal` 映射到 QScintilla 2.13.3 的 `Scintilla` 命名空间。
- `FindOption` 映射为旧接口的 `int flags`。
- 强类型 modification/status 与 UTF 转换调用映射为旧版接口。

Boost 搜索、反向搜索、空匹配和替换算法保持原版实现。

## 范围

当前文档、选区、所有打开文档、Find/Replace in Files 的用户正则路径
统一使用 Scintilla Boost.Regex。`QRegularExpression` 仍可用于文件过滤器、
配置解析等非用户搜索用途。

大文件模式仍是独立待办，不因本决策自动完成。
