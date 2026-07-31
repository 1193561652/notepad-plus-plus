# Lexilla Integration

This directory is the project's separately built static Lexilla library.

## Contents

- `lexers/`: built-in lexer implementations from the QScintilla-compatible
  Scintilla source baseline.
- `lexlib/`: lexer support library.
- `src/Catalogue.*`: static lexer catalogue.
- `src/Lexilla.cpp` and `include/Lexilla.h`: static `CreateLexer` and catalogue
  entry points.
- `LexUser.cxx`: the Notepad++ v8.4.6 user-defined-language lexer.

## Runtime flow

1. CMake builds these sources as `npp-lexilla`.
2. `ScintillaEditView` maps a Notepad++ language to a Lexilla name.
3. The application calls `CreateLexer(name)`.
4. The returned `Scintilla::ILexer*` is passed through `SCI_SETILEXER`.
5. Scintilla owns the instance and calls `Release()` when it is replaced or
   when the document lexer state is destroyed.

QScintilla retains the Qt lexer wrapper only for styles, keyword sets,
properties, and auto-completion metadata. It does not create the lexer engine.

## ABI note

QScintilla 2.13.3 uses the older `Scintilla::ILexer` ABI. The project therefore
implements the original Notepad++ library and ownership structure on that ABI.
It is not binary-compatible with v8.4.6 external `ILexer5` libraries. Exact
external Lexilla ABI compatibility requires a future Scintilla/QScintilla core
upgrade and is part of the deferred plugin boundary.
