# Notepad++ for Qt

本仓库在 Notepad++ v8.4.6 Git 历史上维护 Qt 跨平台移植。当前分支的仓库根目录
是唯一开发主线；原版行为通过 Git tag `v8.4.6` 查阅，不再依赖仓库外的旧工作区。

## 项目目标

- 在保持 Notepad++ 用户体验的前提下实现跨平台版本。
- 优先保持行为一致，而不是代码逐行一致。
- 不重新设计编辑器产品，保持长期可维护的 Qt 架构。
- 功能移植应先分析，再限定独立修改范围并小步验证。

## 仓库结构

- `src/`：Qt 主程序、编辑器、配置、对话框和平台适配代码。
- `resources/`：默认 XML、语言、图标和 Function List 资源。
- `tests/`：纯逻辑、配置语料、编辑器行为和 UI 运行测试。
- `third_party/scintilla/`：与 v8.4.6 对齐的 Scintilla 5.3.0 与 Qt 平台层。
- `third_party/boostregex/`：Notepad++ Boost.Regex 适配层和精简 Boost。
- `third_party/lexilla/`：独立静态 Lexilla、内建 lexer、lexlib 与原版
  v8.4.6 `LexUser.cxx`。
- `codex/`：代码知识库、索引、分析、决策和验证记录。

原版源码通过当前仓库历史读取，例如：

```bash
git show v8.4.6:PowerEditor/src/Notepad_plus.cpp
git show v8.4.6:PowerEditor/src/ScintillaComponent/FindReplaceDlg.cpp
git show v8.4.6:PowerEditor/src/Parameters.cpp
```

需要同时浏览多个原版文件时，可以为 `v8.4.6` 创建临时只读 worktree，但不得把
该 worktree 内容混入当前分支。

## 技术栈

- C++17
- Qt 5 / Qt Widgets
- CMake
- 自建静态 Scintilla 5 Qt 平台层
- Notepad++ Boost.Regex 后端

## 设计原则

- 以最终行为为准，不机械翻译 Win32 API。
- 先理解原始设计目的，再用 Qt 或标准 C++ 实现相同行为。
- 优先保留业务逻辑，仅替换平台相关实现。
- **原版逻辑和代码结构是移植基线，不只是参考。** 修改前必须核验 v8.4.6 中对应
  功能的所有者、管理类、模块边界、初始化/销毁顺序和主要调用方向。
- 原版已有明确管理类或子系统时，Qt 版必须保留对应职责边界、类层次和入口命名；
  Qt 控件或框架能力只能作为该边界内部的实现手段，不能据此把职责摊入
  `MainWindow` 或其他调用者。
- 允许因平台差异调整底层实现和数据类型，但默认保持原版业务流程、状态所有权、
  生命周期和调用关系。不能以“Qt 更方便”为理由主动扁平化原版结构。
- 确实需要偏离原版结构时，必须在修改前记录原版结构、候选方案、偏离理由、兼容
  影响和验证方法，并获得用户确认；未确认时采用最接近原版的方案。
- 平台相关代码统一封装，不污染业务层。
- 修改必须局部、可验证并降低回归风险。

### 结构一致性检查

- 修改前从原版源码列出相关核心类和调用链，并检查 `codex/` 是否已有对应索引。
- 设计 Qt 实现时逐项映射原版的类、职责、状态、入口和生命周期；无法一一映射的项
  必须显式说明。
- 代码审核时检查是否把原版管理类职责直接写进 `MainWindow`、对话框或平台适配器。
- 修改后更新知识库，记录保留了哪些原版结构、底层替换了什么以及仍存在的差异。

## UI 约束

- 菜单、工具栏、快捷键、对话框、状态栏、停靠窗口和工作流尽量对齐原版。
- 尊重原版动态初始化、布局和配置驱动逻辑。
- Qt 控件只是实现手段，不因 Qt 主动改变交互方式。
- 可本地化静态控件必须设置稳定 `objectName`，并同步更新目标语言 XML。

## 配置兼容

- 完全兼容 Notepad++ 配置文件的读取和写入。
- 不修改 XML 结构，不自动升级配置。
- 保留未知节点、属性和用户数据。
- 保持 `plugins/`、`themes/`、`autoCompletion/`、`localization/` 等目录兼容。
- 配置读取、默认值和写回逻辑以 v8.4.6 原版行为为准。

## 插件边界

- 插件系统是当前开发主线之一；Windows 原版 ABI 采用经真实插件源码和行为验证的
  有限兼容，跨平台版本化 ABI 仍是长期目标。
- 动态库扩展名按平台选择：Windows `.dll`、Linux `.so`、macOS `.dylib`。
- 插件路径、加载和动态库调用不得进入业务层。
- `src/Win32PluginSystem/` 专用于原版 Windows 插件 ABI 兼容，目录内允许直接
  使用 Windows API 和 Win32 类型。
- 顶层 CMake 只能在 `WIN32` 条件内加入该子目录；非 Windows 平台不得配置或
  编译其中源码，也不要求提供非 Windows 桩实现。
- Windows 兼容层源码只登记在该目录自己的 `CMakeLists.txt`，不得加入顶层
  通用源码列表。

## 编辑器核心

- 主线通过 CMake 构建 `third_party/scintilla/` 中与 v8.4.6 对齐的
  Scintilla 5.3.0 Qt 平台层 `npp-scintilla-qt`。
- `npp-scintilla-qt` 启用 `SCI_OWNREGEX` 并编入 Boost.Regex 适配层，但不内嵌
  lexer。
- Lexilla 由 CMake 独立构建为 `npp-lexilla` 静态库；应用调用
  `CreateLexer()`，通过 `SCI_SETILEXER` 将实例交给 Scintilla。
- 正则修改必须验证当前文档、打开文档和文件范围均走 Scintilla 语义，不得使用
  `QRegularExpression` 代替用户搜索引擎。
- 修改第三方源码应仅限版本适配或主线集成所必需的局部变更。

## 构建与验证

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

- 每个独立功能完成后至少执行一次针对性验证。
- 阶段开发按风险决定是否执行完整构建，提交前应执行完整测试。
- 构建目录和运行时配置不得加入 Git。

## 代码知识库与索引

- 修改前优先阅读 `codex/` 中已有模块、调用关系和行为分析。
- 索引缺失时，在核验真实源码与原版 tag 后补充。
- 修改代码后同步更新受影响的 `codex/` 索引、功能或变更记录。
- 知识库用于辅助理解，不能替代真实源码和调用关系核验。

## AI 开发原则

- 必须理解最终行为，不能只做语法转换。
- 修改前阅读相关源码、配置和调用关系。
- 不确定原版行为时，优先使用 `git show v8.4.6:<path>` 核验。
- 所有代码修改由 Codex 最终确认、应用并验证。
