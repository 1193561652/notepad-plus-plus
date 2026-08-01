#include "ShortcutMapper.h"

#include "Parameters.h"

#include <QAction>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QTabBar>
#include <QVBoxLayout>

namespace {
constexpr int KindRole = Qt::UserRole;
constexpr int IndexRole = Qt::UserRole + 1;
constexpr int SubIndexRole = Qt::UserRole + 2;
constexpr int ActionRole = Qt::UserRole + 3;

QString actionCategory(const QAction* action)
{
    const QMenu* menu = qobject_cast<const QMenu*>(action->parent());
    return menu ? QString(menu->title()).remove('&') : QString();
}
}

ShortcutMapper::ShortcutMapper(const QList<QAction*>& menuActions,
                               QWidget* parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("shortcutMapperDialog"));
    setWindowTitle(tr("Shortcut Mapper"));
    setFixedSize(680, 580);

    _tabs = new QTabWidget(this);
    _tabs->setObjectName(QStringLiteral("shortcutMapperTabs"));
    _tabs->addTab(createTable(TableKind::MainMenu), tr("Main menu"));
    _tabs->addTab(createTable(TableKind::Macro), tr("Macros"));
    _tabs->addTab(createTable(TableKind::RunCommand), tr("Run commands"));
    _tabs->addTab(createTable(TableKind::Plugin), tr("Plugin commands"));
    _tabs->addTab(createTable(TableKind::Scintilla), tr("Scintilla commands"));
    _tabs->tabBar()->setObjectName(QStringLiteral("shortcutMapperTabs"));

    _conflictText = new QLabel(this);
    _conflictText->setObjectName(QStringLiteral("shortcutConflictText"));
    _conflictText->setFrameShape(QFrame::StyledPanel);
    _conflictText->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    _conflictText->setMargin(5);
    _conflictText->setMinimumHeight(48);

    QLabel* filterLabel = new QLabel(tr("Filter:"), this);
    filterLabel->setObjectName(QStringLiteral("lblShortcutFilter"));
    _filterEdit = new QLineEdit(this);
    _filterEdit->setObjectName(QStringLiteral("shortcutFilterEdit"));

    _modifyButton = new QPushButton(tr("Modify"), this);
    _clearButton = new QPushButton(tr("Clear"), this);
    _deleteButton = new QPushButton(tr("Delete"), this);
    QPushButton* closeButton = new QPushButton(tr("Close"), this);
    _modifyButton->setObjectName(QStringLiteral("btnShortcutModify"));
    _clearButton->setObjectName(QStringLiteral("btnShortcutClear"));
    _deleteButton->setObjectName(QStringLiteral("btnShortcutDelete"));
    closeButton->setObjectName(QStringLiteral("btnShortcutClose"));
    closeButton->setDefault(true);
    _modifyButton->setFixedWidth(69);
    _clearButton->setFixedWidth(69);
    _deleteButton->setFixedWidth(69);
    closeButton->setFixedWidth(69);

    QHBoxLayout* filterLayout = new QHBoxLayout();
    filterLayout->setContentsMargins(6, 0, 0, 0);
    filterLayout->addWidget(filterLabel);
    filterLayout->addWidget(_filterEdit, 1);

    QHBoxLayout* buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(_modifyButton);
    buttons->addSpacing(7);
    buttons->addWidget(_clearButton);
    buttons->addSpacing(7);
    buttons->addWidget(_deleteButton);
    buttons->addSpacing(7);
    buttons->addWidget(closeButton);
    buttons->addStretch();

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(5);
    layout->addWidget(_tabs, 1);
    layout->addWidget(_conflictText);
    layout->addLayout(filterLayout);
    layout->addLayout(buttons);

    populate(menuActions);
    connect(_tabs, &QTabWidget::currentChanged, this,
            [this]() { updateSelectionState(); updateConflictText(); });
    connect(_filterEdit, &QLineEdit::textChanged,
            this, &ShortcutMapper::updateFilter);
    connect(_modifyButton, &QPushButton::clicked,
            this, &ShortcutMapper::modifySelectedShortcut);
    connect(_clearButton, &QPushButton::clicked,
            this, &ShortcutMapper::clearSelectedShortcut);
    connect(closeButton, &QPushButton::clicked, this, [this]() {
        commitChanges();
        accept();
    });
    updateSelectionState();
    updateConflictText();
}

QTableWidget* ShortcutMapper::createTable(TableKind kind)
{
    QTableWidget* table = new QTableWidget(this);
    table->setObjectName(QStringLiteral("shortcutTable%1").arg(_tables.size()));
    table->setProperty("shortcutTableKind", static_cast<int>(kind));
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({tr("Name"), tr("Shortcut"), tr("Category")});
    table->horizontalHeader()->setSectionsClickable(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setColumnWidth(0, 290);
    table->setColumnWidth(1, 140);
    table->verticalHeader()->setDefaultSectionSize(20);
    table->verticalHeader()->setMinimumWidth(28);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(table, &QTableWidget::itemSelectionChanged,
            this, &ShortcutMapper::updateSelectionState);
    connect(table, &QTableWidget::itemDoubleClicked,
            this, [this](QTableWidgetItem*) { modifySelectedShortcut(); });
    _tables.append(table);
    return table;
}

void ShortcutMapper::populate(const QList<QAction*>& menuActions)
{
    QTableWidget* mainTable = _tables.at(0);
    QList<QAction*> actions;
    for (QAction* action : menuActions) {
        if (action && !action->text().isEmpty() && !action->isSeparator()
            && action->property("nppCommandId").isValid()
            && !actions.contains(action))
            actions.append(action);
    }
    mainTable->setRowCount(actions.size());
    for (int row = 0; row < actions.size(); ++row) {
        QAction* action = actions.at(row);
        QTableWidgetItem* name = new QTableWidgetItem(QString(action->text()).remove('&'));
        name->setData(KindRole, static_cast<int>(TableKind::MainMenu));
        name->setData(ActionRole, QVariant::fromValue<quintptr>(
            reinterpret_cast<quintptr>(action)));
        mainTable->setItem(row, 0, name);
        mainTable->setItem(row, 1, new QTableWidgetItem(
            action->shortcut().toString(QKeySequence::NativeText)));
        mainTable->setItem(row, 2, new QTableWidgetItem(actionCategory(action)));
    }

    NppParameters& parameters = NppParameters::getInstance();
    const QVector<MacroDef>& macros = parameters.getMacros();
    QTableWidget* macroTable = _tables.at(1);
    macroTable->setRowCount(macros.size());
    for (int row = 0; row < macros.size(); ++row) {
        ShortcutKey key{macros.at(row).ctrl, macros.at(row).alt,
                        macros.at(row).shift, macros.at(row).key};
        QTableWidgetItem* name = new QTableWidgetItem(macros.at(row).name);
        name->setData(KindRole, static_cast<int>(TableKind::Macro));
        name->setData(IndexRole, row);
        macroTable->setItem(row, 0, name);
        macroTable->setItem(row, 1, new QTableWidgetItem(
            key.toKeySequence().toString(QKeySequence::NativeText)));
        macroTable->setItem(row, 2, new QTableWidgetItem(tr("Macro")));
    }

    const QVector<UserCommandDef>& commands = parameters.getUserCommands();
    QTableWidget* runTable = _tables.at(2);
    runTable->setRowCount(commands.size());
    for (int row = 0; row < commands.size(); ++row) {
        ShortcutKey key{commands.at(row).ctrl, commands.at(row).alt,
                        commands.at(row).shift, commands.at(row).key};
        QTableWidgetItem* name = new QTableWidgetItem(commands.at(row).name);
        name->setData(KindRole, static_cast<int>(TableKind::RunCommand));
        name->setData(IndexRole, row);
        runTable->setItem(row, 0, name);
        runTable->setItem(row, 1, new QTableWidgetItem(
            key.toKeySequence().toString(QKeySequence::NativeText)));
        runTable->setItem(row, 2, new QTableWidgetItem(tr("Run command")));
    }

    const QVector<ScintillaKeyDef>& scintilla = parameters.getScintillaKeys();
    QTableWidget* scintillaTable = _tables.at(4);
    int rows = 0;
    for (const ScintillaKeyDef& command : scintilla)
        rows += command.shortcuts.size();
    scintillaTable->setRowCount(rows);
    int row = 0;
    for (int commandIndex = 0; commandIndex < scintilla.size(); ++commandIndex) {
        const ScintillaKeyDef& command = scintilla.at(commandIndex);
        for (int keyIndex = 0; keyIndex < command.shortcuts.size();
             ++keyIndex, ++row) {
            QTableWidgetItem* name = new QTableWidgetItem(
                tr("Scintilla command %1").arg(command.scintillaId));
            name->setData(KindRole, static_cast<int>(TableKind::Scintilla));
            name->setData(IndexRole, commandIndex);
            name->setData(SubIndexRole, keyIndex);
            scintillaTable->setItem(row, 0, name);
            scintillaTable->setItem(row, 1, new QTableWidgetItem(
                command.shortcuts.at(keyIndex).toKeySequence().toString(
                    QKeySequence::NativeText)));
            scintillaTable->setItem(row, 2,
                new QTableWidgetItem(tr("Scintilla")));
        }
    }
    if (mainTable->rowCount() > 0)
        mainTable->selectRow(0);
}

void ShortcutMapper::modifySelectedShortcut()
{
    QTableWidget* table = qobject_cast<QTableWidget*>(_tabs->currentWidget());
    if (!table || table->currentRow() < 0)
        return;
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Modify Shortcut"));
    QKeySequenceEdit* edit = new QKeySequenceEdit(
        QKeySequence(table->item(table->currentRow(), 1)->text()), &dialog);
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->addWidget(edit);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted)
        return;
    table->item(table->currentRow(), 1)->setText(
        edit->keySequence().toString(QKeySequence::NativeText));
    _shortcutsChanged = true;
    updateConflictText();
}

void ShortcutMapper::clearSelectedShortcut()
{
    QTableWidget* table = qobject_cast<QTableWidget*>(_tabs->currentWidget());
    if (!table || table->currentRow() < 0)
        return;
    table->item(table->currentRow(), 1)->setText(QString());
    _shortcutsChanged = true;
    updateConflictText();
}

void ShortcutMapper::updateSelectionState()
{
    QTableWidget* table = qobject_cast<QTableWidget*>(_tabs->currentWidget());
    const bool selected = table && table->currentRow() >= 0;
    _modifyButton->setEnabled(selected);
    _clearButton->setEnabled(selected);
    _deleteButton->setEnabled(false);
}

void ShortcutMapper::updateFilter(const QString& text)
{
    QTableWidget* table = qobject_cast<QTableWidget*>(_tabs->currentWidget());
    if (!table)
        return;
    for (int row = 0; row < table->rowCount(); ++row) {
        bool match = text.isEmpty();
        for (int column = 0; !match && column < table->columnCount(); ++column)
            match = table->item(row, column)->text().contains(text,
                Qt::CaseInsensitive);
        table->setRowHidden(row, !match);
    }
}

void ShortcutMapper::updateConflictText()
{
    QTableWidget* table = qobject_cast<QTableWidget*>(_tabs->currentWidget());
    if (!table || table->currentRow() < 0) {
        _conflictText->clear();
        return;
    }
    const QString key = table->item(table->currentRow(), 1)->text();
    if (key.isEmpty()) {
        _conflictText->setText(tr("This shortcut has no conflicts."));
        return;
    }
    QStringList conflicts;
    for (QTableWidget* candidate : _tables) {
        for (int row = 0; row < candidate->rowCount(); ++row) {
            if (candidate == table && row == table->currentRow())
                continue;
            if (candidate->item(row, 1)->text() == key)
                conflicts.append(candidate->item(row, 0)->text());
        }
    }
    _conflictText->setText(conflicts.isEmpty()
        ? tr("This shortcut has no conflicts.")
        : tr("This shortcut conflicts with: %1").arg(conflicts.join(", ")));
}

void ShortcutMapper::commitChanges()
{
    if (!_shortcutsChanged)
        return;
    NppParameters& parameters = NppParameters::getInstance();
    for (QTableWidget* table : _tables) {
        for (int row = 0; row < table->rowCount(); ++row) {
            QTableWidgetItem* name = table->item(row, 0);
            const QKeySequence sequence(table->item(row, 1)->text());
            const TableKind kind = static_cast<TableKind>(name->data(KindRole).toInt());
            const ShortcutKey key = ShortcutKey::fromKeySequence(sequence);
            if (kind == TableKind::MainMenu) {
                QAction* action = reinterpret_cast<QAction*>(
                    name->data(ActionRole).value<quintptr>());
                if (action) {
                    action->setShortcut(sequence);
                    parameters.setInternalCommandShortcut(
                        action->property("nppCommandId").toInt(), sequence);
                }
            } else if (kind == TableKind::Macro) {
                MacroDef& macro = parameters.getMacros()[name->data(IndexRole).toInt()];
                macro.ctrl = key.ctrl; macro.alt = key.alt;
                macro.shift = key.shift; macro.key = key.key;
            } else if (kind == TableKind::RunCommand) {
                UserCommandDef& command = parameters.getUserCommands()[name->data(IndexRole).toInt()];
                command.ctrl = key.ctrl; command.alt = key.alt;
                command.shift = key.shift; command.key = key.key;
            } else if (kind == TableKind::Scintilla) {
                parameters.getScintillaKeys()[name->data(IndexRole).toInt()]
                    .shortcuts[name->data(SubIndexRole).toInt()] = key;
            }
        }
    }
    parameters.writeShortcuts();
}
