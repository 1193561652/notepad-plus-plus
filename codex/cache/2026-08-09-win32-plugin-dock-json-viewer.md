# Cached state: Win32 plugin Dock and JSON Viewer

- v8.4.6 Dock ABI is implemented by `Win32PluginDockAdapter` and
  `Win32NativeDockHost` under `src/Win32PluginSystem`.
- Cross-platform `DockingManager` remains the sole owner of QDockWidget layout
  and now supports remove, title update, stable state, and late registration.
- `MainWindow` destroys `Win32PluginManager` before its value-owned
  `DockingManager`; plugin HWNDs are detached before DLL unload.
- JSON Viewer 1.41 x64 is in the managed corpus and load whitelist.
- Runtime verification covers official-package receipt, all four commands,
  real Format/Compress behavior, JSON language switching, Dock create/show/
  hide/float/redock, exact native parent/size, and shutdown cleanup.
- Measured hierarchy delta: four new native Qt widgets (host, QDockWidget,
  close button, float button) and six Win32 descendants total. No editor,
  splitter, central widget, or higher intermediate container is promoted.
- JSON Viewer's three functional commands run in the DLL. Its About command is
  an equivalent Qt dialog because v1.41 closes a modeless `CreateDialog` with
  `EndDialog`, which is unsafe in this host.
- Detailed record: `codex/changes/2026-08-09-win32-plugin-dock-json-viewer.md`.
