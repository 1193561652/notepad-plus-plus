#include "EditorMacro.h"

#include "ScintillaEditView.h"

#include <Scintilla.h>

namespace {

int fromHex(char value)
{
    if (value >= '0' && value <= '9')
        return value - '0';
    if (value >= 'a' && value <= 'f')
        return value - 'a' + 10;
    if (value >= 'A' && value <= 'F')
        return value - 'A' + 10;
    return -1;
}

} // namespace

EditorMacro::EditorMacro(ScintillaEditView* view)
    : QObject(view), _view(view)
{
}

void EditorMacro::startRecording()
{
    if (!_view)
        return;
    _commands.clear();
    _connection = connect(_view, &ScintillaEditBase::macroRecord, this,
        [this](Scintilla::Message message, uptr_t wParam, sptr_t lParam) {
            record(message, wParam, lParam);
        });
    _view->send(SCI_STARTRECORD);
}

void EditorMacro::endRecording()
{
    if (!_view)
        return;
    _view->send(SCI_STOPRECORD);
    disconnect(_connection);
}

void EditorMacro::record(Scintilla::Message message,
                         uptr_t wParam, sptr_t lParam)
{
    Command command;
    command.message = static_cast<unsigned int>(message);
    command.wParam = wParam;
    switch (command.message) {
        case SCI_ADDTEXT:
            command.text = QByteArray(reinterpret_cast<const char*>(lParam),
                                      static_cast<int>(wParam));
            break;
        case SCI_REPLACESEL:
            if (!_commands.isEmpty() &&
                _commands.last().message == SCI_REPLACESEL) {
                _commands.last().text.append(reinterpret_cast<const char*>(lParam));
                return;
            }
            [[fallthrough]];
        case SCI_INSERTTEXT:
        case SCI_APPENDTEXT:
        case SCI_SEARCHNEXT:
        case SCI_SEARCHPREV:
            command.text = QByteArray(reinterpret_cast<const char*>(lParam));
            break;
    }
    _commands.append(command);
}

void EditorMacro::play() const
{
    if (!_view)
        return;
    for (const Command& command : _commands) {
        _view->send(command.message, command.wParam,
            command.text.isEmpty() ? 0
                : reinterpret_cast<sptr_t>(command.text.constData()));
    }
}

QString EditorMacro::save() const
{
    QString result;
    for (const Command& command : _commands) {
        if (!result.isEmpty())
            result += QLatin1Char(' ');
        result += QStringLiteral("%1 %2 %3")
            .arg(command.message).arg(command.wParam).arg(command.text.size());
        if (!command.text.isEmpty()) {
            result += QLatin1Char(' ');
            QByteArray terminated = command.text;
            terminated.append('\0');
            for (const unsigned char value : terminated) {
                if (value == '\\' || value == '"' || value <= ' ' ||
                    value >= 0x7f) {
                    result += QStringLiteral("\\%1").arg(
                        static_cast<unsigned int>(value), 2, 16,
                        QLatin1Char('0'));
                } else {
                    result += QLatin1Char(static_cast<char>(value));
                }
            }
        }
    }
    return result;
}

bool EditorMacro::load(const QString& serialized)
{
    const QStringList fields = serialized.split(QLatin1Char(' '));
    QList<Command> parsed;
    int index = 0;
    while (index < fields.size()) {
        if (index + 3 > fields.size())
            return false;
        bool messageOk = false;
        bool parameterOk = false;
        bool lengthOk = false;
        Command command;
        command.message = fields[index++].toUInt(&messageOk);
        command.wParam = fields[index++].toULongLong(&parameterOk);
        const int length = fields[index++].toInt(&lengthOk);
        if (!messageOk || !parameterOk || !lengthOk || length < 0)
            return false;
        if (length > 0) {
            if (index >= fields.size())
                return false;
            const QByteArray encoded = fields[index++].toLatin1();
            bool pendingNull = false;
            for (int offset = 0; offset < encoded.size(); ++offset) {
                unsigned char value = static_cast<unsigned char>(encoded[offset]);
                if (value == '"' || value <= ' ' || value >= 0x7f)
                    return false;
                if (value == '\\') {
                    if (offset + 2 >= encoded.size())
                        return false;
                    const int high = fromHex(encoded[++offset]);
                    const int low = fromHex(encoded[++offset]);
                    if (high < 0 || low < 0)
                        return false;
                    value = static_cast<unsigned char>((high << 4) | low);
                }
                if (value == 0) {
                    pendingNull = true;
                } else {
                    if (pendingNull) {
                        command.text.append('\0');
                        pendingNull = false;
                    }
                    command.text.append(static_cast<char>(value));
                }
            }
        }
        parsed.append(command);
    }
    _commands = parsed;
    return true;
}
