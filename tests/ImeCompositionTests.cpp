#include <QApplication>
#include <QFocusEvent>
#include <QInputMethod>
#include <QInputMethodEvent>
#include <ScintillaEditBase.h>

#include <iostream>

namespace {
class Editor : public ScintillaEditBase {
public:
    using ScintillaEditBase::inputMethodQuery;
};

bool expect(bool value, const char *message)
{
    if (!value)
        std::cerr << "FAIL: " << message << '\n';
    return value;
}

QByteArray text(Editor &editor)
{
    QByteArray bytes(static_cast<int>(editor.send(SCI_GETLENGTH)) + 1, '\0');
    editor.send(SCI_GETTEXT, bytes.size(), reinterpret_cast<sptr_t>(bytes.data()));
    bytes.chop(1);
    return bytes;
}

void savedText(Editor &editor, const QByteArray &bytes)
{
    editor.sends(SCI_SETTEXT, 0, bytes.constData());
    editor.send(SCI_EMPTYUNDOBUFFER);
    editor.send(SCI_SETSAVEPOINT);
    editor.send(SCI_GOTOPOS, bytes.size());
}

void preedit(Editor &editor, const QString &value)
{
    const QList<QInputMethodEvent::Attribute> attributes = value.isEmpty()
        ? QList<QInputMethodEvent::Attribute>()
        : QList<QInputMethodEvent::Attribute>{
              {QInputMethodEvent::Cursor, value.size(), 1, QVariant()}};
    QInputMethodEvent event(value, attributes);
    QApplication::sendEvent(&editor, &event);
}

void commit(Editor &editor, const QString &value)
{
    QInputMethodEvent event;
    event.setCommitString(value);
    QApplication::sendEvent(&editor, &event);
}
} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    Editor editor;
    editor.send(SCI_SETCODEPAGE, SC_CP_UTF8);
    editor.resize(500, 200);
    editor.show();
    QApplication::setActiveWindow(&editor);
    editor.setFocus();
    app.processEvents();
    bool ok = expect(QGuiApplication::focusObject() == &editor,
                     "The test editor must own the input context");

    // Replay the real sequence up to cancellation. Offscreen has no IBus cache;
    // the desktop reproduction still validates that platform-specific part.
    savedText(editor, "saved text");
    for (const auto &value : {QString::fromUtf8("年"), QString::fromUtf8("你"),
                             QString::fromUtf8("你好"), QString::fromUtf8("你"),
                             QString::fromUtf8("年"), QString()})
        preedit(editor, value);
    ok &= expect(text(editor) == "saved text" && !editor.send(SCI_GETMODIFY),
                 "Cancelling preedit must restore the saved text and save point");
    QGuiApplication::inputMethod()->commit();
    QFocusEvent out(QEvent::FocusOut, Qt::ActiveWindowFocusReason);
    QApplication::sendEvent(&editor, &out);
    QFocusEvent in(QEvent::FocusIn, Qt::ActiveWindowFocusReason);
    QApplication::sendEvent(&editor, &in);
    ok &= expect(text(editor) == "saved text" && !editor.send(SCI_GETMODIFY),
                 "Focus changes after cancellation must not modify the document");
    editor.send(SCI_GOTOPOS, 0);
    const QRect cursorRect = editor.inputMethodQuery(Qt::ImCursorRectangle).toRect();
    ok &= expect(cursorRect.x() == editor.send(SCI_POINTXFROMPOSITION, 0, 0),
                 "Ended composition must not leave a stale candidate position");

    // Do not mark an already dirty document clean or remove earlier undo items.
    savedText(editor, "saved:");
    commit(editor, "X");
    preedit(editor, QString::fromUtf8("你"));
    preedit(editor, QString());
    ok &= expect(text(editor) == "saved:X" && editor.send(SCI_GETMODIFY),
                 "Cancellation must preserve earlier unsaved edits");
    editor.send(SCI_UNDO);
    ok &= expect(text(editor) == "saved:" && !editor.send(SCI_GETMODIFY),
                 "Earlier committed text must remain undoable to the save point");

    // Normal selection, then cancellation followed by a fresh direct commit.
    savedText(editor, "");
    preedit(editor, QString::fromUtf8("你好"));
    commit(editor, QString::fromUtf8("你好"));
    preedit(editor, QString()); // Engines may hide preedit after committing.
    ok &= expect(text(editor) == QString::fromUtf8("你好").toUtf8()
                     && editor.send(SCI_GETMODIFY),
                 "Normal candidate confirmation must survive a trailing hide");
    preedit(editor, QString::fromUtf8("年"));
    preedit(editor, QString());
    commit(editor, QString::fromUtf8("😀"));
    ok &= expect(text(editor) == QString::fromUtf8("你好😀").toUtf8(),
                 "A valid later commit must not be discarded after cancellation");
    preedit(editor, QString::fromUtf8("你"));
    commit(editor, QString::fromUtf8("你"));
    ok &= expect(text(editor) == QString::fromUtf8("你好😀你").toUtf8(),
                 "A new composition must work after cancellation");

    return ok ? 0 : 1;
}
