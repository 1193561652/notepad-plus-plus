#ifndef PLATFORMSERVICES_H
#define PLATFORMSERVICES_H

#include <QString>
#include <QStringList>

namespace PlatformServices
{
bool openInFileManager(const QString& path, bool selectFile = true);
bool openTerminal(const QString& directory);
bool openDefaultApplication(const QString& path);
bool moveToTrash(const QString& path, QString* errorMessage = nullptr);
bool canManageFileAssociations();
QStringList registeredFileAssociations();
bool registerFileAssociation(const QString& extension, const QString& applicationPath,
                             QString* errorMessage = nullptr);
bool unregisterFileAssociation(const QString& extension,
                               QString* errorMessage = nullptr);
bool openDefaultApplicationsSettings();
}

#endif // PLATFORMSERVICES_H
