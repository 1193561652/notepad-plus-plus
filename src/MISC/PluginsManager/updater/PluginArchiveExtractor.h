#pragma once

#include <QString>

class PluginArchiveExtractor
{
public:
    static bool extractZip(const QString& archivePath,
                           const QString& destination,
                           QString* error = nullptr);

private:
    static bool validateEntry(const QString& entry, QString* error);
};
