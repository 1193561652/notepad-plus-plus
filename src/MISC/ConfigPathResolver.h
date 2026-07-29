#pragma once

#include <QString>

enum class ConfigPathSource
{
    PlatformDefault,
    Portable,
    CommandLine
};

struct ConfigPathResolution
{
    QString directory;
    ConfigPathSource source = ConfigPathSource::PlatformDefault;
    QString error;

    bool isValid() const { return error.isEmpty() && !directory.isEmpty(); }
};

class ConfigPathResolver
{
public:
    static ConfigPathResolution resolve(const QString& applicationDirectory,
                                        const QString& commandLineDirectory = QString());
    static QString platformDefaultDirectory();
    static bool prepareDirectory(const QString& directory, bool allowCreate,
                                 QString* error = nullptr);

private:
    static QString normalizedAbsolutePath(const QString& path);
};
