# Notepad++ for Qt

Notepad++ for Qt is a cross-platform port of Notepad++ v8.4.6. The project
targets behavioral, configuration, and workflow compatibility while replacing
Win32-specific implementation with Qt and standard C++.

## Stack

- C++14
- Qt 5 / Qt Widgets
- CMake
- Bundled QScintilla 2.13.3 / Scintilla
- Notepad++ Boost.Regex search backend

## Build

Install Qt 5 with a matching C++ toolchain, then configure from the repository
root:

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j 4
```

The application is produced as `build/notepadpp-qt` or
`build/notepadpp-qt.exe`.

Run the complete automated suite with:

```bash
ctest --test-dir build --output-on-failure
```

See [BUILD_AND_TEST.md](BUILD_AND_TEST.md) for platform notes and targeted
validation commands.

## Platform Support

- Windows with Qt 5.12.12 and MinGW is the currently verified build.
- Ubuntu 22.04 with Qt 5.15.3 and GCC 11.4 is built and tested.
- Other Linux distributions and Clang remain supported build paths but are not
  yet part of the verified matrix.
- macOS with Qt 5 and Apple Clang has a build path but has not yet been verified.

Each platform builds the bundled, project-specific static QScintilla library
with the Notepad++ Boost.Regex backend. Linux and macOS therefore require
`qmake` and GNU Make in addition to the CMake build tool. Packaging and plugin
ABI compatibility are outside the current platform-build scope.

## Repository Layout

- `src/`: application and Qt port implementation
- `resources/`: compatible XML defaults, localization, icons, and parsers
- `tests/`: behavior, configuration, and UI runtime tests
- `third_party/qscintilla/`: bundled QScintilla and Scintilla source
- `third_party/boostregex/`: Boost.Regex integration
- `third_party/lexilla/`: LexUser source used by the static editor core
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
- Plugin ABI work remains deferred; host interfaces stay isolated.

## License

The port builds on Notepad++, QScintilla, Scintilla, Boost, Lexilla, Qt, and
other bundled components. Refer to the license files and source headers in the
repository and its Git history for component-specific terms.
