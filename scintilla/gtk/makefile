# Make file for Scintilla on Linux or compatible OS
# Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
# The License.txt file describes the conditions under which this software may be distributed.
# This makefile assumes GCC 4.3 is used and changes will be needed to use other compilers.
# GNU make does not like \r\n line endings so should be saved to CVS in binary form.
# Builds for GTK+ 2 and no longer supports GTK+ 1.
# Also works with ming32-make on Windows.

.SUFFIXES: .cxx .c .o .h .a
ifdef CLANG
CXX = clang++ -Wno-deprecated-register
CC = clang
# Can choose aspect to sanitize: address and undefined can simply change SANITIZE but for
# thread also need to create Position Independent Executable -> search online documentation
SANITIZE = address
#SANITIZE = undefined
endif
RANLIB = touch

ifdef GTK3
GTKVERSION=gtk+-3.0
else
GTKVERSION=gtk+-2.0
endif

# Environment variable windir always defined on Win32

ifndef windir
ifeq ($(shell uname),Darwin)
RANLIB = ranlib
endif
endif

ifdef windir
DEL = del /q
COMPLIB=..\bin\scintilla.a
else
DEL = rm -f
COMPLIB=../bin/scintilla.a
endif

vpath %.h ../src ../include ../lexlib
vpath %.cxx ../src ../lexlib ../lexers

INCLUDEDIRS=-I ../include -I ../src -I ../lexlib
ifdef CHECK_DEPRECATED
DEPRECATED=-DGDK_PIXBUF_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DDISABLE_GDK_FONT
endif
CXXBASEFLAGS=-Wall -pedantic -DGTK -DSCI_LEXER $(INCLUDEDIRS) $(DEPRECATED)

ifdef NOTHREADS
THREADFLAGS=-DG_THREADS_IMPL_NONE
else
THREADFLAGS=
endif

ifdef CXX11_REGEX
REFLAGS=-DCXX11_REGEX
endif

ifdef DEBUG
ifdef CLANG
CTFLAGS=-DDEBUG -g -fsanitize=$(SANITIZE) $(CXXBASEFLAGS) $(THREADFLAGS)
else
CTFLAGS=-DDEBUG -g $(CXXBASEFLAGS) $(THREADFLAGS)
endif
else
CTFLAGS=-DNDEBUG -Os $(CXXBASEFLAGS) $(THREADFLAGS)
endif

CFLAGS:=$(CTFLAGS)
CXXTFLAGS:=--std=c++0x $(CTFLAGS) $(REFLAGS)

CONFIGFLAGS:=$(shell pkg-config --cflags $(GTKVERSION))
MARSHALLER=scintilla-marshal.o

.cxx.o:
	$(CXX) $(CONFIGFLAGS) $(CXXTFLAGS) $(CXXFLAGS) -c $<
.c.o:
	$(CC) $(CONFIGFLAGS) $(CFLAGS) -w -c $<

LEXOBJS:=$(addsuffix .o,$(basename $(notdir $(wildcard ../lexers/Lex*.cxx))))

all: $(COMPLIB)

clean:
	$(DEL) *.o $(COMPLIB) *.plist

analyze:
	clang --analyze $(CONFIGFLAGS) $(CXXTFLAGS) $(CXXFLAGS) *.cxx ../src/*.cxx ../lexlib/*.cxx ../lexers/*.cxx

deps:
	$(CXX) -MM $(CONFIGFLAGS) $(CXXTFLAGS) *.cxx ../src/*.cxx ../lexlib/*.cxx ../lexers/*.cxx | sed -e 's/\/usr.* //' | grep [a-zA-Z] >deps.mak

$(COMPLIB): Accessor.o CharacterSet.o LexerBase.o LexerModule.o LexerSimple.o StyleContext.o WordList.o \
	CharClassify.o Decoration.o Document.o PerLine.o Catalogue.o CallTip.o CaseConvert.o CaseFolder.o \
	ScintillaBase.o ContractionState.o EditModel.o Editor.o EditView.o ExternalLexer.o MarginView.o \
	PropSetSimple.o PlatGTK.o \
	KeyMap.o LineMarker.o PositionCache.o ScintillaGTK.o CellBuffer.o CharacterCategory.o ViewStyle.o \
	RESearch.o RunStyles.o Selection.o Style.o Indicator.o AutoComplete.o UniConversion.o XPM.o \
	$(MARSHALLER) $(LEXOBJS)
	$(AR) rc $@ $^
	$(RANLIB) $@

# Automatically generate header dependencies with "make deps"
include deps.mak
