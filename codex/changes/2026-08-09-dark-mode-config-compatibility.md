# Dark Mode Configuration Compatibility

## Problem

Notepad++ v8.4.6 stores the dark-mode switch in
`config.xml/GUIConfigs/GUIConfig[@name='DarkMode']/@enable`. The Qt port did
not consume that node and instead persisted an `Editor/darkMode` value in
`qtState.ini`. A stale Qt value could therefore override the shared XML and
make the original and ported applications display different modes.

## Resolution

- `NppParameters::feedGUIConfig()` reads the original `DarkMode@enable` value.
- `writeConfigXml()` updates or creates the original `DarkMode` element while
  retaining its color attributes and unknown data.
- `loadQtState()` no longer lets private state override the XML value.
- `writeQtState()` removes the obsolete private key.
- Configuration corpus tests cover a conflicting stale private value, XML
  read/write behavior, missing-node creation, and private-key cleanup.

## Compatibility Result

Qt-written light and dark configurations were opened by the original v8.4.6
binary through `-settingsDir`. Both loaded without errors and selected the
expected mode.
