// This file is part of the Notepad++ Qt port.
// Language names and filenames are retained from Notepad++ v8.4.6
// PowerEditor/src/localizationString.h (GPL-3.0-or-later).

#ifndef NPP_LOCALIZATION_STRING_H
#define NPP_LOCALIZATION_STRING_H

#include <cstddef>

struct NativeLanguageDefinition
{
    const char* displayName;
    const char* xmlFileName;
};

inline const NativeLanguageDefinition* nativeLanguageDefinitions(
    std::size_t& count)
{
    static const NativeLanguageDefinition definitions[] = {
        {"English", "english.xml"},
        {"English (customizable)", "english_customizable.xml"},
        {"Français", "french.xml"},
        {"台灣繁體", "taiwaneseMandarin.xml"},
        {"中文简体", "chineseSimplified.xml"},
        {"한국어", "korean.xml"},
        {"日本語", "japanese.xml"},
        {"Deutsch", "german.xml"},
        {"Español", "spanish.xml"},
        {"Italiano", "italian.xml"},
        {"Português", "portuguese.xml"},
        {"Português brasileiro", "brazilian_portuguese.xml"},
        {"Nederlands", "dutch.xml"},
        {"Русский", "russian.xml"},
        {"Polski", "polish.xml"},
        {"Català", "catalan.xml"},
        {"Česky", "czech.xml"},
        {"Magyar", "hungarian.xml"},
        {"Română", "romanian.xml"},
        {"Türkçe", "turkish.xml"},
        {"فارسی", "farsi.xml"},
        {"Українська", "ukrainian.xml"},
        {"עברית", "hebrew.xml"},
        {"Nynorsk", "nynorsk.xml"},
        {"Norsk", "norwegian.xml"},
        {"Occitan", "occitan.xml"},
        {"ไทย", "thai.xml"},
        {"Furlan", "friulian.xml"},
        {"العربية", "arabic.xml"},
        {"Suomi", "finnish.xml"},
        {"Lietuvių", "lithuanian.xml"},
        {"Ελληνικά", "greek.xml"},
        {"Svenska", "swedish.xml"},
        {"Galego", "galician.xml"},
        {"Slovenščina", "slovenian.xml"},
        {"Slovenčina", "slovak.xml"},
        {"Dansk", "danish.xml"},
        {"Estremeñu", "extremaduran.xml"},
        {"Žemaitiu ruoda", "samogitian.xml"},
        {"Български", "bulgarian.xml"},
        {"Bahasa Indonesia", "indonesian.xml"},
        {"Gjuha shqipe", "albanian.xml"},
        {"Hrvatski", "croatian.xml"},
        {"ქართული ენა", "georgian.xml"},
        {"Euskara", "basque.xml"},
        {"Español argentina", "spanish_ar.xml"},
        {"Беларуская мова", "belarusian.xml"},
        {"Srpski", "serbian.xml"},
        {"Cрпски", "serbianCyrillic.xml"},
        {"Bahasa Melayu", "malay.xml"},
        {"Lëtzebuergesch", "luxembourgish.xml"},
        {"Tagalog", "tagalog.xml"},
        {"Afrikaans", "afrikaans.xml"},
        {"Қазақша", "kazakh.xml"},
        {"O‘zbekcha", "uzbek.xml"},
        {"Ўзбекча", "uzbekCyrillic.xml"},
        {"Кыргыз тили", "kyrgyz.xml"},
        {"Македонски јазик", "macedonian.xml"},
        {"latviešu valoda", "latvian.xml"},
        {"தமிழ்", "tamil.xml"},
        {"Azərbaycan dili", "azerbaijani.xml"},
        {"Bosanski", "bosnian.xml"},
        {"Esperanto", "esperanto.xml"},
        {"Zeneize", "ligurian.xml"},
        {"हिन्दी", "hindi.xml"},
        {"Sardu", "sardinian.xml"},
        {"ئۇيغۇرچە", "uyghur.xml"},
        {"తెలుగు", "telugu.xml"},
        {"aragonés", "aragonese.xml"},
        {"বাংলা", "bengali.xml"},
        {"සිංහල", "sinhala.xml"},
        {"Taqbaylit", "kabyle.xml"},
        {"मराठी", "marathi.xml"},
        {"tiếng Việt", "vietnamese.xml"},
        {"Aranés", "aranese.xml"},
        {"ગુજરાતી", "gujarati.xml"},
        {"Монгол хэл", "mongolian.xml"},
        {"اُردُو‎", "urdu.xml"},
        {"ಕನ್ನಡ‎", "kannada.xml"},
        {"Cymraeg", "welsh.xml"},
        {"eesti keel", "estonian.xml"},
        {"Тоҷик", "tajikCyrillic.xml"},
        {"татарча", "tatar.xml"},
        {"ਪੰਜਾਬੀ", "punjabi.xml"},
        {"Corsu", "corsican.xml"},
        {"Brezhoneg", "breton.xml"},
        {"کوردی‬", "kurdish.xml"},
        {"Pig latin", "piglatin.xml"},
        {"Zulu", "zulu.xml"},
        {"Vèneto", "venetian.xml"},
        {"Gaeilge", "irish.xml"},
        {"नेपाली", "nepali.xml"},
        {"香港繁體", "hongKongCantonese.xml"},
        {"Аԥснышәа", "abkhazian.xml"}
    };
    count = sizeof(definitions) / sizeof(definitions[0]);
    return definitions;
}

#endif
