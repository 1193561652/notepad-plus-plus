# 2026-08-08 Qt DockingManager 结构恢复

## 原因

Qt 版此前直接在 `MainWindow` 创建和管理全部 `QDockWidget`，虽然基本行为可用，
但丢失了原版 `_dockingManager` 的所有权、模块边界和统一注册入口，不利于接入原版
插件 Dock。

## 改动

- 新增跨平台 `DockingManager`、`DockingData` 和四个 `DockingCont`。
- 全部 10 个内置 Dock 改为通过管理器注册和控制。
- 同侧可见 Dock 使用 Qt 标签化，对应原版一个 `DockingCont` 容纳多个 client。
- Dock 布局状态保存/恢复和四侧尺寸统一由管理器负责。
- 重新接通原版 `config.xml/DockingManager` 四个尺寸属性的读写。
- 强化 `AGENTS.md`：原版类、职责、生命周期和调用关系成为强制移植基线；结构
  偏离必须先记录并确认。

## 验证

- `docking-manager-tests` 覆盖注册、四侧区域、标签化、显示隐藏、区域迁移、尺寸和
  状态往返。
- `ui-parity-capture` 断言 MainWindow 的内置 Dock 均进入管理器索引。
- 原版/当前用户配置语料验证 DockingManager XML 可继续往返。
