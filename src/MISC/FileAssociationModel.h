#ifndef FILEASSOCIATIONMODEL_H
#define FILEASSOCIATIONMODEL_H

#include <QString>
#include <QStringList>
#include <QVector>

struct FileAssociationCategory
{
    QString name;
    QStringList extensions;
};

const QVector<FileAssociationCategory>& fileAssociationCategories();
QString normalizeFileAssociationExtension(const QString& extension);

#endif // FILEASSOCIATIONMODEL_H
