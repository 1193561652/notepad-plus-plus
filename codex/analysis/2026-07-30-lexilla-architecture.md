# Lexilla architecture analysis

Date: 2026-07-30

## Original v8.4.6

The original solution builds `lexilla/src/Lexilla.vcxproj` as the independent
static library `liblexilla.lib`. It compiles the Lexilla lexer catalogue,
`lexers/*.cxx`, `lexlib/*.cxx`, `Lexilla.cxx`, and the Notepad++ lexer modules.
`notepad++.exe` links both `libscintilla.lib` and `liblexilla.lib`.

The internal-language call chain is:

1. `ScintillaEditView::setLexerFromLangID()` reads the language's lexer name.
2. The application calls `CreateLexer(lexerName)`.
3. Lexilla returns a newly allocated `ILexer5*`.
4. The application sends it with `SCI_SETILEXER`.
5. Scintilla owns and releases the lexer instance.

The same pattern is used by the scratch Scintilla document during file loading.
External lexer plugins provide another `CreateLexer` function pointer but use
the same `SCI_SETILEXER` boundary.

## Previous Qt implementation

QScintilla's upstream project compiled all `scintilla/lexers` and `lexlib`
sources into the QScintilla archive. `QsciScintilla::setLexer()` sent
`SCI_SETLEXERLANGUAGE`, and Scintilla's internal `Catalogue` created the lexer.
`LexUser.cxx` was also compiled into that archive.

This produced correct highlighting but collapsed the original Scintilla and
Lexilla library boundary. The application did not call `CreateLexer`, so lexer
creation and ownership differed from Notepad++.

## Implemented structure

- `npp-qscintilla` contains the Qt editor widget, Scintilla core, and
  `SCI_OWNREGEX` Boost.Regex backend. It no longer compiles lexer, lexlib,
  Catalogue, or ExternalLexer sources.
- `npp-lexilla` is an independent CMake static library containing lexer,
  lexlib, catalogue, static Lexilla entry points, and `LexUser.cxx`.
- `ScintillaEditView::installLexer()` calls `CreateLexer` and passes the result
  to the QScintilla adapter's `SCI_SETILEXER` path.
- The adapted Scintilla `LexState::SetILexer()` releases the old instance,
  accepts ownership of the new instance, updates the interface version, and
  notifies the document that its lexer changed.
- QsciLexer objects remain Qt-side configuration wrappers. They provide style,
  keyword, property, and auto-completion metadata but do not create the engine.

## Compatibility boundary

QScintilla 2.13.3 contains the older `Scintilla::ILexer` ABI and has no native
`SCI_SETILEXER`. The project adds message 4033 and equivalent ownership
semantics to that core. The independent library therefore uses the bundled
QScintilla-compatible lexer baseline, plus the adapted v8.4.6 LexUser.

This matches the original static library boundary and runtime call direction,
but it does not make external v8.4.6 `ILexer5` binaries ABI-compatible. Exact
external lexer compatibility requires upgrading the editor core and remains in
the deferred plugin scope.

## Planned final core

The final editor-core direction is now recorded: replace QScintilla with the
official Scintilla 5.3.0 Qt platform implementation and replace this
transitional old-ABI Lexilla build with exact Lexilla 5.1.9 from the Notepad++
v8.4.6 baseline. The target will use real `ILexer5` and the original
`CreateLexer` / `SCI_SETILEXER` ownership contract.

See:

- `codex/decisions/2026-07-30-scintilla5-qt-baseline.md`
- `codex/analysis/2026-07-30-scintilla5-qt-migration-plan.md`

## Verification

- `lexilla-integration-tests` enumerates the catalogue and creates/releases
  `cpp`, `xml`, `errorlist`, and `user`.
- `large-file-mode-tests` verifies that a built-in language installs an
  external lexer instance (`SCI_GETLEXER == SCLEX_AUTOMATIC`), XML is styled,
  and the original LexUser keyword semantics remain active.
- Symbol inspection confirms `lmCPP`, `lmUserDefine`, and `CreateLexer` are in
  `libnpp-lexilla.a` and absent from the QScintilla archive.
