#ifndef ENCODINGMAPPER_H
#define ENCODINGMAPPER_H

#include <QString>

class EncodingMapper
{
public:
    static QString codecNameForCodePage(int codePage);
    static int codePageForName(const QString& encodingName);
    static QString resolveAlias(const QString& encodingAlias);
};

#endif // ENCODINGMAPPER_H
