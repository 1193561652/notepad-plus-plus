# Multi-platform build documentation

Date: 2026-07-29

## Change

- Expanded `BUILD_AND_TEST.md` into the canonical Windows, Linux, and macOS
  build and test guide.
- Added the verified/pending platform matrix to `README.md`.
- Documented that the bundled static QScintilla build requires the matching
  Qt 5 `qmake` and a native Make implementation, even when CMake uses another
  generator.
- Added a Linux first-validation checklist covering toolchain details, clean
  build, CTest, configuration paths, localization, desktop integration, and
  high-DPI behavior.
- Documented CMake 3.20 as the baseline for the command forms used by the guide.

## Current status

- Windows Qt 5.12.12 + MinGW 7.3: verified.
- Linux Qt 5 + GCC/Clang: build path implemented, runtime verification pending.
- macOS Qt 5 + Apple Clang: build path implemented, verification pending.

This change is documentation-only and does not claim Linux or macOS validation
before those builds have been run on the target systems.
