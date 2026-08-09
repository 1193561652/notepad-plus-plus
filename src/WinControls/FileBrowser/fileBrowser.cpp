// FileBrowserPanel.cpp - 文件浏览器面板实现
// 移植自: v8.4.6:PowerEditor/src/WinControls/FileBrowser/

#include "fileBrowser.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QTimer>

FileBrowserPanel::FileBrowserPanel(QWidget* parent)
    : QWidget(parent)
{
    // 文件系统模型（只显示文件和目录，不显示隐藏文件）
    _model = new QFileSystemModel(this);
    _model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    _model->setRootPath(QDir::rootPath());

    // 树形视图
    _treeView = new QTreeView(this);
    _treeView->setModel(_model);
    _treeView->setHeaderHidden(true);
    // 只显示文件名列，隐藏 Size / Type / Date 列
    _treeView->hideColumn(1);
    _treeView->hideColumn(2);
    _treeView->hideColumn(3);
    _treeView->setAnimated(true);
    _treeView->setIndentation(16);
    _treeView->setSortingEnabled(false);
    _treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 顶部工具栏：显示当前根路径 + 设置按钮
    _rootEdit = new QLineEdit(this);
    _rootEdit->setReadOnly(true);
    _rootEdit->setPlaceholderText(tr("No folder selected"));
    _rootEdit->setToolTip(tr("Current root folder"));

    _setRootBtn = new QPushButton(tr("..."), this);
    _setRootBtn->setFixedWidth(28);
    _setRootBtn->setToolTip(tr("Set root folder"));

    QHBoxLayout* topBar = new QHBoxLayout();
    topBar->setContentsMargins(2, 2, 2, 2);
    topBar->setSpacing(4);
    topBar->addWidget(_rootEdit);
    topBar->addWidget(_setRootBtn);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addLayout(topBar);
    mainLayout->addWidget(_treeView);

    connect(_treeView, &QTreeView::activated,
            this, &FileBrowserPanel::onItemActivated);
    connect(_setRootBtn, &QPushButton::clicked,
            this, &FileBrowserPanel::onSetRootClicked);

    // 默认展开到当前工作目录
    setRootPath(QDir::currentPath());
}

void FileBrowserPanel::setRootPath(const QString& path)
{
    if (path.isEmpty()) return;
    QModelIndex root = _model->setRootPath(path);
    _treeView->setRootIndex(root);
    _rootEdit->setText(QDir::toNativeSeparators(path));
    _rootEdit->setToolTip(path);
}

QString FileBrowserPanel::rootPath() const
{
    return _model->rootPath();
}

QString FileBrowserPanel::selectedPath() const
{
    const QModelIndex index = _treeView->currentIndex();
    return index.isValid() ? _model->filePath(index) : QString();
}

void FileBrowserPanel::setSelectedPath(const QString& path)
{
    if (path.isEmpty())
        return;

    const auto selectPath = [this, path]() {
        const QModelIndex index = _model->index(path);
        if (!index.isValid())
            return;
        QModelIndex parent = index.parent();
        while (parent.isValid()) {
            _treeView->expand(parent);
            parent = parent.parent();
        }
        _treeView->setCurrentIndex(index);
        _treeView->scrollTo(index, QAbstractItemView::PositionAtCenter);
    };
    selectPath();
    QTimer::singleShot(0, this, selectPath);
}

void FileBrowserPanel::onItemActivated(const QModelIndex& index)
{
    if (!_model->isDir(index))
        emit fileActivated(_model->filePath(index));
}

void FileBrowserPanel::onSetRootClicked()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Select Root Folder"),
        rootPath().isEmpty() ? QDir::homePath() : rootPath());

    if (!dir.isEmpty())
        setRootPath(dir);
}
