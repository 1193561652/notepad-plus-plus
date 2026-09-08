# Cross-platform plugin ABI v1

`PluginInterface.h` is the complete public ABI definition for the first
cross-platform plugin version. It deliberately contains only C data types and
does not expose Qt, STL, C++ classes, exceptions, or native window handles.

## Relationship to the original interface

The six exports follow the Notepad++ plugin shape: version query, name,
`setInfo`, command array, notification callback, and message callback. The
platform-independent ABI replaces `NppData` window handles with
`NppPluginHostInfo`, which contains host services and immutable environment
information.

All exported strings and paths are UTF-8. The host owns the strings in
`NppPluginHostInfo`; they remain valid from `nppSetInfo` until the shutdown
notification returns. Plugins own their name, function array, and function
item strings until shutdown.

## Required exports

- `nppGetPluginAbiVersion`: return `NPP_PLUGIN_ABI_VERSION`.
- `nppGetName`: return a stable, non-empty UTF-8 display name.
- `nppSetInfo`: retain only the host pointer or copy required values; return
  non-zero when initialization succeeds.
- `nppGetFuncsArray`: return a stable array of `NppPluginFuncItem` values.
- `nppBeNotified`: handle lifecycle, document, Buffer, language, editor
  modification, UI-update, and dark-mode notifications. `READY` and
  `SHUTDOWN` are each sent once per load lifetime.
- `nppMessageProc`: optional plugin-specific synchronous messages; return zero
  for unsupported messages.

## Optional command state and notification tail

`nppGetCommandState(uint32_t command_index)` returns the bit flags
`NPP_PLUGIN_COMMAND_CHECKABLE` and `NPP_PLUGIN_COMMAND_CHECKED`. The index is
the position in `nppGetFuncsArray`. The host refreshes state before showing
the menu and after running a command. Without this export it uses the existing
initial state. Null command pointers represent separators.

`NPP_PLUGIN_NOTIFICATION_ZOOM` forwards editor zoom changes.
`NppPluginNotification::lines_added` is an optional signed tail field carrying
Scintilla's line-count delta. Check `struct_size` against its offset plus field
size before reading it. The original prefix and ABI version are unchanged.

## Optional editor context menu exports

Plugins that provide position-dependent editor actions may export both
`nppGetEditorContextMenu` and `nppExecuteEditorContextMenuCommand`. The host
passes the stable view identifier and clicked Scintilla byte position, copies
the returned UTF-8 labels immediately, and invokes the selected command ID
synchronously. A plugin must provide both symbols or the host ignores the
extension.

The returned array and labels must remain valid until the next context-menu
query or command callback. Items are flat commands or separators in ABI v1;
Qt menu objects and native handles never cross the plugin boundary.

## Host and platform information

`NppPluginHostInfo` is passed through `nppSetInfo` before command discovery. It
contains:

- operating-system family, system name, and system version;
- CPU architecture;
- application name and software version;
- plugin installation and writable configuration paths;
- UTF-8 current-file query, file-open, and logging callbacks;
- append-only raw document and selection callbacks with explicit byte counts;
- current Buffer/View identity, new-document, clipboard, and status callbacks.
- view-specific document access and Buffer placement in the permanent main and
  secondary editor views;
- line-background compare markers, first-visible-line synchronization, and
  line navigation.
- a platform-independent Scintilla message channel, Buffer-path lookup,
  current-file save, and Notepad++ menu-command execution.

Plugins should branch on `system_type` only where behavior is genuinely
platform-specific. They must check `struct_size` before reading fields added by
future ABI versions. `NppPluginHostInfo` may grow by appending fields. The
function array is contiguous, so `NppPluginFuncItem` must have the exact ABI v1
size; changing its layout requires a new ABI version.

## Build and layout

Define `NPP_PLUGIN_BUILD` while compiling the shared library and include only
`PluginInterface.h`. Use the platform library format selected by the host:
`.dll` on Windows, `.so` on Linux, and `.dylib` on macOS. The plugin folder and
binary base name must match. The host first probes the standard plugin binary
path. If it exports `nppGetPluginAbiVersion`, the new ABI owns that folder and
the legacy loader is skipped. If the symbol is absent, Windows falls back to
the original ABI. A binary exporting both interfaces is loaded through the new
ABI. The earlier distinct path remains a supported compatibility layout:

```text
plugins/<plugin-id>/cross-platform/windows/<plugin-id>.dll
plugins/<plugin-id>/cross-platform/linux/<plugin-id>.so
plugins/<plugin-id>/cross-platform/macos/<plugin-id>.dylib
```

The preferred layout is the normal platform layout:

```text
plugins/<plugin-id>/<plugin-id>.dll
plugins/<plugin-id>/linux/<plugin-id>.so
plugins/<plugin-id>/macos/<plugin-id>.dylib
```

Document callbacks are binary-safe: returned sizes exclude terminators and
replacement callbacks consume exactly the supplied byte count, including
embedded NUL bytes.

Views use stable integer identifiers: `0` is the main editor and `1` is the
secondary editor. Compare marker kinds are `0` added, `1` removed, `2` changed,
and `3` moved. Plugins must bounds-check the host structure before using these
append-only callbacks.

Callbacks run synchronously on the host UI thread in ABI v1. Plugins must not
allow exceptions to cross the C boundary, retain temporary callback buffers,
or call host functions after the shutdown notification.

`NppPluginNotification::text_utf8` is valid only for the duration of the
notification callback. Plugins that defer work must copy it. Scintilla pointer
parameters are valid only for synchronous in-process calls and must use the
public Scintilla ABI structures, never Qt or host-private objects.
