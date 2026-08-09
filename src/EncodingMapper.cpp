#include "EncodingMapper.h"

#include <QTextCodec>

struct EncodingEntry {
    int codePage;
    const char* codecName;
};

static const EncodingEntry encodingEntries[] = {
    {1250, "windows-1250"}, {1251, "windows-1251"},
    {1252, "windows-1252"}, {1253, "windows-1253"},
    {1254, "windows-1254"}, {1255, "windows-1255"},
    {1256, "windows-1256"}, {1257, "windows-1257"},
    {1258, "windows-1258"},
    {28591, "ISO-8859-1"}, {28592, "ISO-8859-2"},
    {28593, "ISO-8859-3"}, {28594, "ISO-8859-4"},
    {28595, "ISO-8859-5"}, {28596, "ISO-8859-6"},
    {28597, "ISO-8859-7"}, {28598, "ISO-8859-8"},
    {28599, "ISO-8859-9"}, {28600, "ISO-8859-10"},
    {28603, "ISO-8859-13"}, {28604, "ISO-8859-14"},
    {28605, "ISO-8859-15"}, {28606, "ISO-8859-16"},
    {437, "IBM 437"}, {720, "IBM 720"}, {737, "IBM 737"},
    {775, "IBM 775"}, {850, "IBM 850"}, {852, "IBM 852"},
    {855, "IBM 855"}, {857, "IBM 857"}, {858, "IBM 858"},
    {860, "IBM 860"}, {861, "IBM 861"}, {862, "IBM 862"},
    {863, "IBM 863"}, {865, "IBM 865"}, {866, "IBM 866"},
    {869, "IBM 869"},
    {950, "Big5"}, {936, "GB18030"}, {932, "Shift-JIS"},
    {949, "windows-949"}, {51949, "EUC-KR"}, {51932, "EUC-JP"},
    {874, "windows-874"}, {10007, "macintosh"},
    {21866, "KOI8-U"}, {20866, "KOI8-R"},
    {65001, "UTF-8"}, {1200, "UTF-16LE"}, {1201, "UTF-16BE"}
};

QString EncodingMapper::codecNameForCodePage(int codePage)
{
    for (const EncodingEntry& entry : encodingEntries) {
        if (entry.codePage == codePage &&
            QTextCodec::codecForName(entry.codecName)) {
            return QString::fromLatin1(entry.codecName);
        }
    }
    return QString();
}

QString EncodingMapper::resolveAlias(const QString& encodingAlias)
{
    const QByteArray alias = encodingAlias.trimmed().toLatin1();
    QTextCodec* codec = QTextCodec::codecForName(alias);
    if (!codec)
        return QString();

    const QString canonical = QString::fromLatin1(codec->name());
    if (canonical.compare("UTF-16", Qt::CaseInsensitive) == 0)
        return "UTF-16LE";
    return canonical;
}

int EncodingMapper::codePageForName(const QString& encodingName)
{
    const QString canonical = resolveAlias(encodingName);
    if (canonical.isEmpty())
        return -1;

    for (const EncodingEntry& entry : encodingEntries) {
        QTextCodec* codec = QTextCodec::codecForName(entry.codecName);
        if (codec && QString::fromLatin1(codec->name()).compare(
                canonical, Qt::CaseInsensitive) == 0) {
            return entry.codePage;
        }
    }
    return -1;
}
