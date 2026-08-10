# 主控制器结构拆分回归验证

## 范围

- Release 全目标编译与链接。
- 全部 38 项 CTest，包括结构所有权、UI 本地化运行捕获、无插件启动、插件恢复与回滚、
  配置语料、Scintilla/Lexilla、大文件和 Boost.Regex。
- 拆分前后 `MainWindow` 成员定义集合静态比对。

## 结果

- Release 构建通过。
- CTest 38/38 通过。
- 成员定义无缺失、无重复。

## 说明

本批次是纯结构调整，没有新增人工 UI 截图。现有自动 UI 运行捕获已包含在 CTest 中。
