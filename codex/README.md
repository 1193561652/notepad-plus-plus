# Codex 代码知识库与代码索引

本目录用于存放 Codex 在理解和修改本项目代码时维护的代码知识库、模块索引、调用关系和行为分析。这里的内容用于辅助后续开发决策，但不能替代对真实源码、配置文件和调用关系的核验。

## 目录结构

建议按以下结构组织：

```text
codex/
  README.md
  index/
    repository.md
    modules.md
    build.md
  modules/
    notepad-plus-plus/
  features/
    file-management.md
    editor-buffer.md
    configuration.md
    plugin-system.md
    ui-menus-toolbars.md
    search-replace.md
  analysis/
    original-vs-qt-feature-comparison.md
    feature-points-and-gaps.md
    logic-differences.md
    stage-1-xml-configuration.md
  decisions/
    YYYY-MM-DD-topic.md
  changes/
    YYYY-MM-DD-change-summary.md
  cache/
    latest.md
    YYYY-MM-DD-analysis.md
```

各目录用途：

- `index/`：全局索引，记录仓库结构、主要模块、构建方式和高层调用关系。
- `modules/`：按代码目录记录模块职责、关键类、关键文件和依赖关系。
- `features/`：按功能记录原版行为、Qt 实现位置、配置影响和测试关注点。
- `analysis/`：记录跨模块分析、原版与 Qt 版功能对照、逻辑差异和阶段性结论。
- `decisions/`：记录重要移植决策、设计取舍和平台适配原则。
- `changes/`：记录代码修改后需要同步给后续 Codex 使用的变更摘要。
- `cache/`：记录最近一次或阶段性的本地分析缓存，便于后续快速恢复上下文。

## 文件内容建议

### 全局索引

`index/repository.md` 建议包含：

- 仓库主要目录说明。
- 主开发目标目录。
- 参考源码目录。
- 不应修改或通常不需要构建的目录。

`index/modules.md` 建议包含：

- 核心模块列表。
- 模块职责。
- 模块之间的主要依赖关系。
- 需要优先理解的入口文件。

`index/build.md` 建议包含：

- Qt 移植版本的构建命令。
- Qt、CMake、QScintilla 的查找方式。
- 已知构建限制和常见问题。

### 模块知识

`modules/` 下的模块说明建议包含：

- 模块职责。
- 关键类和关键文件。
- 主要入口函数。
- 与原版 Notepad++ 的对应关系。
- 与 Qt / QScintilla 的适配方式。
- 修改该模块时的风险点。

### 功能索引

`features/` 下的功能说明建议包含：

- 用户可见行为。
- 原版 Notepad++ 行为来源。
- Qt 版本实现位置。
- 涉及的配置文件和资源目录。
- 相关菜单、快捷键和 UI 元素。
- 修改后的验证方式。

### 决策记录

`decisions/` 下的文件用于记录重要设计决策，建议格式：

```markdown
# 决策标题

## 背景

## 结论

## 原因

## 影响范围

## 后续注意事项
```

### 变更摘要

`changes/` 下的文件用于记录一次代码修改后对知识库有价值的信息，建议格式：

```markdown
# YYYY-MM-DD 变更标题

## 修改范围

## 行为变化

## 影响模块

## 验证方式

## 需要同步更新的索引
```

## 构建代码知识库的方法

构建知识库时应遵循“先行为、后实现、再索引”的顺序：

1. 明确要理解的功能或模块。
2. 先阅读 `AGENTS.md` 中的项目约束。
3. 查阅 `codex/` 中已有索引，确认是否已有相关分析。
4. 阅读 `v8.4.6:` 中对应原版实现，理解原始行为和设计目的。
5. 阅读 `sample/notepadppqt2/` 中可参考的 Qt 实现，但不要直接照搬。
6. 阅读 `./` 中当前实现，确认真实代码状态。
7. 将模块职责、调用关系、行为差异、风险点写入对应索引文件。
8. 如发现已有索引过期，应同步修正。

## 构建代码索引的方法

构建代码索引时优先使用快速文本搜索和静态阅读：

```bash
rg --files
rg "class ClassName"
rg "functionName"
rg "SIGNAL|SLOT|connect"
rg "QAction|QMenu|QToolBar|QSettings|QDomDocument"
```

索引时建议记录：

- 文件路径。
- 类、函数、枚举和常量名称。
- 调用方和被调用方。
- Qt signal / slot 连接关系。
- 配置文件读写位置。
- UI 初始化位置。
- 平台相关代码边界。
- 与原版 Notepad++ 的对应源码位置。

索引内容应尽量稳定，避免记录临时推测。若必须记录推测，应明确标注为“待验证”。

## 修改代码前的使用流程

修改代码前应执行：

1. 阅读相关 `codex/` 索引。
2. 核对索引中提到的真实源码位置。
3. 确认原版 Notepad++ 的目标行为。
4. 确认 Qt 当前实现与原版行为的差异。
5. 只修改与当前目标直接相关的代码。

## 修改代码后的更新流程

修改代码后应执行：

1. 更新受影响的模块索引。
2. 更新受影响的功能索引。
3. 如产生新的设计取舍，新增 `decisions/` 记录。
4. 如本次修改影响后续理解，新增 `changes/` 摘要。
5. 尽量记录验证方式，例如构建命令、手动验证步骤或静态检查结果。

## 维护原则

- 知识库内容必须服务于后续代码理解和变更决策。
- 不记录与项目无关的泛泛说明。
- 不用知识库覆盖源码事实。
- 不确定的信息必须标注待验证。
- 代码修改后，索引应尽量保持同步。
- 本地缓存应定期刷新，`cache/latest.md` 始终指向最近一次有效分析摘要。
