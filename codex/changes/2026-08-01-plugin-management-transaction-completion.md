# 插件管理事务闭环

日期：2026-08-01

## 修改范围

- 抽出 `PluginUpdateExecutor`，让退出后更新逻辑可独立测试。
- 修复 Windows PowerShell ZIP 脚本参数传递。
- 增加全批次预检、目标平台动态库检查、事务替换和失败回滚。
- 更新计划在主程序与更新器交接时使用内容 SHA-256 防篡改校验。
- Windows 不可写插件目录通过 UAC 启动更新器，并以桌面普通用户 token 重启主程序。
- Linux/macOS 不可写插件目录在主程序退出前明确失败。

## 行为

安装和更新必须先完成下载、SHA-256、ZIP 路径、符号链接和平台二进制检查。
所有操作预检成功后才在 `plugins/` 内提交；旧目录先移动到事务备份，任一提交
失败会逆序恢复。成功后写 `.npp-package.json` 并删除更新计划。

## 验证

- `plugin-admin-tests`：通过。
- `plugin-updater-tests`：通过，覆盖真实 stored ZIP 和计划篡改拒绝。
- `notepadpp-qt`、`npp-plugin-updater`：Windows/MinGW 构建通过。
- `ENABLE_PLUGIN_SYSTEM=ON` 全量 CTest `30/30` 通过；隔离配置启动冒烟通过。

## 延期边界

原版 Windows 插件 ABI、跨平台稳定 ABI、非 Windows 原生清单和发布签名策略
仍是独立插件运行/生态任务，不属于本次包管理闭环。
