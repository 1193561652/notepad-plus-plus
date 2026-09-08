#include <PluginInterface.h>
#include <ScintillaEditBase.h>
#include <QApplication>
#include <QLibrary>
#include <QTemporaryDir>
#include <QKeyEvent>
#include <QLineEdit>
#include <QClipboard>
#include <QMimeData>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QDialogButtonBox>
#include <QAbstractButton>
#include <QPushButton>
#include <QCheckBox>
#include <QMainWindow>
#include <QToolBar>
#include <QAction>
#include <QFile>
#include <QDir>
#include <QTreeWidget>
#include <QSpinBox>
#include <QLabel>
#include "../../editorconfig-notepad-plus-plus/src/menuCmdID.hpp"
#include <memory>
#include <cstring>
#include <stdexcept>
static void require(bool value,const char* why) { if (!value) throw std::runtime_error(why); }
struct Host {
    ScintillaEditBase editor;
    QByteArray pathBytes="test.cpp",created;
    std::vector<int> menuCommands;
    int selectedLanguage = -1;
    static int NPP_PLUGIN_CALL setLanguage(void* c, int32_t value) { static_cast<Host*>(c)->selectedLanguage=value; return 1; }
    static int NPP_PLUGIN_CALL menu(void* c,int32_t command) {
        auto& h=*static_cast<Host*>(c); h.menuCommands.push_back(command);
        if(command==IDM_EDIT_TRIMTRAILING) {
            for(int line=int(h.editor.send(SCI_GETLINECOUNT))-1;line>=0;--line) {
                auto end=h.editor.send(SCI_GETLINEENDPOSITION,line), start=end;
                while(start>h.editor.send(SCI_POSITIONFROMLINE,line)) {
                    auto ch=h.editor.send(SCI_GETCHARAT,start-1);
                    if(ch!=' ' && ch!='\t') break;
                    --start;
                }
                h.editor.send(SCI_DELETERANGE,start,end-start);
            }
        } else if(command==IDM_FORMAT_TOUNIX) h.editor.send(SCI_CONVERTEOLS,SC_EOL_LF);
        return 1;
    }
    static int NPP_PLUGIN_CALL create(void* c,const uint8_t* bytes,size_t length) {
        static_cast<Host*>(c)->created=QByteArray(reinterpret_cast<const char*>(bytes),int(length)); return 1;
    }
    static intptr_t NPP_PLUGIN_CALL send(void* c,int32_t view,uint32_t msg,uintptr_t w,intptr_t l) {
        require(view==1,"incorrect active view"); return static_cast<Host*>(c)->editor.send(msg,w,l);
    }
    static int32_t NPP_PLUGIN_CALL view(void*) { return 1; }
    static int32_t NPP_PLUGIN_CALL language(void*) { return 3; }
    static uint64_t NPP_PLUGIN_CALL buffer(void*) { return 42; }
    static size_t NPP_PLUGIN_CALL path(void* c,char* out,size_t capacity) {
        auto& text=static_cast<Host*>(c)->pathBytes;
        if (out && capacity>size_t(text.size())) std::memcpy(out,text.constData(),text.size()+1);
        return text.size();
    }
    void set(const QByteArray& text) {
        editor.sends(SCI_SETTEXT,0,text.constData()); editor.send(SCI_EMPTYUNDOBUFFER);
        editor.send(SCI_SETSEL,0,text.size());
    }
    QByteArray text() {
        auto length=editor.send(SCI_GETLENGTH); QByteArray result(length+1,'\0');
        editor.send(SCI_GETTEXT,result.size(),reinterpret_cast<intptr_t>(result.data())); result.resize(length); return result;
    }
};
int main(int argc,char** argv) {
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc,argv);
    try {
        require(argc>=3,"expected plugin file and id");
        Host h; h.editor.resize(500,300); h.editor.show(); h.editor.setFocus();
        h.editor.send(SCI_SETCODEPAGE,SC_CP_UTF8); h.editor.send(SCI_SETEOLMODE,SC_EOL_LF);
        QCoreApplication::processEvents();
        QTemporaryDir temp; require(temp.isValid(),"config directory"); auto config=temp.path().toUtf8();
        NppPluginHostInfo info{}; info.struct_size=sizeof(info); info.abi_version=NPP_PLUGIN_ABI_VERSION;
        info.host_context=&h; info.send_scintilla=Host::send; info.get_current_view=Host::view;
        info.get_current_file_path=Host::path; info.get_current_buffer_id=Host::buffer;
        info.get_current_language=Host::language; info.plugin_config_path_utf8=config.constData();
        info.create_document=Host::create;
        info.execute_menu_command=Host::menu; info.set_current_language=Host::setLanguage;
        QLibrary library(QString::fromLocal8Bit(argv[1])); library.setLoadHints(QLibrary::PreventUnloadHint);
        require(library.load(),qPrintable(library.errorString()));
        auto init=reinterpret_cast<NppSetInfoFn>(library.resolve("nppSetInfo"));
        auto funcs=reinterpret_cast<NppGetFuncsArrayFn>(library.resolve("nppGetFuncsArray"));
        auto notify=reinterpret_cast<NppBeNotifiedFn>(library.resolve("nppBeNotified"));
        require(init && funcs && notify && init(&info),"initialize plugin");
        uint32_t count; auto commands=funcs(&count);
        auto run=[&](int index) { require(index<int(count) && commands[index].command,"command missing"); commands[index].command(commands[index].user_data); };
        auto event=[&](uint32_t code) { NppPluginNotification n{}; n.struct_size=sizeof(n); n.code=code; n.buffer_id=42; n.source_view=1; notify(&n); };
        QByteArray id(argv[2]);
        std::unique_ptr<QMainWindow> toolbarHost;
        if(id=="urlPlugin") { toolbarHost=std::make_unique<QMainWindow>(); toolbarHost->addToolBar("Test"); }
        event(NPP_PLUGIN_NOTIFICATION_READY);
        if (id=="GotoLineCol") {
            if(argc>3) {
                h.set("first\nabcdef"); h.editor.send(SCI_GOTOPOS,6);
                event(NPP_PLUGIN_NOTIFICATION_BUFFER_ACTIVATED);
                require(h.editor.send(SCI_GETCURRENTPOS)==8,"GotoLineCol command line moves to requested column on matching active line");
                h.editor.send(SCI_GOTOPOS,6); event(NPP_PLUGIN_NOTIFICATION_BUFFER_ACTIVATED);
                require(h.editor.send(SCI_GETCURRENTPOS)==6,"GotoLineCol command line consumed once by default");
                event(NPP_PLUGIN_NOTIFICATION_SHUTDOWN); return 0;
            }
            h.set(QByteArray("a\t")+QByteArray::fromHex("c3a9")+"z\nlast"); h.editor.send(SCI_SETSEL,0,0); h.editor.send(SCI_SETTABWIDTH,4);
            run(0); QWidget* panel=nullptr;
            for(auto w:QApplication::topLevelWidgets()) if(w->objectName()=="gotoLineColPanel") panel=w;
            require(panel,"GotoLineCol panel");
            auto ln=panel->findChild<QSpinBox*>("gotoLine"),col=panel->findChild<QSpinBox*>("gotoColumn");
            auto bytes=panel->findChild<QCheckBox*>("useByteCol"); auto go=panel->findChild<QPushButton*>("gotoGo");
            ln->setValue(1); col->setValue(4); go->click();
            require(h.editor.send(SCI_GETCURRENTPOS)==3,"GotoLineCol byte column may point inside UTF-8 character");
            auto infoText=panel->findChild<QLabel*>("cursorInfo")->text();
            require(infoText.contains("U+E9") && infoText.contains("LATIN SMALL LETTER E ACUTE"),qPrintable("GotoLineCol original character info: "+infoText));
            bytes->setChecked(false); ln->setValue(1); col->setValue(6); go->click();
            require(h.editor.send(SCI_GETCURRENTPOS)==4,"GotoLineCol character column expands tabs and counts UTF-8 once");
            ln->setValue(2); col->setValue(999); go->click();
            require(h.editor.send(SCI_GETCURRENTPOS)==10,"GotoLineCol clamps to line end");
            require(h.editor.send(SCI_GETCARETSTYLE)==CARETSTYLE_BLOCK,"GotoLineCol caret flash starts");
            event(NPP_PLUGIN_NOTIFICATION_SHUTDOWN);
            require(h.editor.send(SCI_GETCARETSTYLE)!=CARETSTYLE_BLOCK,"GotoLineCol shutdown restores flashing caret");
            require(init(&info),"GotoLineCol reload"); commands=funcs(&count);
            run(0); panel=nullptr; for(auto w:QApplication::topLevelWidgets()) if(w->objectName()=="gotoLineColPanel") panel=w;
            require(panel && !panel->findChild<QCheckBox*>("useByteCol")->isChecked(),"GotoLineCol persists byte/character mode");
            QTimer preferenceTimer;
            QObject::connect(&preferenceTimer,&QTimer::timeout,[&] {
                for(auto w:QApplication::topLevelWidgets()) if(w->objectName()=="gotoPreferences") {
                    w->findChild<QCheckBox*>("ShowCallTip")->setChecked(false);
                    w->findChild<QSpinBox*>("EdgeBuffer")->setValue(17);
                    qobject_cast<QDialog*>(w)->accept();
                }
            });
            preferenceTimer.start(20); run(1); preferenceTimer.stop();
            QFile saved(temp.path()+"/GotoLineCol.ini"); require(saved.open(QIODevice::ReadOnly),"GotoLineCol saved preferences"); auto settingsText=saved.readAll();
            require(settingsText.contains("ShowCallTip=0") && settingsText.contains("EdgeBuffer=17") && settingsText.contains("UseByteCol=0"),"GotoLineCol preference dialog preserves independent byte toggle");
        } else if (id=="JSONViewer") {
            h.editor.send(SCI_SETUSETABS,0); h.editor.send(SCI_SETTABWIDTH,2);
            h.set("{ /*comment*/ \"z\":123456789012345678901234567890, \"a\":[1.234567890123456789,], }");
            run(2); require(h.text()=="{\"z\":123456789012345678901234567890,\"a\":[1.234567890123456789]}","JSON original fork preserves raw numbers and accepts configured comments/comma");
            run(3); require(h.text()=="{\n  \"a\": [\n    1.234567890123456789\n  ],\n  \"z\": 123456789012345678901234567890\n}","JSON recursive sorting with original formatter");
            require(h.selectedLanguage==57,"JSON original highlight language");
            h.editor.send(SCI_UNDO); require(h.text().startsWith("{\"z\":"),"JSON sort undo");
            h.set("{\"items\":[{\"name\":\"value\"},false]}"); run(0);
            QWidget* panel=nullptr; for(auto w:QApplication::topLevelWidgets()) if(w->objectName()=="jsonViewerPanel") panel=w;
            require(panel,"JSON tree panel"); auto tree=panel->findChild<QTreeWidget*>("jsonTree");
            auto root=tree->topLevelItem(0); require(root && root->text(0)=="JSON","JSON root");
            auto items=root->child(0); require(items && items->text(0)=="items [2]" && items->child(0)->text(0)=="[0] {1}","JSON original SAX array counts");
            auto leaf=items->child(0)->child(0); tree->setCurrentItem(leaf);
            require(panel->findChild<QLineEdit*>("jsonPath")->text()=="JSON.items[0].name","JSON original path construction");
            tree->itemActivated(leaf,0);
            require(h.editor.send(SCI_GETSELECTIONSTART)==12 && h.editor.send(SCI_GETSELECTIONEND)==16,"JSON original byte position navigation");
            run(0); // Close before malformed input so only the command reports its error.
            h.set("{\"bad\":}"); bool sawError=false;
            QTimer dismiss; QObject::connect(&dismiss,&QTimer::timeout,[&] { for(auto w:QApplication::topLevelWidgets()) if(auto m=qobject_cast<QMessageBox*>(w)) { sawError=true; m->accept(); } }); dismiss.start(20);
            run(1); dismiss.stop(); require(sawError && h.text()=="{\"bad\":}","JSON errors preserve input");
            QTimer settingsTimer;
            QObject::connect(&settingsTimer,&QTimer::timeout,[&] {
                for(auto w:QApplication::topLevelWidgets()) if(w->objectName()=="jsonSettings") {
                    w->findChild<QCheckBox*>("Replace undefined")->setChecked(true);
                    qobject_cast<QDialog*>(w)->accept();
                }
            });
            settingsTimer.start(20); run(5); settingsTimer.stop();
            h.set("{\"z\":undefined,\"a\":1}"); run(3);
            require(h.text()=="{\"z\":null,\"a\":1}","JSON original undefined sort fallback retains normalized input order");
            h.set("{\"v\":undefined}"); run(0);
            require(h.text()=="{\"v\":null}" && tree->topLevelItem(0)->childCount()==1,"JSON undefined tree retry has valid tracking and no duplicate partial nodes");
        } else if (id=="EditorConfig") {
            auto write=[&](const QString& path,const QByteArray& bytes) {
                QFile f(path); require(f.open(QIODevice::WriteOnly),"write editorconfig fixture"); require(f.write(bytes)==bytes.size(),"write all settings");
            };
            write(temp.path()+"/.editorconfig","root = true\n[*]\nindent_style = space\nindent_size = 3\ntab_width = 5\nend_of_line = lf\ntrim_trailing_whitespace = true\ninsert_final_newline = true\ncharset = utf-8\n");
            QDir(temp.path()).mkdir(QString::fromUtf8("子目录"));
            write(temp.path()+QString::fromUtf8("/子目录/.editorconfig"),"[*.{cpp,h}]\nindent_style = tab\nindent_size = tab\ntab_width = 4\n");
            h.pathBytes=(temp.path()+QString::fromUtf8("/子目录/example.cpp")).toUtf8();
            event(NPP_PLUGIN_NOTIFICATION_BUFFER_ACTIVATED);
            require(h.editor.send(SCI_GETUSETABS)==1 && h.editor.send(SCI_GETTABWIDTH)==4 && h.editor.send(SCI_GETINDENT)==4,"EditorConfig parent inheritance and brace glob override");
            h.set("    first\n    second\n        third"); run(3);
            require(h.text()=="    first\n\tsecond\n\t\tthird","EditorConfig original fix skips first line");
            h.editor.send(SCI_UNDO); require(h.text()=="    first\n    second\n        third","EditorConfig fix single undo");
            h.set("value  \r\nnext\t"); h.editor.send(SCI_SETAUTOMATICFOLD,7);
            event(NPP_PLUGIN_NOTIFICATION_FILE_BEFORE_SAVE);
            require(h.text()=="value\nnext\n","EditorConfig trim then append then convert EOL");
            require(h.menuCommands==std::vector<int>{IDM_EDIT_TRIMTRAILING,IDM_FORMAT_TOUNIX,IDM_FORMAT_CONV2_AS_UTF_8},"EditorConfig original save command order");
            require(h.editor.send(SCI_GETAUTOMATICFOLD)==7 && h.editor.send(SCI_GETSELECTIONEMPTY),"EditorConfig restores folds and collapses selection");
            write(temp.path()+QString::fromUtf8("/子目录/.editorconfig"),"[*]\ninsert_final_newline = false\n");
            h.set("value\r\n\n\r"); event(NPP_PLUGIN_NOTIFICATION_FILE_BEFORE_SAVE);
            require(h.text()=="value","EditorConfig false removes all final newlines");
            h.pathBytes=(temp.path()+"/.editorconfig").toUtf8(); event(NPP_PLUGIN_NOTIFICATION_BUFFER_ACTIVATED);
            require(h.selectedLanguage==13,"EditorConfig filename selects original INI language");
        } else if (id=="nppConverter") {
            h.set("41 00 42"); run(1); require(h.text()==QByteArray("A\0B",3),"converter HEX state machine preserves NUL");
            run(0); require(h.text()=="410042","converter selected decoded bytes re-encode");
            h.editor.send(SCI_UNDO); require(h.text()==QByteArray("A\0B",3),"converter single undo");
            run(3); QWidget* panel=nullptr;
            for(auto w:QApplication::topLevelWidgets()) if(w->objectName()=="converterPanel") panel=w;
            require(panel,"converter floating panel created");
            auto decimal=panel->findChild<QLineEdit*>("DECEdit"); decimal->setText("255");
            require(panel->findChild<QLineEdit*>("HEXEdit")->text()=="FF" && panel->findChild<QLineEdit*>("BINEdit")->text()=="11111111" && panel->findChild<QLineEdit*>("OCTEdit")->text()=="377","converter number bases");
            panel->findChild<QLineEdit*>("HEXEdit")->setText("4G"); require(decimal->text()=="4","converter trims invalid final digit");
            decimal->setText("0"); h.set("replace"); panel->findChild<QPushButton*>("ASCIIInsert")->click();
            require(h.text()==QByteArray(1,'\0'),"converter inserts zero byte");
            panel->findChild<QPushButton*>("ASCIICopy")->click();
            require(QApplication::clipboard()->mimeData()->data("application/octet-stream")==QByteArray(1,'\0'),"converter binary clipboard payload");
            decimal->setText("4294967296"); require(panel->findChild<QLineEdit*>("HEXEdit")->text()=="FFFFFFFF","converter original 32-bit unsigned parse saturation");
        } else if (id=="BetterMultiSelection") {
            auto key=[&](int code,Qt::KeyboardModifiers modifiers=Qt::NoModifier) {
                QKeyEvent press(QEvent::KeyPress,code,modifiers); QApplication::sendEvent(&h.editor,&press);
            };
            auto two=[&](int a,int b,int c,int d) {
                h.editor.send(SCI_SETMULTIPLESELECTION,1); h.editor.send(SCI_SETSELECTION,a,b); h.editor.send(SCI_ADDSELECTION,c,d);
            };
            h.set("ab\ncd"); two(1,1,4,4); key(Qt::Key_Left);
            require(h.editor.send(SCI_GETSELECTIONS)==2 && h.editor.send(SCI_GETSELECTIONNCARET,0)==0 && h.editor.send(SCI_GETSELECTIONNCARET,1)==3,"BMS moves every caret");
            key(Qt::Key_Right,Qt::ShiftModifier);
            require(h.editor.send(SCI_GETSELECTIONNEND,0)==1 && h.editor.send(SCI_GETSELECTIONNEND,1)==4,"BMS extends every selection");
            QKeyEvent overrideCopy(QEvent::ShortcutOverride,Qt::Key_C,Qt::ControlModifier);
            overrideCopy.ignore(); QApplication::sendEvent(&h.editor,&overrideCopy);
            require(overrideCopy.isAccepted(),"BMS takes priority over host QAction shortcut");
            key(Qt::Key_C,Qt::ControlModifier);
            require(QApplication::clipboard()->text()=="a\nc\n" && QApplication::clipboard()->mimeData()->hasFormat("application/x-bms-multiselect"),"BMS clipboard row order and marker");
            h.set("x\ny"); two(1,0,3,2); key(Qt::Key_V,Qt::ControlModifier);
            require(h.text()=="a\nc","BMS distributes clipboard rows");
            h.editor.send(SCI_UNDO); require(h.text()=="x\ny","BMS paste single undo");
            h.set("ab\ncd"); two(1,1,4,4); key(Qt::Key_Return);
            require(h.text()=="a\nb\nc\nd" && h.editor.send(SCI_GETSELECTIONNCARET,1)==6,"BMS newline updates subsequent offsets");
            h.editor.send(SCI_UNDO); require(h.text()=="ab\ncd","BMS multi edit single undo");
            h.set("a\nb"); two(1,1,3,3); key(Qt::Key_Escape);
            require(h.editor.send(SCI_GETSELECTIONS)==1,"BMS escape collapses to main caret");
            QLineEdit other; other.setText("abc"); other.setCursorPosition(1);
            QKeyEvent left(QEvent::KeyPress,Qt::Key_Left,Qt::NoModifier); QApplication::sendEvent(&other,&left);
            require(other.cursorPosition()==0 && h.text()=="a\nb","BMS ignores other inputs");
        } else if (id=="urlPlugin") {
            h.set("a b&x=+"); run(1); require(h.created=="a%20b&x=+" && h.text()=="a b&x=+","URL default new document");
            run(0); QWidget* settings=nullptr;
            for(auto w:QApplication::topLevelWidgets()) if(w->objectName()=="urlSettingsDialog") settings=w;
            require(settings,"URL settings created");
            settings->findChild<QCheckBox*>("sameFile")->setChecked(true);
            settings->findChild<QCheckBox*>("breakLines")->setChecked(true);
            settings->findChild<QLineEdit*>("delimiter")->setText("&");
            QMetaObject::invokeMethod(settings->findChild<QDialogButtonBox*>("settingsButtons"),"accepted",Qt::DirectConnection);
            h.set("a=1&b=x%20y"); run(2); require(h.text()=="a=1\r\n&b=x y","URL decode break before delimiter");
            h.editor.send(SCI_UNDO); require(h.text()=="a=1&b=x%20y","URL replace is undoable");
            h.set("a b"); run(1); require(h.text()=="a%20b","URL saved same-file setting");
            auto actions=toolbarHost->findChild<QToolBar*>()->actions();
            QAction* decodeAction=nullptr; for(auto a:actions) if(a->objectName()=="urlDecodeAction") decodeAction=a;
            require(decodeAction && !decodeAction->icon().isNull(),"URL original toolbar icon");
            h.editor.send(SCI_SELECTALL); decodeAction->trigger(); require(h.text()=="a b","URL toolbar decode command");
        } else if (id=="Merge_files_in_one") {
            h.set("a\nb"); run(0);
            QWidget* dialog=nullptr;
            for(auto w:QApplication::topLevelWidgets()) if(w->objectName()=="mergeFilesDialog") dialog=w;
            require(dialog,"merge dialog");
            h.pathBytes="second.txt"; h.set("x\ny");
            QEvent activate(QEvent::WindowActivate); QApplication::sendEvent(dialog,&activate);
            auto button=dialog->findChild<QPushButton*>("button1"); require(button && button->isVisible(),"second file activates merge");
            button->click();
            QByteArray marker=" 3f5456cfsd661lld33dGuid9CA0F324 ";
            require(h.created=="a"+marker+"x\r\nb"+marker+"y" && h.text()=="x\ny","merge creates output without editing sources");
            dialog->findChild<QCheckBox*>("checkBox1")->setChecked(true); button->click();
            require(h.created=="x"+marker+"a\r\ny"+marker+"b","merge reverse checkbox preserves original order");
        } else if (id=="ElasticTabstops") {
            h.editor.sends(SCI_STYLESETFONT,STYLE_DEFAULT,"Courier New");
            h.editor.send(SCI_STYLECLEARALL); h.editor.send(SCI_SETTABWIDTH,4);
            h.set("a\tb\nlonger\tc\n\nx\tz"); event(NPP_PLUGIN_NOTIFICATION_BUFFER_ACTIVATED);
            auto stop=h.editor.send(SCI_GETNEXTTABSTOP,0,0);
            require(stop>0 && stop==h.editor.send(SCI_GETNEXTTABSTOP,1,0),"elastic adjacent rows align");
            require(h.editor.send(SCI_GETNEXTTABSTOP,3,0)<stop,"elastic blank row splits blocks");
            h.editor.send(SCI_SETZOOM,3); event(NPP_PLUGIN_NOTIFICATION_ZOOM);
            require(h.editor.send(SCI_GETNEXTTABSTOP,0,0)>stop,"elastic zoom recomputes pixel widths");
            h.editor.send(SCI_SETZOOM,0); event(NPP_PLUGIN_NOTIFICATION_ZOOM);
            h.editor.send(SCI_SETTARGETRANGE,0,1); h.editor.sends(SCI_REPLACETARGET,10,"muchlonger");
            NppPluginNotification modified{}; modified.struct_size=sizeof(modified);
            modified.code=NPP_PLUGIN_NOTIFICATION_TEXT_MODIFIED; modified.source_view=1;
            modified.position=0; modified.length=10; modified.modification_type=SC_MOD_INSERTTEXT;
            modified.text_utf8="muchlonger"; notify(&modified); event(NPP_PLUGIN_NOTIFICATION_UPDATE_UI);
            require(h.editor.send(SCI_GETNEXTTABSTOP,1,0)>stop,"elastic incremental cell edit resizes neighbors");
            run(0); require(h.editor.send(SCI_GETNEXTTABSTOP,0,0)==0,"disabled clears custom stops");
            run(0); require(h.editor.send(SCI_GETNEXTTABSTOP,0,0)>0,"enabled rebuilds stops");
            h.set("a\tb\nlonger\tc"); event(NPP_PLUGIN_NOTIFICATION_BUFFER_ACTIVATED); run(2);
            require(h.text()=="a      b\nlonger c","elastic conversion uses original measured widths");
            h.editor.send(SCI_UNDO); require(h.text()=="a\tb\nlonger\tc","elastic conversion single undo");
        } else if (id=="SurroundSelection") {
            h.set("one two"); h.editor.send(SCI_SETMULTIPLESELECTION,1);
            h.editor.send(SCI_SETSELECTION,3,0); h.editor.send(SCI_ADDSELECTION,7,4);
            QKeyEvent key(QEvent::KeyPress,Qt::Key_ParenLeft,Qt::ShiftModifier,"(");
            QApplication::sendEvent(&h.editor,&key);
            require(h.text()=="(one) (two)","real multi-selection surround");
            require(h.editor.send(SCI_GETSELECTIONS)==2 && h.editor.send(SCI_GETSELECTIONNSTART,0)==1 && h.editor.send(SCI_GETSELECTIONNSTART,1)==7,"restored inner selections");
            h.editor.send(SCI_UNDO); require(h.text()=="one two","surround must undo in one action");
            QLineEdit edit; edit.setText("other"); edit.selectAll();
            QApplication::sendEvent(&edit,&key); require(edit.text()=="(" && h.text()=="one two","filter must ignore other Qt inputs");
            run(0); h.editor.send(SCI_SETSEL,0,3); QApplication::sendEvent(&h.editor,&key);
            require(h.text()=="( two","disabled hook uses normal editor behavior");
        } else if (id=="SecurePad") {
            QTimer timer;
            QObject::connect(&timer,&QTimer::timeout,[&]() {
                for(auto w:QApplication::topLevelWidgets()) if(w->objectName()=="securePadKeyDialog") {
                    w->findChild<QLineEdit*>("cryptKey")->setText("test-key");
                    w->findChild<QLineEdit*>("confirmCryptKey")->setText("test-key");
                    QMetaObject::invokeMethod(w->findChild<QDialogButtonBox*>("keyButtons"),"accepted",Qt::DirectConnection);
                }
            }); timer.start(1);
            h.set("hello"); run(0); require(h.text().size()==16,"encrypted document size");
            run(1); require(h.text()=="hello","real document encryption roundtrip");
        } else if (id=="qkNppReverseLines") {
            h.set("one\ntwo\nthree"); run(0); require(h.text()=="three\ntwo\none","reverse selection");
            h.editor.send(SCI_UNDO); require(h.text()=="one\ntwo\nthree","reverse undo");
        } else if (id=="mimeTools") {
            h.set("hello"); run(1); require(h.text()=="aGVsbG8=","base64 encode");
            h.editor.send(SCI_SELECTALL); run(5); require(h.text()=="hello","base64 strict decode");
        } else if (id=="nppAutoDetectIndent") {
            h.set("root\n    a\n        b\n    c\n"); event(NPP_PLUGIN_NOTIFICATION_BUFFER_ACTIVATED);
            require(h.editor.send(SCI_GETUSETABS)==0 && h.editor.send(SCI_GETINDENT)==4,"real indentation histogram");
        } else if (id=="SelectToClipboard") {
            h.set("hello world"); h.editor.send(SCI_SETSEL,1,1); run(0); event(NPP_PLUGIN_NOTIFICATION_UPDATE_UI);
            h.editor.send(SCI_SETSEL,0,5); event(NPP_PLUGIN_NOTIFICATION_UPDATE_UI);
            require(QApplication::clipboard()->text()=="hello","real selection copy");
        } else if (id=="SelectQuotedText") {
            h.set("hello world"); h.editor.send(SCI_SETSEL,2,2); run(0);
            require(h.editor.send(SCI_GETSELECTIONSTART)==0 && h.editor.send(SCI_GETSELECTIONEND)==5,"real word fallback");
        } else if (id=="Remove_dup_lines") {
            h.set("a\na\n\nb"); run(0); require(h.text()=="a\r\n\r\nb","real dedup selection");
        } else if (id=="BracketsCheck") {
            h.set("\n\t(]"); QString text; QTimer timer;
            QObject::connect(&timer,&QTimer::timeout,[&]() { for(auto w:QApplication::topLevelWidgets()) if(auto b=qobject_cast<QMessageBox*>(w)) { text=b->text(); if(auto button=b->button(QMessageBox::Ok)) button->click(); } });
            timer.start(1); run(0); require(text.contains("row 2 and character 5"),"real bracket diagnostic");
        }
        event(NPP_PLUGIN_NOTIFICATION_SHUTDOWN);
        qInfo()<<id<<"real Scintilla integration passed"; return 0;
    } catch(const std::exception& e) { qCritical()<<e.what(); return 1; }
}
