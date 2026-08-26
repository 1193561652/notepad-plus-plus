# Windows installer

The Windows package uses CPack's NSIS generator. It keeps the original
Notepad++ installer essentials: a license page, destination selection,
component selection, Start Menu/Desktop shortcuts, a finish-page launch
option, Apps & Features registration, and an uninstaller.

English is installed as the required base language. The other official
v8.4.6 XML files appear as initially unselected entries in the
`Localization` component group and are installed under `localization/`.
Their payload and component-generation rules are shared through
`../installer_common/`; no language files are duplicated here.

From a Windows PowerShell configured with the matching Qt and MinGW kit:

```powershell
cmake -S . -B build-windows-package -G "MinGW Makefiles" `
  -DCMAKE_BUILD_TYPE=Release `
  -DBUILD_TESTING=OFF `
  -DCMAKE_PREFIX_PATH="$env:QT_ROOT" `
  -DCMAKE_CXX_COMPILER="$env:MINGW_ROOT\bin\g++.exe" `
  -DCMAKE_MAKE_PROGRAM="$env:MINGW_ROOT\bin\mingw32-make.exe"
cmake --build build-windows-package -j 4
cpack --config build-windows-package\CPackConfig.cmake -G NSIS
```

NSIS 3.03 or newer must provide `makensis.exe` on `PATH`.
