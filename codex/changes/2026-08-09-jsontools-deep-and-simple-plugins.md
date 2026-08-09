# JsonTools 深层功能与简单插件适配

## 范围

- 保持 Windows 原版 DLL、六导出 ABI、三个兼容 HWND 和现有 DockingManager 架构不变。
- 深化 JsonTools 3.2.0 的 Settings、RemesPath、JSON Lines、YAML、树节点跳转和
  4 MB 树加载阈值验证。
- 增加官方 nppConverter 4.4.0 与 NppPluginDemo 4.2 x64 包，继续通过插件管理安装，
  仅对白名单中的已审核插件启用。

## JsonTools 结果

- Settings 对话框可打开，标题、OK/Cancel/Reset、属性网格和
  `Ctrl+Alt+Shift+S` 快捷键均由真实 DLL 提供。
- RemesPath 查询 `@.foo` 会把树缩小到查询结果；赋值
  `@.bar = \`changed\`` 会写回当前 Scintilla Buffer。
- JSON Lines 把逐行 JSON 解析为独立树根；JSON-to-YAML 在保留上游警告后新建文档并
  输出 YAML。
- 点击树中 `foo` 节点会通过 `SCI_GOTOLINE` 把编辑器光标移到对应源码行。
- 文件长度超过 4,000,000 字节时默认只建立直接子树；用户确认后才建立完整子树。
  自动测试使用 4,000,100 字节载荷验证 2 节点到 3 节点的切换。
- 重型 4 MB 矩阵只在 `ui-localization-runtime-100` 执行，其余 DPI/主题场景保留基础
  DLL、菜单、Dock 和格式化验证。

## 内置测试边界

JsonTools 3.2.0 的 `Run tests` 命令已正确注册，但上游
`JsonGrepperTests.cs` 无条件读取作者机器上的绝对目录
`C:\Users\mjols\Documents\csharp\JSONToolsPlugin\testfiles\small`，且
`DirectoryInfo.GetFiles()` 外没有异常处理。后续 benchmark 也包含作者绝对路径，但会自行
报告文件缺失。官方 DLL 没有路径参数，因此当前不能在普通机器上完整运行该菜单命令。

本轮没有伪造作者目录、截断命令或改写官方 DLL。解析器、JSON Lines、YAML、RemesPath
和树行为改由宿主真实 DLL 回归覆盖。若以后要修复菜单自身，只能选择维护 JsonTools 补丁版
DLL，或升级到已移除绝对路径的上游版本；两者都需要单独版本决策。

## 简单插件

- nppConverter 4.4.0：官方 x64 ZIP SHA-256
  `e7eafbac35d91572aa93c5fd6a8a957669b50d0160aa2e35e9e632e2ac6ab63d`。
  已验证 ASCII/HEX 双向转换、`converter.ini` 创建、Conversion Panel Dock，以及面板通过
  `SCI_ADDTEXT` 向当前编辑器插入字符。
- NppPluginDemo 4.2：官方 x64 ZIP SHA-256
  `afa33fe958a907c2b80c0e8ae26deaea533455a2a2826f30a9b46fba0ba59941`。
  已验证 16 个功能槽位及分隔符、Hello 新建文档、Demo Dock 和通过
  `SCI_ENSUREVISIBLE`/`SCI_GOTOLINE` 跳到第 3 行。
- Demo 中的会话文件示例、文件名枚举和自动闭合标签需要更宽的 NPPM/通知面，本轮不为
  演示命令扩大通用适配层，维持有限兼容等级 B。

## 验证

- `cmake --build build --target ui-parity-capture npp-plugin-updater -j 4`
- `ctest --test-dir build --output-on-failure`：37/37 通过。

