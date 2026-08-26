# Modeless dialogs and empty-line caret

## Scope

- Changed the Qt counterparts of original Notepad++ modeless windows to keep
  the main editor enabled. This includes About, Preferences, Go To Line,
  Column Editor, User Defined Language, Style Configurator, Plugins Admin,
  Run, hash tools, Debug Info, and informational result windows.
- Column Editor, User Defined Language, and Style Configurator are persistent
  main-window-owned dialogs: closing hides them, reopening reuses the same
  instance, and accepted work runs from Qt signals without a nested event loop.
- Kept operation-confirmation dialogs, printing, shortcut mapping, the Windows
  document list, and system file pickers modal where their workflow requires a
  blocking decision.
- Made the Qt Scintilla platform adapter explicitly restore editor focus on a
  mouse press. This covers the viewport focus-transfer path that could leave
  the main caret inactive after closing or switching away from a dialog.
- Kept the original one-pixel caret on Windows, while using a two-pixel minimum
  on Linux. On the Qt/X11 surface, a one-pixel caret at column zero can coincide
  with the left text boundary and be clipped; the second pixel remains inside
  the text area. This also covers empty lines, whose caret is always at column
  zero.

## Ubuntu verification

- `cmake --build build-deb --target notepadpp-qt large-file-mode-tests ui-parity-capture -j2`
- `ctest --test-dir build-deb --output-on-failure`: 35/35 passed.
- Runtime coverage checks that About and Preferences are non-modal and that a
  focused caret is painted on internal, final, and document-only empty lines.
  It also checks the Linux column-zero width and that clicking an empty line
  after focus loss restores both Qt and Scintilla focus.
- UI runtime coverage closes and reopens Column Editor, User Defined Language,
  and Style Configurator, checking that each remains non-modal and reuses the
  same instance.
