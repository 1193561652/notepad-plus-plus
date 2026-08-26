#ifndef TOOLBARICONTHEME_H
#define TOOLBARICONTHEME_H

#include <QIcon>
#include <QString>

class ToolbarIconTheme
{
public:
    static ToolbarIconTheme fromUserDirectory(const QString& userDirectory);

    bool isConfigured() const { return !_iconDirectory.isEmpty(); }
    QString iconDirectory() const { return _iconDirectory; }
    QString iconPath(const QString& iconId, bool disabled = false) const;
    bool hasIcon(const QString& iconId) const { return !iconPath(iconId).isEmpty(); }
    QIcon icon(const QString& iconId, const QIcon& fallback = QIcon(),
               const QSize& iconSize = QSize(16, 16)) const;

private:
    QString _iconDirectory;
};

#endif // TOOLBARICONTHEME_H
