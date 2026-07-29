# 2026-07-22 阶段一 XML 资源基础变更

## 修改范围

- `./resources/resources.qrc`
- `./resources/config.model.xml`
- `./resources/langs.model.xml`
- `./resources/shortcuts.xml`
- `./resources/contextMenu.xml`
- `./resources/toolbarIcons.xml`
- `./resources/userDefineLang.xml`
- `./src/Parameters.cpp`

## 行为变化

- 配置目录初始化时会创建 Notepad++ 兼容子目录骨架。
- `langs.xml` 缺失时可回退加载内嵌 `langs.model.xml`。
- `shortcuts.xml` 缺失时可回退加载内嵌 `shortcuts.xml`。
- 默认 context menu、toolbar icons、UDL XML 已作为资源进入 Qt 可执行程序，供后续功能接入。
- 首次缺少默认 XML 文件时，会从 Qt 资源复制到用户配置目录。
- `config.xml` 写回会尽量保留当前未支持的原版节点。
- `shortcuts.xml` 写回会保留非 `Macros` 节点。

## 验证方式

已在 `./build` 执行：

```bash
cmake --build .
```

构建成功。

## 需要同步更新的索引

- `codex/analysis/stage-1-xml-configuration.md`
- `codex/features/configuration.md`
- `codex/modules/notepad-plus-plus/configuration.md`
- `codex/cache/latest.md`
