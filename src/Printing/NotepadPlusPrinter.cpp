#include "Printing/NotepadPlusPrinter.h"

#include "ScintillaComponent/ScintillaEditView.h"

#include <QDateTime>
#include <QFileInfo>
#include <QLocale>
#include <QPainter>
#include <Qsci/qsciscintilla.h>

NotepadPlusPrinter::NotepadPlusPrinter(
    const NppGUI& gui, const QString& filePath)
    : QsciPrinter(QPrinter::HighResolution), _gui(gui), _filePath(filePath)
{
}

void NotepadPlusPrinter::printView(ScintillaEditView* view)
{
    if (!view)
        return;
    const long previousColourMode = view->SendScintilla(
        QsciScintilla::SCI_GETPRINTCOLOURMODE);
    const int previousMarginWidth = view->marginWidth(0);
    view->SendScintilla(
        QsciScintilla::SCI_SETPRINTCOLOURMODE,
        static_cast<unsigned long>(qBound(0, _gui._printOption, 3)));
    if (!_gui._printLineNumber)
        view->setMarginWidth(0, 0);
    printRange(view);
    view->setMarginWidth(0, previousMarginWidth);
    view->SendScintilla(
        QsciScintilla::SCI_SETPRINTCOLOURMODE,
        static_cast<unsigned long>(previousColourMode));
}

void NotepadPlusPrinter::formatPage(
    QPainter& painter, bool drawing, QRect& area, int pageNumber)
{
    const int lineHeight = painter.fontMetrics().height() + 6;
    const QRect original = area;
    area.adjust(0, lineHeight, 0, -lineHeight);
    if (!drawing)
        return;

    const QRect header(
        original.left(), original.top(), original.width(), lineHeight);
    const QRect footer(
        original.left(), original.bottom() - lineHeight + 1,
        original.width(), lineHeight);
    drawTriplet(painter, header, _gui._printHeaderLeft,
                _gui._printHeaderMiddle, _gui._printHeaderRight,
                pageNumber);
    drawTriplet(painter, footer, _gui._printFooterLeft,
                _gui._printFooterMiddle, _gui._printFooterRight,
                pageNumber);
}

QString NotepadPlusPrinter::expand(QString text, int pageNumber) const
{
    const QFileInfo info(_filePath);
    const QDateTime now = QDateTime::currentDateTime();
    text.replace(QStringLiteral("$(FULL_CURRENT_PATH)"), _filePath);
    text.replace(QStringLiteral("$(CURRENT_DIRECTORY)"), info.absolutePath());
    text.replace(QStringLiteral("$(FILE_NAME)"), info.fileName());
    text.replace(QStringLiteral("$(NAME_PART)"), info.completeBaseName());
    text.replace(QStringLiteral("$(EXT_PART)"), info.suffix());
    text.replace(QStringLiteral("$(CURRENT_PRINTING_PAGE)"),
                 QString::number(pageNumber));
    text.replace(QStringLiteral("$(SHORT_DATE)"),
                 QLocale().toString(now.date(), QLocale::ShortFormat));
    text.replace(QStringLiteral("$(LONG_DATE)"),
                 QLocale().toString(now.date(), QLocale::LongFormat));
    text.replace(QStringLiteral("$(TIME)"),
                 QLocale().toString(now.time(), QLocale::ShortFormat));
    return text;
}

void NotepadPlusPrinter::drawTriplet(
    QPainter& painter, const QRect& rect,
    const QString& left, const QString& middle, const QString& right,
    int pageNumber) const
{
    const int third = rect.width() / 3;
    painter.drawText(
        QRect(rect.left(), rect.top(), third, rect.height()),
        Qt::AlignLeft | Qt::AlignVCenter, expand(left, pageNumber));
    painter.drawText(
        QRect(rect.left() + third, rect.top(), third, rect.height()),
        Qt::AlignHCenter | Qt::AlignVCenter, expand(middle, pageNumber));
    painter.drawText(
        QRect(rect.left() + 2 * third, rect.top(),
              rect.width() - 2 * third, rect.height()),
        Qt::AlignRight | Qt::AlignVCenter, expand(right, pageNumber));
}
