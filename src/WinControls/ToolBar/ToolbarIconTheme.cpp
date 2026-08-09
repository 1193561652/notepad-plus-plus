#include "ToolbarIconTheme.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>

ToolbarIconTheme ToolbarIconTheme::fromUserDirectory(
    const QString& userDirectory)
{
    ToolbarIconTheme theme;
    QFile file(QDir(userDirectory).filePath(QStringLiteral("toolbarIcons.xml")));
    if (!file.open(QIODevice::ReadOnly))
        return theme;

    QXmlStreamReader xml(&file);
    QString folderName;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() &&
            xml.name() == QStringLiteral("ToolBarIcons")) {
            folderName = xml.attributes()
                .value(QStringLiteral("icoFolderName")).toString().trimmed();
            break;
        }
    }
    if (xml.hasError())
        return theme;
    if (folderName.isEmpty())
        folderName = QStringLiteral("default");
    if (folderName == QStringLiteral(".") ||
        folderName == QStringLiteral("..") ||
        folderName.contains('/') || folderName.contains('\\')) {
        return theme;
    }

    theme._iconDirectory = QDir(userDirectory).filePath(
        QStringLiteral("toolbarIcons/") + folderName);
    return theme;
}

QString ToolbarIconTheme::iconPath(const QString& iconId, bool disabled) const
{
    if (_iconDirectory.isEmpty() || iconId.isEmpty())
        return {};
    const QString suffix = disabled
        ? QStringLiteral("_disabled.ico") : QStringLiteral(".ico");
    const QString path = QDir(_iconDirectory).filePath(iconId + suffix);
    return QFileInfo(path).isFile() ? path : QString();
}

QIcon ToolbarIconTheme::icon(const QString& iconId, const QIcon& fallback) const
{
    const QString normalPath = iconPath(iconId);
    if (normalPath.isEmpty())
        return fallback;

    QIcon themed(normalPath);
    const QString disabledPath = iconPath(iconId, true);
    if (!disabledPath.isEmpty())
        themed.addFile(disabledPath, QSize(), QIcon::Disabled);
    return themed.isNull() ? fallback : themed;
}
