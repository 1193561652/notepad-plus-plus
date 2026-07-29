# 构建与验证索引

## 主线项目

主线目录：仓库根目录

构建系统：CMake

主要要求：

- Qt 5.12.12 或更高版本。
- CMake 3.10 或更高版本。
- MinGW 或 MSVC。
- QScintilla 2.13.3，来源于 `third_party/qscintilla/`。

## 现有构建配置

`CMakeLists.txt` 当前声明：

- C++14。
- `CMAKE_AUTOMOC`、`CMAKE_AUTORCC`、`CMAKE_AUTOUIC` 开启。
- 依赖 `Qt5::Core`、`Qt5::Gui`、`Qt5::Widgets`。
- QScintilla include 路径：`third_party/qscintilla/src`。
- QScintilla 由
  `third_party/qscintilla/src/npp-qscintilla-static.pro`
  从仓库内唯一 QScintilla 源树构建。
- CMake 目标 `npp-qscintilla-build` 生成并暂存平台原生静态库。
- 静态库启用 `SCI_OWNREGEX` 并包含原版 Boost.Regex 适配层。
- 静态库同时包含大文件文档选项和指针宽度 Scintilla 消息扩展。
- 主程序链接 imported target `npp-qscintilla`，不依赖 QScintilla DLL。

## 插件构建注意

当前 `ENABLE_PLUGIN_SYSTEM` 选项默认值为 `OFF`。项目约束已确认插件不是近期功能目标，但代码接口继续保留。

## 验证策略

- 每个独立功能完成后至少执行一次验证。
- 验证可以是构建、静态检查、手动运行、或针对该功能的最小行为测试。
- 阶段开发过程中，根据修改范围、风险和依赖情况决定是否执行完整构建。
- 修改配置读写、Buffer 生命周期、文件保存、编码、插件接口时，应优先进行更强验证。

## 常用命令

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

## P0 配置与 UI 验证目标

- `config-corpus-tests`：对真实 Notepad++ XML 语料执行读取、真实写回和
  QDom 语义保持比较。
- `ui-parity-capture`：进程内实例化真实主窗口、Find 和 Preferences，
  生成 5 个查找页及 19 个首选项页截图。

```bash
cmake --build build --target config-corpus-tests ui-parity-capture
ctest --test-dir build -R "^(config|session|shortcuts|langs|stylers|context|udl)-"
```

Windows 下可通过 `QT_SCALE_FACTOR=1.0` 和 `1.5` 分别生成常规与高 DPI
截图矩阵。详细记录见
`codex/analysis/2026-07-26-p0-config-ui-validation.md`。

2026-07-29 起，CTest 自动包含以下真实入口 UI 测试：

- `ui-localization-runtime-100`
- `ui-localization-runtime-150`
- `ui-localization-dark-runtime-100`
- `ui-localization-dark-runtime-150`

它们使用隔离配置目录和简体中文资源，不读写用户实际配置。

## 大文件模式验证

```bash
cmake --build build --target notepadpp-qt large-file-mode-tests
ctest --test-dir build -R "^(core-behavior-tests|large-file-mode-tests)$" \
  --output-on-failure
```

完整验证可直接执行 `ctest --test-dir build --output-on-failure`。

如使用 MSVC：

```bash
cmake -S . -B build -G "Visual Studio 16 2019" -A Win32
cmake --build build --config Release
```
## P1 运行验证

```bash
cmake --build build --target p1-runtime-capture --parallel 4
p1-runtime-capture.exe <output-directory>
```

该目标只在 `BUILD_TESTING` 开启时构建，使用临时文件验证 P1 UI 和系统交互，
不会写用户 UDL 配置。Windows 上运行 GUI 目标需要桌面会话。
