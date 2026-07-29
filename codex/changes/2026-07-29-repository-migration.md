# Git 仓库迁移

## 结构

- 用 Qt 主线替换 `qt-port` 分支中的原版 v8.4.6 工作树。
- 保留原版 Git 历史和 tag，移除当前分支不再使用的 Win32 工作树文件。
- 将 `codex/` 和 `AGENTS.md` 纳入仓库根。
- 内置 QScintilla、Scintilla 和 LexUser 构建源码。

## 构建

- `QSCINTILLA_ROOT` 改为 `third_party/qscintilla`。
- LexUser 改由 `NPP_LEXUSER_SOURCE` 显式传给 qmake。
- v8.4.6 配置语料固定存放于 `tests/corpus/v8.4.6/`。
- 删除 v7.81 本机诊断目录的 CMake 路径依赖。
- Windows 构建后将 Qt、MinGW 和 Qt platform/imageformat 运行库部署到输出目录，
  测试不再依赖调用 shell 的 PATH。
- `ToolbarIconThemeTests` 按 Qt 要求创建 `QGuiApplication`。

## 文档

- 重写 README、构建说明和 AGENTS。
- 更新 `codex/` 中旧主线、QScintilla 和原版源码路径。
- 新增适用于 CMake、qmake和运行时配置的 `.gitignore`。

## 验证

- 从新仓库空 `build/` 配置并构建成功。
- 仓库内 QScintilla、Boost.Regex 和 LexUser 静态集成构建成功。
- CTest `25/25` 通过。
