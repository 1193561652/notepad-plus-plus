#ifndef NPPCOMMANDREGISTRY_H
#define NPPCOMMANDREGISTRY_H

#include <QVector>

struct NppCommandMapping
{
    const char* objectName;
    int commandId;
};

const QVector<NppCommandMapping>& nppCommandMappings();
int nppCommandIdForObjectName(const char* objectName);

#endif // NPPCOMMANDREGISTRY_H
