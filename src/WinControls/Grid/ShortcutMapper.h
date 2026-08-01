#ifndef SHORTCUTMAPPER_H
#define SHORTCUTMAPPER_H

#include <QDialog>
#include <QList>

class QAction;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTabWidget;

class ShortcutMapper : public QDialog
{
    Q_OBJECT

public:
    explicit ShortcutMapper(const QList<QAction*>& menuActions,
                            QWidget* parent = nullptr);
    bool shortcutsChanged() const { return _shortcutsChanged; }

private:
    enum class TableKind { MainMenu, Macro, RunCommand, Plugin, Scintilla };

    QTableWidget* createTable(TableKind kind);
    void populate(const QList<QAction*>& menuActions);
    void modifySelectedShortcut();
    void clearSelectedShortcut();
    void updateSelectionState();
    void updateFilter(const QString& text);
    void updateConflictText();
    void commitChanges();

    QTabWidget* _tabs = nullptr;
    QLabel* _conflictText = nullptr;
    QLineEdit* _filterEdit = nullptr;
    QPushButton* _modifyButton = nullptr;
    QPushButton* _clearButton = nullptr;
    QPushButton* _deleteButton = nullptr;
    QList<QTableWidget*> _tables;
    bool _shortcutsChanged = false;
};

#endif
