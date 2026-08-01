#include "PluginUpdateExecutor.h"

#include "PluginArchiveExtractor.h"
#include "PluginArtifactResolver.h"

#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>

namespace {

struct PreparedOperation
{
    PluginOperation operation;
    QString stagedDirectory;
};

struct CommittedOperation
{
    QString target;
    QString backup;
    bool installedReplacement = false;
};

bool copyFileChecked(const QString& source, const QString& destination,
                     QString* error)
{
    const QFileInfo destinationInfo(destination);
    if (!QDir().mkpath(destinationInfo.absolutePath())) {
        if (error)
            *error = QStringLiteral("Could not create the package directory.");
        return false;
    }
    QFile::remove(destination);
    if (!QFile::copy(source, destination)) {
        if (error)
            *error = QStringLiteral("Could not copy the plugin package.");
        return false;
    }
    return true;
}

bool downloadPackage(QNetworkAccessManager* manager, const QUrl& url,
                     const QString& destination, QString* error)
{
    if (url.isLocalFile())
        return copyFileChecked(url.toLocalFile(), destination, error);

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
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

bool rollback(const QVector<CommittedOperation>& committed,
              QString* rollbackError)
{
    bool restored = true;
    for (int index = committed.size() - 1; index >= 0; --index) {
        const CommittedOperation& item = committed.at(index);
        if (item.installedReplacement && QFileInfo::exists(item.target) &&
            !QDir(item.target).removeRecursively()) {
            restored = false;
        }
        if (!item.backup.isEmpty() && QFileInfo::exists(item.backup) &&
            !QDir().rename(item.backup, item.target)) {
            restored = false;
        }
    }
    if (!restored && rollbackError) {
        *rollbackError = QStringLiteral(
            " Some plugin files could not be restored automatically.");
    }
    return restored;
}

} // namespace

bool PluginUpdateExecutor::apply(const PluginUpdatePlan& plan, QString* error)
{
    if (error)
        error->clear();
    if (!plan.isValid(error))
        return false;

    QDir pluginRoot(plan.pluginRoot);
    if ((!pluginRoot.exists() && !QDir().mkpath(pluginRoot.absolutePath())) ||
        QFileInfo(pluginRoot.absolutePath()).isSymLink()) {
        if (error)
            *error = QStringLiteral("Plugin directory is unavailable.");
        return false;
    }

    QTemporaryDir transaction(pluginRoot.filePath(
        QStringLiteral(".npp-plugin-update-XXXXXX")));
    if (!transaction.isValid()) {
        if (error) {
            *error = QStringLiteral(
                "Could not create a transaction in the plugin directory.");
        }
        return false;
    }
    const QString packages =
        QDir(transaction.path()).filePath(QStringLiteral("packages"));
    const QString staged =
        QDir(transaction.path()).filePath(QStringLiteral("staged"));
    const QString backups =
        QDir(transaction.path()).filePath(QStringLiteral("backups"));
    if (!QDir().mkpath(packages) || !QDir().mkpath(staged) ||
        !QDir().mkpath(backups)) {
        if (error)
            *error = QStringLiteral("Could not prepare the plugin transaction.");
        return false;
    }

    QVector<PreparedOperation> prepared;
    QNetworkAccessManager manager;
    for (const PluginOperation& operation : plan.operations) {
        const QString target = pluginRoot.filePath(operation.folderName);
        const QFileInfo targetInfo(target);
        if (targetInfo.exists() &&
            (targetInfo.isSymLink() || !targetInfo.isDir())) {
            if (error)
                *error = QStringLiteral("Invalid plugin target: %1")
                             .arg(operation.folderName);
            return false;
        }
        if (operation.type == PluginOperationType::Install &&
            targetInfo.exists()) {
            if (error)
                *error = QStringLiteral("Plugin is already installed: %1")
                             .arg(operation.folderName);
            return false;
        }

        PreparedOperation item;
        item.operation = operation;
        if (operation.type != PluginOperationType::Remove) {
            const QString packagePath = QDir(packages).filePath(
                operation.folderName + QStringLiteral(".zip"));
            item.stagedDirectory =
                QDir(staged).filePath(operation.folderName);
            if (!downloadPackage(&manager, QUrl(operation.repository),
                                 packagePath, error) ||
                !verifySha256(packagePath, operation.packageSha256, error) ||
                !PluginArchiveExtractor::extractZip(
                    packagePath, item.stagedDirectory, error)) {
                return false;
            }
            const QString expectedBinary =
                PluginArtifactResolver::binaryPath(
                    staged, operation.folderName);
            const QFileInfo binaryInfo(expectedBinary);
            if (!binaryInfo.isFile() || binaryInfo.isSymLink()) {
                if (error) {
                    *error = QStringLiteral(
                        "Package does not contain the expected plugin binary: %1")
                                 .arg(QDir::toNativeSeparators(
                                     expectedBinary));
                }
                return false;
            }
            if (!writeReceipt(item.stagedDirectory, operation, error))
                return false;
        }
        prepared.append(item);
    }

    QVector<CommittedOperation> committed;
    for (const PreparedOperation& item : prepared) {
        const QString target = pluginRoot.filePath(item.operation.folderName);
        CommittedOperation commit;
        commit.target = target;
        if (QFileInfo::exists(target)) {
            commit.backup =
                QDir(backups).filePath(item.operation.folderName);
            if (!QDir().rename(target, commit.backup)) {
                QString rollbackError;
                rollback(committed, &rollbackError);
                if (error) {
                    *error = QStringLiteral("Could not back up plugin: %1.%2")
                                 .arg(item.operation.folderName,
                                      rollbackError);
                }
                return false;
            }
        }

        if (item.operation.type != PluginOperationType::Remove) {
            if (!QDir().rename(item.stagedDirectory, target)) {
                if (!commit.backup.isEmpty())
                    QDir().rename(commit.backup, target);
                QString rollbackError;
                rollback(committed, &rollbackError);
                if (error) {
                    *error = QStringLiteral("Could not install plugin: %1.%2")
                                 .arg(item.operation.folderName,
                                      rollbackError);
                }
                return false;
            }
            commit.installedReplacement = true;
        }
        committed.append(commit);
    }
    return true;
}
