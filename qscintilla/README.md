# QScintilla Integration

The application builds a project-specific static QScintilla library from
`third_party/qscintilla/`.

`third_party/qscintilla/src/npp-qscintilla-static.pro`:

- enables `SCI_OWNREGEX`;
- compiles the Notepad++ Boost.Regex adapter from `third_party/boostregex/`;
- compiles the v8.4.6 LexUser lexer from `third_party/lexilla/`.

`cmake/BuildNppQScintilla.cmake` invokes qmake with the same compiler family as
the main CMake build and stages the resulting static archive in the build tree.
No generated QScintilla files are written to the source tree.
