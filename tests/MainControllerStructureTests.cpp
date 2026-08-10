#include <QFile>
#include <QHash>
#include <QString>

#include <iostream>

namespace {

bool expect(bool condition, const QString& message)
{
    if (!condition)
        std::cerr << "FAILED: " << message.toStdString() << '\n';
    return condition;
}

QString readSource(const QString& name)
{
    QFile file(QStringLiteral(NPP_SOURCE_DIR) + QLatin1Char('/') + name);
    if (!file.open(QFile::ReadOnly))
        return QString();
    return QString::fromUtf8(file.readAll());
}

} // namespace

int main()
{
    const QHash<QString, QString> sources = {
        {QStringLiteral("MainWindow.cpp"), readSource(QStringLiteral("MainWindow.cpp"))},
        {QStringLiteral("Notepad_plus.cpp"), readSource(QStringLiteral("Notepad_plus.cpp"))},
        {QStringLiteral("NppCommands.cpp"), readSource(QStringLiteral("NppCommands.cpp"))},
        {QStringLiteral("NppIO.cpp"), readSource(QStringLiteral("NppIO.cpp"))},
        {QStringLiteral("NppNotification.cpp"), readSource(QStringLiteral("NppNotification.cpp"))},
    };

    bool ok = true;
    for (auto it = sources.cbegin(); it != sources.cend(); ++it)
        ok &= expect(!it.value().isEmpty(), it.key() + QStringLiteral(" must be readable"));

    const QHash<QString, QStringList> ownership = {
        {QStringLiteral("MainWindow.cpp"), {
            QStringLiteral("MainWindow::setupTabViews("),
            QStringLiteral("MainWindow::setupPluginSystem("),
            QStringLiteral("MainWindow::closeEvent(")}},
        {QStringLiteral("Notepad_plus.cpp"), {
            QStringLiteral("MainWindow::MainWindow("),
            QStringLiteral("MainWindow::~MainWindow("),
            QStringLiteral("MainWindow::applyCommandLineInvocation(")}},
        {QStringLiteral("NppCommands.cpp"), {
            QStringLiteral("MainWindow::createActions("),
            QStringLiteral("MainWindow::createMenus("),
            QStringLiteral("MainWindow::executeNppCommand("),
            QStringLiteral("MainWindow::onFindInFilesRequested(")}},
        {QStringLiteral("NppIO.cpp"), {
            QStringLiteral("MainWindow::doOpenFile("),
            QStringLiteral("MainWindow::doSave("),
            QStringLiteral("MainWindow::saveSession("),
            QStringLiteral("MainWindow::restoreSession(")}},
        {QStringLiteral("NppNotification.cpp"), {
            QStringLiteral("MainWindow::onTextChanged("),
            QStringLiteral("MainWindow::onCurrentTabChanged("),
            QStringLiteral("MainWindow::onCursorPositionChanged("),
            QStringLiteral("MainWindow::notifyCurrentLanguageChanged(")}},
    };

    for (auto owner = ownership.cbegin(); owner != ownership.cend(); ++owner) {
        for (const QString& definition : owner.value()) {
            int total = 0;
            for (auto source = sources.cbegin(); source != sources.cend(); ++source)
                total += source.value().count(definition);
            ok &= expect(total == 1, definition + QStringLiteral(" must be defined exactly once"));
            ok &= expect(sources.value(owner.key()).contains(definition),
                         definition + QStringLiteral(" must be owned by ") + owner.key());
        }
    }

    return ok ? 0 : 1;
}
