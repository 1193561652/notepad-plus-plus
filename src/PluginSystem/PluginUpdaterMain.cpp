#include "PluginArchiveExtractor.h"
#include "PluginUpdatePlan.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>

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

bool copyFileChecked(const QString& source, const QString& destination,
                     QString* error)
{
    const QFileInfo destinationInfo(destination);
    if (!QDir().mkpath(destinationInfo.absolutePath())) {
        if (error)
            *error = QStringLiteral("Could not create plugin directory.");
        return false;
    }
    QFile::remove(destination);
    if (!QFile::copy(source, destination)) {
        if (error)
            *error = QStringLiteral("Could not copy plugin file: %1")
                         .arg(destination);
        return false;
    }
    return true;
}

bool copyTree(const QString& source, const QString& destination,
              QString* error)
{
    const QDir sourceDir(source);
    if (!sourceDir.exists() || !QDir().mkpath(destination)) {
        if (error)
            *error = QStringLiteral("Could not prepare plugin destination.");
        return false;
    }
    const QFileInfoList entries = sourceDir.entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot,
        QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo& entry : entries) {
        if (entry.isSymLink()) {
            if (error)
                *error = QStringLiteral("Symbolic links are not allowed.");
            return false;
        }
        const QString target =
            QDir(destination).filePath(entry.fileName());
        if (entry.isDir()) {
            if (!copyTree(entry.absoluteFilePath(), target, error))
                return false;
        } else if (!copyFileChecked(entry.absoluteFilePath(), target, error)) {
            return false;
        }
    }
    return true;
}

bool downloadPackage(QNetworkAccessManager* manager, const QUrl& url,
                     const QString& destination, QString* error)
{
    if (url.isLocalFile())
        return copyFileChecked(url.toLocalFile(), destination, error);

    QNetworkRequest request(url);
    request.setAttribute(
        QNetworkRequest::RedirectPolicyAttribute,
        QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply* reply = manager->get(request);
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, reply,
                     &QNetworkReply::abort);
    QObject::connect(reply, &QNetworkReply::finished, &loop,
                     &QEventLoop::quit);
    timeout.start(120000);
    loop.exec();

    const bool timedOut = !timeout.isActive();
    timeout.stop();
    if (reply->error() != QNetworkReply::NoError) {
        if (error) {
            *error = timedOut ? QStringLiteral("Plugin download timed out.")
                             : reply->errorString();
        }
        reply->deleteLater();
        return false;
    }
    const QByteArray content = reply->readAll();
    reply->deleteLater();

    QSaveFile file(destination);
    if (!file.open(QFile::WriteOnly) ||
        file.write(content) != content.size() || !file.commit()) {
        if (error)
            *error = file.errorString();
        file.cancelWriting();
        return false;
    }
    return true;
}

bool verifySha256(const QString& filePath, const QString& expected,
                  QString* error)
{
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        const QByteArray data = file.read(1024 * 1024);
        if (data.isEmpty() && file.error() != QFile::NoError) {
            if (error)
                *error = file.errorString();
            return false;
        }
        hash.addData(data);
    }
    if (QString::fromLatin1(hash.result().toHex())
            .compare(expected, Qt::CaseInsensitive) != 0) {
        if (error)
            *error = QStringLiteral("Plugin package SHA-256 does not match.");
        return false;
    }
    return true;
}

bool writeReceipt(const QString& target, const PluginOperation& operation,
                  QString* error)
{
    QJsonObject object;
    object.insert(QStringLiteral("folder-name"), operation.folderName);
    object.insert(QStringLiteral("version"), operation.version);
    object.insert(QStringLiteral("sha256"),
                  operation.packageSha256.toLower());
    QSaveFile file(QDir(target).filePath(
        QStringLiteral(".npp-package.json")));
    if (!file.open(QFile::WriteOnly)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    const QByteArray content =
        QJsonDocument(object).toJson(QJsonDocument::Indented);
    if (file.write(content) != content.size() || !file.commit()) {
        if (error)
            *error = file.errorString();
        file.cancelWriting();
        return false;
    }
    return true;
}

bool applyOperation(const PluginUpdatePlan& plan,
                    const PluginOperation& operation,
                    QNetworkAccessManager* manager, QString* error)
{
    const QString target =
        QDir(plan.pluginRoot).filePath(operation.folderName);
    if (operation.type == PluginOperationType::Remove) {
        QDir targetDir(target);
        if (!targetDir.exists() || targetDir.removeRecursively())
            return true;
        if (error)
            *error = QStringLiteral("Could not remove the plugin directory.");
        return false;
    }

    QTemporaryDir temporary;
    if (!temporary.isValid()) {
        if (error)
            *error = QStringLiteral("Could not create plugin staging directory.");
        return false;
    }
    const QString packagePath =
        QDir(temporary.path()).filePath(QStringLiteral("plugin.zip"));
    if (!downloadPackage(manager, QUrl(operation.repository),
                         packagePath, error) ||
        !verifySha256(packagePath, operation.packageSha256, error)) {
        return false;
    }

    const QString extracted =
        QDir(temporary.path()).filePath(QStringLiteral("extracted"));
    if (!PluginArchiveExtractor::extractZip(packagePath, extracted, error))
        return false;

    if (operation.type == PluginOperationType::Update) {
        QDir targetDir(target);
        if (targetDir.exists() && !targetDir.removeRecursively()) {
            if (error)
                *error = QStringLiteral("Could not clean old plugin files.");
            return false;
        }
    } else if (QFileInfo::exists(target)) {
        if (error)
            *error = QStringLiteral("Plugin is already installed.");
        return false;
    }

    if (!copyTree(extracted, target, error))
        return false;
    return writeReceipt(target, operation, error);
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    const QStringList arguments = application.arguments();
    const int planIndex = arguments.indexOf(QStringLiteral("--plan"));
    const int waitIndex = arguments.indexOf(QStringLiteral("--wait-pid"));
    if (planIndex < 0 || planIndex + 1 >= arguments.size()) {
        writeError(QStringLiteral("Usage: npp-plugin-updater --plan <file> "
                                  "[--wait-pid <pid>]"));
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
    const PluginUpdatePlan plan = PluginUpdatePlan::read(planPath, &error);
    if (!error.isEmpty()) {
        writeError(error);
        return 4;
    }

    QNetworkAccessManager manager;
    for (const PluginOperation& operation : plan.operations) {
        if (!applyOperation(plan, operation, &manager, &error)) {
            writeError(QStringLiteral("%1: %2")
                           .arg(operation.folderName, error));
            return 5;
        }
    }

    QFile::remove(planPath);
    if (!QProcess::startDetached(plan.applicationPath, QStringList())) {
        writeError(QStringLiteral("Plugin changes completed, but Notepad++ "
                                  "could not be restarted."));
        return 6;
    }
    return 0;
}
