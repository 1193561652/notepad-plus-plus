#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QVector>

#include <iostream>

namespace {

int failures = 0;

void check(bool condition, const QString& message)
{
    if (condition)
        return;
    std::cerr << "FAIL: " << message.toStdString() << '\n';
    ++failures;
}

QString readUtf8(const QString& path)
{
    QFile file(path);
    check(file.open(QFile::ReadOnly),
          QStringLiteral("could not read %1").arg(path));
    return QString::fromUtf8(file.readAll());
}

QStringList tableCells(const QString& line)
{
    QStringList cells = line.split(QLatin1Char('|'));
    if (!cells.isEmpty() && cells.first().trimmed().isEmpty())
        cells.removeFirst();
    if (!cells.isEmpty() && cells.last().trimmed().isEmpty())
        cells.removeLast();
    for (QString& cell : cells)
        cell = cell.trimmed();
    return cells;
}

struct InventoryRow
{
    QString folder;
    QString version;
    QString source;
    QString importance;
    QString grade;
};

QVector<InventoryRow> parseInventory(const QString& text)
{
    QVector<InventoryRow> rows;
    const QRegularExpression numbered(QStringLiteral("^\\d+$"));
    for (const QString& line : text.split(QLatin1Char('\n'))) {
        const QStringList cells = tableCells(line);
        if (cells.size() != 10 ||
            !numbered.match(cells.at(0)).hasMatch()) {
            continue;
        }
        InventoryRow row;
        row.folder = cells.at(1);
        row.version = cells.at(3);
        row.source = cells.at(7);
        row.importance = cells.at(8);
        row.grade = cells.at(9);
        rows.append(row);
    }
    return rows;
}

QHash<QString, QPair<QString, QString>> parseImportance(
    const QString& text)
{
    QHash<QString, QPair<QString, QString>> rows;
    const QSet<QString> levels = {
        QStringLiteral("高"), QStringLiteral("中"), QStringLiteral("低")
    };
    const QSet<QString> grades = {
        QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C"),
        QStringLiteral("D"), QStringLiteral("U")
    };
    for (const QString& line : text.split(QLatin1Char('\n'))) {
        const QStringList cells = tableCells(line);
        if (cells.size() != 7 || !levels.contains(cells.at(2)) ||
            !grades.contains(cells.at(5))) {
            continue;
        }
        rows.insert(cells.at(0), {cells.at(2), cells.at(5)});
    }
    return rows;
}

bool batchContains(const QString& text, const QString& folder)
{
    return text.contains(QStringLiteral("| `%1` |").arg(folder)) ||
           text.contains(QStringLiteral("| %1 |").arg(folder)) ||
           text.contains(QStringLiteral("`%1`").arg(folder));
}

int expectedBatch(const QString& folder)
{
    int offset = 0;
    while (offset < folder.size() && !folder.at(offset).isLetterOrNumber())
        ++offset;
    if (offset == folder.size() || folder.at(offset).isDigit())
        return 0;

    const QChar initial = folder.at(offset).toUpper();
    if (initial <= QLatin1Char('C'))
        return 0;
    if (initial <= QLatin1Char('H'))
        return 1;
    if (initial <= QLatin1Char('M'))
        return 2;
    if (initial == QLatin1Char('N'))
        return 3;
    if (initial <= QLatin1Char('S'))
        return 4;
    return 5;
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    const QString catalogText = readUtf8(
        QString::fromUtf8(NPP_PLUGIN_CATALOG));
    const QJsonDocument catalog =
        QJsonDocument::fromJson(catalogText.toUtf8());
    const QJsonArray plugins =
        catalog.object().value(QStringLiteral("npp-plugins")).toArray();
    check(plugins.size() == 169,
          QStringLiteral("x86 catalog must contain 169 plugins"));

    const QVector<InventoryRow> inventory = parseInventory(
        readUtf8(QString::fromUtf8(NPP_PLUGIN_INVENTORY)));
    const QHash<QString, QPair<QString, QString>> importance =
        parseImportance(readUtf8(
            QString::fromUtf8(NPP_PLUGIN_IMPORTANCE)));
    check(inventory.size() == plugins.size(),
          QStringLiteral("inventory must match catalog size"));
    check(importance.size() == plugins.size(),
          QStringLiteral("importance matrix must match catalog size"));

    const QStringList batchPaths = {
        QString::fromUtf8(NPP_PLUGIN_BATCH_A_C),
        QString::fromUtf8(NPP_PLUGIN_BATCH_D_H),
        QString::fromUtf8(NPP_PLUGIN_BATCH_I_M),
        QString::fromUtf8(NPP_PLUGIN_BATCH_N),
        QString::fromUtf8(NPP_PLUGIN_BATCH_O_S),
        QString::fromUtf8(NPP_PLUGIN_BATCH_T_Z)
    };
    QStringList batchTexts;
    for (const QString& path : batchPaths)
        batchTexts.append(readUtf8(path));

    QHash<QString, int> gradeCounts;
    QHash<QString, int> importanceCounts;
    QVector<int> batchCounts(batchTexts.size());
    int exactSource = 0;
    QSet<QString> folders;
    for (int index = 0; index < inventory.size(); ++index) {
        const InventoryRow& row = inventory.at(index);
        const QJsonObject catalogEntry = plugins.at(index).toObject();
        const QString catalogFolder =
            catalogEntry.value(QStringLiteral("folder-name")).toString();
        const QString catalogVersion =
            catalogEntry.value(QStringLiteral("version")).toString();
        check(row.folder == catalogFolder,
              QStringLiteral("inventory order/folder mismatch at %1")
                  .arg(index + 1));
        check(row.version == catalogVersion,
              QStringLiteral("version mismatch for %1").arg(row.folder));
        check(!folders.contains(row.folder.toCaseFolded()),
              QStringLiteral("duplicate inventory folder %1")
                  .arg(row.folder));
        folders.insert(row.folder.toCaseFolded());

        check(importance.contains(row.folder),
              QStringLiteral("importance row missing for %1")
                  .arg(row.folder));
        if (importance.contains(row.folder)) {
            check(importance.value(row.folder).first == row.importance,
                  QStringLiteral("importance mismatch for %1")
                      .arg(row.folder));
            check(importance.value(row.folder).second == row.grade,
                  QStringLiteral("grade mismatch for %1").arg(row.folder));
        }

        const int batchIndex = expectedBatch(row.folder);
        ++batchCounts[batchIndex];
        check(batchContains(batchTexts.at(batchIndex), row.folder),
                  QStringLiteral("%1 missing from assigned letter batch")
                      .arg(row.folder));

        ++gradeCounts[row.grade];
        ++importanceCounts[row.importance];
        if (row.source.startsWith(QStringLiteral("有，")))
            ++exactSource;
        else
            check(row.grade == QStringLiteral("U"),
                  QStringLiteral("non-exact source must remain U: %1")
                      .arg(row.folder));
    }

    check(gradeCounts.value(QStringLiteral("A")) == 13 &&
              gradeCounts.value(QStringLiteral("B")) == 25 &&
              gradeCounts.value(QStringLiteral("C")) == 25 &&
              gradeCounts.value(QStringLiteral("D")) == 1 &&
              gradeCounts.value(QStringLiteral("U")) == 105,
          QStringLiteral("compatibility grade totals changed"));
    check(importanceCounts.value(QStringLiteral("高")) == 20 &&
              importanceCounts.value(QStringLiteral("中")) == 94 &&
              importanceCounts.value(QStringLiteral("低")) == 55,
          QStringLiteral("importance totals changed"));
    check(exactSource == 64,
          QStringLiteral("exact-version source total must be 64"));
    check(batchCounts == QVector<int>({24, 23, 23, 50, 32, 17}),
          QStringLiteral("letter batch totals changed"));

    const QString summary = readUtf8(
        QString::fromUtf8(NPP_PLUGIN_API_MATRIX));
    check(summary.contains(QStringLiteral("64")) &&
              summary.contains(QStringLiteral("105")) &&
              summary.contains(QStringLiteral("SCI_GETDIRECTFUNCTION")) &&
              summary.contains(QStringLiteral("NPPM_GETCURRENTSCINTILLA")),
          QStringLiteral("API matrix is missing current corpus conclusions"));

    if (failures == 0)
        std::cout << "Plugin investigation tests passed\n";
    return failures == 0 ? 0 : 1;
}
