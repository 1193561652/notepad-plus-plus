# 高级编辑、打印、宏与 UDL

## 实现入口

- `./src/MainWindow.h|cpp`
- `./src/Parameters.h|cpp`
- `./src/ScintillaComponent/UserDefinedLexer.h|cpp`
- `./src/ScintillaComponent/ScintillaEditView.h|cpp`

## 当前能力

- 宏：使用 `QsciMacro` 录制、停止、回放，并支持 `.macro` 文件导入导出。录制对象独立保存在主窗口，切换标签后仍可正确停止。
- 打印：使用 Qt PrintSupport 和 `QsciPrinter`，支持打印对话框与直接打印。
- 列编辑：在选中行范围或全文行范围的同一列插入文本或递增数字，整体作为一次撤销操作。
- 行处理：字典升降序、忽略大小写升降序、反转行序、删除空行/空白行、首尾去空白、TAB 转空格。
- UDL：只读解析 `userDefineLang.xml`，识别名称、扩展名、大小写、四组关键字、注释、操作符和样式；可从 Language 菜单选择，并可按扩展名自动应用。

## 行为边界

- 排序当前覆盖高频文本排序，尚未实现整数/小数/随机排序和矩形选区列范围排序。
- 列编辑未实现原版前导零、重复、二进制/八进制/十六进制等完整选项。
- UDL lexer 是轻量实现；多行注释跨增量样式区间、嵌套分隔符、折叠规则和所有 UDL 版本语义尚未完全对齐。
- UDL XML 不写回，不修改结构，不执行自动升级。
- 宏尚未把全部 Notepad++ 菜单命令映射到原版 `shortcuts.xml` Action 序列。

## 2026-07-26 P1 更新

前述“轻量 UDL、只读 XML、缺少整数/小数/随机排序和列编辑选项”已经过期。
当前状态以 `codex/features/p1-compatibility.md` 为准。

## 2026-07-26 P0 更新

- `shortcuts.xml` 已支持 InternalCommands、ScintillaKeys、NextKey、Macros 和 UserDefinedCommands。
- Qt 主线中已经实现的 QAction 已注册原版 Notepad++ command ID，并可从 Shortcut Mapper 保守写回。
- 配置宏可按原顺序执行 Scintilla 数值命令、字符串命令和已映射菜单命令（type 0、1、2）。
- 上述旧结论仅代表 2026-07-22 阶段状态；当前剩余边界是未移植菜单命令没有可执行 QAction，以及专用保存查找状态仍需真实语料核验。

## 验证

- `cmake ..` 配置通过。
- `cmake --build .` 构建通过。
- Windows 平台隐藏窗口启动 3 秒保持运行，随后由测试脚本终止。
