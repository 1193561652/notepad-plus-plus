# Notepad++ Qt editor core. Lexilla is built as a separate static library.

CONFIG += staticlib
DEFINES += SCI_OWNREGEX

include(qscintilla.pro)

TARGET = qscintilla2_qt$${QT_MAJOR_VERSION}_npp

NPP_BOOSTREGEX_ROOT = $$clean_path($$NPP_BOOSTREGEX_SOURCE_ROOT)
NPP_LEXILLA = $$clean_path($$NPP_LEXILLA_ROOT)
INCLUDEPATH += $$NPP_BOOSTREGEX_ROOT $$NPP_LEXILLA/lexlib

HEADERS += \
    $$NPP_BOOSTREGEX_ROOT/AnsiDocumentIterator.h \
    $$NPP_BOOSTREGEX_ROOT/BoostRegexSearch.h \
    $$NPP_BOOSTREGEX_ROOT/UTF8DocumentIterator.h

SOURCES += \
    $$NPP_BOOSTREGEX_ROOT/BoostRegExSearch.cxx \
    $$NPP_BOOSTREGEX_ROOT/UTF8DocumentIterator.cxx
