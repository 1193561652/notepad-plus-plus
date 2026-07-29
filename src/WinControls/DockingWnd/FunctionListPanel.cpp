// FunctionListPanel.cpp - 函数列表面板实现

#include "FunctionListPanel.h"
#include <Qsci/qscilexer.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRegExp>
#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "MISC/FunctionListParser.h"
#include "Parameters.h"

FunctionListPanel::FunctionListPanel(QWidget* parent)
    : QWidget(parent)
{
    _filterEdit = new QLineEdit(this);
    _filterEdit->setPlaceholderText(tr("Filter..."));

    _refreshBtn = new QPushButton(tr("Refresh"), this);
    _refreshBtn->setFixedWidth(60);

    QHBoxLayout* topBar = new QHBoxLayout();
    topBar->setContentsMargins(2, 2, 2, 2);
    topBar->addWidget(_filterEdit);
    topBar->addWidget(_refreshBtn);

    _list = new QListWidget(this);
    _list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QVBoxLayout* mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(0, 0, 0, 0);
    mainLay->setSpacing(0);
    mainLay->addLayout(topBar);
    mainLay->addWidget(_list);

    connect(_refreshBtn, &QPushButton::clicked, this, &FunctionListPanel::refresh);
    connect(_list, &QListWidget::itemActivated,
            this, &FunctionListPanel::onItemActivated);
    connect(_filterEdit, &QLineEdit::textChanged,
            this, &FunctionListPanel::onFilterChanged);
}

void FunctionListPanel::updateForView(ScintillaEditView* view)
{
    _currentView = view;
    refresh();
}

void FunctionListPanel::refresh()
{
    _list->clear();
    _allEntries.clear();

    if (!_currentView || _currentView->isLargeFileMode()) return;

    QsciLexer* lex  = _currentView->lexer();
    QString lang     = lex ? QString(lex->language()).toLower() : QString();
    QString text     = _currentView->text();
    _allEntries      = parseText(text, lang);

    QString filter = _filterEdit->text().toLower();
    for (const FuncEntry& e : _allEntries) {
        if (!filter.isEmpty() && !e.display.toLower().contains(filter))
            continue;
        QListWidgetItem* item = new QListWidgetItem(e.display, _list);
        item->setData(Qt::UserRole, e.line);
    }
}

void FunctionListPanel::onItemActivated(QListWidgetItem* item)
{
    if (!item) return;
    int line = item->data(Qt::UserRole).toInt();
    emit navigationRequested(line);
}

void FunctionListPanel::onFilterChanged(const QString& text)
{
    _list->clear();
    QString filter = text.toLower();
    for (const FuncEntry& e : _allEntries) {
        if (!filter.isEmpty() && !e.display.toLower().contains(filter))
            continue;
        QListWidgetItem* item = new QListWidgetItem(e.display, _list);
        item->setData(Qt::UserRole, e.line);
    }
}

bool FunctionListPanel::serialize(const QString& outputFilePath,
                                  const QString& sourceName) const
{
    QJsonObject root;
    root.insert(QStringLiteral("root"), sourceName);
    QJsonArray leaves;
    for (const FuncEntry& entry : _allEntries) {
        QString name = entry.display;
        const int suffix = name.lastIndexOf(QStringLiteral("  (line "));
        if (suffix >= 0)
            name.truncate(suffix);
        leaves.append(name);
    }
    root.insert(QStringLiteral("leaves"), leaves);

    QFile output(outputFilePath);
    if (!output.open(QFile::WriteOnly | QFile::Truncate))
        return false;
    return output.write(QJsonDocument(root).toJson(QJsonDocument::Compact)) >= 0;
}

// ─── 各语言函数解析 ───────────────────────────────────────────────────────────

QList<FunctionListPanel::FuncEntry>
FunctionListPanel::parseText(const QString& text, const QString& lang) const
{
    QList<FuncEntry> result;
    const NppParameters& parameters = NppParameters::getInstance();
    const QList<FunctionListEntry> configured = FunctionListParser::parse(
        text, lang,
        {QDir(parameters.getUserPath()).filePath(QStringLiteral("functionList")),
         QDir(parameters.getNppPath()).filePath(QStringLiteral("functionList"))});
    for (const FunctionListEntry& entry : configured) {
        result.append(
            {QStringLiteral("%1  (line %2)")
                 .arg(entry.name).arg(entry.line + 1),
             entry.line});
    }
    if (!result.isEmpty())
        return result;

    QStringList lines = text.split('\n');

    // 各语言的匹配模式
    QList<QRegExp> patterns;

    if (lang.contains("c++") || lang.contains("c#") || lang.contains("java")) {
        // 函数/方法：返回类型 + 函数名 + 括号（忽略纯声明：以 ; 结尾的行）
        patterns << QRegExp("^(?!\\s*//)(?:[\\w:*&<>]+\\s+)+([\\w~]+)\\s*\\(");
        // 类/结构体
        patterns << QRegExp("^\\s*(?:class|struct|enum)\\s+(\\w+)");
    } else if (lang == "python") {
        patterns << QRegExp("^\\s*def\\s+(\\w+)\\s*\\(");
        patterns << QRegExp("^\\s*class\\s+(\\w+)");
    } else if (lang == "javascript" || lang == "coffeescript") {
        patterns << QRegExp("^\\s*function\\s+(\\w+)\\s*\\(");
        patterns << QRegExp("^\\s*(\\w+)\\s*[=:]\\s*(?:async\\s+)?function");
        patterns << QRegExp("^\\s*(?:class|const|let|var)\\s+(\\w+)");
    } else if (lang == "ruby") {
        patterns << QRegExp("^\\s*def\\s+(\\w+[?!]?)");
        patterns << QRegExp("^\\s*class\\s+(\\w+)");
        patterns << QRegExp("^\\s*module\\s+(\\w+)");
    } else if (lang == "bash") {
        patterns << QRegExp("^\\s*(\\w+)\\s*\\(\\s*\\)");
        patterns << QRegExp("^\\s*function\\s+(\\w+)");
    } else if (lang == "lua") {
        patterns << QRegExp("^\\s*(?:local\\s+)?function\\s+([\\w.:]+)\\s*\\(");
    } else if (lang == "sql") {
        patterns << QRegExp("^\\s*(?:CREATE\\s+(?:OR\\s+REPLACE\\s+)?)?(?:PROCEDURE|FUNCTION|TRIGGER|VIEW)\\s+(\\w+)",
                            Qt::CaseInsensitive);
    } else if (lang == "vhdl") {
        patterns << QRegExp("^\\s*(?:ENTITY|ARCHITECTURE|PACKAGE|PROCEDURE|FUNCTION)\\s+(\\w+)",
                            Qt::CaseInsensitive);
    } else {
        // 通用：形如 `word(` 开头的行
        patterns << QRegExp("^(?!\\s*//)\\s*([\\w]+)\\s*\\(");
    }

    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines[i];
        if (line.trimmed().isEmpty()) continue;

        for (QRegExp& rx : patterns) {
            if (rx.indexIn(line) != -1) {
                QString name = rx.cap(1).trimmed();
                if (!name.isEmpty())
                    result.append({QString("%1  (line %2)").arg(name).arg(i + 1), i});
                break;
            }
        }
    }

    return result;
}
