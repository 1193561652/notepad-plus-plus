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
    QIcon icon(const QString& iconId, const QIcon& fallback = QIcon()) const;

private:
    QString _iconDirectory;
};

#endif // TOOLBARICONTHEME_H
