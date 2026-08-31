# Ubuntu FHS layout and runtime paths

The Debian package now uses the conventional Ubuntu filesystem layout:

- `/usr/bin/notepad++` for the application;
- `/usr/share/notepad-plus-plus-qt/` for localization and Function List data;
- `/usr/libexec/notepad-plus-plus-qt/` for the plugin updater;
- freedesktop application and icon locations under `/usr/share`.

`RuntimePathResolver` centralizes resource, plugin and updater lookup. It checks
the executable directory first so Windows and portable distributions retain the
original Notepad++ relative layout. A normal Unix installation then checks its
relocatable sibling `share`/`lib` paths and configured install paths. User plugin
writes use the XDG data directory; `doLocalConf.xml` keeps portable plugin reads
and writes beside the executable. Configuration files remain managed separately
by `ConfigPathResolver` because `%APPDATA%` and XDG config paths are intentionally
platform-specific.

The Debian package intentionally contains no plugins.
This layout change is released as Debian revision `8.4.6-qt2`.

## Audited platform differences

Six path/configuration classes were checked:

| Class | Windows or portable deployment | Ubuntu system deployment | Management |
| --- | --- | --- | --- |
| Executable | package directory, `.exe` | `/usr/bin/notepad++` | CMake target/install rules |
| Shared XML data | beside executable | `/usr/share/notepad-plus-plus-qt` | `RuntimePathResolver` |
| Plugin binaries | `plugins/` beside executable | XDG user data plus optional `/usr/lib` system plugins | `RuntimePathResolver` |
| Plugin updater | beside executable | `/usr/libexec/notepad-plus-plus-qt` | `RuntimePathResolver` |
| User configuration | `%APPDATA%/Notepad++` | XDG config directory | `ConfigPathResolver` |
| Shell integration | NSIS shortcuts/file associations | desktop entry and hicolor icon | platform installers |

The two path resolvers are intentionally separate: runtime assets may be
read-only and shared between users, while configuration must be writable and
per-user. Platform-specific separators and case rules remain delegated to Qt;
call sites no longer construct installation paths themselves.
