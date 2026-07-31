# Scintilla 5 Qt migration baseline

Date: 2026-07-30

Status: approved plan, not yet implemented

## Decision

Replace QScintilla with the official Qt platform implementation shipped in the
Notepad++ v8.4.6 source baseline.

The pinned versions are:

- Scintilla 5.3.0, identified by
  `notepad-plus-plus-v8.4.6/scintilla/version.txt` value `530`.
- Lexilla 5.1.9, identified by
  `notepad-plus-plus-v8.4.6/lexilla/version.txt` value `519`.
- The Boost.Regex integration and reduced Boost 1.78.0 source shipped by
  Notepad++ v8.4.6.

Do not substitute the newest Scintilla or Lexilla release during this
migration. A later upstream upgrade is a separate decision after v8.4.6
behavior parity has been established.

## Required architecture

The target library and call direction is:

```text
Notepad++ Qt business logic
    -> ScintillaEditView compatibility adapter
    -> official Scintilla 5.3.0 ScintillaEditBase for Qt
    -> Scintilla 5.3.0 core

ScintillaEditView
    -> Lexilla 5.1.9 CreateLexer(name)
    -> SCI_SETILEXER(ILexer5*)
    -> Scintilla owns and releases the lexer
```

Build Scintilla and Lexilla as separate static libraries, as the original
Notepad++ solution builds `libscintilla.lib` and `liblexilla.lib`.

## Source policy

- Import the exact `scintilla/` and `lexilla/` source trees from the v8.4.6
  reference into the main repository.
- Preserve their `version.txt`, licenses, source layout, and original Qt
  platform files.
- Keep project-specific changes out of imported upstream files whenever an
  adapter or build definition can provide the same result.
- Record every unavoidable upstream-file patch in an origin/patch manifest.
- Builds must not depend on the sibling reference checkout.

## API policy

- Keep `ScintillaEditView::execute(message, wParam, lParam)` as the primary
  application boundary, matching the original Notepad++ usage.
- Use Scintilla's `SCI_*`, `SCN_*`, `SC_*`, and `SCE_*` definitions directly.
- Replace `QsciLexer`, `QsciDocument`, `QsciPrinter`, `QsciMacro`, and
  `QsciAPIs` behavior with project adapters or direct Scintilla messages.
- Do not create a second high-level editor API that obscures the Scintilla
  message contract.

## Language standard

The v8.4.6 Qt project files select C++17 (`c++1z`) and their public headers use
C++17 library types. The preferred migration raises the main build to C++17.
Back-porting the official Qt platform source to C++14 is rejected because it
would create a project-specific Scintilla fork before behavior parity exists.

## Compatibility boundary

The migration targets source and runtime behavior compatibility with Notepad++
v8.4.6. It does not promise Win32 plugin binary compatibility. Plugin ABI
adaptation remains a separate project scope, but the real `ILexer5` boundary
will remove the current lexer ABI obstacle.
