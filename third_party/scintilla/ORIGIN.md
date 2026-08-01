# Scintilla Source Origin

- Upstream baseline: Scintilla 5.3.0 (`version.txt` = `530`).
- Project baseline: Notepad++ v8.4.6.
- Imported from: `notepad-plus-plus-v8.4.6/scintilla/`.
- Integration: official `qt/ScintillaEditBase` plus the Scintilla core are
  compiled into the static CMake target `npp-scintilla-qt`.
- Notepad++'s Boost.Regex adapter is compiled into the same target with
  `SCI_OWNREGEX`.

Keep this tree version-aligned with the Notepad++ baseline. Do not update it
independently without a recorded compatibility decision and full editor tests.
