#include <QCryptographicHash>
#include <QDomDocument>
#include <QFile>
#include <QSet>
#include <QString>

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}

QByteArray readBytes(const QString& path)
{
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "test source file is not readable");
    return file.readAll();
}

QString readText(const QString& path)
{
    return QString::fromUtf8(readBytes(path));
}

void verifyOfficialLanguage(const QString& path, const QString& filename,
                            const QByteArray& expectedSha256)
{
    const QByteArray data = readBytes(path);
    QByteArray canonicalData = data;
    canonicalData.replace("\r\n", "\n");
    require(QCryptographicHash::hash(canonicalData,
                                    QCryptographicHash::Sha256).toHex()
                == expectedSha256,
            "native language resource differs from Notepad++ v8.4.6");

    QDomDocument document;
    require(document.setContent(data), "native language XML is invalid");
    const QDomElement nativeLanguage = document.documentElement()
        .firstChildElement(QStringLiteral("Native-Langue"));
    require(nativeLanguage.attribute(QStringLiteral("filename")) == filename,
            "native language filename attribute changed");
    require(nativeLanguage.attribute(QStringLiteral("version"))
                == QStringLiteral("8.4.6"),
            "native language version attribute changed");

    const QDomNodeList allElements = document.elementsByTagName(QStringLiteral("*"));
    for (int i = 0; i < allElements.count(); ++i) {
        require(!allElements.at(i).toElement().hasAttribute(
                    QStringLiteral("objectName")),
                "Qt objectName must not be written into original language XML");
    }

    const QDomElement find = nativeLanguage.firstChildElement(QStringLiteral("Dialog"))
        .firstChildElement(QStringLiteral("Find"));
    require(!find.isNull(), "original Find dialog language section is missing");
    QSet<int> ids;
    for (QDomElement item = find.firstChildElement(QStringLiteral("Item"));
         !item.isNull(); item = item.nextSiblingElement(QStringLiteral("Item"))) {
        ids.insert(item.attribute(QStringLiteral("id")).toInt());
    }
    for (int id : {1, 2, 1603, 1615, 1620, 1624, 1633, 1656, 1703})
        require(ids.contains(id), "original Find dialog control ID is missing");
}

} // namespace

int main()
{
    verifyOfficialLanguage(
        QStringLiteral(NPP_ENGLISH_LANGUAGE_SOURCE), QStringLiteral("english.xml"),
        QByteArrayLiteral("912f515c673bc57ee8f359dd31258e3552f7bb5f2cdffbcb258894806a4848c7"));
    verifyOfficialLanguage(
        QStringLiteral(NPP_FIND_LANGUAGE_SOURCE),
        QStringLiteral("chineseSimplified.xml"),
        QByteArrayLiteral("0bfbd9eb282ed4f7b23d4c136b939e186aaf78d8efbd94893744993e86544ad1"));
    verifyOfficialLanguage(
        QStringLiteral(NPP_JAPANESE_LANGUAGE_SOURCE), QStringLiteral("japanese.xml"),
        QByteArrayLiteral("971ac1b027873286fb4dd321fb088e80fb47d16ffc98294d7eecef4841f545dd"));

    const QString localizationSource =
        readText(QStringLiteral(NPP_LOCALIZATION_SOURCE));
    require(localizationSource.contains(QStringLiteral("nppCommandMappings()"))
                && localizationSource.contains(
                    QStringLiteral("findDialogObjectNames(int originalId)"))
                && localizationSource.contains(
                    QStringLiteral("txAttr(item, L\"id\")")),
            "Qt platform layer must adapt original numeric localization IDs");
    require(localizationSource.contains(
                QStringLiteral("getDoSaveOrNotStrings"))
                && localizationSource.contains(
                    QStringLiteral("FirstChildElement(L\"DoSaveOrNot\")")),
            "close-save prompt must use the original nativeLang dialog node");

    const QString findDialogSource =
        readText(QStringLiteral(NPP_FIND_DIALOG_SOURCE));
    for (const QString& objectName : {
             QStringLiteral("btnFindNext"), QStringLiteral("btnMarkAll"),
             QStringLiteral("btnClearMarks"), QStringLiteral("lblFindWhat"),
             QStringLiteral("grpSearchMode"), QStringLiteral("rbModeRegex")}) {
        require(findDialogSource.contains(
                    QStringLiteral("setObjectName(\"%1\")").arg(objectName)),
                "mapped Qt Find control lacks a stable objectName");
    }

    const QString preferencesSource =
        readText(QStringLiteral(NPP_PREFERENCES_SOURCE));
    require(preferencesSource.contains(
                QStringLiteral("getAvailableNativeLanguages()"))
                && preferencesSource.contains(
                    QStringLiteral("_languageCombo->addItem(language.first, language.second)")),
            "Preferences must enumerate installed native language files");

    const QString mainSource = readText(QStringLiteral(NPP_MAIN_SOURCE));
    const QString desktopEntry = readText(QStringLiteral(NPP_DESKTOP_SOURCE));
    require(mainSource.contains(QStringLiteral(
                "setDesktopFileName(QStringLiteral(\"notepad-plus-plus\"))"))
                && desktopEntry.contains(
                    QStringLiteral("StartupWMClass=notepad++")),
            "Ubuntu launcher identity must support pinning to favorites");
    return 0;
}
