#pragma once

#include "PluginAdminModel.h"
#include "PluginUpdatePlan.h"

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;
class QTabWidget;
class QTableWidget;
class QTextEdit;
class NativeLangSpeaker;

class PluginAdminDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PluginAdminDialog(PluginAdminModel* model,
                               QWidget* parent = nullptr);

    QVector<PluginOperation> selectedOperations() const
    {
        return _operations;
    }
    void applyLocalization(NativeLangSpeaker& speaker);

private:
    QTableWidget* createTable(const QString& objectName, bool checkable);
    void populateTable(QTableWidget* table,
                       const QVector<PluginAdminItem>& items,
                       bool checkable);
    void updateCurrentPage();
    void updateDescription(QTableWidget* table);
    void runCurrentOperation();
    void filterAvailable(const QString& text);
    static PluginAdminItem rowItem(QTableWidget* table, int row);

    PluginAdminModel* _model = nullptr;
    QTabWidget* _tabs = nullptr;
    QLineEdit* _search = nullptr;
    QTableWidget* _available = nullptr;
    QTableWidget* _updates = nullptr;
    QTableWidget* _installed = nullptr;
    QTableWidget* _incompatible = nullptr;
    QTextEdit* _description = nullptr;
    QLabel* _catalogVersion = nullptr;
    QPushButton* _operationButton = nullptr;
    QVector<PluginOperation> _operations;
    QString _installText;
    QString _updateText;
    QString _removeText;
};
