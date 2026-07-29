# 跨平台配置路径完善

日期：2026-07-28

## 目标

统一 Windows、Linux 和 macOS 的配置目录决策，确保显式指定的配置目录在
任何配置文件加载前生效，并让所有配置、会话和兼容资源使用同一个根目录。

## 实现

- 新增 `src/MISC/ConfigPathResolver.*`，隔离平台路径策略。
- 配置目录优先级：
  1. `-settingsDir=...`、`--settings-dir=...` 或
     `--settings-dir ...`。
  2. 可执行文件目录存在 `doLocalConf.xml` 时使用便携模式。
  3. 使用平台默认配置目录。
- Windows 默认目录保持原版兼容：`%APPDATA%\Notepad++`。
- Linux 使用 `QStandardPaths::AppConfigLocation`，通常为
  `$XDG_CONFIG_HOME/Notepad++` 或 `~/.config/Notepad++`。
- macOS 使用 `QStandardPaths::AppConfigLocation`，并保留
  `~/Library/Preferences/Notepad++` 回退。
- 支持 `~/...` 路径展开、原生分隔符转换、绝对路径清理及既有目录规范化。
- 显式目录必须已经存在、是目录且可写；无效时显示提示并忽略覆盖，
  与 v8.4.6 的 `-settingsDir` 行为一致。
- 默认目录允许创建；目录或兼容子目录创建失败时阻止继续加载并显示错误。
- 单实例服务名继续由规范化后的配置目录生成，不同配置根目录可独立运行。

## 覆盖范围

指定目录统一承载原版兼容 XML、`qtState.ini` 以及 `backup/`、`themes/`、
`localization/`、`autoCompletion/`、`nativeLang/`、`userDefineLangs/`、
`toolbarIcons/` 和 `plugins/`。
配置目录同时创建 `functionList/`，用于覆盖随程序部署的原版解析规则。

## 验证

- 新增 `config-path-resolver-tests`。
- 扩展 `command-line-options-tests`，覆盖跨平台参数别名和 `~` 展开。
- CTest：23/23 通过。
- 使用独立目录真实启动程序，退出码 0，所有配置文件和子目录均写入指定目录。
