# Notepad++ Qt editor core: QScintilla plus the original Boost.Regex backend.

CONFIG += staticlib
DEFINES += SCI_OWNREGEX

include(qscintilla.pro)

TARGET = qscintilla2_qt$${QT_MAJOR_VERSION}_npp

NPP_BOOSTREGEX_ROOT = $$clean_path($$NPP_BOOSTREGEX_SOURCE_ROOT)
NPP_LEXUSER = $$clean_path($$NPP_LEXUSER_SOURCE)
INCLUDEPATH += $$NPP_BOOSTREGEX_ROOT

HEADERS += \
    $$NPP_BOOSTREGEX_ROOT/AnsiDocumentIterator.h \
    $$NPP_BOOSTREGEX_ROOT/BoostRegexSearch.h \
    $$NPP_BOOSTREGEX_ROOT/UTF8DocumentIterator.h

SOURCES += \
    $$NPP_BOOSTREGEX_ROOT/BoostRegExSearch.cxx \
    $$NPP_BOOSTREGEX_ROOT/UTF8DocumentIterator.cxx \
    $$NPP_LEXUSER
