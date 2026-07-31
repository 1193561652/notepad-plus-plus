# Scintilla 5 Qt module

Status: target architecture; migration not yet implemented

## Baseline

- Scintilla 5.3.0 from Notepad++ v8.4.6.
- Lexilla 5.1.9 from Notepad++ v8.4.6.
- Official `scintilla/qt/ScintillaEditBase` platform widget.
- Static `npp-scintilla-qt` and `npp-lexilla` libraries.
- Original Boost.Regex adapter compiled with `SCI_OWNREGEX`.

## Application boundary

`ScintillaEditView` remains the single application-facing editor adapter. Its
primary operation is the original-style:

```cpp
execute(SCI_MESSAGE, wParam, lParam)
```

Qt event translation, UTF-8 conversion and convenience helpers belong in this
adapter. Buffer, session, search, configuration and MainWindow logic must not
depend on `ScintillaQt` internals.

## Lexer boundary

```text
language/configuration
  -> lexer name
  -> Lexilla::CreateLexer
  -> ILexer5*
  -> SCI_SETILEXER
  -> Scintilla ownership
```

This is the same logical path as original Notepad++ v8.4.6.

## Document boundary

Buffer owns the application document relationship. Shared editor documents use
`SCI_CREATEDOCUMENT`, `SCI_ADDREFDOCUMENT`, `SCI_RELEASEDOCUMENT` and
`SCI_SETDOCPOINTER`. Main/sub views must not introduce an additional Qt-side
document ownership model.

## Detailed references

- Decision:
  `codex/decisions/2026-07-30-scintilla5-qt-baseline.md`
- Migration plan:
  `codex/analysis/2026-07-30-scintilla5-qt-migration-plan.md`
- Current transitional Lexilla state:
  `codex/analysis/2026-07-30-lexilla-architecture.md`
