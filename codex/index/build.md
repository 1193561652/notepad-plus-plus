# 构建与验证索引

## 当前基线

- 构建系统：CMake 3.10+。
- 语言标准：C++17。
- UI：Qt 5.12.12+（Core、Gui、Widgets、Network、PrintSupport、Xml）。
- 编辑器：Scintilla 5.3.0，位于 `third_party/scintilla/`。
- 词法器：Lexilla 5.1.9，位于 `third_party/lexilla/`。
- 正则：Notepad++ v8.4.6 Boost.Regex 适配层，位于
  `third_party/boostregex/`。

版本与原版 Notepad++ v8.4.6 完全对齐，不应单独升级其中一个组件。

## 静态目标

| CMake 目标 | 内容 |
| --- | --- |
| `npp-scintilla-qt` | Scintilla core、官方 `ScintillaEditBase` Qt 平台、`SCI_OWNREGEX` |
| `npp-lexilla` | Lexilla catalogue、lexlib、内置 lexer、LexUser、LexSearchResult |
| `notepadpp-qt` | Qt 应用层，链接上述两个静态库 |

`npp-scintilla-qt` 定义 `SCINTILLA_QT`、`MAKING_LIBRARY`、
`SCI_OWNREGEX` 和 `SCI_NAMESPACE`。应用不链接 QScintilla，也不依赖
QScintilla 动态库或 qmake 子构建。

## 常用命令

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j 6
ctest --test-dir build --output-on-failure -j 4
```

编辑器迁移后的 Windows/MinGW 验证结果：全量构建成功，CTest `29/29`
通过。Linux 构建仍需在目标 Linux 环境重新执行，不能以迁移前的 Linux
结果代替。

## 编辑器专项验证

```bash
cmake --build build --target npp-scintilla-qt npp-lexilla
ctest --test-dir build -R "^(lexilla-integration-tests|boost-regex-tests|large-file-mode-tests)$" --output-on-failure
```

- `lexilla-integration-tests`：验证 catalogue 和真实 `ILexer5`。
- `boost-regex-tests`：验证 lookbehind、`\R`、捕获替换和空匹配。
- `large-file-mode-tests`：验证大文档选项、共享 Document、Lexilla XML、UDL。

## 构建注意事项

- `third_party/scintilla/qt/ScintillaEditBase` 的公开头依赖 Scintilla
  `src/` 内部平台头，因此 `npp-scintilla-qt` 会公开这两个 include 路径。
- `LexUser.cxx` 需要显式 `using namespace Lexilla` 才能在 GCC/MinGW 下构建。
- `SCI_SETILEXER` 接管 lexer 生命周期；应用不得在发送后释放 `ILexer5*`。
- `SCI_CREATEDOCUMENT` 返回的创建者引用在 `SCI_SETDOCPOINTER` 后必须用
  `SCI_RELEASEDOCUMENT` 释放。
