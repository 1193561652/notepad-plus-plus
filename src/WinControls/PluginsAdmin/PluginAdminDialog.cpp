#include "PluginAdminDialog.h"
#include "localization.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {

const int kItemRole = Qt::UserRole;

QVariant itemVariant(const PluginAdminItem& item)
{
    QVariantMap map;
    map.insert(QStringLiteral("folder"), item.folderName);
    map.insert(QStringLiteral("display"), item.displayName);
    map.insert(QStringLiteral("description"), item.description);
    map.insert(QStringLiteral("author"), item.author);
    map.insert(QStringLiteral("repository"), item.repository);
    map.insert(QStringLiteral("sha256"), item.packageSha256);
    map.insert(QStringLiteral("installed"),
               item.installedVersion.toString());
    map.insert(QStringLiteral("available"),
               item.availableVersion.toString());
    return map;
}

PluginAdminItem variantItem(const QVariant& value)
{
    const QVariantMap map = value.toMap();
    PluginAdminItem item;
    item.folderName = map.value(QStringLiteral("folder")).toString();
    item.displayName = map.value(QStringLiteral("display")).toString();
    item.description = map.value(QStringLiteral("description")).toString();
    item.author = map.value(QStringLiteral("author")).toString();
    item.repository = map.value(QStringLiteral("repository")).toString();
    item.packageSha256 = map.value(QStringLiteral("sha256")).toString();
    item.installedVersion =
        PluginVersion(map.value(QStringLiteral("installed")).toString());
    item.availableVersion =
        PluginVersion(map.value(QStringLiteral("available")).toString());
    return item;
}

} // namespace

PluginAdminDialog::PluginAdminDialog(PluginAdminModel* model, QWidget* parent)
    : QDialog(parent), _model(model),
      _installText(tr("Install")),
      _updateText(tr("Update")),
      _removeText(tr("Remove"))
{
    setObjectName(QStringLiteral("pluginsAdminDialog"));
    setWindowTitle(tr("Plugins Admin"));
    resize(760, 560);

    QVBoxLayout* root = new QVBoxLayout(this);
    QHBoxLayout* searchRow = new QHBoxLayout;
    QLabel* searchLabel = new QLabel(tr("Search:"), this);
    searchLabel->setObjectName(QStringLiteral("pluginsAdminSearchLabel"));
    _search = new QLineEdit(this);
    _search->setObjectName(QStringLiteral("pluginsAdminSearchEdit"));
    searchRow->addWidget(searchLabel);
    searchRow->addWidget(_search);
    root->addLayout(searchRow);

    _tabs = new QTabWidget(this);
    _tabs->setObjectName(QStringLiteral("pluginsAdminTabs"));
    _tabs->tabBar()->setObjectName(QStringLiteral("pluginsAdminTabs"));
    _available = createTable(QStringLiteral("availablePluginsTable"), true);
    _updates = createTable(QStringLiteral("updatedPluginsTable"), true);
    _installed = createTable(QStringLiteral("installedPluginsTable"), false);
    _installed->setColumnCount(3);
    _installed->setHorizontalHeaderLabels(
        {tr("Plugin"), tr("Enabled"), tr("Version")});
    _installed->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);
    _installed->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    _installed->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);
    _incompatible =
        createTable(QStringLiteral("incompatiblePluginsTable"), false);
    _tabs->addTab(_available, tr("Available"));
    _tabs->addTab(_updates, tr("Updates"));
    _tabs->addTab(_installed, tr("Installed"));
    _tabs->addTab(_incompatible, tr("Incompatible"));
    root->addWidget(_tabs, 1);

    _description = new QTextEdit(this);
    _description->setObjectName(QStringLiteral("pluginsAdminDescription"));
    _description->setReadOnly(true);
    _description->setMaximumHeight(120);
    root->addWidget(_description);

    QHBoxLayout* bottom = new QHBoxLayout;
    _catalogVersion = new QLabel(this);
    _catalogVersion->setObjectName(
        QStringLiteral("pluginsAdminCatalogVersion"));
    _catalogVersion->setText(
        tr("Plugin list version: %1")
            .arg(model ? model->catalogVersion() : QString()));
    bottom->addWidget(_catalogVersion);
    bottom->addStretch();
    _operationButton = new QPushButton(this);
    _operationButton->setObjectName(
        QStringLiteral("pluginsAdminOperationButton"));
    QPushButton* closeButton = new QPushButton(tr("Close"), this);
    closeButton->setObjectName(QStringLiteral("pluginsAdminCloseButton"));
    bottom->addWidget(_operationButton);
    bottom->addWidget(closeButton);
    root->addLayout(bottom);

    if (_model) {
        populateTable(_available, _model->availableItems(), true);
        populateTable(_updates, _model->updateItems(), true);
        populateTable(_installed, _model->installedItems(), true);
        populateTable(_incompatible, _model->incompatibleItems(), false);
    }

    connect(_tabs, &QTabWidget::currentChanged, this,
            [this]() { updateCurrentPage(); });
    connect(_search, &QLineEdit::textChanged, this,
            &PluginAdminDialog::filterAvailable);
    connect(_operationButton, &QPushButton::clicked, this,
            &PluginAdminDialog::runCurrentOperation);
    connect(closeButton, &QPushButton::clicked, this,
            &PluginAdminDialog::reject);
    for (QTableWidget* table :
         {_available, _updates, _installed, _incompatible}) {
        connect(table, &QTableWidget::itemSelectionChanged, this,
                [this, table]() { updateDescription(table); });
        connect(table, &QTableWidget::itemChanged, this,
                [this, table](QTableWidgetItem* item) {
                    if (table == _installed && item && item->column() == 1)
                        updatePluginEnabled(item);
                    updateCurrentPage();
                });
    }
    updateCurrentPage();
}

void PluginAdminDialog::applyLocalization(NativeLangSpeaker& speaker)
{
    if (!speaker.isLoaded())
        return;

    speaker.changeDlgLang(this, QStringLiteral("PluginsAdmin"));
    _installText = speaker.getLocalizedStrFromID(
        QStringLiteral("pluginAdminInstall"), _installText);
    _updateText = speaker.getLocalizedStrFromID(
        QStringLiteral("pluginAdminUpdate"), _updateText);
    _removeText = speaker.getLocalizedStrFromID(
        QStringLiteral("pluginAdminRemove"), _removeText);

    const QString pluginHeader = speaker.getLocalizedStrFromID(
        QStringLiteral("pluginAdminPluginColumn"), tr("Plugin"));
    const QString versionHeader = speaker.getLocalizedStrFromID(
        QStringLiteral("pluginAdminVersionColumn"), tr("Version"));
    for (QTableWidget* table : {_available, _updates, _incompatible}) {
        table->setHorizontalHeaderLabels({pluginHeader, versionHeader});
    }
    _installed->setHorizontalHeaderLabels(
        {pluginHeader,
         speaker.getLocalizedStrFromID(
             QStringLiteral("pluginAdminEnabledColumn"), tr("Enabled")),
         versionHeader});
    _catalogVersion->setText(
        speaker.getLocalizedStrFromID(
            QStringLiteral("pluginAdminListVersion"),
            tr("Plugin list version: %1"))
            .arg(_model ? _model->catalogVersion() : QString()));
    updateCurrentPage();
}

QTableWidget* PluginAdminDialog::createTable(const QString& objectName,
                                             bool checkable)
{
    QTableWidget* table = new QTableWidget(this);
    table->setObjectName(objectName);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({tr("Plugin"), tr("Version")});
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setProperty("checkable", checkable);
    return table;
}

void PluginAdminDialog::populateTable(
    QTableWidget* table, const QVector<PluginAdminItem>& items,
    bool checkable)
{
    table->setRowCount(items.size());
    for (int row = 0; row < items.size(); ++row) {
        const PluginAdminItem& item = items.at(row);
        QTableWidgetItem* name = new QTableWidgetItem(item.displayName);
        name->setData(kItemRole, itemVariant(item));
        if (checkable && table != _installed) {
            name->setFlags(name->flags() | Qt::ItemIsUserCheckable);
            name->setCheckState(Qt::Unchecked);
        }
        table->setItem(row, 0, name);

        int versionColumn = 1;
        if (table == _installed) {
            QTableWidgetItem* enabled = new QTableWidgetItem;
            enabled->setFlags(enabled->flags() | Qt::ItemIsUserCheckable);
            enabled->setCheckState(
                item.enabled ? Qt::Checked : Qt::Unchecked);
            table->setItem(row, 1, enabled);
            versionColumn = 2;
        }
        QString version = item.availableVersion.toString();
        if (table == _installed || table == _incompatible)
            version = item.installedVersion.toString();
        if (version.isEmpty())
            version = tr("Unknown");
        table->setItem(
            row, versionColumn, new QTableWidgetItem(version));
    }
    table->sortItems(0, Qt::AscendingOrder);
}

PluginAdminItem PluginAdminDialog::rowItem(QTableWidget* table, int row)
{
    if (!table || row < 0 || row >= table->rowCount() ||
        !table->item(row, 0)) {
        return PluginAdminItem();
    }
    return variantItem(table->item(row, 0)->data(kItemRole));
}

void PluginAdminDialog::updateCurrentPage()
{
    const int page = _tabs->currentIndex();
    _search->setEnabled(page == 0);
    _operationButton->setVisible(page != 3);
    if (page == 0)
        _operationButton->setText(_installText);
    else if (page == 1)
        _operationButton->setText(_updateText);
    else if (page == 2)
        _operationButton->setText(_removeText);

    QTableWidget* table = qobject_cast<QTableWidget*>(
        _tabs->currentWidget());
    bool canOperate = page == 2 && table && table->currentRow() >= 0;
    if (table && table->property("checkable").toBool()) {
        for (int row = 0; row < table->rowCount(); ++row) {
            QTableWidgetItem* item = table->item(row, 0);
            if (item && !table->isRowHidden(row) &&
                item->checkState() == Qt::Checked) {
                canOperate = true;
                break;
            }
        }
    }
    _operationButton->setEnabled(page != 3 && canOperate);
}

void PluginAdminDialog::updateDescription(QTableWidget* table)
{
    if (table != _tabs->currentWidget())
        return;
    const PluginAdminItem item = rowItem(table, table->currentRow());
    QStringList details;
    if (!item.author.isEmpty())
        details.append(tr("Author: %1").arg(item.author));
    if (!item.description.isEmpty())
        details.append(item.description);
    _description->setPlainText(details.join(QStringLiteral("\n\n")));
}

void PluginAdminDialog::filterAvailable(const QString& text)
{
    const QString needle = text.trimmed();
    for (int row = 0; row < _available->rowCount(); ++row) {
        const PluginAdminItem item = rowItem(_available, row);
        const bool visible =
            needle.size() < 2 ||
            item.displayName.contains(needle, Qt::CaseInsensitive) ||
            item.description.contains(needle, Qt::CaseInsensitive);
        _available->setRowHidden(row, !visible);
    }
    updateCurrentPage();
}

void PluginAdminDialog::updatePluginEnabled(QTableWidgetItem* checkItem)
{
    if (_updatingEnablement || !_model || !checkItem)
        return;
    const PluginAdminItem item = rowItem(_installed, checkItem->row());
    const bool enabled = checkItem->checkState() == Qt::Checked;
    QString error;
    if (_model->setPluginEnabled(item.folderName, enabled, &error))
        return;

    _updatingEnablement = true;
    checkItem->setCheckState(enabled ? Qt::Unchecked : Qt::Checked);
    _updatingEnablement = false;
    QMessageBox::warning(
        this, tr("Plugins Admin"),
        tr("Could not save the plugin enablement configuration:\n%1")
            .arg(error));
}

void PluginAdminDialog::runCurrentOperation()
{
    QTableWidget* table = qobject_cast<QTableWidget*>(
        _tabs->currentWidget());
    if (!table)
        return;

    PluginOperationType type = PluginOperationType::Install;
    if (_tabs->currentIndex() == 1)
        type = PluginOperationType::Update;
    else if (_tabs->currentIndex() == 2)
        type = PluginOperationType::Remove;

    QVector<PluginOperation> operations;
    const int firstRow = _tabs->currentIndex() == 2
        ? table->currentRow() : 0;
    const int rowLimit = _tabs->currentIndex() == 2
        ? firstRow + 1 : table->rowCount();
    for (int row = firstRow; row >= 0 && row < rowLimit; ++row) {
        QTableWidgetItem* check = table->item(row, 0);
        if (!check || (_tabs->currentIndex() != 2
                       && check->checkState() != Qt::Checked))
            continue;
        const PluginAdminItem item = rowItem(table, row);
        PluginOperation operation;
        operation.type = type;
        operation.folderName = item.folderName;
        operation.version = item.availableVersion.toString();
        operation.repository = item.repository;
        operation.packageSha256 = item.packageSha256;
        operations.append(operation);
    }
    if (operations.isEmpty())
        return;

    const QMessageBox::StandardButton answer = QMessageBox::question(
        this, tr("Notepad++ is about to exit"),
        tr("Notepad++ must exit to complete the selected plugin "
           "operations and will be restarted afterwards.\n\nContinue?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;
    _operations = operations;
    accept();
}
