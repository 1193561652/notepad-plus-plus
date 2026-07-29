# 2026-07-25 原版配置加载兼容修复

## 根因

移植版语言选择逻辑把 `%APPDATA%/Notepad++/nativeLang.xml` 写成仅含
`lang="zh_CN"` 的短标记。Notepad++ v8.4.6 要求该文件是完整语言包，
因此原版启动加载时会报错。

此外，Qt 私有的编辑器、深色模式和窗口状态曾写入原版 `config.xml`，
不符合“不得修改原版 XML 结构”的项目约束。

## 修改

- 内置原版 v8.4.6 官方简体中文语言包。
- 语言切换改为完整复制语言包，与原版 `LocalizationSwitcher` 行为一致。
- 共享 `%APPDATA%` 时优先采用机器上已安装原版自带的语言包，避免不同原版
  版本之间的语言包版本检查失败；无安装来源时回退到内置 v8.4.6。
- 自动迁移旧短标记语言文件。
- Qt 私有字段迁移到独立 `qtState.ini`。
- 写回 `config.xml` 时移除旧 Qt 私有 GUIConfig。

## 验证

- 旧 102 字节语言标记自动迁移为官方 101722 字节语言包。
- 迁移结果包含 `filename="chineseSimplified.xml"` 和 `version="8.4.6"`。
- 官方资源与迁移结果 SHA-256 一致。
- `config.xml` 中 Qt 私有节点数量为 0。
- 测试注入的未知节点、未知属性和嵌套子节点完整保留。
- `config.xml`、`session.xml`、`nativeLang.xml`、`shortcuts.xml` 均通过 XML 解析。
- 全量构建及 2 个自动化测试通过。

用户真实 `nativeLang.xml` 已备份为
`nativeLang.xml.qt-incompatible-20260725.bak` 后修复。

后续检查发现当前安装原版实际为 Notepad++ v7.81，其随附简体中文包版本为
7.7.2。共享 `nativeLang.xml` 已再次备份为
`nativeLang.xml.v846-20260725.bak`，并切换为该安装版随附语言包。
