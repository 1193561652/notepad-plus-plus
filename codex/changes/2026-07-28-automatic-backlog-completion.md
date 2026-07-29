# 无人工介入待办完成

日期：2026-07-28

## 代码

- 新增 `ClosedFileHistory`，接入恢复最近关闭文件菜单、快捷键和原版命令 ID。
- 新增 `FunctionListParser`，加载和执行原版 v8.4.6 Function List XML。
- 随程序部署 34 份原版 Function List 规则，配置目录可提供同名覆盖。
- 配置和会话写入入口不再忽略配置目录创建失败；Qt state 和 XML 保存结果向上传递。
- 补充恢复关闭文件的简体中文资源，并移除工具栏重复的开始录制动作。

## 测试

- 新增 `closed-file-history-tests`。
- 新增 `function-list-parser-tests`。
- `P1RuntimeCapture` 增加恢复最近关闭文档的可执行检查，但本轮未启动该 GUI 工具。
- 完整 CTest 25/25 通过。
