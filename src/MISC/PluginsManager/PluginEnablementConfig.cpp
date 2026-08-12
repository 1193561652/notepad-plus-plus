#include "PluginEnablementConfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace {

bool enabledAttribute(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    return normalized == QStringLiteral("yes")
        || normalized == QStringLiteral("true")
        || normalized == QStringLiteral("1");
}

} // namespace

PluginEnablementConfig::PluginEnablementConfig(const QString& filePath)
    : _filePath(filePath.isEmpty() ? QString() : QDir::cleanPath(filePath))
{}

QString PluginEnablementConfig::filePathForConfigDirectory(
    const QString& directory)
{
    return QDir(directory).filePath(QStringLiteral("pluginsEnabled.xml"));
}

QString PluginEnablementConfig::keyForFolder(const QString& folderName)
{
    return folderName.trimmed().toCaseFolded();
}

bool PluginEnablementConfig::isValidFolderName(const QString& folderName)
{
    const QString trimmed = folderName.trimmed();
    return !trimmed.isEmpty() && trimmed != QStringLiteral(".")
        && trimmed != QStringLiteral("..")
        && !trimmed.contains(QLatin1Char('/'))
        && !trimmed.contains(QLatin1Char('\\'));
}

bool PluginEnablementConfig::load(QString* error)
{
    if (error)
        error->clear();
    _entries.clear();

    QFile file(_filePath);
    if (!file.exists())
        return true;
    if (!file.open(QFile::ReadOnly)) {
        if (error)
            *error = file.errorString();
        return false;
    }

    QXmlStreamReader xml(&file);
    if (!xml.readNextStartElement()
        || xml.name() != QStringLiteral("NotepadPlus")) {
        if (error)
            *error = QStringLiteral("Invalid pluginsEnabled.xml root element");
        return false;
    }

    while (xml.readNextStartElement()) {
        if (xml.name() != QStringLiteral("Plugins")) {
            xml.skipCurrentElement();
            continue;
        }
        while (xml.readNextStartElement()) {
            if (xml.name() != QStringLiteral("Plugin")) {
                xml.skipCurrentElement();
                continue;
            }
            const QString folderName =
                xml.attributes().value(QStringLiteral("folderName"))
                    .toString().trimmed();
            const QString enabled =
                xml.attributes().value(QStringLiteral("enabled")).toString();
            if (isValidFolderName(folderName)) {
                _entries.insert(
                    keyForFolder(folderName),
                    Entry{folderName, enabledAttribute(enabled)});
            }
            xml.skipCurrentElement();
        }
    }

    if (xml.hasError()) {
        _entries.clear();
        if (error)
            *error = xml.errorString();
        return false;
    }
    return true;
}

bool PluginEnablementConfig::save(QString* error) const
{
    if (error)
        error->clear();
    if (_filePath.isEmpty()) {
        if (error)
            *error = QStringLiteral("Plugin enablement path is empty");
        return false;
    }
    if (!QDir().mkpath(QFileInfo(_filePath).absolutePath())) {
        if (error)
            *error = QStringLiteral("Could not create plugin config directory");
        return false;
    }

    QSaveFile file(_filePath);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        if (error)
            *error = file.errorString();
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument(QStringLiteral("1.0"));
    xml.writeStartElement(QStringLiteral("NotepadPlus"));
    xml.writeStartElement(QStringLiteral("Plugins"));
    for (const Entry& entry : _entries) {
        xml.writeEmptyElement(QStringLiteral("Plugin"));
        xml.writeAttribute(QStringLiteral("folderName"), entry.folderName);
        xml.writeAttribute(QStringLiteral("enabled"),
                           entry.enabled ? QStringLiteral("yes")
                                         : QStringLiteral("no"));
    }
    xml.writeEndElement();
    xml.writeEndElement();
    xml.writeEndDocument();

    if (file.error() != QFileDevice::NoError || !file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}

bool PluginEnablementConfig::isEnabled(const QString& folderName) const
{
    const auto found = _entries.constFind(keyForFolder(folderName));
    return found != _entries.constEnd() && found->enabled;
}

QStringList PluginEnablementConfig::enabledPlugins() const
{
    QStringList enabled;
    for (const Entry& entry : _entries) {
        if (entry.enabled)
            enabled.append(entry.folderName);
    }
    return enabled;
}

bool PluginEnablementConfig::setEnabled(
    const QString& folderName, bool enabled)
{
    if (!isValidFolderName(folderName))
        return false;
    const QString trimmed = folderName.trimmed();
    _entries.insert(keyForFolder(trimmed), Entry{trimmed, enabled});
    return true;
}
