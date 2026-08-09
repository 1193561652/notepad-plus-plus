# Win32 plugin Dock and JSON Viewer 1.41

## Scope

- Implements the Notepad++ v8.4.6 `tTbData` layout and the Dock-related
  `NPPM_*`, `DWS_*`, and `DMN_*` semantics used by Windows plugins.
- Keeps generic Qt Dock ownership in `WinControls/DockingWnd/DockingManager`.
- Keeps HWND hosting, modeless-dialog routing, and Win32 notifications inside
  `Win32PluginSystem`.
- Adds the official x64 JSON Viewer 1.41 package to the managed plugin corpus.

## Structure

- `Win32PluginDockAdapter` converts `Win32DockingData` into `DockingData`,
  routes show/hide/update/register/view-by-name messages, sends `WM_NOTIFY`
  Dock lifecycle notifications, and handles registered modeless dialogs.
- `Win32NativeDockHost` is the only widget that directly owns a plugin client
  HWND. It changes the plugin window to `WS_CHILD`, reparents it, keeps it at
  the host client size, forwards visibility/focus, and restores its original
  parent/styles before DLL unload.
- Plugin shutdown sends `NPPN_SHUTDOWN`, releases hosted Dock windows, and only
  then frees DLL modules. `MainWindow` explicitly destroys the Win32 plugin
  manager while its value-owned `DockingManager` is still alive.
- Plugin Dock object names use module name plus dialog ID, not runtime HWNDs.
  Late registration uses `QMainWindow::restoreDockWidget()` after the normal
  state restore, preserving saved area, floating state, and visibility.

## Window hierarchy result

Opening JSON Viewer adds six descendant Win32 windows in the measured fixture.
Four Qt widgets become native: `Win32NativeDockHost`, its `QDockWidget`, and
the Dock's standard close and float title buttons. No editor, splitter,
central widget, or higher intermediate container is promoted. The plugin
client remains a direct child of the dedicated host and exactly fills it.

The QDockWidget promotion is a Qt 5 consequence of placing a native child in
the Dock. `QWindow::fromWinId/createWindowContainer` has broader documented
ancestor-promotion behavior, so the explicit QWidget/HWND boundary is retained
and covered by a runtime hierarchy assertion.

## JSON Viewer verification

- Official package SHA-256:
  `f9e3f6e1d93e088f7477cd6bc3cb9767b803acc4171533155435fbc02ae8926a`.
- Verified updater receipt, DLL loading, and all four exported menu commands.
- Verified tree Dock creation, native parent/size, hide/show, float, redock,
  and shutdown cleanup.
- Verified real DLL Format JSON and Compress JSON commands and the JSON lexer
  switch through `NPPM_SETCURRENTLANGTYPE`.
- The three editor/Dock commands execute the real DLL. About uses an equivalent
  nonmodal Qt dialog because v1.41 incorrectly calls `EndDialog` for its
  `CreateDialog` window, which raises a callback exception in the Qt host.
