// Lexilla interface used by the statically linked Notepad++ lexer library.

#ifndef NPP_LEXILLA_H
#define NPP_LEXILLA_H

#include "ILexer.h"

#if defined(_WIN32)
#define NPP_LEXILLA_CALL __stdcall
#else
#define NPP_LEXILLA_CALL
#endif

extern "C" {

Scintilla::ILexer *NPP_LEXILLA_CALL CreateLexer(const char *name);
int NPP_LEXILLA_CALL GetLexerCount();
void NPP_LEXILLA_CALL GetLexerName(
    unsigned int index, char *name, int bufferLength);
const char *NPP_LEXILLA_CALL LexerNameFromID(int identifier);

}

#endif
