# 插件管理验证记录

首次验证：2026-07-30

事务闭环复验：2026-08-01

## 当前自动化结果

Windows、Qt 5、MinGW，使用 `ENABLE_PLUGIN_SYSTEM=ON`：

```powershell
cmake -S . -B build -DBUILD_TESTING=ON -DENABLE_PLUGIN_SYSTEM=ON
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

结果：

- `notepadpp-qt`、`npp-plugin-updater` 和全部测试目标构建成功。
- CTest `30/30` 通过。
- 隔离 `-settingsDir` 的主程序启动冒烟通过，进程保持运行 3 秒后由测试结束。
- `plugin-admin-tests` 覆盖三架构内置清单、版本/兼容区间、平台路径、插件发现、
  安装收据、四类状态和更新计划往返/路径拒绝。
- `plugin-updater-tests` 使用真实 ZIP 覆盖安装、更新、卸载、收据、错误 SHA-256、
  缺失平台二进制、批次预检不部分修改以及 `..` 目录穿越拒绝。
- 更新器子进程测试确认计划内容哈希不匹配时以错误码 4 拒绝且不执行操作。
- Windows PowerShell `-File` 列表与解压路径由上述真实 ZIP 测试实际执行。

## 已验证行为边界

- Windows：`plugins/<name>/<name>.dll`。
- Linux：`plugins/<name>/linux/<name>.so`。
- macOS：`plugins/<name>/macos/<name>.dylib`。
- `plugins/Config` 不参与插件发现或更新目标。
- 所有包预检成功后才提交；提交使用 staging/backup，失败时回滚。
- 主程序取消退出时删除待执行计划，正常退出后才启动更新器。
- 程序目录不可写时，Windows 走 UAC 更新器；Linux/macOS 在退出前明确失败。

## 发布环境人工矩阵

以下项目涉及外部状态，代码已具备路径，但仍需在发布候选包中人工确认：

- Windows UAC 确认/取消、受保护安装目录写入和普通权限重启。
- 通过代理、TLS 和断网环境下载真实清单插件包。
- Plugin Admin 四页在目标语言、浅色/深色和 100%/150% DPI 下的最终视觉检查。
- x86、x64、ARM64 发布物各自选择正确清单和插件包。
- Linux/macOS 获得原生插件清单后的真实 `.so` / `.dylib` 包安装。
- macOS quarantine、签名和公证策略。

这些是发布/生态验证项；原版插件 ABI 与跨平台稳定 ABI 仍属于延期开发范围。
