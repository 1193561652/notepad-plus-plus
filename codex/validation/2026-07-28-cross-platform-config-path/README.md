# 跨平台配置路径验证

日期：2026-07-28

## 自动测试

- 完整构建成功。
- CTest 23/23 通过。
- `config-path-resolver-tests` 覆盖命令行目录优先级、便携模式、Windows
  默认路径、无效目录拒绝、目录创建和写权限验证。
- `command-line-options-tests` 覆盖 `-settingsDir=...`、
  `--settings-dir ...`、`--settings-dir=~/...`。

## 真实进程验证

使用本目录作为 `--settings-dir` 启动自动关闭的程序实例：

- 进程退出码：0
- 原版兼容 XML、`qtState.ini` 和全部兼容子目录均生成在本目录。
- 没有回落到系统默认配置目录。

