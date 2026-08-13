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
- `nppBeNotified`: handle lifecycle notifications. Version 1 sends `READY`
  once and `SHUTDOWN` once.
- `nppMessageProc`: optional plugin-specific synchronous messages; return zero
  for unsupported messages.

## Host and platform information

`NppPluginHostInfo` is passed through `nppSetInfo` before command discovery. It
contains:

- operating-system family, system name, and system version;
- CPU architecture;
- application name and software version;
- plugin installation and writable configuration paths;
- UTF-8 current-file query, file-open, and logging callbacks.

Plugins should branch on `system_type` only where behavior is genuinely
platform-specific. They must check `struct_size` before reading fields added by
future ABI versions. `NppPluginHostInfo` may grow by appending fields. The
function array is contiguous, so `NppPluginFuncItem` must have the exact ABI v1
size; changing its layout requires a new ABI version.

## Build and layout

Define `NPP_PLUGIN_BUILD` while compiling the shared library and include only
`PluginInterface.h`. Use the platform library format selected by the host:
`.dll` on Windows, `.so` on Linux, and `.dylib` on macOS. The plugin folder and
binary base name must match. ABI v1 binaries use a distinct path so probing a
cross-platform plugin never executes an original Windows plugin DLL:

```text
plugins/<plugin-id>/cross-platform/windows/<plugin-id>.dll
plugins/<plugin-id>/cross-platform/linux/<plugin-id>.so
plugins/<plugin-id>/cross-platform/macos/<plugin-id>.dylib
```

Callbacks run synchronously on the host UI thread in ABI v1. Plugins must not
allow exceptions to cross the C boundary, retain temporary callback buffers,
or call host functions after the shutdown notification.
