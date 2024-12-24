#-------------------------------------------------
#
# Project created by QtCreator 2011-05-05T12:41:23
#
#-------------------------------------------------

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
equals(QT_MAJOR_VERSION, 6): QT += core5compat

TARGET = ScintillaEdit
TEMPLATE = lib
CONFIG += lib_bundle
CONFIG += c++1z

VERSION = 5.5.4

SOURCES += \
    ScintillaEdit.cpp \
    ScintillaDocument.cpp \
    ../ScintillaEditBase/PlatQt.cpp \
    ../ScintillaEditBase/ScintillaQt.cpp \
    ../ScintillaEditBase/ScintillaEditBase.cpp \
    ../../src/XPM.cxx \
    ../../src/ViewStyle.cxx \
    ../../src/UndoHistory.cxx \
    ../../src/UniqueString.cxx \
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
    ../../src/Geometry.cxx \
    ../../src/EditView.cxx \
    ../../src/Editor.cxx \
    ../../src/EditModel.cxx \
    ../../src/Document.cxx \
    ../../src/Decoration.cxx \
    ../../src/DBCS.cxx \
    ../../src/ContractionState.cxx \
    ../../src/CharClassify.cxx \
    ../../src/CharacterType.cxx \
    ../../src/CharacterCategoryMap.cxx \
    ../../src/ChangeHistory.cxx \
    ../../src/CellBuffer.cxx \
    ../../src/CaseFolder.cxx \
    ../../src/CaseConvert.cxx \
    ../../src/CallTip.cxx \
    ../../src/AutoComplete.cxx

HEADERS  += \
    ScintillaEdit.h \
    ScintillaDocument.h \
    ../ScintillaEditBase/ScintillaEditBase.h \
    ../ScintillaEditBase/ScintillaQt.h

OTHER_FILES +=

INCLUDEPATH += ../ScintillaEditBase ../../include ../../src

DEFINES += SCINTILLA_QT=1 MAKING_LIBRARY=1
CONFIG(release, debug|release) {
    DEFINES += NDEBUG=1
}

DESTDIR = ../../bin
DLLDESTDIR = ../../bin

macx {
	QMAKE_LFLAGS_SONAME = -Wl,-install_name,@executable_path/../Frameworks/
}
