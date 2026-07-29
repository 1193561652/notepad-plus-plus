// FileBrowserPanel.h - 文件浏览器面板
// 移植自: v8.4.6:PowerEditor/src/WinControls/FileBrowser/

#ifndef FILEBROWSERPANEL_H
#define FILEBROWSERPANEL_H

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>

class FileBrowserPanel : public QWidget
{
    Q_OBJECT

public:
    explicit FileBrowserPanel(QWidget* parent = nullptr);
    ~FileBrowserPanel() = default;

    void setRootPath(const QString& path);
    QString rootPath() const;
    QString selectedPath() const;
    void setSelectedPath(const QString& path);

signals:
    // 用户激活（双击/回车）某个文件时发出
    void fileActivated(const QString& filePath);

private slots:
    void onItemActivated(const QModelIndex& index);
    void onSetRootClicked();

private:
    QTreeView*        _treeView   = nullptr;
    QFileSystemModel* _model      = nullptr;
    QPushButton*      _setRootBtn = nullptr;
    QLineEdit*        _rootEdit   = nullptr;
};

#endif // FILEBROWSERPANEL_H
