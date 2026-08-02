# 插件移植阶段 0/1 完成记录

日期：2026-08-01

## 范围

- 完成阶段 0：按当前依赖重排插件移植路线，并把已完成的 Plugin Admin/包管理
  从未来开发阶段中移出。
- 完成阶段 1：以 Notepad++ v8.4.6 的 x86 插件列表为唯一身份和版本基线，整理
  169 项插件的源码证据、API、依赖、Win32/UI、线程/运行时和双代理风险。

## 结论

- 169/169 项已建立记录。
- 62 项具有对应版本源码证据；107 项证据不足，保持 `U`，未反汇编。
- 代理初评：A 13、B 23、C 25、D 1、U 107。
- 重要度：高 20、中 94、低 55。
- `SCI_GETDIRECTFUNCTION`/`SCI_GETDIRECTPOINTER`、窗口 subclass/hook、复杂 Dock、
  特殊文档视图和脚本宿主不纳入有限代理的默认能力。

## 产物与验证

- 统一矩阵：`codex/analysis/plugins-v846/api-dependency-matrix.md`。
- 自动校验：`tests/PluginInvestigationTests.cpp`。
- CMake 目标与 CTest：`plugin-investigation-tests`。
- Debug 全目标构建成功，CTest `31/31` 通过。

## 边界

本次没有加载真实插件 DLL，也没有实现 Windows ABI。下一阶段必须先完成 PE 架构、
六导出、依赖预检、加载日志、异常恢复和资源回滚，再允许首个真实插件进入验证。
