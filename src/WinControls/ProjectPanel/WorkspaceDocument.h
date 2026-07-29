#ifndef WORKSPACEDOCUMENT_H
#define WORKSPACEDOCUMENT_H

#include <QString>
#include <QVector>

struct WorkspaceNode
{
    enum Type { Project, Folder, File };

    Type type = Folder;
    QString name;
    QString filePath;
    QVector<WorkspaceNode> children;
};

class WorkspaceDocument
{
public:
    bool load(const QString& filePath, QString* errorMessage = nullptr);
    bool save(const QString& filePath, QString* errorMessage = nullptr) const;

    void clear();
    QString filePath() const { return _filePath; }
    void setFilePath(const QString& path) { _filePath = path; }
    QVector<WorkspaceNode>& projects() { return _projects; }
    const QVector<WorkspaceNode>& projects() const { return _projects; }
    QStringList allFiles() const;

private:
    QString _filePath;
    QVector<WorkspaceNode> _projects;
};

#endif
