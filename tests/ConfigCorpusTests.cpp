#include "Parameters.h"
#include "TinyXml/tinyxml.h"

#include <QApplication>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTemporaryDir>
#include <algorithm>
#include <cstdio>

namespace {

QStringList canonicalNode(const QDomNode& node)
{
    QStringList result;
    if (node.isElement()) {
        const QDomElement element = node.toElement();
        QStringList attributes;
        const QDomNamedNodeMap map = element.attributes();
        for (int i = 0; i < map.count(); ++i) {
            const QDomAttr attribute = map.item(i).toAttr();
            attributes.append(attribute.name() + "=" + attribute.value());
        }
        std::sort(attributes.begin(), attributes.end());
        result.append("<" + element.tagName() + " " + attributes.join("|") + ">");
    } else if (node.isText() || node.isCDATASection()) {
        const QString value = node.nodeValue();
        if (!value.trimmed().isEmpty())
            result.append("#text:" + value);
    }

    for (QDomNode child = node.firstChild(); !child.isNull();
         child = child.nextSibling()) {
        result.append(canonicalNode(child));
    }

    if (node.isElement())
        result.append("</" + node.toElement().tagName() + ">");
    return result;
}

bool loadDom(const QString& path, QDomDocument* document, QString* error)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly)) {
        *error = file.errorString();
        return false;
    }

    QString message;
    int line = 0;
    int column = 0;
    if (!document->setContent(&file, true, &message, &line, &column)) {
        *error = QString("%1 at %2:%3").arg(message).arg(line).arg(column);
        return false;
    }
    return true;
}

bool saveDom(const QString& path, const QDomDocument& document, QString* error)
{
    QFile file(path);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        *error = file.errorString();
        return false;
    }
    if (file.write(document.toByteArray(2)) < 0) {
        *error = file.errorString();
        return false;
    }
    return true;
}

bool addPreservationSentinels(const QString& path, QString* error)
{
    QDomDocument document;
    if (!loadDom(path, &document, error))
        return false;

    QDomElement root = document.documentElement();
    root.setAttribute("codexUnknownRootAttribute", "preserve-root");

    QDomElement firstElement = root.firstChildElement();
    if (!firstElement.isNull())
        firstElement.setAttribute("codexUnknownChildAttribute", "preserve-child");

    QDomElement unknown = document.createElement("CodexUnknownRootNode");
    unknown.setAttribute("order", "preserve");
    QDomElement nested = document.createElement("NestedUnknownNode");
    nested.setAttribute("value", "42");
    nested.appendChild(document.createTextNode("preserve-me"));
    unknown.appendChild(nested);
    root.appendChild(unknown);

    QDomNodeList files = root.elementsByTagName("File");
    if (!files.isEmpty()) {
        QDomElement file = files.at(0).toElement();
        file.setAttribute("codexUnknownFileAttribute", "preserve-file");
        QDomElement fileChild = document.createElement("CodexUnknownFileState");
        fileChild.setAttribute("enabled", "yes");
        file.appendChild(fileChild);
    }

    return saveDom(path, document, error);
}

bool setDarkMode(const QString& path, bool enabled, QString* error)
{
    QDomDocument document;
    if (!loadDom(path, &document, error))
        return false;
    const QDomNodeList configs = document.elementsByTagName("GUIConfig");
    for (int i = 0; i < configs.count(); ++i) {
        QDomElement element = configs.at(i).toElement();
        if (element.attribute("name") == "DarkMode") {
            element.setAttribute("enable", enabled ? "yes" : "no");
            return saveDom(path, document, error);
        }
    }
    const QDomNodeList containers = document.elementsByTagName("GUIConfigs");
    if (containers.isEmpty()) {
        *error = "GUIConfigs container is missing";
        return false;
    }
    QDomElement element = document.createElement("GUIConfig");
    element.setAttribute("name", "DarkMode");
    element.setAttribute("enable", enabled ? "yes" : "no");
    containers.at(0).appendChild(element);
    return saveDom(path, document, error);
}

bool compareXml(const QString& beforePath, const QString& afterPath,
                QString* error)
{
    QDomDocument before;
    QDomDocument after;
    if (!loadDom(beforePath, &before, error) ||
        !loadDom(afterPath, &after, error)) {
        return false;
    }

    const QStringList expected = canonicalNode(before.documentElement());
    const QStringList actual = canonicalNode(after.documentElement());
    if (expected == actual)
        return true;

    const int common = std::min(expected.size(), actual.size());
    int index = 0;
    while (index < common && expected.at(index) == actual.at(index))
        ++index;
    *error = QString("semantic XML differs at token %1\nexpected: %2\nactual: %3")
        .arg(index)
        .arg(index < expected.size() ? expected.at(index) : "<end>")
        .arg(index < actual.size() ? actual.at(index) : "<end>");
    return false;
}

QString targetNameForMode(const QString& mode)
{
    if (mode == "config")
        return "config.xml";
    if (mode == "session")
        return "session.xml";
    if (mode == "shortcuts")
        return "shortcuts.xml";
    if (mode == "langs")
        return "langs.xml";
    if (mode == "stylers")
        return "stylers.xml";
    if (mode == "udl")
        return "userDefineLang.xml";
    if (mode == "context")
        return "contextMenu.xml";
    return QString();
}

bool checkReadOnlyCorpus(const QString& mode, NppParameters& parameters,
                         QString* error)
{
    if (mode == "langs") {
        if (!parameters.loadLangs() || parameters.getLangDescs().isEmpty()) {
            *error = "langs.xml did not produce any language descriptions";
            return false;
        }
    } else if (mode == "stylers") {
        if (!parameters.loadStylers() || parameters.getGlobalStyles().isEmpty()) {
            *error = "stylers.xml did not produce any global styles";
            return false;
        }
    } else if (mode == "udl") {
        if (!parameters.loadUserDefinedLanguages()) {
            *error = "userDefineLang.xml could not be loaded";
            return false;
        }
    }
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    if (argc != 3) {
        std::fprintf(stderr, "usage: config-corpus-tests MODE SOURCE_XML\n");
        return 2;
    }

    const QString mode = QString::fromLocal8Bit(argv[1]);
    const QString sourcePath = QFileInfo(QString::fromLocal8Bit(argv[2]))
        .absoluteFilePath();
    const QString targetName = targetNameForMode(mode);
    if (targetName.isEmpty() || !QFileInfo::exists(sourcePath)) {
        std::fprintf(stderr, "invalid mode or missing corpus: %s\n",
                     qPrintable(sourcePath));
        return 2;
    }

    QTemporaryDir temporary;
    if (!temporary.isValid()) {
        std::fprintf(stderr, "could not create temporary directory\n");
        return 2;
    }

    qputenv("APPDATA", QDir::toNativeSeparators(temporary.path()).toLocal8Bit());
    qputenv("LOCALAPPDATA",
            QDir::toNativeSeparators(temporary.path() + "/Local").toLocal8Bit());

    const QString userPath = temporary.path() + "/Notepad++";
    QDir().mkpath(userPath);
    const QString targetPath = userPath + "/" + targetName;
    if (!QFile::copy(sourcePath, targetPath)) {
        std::fprintf(stderr, "could not copy corpus to isolated APPDATA\n");
        return 2;
    }

    QString error;
    if (!addPreservationSentinels(targetPath, &error)) {
        std::fprintf(stderr, "could not prepare corpus: %s\n", qPrintable(error));
        return 2;
    }
    if (mode == "config") {
        if (!setDarkMode(targetPath, true, &error)) {
            std::fprintf(stderr, "could not enable original DarkMode: %s\n",
                         qPrintable(error));
            return 2;
        }
        QFile staleQtState(userPath + "/qtState.ini");
        if (!staleQtState.open(QFile::WriteOnly | QFile::Truncate)
            || staleQtState.write("[Editor]\ndarkMode=false\n") < 0) {
            std::fprintf(stderr, "could not create stale Qt state\n");
            return 2;
        }
    }
    const QString expectedPath = temporary.path() + "/expected.xml";
    if (!QFile::copy(targetPath, expectedPath)) {
        std::fprintf(stderr, "could not preserve expected corpus\n");
        return 2;
    }

    const std::wstring targetPathWide = targetPath.toStdWString();
    TiXmlDocument directDocument;
    if (!directDocument.LoadFile(targetPathWide.c_str())) {
        std::fprintf(stderr, "TinyXml direct Unicode-path load failed\n");
        return 1;
    }
    const QString unicodeRoundTripPath =
        temporary.path() + QString::fromUtf8("/统一路径.xml");
    const std::wstring unicodeRoundTripPathWide =
        unicodeRoundTripPath.toStdWString();
    if (!directDocument.SaveFile(unicodeRoundTripPathWide.c_str())) {
        std::fprintf(stderr, "TinyXml direct Unicode-path save failed\n");
        return 1;
    }
    TiXmlDocument reloadedDocument;
    if (!reloadedDocument.LoadFile(unicodeRoundTripPathWide.c_str())) {
        std::fprintf(stderr, "TinyXml Unicode-path round trip failed\n");
        return 1;
    }

    NppParameters& parameters = NppParameters::getInstance();
    if (!parameters.setUserPathOverride(userPath)) {
        std::fprintf(stderr, "could not select isolated settings directory\n");
        return 2;
    }
    if (!parameters.load()) {
        std::fprintf(stderr, "NppParameters::load failed for %s\n",
                     qPrintable(sourcePath));
        return 1;
    }

    if (mode == "config") {
        if (!parameters.getNppGUI()._darkModeEnabled) {
            std::fprintf(stderr,
                         "original DarkMode setting was not loaded\n");
            return 1;
        }
        parameters.getNppGUI()._darkModeEnabled = false;
        if (!setDarkMode(expectedPath, false, &error)) {
            std::fprintf(stderr, "could not update expected DarkMode: %s\n",
                         qPrintable(error));
            return 2;
        }
    }

    bool written = true;
    if (mode == "config")
        written = parameters.writeNppGUI() && parameters.writeFindHistory();
    else if (mode == "session")
        written = parameters.writeSession(parameters.getSession());
    else if (mode == "shortcuts")
        written = parameters.writeShortcuts();
    else
        written = checkReadOnlyCorpus(mode, parameters, &error);

    if (!written) {
        std::fprintf(stderr, "load/write operation failed: %s\n",
                     qPrintable(error));
        return 1;
    }

    if (!compareXml(expectedPath, targetPath, &error)) {
        std::fprintf(stderr, "%s\ncorpus: %s\n", qPrintable(error),
                     qPrintable(sourcePath));
        return 1;
    }
    if (mode == "config") {
        QSettings qtState(userPath + "/qtState.ini", QSettings::IniFormat);
        if (qtState.contains("Editor/darkMode")) {
            std::fprintf(stderr,
                         "legacy Qt darkMode key was not removed\n");
            return 1;
        }
    }

    return 0;
}
