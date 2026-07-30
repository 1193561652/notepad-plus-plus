#include "TextFileCodec.h"
#include "EncodingMapper.h"
#include "uchardet.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QTextCodec>
#include <algorithm>

QString TextFileCodec::normalizedEncoding(const QString& encoding)
{
    QTextCodec* codec = QTextCodec::codecForName(encoding.toUtf8());
    if (!codec)
        return encoding;

    const QByteArray name = codec->name().toUpper();
    if (name == "UTF-8")
        return "UTF-8";
    if (name == "UTF-16LE")
        return "UTF-16LE";
    if (name == "UTF-16BE")
        return "UTF-16BE";
    return QString::fromLatin1(codec->name());
}

bool TextFileCodec::looksLikeUtf16(const QByteArray& data, bool* littleEndian)
{
    if (data.size() < 4 || (data.size() % 2) != 0)
        return false;

    const int sampleSize = qMin(data.size(), 4096);
    int evenZeros = 0;
    int oddZeros = 0;
    int pairs = 0;
    for (int i = 0; i + 1 < sampleSize; i += 2) {
        evenZeros += data.at(i) == '\0';
        oddZeros += data.at(i + 1) == '\0';
        ++pairs;
    }

    const int threshold = qMax(2, pairs / 3);
    if (oddZeros >= threshold && evenZeros < pairs / 10) {
        if (littleEndian)
            *littleEndian = true;
        return true;
    }
    if (evenZeros >= threshold && oddZeros < pairs / 10) {
        if (littleEndian)
            *littleEndian = false;
        return true;
    }
    return false;
}

bool TextFileCodec::looksBinary(const QByteArray& data)
{
    if (data.isEmpty())
        return false;

    const int sampleSize = qMin(data.size(), 8192);
    int controls = 0;
    for (int i = 0; i < sampleSize; ++i) {
        const unsigned char ch = static_cast<unsigned char>(data.at(i));
        if (ch == 0)
            return true;
        if (ch < 0x20 && ch != '\t' && ch != '\r' && ch != '\n' && ch != '\f')
            ++controls;
    }
    return controls * 20 > sampleSize;
}

TextEolMode TextFileCodec::detectEolMode(const QString& text)
{
    int crlf = 0;
    int lf = 0;
    int cr = 0;
    for (int i = 0; i < text.size(); ++i) {
        if (text.at(i) == QLatin1Char('\r')) {
            if (i + 1 < text.size() && text.at(i + 1) == QLatin1Char('\n')) {
                ++crlf;
                ++i;
            } else {
                ++cr;
            }
        } else if (text.at(i) == QLatin1Char('\n')) {
            ++lf;
        }
    }

    if (crlf == 0 && lf == 0 && cr == 0)
        return TextEolMode::Unknown;
    if (lf > crlf && lf >= cr)
        return TextEolMode::Unix;
    if (cr > crlf && cr > lf)
        return TextEolMode::Mac;
    return TextEolMode::Windows;
}

DecodedTextFile TextFileCodec::decode(const QByteArray& data,
                                      const QString& fallbackEncoding)
{
    TextDecodingOptions options;
    options.fallbackEncoding = fallbackEncoding;
    return decode(data, options);
}

QString TextFileCodec::declaredEncoding(const QByteArray& data,
                                        const QString& filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    const bool xml = suffix == "xml" || suffix == "xsl" || suffix == "xslt" ||
                     suffix == "xsd" || suffix == "svg";
    const bool html = suffix == "html" || suffix == "htm" ||
                      suffix == "shtml" || suffix == "xhtml";
    if (!xml && !html)
        return QString();

    const QString header = QString::fromLatin1(data.left(1024));
    QRegularExpression expression(
        xml
            ? QStringLiteral(
                "<\\?xml\\b[^>]*\\bencoding\\s*=\\s*[\"']([^\"']+)[\"']")
            : QStringLiteral(
                "<meta\\b[^>]*\\bcharset\\s*=\\s*[\"']?\\s*"
                "([^\"'\\s/>;]+)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = expression.match(header);
    if (!match.hasMatch())
        return QString();
    return EncodingMapper::resolveAlias(match.captured(1));
}

QString TextFileCodec::detectLegacyEncoding(const QByteArray& data)
{
    if (data.isEmpty())
        return QString();

    uchardet_t detector = uchardet_new();
    if (!detector)
        return QString();

    const QByteArray sample = data.left(128 * 1024 + 4);
    uchardet_handle_data(detector, sample.constData(), sample.size());
    uchardet_data_end(detector);
    const QString detected =
        QString::fromLatin1(uchardet_get_charset(detector)).trimmed();
    uchardet_delete(detector);

    // Keep the original Notepad++ safeguard. This detector frequently labels
    // unrelated UTF-8/ANSI input as TIS-620.
    if (detected.compare("TIS-620", Qt::CaseInsensitive) == 0)
        return QString();
    return EncodingMapper::resolveAlias(detected);
}

DecodedTextFile TextFileCodec::decode(
    const QByteArray& data, const TextDecodingOptions& options)
{
    DecodedTextFile result;
    result.encoding.clear();
    int bomSize = 0;

    if (data.startsWith("\xEF\xBB\xBF")) {
        result.encoding = "UTF-8";
        result.hasBom = true;
        bomSize = 3;
    } else if (data.startsWith("\xFF\xFE")) {
        result.encoding = "UTF-16LE";
        result.hasBom = true;
        bomSize = 2;
    } else if (data.startsWith("\xFE\xFF")) {
        result.encoding = "UTF-16BE";
        result.hasBom = true;
        bomSize = 2;
    } else {
        bool littleEndian = true;
        if (looksLikeUtf16(data, &littleEndian)) {
            result.encoding = littleEndian ? "UTF-16LE" : "UTF-16BE";
        } else {
            const QString declaration =
                declaredEncoding(data, options.filePath);
            if (!declaration.isEmpty()) {
                result.encoding = declaration;
                result.usesEncodingCookie = true;
            }

            QTextCodec::ConverterState utf8State;
            QTextCodec::codecForName("UTF-8")->toUnicode(
                data.constData(), data.size(), &utf8State);
            const bool validUtf8 = utf8State.invalidChars == 0;
            const bool asciiOnly = validUtf8 &&
                std::all_of(data.cbegin(), data.cend(), [](char ch) {
                    return static_cast<unsigned char>(ch) < 0x80;
                });

            if (result.encoding.isEmpty() && validUtf8 &&
                (!asciiOnly || options.openAnsiAsUtf8)) {
                result.encoding = "UTF-8";
                result.usesEncodingCookie = true;
            } else if (result.encoding.isEmpty() && options.detectEncoding) {
                result.encoding = detectLegacyEncoding(data);
                result.usesEncodingCookie = !result.encoding.isEmpty();
            }
            if (result.encoding.isEmpty()) {
                result.encoding = options.fallbackEncoding.isEmpty()
                    ? QString::fromLatin1(QTextCodec::codecForLocale()->name())
                    : options.fallbackEncoding;
            }
        }
    }

    if (result.encoding != "UTF-16LE" && result.encoding != "UTF-16BE" &&
        looksBinary(data)) {
        result.isBinary = true;
        result.encoding = "ISO-8859-1";
        result.text = QString::fromLatin1(data);
        result.eolMode = detectEolMode(result.text);
        result.isValid = true;
        return result;
    }

    QTextCodec* codec = QTextCodec::codecForName(result.encoding.toUtf8());
    if (!codec)
        return result;

    QTextCodec::ConverterState state;
    result.text = codec->toUnicode(
        data.constData() + bomSize, data.size() - bomSize, &state);
    if (state.invalidChars > 0)
        return result;

    result.encoding = normalizedEncoding(result.encoding);
    if (result.encoding == "UTF-8" && !result.hasBom)
        result.usesEncodingCookie = true;
    result.eolMode = detectEolMode(result.text);
    result.isValid = true;
    return result;
}

DecodedTextFile TextFileCodec::decodeAs(const QByteArray& data,
                                        const QString& encoding)
{
    DecodedTextFile result;
    result.encoding = normalizedEncoding(encoding);

    QTextCodec* codec = QTextCodec::codecForName(result.encoding.toUtf8());
    if (!codec)
        return result;

    int bomSize = 0;
    if (result.encoding == "UTF-8" && data.startsWith("\xEF\xBB\xBF")) {
        result.hasBom = true;
        bomSize = 3;
    } else if (result.encoding == "UTF-16LE" && data.startsWith("\xFF\xFE")) {
        result.hasBom = true;
        bomSize = 2;
    } else if (result.encoding == "UTF-16BE" && data.startsWith("\xFE\xFF")) {
        result.hasBom = true;
        bomSize = 2;
    }

    QTextCodec::ConverterState state;
    result.text = codec->toUnicode(
        data.constData() + bomSize, data.size() - bomSize, &state);
    if (state.invalidChars > 0)
        return result;

    result.isBinary =
        result.encoding != "UTF-16LE" && result.encoding != "UTF-16BE" &&
        looksBinary(data);
    result.eolMode = detectEolMode(result.text);
    result.usesEncodingCookie =
        !result.hasBom &&
        (result.encoding == "UTF-8" ||
         (result.encoding != "UTF-16LE" && result.encoding != "UTF-16BE"));
    result.isValid = true;
    return result;
}

bool TextFileCodec::encode(const QString& text, const QString& encoding,
                           bool hasBom, QByteArray* output,
                           QString* errorMessage)
{
    if (!output)
        return false;

    const QString normalized = normalizedEncoding(encoding);
    QTextCodec* codec = QTextCodec::codecForName(normalized.toUtf8());
    if (!codec) {
        if (errorMessage)
            *errorMessage = QString("Unknown encoding: %1").arg(encoding);
        return false;
    }

    QTextCodec::ConverterState state(
        QTextCodec::ConvertInvalidToNull | QTextCodec::IgnoreHeader);
    QByteArray encoded = codec->fromUnicode(text.constData(), text.size(), &state);
    int invalidChars = state.invalidChars;
    // Codec backends do not report every substitution consistently. Verify the
    // result on every platform so saving can never silently lose text.
    QTextCodec::ConverterState verificationState(
        QTextCodec::ConvertInvalidToNull | QTextCodec::IgnoreHeader);
    const QString verifiedText = codec->toUnicode(
        encoded.constData(), encoded.size(), &verificationState);
    invalidChars += verificationState.invalidChars;
    if (invalidChars > 0 || verifiedText != text) {
        if (errorMessage) {
            *errorMessage = QString("%1 character(s) cannot be represented in %2.")
                .arg(qMax(1, invalidChars))
                .arg(normalized);
        }
        return false;
    }

    output->clear();
    if (hasBom) {
        if (normalized == "UTF-8")
            output->append(QByteArray("\xEF\xBB\xBF", 3));
        else if (normalized == "UTF-16LE")
            output->append(QByteArray("\xFF\xFE", 2));
        else if (normalized == "UTF-16BE")
            output->append(QByteArray("\xFE\xFF", 2));
    }
    output->append(encoded);
    return true;
}

bool TextFileCodec::encodeLike(const QByteArray& original, const QString& text,
                               QByteArray* output, QString* errorMessage)
{
    const DecodedTextFile decoded = decode(original);
    if (!decoded.isValid || decoded.isBinary) {
        if (errorMessage)
            *errorMessage = "The source file encoding could not be determined.";
        return false;
    }
    return encode(text, decoded.encoding, decoded.hasBom, output, errorMessage);
}
