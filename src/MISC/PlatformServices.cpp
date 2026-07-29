#include "PlatformServices.h"
#include "FileAssociationModel.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>

namespace
{
const wchar_t kNppProgId[] = L"Notepad++_file";
const wchar_t kNppBackup[] = L"Notepad++_backup";

QString windowsErrorMessage(LONG error)
{
    wchar_t* buffer = nullptr;
    const DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, static_cast<DWORD>(error), 0,
        reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);
    const QString message = length && buffer
        ? QString::fromWCharArray(buffer, static_cast<int>(length)).trimmed()
        : QStringLiteral("Windows error %1").arg(error);
    if (buffer)
        LocalFree(buffer);
    return message;
}

QString readRegistryString(HKEY root, const QString& subKey, const wchar_t* valueName)
{
    HKEY key = nullptr;
    if (RegOpenKeyExW(root, reinterpret_cast<LPCWSTR>(subKey.utf16()), 0,
                      KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
        return {};
    }
    DWORD type = 0;
    DWORD bytes = 0;
    LONG result = RegQueryValueExW(key, valueName, nullptr, &type, nullptr, &bytes);
    if (result != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) {
        RegCloseKey(key);
        return {};
    }
    QVector<wchar_t> value(static_cast<int>(bytes / sizeof(wchar_t)) + 1, L'\0');
    result = RegQueryValueExW(key, valueName, nullptr, &type,
                             reinterpret_cast<LPBYTE>(value.data()), &bytes);
    RegCloseKey(key);
    return result == ERROR_SUCCESS ? QString::fromWCharArray(value.constData())
                                   : QString();
}

bool writeRegistryString(HKEY root, const QString& subKey, const wchar_t* valueName,
                         const QString& value, QString* errorMessage)
{
    HKEY key = nullptr;
    LONG result = RegCreateKeyExW(root, reinterpret_cast<LPCWSTR>(subKey.utf16()), 0,
                                  nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr);
    if (result == ERROR_SUCCESS) {
        const DWORD bytes = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
        result = RegSetValueExW(key, valueName, 0, REG_SZ,
                               reinterpret_cast<const BYTE*>(value.utf16()), bytes);
        RegCloseKey(key);
    }
    if (result == ERROR_SUCCESS)
        return true;
    if (errorMessage)
        *errorMessage = windowsErrorMessage(result);
    return false;
}
}
#endif

namespace PlatformServices
{
bool openInFileManager(const QString& path, bool selectFile)
{
    const QFileInfo info(path);
#ifdef Q_OS_WIN
    QStringList args;
    if (selectFile && info.isFile())
        args << "/select," << QDir::toNativeSeparators(info.absoluteFilePath());
    else
        args << QDir::toNativeSeparators(info.isDir() ? info.absoluteFilePath()
                                                       : info.absolutePath());
    return QProcess::startDetached("explorer.exe", args);
#elif defined(Q_OS_MAC)
    if (selectFile && info.isFile())
        return QProcess::startDetached("open", {"-R", info.absoluteFilePath()});
    return QProcess::startDetached("open", {info.isDir() ? info.absoluteFilePath()
                                                          : info.absolutePath()});
#else
    Q_UNUSED(selectFile)
    const QString directory = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    return QDesktopServices::openUrl(QUrl::fromLocalFile(directory));
#endif
}

bool openTerminal(const QString& directory)
{
    const QString workingDirectory = QFileInfo(directory).isDir()
            ? QFileInfo(directory).absoluteFilePath()
            : QFileInfo(directory).absolutePath();
#ifdef Q_OS_WIN
    return QProcess::startDetached("cmd.exe", {"/K", "cd", "/d",
                                               QDir::toNativeSeparators(workingDirectory)},
                                   workingDirectory);
#elif defined(Q_OS_MAC)
    return QProcess::startDetached("open", {"-a", "Terminal", workingDirectory});
#else
    const QStringList terminals = {"x-terminal-emulator", "konsole", "gnome-terminal",
                                   "xfce4-terminal"};
    for (const QString& terminal : terminals) {
        const QString executable = QStandardPaths::findExecutable(terminal);
        if (!executable.isEmpty())
            return QProcess::startDetached(executable, {}, workingDirectory);
    }
    return false;
#endif
}

bool openDefaultApplication(const QString& path)
{
    return QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath()));
}

bool moveToTrash(const QString& path, QString* errorMessage)
{
    const QString absolutePath = QFileInfo(path).absoluteFilePath();
#ifdef Q_OS_WIN
    std::wstring source = QDir::toNativeSeparators(absolutePath).toStdWString();
    source.push_back(L'\0');
    SHFILEOPSTRUCTW op = {};
    op.wFunc = FO_DELETE;
    op.pFrom = source.c_str();
    op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI;
    const int result = SHFileOperationW(&op);
    if (result == 0 && !op.fAnyOperationsAborted)
        return true;
    if (errorMessage) *errorMessage = QString("SHFileOperation failed: %1").arg(result);
    return false;
#elif defined(Q_OS_MAC)
    QProcess process;
    process.start("osascript", {"-e",
        QString("tell application \"Finder\" to delete POSIX file \"%1\"")
            .arg(QString(absolutePath).replace("\"", "\\\""))});
    process.waitForFinished();
    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0)
        return true;
    if (errorMessage) *errorMessage = QString::fromUtf8(process.readAllStandardError());
    return false;
#else
    QProcess process;
    process.start("gio", {"trash", absolutePath});
    process.waitForFinished();
    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0)
        return true;
    if (errorMessage) *errorMessage = QString::fromUtf8(process.readAllStandardError());
    return false;
#endif
}

bool canManageFileAssociations()
{
#ifdef Q_OS_WIN
    SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
    PSID administrators = nullptr;
    BOOL isMember = FALSE;
    if (!AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                  &administrators)) {
        return false;
    }
    CheckTokenMembership(nullptr, administrators, &isMember);
    FreeSid(administrators);
    return isMember == TRUE;
#else
    return false;
#endif
}

QStringList registeredFileAssociations()
{
    QStringList extensions;
#ifdef Q_OS_WIN
    DWORD index = 0;
    wchar_t keyName[256] = {};
    DWORD keyNameLength = 256;
    while (RegEnumKeyExW(HKEY_CLASSES_ROOT, index++, keyName, &keyNameLength,
                         nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
        const QString extension = QString::fromWCharArray(keyName);
        if (extension.startsWith('.') &&
            readRegistryString(HKEY_CLASSES_ROOT, extension, nullptr) ==
                QString::fromWCharArray(kNppProgId)) {
            extensions.append(extension.toLower());
        }
        keyNameLength = 256;
    }
    extensions.sort(Qt::CaseInsensitive);
#endif
    return extensions;
}

bool registerFileAssociation(const QString& extension, const QString& applicationPath,
                             QString* errorMessage)
{
#ifdef Q_OS_WIN
    const QString normalized = normalizeFileAssociationExtension(extension);
    if (normalized.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Invalid file extension.");
        return false;
    }
    const QString executable = QDir::toNativeSeparators(
        QFileInfo(applicationPath).absoluteFilePath());
    const QString quotedExecutable = QStringLiteral("\"%1\"").arg(executable);
    if (!writeRegistryString(HKEY_CLASSES_ROOT, QString::fromWCharArray(kNppProgId),
                             nullptr, QStringLiteral("Notepad++ Document"),
                             errorMessage) ||
        !writeRegistryString(HKEY_CLASSES_ROOT,
                             QString::fromWCharArray(kNppProgId) +
                                 QStringLiteral("\\DefaultIcon"),
                             nullptr, quotedExecutable + QStringLiteral(",0"),
                             errorMessage) ||
        !writeRegistryString(HKEY_CLASSES_ROOT,
                             QString::fromWCharArray(kNppProgId) +
                                 QStringLiteral("\\shell\\open\\command"),
                             nullptr, quotedExecutable + QStringLiteral(" \"%1\""),
                             errorMessage)) {
        return false;
    }

    const QString previous =
        readRegistryString(HKEY_CLASSES_ROOT, normalized, nullptr);
    if (!previous.isEmpty() && previous != QString::fromWCharArray(kNppProgId) &&
        readRegistryString(HKEY_CLASSES_ROOT, normalized, kNppBackup).isEmpty() &&
        !writeRegistryString(HKEY_CLASSES_ROOT, normalized, kNppBackup, previous,
                             errorMessage)) {
        return false;
    }
    return writeRegistryString(HKEY_CLASSES_ROOT, normalized, nullptr,
                               QString::fromWCharArray(kNppProgId), errorMessage);
#else
    Q_UNUSED(extension)
    Q_UNUSED(applicationPath)
    if (errorMessage)
        *errorMessage = QStringLiteral("File association editing is only supported on Windows.");
    return false;
#endif
}

bool unregisterFileAssociation(const QString& extension, QString* errorMessage)
{
#ifdef Q_OS_WIN
    const QString normalized = normalizeFileAssociationExtension(extension);
    if (normalized.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Invalid file extension.");
        return false;
    }
    if (readRegistryString(HKEY_CLASSES_ROOT, normalized, nullptr) !=
        QString::fromWCharArray(kNppProgId)) {
        return true;
    }
    const QString backup =
        readRegistryString(HKEY_CLASSES_ROOT, normalized, kNppBackup);
    if (!backup.isEmpty()) {
        if (!writeRegistryString(HKEY_CLASSES_ROOT, normalized, nullptr, backup,
                                 errorMessage)) {
            return false;
        }
    } else {
        HKEY key = nullptr;
        LONG result = RegOpenKeyExW(HKEY_CLASSES_ROOT,
                                    reinterpret_cast<LPCWSTR>(normalized.utf16()), 0,
                                    KEY_SET_VALUE, &key);
        if (result == ERROR_SUCCESS) {
            result = RegDeleteValueW(key, nullptr);
            RegCloseKey(key);
        }
        if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
            if (errorMessage)
                *errorMessage = windowsErrorMessage(result);
            return false;
        }
    }
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT,
                      reinterpret_cast<LPCWSTR>(normalized.utf16()), 0,
                      KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
        RegDeleteValueW(key, kNppBackup);
        RegCloseKey(key);
    }
    return true;
#else
    Q_UNUSED(extension)
    if (errorMessage)
        *errorMessage = QStringLiteral("File association editing is only supported on Windows.");
    return false;
#endif
}

bool openDefaultApplicationsSettings()
{
#ifdef Q_OS_WIN
    return QProcess::startDetached(QStringLiteral("explorer.exe"),
                                   {QStringLiteral("ms-settings:defaultapps")});
#elif defined(Q_OS_MAC)
    return QProcess::startDetached(
        QStringLiteral("open"),
        {QStringLiteral("x-apple.systempreferences:com.apple.preference.general")});
#else
    return QDesktopServices::openUrl(
        QUrl(QStringLiteral("settings://default-applications")));
#endif
}
}
