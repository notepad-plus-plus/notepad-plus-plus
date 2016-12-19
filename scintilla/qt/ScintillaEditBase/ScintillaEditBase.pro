#-------------------------------------------------
#
# Project created by QtCreator 2011-05-05T12:41:23
#
#-------------------------------------------------

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ScintillaEditBase
TEMPLATE = lib
CONFIG += lib_bundle

VERSION = 3.5.6

SOURCES += \
    PlatQt.cpp \
    ScintillaQt.cpp \
    ScintillaEditBase.cpp \
    ../../src/XPM.cxx \
    ../../src/ViewStyle.cxx \
    ../../src/UniConversion.cxx \
    ../../src/Style.cxx \
    ../../src/Selection.cxx \
    ../../src/ScintillaBase.cxx \
    ../../src/RunStyles.cxx \
    ../../src/RESearch.cxx \
    ../../src/PositionCache.cxx \
    ../../src/PerLine.cxx \
    ../../src/MarginView.cxx \
    ../../src/LineMarker.cxx \
    ../../src/KeyMap.cxx \
    ../../src/Indicator.cxx \
    ../../src/ExternalLexer.cxx \
    ../../src/EditView.cxx \
    ../../src/Editor.cxx \
    ../../src/EditModel.cxx \
    ../../src/Document.cxx \
    ../../src/Decoration.cxx \
    ../../src/ContractionState.cxx \
    ../../src/CharClassify.cxx \
    ../../src/CellBuffer.cxx \
    ../../src/Catalogue.cxx \
    ../../src/CaseFolder.cxx \
    ../../src/CaseConvert.cxx \
    ../../src/CallTip.cxx \
    ../../src/AutoComplete.cxx \
    ../../lexlib/WordList.cxx \
    ../../lexlib/StyleContext.cxx \
    ../../lexlib/PropSetSimple.cxx \
    ../../lexlib/LexerSimple.cxx \
    ../../lexlib/LexerNoExceptions.cxx \
    ../../lexlib/LexerModule.cxx \
    ../../lexlib/LexerBase.cxx \
    ../../lexlib/CharacterSet.cxx \
    ../../lexlib/Accessor.cxx \
    ../../lexlib/CharacterCategory.cxx \
    ../../lexers/*.cxx

HEADERS  += \
    PlatQt.h \
    ScintillaQt.h \
    ScintillaEditBase.h \
    ../../src/XPM.h \
    ../../src/ViewStyle.h \
    ../../src/UniConversion.h \
    ../../src/UnicodeFromUTF8.h \
    ../../src/Style.h \
    ../../src/SplitVector.h \
    ../../src/Selection.h \
    ../../src/ScintillaBase.h \
    ../../src/RunStyles.h \
    ../../src/RESearch.h \
    ../../src/PositionCache.h \
    ../../src/PerLine.h \
    ../../src/Partitioning.h \
    ../../src/LineMarker.h \
    ../../src/KeyMap.h \
    ../../src/Indicator.h \
    ../../src/FontQuality.h \
    ../../src/ExternalLexer.h \
    ../../src/Editor.h \
    ../../src/Document.h \
    ../../src/Decoration.h \
    ../../src/ContractionState.h \
    ../../src/CharClassify.h \
    ../../src/CellBuffer.h \
    ../../src/Catalogue.h \
    ../../src/CaseFolder.h \
    ../../src/CaseConvert.h \
    ../../src/CallTip.h \
    ../../src/AutoComplete.h \
    ../../include/Scintilla.h \
    ../../include/SciLexer.h \
    ../../include/Platform.h \
    ../../include/ILexer.h \
    ../../lexlib/WordList.h \
    ../../lexlib/StyleContext.h \
    ../../lexlib/SparseState.h \
    ../../lexlib/PropSetSimple.h \
    ../../lexlib/OptionSet.h \
    ../../lexlib/LexerSimple.h \
    ../../lexlib/LexerNoExceptions.h \
    ../../lexlib/LexerModule.h \
    ../../lexlib/LexerBase.h \
    ../../lexlib/LexAccessor.h \
    ../../lexlib/CharacterSet.h \
    ../../lexlib/CharacterCategory.h \
    ../../lexlib/Accessor.h

OTHER_FILES +=

INCLUDEPATH += ../../include ../../src ../../lexlib

DEFINES += SCINTILLA_QT=1 MAKING_LIBRARY=1 SCI_LEXER=1 _CRT_SECURE_NO_DEPRECATE=1

DESTDIR = ../../bin

macx {
	QMAKE_LFLAGS_SONAME = -Wl,-install_name,@executable_path/../Frameworks/
}
