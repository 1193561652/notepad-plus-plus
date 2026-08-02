#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

enum class PluginOperationType
{
    Install,
    Update,
    Remove
};

struct PluginOperation
{
    PluginOperationType type = PluginOperationType::Install;
    QString folderName;
    QString version;
    QString repository;
    QString packageSha256;
};

struct PluginUpdatePlan
{
    QString applicationPath;
    QString pluginRoot;
    QVector<PluginOperation> operations;

    bool isValid(QString* error = nullptr) const;
    bool write(const QString& filePath, QString* error = nullptr) const;

    static PluginUpdatePlan read(const QString& filePath,
                                 QString* error = nullptr);
    static PluginUpdatePlan fromJson(const QByteArray& bytes,
                                     QString* error = nullptr);
    static QString operationName(PluginOperationType type);
    static bool parseOperationName(const QString& name,
                                   PluginOperationType* type);
};
