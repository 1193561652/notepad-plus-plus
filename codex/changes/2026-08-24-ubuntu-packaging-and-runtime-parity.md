# Ubuntu packaging and runtime parity

## Scope

- Added the original-style `installer/` placeholder plus Ubuntu and macOS
  installer trees.
- Added CPack Debian packaging, `/usr/bin/notepad++`, a desktop entry and all
  optional native-language files.
- Added a stable Linux desktop file identity and X11 `StartupWMClass` so the
  installed application can be pinned by Ubuntu desktop environments.
- Restored v8.4.6 first-line language detection for otherwise unrecognized
  filenames: supported shebangs and XML/PHP/HTML markers only.
- Loaded the close-document save prompt and its buttons from the original
  `Dialog/DoSaveOrNot` native-language node.

## Verification

- Ubuntu Release build succeeded.
- All 35 configured Ubuntu tests passed.
- The generated Debian package passed desktop-file validation, dependency and
  content inspection, included 94 language files, and remained running during
  an offscreen startup smoke test.

See [PROJECT_STATUS.md](../../PROJECT_STATUS.md) for the current summary and
[README.md](../../README.md) for build and packaging commands.
