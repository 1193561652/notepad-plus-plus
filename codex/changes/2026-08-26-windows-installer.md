# Windows installer

The Windows package now uses CPack's NSIS generator while retaining the
original Notepad++ installer responsibilities that apply to the Qt port:

- license and destination pages;
- required application component and optional localization components;
- Start Menu and Desktop shortcuts;
- finish-page launch option;
- Apps & Features registration and uninstaller;
- program-relative `localization/` and `functionList/` directories;
- the matching Qt and MinGW runtime files.

English is the required base localization, as in the v8.4.6 installer. Every
other official language XML is initially unselected under the Localization
component group. NSIS supplies localized installer UI strings according to
the Windows locale; installed application languages remain independent
components and are selected later through the original Notepad++ language
loading flow.

Shared package metadata and localization-component generation live in
`installer_common/`. The official XML payload remains solely under
`installer_common/nativeLang/`; platform installer directories do not duplicate it.
The Ubuntu package now consumes the same helper while continuing to install
all languages in its monolithic package.

The Qt port does not bundle original plugins, the original GUP online updater,
the Explorer shell extension, or file-association UI in this initial Windows
installer. Those features require separate compatibility and release-policy
decisions. Windows installer generation and installation are intentionally
left for the requested Windows-side test pass.
