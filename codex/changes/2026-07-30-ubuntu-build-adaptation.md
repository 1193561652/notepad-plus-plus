# Ubuntu build adaptation

Date: 2026-07-30

## Environment

- Ubuntu 22.04.5 LTS (x86_64)
- Linux 6.8
- GCC 11.4.0
- CMake 3.22.1
- Qt 5.15.3
- GNU Make 4.3
- Debug configuration with `BUILD_TESTING=ON`

## Build fixes

- Removed the `tchar.h` dependency and defined the port's Unicode TinyXml
  character type directly as standard `wchar_t` on every platform.
- Replaced `_wtoi`, `_wtof`, TCHAR classification macros, `wcscpy_s`, and the
  platform-specific `swprintf` form with `std::wcstol`, `std::wcstod`,
  `std::isw*`, `std::wmemcpy`, and bounds-aware `std::swprintf`.
- Replaced `_wfopen`-style Unicode path handling with Qt file APIs for both
  TinyXml variants. UTF-8 serialization uses Qt's Unicode conversion so
  Windows surrogate pairs and Unix code points follow one implementation.
- Existing TinyXml helper and wrapper names such as `wstring2string`,
  `generic_atoi`, and `Win32_IO_File` remain stable; only their internals were
  made platform-neutral.
- Added the standard headers directly required for `uintptr_t` in uchardet and
  `strlen`/`strcmp` in the bundled v8.4.6 LexUser lexer.
- Added an encode/decode round-trip check to legacy text encoding on every
  platform. Codec backends can replace unrepresentable input without
  incrementing `ConverterState::invalidChars`; the explicit check preserves
  the existing contract that saving must reject data loss.

## Test fixes

- Linux configuration corpus tests now declare
  `QT_QPA_PLATFORM=offscreen`, matching the other GUI-backed CTest targets.
- Configuration corpus isolation now uses
  `NppParameters::setUserPathOverride()` on every platform instead of relying
  on the Windows-specific `APPDATA` environment variable.
- Configuration corpus tests directly load, save, and reload a Unicode-path
  XML file through TinyXml.

## Validation

Commands:

```bash
cmake -S . -B build-ubuntu \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON
cmake --build build-ubuntu -j 4
ctest --test-dir build-ubuntu --output-on-failure -j 4
cmake -S . -B build-ubuntu-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF
cmake --build build-ubuntu-release -j 4
```

Results:

- Main application linked successfully.
- Independent Debug and Release builds both completed successfully.
- Bundled static QScintilla, Scintilla, Boost.Regex, and LexUser linked
  successfully.
- CTest: 22/22 passed.
- Configuration corpus tests passed direct TinyXml load/save/reload through a
  non-ASCII Unicode path.
- Light and dark localization UI tests passed offscreen at 100% and 150%.
- Retained TinyXmlA sources passed standalone C++14 compilation in both STL and
  non-STL modes.
- A three-second offscreen startup smoke test remained alive until the timeout
  and created `config.xml`, `contextMenu.xml`, `langs.xml`, `shortcuts.xml`,
  `stylers.xml`, `toolbarIcons.xml`, and `userDefineLang.xml` in an isolated
  settings directory.

## Remaining platform scope

- Interactive desktop behavior such as native file dialogs, clipboard
  integration, physical printing, desktop launchers, Wayland/X11 differences,
  and subjective font rendering still requires a graphical manual validation
  pass.
- macOS and non-Ubuntu Linux distributions remain unverified.
- Plugin ABI compatibility remains outside the current build scope.

## Cross-platform API audit

- The TinyXml compatibility layer contains no `tchar.h`, `_wtoi`, `_wtof`,
  `_wfopen`, `wcscpy_s`, TCHAR classification macro, or platform-specific
  `swprintf` use.
- The targeted wide-character parsing, formatting, copying, encoding, and file
  paths have no `_WIN32` or `Q_OS_WIN` implementation branch.
- The offscreen CTest environment remains Linux-only because it selects a
  platform plugin rather than implementing application behavior.
- Both STL and non-STL forms of the retained TinyXmlA module compile in
  standalone C++14 checks.

No Windows Qt cross-toolchain is installed on the Ubuntu validation host, so a
new Windows binary was not produced locally. The unified code uses C++14 and
Qt 5 APIs available to the verified Windows toolchain; the next Windows CI or
native build should still be run before publishing a Windows release.
