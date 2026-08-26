// FileBrowserPanel.cpp - 文件浏览器面板实现
// 移植自: v8.4.6:PowerEditor/src/WinControls/FileBrowser/

#include "fileBrowser.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QToolButton>
#include "WinControls/UiResourceLoader.h"

namespace {
class OriginalIconFileSystemModel : public QFileSystemModel
{
public:
    explicit OriginalIconFileSystemModel(QObject* parent = nullptr)
        : QFileSystemModel(parent),
          _folderClosed(NppUiResources::bitmapIcon(
              QStringLiteral(":/icons/project_folder_close.bmp"),
              NppUiResources::BitmapMode::MaskGray192)),
          _folderOpen(NppUiResources::bitmapIcon(
              QStringLiteral(":/icons/project_folder_open.bmp"),
              NppUiResources::BitmapMode::MaskGray192)),
          _file(NppUiResources::bitmapIcon(
              QStringLiteral(":/icons/project_file.bmp"),
              NppUiResources::BitmapMode::MaskGray192))
    {
    }

    void setTreeView(QTreeView* tree) { _tree = tree; }

    QVariant data(const QModelIndex& index, int role) const override
    {
        if (role == Qt::DecorationRole && index.column() == 0) {
            if (!isDir(index))
                return _file;
            return _tree && _tree->isExpanded(index) ? _folderOpen
                                                      : _folderClosed;
        }
        return QFileSystemModel::data(index, role);
    }

private:
    QTreeView* _tree = nullptr;
    QIcon _folderClosed;
    QIcon _folderOpen;
    QIcon _file;
};
}

FileBrowserPanel::FileBrowserPanel(QWidget* parent)
    : QWidget(parent)
{
    // 文件系统模型（只显示文件和目录，不显示隐藏文件）
    _model = new OriginalIconFileSystemModel(this);
    _model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    _model->setRootPath(QDir::rootPath());

    // 树形视图
    _treeView = new QTreeView(this);
    _treeView->setModel(_model);
    static_cast<OriginalIconFileSystemModel*>(_model)->setTreeView(_treeView);
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

    _locateBtn = new QToolButton(this);
    _locateBtn->setObjectName(QStringLiteral("fileBrowserLocateButton"));
    _locateBtn->setToolTip(tr("Locate current file"));
    _collapseBtn = new QToolButton(this);
    _collapseBtn->setObjectName(QStringLiteral("fileBrowserCollapseButton"));
    _collapseBtn->setToolTip(tr("Collapse all"));
    _expandBtn = new QToolButton(this);
    _expandBtn->setObjectName(QStringLiteral("fileBrowserExpandButton"));
    _expandBtn->setToolTip(tr("Expand all"));

    QHBoxLayout* topBar = new QHBoxLayout();
    topBar->setContentsMargins(2, 2, 2, 2);
    topBar->setSpacing(4);
    topBar->addWidget(_rootEdit);
    topBar->addWidget(_locateBtn);
    topBar->addWidget(_collapseBtn);
    topBar->addWidget(_expandBtn);
    topBar->addWidget(_setRootBtn);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addLayout(topBar);
    mainLayout->addWidget(_treeView);

    connect(_treeView, &QTreeView::activated,
            this, &FileBrowserPanel::onItemActivated);
    connect(_treeView, &QTreeView::expanded,
            _treeView->viewport(), QOverload<>::of(&QWidget::update));
    connect(_treeView, &QTreeView::collapsed,
            _treeView->viewport(), QOverload<>::of(&QWidget::update));
    connect(_setRootBtn, &QPushButton::clicked,
            this, &FileBrowserPanel::onSetRootClicked);
    connect(_locateBtn, &QToolButton::clicked,
            this, &FileBrowserPanel::locateCurrentFileRequested);
    connect(_collapseBtn, &QToolButton::clicked,
            _treeView, &QTreeView::collapseAll);
    connect(_expandBtn, &QToolButton::clicked,
            _treeView, &QTreeView::expandAll);

    refreshResources(false);

    // 默认展开到当前工作目录
    setRootPath(QDir::currentPath());
}

void FileBrowserPanel::refreshResources(bool darkMode)
{
    const QString base = darkMode
        ? QStringLiteral(":/icons/darkMode/panels/")
        : QStringLiteral(":/icons/");
    _locateBtn->setIcon(NppUiResources::bitmapIcon(
        base + QStringLiteral("fb_select_current_file.bmp")));
    _collapseBtn->setIcon(NppUiResources::bitmapIcon(
        base + QStringLiteral("fb_fold_all.bmp")));
    _expandBtn->setIcon(NppUiResources::bitmapIcon(
        base + QStringLiteral("fb_expand_all.bmp")));
    _setRootBtn->setIcon(NppUiResources::bitmapIcon(
        QStringLiteral(":/icons/fb_root_open.bmp"),
        NppUiResources::BitmapMode::MaskGray192));
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
