#ifndef USERDEFINEDLEXER_H
#define USERDEFINEDLEXER_H

#include <Qsci/qscilexer.h>
#include "Parameters.h"

class UserDefinedLexer : public QsciLexer
{
public:
    explicit UserDefinedLexer(const UserLangDesc& language, QObject* parent = nullptr);

    const char* language() const override;
    const char* lexer() const override;
    const char* keywords(int set) const override;
    bool caseSensitive() const override;
    QString description(int style) const override;
    void refreshProperties() override;

private:
    void applyConfiguredStyles();

    UserLangDesc _language;
    QByteArray _languageName;
    QByteArray _keywordLists[28];
};

#endif // USERDEFINEDLEXER_H
