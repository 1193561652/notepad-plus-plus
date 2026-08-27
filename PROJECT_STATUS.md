# Project Status

Last updated: 2026-08-27

This is an independent Qt port of Notepad++ v8.4.6 maintained by Jiang Liwei.
The application name remains **Notepad++**. See [QT_PORT_NOTICE.md](QT_PORT_NOTICE.md)
for attribution and license scope.

## Platform status

| Platform | Status | Latest verification |
| --- | --- | --- |
| Windows | Built and tested; installer pending test | Qt 5.12.12, MinGW 7.3; NSIS installer definition completed but not yet generated or installed on Windows |
| Ubuntu | Built, tested and packaged | Ubuntu 22.04, Qt 5.15.3, GCC 11.4; Release build and 36/36 tests passed; `.deb` smoke-tested |
| macOS | Build path only | Not tested yet; testing will have to wait until I can afford a Mac |

Build commands and tested environments are documented in [README.md](README.md).

## Current implementation

- Core editing, file/session handling, search, configuration, main UI and the
  permanent main/sub Scintilla views are implemented. Detailed compatibility
  notes: [core status](codex/analysis/2026-07-25-core-compatibility-status.md)
  and [structure audit](codex/analysis/2026-08-09-original-structure-interface-audit.md).
- Scintilla 5.3.0, Lexilla 5.1.9 and the Boost.Regex backend are built as
  independent CMake targets. See [repository index](codex/index/repository.md).
- The original native-language model is used. All 94 v8.4.6 language files are
  packaged under `localization/`; no Qt `.ts/.qm` translation path is used.
  See [language loading record](codex/changes/2026-08-24-original-native-language-loading.md).
- Plugin catalogs are pinned through the JSON-only `nppPluginList` submodule
  and are not updated online. See [catalog record](codex/changes/2026-08-24-plugin-catalog-submodule.md).
- Ubuntu packaging installs the application under `/opt/notepad-plus-plus`,
  provides `/usr/bin/notepad++`, installs optional languages and supports
  desktop-launcher pinning.
- Windows packaging now provides a simplified original-style NSIS flow with
  optional language components, shortcuts, Qt runtime deployment and an
  uninstaller. It is awaiting the requested Windows-side test pass. See the
  [Windows installer record](codex/changes/2026-08-26-windows-installer.md).
- Original modeless-dialog behavior is restored for the corresponding Qt
  dialogs, and empty-line clicks reliably restore Scintilla caret focus. See
  [dialog and caret record](codex/changes/2026-08-24-modeless-dialogs-and-empty-line-caret.md).
- The complete v8.4.6 icon tree is embedded, with original toolbar, document
  tab/close, dock panel, panel toolbar, tree, and About loading rules. See the
  [resource loading record](codex/changes/2026-08-25-original-resource-loading.md).

## Plugin ports

Qt port branches exist for ComparePlus, HEX-Editor, DSpellCheck,
MarkdownViewerPlusPlus, Explorer, NppExec, NppFTP, NppMarkdownPanel,
NPPTextFX2 and PythonScript. Their repositories, baselines and remaining
differences are tracked in [plugin repositories](codex/index/plugin-source-repositories.md)
and [plugin port status](codex/index/high-priority-plugin-ports.md). PythonScript
now embeds Python 3 and exposes the generated Scintilla editor API; its remaining
host-API limits are recorded in the plugin status document. ComparePlus and
HEX-Editor have also completed their Ubuntu Qt interaction regressions, including
real widget/modal-dialog operations and binary-safe document round trips.

## Remaining work

- Test and package the macOS build; implement signing/notarization when a macOS
  environment is available.
- Generate and install-test the NSIS package on Windows, then address any
  toolchain-specific packaging differences found there.
- Replace the placeholder macOS installer tree with a production installer.
- Continue behavior-level parity work for advanced editor/UI details and the
  documented deep differences in individual plugins.
- Run final interactive desktop checks on each release target in addition to
  automated tests.

Historical implementation records are under [codex/changes](codex/changes),
and validation artifacts are under [codex/validation](codex/validation).
