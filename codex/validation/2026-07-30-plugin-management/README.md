# 插件管理首轮移植验证

日期：2026-07-30

## 已执行

环境：Ubuntu，Qt 5，CMake，内置插件清单为空占位。

```bash
cmake -S . -B build-ubuntu -DBUILD_TESTING=ON
cmake --build build-ubuntu --target \
  plugin-admin-tests npp-plugin-updater notepadpp-qt -j 4
ctest --test-dir build-ubuntu -R plugin-admin-tests --output-on-failure
cmake --build build-ubuntu --target \
  parameters-style-udl-tests ui-parity-capture -j 4
ctest --test-dir build-ubuntu \
  -R 'plugin-admin-tests|parameters-style-udl-tests|ui-localization-runtime-100' \
  --output-on-failure
```

结果：

- CMake 配置成功。
- `plugin-admin-tests`、`npp-plugin-updater`、`notepadpp-qt` 编译成功。
- `plugin-admin-tests` 通过。
- 管理测试、配置/语言 XML 测试和 100% 缩放本地化运行测试共 3 项通过。
- 已覆盖版本比较、兼容区间、旧版本映射、坏条目跳过、空内置清单、三平台路径
  规则、本地插件发现、安装收据、更新/不兼容分类、更新计划往返和目录穿越拒绝。
- 已验证编译进资源的 v1.5.4 x86、x64、ARM64 JSON 全部可读，解析条目数分别为
  169、134、20，无条目被当前解析器丢弃。

未执行完整 CTest、真实网络下载、真实插件包安装及人工 GUI 操作；本轮不据此
声明完整运行验收。

## Windows 待验证

- [ ] MSVC 与项目支持的 MinGW 配置均能编译主程序、管理测试和更新器。
- [ ] `.dll` 只从 `plugins/<name>/<name>.dll` 发现，`Config` 和错误命名忽略。
- [ ] 手工安装 DLL 的文件版本读取正确；无版本资源时显示 Unknown 且不崩溃。
- [ ] Plugins 菜单、四个页签、搜索、描述、勾选、按钮状态和中文本地化正确。
- [ ] 安装、更新、卸载确认会先完成文档保存/退出流程，再启动更新器。
- [ ] 用户取消保存或退出时删除待执行计划，之后正常退出不会意外执行旧操作。
- [ ] 多实例仍在使用同一插件目录时不会静默替换正在加载的插件。
- [ ] 更新器能等待主进程退出，并能处理含空格、中文和长路径的程序/插件目录。
- [ ] PowerShell 能列出和解压真实插件 ZIP；绝对路径、`..` 和符号链接被拒绝。
- [ ] HTTP(S) 下载、重定向、超时、代理、TLS 失败和断网错误可诊断。
- [ ] 正确 SHA-256 安装成功；错误 SHA-256 不修改现有插件。
- [ ] 更新在下载、哈希和解压预检完成后清理旧目录；卸载只删除目标插件目录。
- [ ] `.npp-package.json` 收据的版本和哈希正确，重启后四个列表刷新正确。
- [ ] 更新器完成后重启主程序；失败时的可见错误和恢复路径可接受。
- [ ] 实现普通安装目录不可写时的 UAC/权限提升启动与非特权重启，再完成验证。
- [ ] 便携模式、普通配置目录和 `-settingsDir` 不改变插件二进制根目录，更新计划
  写入正确的用户配置 `plugins/Config`。
- [ ] `-noPlugin` 仍只影响加载，不影响打开 Plugin Admin 所需的管理代码。
- [ ] 分别验证 x86、x64、ARM64 构建选择正确清单，下载包和进程架构匹配。
- [ ] 与 v8.4.6 官方发行包内 `nppPluginList.dll` JSON 资源逐字节比较。

## Linux/macOS 后续验证

- [ ] `unzip` 缺失、版本差异和发布包依赖处理有明确结果。
- [ ] Linux 真实包只安装并发现 `linux/<name>.so`。
- [ ] macOS 构建通过，真实包只安装并发现 `macos/<name>.dylib`。
- [ ] 不可写程序目录的提权或明确失败恢复流程完成。
- [ ] macOS quarantine、签名和公证要求在进入发布阶段前另行决策。
