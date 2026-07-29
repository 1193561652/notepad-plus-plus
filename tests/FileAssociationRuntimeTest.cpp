#include "MISC/PlatformServices.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTextStream>
#include <QUuid>
#include <QVector>

#include <iostream>

#ifdef Q_OS_WIN
#include <windows.h>

namespace
{
struct RegistryValueSnapshot
{
    QString path;
    bool keyExisted = false;
    bool existed = false;
    QString value;
};

RegistryValueSnapshot snapshotDefaultValue(const QString& path)
{
    RegistryValueSnapshot snapshot;
    snapshot.path = path;
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT,
                      reinterpret_cast<LPCWSTR>(path.utf16()), 0,
                      KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
        return snapshot;
    }
    snapshot.keyExisted = true;
    DWORD type = 0;
    DWORD bytes = 0;
    if (RegQueryValueExW(key, nullptr, nullptr, &type, nullptr, &bytes) ==
            ERROR_SUCCESS &&
        type == REG_SZ) {
        QVector<wchar_t> data(static_cast<int>(bytes / sizeof(wchar_t)) + 1, 0);
        if (RegQueryValueExW(key, nullptr, nullptr, &type,
                             reinterpret_cast<LPBYTE>(data.data()), &bytes) ==
            ERROR_SUCCESS) {
            snapshot.existed = true;
            snapshot.value = QString::fromWCharArray(data.constData());
        }
    }
    RegCloseKey(key);
    return snapshot;
}

bool setDefaultValue(const QString& path, const QString& value)
{
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT,
                        reinterpret_cast<LPCWSTR>(path.utf16()), 0, nullptr, 0,
                        KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    const DWORD bytes = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
    const LONG result = RegSetValueExW(
        key, nullptr, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(value.utf16()), bytes);
    RegCloseKey(key);
    return result == ERROR_SUCCESS;
}

QString defaultValue(const QString& path)
{
    return snapshotDefaultValue(path).value;
}

void restoreValue(const RegistryValueSnapshot& snapshot)
{
    if (!snapshot.keyExisted) {
        RegDeleteKeyW(HKEY_CLASSES_ROOT,
                      reinterpret_cast<LPCWSTR>(snapshot.path.utf16()));
        return;
    }
    if (snapshot.existed) {
        setDefaultValue(snapshot.path, snapshot.value);
        return;
    }
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT,
                      reinterpret_cast<LPCWSTR>(snapshot.path.utf16()), 0,
                      KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
        RegDeleteValueW(key, nullptr);
        RegCloseKey(key);
    }
}

struct Cleanup
{
    QString extension;
    bool progIdRootExisted = false;
    RegistryValueSnapshot progId;
    RegistryValueSnapshot icon;
    RegistryValueSnapshot command;

    ~Cleanup()
    {
        RegDeleteKeyW(HKEY_CLASSES_ROOT,
                      reinterpret_cast<LPCWSTR>(extension.utf16()));
        if (!progIdRootExisted) {
            RegDeleteKeyW(HKEY_CLASSES_ROOT,
                          L"Notepad++_file\\shell\\open\\command");
            RegDeleteKeyW(HKEY_CLASSES_ROOT, L"Notepad++_file\\shell\\open");
            RegDeleteKeyW(HKEY_CLASSES_ROOT, L"Notepad++_file\\shell");
            RegDeleteKeyW(HKEY_CLASSES_ROOT, L"Notepad++_file\\DefaultIcon");
            RegDeleteKeyW(HKEY_CLASSES_ROOT, L"Notepad++_file");
            return;
        }
        restoreValue(progId);
        restoreValue(icon);
        restoreValue(command);
    }
};
}
#endif

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
#ifndef Q_OS_WIN
    Q_UNUSED(argv)
    return 77;
#else
    if ((argc != 2 && argc != 3) ||
        !PlatformServices::canManageFileAssociations())
        return 2;

    const QString applicationPath = QFileInfo(
        QString::fromLocal8Bit(argv[1])).absoluteFilePath();
    if (!QFileInfo::exists(applicationPath))
        return 3;

    const QString extension = QStringLiteral(".nppq%1").arg(
        QUuid::createUuid().toString(QUuid::Id128).left(8));
    Cleanup cleanup;
    cleanup.extension = extension;
    HKEY existingProgId = nullptr;
    cleanup.progIdRootExisted =
        RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Notepad++_file", 0, KEY_READ,
                      &existingProgId) == ERROR_SUCCESS;
    if (existingProgId)
        RegCloseKey(existingProgId);
    cleanup.progId = snapshotDefaultValue(QStringLiteral("Notepad++_file"));
    cleanup.icon =
        snapshotDefaultValue(QStringLiteral("Notepad++_file\\DefaultIcon"));
    cleanup.command = snapshotDefaultValue(
        QStringLiteral("Notepad++_file\\shell\\open\\command"));

    const QString originalProgId = QStringLiteral("NppQtValidationOriginal");
    if (!setDefaultValue(extension, originalProgId))
        return 4;

    QString error;
    if (!PlatformServices::registerFileAssociation(
            extension, applicationPath, &error)) {
        std::cerr << error.toStdString() << '\n';
        return 5;
    }
    if (defaultValue(extension) != QStringLiteral("Notepad++_file"))
        return 6;
    if (!PlatformServices::registeredFileAssociations().contains(
            extension, Qt::CaseInsensitive)) {
        return 7;
    }
    const QString command = defaultValue(
        QStringLiteral("Notepad++_file\\shell\\open\\command"));
    if (!command.contains(QDir::toNativeSeparators(applicationPath),
                          Qt::CaseInsensitive) ||
        !command.contains(QStringLiteral("\"%1\""))) {
        return 8;
    }

    if (!PlatformServices::unregisterFileAssociation(extension, &error)) {
        std::cerr << error.toStdString() << '\n';
        return 9;
    }
    if (defaultValue(extension) != originalProgId)
        return 10;

    const QString report =
        QStringLiteral("extension=%1\n"
                       "register=pass\n"
                       "enumerate=pass\n"
                       "open-command=pass\n"
                       "restore=pass\n"
                       "cleanup=scheduled\n").arg(extension);
    std::cout << report.toStdString();
    if (argc == 3) {
        QFile reportFile(QString::fromLocal8Bit(argv[2]));
        if (!reportFile.open(QIODevice::WriteOnly | QIODevice::Text))
            return 11;
        QTextStream(&reportFile) << report;
    }
    return 0;
#endif
}
