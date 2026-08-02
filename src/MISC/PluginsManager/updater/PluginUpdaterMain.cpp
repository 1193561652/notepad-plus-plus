#include "PluginUpdateExecutor.h"
#include "MISC/PluginsManager/PluginUpdatePlan.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

#include <cstdio>

#if defined(Q_OS_WIN)
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <unistd.h>
#endif

namespace {

void writeError(const QString& message)
{
    fprintf(stderr, "%s\n", message.toLocal8Bit().constData());
}

bool processExists(qint64 pid)
{
    if (pid <= 0)
        return false;
#if defined(Q_OS_WIN)
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(pid));
    if (!process)
        return false;
    const DWORD state = WaitForSingleObject(process, 0);
    CloseHandle(process);
    return state == WAIT_TIMEOUT;
#else
    if (::kill(static_cast<pid_t>(pid), 0) == 0)
        return true;
    return errno == EPERM;
#endif
}

bool waitForProcess(qint64 pid, QString* error)
{
    QElapsedTimer timer;
    timer.start();
    while (processExists(pid)) {
        if (timer.elapsed() > 60000) {
            if (error)
                *error = QStringLiteral("Timed out waiting for Notepad++ to exit.");
            return false;
        }
#if defined(Q_OS_WIN)
        Sleep(100);
#else
        usleep(100000);
#endif
    }
    return true;
}

#if defined(Q_OS_WIN)
bool restartUnelevated(const QString& applicationPath, QString* error)
{
    const HWND shellWindow = GetShellWindow();
    DWORD shellPid = 0;
    if (shellWindow)
        GetWindowThreadProcessId(shellWindow, &shellPid);
    HANDLE shellProcess = shellPid
        ? OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, shellPid)
        : nullptr;
    HANDLE shellToken = nullptr;
    HANDLE primaryToken = nullptr;
    bool started = false;
    if (shellProcess &&
        OpenProcessToken(shellProcess,
                         TOKEN_QUERY | TOKEN_DUPLICATE |
                             TOKEN_ASSIGN_PRIMARY,
                         &shellToken) &&
        DuplicateTokenEx(shellToken, MAXIMUM_ALLOWED, nullptr,
                         SecurityImpersonation, TokenPrimary,
                         &primaryToken)) {
        std::wstring commandLine =
            QStringLiteral("\"%1\"").arg(
                QDir::toNativeSeparators(applicationPath)).toStdWString();
        std::wstring workingDirectory =
            QDir::toNativeSeparators(
                QFileInfo(applicationPath).absolutePath()).toStdWString();
        STARTUPINFOW startup = {};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION process = {};
        started = CreateProcessWithTokenW(
            primaryToken, LOGON_WITH_PROFILE,
            reinterpret_cast<LPCWSTR>(applicationPath.utf16()),
            commandLine.data(), 0, nullptr, workingDirectory.c_str(),
            &startup, &process) != FALSE;
        if (started) {
            CloseHandle(process.hThread);
            CloseHandle(process.hProcess);
        }
    }
    if (primaryToken)
        CloseHandle(primaryToken);
    if (shellToken)
        CloseHandle(shellToken);
    if (shellProcess)
        CloseHandle(shellProcess);
    if (!started && error) {
        *error = QStringLiteral(
            "Could not restart Notepad++ with the desktop user token.");
    }
    return started;
}
#endif

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    const QStringList arguments = application.arguments();
    const int planIndex = arguments.indexOf(QStringLiteral("--plan"));
    const int waitIndex = arguments.indexOf(QStringLiteral("--wait-pid"));
    const int hashIndex = arguments.indexOf(QStringLiteral("--plan-sha256"));
    const bool restartWithoutElevation =
        arguments.contains(QStringLiteral("--restart-unelevated"));
    const bool noRestart = arguments.contains(QStringLiteral("--no-restart"));
    if (planIndex < 0 || planIndex + 1 >= arguments.size()) {
        writeError(QStringLiteral("Usage: npp-plugin-updater --plan <file> "
                                  "[--plan-sha256 <hash>] "
                                  "[--wait-pid <pid>] [--no-restart]"));
        return 2;
    }

    QString error;
    if (waitIndex >= 0 && waitIndex + 1 < arguments.size()) {
        bool ok = false;
        const qint64 pid = arguments.at(waitIndex + 1).toLongLong(&ok);
        if (!ok || !waitForProcess(pid, &error)) {
            writeError(error.isEmpty()
                           ? QStringLiteral("Invalid process ID.") : error);
            return 3;
        }
    }

    const QString planPath = arguments.at(planIndex + 1);
    QFile planFile(planPath);
    if (!planFile.open(QFile::ReadOnly)) {
        writeError(planFile.errorString());
        return 4;
    }
    const QByteArray planBytes = planFile.readAll();
    planFile.close();
    if (hashIndex >= 0) {
        if (hashIndex + 1 >= arguments.size()) {
            writeError(QStringLiteral("Plugin update plan hash is missing."));
            return 4;
        }
        const QByteArray actualHash =
            QCryptographicHash::hash(planBytes,
                                     QCryptographicHash::Sha256).toHex();
        if (actualHash.compare(arguments.at(hashIndex + 1).toLatin1(),
                               Qt::CaseInsensitive) != 0) {
            writeError(QStringLiteral(
                "Plugin update plan changed after it was scheduled."));
            return 4;
        }
    }
    const PluginUpdatePlan plan =
        PluginUpdatePlan::fromJson(planBytes, &error);
    if (!error.isEmpty()) {
        writeError(error);
        return 4;
    }

    if (!PluginUpdateExecutor::apply(plan, &error)) {
        writeError(error);
        return 5;
    }

    if (noRestart)
        return 0;

    QFile::remove(planPath);

    bool restarted = false;
#if defined(Q_OS_WIN)
    if (restartWithoutElevation)
        restarted = restartUnelevated(plan.applicationPath, &error);
    else
#else
    Q_UNUSED(restartWithoutElevation)
#endif
        restarted = QProcess::startDetached(
            plan.applicationPath, QStringList());
    if (!restarted) {
        writeError(QStringLiteral("Plugin changes completed, but Notepad++ "
                                  "could not be restarted. %1")
                       .arg(error));
        return 6;
    }
    return 0;
}
