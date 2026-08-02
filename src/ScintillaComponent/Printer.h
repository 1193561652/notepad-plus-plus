#pragma once

#include "Parameters.h"

#include <QPrinter>

class ScintillaEditView;

class Printer : public QPrinter
{
public:
    Printer(const NppGUI& gui, const QString& filePath);

    void printView(ScintillaEditView* view);
    void formatPage(QPainter& painter, bool drawing, QRect& area,
                    int pageNumber);

private:
    QString expand(QString text, int pageNumber) const;
    void drawTriplet(QPainter& painter, const QRect& rect,
                     const QString& left, const QString& middle,
                     const QString& right, int pageNumber) const;

    NppGUI _gui;
    QString _filePath;
};
