#include <ILexer.h>
#include <Lexilla.h>
#include <SciLexer.h>
#include <cstdio>

namespace {

bool check(bool condition, const char* message)
{
    if (!condition)
        std::fprintf(stderr, "FAILED: %s\n", message);
    return condition;
}

bool verifyLexer(const char* name)
{
    Scintilla::ILexer5* lexer = CreateLexer(name);
    const bool ok = check(lexer != nullptr, name);
    if (lexer)
        lexer->Release();
    return ok;
}

}

int main()
{
    bool ok = true;
    ok &= check(GetLexerCount() > 100, "Lexilla catalogue is incomplete");
    ok &= verifyLexer("cpp");
    ok &= verifyLexer("xml");
    ok &= verifyLexer("errorlist");
    ok &= verifyLexer("user");
    ok &= check(CreateLexer("not-a-lexer") == nullptr,
                "Unknown lexer unexpectedly resolved");

    char firstName[64] = {};
    GetLexerName(0, firstName, sizeof(firstName));
    ok &= check(firstName[0] != '\0',
                "Lexilla catalogue did not expose lexer names");
    ok &= check(LexerNameFromID(SCLEX_CPP) != nullptr,
                "Lexer ID lookup failed");
    return ok ? 0 : 1;
}
