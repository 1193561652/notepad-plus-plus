#ifndef USERDEFINEDLEXER_H
#define USERDEFINEDLEXER_H

#include "Parameters.h"

class ScintillaEditView;

class UserDefinedLexer
{
public:
    static void configure(ScintillaEditView& view,
                          const UserLangDesc& language);
};

#endif // USERDEFINEDLEXER_H
