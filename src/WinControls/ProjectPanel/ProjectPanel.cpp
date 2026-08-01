#include "ProjectPanel.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QStyle>
#include <QTabWidget>
#include <QTabBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <functional>

enum ItemRole {
    NodeTypeRole = Qt::UserRole,
    FilePathRole,
    SyntheticRootRole
};

ProjectPanel::ProjectPanel(int panelId, QWidget* parent)
    : QWidget(parent), _panelId(panelId)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    QTabWidget* pages = new QTabWidget(this);
    pages->setObjectName(QStringLiteral("projectPanelPages"));

    QWidget* workspacePage = new QWidget(pages);
    QVBoxLayout* workspaceLayout = new QVBoxLayout(workspacePage);
    workspaceLayout->setContentsMargins(1, 1, 1, 1);
    _tree = new QTreeWidget(workspacePage);
    _tree->setObjectName(QStringLiteral("projectWorkspaceTree"));
    _tree->setHeaderHidden(true);
    _tree->setContextMenuPolicy(Qt::CustomContextMenu);
    _tree->setEditTriggers(QAbstractItemView::EditKeyPressed |
                           QAbstractItemView::SelectedClicked);
    workspaceLayout->addWidget(_tree);
    pages->addTab(workspacePage, tr("Workspace"));

    QWidget* editPage = new QWidget(pages);
    QVBoxLayout* tools = new QVBoxLayout(editPage);
    tools->setContentsMargins(6, 6, 6, 6);
    auto addTool = [this, tools](QStyle::StandardPixmap icon,
                                const QString& tip,
                                const std::function<void()>& command) {
        QToolButton* button = new QToolButton(this);
        button->setIcon(style()->standardIcon(icon));
        button->setText(tip);
        button->setToolTip(tip);
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(button, &QToolButton::clicked, this, command);
        tools->addWidget(button);
    };
    addTool(QStyle::SP_DialogOpenButton, tr("Open Workspace"), [this]() {
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Open Workspace"), QString(), tr("Workspace Files (*)"));
        if (path.isEmpty())
            return;
        QString error;
        if (!loadWorkspace(path, &error))
            QMessageBox::warning(this, tr("Open Workspace"), error);
    });
    addTool(QStyle::SP_DialogSaveButton, tr("Save Workspace"), [this]() {
        QString path = _document.filePath();
        if (path.isEmpty())
            path = QFileDialog::getSaveFileName(
                this, tr("Save Workspace"), QString(),
                tr("Workspace Files (*)"));
        if (path.isEmpty())
            return;
        QString error;
        if (!saveWorkspace(path, &error))
            QMessageBox::warning(this, tr("Save Workspace"), error);
    });
    addTool(QStyle::SP_FileDialogNewFolder, tr("Add Project"), [this]() {
        bool ok = false;
        const QString name = QInputDialog::getText(
            this, tr("Add Project"), tr("Name:"), QLineEdit::Normal,
            tr("New Project"), &ok);
        if (!ok || name.isEmpty())
            return;
        QTreeWidgetItem* item = new QTreeWidgetItem(selectedContainer(), {name});
        item->setData(0, NodeTypeRole, WorkspaceNode::Project);
        markDirty();
    });
    addTool(QStyle::SP_DirIcon, tr("Add Folder"), [this]() {
        QTreeWidgetItem* parent = selectedContainer();
        if (!parent)
            return;
        bool ok = false;
        const QString name = QInputDialog::getText(
            this, tr("Add Folder"), tr("Name:"), QLineEdit::Normal,
            tr("New Folder"), &ok);
        if (!ok || name.isEmpty())
            return;
        QTreeWidgetItem* item = new QTreeWidgetItem(parent, {name});
        item->setData(0, NodeTypeRole, WorkspaceNode::Folder);
        parent->setExpanded(true);
        markDirty();
    });
    addTool(QStyle::SP_FileIcon, tr("Add Files"), [this]() {
        QTreeWidgetItem* parent = selectedContainer();
        if (!parent)
            return;
        const QStringList files = QFileDialog::getOpenFileNames(
            this, tr("Add Files"));
        for (const QString& path : files) {
            QTreeWidgetItem* item =
                new QTreeWidgetItem(parent, {QFileInfo(path).fileName()});
            item->setData(0, NodeTypeRole, WorkspaceNode::File);
            item->setData(0, FilePathRole, QFileInfo(path).absoluteFilePath());
        }
        if (!files.isEmpty()) {
            parent->setExpanded(true);
            markDirty();
        }
    });
    tools->addStretch();
    pages->addTab(editPage, tr("Edit"));
    pages->tabBar()->setObjectName(QStringLiteral("projectPanelTabs"));
    layout->addWidget(pages);
    rebuildTree();
    connect(_tree, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem* item) {
        if (item->data(0, NodeTypeRole).toInt() == WorkspaceNode::File)
            emit fileActivated(item->data(0, FilePathRole).toString());
    });
    connect(_tree, &QTreeWidget::itemChanged,
            this, [this]() { markDirty(); });
    connect(_tree, &QTreeWidget::customContextMenuRequested, this,
            [this](const QPoint& point) {
        QTreeWidgetItem* item = _tree->itemAt(point);
        QMenu menu(this);
        QAction* findAction = menu.addAction(tr("Find in Projects"));
        QAction* renameAction = item ? menu.addAction(tr("Rename")) : nullptr;
        QAction* removeAction = item ? menu.addAction(tr("Remove")) : nullptr;
        QAction* chosen = menu.exec(_tree->viewport()->mapToGlobal(point));
        if (chosen == findAction)
            emit findInProjectsRequested(1 << _panelId);
        else if (chosen == renameAction)
            _tree->editItem(item);
        else if (chosen == removeAction) {
            delete item;
            markDirty();
        }
    });
}

bool ProjectPanel::loadWorkspace(const QString& filePath, QString* errorMessage)
{
    if (!_document.load(filePath, errorMessage))
        return false;
    _dirty = false;
    rebuildTree();
    emit workspacePathChanged(_panelId, _document.filePath());
    return true;
}

bool ProjectPanel::saveWorkspace(const QString& filePath, QString* errorMessage)
{
    const QString target = filePath.isEmpty() ? _document.filePath() : filePath;
    if (target.isEmpty())
        return false;
    rebuildDocument();
    if (!_document.save(target, errorMessage))
        return false;
    _document.setFilePath(QFileInfo(target).absoluteFilePath());
    _dirty = false;
    emit workspacePathChanged(_panelId, _document.filePath());
    return true;
}

QTreeWidgetItem* ProjectPanel::addNodeItem(
    QTreeWidgetItem* parent, const WorkspaceNode& node)
{
    QTreeWidgetItem* item = parent
        ? new QTreeWidgetItem(parent, {node.name})
        : new QTreeWidgetItem(_tree, {node.name});
    item->setData(0, NodeTypeRole, node.type);
    item->setData(0, FilePathRole, node.filePath);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    for (const WorkspaceNode& child : node.children)
        addNodeItem(item, child);
    return item;
}

void ProjectPanel::rebuildTree()
{
    _tree->blockSignals(true);
    _tree->clear();
    QTreeWidgetItem* root = new QTreeWidgetItem(_tree, {tr("Workspace")});
    root->setData(0, SyntheticRootRole, true);
    root->setIcon(0, style()->standardIcon(QStyle::SP_DirHomeIcon));
    root->setFlags(root->flags() & ~Qt::ItemIsEditable);
    for (const WorkspaceNode& project : _document.projects())
        addNodeItem(root, project)->setExpanded(true);
    root->setExpanded(true);
    _tree->blockSignals(false);
}

WorkspaceNode ProjectPanel::nodeFromItem(QTreeWidgetItem* item) const
{
    WorkspaceNode node;
    node.type = static_cast<WorkspaceNode::Type>(
        item->data(0, NodeTypeRole).toInt());
    node.name = item->text(0);
    node.filePath = item->data(0, FilePathRole).toString();
    for (int i = 0; i < item->childCount(); ++i)
        node.children.append(nodeFromItem(item->child(i)));
    return node;
}

void ProjectPanel::rebuildDocument()
{
    _document.projects().clear();
    QTreeWidgetItem* root = _tree->topLevelItemCount() > 0
        ? _tree->topLevelItem(0) : nullptr;
    if (!root)
        return;
    for (int i = 0; i < root->childCount(); ++i)
        _document.projects().append(nodeFromItem(root->child(i)));
}

QTreeWidgetItem* ProjectPanel::selectedContainer() const
{
    QTreeWidgetItem* item = _tree->currentItem();
    if (!item)
        return _tree->topLevelItemCount() > 0 ? _tree->topLevelItem(0) : nullptr;
    if (item->data(0, NodeTypeRole).toInt() == WorkspaceNode::File)
        item = item->parent();
    return item;
}

void ProjectPanel::markDirty()
{
    _dirty = true;
    rebuildDocument();
}
