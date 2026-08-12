# 2026-08-12 Ubuntu Regression

## Scope

- Ubuntu 22.04.5 LTS Release build with GCC 11.4 and Qt 5.
- `BUILD_TESTING=ON` with the complete CTest suite.
- Plugin system disabled, matching the non-Windows build boundary.

## Original Structure Mapping

- The v8.4.6 UDL implementation remains owned by Lexilla's `LexUser.cxx`.
- Lexilla remains the independent `npp-lexilla` static library and is passed to
  Scintilla through the existing application-facing lexer path.
- `ScintillaEditView` remains the Qt editor adapter and owner of its Qt signal
  connections. No signal or editor responsibility moved into `MainWindow`.

## Fixes

- Made `LexUser.cxx` explicitly include the standard C string declarations and
  qualified its `strlen` and `strcmp` calls. The original lexer logic is
  unchanged; this replaces a declaration that the Windows build received
  transitively from `windows.h`.
- Made the C-compatible `Sci_Position.h` self-contained for `intptr_t` by
  including `stdint.h`.
- Qualified two `ScintillaEditView` signal member pointers as required by GCC.
  Signal ownership, receivers, and runtime behavior are unchanged.

## Results

- CMake configure: passed.
- Release build: passed.
- CTest: 32/32 passed in 4.06 seconds.
- UI localization captures passed at 100% and 150% scale in light and dark
  modes using the Qt offscreen platform.
- No-plugin startup, Lexilla integration, large-file mode, Boost.Regex, and the
  v8.4.6 configuration corpus all passed.

## Warning Cleanup Follow-up

- Replaced deprecated Qt 5.15 split-behavior overloads through a shared
  `QtCompat.h` selector that keeps the Qt 5.12.12 baseline buildable.
- Used `QRegularExpression` for configuration token splitting and UI child
  lookup where the old `QRegExp` overload was deprecated.
- Used `QButtonGroup::idClicked` on Qt 5.15 and newer while retaining the
  non-deprecated Qt 5.12 signal behind a version guard.
- Preserved command-line launching semantics through `QProcess::splitCommand`
  on Qt 5.15 and newer and the original overload on older Qt.
- Removed the deprecated Lexilla ID lookup from the integration test; the same
  built-in lexer remains covered by `CreateLexer("cpp")`.
- A clean-first Release rebuild completed with zero compiler warnings.
- Post-cleanup CTest: 32/32 passed in 4.18 seconds.

Build and runtime artifacts remain under `build/` and are not tracked.
