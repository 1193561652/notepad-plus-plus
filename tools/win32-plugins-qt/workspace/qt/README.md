# Qt 插件构建

状态与原始逻辑映射见 [PORTING_STATUS.md](PORTING_STATUS.md)。目前 17 个插件接入构建，
其余有源码插件仍在移植；7 个仅提供 DLL 的插件按用户要求跳过。

需要 CMake、C++17 编译器、Qt Widgets 和旁边的 `notepad-plus-plus` Qt 宿主源码。
Qt 6 构建 BetterMultiSelection 时还需要 Core5Compat。插件与宿主必须使用兼容的 Qt 和编译器。

先准备原插件固定的依赖，在 `win32-plugins` 下执行：

```powershell
cmake -P editorconfig-notepad-plus-plus/qt/fetch-dependencies.cmake
git -C JSON-Viewer submodule update --init external/rapidjson
```

EditorConfig 使用原 `init.ps1` 的 core 0.12.10 和 PCRE2 10.46；准备脚本验证提交，
保留已有不同提交的目录并报错。JSON Viewer 必须使用仓库指定的 RapidJSON 分支和 gitlink，
其 RawNumber、转义和换行扩展是原逻辑的一部分，不能换成系统 RapidJSON。
配置和正常构建不会下载依赖。

本机已验证的 Windows 构建命令：

```powershell
$env:PATH = 'F:\Qt\Qt5.12.12\Tools\mingw730_64\bin;' + $env:PATH
cmake -S . -B build-qt -G 'MinGW Makefiles' -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=F:/Qt/Qt5.12.12/5.12.12/mingw73_64 -DNPP_QT_HOST_BUILD=F:/project2/Notepad++/notepad-plus-plus/build-windows-regression
cmake --build build-qt -j 4
ctest --test-dir build-qt --output-on-failure --timeout 30
cmake --install build-qt --prefix build-qt/package
```

`NPP_QT_HOST_SOURCE` 可指定其他宿主源码目录。`NPP_QT_HOST_BUILD` 可省略，但这样只运行
ABI 和独立算法测试；指定时会链接该目录内的 `npp-scintilla-qt`，验证真实编辑器行为。
Windows 测试会短暂显示测试窗口，并使用系统剪贴板。

产物位于 `build-qt/plugins/<插件名>-qt/`。安装包中对应文件夹可复制到 Qt 宿主的
`plugins` 目录；这是 Qt 跨平台 ABI 模块，不能加载到原版 Win32 Notepad++。
当前只实际验证 Windows x64 / Qt 5.12.12 / MinGW 7.3。
