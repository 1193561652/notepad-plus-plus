# Independent static Lexilla

Date: 2026-07-30

## Change

- Moved built-in lexer and lexlib sources out of the QScintilla source tree.
- Added the independent CMake static target `npp-lexilla`.
- Added static Lexilla catalogue entry points, including `CreateLexer`.
- Added the QScintilla-compatible `SCI_SETILEXER` message and ownership path.
- Routed built-in and UDL language installation through
  `ScintillaEditView::installLexer()`.
- Removed lexer, lexlib, Catalogue, and ExternalLexer sources from the
  QScintilla qmake project.
- Refreshed the staged QScintilla archive timestamp after successful custom
  builds so unchanged archive bytes do not cause perpetual rebuilds.

## Reason

Notepad++ v8.4.6 links Scintilla and Lexilla as separate static libraries and
has the application create lexer instances. The previous Qt integration
embedded Lexilla inside QScintilla and relied on `SCI_SETLEXERLANGUAGE`.

## Validation

- Independent Lexilla build: passed.
- Lexilla catalogue/create/release test: passed.
- Runtime external lexer installation, XML styling, and LexUser semantics:
  passed.
- Full Debug project build: passed.
- CTest: `29/29` passed.
- Windows Qt 5.12 compatibility exposed two `QString::SkipEmptyParts`
  qualifiers in `PluginArchiveExtractor`; these were corrected without changing
  plugin archive behavior so the full validation could complete.
