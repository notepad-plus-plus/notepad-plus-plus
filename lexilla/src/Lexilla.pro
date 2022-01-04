# This Qt Creator project file is not meant for creating Lexilla libraries
# but instead for easily running Clang-Tidy on lexers.

QT       += core

TARGET = Lexilla
TEMPLATE = lib
CONFIG += lib_bundle
CONFIG += c++1z

VERSION = 5.1.3

SOURCES += \
    Lexilla.cxx \
    $$files(../lexlib/*.cxx, false) \
    $$files(../lexers/*.cxx, false)

HEADERS  += \
    ../include/Lexilla.h \
    $$files(../lexers/*.h, false)

INCLUDEPATH += ../include ../lexlib ../../scintilla/include

DEFINES += _CRT_SECURE_NO_DEPRECATE=1
CONFIG(release, debug|release) {
    DEFINES += NDEBUG=1
}

DESTDIR = ../bin

macx {
	QMAKE_LFLAGS_SONAME = -Wl,-install_name,@executable_path/../Frameworks/
}
