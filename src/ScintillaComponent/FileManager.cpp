// FileManager.cpp - 文件管理器实现
// 对应原版 ScintillaComponent/Buffer.cpp 中的 FileManager 实现

#include "FileManager.h"
#include "ScintillaComponent/ScintillaEditView.h"
#include "MISC/TextFileCodec.h"
#include <QFile>
#include <QSaveFile>
#include <QFileInfo>
#include <QDir>
#include <QScopedPointer>
#include <QTextCodec>
#include <QTextDecoder>
#include <QTextEncoder>
#include <QRegularExpression>

namespace {

constexpr qint64 IoBlockSize = 128 * 1024 + 4;

int bomSizeFor(const QString& encoding, const QByteArray& sample)
{
    if (encoding == QStringLiteral("UTF-8") &&
        sample.startsWith("\xEF\xBB\xBF"))
        return 3;
    if (encoding == QStringLiteral("UTF-16LE") &&
        sample.startsWith("\xFF\xFE"))
        return 2;
    if (encoding == QStringLiteral("UTF-16BE") &&
        sample.startsWith("\xFE\xFF"))
        return 2;
    return 0;
}

bool writeAll(QIODevice& device, const QByteArray& data)
{
    qint64 written = 0;
    while (written < data.size()) {
        const qint64 amount =
            device.write(data.constData() + written, data.size() - written);
        if (amount <= 0)
            return false;
        written += amount;
    }
    return true;
}

quint16 utf16Unit(const char* bytes, bool littleEndian)
{
    const quint8 first = static_cast<quint8>(bytes[0]);
    const quint8 second = static_cast<quint8>(bytes[1]);
    return littleEndian
        ? quint16(first | (quint16(second) << 8))
        : quint16((quint16(first) << 8) | second);
}

QByteArray takeIncompleteUtf16Tail(
    QByteArray* input, bool littleEndian)
{
    QByteArray tail;
    if (input->size() % 2 != 0) {
        tail.prepend(input->right(1));
        input->chop(1);
    }
    if (input->size() >= 2) {
        const quint16 lastUnit =
            utf16Unit(input->constData() + input->size() - 2, littleEndian);
        if (lastUnit >= 0xD800 && lastUnit <= 0xDBFF) {
            tail.prepend(input->right(2));
            input->chop(2);
        }
    }
    return tail;
}

} // namespace

FileManager& FileManager::getInstance()
{
    static FileManager instance;
    return instance;
}

bool FileManager::backupFileBeforeSave(const QString& sourcePath, int mode,
                                       bool useCustomDirectory,
                                       const QString& customDirectory,
                                       const QString& timestamp,
                                       QString* backupPath,
                                       QString* errorMessage)
{
    if (backupPath)
        backupPath->clear();
    if (mode == 0 || !QFileInfo::exists(sourcePath))
        return true;

    QString directory = useCustomDirectory && !customDirectory.isEmpty()
        ? customDirectory
        : QFileInfo(sourcePath).absolutePath();
    directory = QDir::fromNativeSeparators(directory);
    QRegularExpression envPattern("%([^%]+)%");
    QRegularExpressionMatchIterator it = envPattern.globalMatch(directory);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QByteArray key = match.captured(1).toLocal8Bit();
        const QByteArray value = qgetenv(key.constData());
        if (!value.isEmpty())
            directory.replace(match.captured(0), QString::fromLocal8Bit(value));
    }
    if (mode == 2 && !(useCustomDirectory && !customDirectory.isEmpty()))
        directory = QDir(directory).filePath(QStringLiteral("nppBackup"));
    if (!QDir().mkpath(directory)) {
        if (errorMessage)
            *errorMessage = QObject::tr("Cannot create backup directory: %1")
                .arg(QDir::toNativeSeparators(directory));
        return false;
    }

    const QString name = QFileInfo(sourcePath).fileName();
    const QString target = QDir(directory).filePath(mode == 1
        ? name + QStringLiteral(".bak")
        : name + QStringLiteral(".") + timestamp + QStringLiteral(".bak"));
    if (backupPath)
        *backupPath = target;
    if (QFile::exists(target) && !QFile::remove(target)) {
        if (errorMessage)
            *errorMessage = QObject::tr("Cannot replace backup file: %1")
                .arg(QDir::toNativeSeparators(target));
        return false;
    }
    if (!QFile::copy(sourcePath, target)) {
        if (errorMessage)
            *errorMessage = QObject::tr("Cannot copy the previous file to: %1")
                .arg(QDir::toNativeSeparators(target));
        return false;
    }
    return true;
}

Buffer* FileManager::newBuffer()
{
    ++_untitledCounter;
    Buffer* buf = new Buffer(_untitledCounter);
    _buffers.append(buf);
    return buf;
}

Buffer* FileManager::loadBuffer(const QString& filePath)
{
    // 检查文件是否已经打开
    Buffer* existing = findBufferByPath(filePath);
    if (existing)
        return existing;

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly))
        return nullptr;
    file.close();

    ++_untitledCounter;
    Buffer* buf = new Buffer(_untitledCounter);
    buf->setFilePath(filePath);
    const qint64 size = QFileInfo(filePath).size();
    buf->setSourceFileSize(size);
    buf->setLargeFile(Buffer::isLargeFileSize(size));
    buf->setDirty(false);
    buf->setLastKnownModificationTime(QFileInfo(filePath).lastModified());

    _buffers.append(buf);
    return buf;
}

bool FileManager::loadBufferContent(
    Buffer* buf, ScintillaEditView* view,
    const TextDecodingOptions& options, const QString& forcedEncoding,
    QString* errorMessage, const QString& sourcePath)
{
    if (!buf || !view) {
        if (errorMessage)
            *errorMessage = "The document has no editor view.";
        return false;
    }

    QFile file(sourcePath.isEmpty() ? buf->getFullPath() : sourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }

    const QByteArray sample = file.read(IoBlockSize);
    if (file.error() != QFile::NoError) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }

    const bool hasUnicodeBom =
        sample.startsWith("\xEF\xBB\xBF") ||
        sample.startsWith("\xFF\xFE") ||
        sample.startsWith("\xFE\xFF");
    DecodedTextFile format = forcedEncoding.isEmpty() || hasUnicodeBom
        ? TextFileCodec::decode(sample, options)
        : TextFileCodec::decodeAs(sample, forcedEncoding);
    if (!format.isValid) {
        if (errorMessage)
            *errorMessage = "The file encoding could not be determined.";
        return false;
    }

    QTextCodec* codec =
        QTextCodec::codecForName(format.encoding.toLatin1());
    if (!codec) {
        if (errorMessage)
            *errorMessage =
                QString("Unknown encoding: %1").arg(format.encoding);
        return false;
    }

    if (buf->isLargeFile() && !view->createLargeDocument()) {
        if (errorMessage)
            *errorMessage =
                "Scintilla could not create a large-text document.";
        return false;
    }
    view->setLargeFileMode(buf->isLargeFile());
    view->beginBulkLoad(buf->sourceFileSize());

    const int bomSize = bomSizeFor(format.encoding, sample);
    if (!file.seek(bomSize)) {
        if (errorMessage)
            *errorMessage = file.errorString();
        view->endBulkLoad();
        return false;
    }

    QScopedPointer<QTextDecoder> decoder(
        codec->makeDecoder(QTextCodec::IgnoreHeader));
    QByteArray pendingInput;
    const bool isUtf16Le =
        format.encoding == QStringLiteral("UTF-16LE");
    const bool isUtf16Be =
        format.encoding == QStringLiteral("UTF-16BE");
    while (!file.atEnd()) {
        const QByteArray block = file.read(IoBlockSize);
        if (block.isEmpty() && file.error() != QFile::NoError) {
            if (errorMessage)
                *errorMessage = file.errorString();
            view->endBulkLoad();
            return false;
        }
        QByteArray input = pendingInput;
        input += block;
        pendingInput.clear();
        if (isUtf16Le || isUtf16Be) {
            pendingInput = takeIncompleteUtf16Tail(
                &input, isUtf16Le);
        }
        const QString text = decoder->toUnicode(
            input.constData(), input.size());
        if (decoder->hasFailure() ||
            !view->appendUtf8Chunk(text.toUtf8())) {
            if (errorMessage)
                *errorMessage =
                    "The file could not be decoded or allocated.";
            view->endBulkLoad();
            return false;
        }
    }
    const QString finalText = decoder->toUnicode(
        pendingInput.constData(), pendingInput.size());
    if (!finalText.isEmpty() &&
        !view->appendUtf8Chunk(finalText.toUtf8())) {
        if (errorMessage)
            *errorMessage = "Scintilla could not allocate the final text block.";
        view->endBulkLoad();
        return false;
    }
    decoder->toUnicode("", 0);
    const bool scintillaOk = view->endBulkLoad();
    if (decoder->hasFailure() || decoder->needsMoreData() ||
        !scintillaOk) {
        if (errorMessage)
            *errorMessage =
                "The file ended with an incomplete character or Scintilla error.";
        return false;
    }

    buf->setEncoding(format.encoding);
    buf->setHasBom(format.hasBom);
    buf->setUsesEncodingCookie(format.usesEncodingCookie);
    buf->setBinary(format.isBinary);
    buf->setEolMode(format.eolMode);
    return true;
}

bool FileManager::saveBuffer(Buffer* buf, const QString& filePath,
                             QString* errorMessage)
{
    if (!saveBufferCopy(buf, filePath, errorMessage))
        return false;

    buf->setFilePath(filePath);
    buf->setSourceFileSize(QFileInfo(filePath).size());
    buf->setDirty(false);
    buf->setLastKnownModificationTime(QFileInfo(filePath).lastModified());
    return true;
}

bool FileManager::saveBufferCopy(const Buffer* buf, const QString& filePath,
                                 QString* errorMessage) const
{
    ScintillaEditView* view = nullptr;
    if (buf) {
        for (ScintillaEditView* candidate : buf->views()) {
            if (candidate && buf->document() ==
                    static_cast<qintptr>(candidate->document())) {
                view = candidate;
                break;
            }
        }
        if (!view)
            view = buf->getView();
    }
    if (!view) {
        if (errorMessage)
            *errorMessage = "The document has no active editor view.";
        return false;
    }

    const QString normalized =
        TextFileCodec::normalizedEncoding(buf->getEncoding());
    QTextCodec* targetCodec =
        QTextCodec::codecForName(normalized.toLatin1());
    QTextCodec* utf8Codec = QTextCodec::codecForName("UTF-8");
    if (!targetCodec || !utf8Codec) {
        if (errorMessage)
            *errorMessage = QString("Unknown encoding: %1")
                .arg(buf->getEncoding());
        return false;
    }

    QSaveFile file(filePath);
    if (!file.open(QFile::WriteOnly)) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }
    QByteArray bom;
    if (buf->hasBom()) {
        if (normalized == QStringLiteral("UTF-8"))
            bom = QByteArray("\xEF\xBB\xBF", 3);
        else if (normalized == QStringLiteral("UTF-16LE"))
            bom = QByteArray("\xFF\xFE", 2);
        else if (normalized == QStringLiteral("UTF-16BE"))
            bom = QByteArray("\xFE\xFF", 2);
    }
    if (!writeAll(file, bom)) {
        if (errorMessage)
            *errorMessage = file.errorString();
        file.cancelWriting();
        return false;
    }

    QScopedPointer<QTextDecoder> decoder(
        utf8Codec->makeDecoder(QTextCodec::IgnoreHeader));
    QScopedPointer<QTextEncoder> encoder(
        targetCodec->makeEncoder(QTextCodec::IgnoreHeader));
    const qintptr length = view->documentLengthNpp();
    const qintptr gap = view->gapPositionNpp();
    qintptr position = 0;
    while (position < length) {
        qintptr amount = qMin<qintptr>(IoBlockSize, length - position);
        if (position < gap && position + amount > gap)
            amount = gap - position;
        if (amount <= 0)
            amount = qMin<qintptr>(IoBlockSize, length - position);

        const char* source = view->utf8RangePointer(position, amount);
        if (!source) {
            if (errorMessage)
                *errorMessage =
                    "Scintilla could not expose a contiguous text range.";
            file.cancelWriting();
            return false;
        }
        const QString text = decoder->toUnicode(
            source, static_cast<int>(amount));
        const QByteArray output =
            encoder->fromUnicode(text.constData(), text.size());
        if (decoder->hasFailure() || encoder->hasFailure() ||
            !writeAll(file, output)) {
            if (errorMessage) {
                *errorMessage = decoder->hasFailure() ||
                                encoder->hasFailure()
                    ? QString("%1 character(s) cannot be represented in %2.")
                          .arg(1).arg(normalized)
                    : file.errorString();
            }
            file.cancelWriting();
            return false;
        }
        position += amount;
    }
    decoder->toUnicode("", 0);
    if (decoder->hasFailure() || decoder->needsMoreData() ||
        encoder->hasFailure()) {
        if (errorMessage)
            *errorMessage =
                "The document contains an incomplete or unrepresentable character.";
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }
    return true;
}

void FileManager::closeBuffer(Buffer* buf)
{
    _buffers.removeOne(buf);
    delete buf;
}

Buffer* FileManager::findBufferByPath(const QString& filePath) const
{
    QFileInfo requestedInfo(filePath);
    QString canonical = requestedInfo.canonicalFilePath();
    if (canonical.isEmpty())
        canonical = QDir::cleanPath(requestedInfo.absoluteFilePath());
#ifdef Q_OS_WIN
    const Qt::CaseSensitivity sensitivity = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity sensitivity = Qt::CaseSensitive;
#endif
    for (Buffer* buf : _buffers) {
        if (buf->isUntitled())
            continue;
        QFileInfo openInfo(buf->getFullPath());
        QString openPath = openInfo.canonicalFilePath();
        if (openPath.isEmpty())
            openPath = QDir::cleanPath(openInfo.absoluteFilePath());
        if (openPath.compare(canonical, sensitivity) == 0)
            return buf;
    }
    return nullptr;
}

Buffer* FileManager::findBufferByView(const ScintillaEditView* view) const
{
    if (!view)
        return nullptr;
    for (Buffer* buffer : _buffers) {
        if (buffer && buffer->containsView(
                const_cast<ScintillaEditView*>(view)) &&
            buffer->document() == static_cast<qintptr>(view->document()))
            return buffer;
    }
    return nullptr;
}
