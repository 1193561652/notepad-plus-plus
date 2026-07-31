// Static Lexilla entry points for the QScintilla-compatible ILexer ABI.

#include "Lexilla.h"

#include <cstring>

#include "LexerModule.h"
#include "Catalogue.h"

using namespace Scintilla;

extern "C" {

ILexer *NPP_LEXILLA_CALL CreateLexer(const char *name)
{
    const LexerModule *module = Catalogue::Find(name);
    return module ? module->Create() : nullptr;
}

int NPP_LEXILLA_CALL GetLexerCount()
{
    return static_cast<int>(Catalogue::Count());
}

void NPP_LEXILLA_CALL GetLexerName(
    unsigned int index, char *name, int bufferLength)
{
    if (!name || bufferLength <= 0)
        return;

    name[0] = '\0';
    const char *lexerName = Catalogue::Name(index);
    if (!lexerName)
        return;

    const size_t length = std::strlen(lexerName);
    if (length < static_cast<size_t>(bufferLength))
        std::memcpy(name, lexerName, length + 1);
}

const char *NPP_LEXILLA_CALL LexerNameFromID(int identifier)
{
    const LexerModule *module = Catalogue::Find(identifier);
    return module ? module->languageName : nullptr;
}

}
