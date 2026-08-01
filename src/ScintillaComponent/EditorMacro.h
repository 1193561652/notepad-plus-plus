#pragma once

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>

#include <Scintilla.h>
#include <ScintillaMessages.h>

class ScintillaEditView;

class EditorMacro : public QObject
{
    Q_OBJECT
public:
    explicit EditorMacro(ScintillaEditView* view);

    void startRecording();
    void endRecording();
    void play() const;
    QString save() const;
    bool load(const QString& serialized);

private:
    struct Command {
        unsigned int message = 0;
        uptr_t wParam = 0;
        QByteArray text;
    };

    void record(Scintilla::Message message, uptr_t wParam, sptr_t lParam);

    ScintillaEditView* _view = nullptr;
    QList<Command> _commands;
    QMetaObject::Connection _connection;
};
