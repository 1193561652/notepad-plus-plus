# Notepad++ for Qt

Notepad++ for Qt is a cross-platform port of Notepad++ v8.4.6. The project
targets behavioral, configuration, and workflow compatibility while replacing
Win32-specific implementation with Qt and standard C++.

This is an independent Qt port, not an official Notepad++ release. The Qt port
is authored and maintained by **Jiang Liwei** at
<https://github.com/1193561652/notepad-plus-plus/tree/qt-port>. The original
Notepad++ project is <https://github.com/notepad-plus-plus/notepad-plus-plus>.
See [QT_PORT_NOTICE.md](QT_PORT_NOTICE.md) for complete attribution and license
scope.

Current platform, feature, packaging, and plugin-port progress is summarized in
[PROJECT_STATUS.md](PROJECT_STATUS.md).

## Stack

- C++17
- Qt 5 / Qt Widgets
- CMake
- Bundled Scintilla 5.3.0 Qt platform implementation
- Separately linked static Lexilla
- Notepad++ Boost.Regex search backend

## Build

All platforms require CMake, Qt 5 with the Core, Gui, Widgets, Network,
PrintSupport, and Xml modules, and a C++17 compiler. Initialize the pinned
plugin catalog after cloning:

```bash
git submodule update --init
```

### Windows

Tested build environment: **Windows, Qt 5.12.12, MinGW 7.3, and CMake**. Use
the compiler and `mingw32-make` supplied with the same Qt installation. From
PowerShell, first set `QT_ROOT` to the selected Qt kit directory and
`MINGW_ROOT` to its matching MinGW toolchain directory:

```powershell
$env:Path = "$env:MINGW_ROOT\bin;$env:QT_ROOT\bin;$env:Path"

cmake -S . -B build-windows -G "MinGW Makefiles" `
  -DCMAKE_BUILD_TYPE=Debug `
  -DBUILD_TESTING=ON `
  -DCMAKE_PREFIX_PATH="$env:QT_ROOT" `
  -DCMAKE_CXX_COMPILER="$env:MINGW_ROOT\bin\g++.exe" `
  -DCMAKE_MAKE_PROGRAM="$env:MINGW_ROOT\bin\mingw32-make.exe"
cmake --build build-windows -j 4
ctest --test-dir build-windows --output-on-failure
```

The executable and required runtime libraries are placed in `build-windows`.

### Ubuntu

Tested build environment: **Ubuntu 22.04.5 LTS x86_64, Linux 6.8, Qt 5.15.3,
GCC 11.4.0, CMake 3.22.1, and GNU Make 4.3**. Install the required development
packages:

```bash
sudo apt update
sudo apt install build-essential cmake qtbase5-dev qtbase5-dev-tools unzip
```

Configure, build, test, and run from the repository root:

```bash
cmake -S . -B build-ubuntu \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON
cmake --build build-ubuntu -j "$(nproc)"
ctest --test-dir build-ubuntu --output-on-failure
./build-ubuntu/notepadpp-qt
```

Build the Ubuntu `.deb` package in Release mode:

```bash
cmake -S . -B build-ubuntu-package \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON
cmake --build build-ubuntu-package -j "$(nproc)"
ctest --test-dir build-ubuntu-package --output-on-failure
cpack --config build-ubuntu-package/CPackConfig.cmake -G DEB
```

The package installs the self-contained application under
`/opt/notepad-plus-plus` and provides `/usr/bin/notepad++` plus a desktop entry.

### macOS

**Not tested yet; testing will have to wait until I can afford a Mac.**

The intended build path requires Qt 5, CMake, and the Xcode command-line tools:

```bash
cmake -S . -B build-macos \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON \
  -DCMAKE_PREFIX_PATH="$HOME/Qt/5.15.2/clang_64"
cmake --build build-macos -j "$(sysctl -n hw.ncpu)"
ctest --test-dir build-macos --output-on-failure
./build-macos/notepadpp-qt
```

The current macOS path produces a plain executable; application bundling,
signing, notarization, and deployment packaging are not implemented. See
[BUILD_AND_TEST.md](BUILD_AND_TEST.md) for additional platform notes and
targeted validation commands.

## Platform Support

- Windows with Qt 5.12.12 and MinGW is the currently verified build.
- Ubuntu 22.04 with Qt 5.15.3 and GCC 11.4 is built and tested.
- Other Linux distributions and Clang remain supported build paths but are not
  yet part of the verified matrix.
- macOS with Qt 5 and Apple Clang has a build path, but it has not been tested
  yet; testing will have to wait until I can afford a Mac.

Each platform builds the bundled `npp-scintilla-qt` static library with the
Notepad++ Boost.Regex backend and the separate `npp-lexilla` static library.
Both are built directly by CMake; qmake is not part of the current build.
Plugin package management, the reviewed Windows Notepad++ ABI compatibility
layer, and the minimal cross-platform C ABI v1 are available. The public ABI
is documented in `src/CrossPlatformPluginSystem/README.md`.

## Repository Layout

- `src/`: application and Qt port implementation
- `resources/`: compatible XML defaults, localization, icons, and parsers
- `installer/`: reserved Windows installer tree, corresponding to the original
  Notepad++ installer location
- `installer_ubuntu/`: Debian package metadata and desktop integration
- `installer_mac/`: reserved macOS installer tree
- `tests/`: behavior, configuration, and UI runtime tests
- `third_party/scintilla/`: bundled Scintilla 5.3.0 source and Qt platform code
- `third_party/boostregex/`: Boost.Regex integration
- `third_party/lexilla/`: separately built Lexilla, lexlib, built-in lexers,
  and the v8.4.6 LexUser lexer
- `third_party/nppPluginList/`: pinned JSON-only plugin catalog submodule
- `src/CrossPlatformPluginSystem/`: public cross-platform plugin ABI and guide
- `codex/`: project knowledge base and implementation records

## Original Source

This branch retains the original Notepad++ repository history. Reference
v8.4.6 behavior without a second source checkout:

```bash
git show v8.4.6:PowerEditor/src/Notepad_plus.cpp
```

## Compatibility Principles

- Preserve user-visible behavior before source-level similarity.
- Preserve Notepad++ XML formats and unknown user data.
- Keep platform-specific code behind explicit adaptation boundaries.
- Keep changes local and verify every completed feature.
- Keep original Windows plugin adaptation isolated from the cross-platform ABI.

## License

This modified Qt port is distributed under the Notepad++ GNU GPL version 3
terms, clarifications, and exceptions in [LICENSE](LICENSE). Original
copyright notices are retained. Scintilla, Boost, Lexilla, Qt, plugins, and
other third-party components retain their own licenses; see their license files
and [QT_PORT_NOTICE.md](QT_PORT_NOTICE.md).
