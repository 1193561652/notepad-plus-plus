#ifndef PROJECTPANEL_H
#define PROJECTPANEL_H

#include <QWidget>
#include "WorkspaceDocument.h"

class QTreeWidget;
class QTreeWidgetItem;

class ProjectPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectPanel(int panelId, QWidget* parent = nullptr);

    bool loadWorkspace(const QString& filePath, QString* errorMessage = nullptr);
    bool saveWorkspace(const QString& filePath = QString(),
                       QString* errorMessage = nullptr);
    QString workspaceFilePath() const { return _document.filePath(); }
    QStringList allFiles() const { return _document.allFiles(); }
    int panelId() const { return _panelId; }

signals:
    void fileActivated(const QString& filePath);
    void workspacePathChanged(int panelId, const QString& filePath);
    void findInProjectsRequested(int panelMask);

private:
    void rebuildTree();
    QTreeWidgetItem* addNodeItem(QTreeWidgetItem* parent,
                                 const WorkspaceNode& node);
    void rebuildDocument();
    WorkspaceNode nodeFromItem(QTreeWidgetItem* item) const;
    QTreeWidgetItem* selectedContainer() const;
    void markDirty();

    int _panelId = 0;
    bool _dirty = false;
    WorkspaceDocument _document;
    QTreeWidget* _tree = nullptr;
};

#endif
