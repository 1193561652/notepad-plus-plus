#pragma once

#include <QFont>
#include <QFontDatabase>

inline QFont notepadPlusPlusUiFont()
{
#ifdef Q_OS_WIN
    const QString windowsUiFamily = QStringLiteral("Segoe UI");
    if (QFontDatabase().families().contains(
            windowsUiFamily, Qt::CaseInsensitive)) {
        return QFont(windowsUiFamily, 9);
    }
#endif
    return QFontDatabase::systemFont(QFontDatabase::GeneralFont);
}
