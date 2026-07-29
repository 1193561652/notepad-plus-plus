#pragma once

#include "Parameters.h"

#include <Qsci/qsciprinter.h>

class ScintillaEditView;

class NotepadPlusPrinter : public QsciPrinter
{
public:
    NotepadPlusPrinter(const NppGUI& gui, const QString& filePath);

    void printView(ScintillaEditView* view);
    void formatPage(QPainter& painter, bool drawing, QRect& area,
                    int pageNumber) override;

private:
    QString expand(QString text, int pageNumber) const;
    void drawTriplet(QPainter& painter, const QRect& rect,
                     const QString& left, const QString& middle,
                     const QString& right, int pageNumber) const;

    NppGUI _gui;
    QString _filePath;
};
