#ifndef TEXTFILECODEC_H
#define TEXTFILECODEC_H

#include <QByteArray>
#include <QString>

enum class TextEolMode {
    Windows = 0,
    Mac = 1,
    Unix = 2,
    Unknown = 3
};

struct DecodedTextFile {
    QString text;
    QString encoding = "UTF-8";
    bool hasBom = false;
    bool usesEncodingCookie = false;
    bool isBinary = false;
    bool isValid = false;
    TextEolMode eolMode = TextEolMode::Unknown;
};

struct TextDecodingOptions {
    QString fallbackEncoding;
    QString filePath;
    bool detectEncoding = true;
    bool openAnsiAsUtf8 = true;
};

class TextFileCodec
{
public:
    static DecodedTextFile decode(const QByteArray& data,
                                  const QString& fallbackEncoding = QString());
    static DecodedTextFile decode(const QByteArray& data,
                                  const TextDecodingOptions& options);
    static DecodedTextFile decodeAs(const QByteArray& data,
                                    const QString& encoding);
    static bool encode(const QString& text, const QString& encoding,
                       bool hasBom, QByteArray* output,
                       QString* errorMessage = nullptr);
    static bool encodeLike(const QByteArray& original, const QString& text,
                           QByteArray* output,
                           QString* errorMessage = nullptr);
    static TextEolMode detectEolMode(const QString& text);
    static QString normalizedEncoding(const QString& encoding);

private:
    static QString declaredEncoding(const QByteArray& data,
                                    const QString& filePath);
    static QString detectLegacyEncoding(const QByteArray& data);
    static bool looksLikeUtf16(const QByteArray& data, bool* littleEndian);
    static bool looksBinary(const QByteArray& data);
};

#endif // TEXTFILECODEC_H
