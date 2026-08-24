#include "MISC/TextFileCodec.h"
#include "EncodingMapper.h"
#include "ScintillaComponent/Buffer.h"

#include <QCoreApplication>
#include <QTextCodec>
#include <cstdio>

static bool check(bool condition, const char* message)
{
    if (!condition)
        std::fprintf(stderr, "FAILED: %s\n", message);
    return condition;
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    bool ok = true;

    const QString sample = QString::fromUtf8("第一行\r\n第二行\r\n");
    QByteArray utf8Bom;
    ok &= check(TextFileCodec::encode(sample, "UTF-8", true, &utf8Bom),
                "UTF-8 BOM encoding failed");
    DecodedTextFile decoded = TextFileCodec::decode(utf8Bom);
    ok &= check(decoded.isValid && decoded.text == sample,
                "UTF-8 BOM round trip failed");
    ok &= check(decoded.hasBom && decoded.encoding == "UTF-8",
                "UTF-8 BOM metadata failed");
    ok &= check(!decoded.usesEncodingCookie,
                "UTF-8 BOM must not use the encoding cookie");
    ok &= check(decoded.eolMode == TextEolMode::Windows,
                "UTF-8 CRLF detection failed");

    QByteArray utf16le;
    ok &= check(TextFileCodec::encode(sample, "UTF-16LE", true, &utf16le),
                "UTF-16LE encoding failed");
    decoded = TextFileCodec::decode(utf16le);
    ok &= check(decoded.isValid && decoded.text == sample,
                "UTF-16LE round trip failed");
    ok &= check(decoded.encoding == "UTF-16LE" && decoded.hasBom,
                "UTF-16LE metadata failed");
    ok &= check(decoded.eolMode == TextEolMode::Windows,
                "UTF-16LE CRLF detection failed");

    const QString unixText = QString::fromUtf8("甲\n乙\n");
    QByteArray utf16be;
    ok &= check(TextFileCodec::encode(unixText, "UTF-16BE", true, &utf16be),
                "UTF-16BE encoding failed");
    decoded = TextFileCodec::decode(utf16be);
    ok &= check(decoded.isValid && decoded.text == unixText,
                "UTF-16BE round trip failed");
    ok &= check(decoded.eolMode == TextEolMode::Unix,
                "UTF-16BE LF detection failed");

    QByteArray utf16NoBom;
    ok &= check(TextFileCodec::encode("alpha\r\nbeta\r\n", "UTF-16LE", false,
                                      &utf16NoBom),
                "UTF-16LE no-BOM encoding failed");
    decoded = TextFileCodec::decode(utf16NoBom);
    ok &= check(decoded.isValid && decoded.encoding == "UTF-16LE" &&
                !decoded.hasBom,
                "UTF-16LE no-BOM detection failed");
    ok &= check(decoded.eolMode == TextEolMode::Windows,
                "UTF-16LE no-BOM EOL detection failed");

    const QByteArray windows1252("caf\xE9\r\n", 6);
    decoded = TextFileCodec::decode(windows1252, "windows-1252");
    ok &= check(decoded.isValid && decoded.text == QString::fromUtf8("café\r\n"),
                "Legacy code page decoding failed");

    TextDecodingOptions xmlOptions;
    xmlOptions.filePath = QStringLiteral("declared.xml");
    const QByteArray declaredXml(
        "<?xml version=\"1.0\" encoding=\"windows-1252\"?>"
        "<x>caf\xE9</x>");
    decoded = TextFileCodec::decode(declaredXml, xmlOptions);
    ok &= check(decoded.isValid &&
                decoded.text.contains(QString::fromUtf8("caf\xC3\xA9")) &&
                decoded.encoding.compare(
                    QStringLiteral("windows-1252"),
                    Qt::CaseInsensitive) == 0,
                "XML encoding declaration was not honored");
    ok &= check(decoded.usesEncodingCookie,
                "Declared legacy encoding must use the encoding cookie");

    TextDecodingOptions utf8Options;
    decoded = TextFileCodec::decode(
        QByteArray("caf\xC3\xA9"), utf8Options);
    ok &= check(decoded.isValid && decoded.encoding == "UTF-8" &&
                !decoded.hasBom && decoded.usesEncodingCookie,
                "UTF-8 without BOM cookie metadata failed");

    QTextCodec* windows1251Codec =
        QTextCodec::codecForName("windows-1251");
    const QString russianText = QString::fromUtf8(
        "Это длинный русский текст для надежного определения кодировки. "
        "Он содержит достаточно кириллических букв и повторяется несколько "
        "раз. Это длинный русский текст для определения кодировки.");
    const QByteArray windows1251 =
        windows1251Codec ? windows1251Codec->fromUnicode(russianText)
                         : QByteArray();
    TextDecodingOptions detectedOptions;
    detectedOptions.openAnsiAsUtf8 = false;
    decoded = TextFileCodec::decode(windows1251, detectedOptions);
    ok &= check(windows1251Codec && decoded.isValid &&
                decoded.text == russianText,
                "uchardet legacy encoding detection failed");

    ok &= check(
        EncodingMapper::codePageForName(QStringLiteral("windows-1251")) ==
            1251 &&
        EncodingMapper::codecNameForCodePage(1251).compare(
            QStringLiteral("windows-1251"), Qt::CaseInsensitive) == 0,
        "Session code page mapping failed");

    decoded = TextFileCodec::decodeAs(
        QByteArray("caf\xC3\xA9\r\n", 7), "windows-1252");
    const QString reinterpreted =
        QStringLiteral("caf") + QChar(0x00C3) + QChar(0x00A9) +
        QStringLiteral("\r\n");
    ok &= check(decoded.isValid && decoded.text == reinterpreted,
                "Explicit code page reinterpretation failed");

    QByteArray rejected;
    QString error;
    ok &= check(!TextFileCodec::encode(QString::fromUtf8("中文"),
                                       "windows-1252", false,
                                       &rejected, &error) &&
                !error.isEmpty(),
                "Unrepresentable legacy encoding was not rejected");

    decoded = TextFileCodec::decode(QByteArray("\x01\x02\x00\x7F", 4));
    ok &= check(decoded.isValid && decoded.isBinary,
                "Binary detection failed");

    Buffer buffer(1);
    auto* first = reinterpret_cast<ScintillaEditView*>(quintptr(0x1));
    auto* second = reinterpret_cast<ScintillaEditView*>(quintptr(0x2));
    buffer.setView(first);
    buffer.addView(second);
    ok &= check(buffer.getView() == first && buffer.views().size() == 2,
                "Buffer view registration failed");
    buffer.removeView(first);
    ok &= check(buffer.getView() == second && buffer.views().size() == 1,
                "Buffer primary view promotion failed");
    buffer.setMetadataDirty(true);
    ok &= check(buffer.isDirty() && buffer.getTabLabel().endsWith(" *"),
                "Buffer metadata dirty state was not reflected in the tab");
    buffer.setDirty(false);
    ok &= check(!buffer.isDirty() && !buffer.getTabLabel().endsWith(" *"),
                "Buffer clean state did not clear metadata dirty state");
    BufferMapState mapState;
    mapState.firstVisibleDisplayLine = 42;
    mapState.width = 240;
    buffer.setMapState(mapState);
    buffer.setIndividualTabColour(3);
    buffer.setUsesEncodingCookie(true);
    ok &= check(buffer.mapState().firstVisibleDisplayLine == 42 &&
                buffer.mapState().width == 240 &&
                buffer.individualTabColour() == 3 &&
                buffer.usesEncodingCookie(),
                "Buffer session UI metadata failed");

    ok &= check(!Buffer::isLargeFileSize(
                    Buffer::LargeFileThreshold - 1) &&
                Buffer::isLargeFileSize(Buffer::LargeFileThreshold) &&
                Buffer::isLargeFileSize(Buffer::LargeFileThreshold + 1),
                "Large-file 200 MiB threshold boundaries failed");
    ok &= check(!Buffer::requiresHugeFileConfirmation(
                    std::numeric_limits<int>::max() -
                    (qint64(1) << 20)) &&
                Buffer::requiresHugeFileConfirmation(
                    std::numeric_limits<int>::max()),
                "Huge-file confirmation boundary failed");

    ok &= check(Buffer::detectLanguageFromTextBeginning(
                    QByteArray("#!/usr/bin/env python3\nprint('ok')\n")) ==
                    QStringLiteral("python"),
                "Extensionless Python shebang detection failed");
    ok &= check(Buffer::detectLanguageFromTextBeginning(
                    QByteArray("  <?xml version=\"1.0\"?>\n<root/>")) ==
                    QStringLiteral("xml"),
                "Extensionless XML prolog detection failed");
    ok &= check(Buffer::detectLanguageFromTextBeginning(
                    QByteArray("const value = 1;\n")).isEmpty(),
                "Ordinary text must not be guessed heuristically");

    return ok ? 0 : 1;
}
