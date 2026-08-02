#include "PluginUpdatePlan.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QUrl>
#include <QRegularExpression>
#include <QSaveFile>

namespace {

bool validFolderName(const QString& folderName)
{
    if (folderName.isEmpty() || folderName == QStringLiteral(".") ||
        folderName == QStringLiteral("..") ||
        folderName.compare(QStringLiteral("Config"),
                           Qt::CaseInsensitive) == 0) {
        return false;
    }
    if (folderName.contains(QLatin1Char('/')) ||
        folderName.contains(QLatin1Char('\\')) ||
        folderName.contains(QLatin1Char(':'))) {
        return false;
    }
    return QDir::cleanPath(folderName) == folderName;
}

bool validSha256(const QString& value)
{
    return QRegularExpression(QStringLiteral("^[0-9A-Fa-f]{64}$"))
        .match(value)
        .hasMatch();
}

} // namespace

QString PluginUpdatePlan::operationName(PluginOperationType type)
{
    switch (type) {
        case PluginOperationType::Install: return QStringLiteral("install");
        case PluginOperationType::Update: return QStringLiteral("update");
        case PluginOperationType::Remove: return QStringLiteral("remove");
    }
    return QString();
}

bool PluginUpdatePlan::parseOperationName(const QString& name,
                                          PluginOperationType* type)
{
    if (!type)
        return false;
    if (name == QStringLiteral("install")) {
        *type = PluginOperationType::Install;
        return true;
    }
    if (name == QStringLiteral("update")) {
        *type = PluginOperationType::Update;
        return true;
    }
    if (name == QStringLiteral("remove")) {
        *type = PluginOperationType::Remove;
        return true;
    }
    return false;
}

bool PluginUpdatePlan::isValid(QString* error) const
{
    if (error)
        error->clear();
    if (applicationPath.isEmpty() || pluginRoot.isEmpty() ||
        operations.isEmpty()) {
        if (error)
            *error = QStringLiteral("Plugin update plan is incomplete.");
        return false;
    }

    QSet<QString> folders;
    for (const PluginOperation& operation : operations) {
        const QString key = operation.folderName.toCaseFolded();
        if (!validFolderName(operation.folderName) || folders.contains(key)) {
            if (error) {
                *error = QStringLiteral("Invalid or duplicate plugin folder: %1")
                             .arg(operation.folderName);
            }
            return false;
        }
        folders.insert(key);

        if (operation.type != PluginOperationType::Remove) {
            const QUrl repository(operation.repository);
            if (!repository.isValid() ||
                (repository.scheme() != QStringLiteral("https") &&
                 repository.scheme() != QStringLiteral("http") &&
                 repository.scheme() != QStringLiteral("file")) ||
                !validSha256(operation.packageSha256)) {
                if (error) {
                    *error = QStringLiteral(
                        "Invalid repository or SHA-256 for plugin: %1")
                                 .arg(operation.folderName);
                }
                return false;
            }
        }
    }
    return true;
}

bool PluginUpdatePlan::write(const QString& filePath, QString* error) const
{
    if (error)
        error->clear();
    if (!isValid(error))
        return false;

    QJsonObject root;
    root.insert(QStringLiteral("format"), 1);
    root.insert(QStringLiteral("application"), applicationPath);
    root.insert(QStringLiteral("plugin-root"), pluginRoot);
    QJsonArray operationsArray;
    for (const PluginOperation& operation : operations) {
        QJsonObject object;
        object.insert(QStringLiteral("operation"), operationName(operation.type));
        object.insert(QStringLiteral("folder-name"), operation.folderName);
        if (operation.type != PluginOperationType::Remove) {
            object.insert(QStringLiteral("version"), operation.version);
            object.insert(QStringLiteral("repository"), operation.repository);
            object.insert(QStringLiteral("sha256"),
                          operation.packageSha256.toLower());
        }
        operationsArray.append(object);
    }
    root.insert(QStringLiteral("operations"), operationsArray);

    QDir parent = QFileInfo(filePath).absoluteDir();
    if (!parent.exists() && !parent.mkpath(QStringLiteral("."))) {
        if (error)
            *error = QStringLiteral("Could not create update plan directory.");
        return false;
    }

    QSaveFile file(filePath);
    if (!file.open(QFile::WriteOnly)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    const QByteArray bytes =
        QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        if (error)
            *error = file.errorString();
        file.cancelWriting();
        return false;
    }
    return true;
}

PluginUpdatePlan PluginUpdatePlan::read(const QString& filePath,
                                        QString* error)
{
    if (error)
        error->clear();
    PluginUpdatePlan plan;
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly)) {
        if (error)
            *error = file.errorString();
        return plan;
    }

    return fromJson(file.readAll(), error);
}

PluginUpdatePlan PluginUpdatePlan::fromJson(const QByteArray& bytes,
                                            QString* error)
{
    if (error)
        error->clear();
    PluginUpdatePlan plan;

    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError ||
        !document.isObject()) {
        if (error)
            *error = parseError.errorString();
        return PluginUpdatePlan();
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toInt() != 1) {
        if (error)
            *error = QStringLiteral("Unsupported plugin update plan format.");
        return PluginUpdatePlan();
    }

    plan.applicationPath =
        root.value(QStringLiteral("application")).toString();
    plan.pluginRoot =
        root.value(QStringLiteral("plugin-root")).toString();
    const QJsonValue operationsValue =
        root.value(QStringLiteral("operations"));
    if (!operationsValue.isArray()) {
        if (error)
            *error = QStringLiteral("Plugin update operations are missing.");
        return PluginUpdatePlan();
    }

    for (const QJsonValue& value : operationsValue.toArray()) {
        if (!value.isObject()) {
            if (error)
                *error = QStringLiteral("Invalid plugin update operation.");
            return PluginUpdatePlan();
        }
        const QJsonObject object = value.toObject();
        PluginOperation operation;
        if (!parseOperationName(
                object.value(QStringLiteral("operation")).toString(),
                &operation.type)) {
            if (error)
                *error = QStringLiteral("Unknown plugin update operation.");
            return PluginUpdatePlan();
        }
        operation.folderName =
            object.value(QStringLiteral("folder-name")).toString();
        operation.version =
            object.value(QStringLiteral("version")).toString();
        operation.repository =
            object.value(QStringLiteral("repository")).toString();
        operation.packageSha256 =
            object.value(QStringLiteral("sha256")).toString();
        plan.operations.append(operation);
    }
    if (!plan.isValid(error))
        return PluginUpdatePlan();
    return plan;
}
