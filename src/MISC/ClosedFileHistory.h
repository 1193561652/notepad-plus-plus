#pragma once

#include <QString>
#include <QStringList>

class ClosedFileHistory
{
public:
    explicit ClosedFileHistory(int capacity = 20);

    void remember(const QString& filePath);
    QString takeNextExisting();
    bool isEmpty() const { return _paths.isEmpty(); }
    int size() const { return _paths.size(); }

private:
    int _capacity;
    QStringList _paths;
};
