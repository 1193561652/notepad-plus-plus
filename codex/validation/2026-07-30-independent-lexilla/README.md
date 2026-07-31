# Independent Lexilla validation

Date: 2026-07-30

## Environment

- Windows
- Qt 5.12.12
- MinGW-w64 GCC 7.3
- Debug CMake build

## Build

- Built `npp-lexilla` as an independent static library.
- Rebuilt the project-specific QScintilla archive without lexer, lexlib,
  Catalogue, or ExternalLexer sources.
- Built the main application, runtime capture tools, updater, and all tests.

## Behavior

- The Lexilla catalogue contains more than 100 built-in lexer entries.
- `cpp`, `xml`, `errorlist`, and `user` are created and released through
  `CreateLexer`.
- Unknown lexer names return null.
- A built-in editor language reports `SCLEX_AUTOMATIC`, proving it was
  installed as an external lexer instance rather than through the internal
  QScintilla Catalogue.
- XML content receives `SCE_H_TAG` styling.
- The v8.4.6 LexUser engine preserves UDL keyword style behavior.

## Static boundary

MinGW `nm` confirms `lmCPP`, `lmUserDefine`, and `CreateLexer` are present in
`libnpp-lexilla.a` and absent from
`libqscintilla2_qt5_npp.a`.

## Result

```text
100% tests passed, 0 tests failed out of 29
```
