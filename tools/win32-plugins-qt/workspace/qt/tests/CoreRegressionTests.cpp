#include "../../SecurePad/qt/SecurePadCore.h"
#include "../../mimetools/src/saml.h"
#include "../../mimetools/src/qp.h"
#include "../../mimetools/src/tinf.h"
#include "../../ElasticTabstops/qt/ConfigCore.h"
#include "../../Merge-files-in-one/qt/MergeCore.h"
#include "../../nppURLPlugin/src/urlPlugin/URL.h"
#include "../../nppURLPlugin/qt/SplitCore.h"
#include <QCoreApplication>
#include <iostream>
#include <random>
static void require(bool ok,const char* why) { if(!ok) throw std::runtime_error(why); }
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        Encode urlEncoder; urlEncoder.setUrl("https://example.com/a?q=a b&x=+");
        require(urlEncoder.encode(false)=="https://example.com/a?q=a%20b&x=+","original URL base/query encoding");
        urlEncoder.setUrl("a\nb\tc"); require(urlEncoder.encode(false)=="abc","original URL whitespace removal");
        urlEncoder.setUrl("a\nb"); require(urlEncoder.encode(true)=="a%Ab","original retained LF hex spelling");
        urlEncoder.setUrl("中文"); require(urlEncoder.encode(false)=="%E4%B8%AD%E6%96%87","URL unsigned UTF-8 byte boundary");
        Decode urlDecoder; urlDecoder.setUrl("https://x/?a=1&a=2&b=x%20y+");
        require(urlDecoder.decode()=="https://x/?a=1&a=2&b=x y+" && urlDecoder.getParameters().at("a")=="2","original URL parameter map and plus preservation");
        urlDecoder.setUrl("abc%"); require(urlDecoder.decode()=="abc","original truncated escape handling");
        require(urlSplit(std::string("&&a&b;;c&"),std::string("&;"))==std::vector<std::string>({"a","b","c"}),"original delimiter character-set split");
        QString first="a\nb", marker=" 3f5456cfsd661lld33dGuid9CA0F324 ";
        require(MergeFiles::merge(first,"x\ny",false)=="a"+marker+"x\r\nb"+marker+"y","original merge marker and pairing");
        first="a\nb\nc";
        require(MergeFiles::merge(first,"x",true)=="x"+marker+"a\r\nb\r\nc","merge reverse and unmatched first tail");
        first="a"; require(MergeFiles::merge(first,"x\ny",false)=="a"+marker+"x\r\n"+marker+"y","merge pads short first input");
        first="a"; require(MergeFiles::merge(first,"",false).isEmpty(),"empty second creates no output");
        // Oracle values verified with the original .NET Regex.Replace overload.
        require(MergeFiles::expandEndReplacement("base","$$/$0/$00/${00}/$01/$+/$&/$_/$`")=="$////$01///base/base",".NET zero-width replacement tokens");
        Configuration config{true,{"*"},1,false};
        auto parseConfig=[&](const std::string& text) {
            size_t offset=0;
            ElasticConfig::read([&](char* output,int capacity) {
                if(offset==text.size()) return false;
                size_t n=0; while(offset<text.size() && n<size_t(capacity-1)) {
                    char c=text[offset++]; output[n++]=c; if(c=='\n') break;
                }
                output[n]=0; return true;
            },&config);
        };
        parseConfig(";comment\nenabled false\nextensions !.txt  .cpp *\npadding -1\nconvert_leading_tabs_to_spaces true\n");
        require(!config.enabled && config.file_extensions==std::vector<std::string>({"!.txt",".cpp","*"}) && config.min_padding==256 && config.convert_leading_tabs_to_spaces,"original elastic config grammar/clamping");
        parseConfig("padding 0\n"); require(config.min_padding==1,"elastic zero padding clamp");
        auto saved=ElasticConfig::write(&config); config={}; parseConfig(saved);
        require(!config.enabled && config.min_padding==1 && config.convert_leading_tabs_to_spaces,"elastic config serialization");
        // Independent published vector already included in the original blowfish.h.
        QByteArray key(8,'\0'), block(8,'\0');
        CBlowFish cipher(reinterpret_cast<unsigned char*>(key.data()),key.size());
        cipher.Encrypt(reinterpret_cast<unsigned char*>(block.data()),block.size());
        require(block.toHex().toUpper()=="4EF997456198DD78","original Blowfish test vector");
        cipher.Decrypt(reinterpret_cast<unsigned char*>(block.data()),block.size());
        require(block==QByteArray(8,'\0'),"Blowfish decryption vector");
        auto password=securePadKey("test-key");
        for(const QByteArray plain:{QByteArray("hello"),QByteArray("12345678"),QByteArray("中文 UTF-8")}) {
            auto encrypted=securePadCrypt(plain,password,true);
            require(encrypted.size()%16==0 && securePadCrypt(encrypted,password,false)==plain,"SecurePad file-format roundtrip");
        }
        bool rejected=false; try { securePadCrypt("abc",password,false); } catch(const std::invalid_argument&) { rejected=true; }
        require(rejected,"incomplete encrypted block accepted");
        QByteArray xml("<saml>raw deflate regression</saml>");
        auto compressed=qCompress(xml,9); auto raw=compressed.mid(6,compressed.size()-10);
        auto encoded=raw.toBase64(); QByteArray output(SAML_MESSAGE_MAX_SIZE,'\0');
        int n=samlDecode(output.data(),encoded.constData(),encoded.size()+1);
        require(n==xml.size() && output.left(n)==xml,"original SAML raw inflate pipeline");
        auto truncated=raw.left(raw.size()/2); unsigned int length=0;
        require(tinf_uncompress_bounded(output.data(),&length,truncated.data(),truncated.size(),output.size())==TINF_DATA_ERROR,"truncated deflate accepted");
        require(tinf_uncompress_bounded(output.data(),&length,raw.data(),raw.size(),4)==TINF_DATA_ERROR,"output capacity ignored");
        // Deterministic malformed input corpus exercises the added reader bounds.
        std::mt19937 generator(516);
        for(int i=0;i<5000;++i) {
            QByteArray random(1+generator()%100,'\0');
            for(auto& c:random) c=static_cast<char>(generator());
            tinf_uncompress_bounded(output.data(),&length,random.data(),random.size(),128);
        }
        std::cout << "Original cipher vectors, SAML framing and 5000 malformed streams passed\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
