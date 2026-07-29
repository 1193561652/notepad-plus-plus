# Multi-platform Build and Test

## Requirements

- CMake 3.20 or newer for the documented commands
- Qt 5.12 or newer with Core, Gui, Widgets, Network, PrintSupport, and Xml
- A compiler supported by that Qt installation
- qmake and the matching native build tool
- GNU Make on Linux and macOS, including when CMake itself uses Ninja

QScintilla, Scintilla, Boost.Regex integration, and LexUser are bundled under
`third_party/`; no sibling source directories are required.

The Qt libraries, `qmake`, compiler, and build tool must belong to one compatible
toolchain. In particular, do not mix a distribution Qt build with an unrelated
compiler or a `qmake` from another Qt installation.

## Platform Status

| Platform | Toolchain | Status |
| --- | --- | --- |
| Windows | Qt 5.12.12 + MinGW 7.3 | Built and tested |
| Windows | Qt 5 + MSVC | CMake path present; not currently verified |
| Linux | Qt 5 + GCC/Clang | Build path present; pending Linux verification |
| macOS | Qt 5 + Apple Clang | Build path present; pending macOS verification |

Plugin loading is disabled by default on every platform. The current status
describes the application and automated tests, not plugin ABI compatibility.

## Common Configure

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON
```

When Qt is not discoverable, provide its CMake package prefix:

```bash
cmake -S . -B build -DBUILD_TESTING=ON \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/5.x/compiler
```

Build and test:

```bash
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

The build first creates the project-specific static QScintilla archive and then
links the application and tests.

## Windows

Use the compiler and `mingw32-make` shipped with the selected Qt MinGW
installation. A PowerShell example is:

```powershell
$qt = "F:\Qt\Qt5.12.12\5.12.12\mingw73_64"
$mingw = "F:\Qt\Qt5.12.12\Tools\mingw730_64"
$env:Path = "$mingw\bin;$qt\bin;$env:Path"

cmake -S . -B build-windows -G "MinGW Makefiles" `
  -DCMAKE_BUILD_TYPE=Debug `
  -DBUILD_TESTING=ON `
  -DCMAKE_PREFIX_PATH="$qt" `
  -DCMAKE_CXX_COMPILER="$mingw\bin\g++.exe" `
  -DCMAKE_MAKE_PROGRAM="$mingw\bin\mingw32-make.exe"
cmake --build build-windows -j 4
ctest --test-dir build-windows --output-on-failure
```

The Windows post-build step copies the required Qt and MinGW runtime libraries
beside `notepadpp-qt.exe`.

## Linux

For Debian and Ubuntu, install the basic Qt 5 development toolchain:

```bash
sudo apt update
sudo apt install build-essential cmake qtbase5-dev qtbase5-dev-tools qt5-qmake
```

Configure a separate Linux build directory:

```bash
cmake -S . -B build-linux \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON
cmake --build build-linux -j "$(nproc)"
ctest --test-dir build-linux --output-on-failure
```

Run the application from the repository root with:

```bash
./build-linux/notepadpp-qt
```

If Qt 5 is installed outside the distribution prefix, select it explicitly:

```bash
cmake -S . -B build-linux \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON \
  -DCMAKE_PREFIX_PATH="$HOME/Qt/5.15.2/gcc_64"
```

Before diagnosing a QScintilla build failure, verify that CMake and qmake found
the same Qt installation:

```bash
qmake -query QT_VERSION
qmake -query QT_INSTALL_PREFIX
cmake -S . -B build-linux -LA | grep -E "Qt5_DIR|CMAKE_(CXX_)?COMPILER"
```

The UI tests set `QT_QPA_PLATFORM=offscreen` through CTest on non-Windows
platforms, so the automated suite should run in a headless shell. Interactive
application and screenshot validation still require a graphical session.

The first Linux validation should record:

- distribution, architecture, desktop environment, and display backend;
- Qt, CMake, compiler, qmake, and GNU Make versions;
- configure and compile results from a clean build directory;
- complete CTest results;
- startup, configuration path, localization, fonts, clipboard, printing, file
  dialogs, and high-DPI behavior.

## macOS

Install Qt 5 and CMake, and ensure the Xcode command-line tools are available.
For the official Qt layout:

```bash
cmake -S . -B build-macos \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON \
  -DCMAKE_PREFIX_PATH="$HOME/Qt/5.15.2/clang_64"
cmake --build build-macos -j "$(sysctl -n hw.ncpu)"
ctest --test-dir build-macos --output-on-failure
./build-macos/notepadpp-qt
```

The current build produces a plain executable. Application bundle creation,
signing, notarization, and deployment packaging are separate future tasks.

## Test Groups

Useful targeted groups:

```bash
ctest --test-dir build -R "localization|ui-" --output-on-failure
ctest --test-dir build -R "config|session|shortcuts|langs|stylers|context|udl" --output-on-failure
ctest --test-dir build -R "large-file|boost-regex|core-behavior" --output-on-failure
```

The UI localization matrix runs simplified Chinese in light and dark themes at
100% and 150% scale using isolated settings directories. Replace `build` in
these commands with the platform-specific build directory when applicable.

## Troubleshooting

- Remove or choose a new build directory after changing the Qt installation,
  compiler, architecture, or generator.
- Check `qmake -query` when the static QScintilla configuration fails.
- Ensure `make` or `gmake` is installed on Unix even if CMake uses Ninja.
- Use `ctest --test-dir <build-dir> --output-on-failure -V` for full test
  commands and environment details.
- On Linux, set `QT_DEBUG_PLUGINS=1` to diagnose platform plugin loading.
- Do not add generated build directories or deployed runtime files to Git.

## Original v8.4.6 Reference

Use the repository tag rather than an external checkout:

```bash
git show v8.4.6:PowerEditor/src/Parameters.cpp
git show v8.4.6:PowerEditor/src/ScintillaComponent/Buffer.cpp
```

Configuration corpus fixtures used by CTest are stored in
`tests/corpus/v8.4.6/`.
