# QScintilla Integration

The application builds a project-specific static QScintilla library from
`third_party/qscintilla/`.

`third_party/qscintilla/src/npp-qscintilla-static.pro`:

- enables `SCI_OWNREGEX`;
- compiles the Notepad++ Boost.Regex adapter from `third_party/boostregex/`;
- does not compile Lexilla lexer or lexlib sources.

CMake separately builds `npp-lexilla` from `third_party/lexilla/`, including
the v8.4.6 LexUser lexer. `ScintillaEditView` calls `CreateLexer()` and passes
the returned instance through `SCI_SETILEXER`; Scintilla owns and releases that
instance. This mirrors the original Notepad++ static library boundary while
retaining the QScintilla-compatible `ILexer` ABI.

`cmake/BuildNppQScintilla.cmake` invokes qmake with the same compiler family as
the main CMake build and stages the resulting static archive in the build tree.
No generated QScintilla files are written to the source tree.
