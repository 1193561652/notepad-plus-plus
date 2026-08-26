// FunctionListPanel.h - 函数列表面板
// 移植自: v8.4.6:PowerEditor/src/WinControls/FunctionList/

#ifndef FUNCTIONLISTPANEL_H
#define FUNCTIONLISTPANEL_H

#include <QWidget>
#include <QListWidget>
#include <QToolButton>
#include <QLineEdit>
#include "ScintillaComponent/ScintillaEditView.h"

class FunctionListPanel : public QWidget
{
    Q_OBJECT

public:
    explicit FunctionListPanel(QWidget* parent = nullptr);
    ~FunctionListPanel() = default;

    // 设置当前要分析的视图（tab 切换或保存时调用）
    void updateForView(ScintillaEditView* view);
    bool serialize(const QString& outputFilePath,
                   const QString& sourceName) const;
    void refreshResources(bool darkMode);

signals:
    void navigationRequested(int line);

public slots:
    void refresh();

private slots:
    void onItemActivated(QListWidgetItem* item);
    void onFilterChanged(const QString& text);

private:
    struct FuncEntry { QString display; int line; };
    QList<FuncEntry> parseText(const QString& text, const QString& language) const;

    QListWidget*       _list        = nullptr;
    QLineEdit*         _filterEdit  = nullptr;
    QToolButton*       _refreshBtn  = nullptr;
    QToolButton*       _sortBtn     = nullptr;
    QToolButton*       _preferencesBtn = nullptr;
    ScintillaEditView* _currentView = nullptr;

    QList<FuncEntry>   _allEntries;
};

#endif // FUNCTIONLISTPANEL_H
