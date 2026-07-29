#include "ClosedFileHistory.h"

#include <QFileInfo>

ClosedFileHistory::ClosedFileHistory(int capacity)
    : _capacity(qMax(1, capacity))
{
}

void ClosedFileHistory::remember(const QString& filePath)
{
    const QString path = QFileInfo(filePath).absoluteFilePath();
    if (path.isEmpty())
        return;
    _paths.removeAll(path);
    _paths.append(path);
    while (_paths.size() > _capacity)
        _paths.removeFirst();
}

QString ClosedFileHistory::takeNextExisting()
{
    while (!_paths.isEmpty()) {
        const QString path = _paths.takeLast();
        if (QFileInfo(path).isFile())
            return path;
    }
    return QString();
}
