#include <PluginInterface.h>
#include <Scintilla.h>
#include <SciLexer.h>
#include <QApplication>
#include <QLibrary>
#include <QTimer>
#include <QMessageBox>
#include <QClipboard>
#include <QMimeData>
#include <QTemporaryDir>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QAbstractButton>
#include <QFile>
#include <QDebug>
#include <cstring>
#include <stdexcept>
#include <algorithm>
static void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct Editor {
    QByteArray text;
    intptr_t start = 0, end = 0, targetStart = 0, targetEnd = 0;
    int eol = SC_EOL_LF, selections = 1, lexer = SCLEX_NULL, style = 0;
    int view = 1, modifications = 0;
    int tabIndents = 0, useTabs = 1, backspace = 0, indent = 8;
    uint64_t buffer = 42;
    bool rectangle = false;
    QByteArray path = "test.cpp";
    static int32_t NPP_PLUGIN_CALL language(void*) { return 3; }
    static uint64_t NPP_PLUGIN_CALL bufferId(void* c) { return static_cast<Editor*>(c)->buffer; }
    static size_t NPP_PLUGIN_CALL filePath(void* c, char* out, size_t capacity) {
        auto& p = static_cast<Editor*>(c)->path;
        if (out && capacity) { auto n = std::min<size_t>(p.size(), capacity-1); std::memcpy(out,p.constData(),n); out[n]=0; }
        return p.size();
    }
    intptr_t lineStart(intptr_t line) const {
        intptr_t p = 0;
        while (line-- > 0) { int next = text.indexOf('\n', p); if (next < 0) return text.size(); p = next + 1; }
        return p;
    }
    void set(QByteArray value) { text = value; start = 0; end = text.size(); modifications = 0; }
    static int32_t NPP_PLUGIN_CALL currentView(void* context) { return static_cast<Editor*>(context)->view; }
    static intptr_t NPP_PLUGIN_CALL send(void* context, int32_t view, uint32_t msg, uintptr_t w, intptr_t l) {
        auto& e = *static_cast<Editor*>(context);
        check(view == e.view, "wrong editor view");
        auto position = static_cast<intptr_t>(w);
        switch (msg) {
        case SCI_SETTARGETRANGE: e.targetStart=position; e.targetEnd=l; return 0;
        case SCI_GETTEXT: {
            auto n=std::min<size_t>(e.text.size(),w?w-1:0);
            if (l && w) { std::memcpy(reinterpret_cast<void*>(l),e.text.constData(),n); reinterpret_cast<char*>(l)[n]=0; }
            return n;
        }
        case SCI_CLEARSELECTIONS: e.start=e.end; e.selections=1; return 0;
        case SCI_AUTOCSETMULTI: return 0;
        case SCI_BEGINUNDOACTION: case SCI_ENDUNDOACTION: return 0;
        case SCI_REPLACESEL: {
            QByteArray value(reinterpret_cast<const char*>(l));
            e.text.replace(e.start,e.end-e.start,value); e.end=e.start+value.size(); e.start=e.end;
            ++e.modifications; return 0;
        }
        case SCI_GETLINECOUNT: return e.text.count('\n') + 1;
        case SCI_LINEFROMPOSITION: return e.text.left(position).count('\n');
        case SCI_POSITIONFROMLINE: return e.lineStart(position);
        case SCI_LINELENGTH: return e.lineStart(position+1)-e.lineStart(position);
        case SCI_GETLINEINDENTATION: {
            int width = 0;
            for (auto p = e.lineStart(position); p < e.text.size(); ++p) {
                if (e.text.at(p) == ' ') ++width;
                else if (e.text.at(p) == '\t') width += 4 - width % 4;
                else break;
            }
            return width;
        }
        case SCI_GETTEXTRANGE: {
            auto r = reinterpret_cast<Sci_TextRange*>(l);
            auto bytes = e.text.mid(r->chrg.cpMin, r->chrg.cpMax-r->chrg.cpMin);
            std::memcpy(r->lpstrText, bytes.constData(), bytes.size()+1); return bytes.size();
        }
        case SCI_GETTEXTRANGEFULL: {
            auto r = reinterpret_cast<Sci_TextRangeFull*>(l);
            auto bytes = e.text.mid(r->chrg.cpMin, r->chrg.cpMax-r->chrg.cpMin);
            std::memcpy(r->lpstrText, bytes.constData(), bytes.size()+1); return bytes.size();
        }
        case SCI_GETLINESELSTARTPOSITION: return e.rectangle ? e.lineStart(position) : std::max(e.start, e.lineStart(position));
        case SCI_GETLINESELENDPOSITION: return e.rectangle ? e.lineStart(position)+1 : std::min(e.end, e.lineStart(position+1));
        case SCI_SELECTIONISRECTANGLE: return e.rectangle;
        case SCI_GETCODEPAGE: return SC_CP_UTF8;
        case SCI_GETTABINDENTS: return e.tabIndents;
        case SCI_GETUSETABS: return e.useTabs;
        case SCI_GETTABWIDTH: return 4;
        case SCI_CALLTIPCANCEL: return 0;
        case SCI_GETBACKSPACEUNINDENTS: return e.backspace;
        case SCI_GETINDENT: return e.indent;
        case SCI_SETTABINDENTS: e.tabIndents = w; return 0;
        case SCI_SETUSETABS: e.useTabs = w; return 0;
        case SCI_SETBACKSPACEUNINDENTS: e.backspace = w; return 0;
        case SCI_SETINDENT: e.indent = w; return 0;
        case SCI_GETCURRENTPOS: return e.end;
        case SCI_GETLENGTH: return e.text.size();
        case SCI_GETTEXTLENGTH: return e.text.size();
        case SCI_GETSELECTIONSTART: return e.start;
        case SCI_GETSELECTIONEND: return e.end;
        case SCI_GETSELECTIONS: return e.selections;
        case SCI_GETEOLMODE: return e.eol;
        case SCI_TARGETFROMSELECTION: e.targetStart = e.start; e.targetEnd = e.end; return 0;
        case SCI_GETTARGETTEXT: {
            auto bytes = e.text.mid(e.targetStart, e.targetEnd - e.targetStart);
            if (l) std::memcpy(reinterpret_cast<void*>(l), bytes.constData(), bytes.size() + 1);
            return bytes.size();
        }
        case SCI_REPLACETARGET:
            e.text.replace(e.targetStart, e.targetEnd - e.targetStart, reinterpret_cast<const char*>(l), w);
            e.targetEnd = e.targetStart + w; ++e.modifications; return w;
        case SCI_SETSEL:
            e.end = std::max<intptr_t>(0, l); e.start = position < 0 ? e.end : position; return 0;
        case SCI_SETSELECTIONSTART: e.start = position; return 0;
        case SCI_SETSELECTIONEND: e.end = position; return 0;
        case SCI_SELECTALL: e.start = 0; e.end = e.text.size(); return 0;
        case SCI_GETLEXER: return e.lexer;
        case SCI_GETSTYLEAT: return position >= 0 && position < e.text.size() ? e.style : 0;
        case SCI_WORDSTARTPOSITION: return 0;
        case SCI_WORDENDPOSITION: return e.text.size();
        default: throw std::runtime_error("unexpected Scintilla message");
        }
    }
};
int main(int argc, char** argv) {
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc, argv);
    try {
        check(argc == 3, "expected library and plugin id");
        QLibrary library(QString::fromLocal8Bit(argv[1]));
        // Match PluginManager: QSettings/Qt retain module-owned literals/metatypes.
        library.setLoadHints(QLibrary::PreventUnloadHint);
        check(library.load(), qPrintable(library.errorString()));
        auto abi = reinterpret_cast<NppGetPluginAbiVersionFn>(library.resolve("nppGetPluginAbiVersion"));
        auto init = reinterpret_cast<NppSetInfoFn>(library.resolve("nppSetInfo"));
        auto funcs = reinterpret_cast<NppGetFuncsArrayFn>(library.resolve("nppGetFuncsArray"));
        auto notify = reinterpret_cast<NppBeNotifiedFn>(library.resolve("nppBeNotified"));
        check(abi && init && funcs && notify && library.resolve("nppGetName") && library.resolve("nppMessageProc"), "missing ABI exports");
        check(abi() == NPP_PLUGIN_ABI_VERSION && !init(nullptr), "ABI validation");
        Editor editor;
        QTemporaryDir config;
        check(config.isValid(), "temporary config directory");
        auto configBytes = config.path().toUtf8();
        NppPluginHostInfo host{};
        host.struct_size = sizeof(host); host.abi_version = NPP_PLUGIN_ABI_VERSION;
        host.host_context = &editor; host.send_scintilla = Editor::send; host.get_current_view = Editor::currentView;
        host.get_current_file_path = Editor::filePath; host.get_current_buffer_id = Editor::bufferId;
        host.get_current_language = Editor::language; host.plugin_config_path_utf8 = configBytes.constData();
        host.struct_size = offsetof(NppPluginHostInfo, send_scintilla);
        check(!init(&host), "truncated host accepted");
        host.struct_size = sizeof(host);
        check(init(&host), "initialization failed");
        uint32_t count = 0;
        auto commands = funcs(&count);
        auto run = [&](int index) { check(index < int(count) && commands[index].command, "missing command"); commands[index].command(commands[index].user_data); };
        auto event = [&](uint32_t code) { NppPluginNotification n{}; n.struct_size=sizeof(n); n.code=code; n.buffer_id=editor.buffer; n.source_view=editor.view; notify(&n); };
        auto state = reinterpret_cast<NppGetCommandStateFn>(library.resolve("nppGetCommandState"));
        check(state, "command state export");
        const QByteArray id(argv[2]);
        if (id == "qkNppReverseLines") {
            check(count == 3, "reverse command count");
            editor.set("one\ntwo\nthree"); run(0); check(editor.text == "three\ntwo\none", "LF reverse");
            editor.set("one\r\ntwo\r\n"); editor.eol = SC_EOL_CRLF; run(0); check(editor.text == "\r\ntwo\r\none", "CRLF trailing blank");
            editor.set("\na\r\nb"); run(0); check(editor.text == "b\r\na\n", "partial mixed EOL");
            editor.set(QByteArray("a\0b\nc", 5)); editor.eol = SC_EOL_LF; run(0); check(editor.text == QByteArray("c\na\0b", 5), "binary reverse");
            editor.set(""); run(0); check(editor.modifications == 0, "empty reverse changed document");
            editor.set("abc\ndef"); editor.start = editor.end = 2; run(1);
            check(editor.text == "def\nabc" && editor.start == 5 && editor.end == 5, "document caret mapping");
        } else if (id == "SelectQuotedText") {
            check(count == 3 && commands[0].shortcut.is_alt && commands[0].shortcut.key == 0xde, "quoted menu/shortcut");
            editor.set("\"hello\""); editor.lexer = SCLEX_CPP; editor.style = SCE_C_STRING;
            editor.start = editor.end = 3; run(0); check(editor.start == 1 && editor.end == 6, "quoted selection");
            editor.set("hello"); editor.lexer = SCLEX_NULL; editor.style = 0;
            editor.start = editor.end = 2; run(0); check(editor.start == 0 && editor.end == 5, "word fallback");
        } else if (id == "GotoLineCol") {
            check(count==4 && !commands[2].command && commands[0].shortcut.key==0x76 && commands[0].shortcut.is_ctrl,"GotoLineCol menu and Ctrl-F7");
            QFile ini(config.path()+"/GotoLineCol.ini");
            check(ini.open(QIODevice::ReadOnly) && ini.readAll().contains("EdgeBuffer=10"),"GotoLineCol original configuration defaults");
        } else if (id == "JSONViewer") {
            check(count==7 && !commands[4].command,"JSON original menu");
            check(commands[0].shortcut.key=='J' && commands[3].shortcut.key=='K',"JSON original shortcuts");
            editor.set("{ \"n\" : 9007199254740993 }"); run(2);
            check(editor.text=="{\"n\":9007199254740993}","JSON compression preserves number text");
        } else if (id == "EditorConfig") {
            check(count==6 && !commands[2].command && !commands[4].command,"EditorConfig original menu");
            editor.set("unchanged"); run(0);
            check(editor.text=="unchanged","EditorConfig relative unsaved path is ignored");
        } else if (id == "nppConverter") {
            check(count==7 && !commands[2].command && !commands[4].command,"converter original menu");
            editor.set(QByteArray("A\0z",3)); run(0); check(editor.text=="41007A","converter binary bytes");
            run(1); check(editor.text==QByteArray("A\0z",3),"converter binary decode and selection");
            editor.set(QByteArray(17,'x')); run(0); check(editor.text==QByteArray("78").repeated(16)+"\n78","converter default line break interval");
        } else if (id == "BetterMultiSelection") {
            check(count==3 && !commands[1].command,"BMS command slots");
            event(NPP_PLUGIN_NOTIFICATION_READY);
            check(state(0)&NPP_PLUGIN_COMMAND_CHECKED,"BMS enabled by default at READY");
            run(0); check(!(state(0)&NPP_PLUGIN_COMMAND_CHECKED),"BMS disabled state");
        } else if (id == "urlPlugin") {
            check(count==5 && !commands[3].command,"URL menu slots");
            check(commands[1].shortcut.key=='E' && commands[1].shortcut.is_ctrl && commands[1].shortcut.is_alt && commands[1].shortcut.is_shift,"URL original shortcut");
            run(0); check(state(0)&NPP_PLUGIN_COMMAND_CHECKED,"URL settings menu shown state");
            run(0); check(!(state(0)&NPP_PLUGIN_COMMAND_CHECKED),"URL settings toggle close");
        } else if (id == "Merge_files_in_one") {
            check(count==4 && !commands[1].command && !commands[2].command,"merge original menu slots");
            editor.set("a\nb"); run(0);
            bool found=false; for(auto w:QApplication::topLevelWidgets()) if(w->objectName()=="mergeFilesDialog") found=true;
            check(found,"merge modeless dialog created");
        } else if (id == "ElasticTabstops") {
            check(count==6,"elastic command count");
            check(state(0)==(NPP_PLUGIN_COMMAND_CHECKABLE|NPP_PLUGIN_COMMAND_CHECKED),"elastic initial enabled state");
            check(!commands[1].command && !commands[3].command,"elastic separators");
            run(4);
            QFile ini(config.path()+"/ElasticTabstops.ini");
            check(ini.open(QIODevice::ReadOnly) && ini.readAll().contains("extensions *"),"elastic original settings file");
        } else if (id == "BracketsCheck") {
            check(count==7,"brackets command count");
            QString dialogText;
            QTimer timer;
            QObject::connect(&timer,&QTimer::timeout,[&]() {
                for (auto widget:QApplication::topLevelWidgets()) if (auto box=qobject_cast<QMessageBox*>(widget)) {
                    dialogText=box->text(); if(auto button=box->button(QMessageBox::Ok)) button->click();
                }
            }); timer.start(1);
            editor.set("({[]})"); run(0); check(dialogText.contains("balanced"),"balanced brackets");
            editor.set("\n\t(]"); run(0); check(dialogText.contains("row 2 and character 5"),"mismatched opener position");
            editor.set("<"); run(6); run(0); check(dialogText.contains("All brackets"),"disabled angle brackets");
            editor.set(QString::fromUtf8("中文\nxx(]").toUtf8()); editor.start=9; editor.end=11; run(1);
            check(dialogText.contains("row 2 and character 3"),"UTF-8 byte selection offset");
        } else if (id == "Remove_dup_lines") {
            check(count==4,"duplicate menu");
            editor.set("a\na\n\n\nb\na\n"); run(0); check(editor.text=="a\r\n\r\n\r\nb","stable dedup keeps empty lines");
            editor.set("a\r\nb"); run(0); check(editor.modifications==0 && editor.start==editor.end,"unchanged clears selection");
        } else if (id == "SecurePad") {
            check(count==5,"SecurePad menu");
            QTimer timer;
            QObject::connect(&timer,&QTimer::timeout,[&]() {
                for(auto w:QApplication::topLevelWidgets()) if(w->objectName()=="securePadKeyDialog") {
                    w->findChild<QLineEdit*>("cryptKey")->setText("test-key");
                    w->findChild<QLineEdit*>("confirmCryptKey")->setText("test-key");
                    QMetaObject::invokeMethod(w->findChild<QDialogButtonBox*>("keyButtons"),"accepted",Qt::DirectConnection);
                }
            }); timer.start(1);
            editor.set("hello"); run(0); check(editor.text.size()==16 && editor.text!="hello","SecurePad encrypt");
            run(1); check(editor.text=="hello","SecurePad decrypt");
            editor.set("xhelloy"); editor.start=1; editor.end=6; run(2);
            check(editor.text.startsWith('x') && editor.text.endsWith('y') && editor.text.size()==18,"SecurePad selection boundary");
            editor.start=1; editor.end=17; run(3); check(editor.text=="xhelloy","SecurePad selected decrypt");
        } else if (id == "SurroundSelection") {
            check(count==3 && state(0)==(NPP_PLUGIN_COMMAND_CHECKABLE|NPP_PLUGIN_COMMAND_CHECKED),"surround default state");
            event(NPP_PLUGIN_NOTIFICATION_READY); run(0); check(state(0)==NPP_PLUGIN_COMMAND_CHECKABLE,"surround disable");
        } else if (id == "nppAutoDetectIndent") {
            check(count == 4 && state(0) == NPP_PLUGIN_COMMAND_CHECKABLE, "indent menu state");
            editor.set("root\n    a\n        b\n    c\n");
            event(NPP_PLUGIN_NOTIFICATION_READY); event(NPP_PLUGIN_NOTIFICATION_BUFFER_ACTIVATED);
            check(editor.useTabs == 0 && editor.indent == 4 && editor.tabIndents && editor.backspace, "space indent detection");
            run(0); check(editor.useTabs == 1 && editor.indent == 8 && !editor.tabIndents && !editor.backspace, "disable restores original settings");
            check(state(0) == (NPP_PLUGIN_COMMAND_CHECKABLE|NPP_PLUGIN_COMMAND_CHECKED), "disable checkbox");
            editor.set("root\n\ta\n\tb\n"); run(0);
            check(editor.useTabs == 1 && editor.indent == 8, "tab detection leaves indent width unchanged");
            event(NPP_PLUGIN_NOTIFICATION_FILE_BEFORE_CLOSE);
            editor.set("root\n  a\n    b\n  c\n"); event(NPP_PLUGIN_NOTIFICATION_BUFFER_ACTIVATED);
            check(editor.useTabs == 0 && editor.indent == 2, "close invalidates cache");
            event(NPP_PLUGIN_NOTIFICATION_FILE_BEFORE_SAVE); editor.path = "renamed.cpp"; event(NPP_PLUGIN_NOTIFICATION_FILE_SAVED);
            editor.set("root\n        a\n"); event(NPP_PLUGIN_NOTIFICATION_BUFFER_ACTIVATED);
            check(editor.indent == 2, "save-as transfers path cache");
        } else if (id == "SelectToClipboard") {
            check(count == 2 && state(0) == NPP_PLUGIN_COMMAND_CHECKABLE, "copy default setting");
            QApplication::clipboard()->setText("sentinel");
            editor.set("abc\ndef"); editor.start=editor.end=1; event(NPP_PLUGIN_NOTIFICATION_UPDATE_UI);
            run(0); event(NPP_PLUGIN_NOTIFICATION_UPDATE_UI);
            editor.start=0; editor.end=3; event(NPP_PLUGIN_NOTIFICATION_UPDATE_UI);
            check(QApplication::clipboard()->text() == "abc", "changed selection copied");
            QApplication::clipboard()->setText("manual"); event(NPP_PLUGIN_NOTIFICATION_UPDATE_UI);
            check(QApplication::clipboard()->text() == "manual", "unchanged selection preserves clipboard");
            ++editor.buffer; editor.end=7; event(NPP_PLUGIN_NOTIFICATION_UPDATE_UI);
            check(QApplication::clipboard()->text() == "manual", "tab switch suppresses copy");
            editor.rectangle=true; editor.end=5; event(NPP_PLUGIN_NOTIFICATION_UPDATE_UI);
            check(QApplication::clipboard()->text() == "a\nd" && QApplication::clipboard()->mimeData()->hasFormat("application/x-scintilla-rectangular"), "rectangular selection");
            event(NPP_PLUGIN_NOTIFICATION_SHUTDOWN); check(init(&host), "copy reload"); commands=funcs(&count);
            check(state(0) & NPP_PLUGIN_COMMAND_CHECKED, "copy config persisted");
        } else if (id == "mimeTools") {
            check(count == 22, "MIME command count");
            editor.set("f"); run(0); check(editor.text == "Zg", "base64 no padding");
            editor.set("f"); run(1); check(editor.text == "Zg==", "base64 padding");
            editor.set("Zg=="); run(5); check(editor.text == "f", "strict base64 decode");
            editor.set(QByteArray("a\0b", 3)); run(1); check(editor.text == "YQBi", "binary encode");
            editor.set("YQBi"); run(4); check(editor.text == QByteArray("a\0b", 3), "binary decode");
            editor.set("a b"); run(11); check(editor.text == "a%20b", "RFC URL encode");
            editor.set("a%20b"); run(17); check(editor.text == "a b", "URL decode");
            editor.set("a=b"); run(8); check(editor.text == "a=3Db", "QP encode");
            run(9); check(editor.text == "a=b", "QP decode");
            editor.set("<saml>hello</saml>"); editor.text = editor.text.toBase64(); editor.end = editor.text.size();
            run(19); check(editor.text == "<saml>hello</saml>", "SAML plain XML");
            editor.set("a\nb"); editor.selections = 2; run(0); check(editor.modifications == 0, "multi-selection guard");
        }
        NppPluginNotification shutdown{}; shutdown.struct_size = sizeof(shutdown); shutdown.code = NPP_PLUGIN_NOTIFICATION_SHUTDOWN;
        notify(&shutdown); editor.set("unchanged"); run(0); check(editor.modifications == 0, "command after shutdown");
        check(init(&host), "reload failed"); notify(&shutdown);
        check(library.unload(), "unload failed");
        qInfo() << id << "ABI and behavior checks passed";
        return 0;
    } catch (const std::exception& error) { qCritical() << error.what(); return 1; }
}
