#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileSystemWatcher>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextStream>
#include <QThread>
#include <QTimer>

#include "MainWindow.h"
#include "MISC/FileManager.h"
#include "MISC/UiFont.h"
#include "Parameters.h"
#include "Printing/NotepadPlusPrinter.h"
#include "ScintillaComponent/FindReplaceDlg.h"
#include "ScintillaComponent/ScintillaEditView.h"

namespace {

bool saveWidget(QWidget* widget, const QString& path)
{
    if (!widget)
        return false;
    widget->show();
    QApplication::processEvents();
    return widget->grab().save(path);
}

QAction* action(MainWindow& window, const char* name)
{
    return window.findChild<QAction*>(QString::fromLatin1(name));
}

bool trigger(MainWindow& window, const char* name)
{
    QAction* target = action(window, name);
    if (!target || !target->isEnabled())
        return false;
    target->trigger();
    QApplication::processEvents();
    return true;
}

bool clickMessageButton(QMessageBox* box, QMessageBox::StandardButton button)
{
    if (!box || !box->button(button))
        return false;
    box->button(button)->click();
    return true;
}

QString normalizedEols(QString text)
{
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    return text;
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Notepad++"));
    app.setApplicationVersion(QStringLiteral("8.4.6"));
    app.setOrganizationName(QStringLiteral("Notepad++"));
    app.setFont(notepadPlusPlusUiFont());

    if (argc != 2)
        return 2;
    const QString output = QDir::cleanPath(QString::fromLocal8Bit(argv[1]));
    if (!QDir().mkpath(output))
        return 3;
    QTemporaryDir temporary;
    if (!temporary.isValid())
        return 4;
    QFile::remove(output + QStringLiteral("/progress.log"));
    const auto progress = [&](const char* step) {
        QFile log(output + QStringLiteral("/progress.log"));
        if (log.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            log.write(step);
            log.write("\n");
        }
    };
    progress("start");

    const QString settingsPath =
        temporary.filePath(QStringLiteral("settings"));
    if (!QDir().mkpath(settingsPath) ||
        !NppParameters::getInstance().setUserPathOverride(settingsPath))
        return 5;
    NppParameters::getInstance().load();
    NppParameters::getInstance().reloadNativeLang();
    MainWindow window;
    window.resize(1100, 760);
    window.show();
    QApplication::processEvents();

    ScintillaEditView* view = window.currentView();
    if (!view) {
        QAction* newAction = action(window, "newAction");
        if (newAction)
            newAction->trigger();
        QApplication::processEvents();
        view = window.currentView();
    }
    FindReplaceDlg* findDialog = window.findChild<FindReplaceDlg*>();
    if (!findDialog) {
        QAction* findAction = action(window, "findAction");
        if (findAction)
            findAction->trigger();
        QApplication::processEvents();
        findDialog = window.findChild<FindReplaceDlg*>();
    }
    if (!view || !findDialog)
        return 10;
    findDialog->setCurrentView(view);

    view->setText(QStringLiteral("alpha one\nbeta alpha\n"));
    QComboBox* findCombo =
        findDialog->findChild<QComboBox*>(QStringLiteral("findCombo"));
    if (!findCombo)
        return 11;
    findCombo->setEditText(QStringLiteral("alpha"));
    if (!QMetaObject::invokeMethod(
            findDialog, "onFindAllInCurrentDoc", Qt::DirectConnection))
        return 12;
    QApplication::processEvents();
    QDockWidget* resultDock =
        window.findChild<QDockWidget*>(QStringLiteral("FindResultDock"));
    ScintillaEditView* resultView = resultDock
        ? resultDock->findChild<ScintillaEditView*>(
              QStringLiteral("findResultView")) : nullptr;
    if (!resultDock || !resultDock->isVisible() || !resultView
        || resultView->lines() < 3
        || !resultView->text().contains(QStringLiteral("Line 1:"))
        || !resultView->text().contains(QStringLiteral("Line 2:"))
        || !saveWidget(&window, output + QStringLiteral("/finder-results.png")))
        return 13;

    view->setText(QStringLiteral("alpha beta"));
    view->SendScintilla(
        QsciScintilla::SCI_SETINDICATORCURRENT,
        ScintillaEditView::FIND_MARK_INDICATOR);
    view->SendScintilla(QsciScintilla::SCI_INDICATORFILLRANGE, 0, 5);
    view->SendScintilla(QsciScintilla::SCI_INDICATORFILLRANGE, 6, 4);
    if (!QMetaObject::invokeMethod(
            findDialog, "onCopyMarkedText", Qt::DirectConnection)
        || QApplication::clipboard()->text()
            != QStringLiteral("alpha\r\nbeta\r\n"))
        return 14;

    view->setText(QStringLiteral("zero needle one"));
    if (!findDialog->executeSavedMacroAction(1700, 0, QString())
        || !findDialog->executeSavedMacroAction(
            1601, 0, QStringLiteral("needle"))
        || !findDialog->executeSavedMacroAction(1625, 0, QString())
        || !findDialog->executeSavedMacroAction(1702, 256, QString())
        || !findDialog->executeSavedMacroAction(1701, 1723, QString())
        || view->selectedText() != QStringLiteral("needle"))
        return 15;

    view->setText(QStringLiteral("10\n2\n1\n"));
    view->setSelection(0, 0, 0, 0);
    if (!trigger(window, "sortIntegerAscendingAction")
        || normalizedEols(view->text()) != QStringLiteral("1\n2\n10\n")) {
        QFile actual(output + QStringLiteral("/integer-sort-actual.txt"));
        if (actual.open(QIODevice::WriteOnly))
            actual.write(view->text().toUtf8());
        qCritical("Integer sort actual: %s",
                  qPrintable(view->text().replace('\r', QStringLiteral("\\r"))
                                      .replace('\n', QStringLiteral("\\n"))));
        return 16;
    }

    view->setText(QStringLiteral("1,5\n0,4\n10,0\n"));
    view->setSelection(0, 0, 0, 0);
    if (!trigger(window, "sortDecimalCommaAscendingAction")
        || normalizedEols(view->text())
            != QStringLiteral("0,4\n1,5\n10,0\n")) {
        qCritical("Decimal comma sort actual: %s",
                  qPrintable(view->text().replace('\r', QStringLiteral("\\r"))
                                      .replace('\n', QStringLiteral("\\n"))));
        return 17;
    }

    view->setText(QStringLiteral("x020z\nx003z\nx100z\n"));
    const int anchor = view->positionFromLineIndex(0, 1);
    const int caret = view->positionFromLineIndex(2, 4);
    view->SendScintilla(QsciScintilla::SCI_SETSELECTIONMODE,
        QsciScintilla::SC_SEL_RECTANGLE);
    view->SendScintilla(QsciScintilla::SCI_SETANCHOR, anchor);
    view->SendScintilla(QsciScintilla::SCI_SETCURRENTPOS, caret);
    if (!trigger(window, "sortIntegerAscendingAction")
        || normalizedEols(view->text())
            != QStringLiteral("x003z\nx020z\nx100z\n")) {
        qCritical("Rectangle sort actual: %s",
                  qPrintable(view->text().replace('\r', QStringLiteral("\\r"))
                                      .replace('\n', QStringLiteral("\\n"))));
        return 18;
    }
    view->SendScintilla(QsciScintilla::SCI_SETSELECTIONMODE,
                        QsciScintilla::SC_SEL_STREAM);

    view->setText(QStringLiteral("a\na\na\n"));
    view->setCursorPosition(0, 1);
    bool columnCaptured = false;
    QTimer::singleShot(0, [&]() {
        QDialog* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!dialog)
            return;
        QComboBox* mode =
            dialog->findChild<QComboBox*>(QStringLiteral("columnModeCombo"));
        QComboBox* format =
            dialog->findChild<QComboBox*>(QStringLiteral("columnFormatCombo"));
        QSpinBox* initial =
            dialog->findChild<QSpinBox*>(QStringLiteral("columnInitialValue"));
        QSpinBox* increment =
            dialog->findChild<QSpinBox*>(QStringLiteral("columnIncrementValue"));
        QSpinBox* repeat =
            dialog->findChild<QSpinBox*>(QStringLiteral("columnRepeatValue"));
        QCheckBox* leading =
            dialog->findChild<QCheckBox*>(QStringLiteral("columnLeadingZeros"));
        if (!mode || !format || !initial || !increment || !repeat || !leading)
            return;
        mode->setCurrentIndex(1);
        format->setCurrentIndex(1);
        initial->setValue(14);
        increment->setValue(2);
        repeat->setValue(2);
        leading->setChecked(true);
        columnCaptured = saveWidget(
            dialog, output + QStringLiteral("/column-editor.png"));
        dialog->accept();
    });
    if (!trigger(window, "columnEditorAction") || !columnCaptured
        || normalizedEols(view->text())
            != QStringLiteral("a0E\na0E\na10\n"))
        return 19;

    QAction* udlAction = action(window, "userDefinedLanguageDialogAction");
    bool udlCaptured = false;
    if (udlAction && udlAction->isEnabled()) {
        QTimer::singleShot(0, [&]() {
            QDialog* dialog =
                qobject_cast<QDialog*>(QApplication::activeModalWidget());
            QTabWidget* tabs = dialog ? dialog->findChild<QTabWidget*>() : nullptr;
            udlCaptured = dialog && tabs && tabs->count() == 3
                && saveWidget(dialog, output + QStringLiteral("/udl-designer.png"));
            if (dialog)
                dialog->reject();
        });
        udlAction->trigger();
    }
    if (!udlCaptured)
        return 20;

    const QVector<UserLangDesc>& userLanguages =
        NppParameters::getInstance().getUserLangs();
    if (userLanguages.isEmpty()
        || userLanguages.first().sourceFilePath.isEmpty())
        return 31;
    const QString isolatedUdl =
        temporary.filePath(QStringLiteral("userDefineLang.xml"));
    QFile::remove(isolatedUdl);
    if (!QFile::copy(userLanguages.first().sourceFilePath, isolatedUdl))
        return 32;
    UserLangDesc editedLanguage = userLanguages.first();
    editedLanguage.sourceFilePath = isolatedUdl;
    editedLanguage.exts.append(QStringLiteral("p1check"));
    if (!NppParameters::getInstance().writeUserDefinedLanguage(editedLanguage))
        return 33;
    QFile isolatedUdlFile(isolatedUdl);
    if (!isolatedUdlFile.open(QIODevice::ReadOnly)
        || !isolatedUdlFile.readAll().contains("p1check"))
        return 34;

    view->setText(QStringLiteral("P1 print body\nsecond line\n"));
    NppGUI printGui = NppParameters::getInstance().getNppGUI();
    printGui._printHeaderLeft = QStringLiteral("HEADER_LEFT");
    printGui._printHeaderMiddle = QStringLiteral("$(FILE_NAME)");
    printGui._printHeaderRight =
        QStringLiteral("PAGE $(CURRENT_PRINTING_PAGE)");
    printGui._printFooterLeft = QStringLiteral("FOOTER_LEFT");
    printGui._printFooterMiddle = QStringLiteral("$(NAME_PART)");
    printGui._printFooterRight = QStringLiteral("FOOTER_RIGHT");
    printGui._printLineNumber = true;
    const QString pdfPath = output + QStringLiteral("/print-output.pdf");
    NotepadPlusPrinter printer(
        printGui, output + QStringLiteral("/sample.txt"));
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(pdfPath);
    progress("print-start");
    printer.printView(view);
    progress("print-finished");
    if (!QFileInfo::exists(pdfPath) || QFileInfo(pdfPath).size() < 1000)
        return 21;

    const QString restorePath =
        temporary.filePath(QStringLiteral("restore-closed.txt"));
    QFile restoreFile(restorePath);
    if (!restoreFile.open(QIODevice::WriteOnly) ||
        restoreFile.write("restore me") != 10)
        return 22;
    restoreFile.close();
    window.openFile(restorePath);
    QApplication::processEvents();
    if (QFileInfo(window.currentFilePath()).absoluteFilePath() !=
            QFileInfo(restorePath).absoluteFilePath() ||
        !trigger(window, "closeAction") ||
        !trigger(window, "restoreLastClosedFileAction") ||
        QFileInfo(window.currentFilePath()).absoluteFilePath() !=
            QFileInfo(restorePath).absoluteFilePath())
        return 22;

    QString watchedPath = temporary.filePath(QStringLiteral("watched.txt"));
    QFile watched(watchedPath);
    if (!watched.open(QIODevice::WriteOnly)
        || watched.write("original") != 8)
        return 23;
    watched.close();
    window.openFile(watchedPath);
    QApplication::processEvents();
    QFileSystemWatcher* watcher = window.findChild<QFileSystemWatcher*>();
    if (!watcher)
        return 24;
    watcher->blockSignals(true);
    progress("external-opened");

    QThread::msleep(20);
    if (!watched.open(QIODevice::WriteOnly | QIODevice::Truncate)
        || watched.write("modified") != 8)
        return 24;
    watched.close();
    bool modifiedCaptured = false;
    QTimer modifiedTimer;
    QObject::connect(&modifiedTimer, &QTimer::timeout, [&]() {
        QMessageBox* box =
            qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        if (!box)
            return;
        modifiedCaptured = box
            && saveWidget(box, output + QStringLiteral("/external-modified.png"))
            && clickMessageButton(box, QMessageBox::No);
    });
    modifiedTimer.start(20);
    progress("external-modified-start");
    QMetaObject::invokeMethod(
        &window, "onWatchedFileChanged", Qt::DirectConnection,
        Q_ARG(QString, watchedPath));
    modifiedTimer.stop();
    progress("external-modified-finished");
    if (!modifiedCaptured)
        return 25;

    const QString renamedPath =
        temporary.filePath(QStringLiteral("renamed.txt"));
    if (!QFile::rename(watchedPath, renamedPath))
        return 26;
    bool renamedCaptured = false;
    QTimer renamedTimer;
    QObject::connect(&renamedTimer, &QTimer::timeout, [&]() {
        QMessageBox* box =
            qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        if (!box)
            return;
        renamedCaptured = box
            && saveWidget(box, output + QStringLiteral("/external-renamed.png"))
            && clickMessageButton(box, QMessageBox::Yes);
    });
    renamedTimer.start(20);
    progress("external-renamed-start");
    QMetaObject::invokeMethod(
        &window, "onWatchedDirectoryChanged", Qt::DirectConnection,
        Q_ARG(QString, temporary.path()));
    renamedTimer.stop();
    progress("external-renamed-finished");
    if (!renamedCaptured
        || QDir::cleanPath(window.currentFilePath())
            != QDir::cleanPath(renamedPath))
        return 27;

    Buffer* watchedBuffer = MainFileManager.findBufferByPath(renamedPath);
    if (!watchedBuffer
        || !QFile::setPermissions(
            renamedPath,
            QFileDevice::ReadOwner | QFileDevice::ReadGroup
                | QFileDevice::ReadOther))
        return 35;
    QMetaObject::invokeMethod(
        &window, "onWatchedFileChanged", Qt::DirectConnection,
        Q_ARG(QString, renamedPath));
    if (!watchedBuffer->isReadOnly())
        return 36;
    if (!QFile::setPermissions(
            renamedPath,
            QFileDevice::ReadOwner | QFileDevice::WriteOwner
                | QFileDevice::ReadGroup | QFileDevice::ReadOther))
        return 37;
    QMetaObject::invokeMethod(
        &window, "onWatchedFileChanged", Qt::DirectConnection,
        Q_ARG(QString, renamedPath));
    if (watchedBuffer->isReadOnly())
        return 38;

    if (!QFile::remove(renamedPath))
        return 28;
    bool deletedCaptured = false;
    QTimer deletedTimer;
    QObject::connect(&deletedTimer, &QTimer::timeout, [&]() {
        QMessageBox* box =
            qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        if (!box)
            return;
        deletedCaptured =
            saveWidget(box, output + QStringLiteral("/external-deleted.png"));
        for (QAbstractButton* button : box->buttons()) {
            if (button->text().contains(QStringLiteral("Keep Open"),
                                        Qt::CaseInsensitive)) {
                button->click();
                return;
            }
        }
    });
    deletedTimer.start(20);
    progress("external-deleted-start");
    QMetaObject::invokeMethod(
        &window, "onWatchedFileChanged", Qt::DirectConnection,
        Q_ARG(QString, renamedPath));
    deletedTimer.stop();
    progress("external-deleted-finished");
    if (!deletedCaptured)
        return 29;

    QFile metadata(output + QStringLiteral("/metadata.txt"));
    if (!metadata.open(QIODevice::WriteOnly | QIODevice::Text))
        return 30;
    QTextStream stream(&metadata);
    stream << "finder.results=2\n"
           << "copyMarkedText=pass\n"
           << "savedSearchMacro=pass\n"
           << "sort.integer=pass\n"
           << "sort.decimalComma=pass\n"
           << "sort.rectangle=pass\n"
           << "columnEditor=pass\n"
           << "udlDesigner=pass\n"
           << "udlRoundTrip=pass\n"
           << "printPdf=pass\n"
           << "external.modified=pass\n"
           << "external.renamed=pass\n"
           << "external.permission=pass\n"
           << "external.deleted=pass\n";
    return 0;
}
