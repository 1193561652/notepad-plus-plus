# win32-plugins Qt 适配及宿主命令状态/通知补齐

## 原版职责核验

已读取 v8.4.6 `PowerEditor/src/MISC/PluginsManager/PluginsManager.cpp`：
插件拥有命令及开关逻辑，管理器负责导出加载、命令表和通知，主窗口承载菜单。
本次继续使用 Qt `PluginManager`/`PluginHostServices` 边界，没有把插件算法并入主程序。

## 对应修改

- `PluginManager` 可选解析 `nppGetCommandState`；查询是否可勾选及当前勾选状态。
  未提供导出的旧插件继续使用原初始化勾选值。
- `MainWindow` 仅把空命令呈现为菜单分隔符，在菜单显示前及执行后同步状态。
  不持有任何插件业务开关。
- `NppPluginNotification` 尾部追加可选 `lines_added`，新增 ZOOM 通知代码。
  `PluginManager::notifyPlugins` 接收并传递行数变化，默认参数保持既有调用有效。
  两个编辑器的既有通知连接转发 SCN_ZOOM/linesAdded，避免弹性制表符丢失原增量路径。
- 旧通知前缀、宿主结构和命令数组布局不变；新通知尾字段必须检查 struct_size。

## 独立插件

`../win32-plugins/CMakeLists.txt` 是独立构建入口，Qt 源码在各插件 `qt/` 下。
原生算法可共享时直接编译或抽成原 Win32 与 Qt 都引用的头；C# 插件按原流程移植。
实现状态、原始逻辑来源和边界修复详见同级 `win32-plugins/qt/PORTING_STATUS.md`。
用户确认 NppPlugins 七个仅有 DLL 的插件跳过，未制作占位插件。

## 验证记录

Windows x64 / Qt 5.12.12 / MinGW 7.3；插件动态库 ABI、原始核心和真实 Scintilla 测试。
BracketCheck 弹窗超时定位为 offscreen/minimal 后端；实际 windows 后端可以正常关闭，
无需修改括号算法。跨平台插件宿主测试新增实时勾选状态及负 lines_added 负载断言。
2026-09-08：宿主构建成功；独立插件 17 个，36/36 项测试通过。
宿主完整 CTest 有部分 Qt 进程超时，尚未定位；不能计为全套通过。
复制到独立测试目录后的跨平台 ABI 等目标可退出成功，此现象仍需跟进。
Linux/macOS 尚未运行。

## 提交与恢复

公共构建入口、适配器和测试归档到 `tools/win32-plugins-qt/workspace/`。
`repositories.json` 固定各插件提交；`prepare.py` 恢复同级插件工作区且不覆盖不同文件或重置仓库。
插件业务代码仍位于各自独立仓库，没有并入宿主。

提交前插件回归 36/36 通过。宿主完整套件使用每项 3 秒超时、4 路执行做阻塞复核，
51 项中仅 1 项通过，其余为超时或依赖项未运行；这不是宿主完整回归通过记录。
此前较长超时同样发生 Qt 进程停滞，原因仍未定位。

## 插件分支修正

17 个插件的 Qt 移植均使用原有 `qt-port` 分支。先前误推到 `master` 的移植提交通过追加 revert 恢复原内容，保留公开历史；`qt-port` 保留原移植提交。
版本清单记录分支名，恢复脚本检查现有仓库分支并让新克隆落在 `qt-port`，防止后续再次提交到 `master`。
