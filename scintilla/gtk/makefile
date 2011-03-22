# Make file for Scintilla on Linux or compatible OS
# Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
# The License.txt file describes the conditions under which this software may be distributed.
# This makefile assumes GCC 4.3 is used and changes will be needed to use other compilers.
# GNU make does not like \r\n line endings so should be saved to CVS in binary form.
# Builds for GTK+ 2 and no longer supports GTK+ 1.
# Also works with ming32-make on Windows.

.SUFFIXES: .cxx .c .o .h .a
ifdef CLANG
CC = clang
CCOMP = clang
else
CC = g++
CCOMP = gcc
endif
AR = ar
RANLIB = touch

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
#~ DEPRECATED=-DGDK_PIXBUF_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DDISABLE_GDK_FONT
CXXBASEFLAGS=-Wall -Wno-missing-braces -Wno-char-subscripts -Wno-long-long -pedantic -DGTK -DSCI_LEXER $(INCLUDEDIRS) $(DEPRECATED)

ifdef NOTHREADS
THREADFLAGS=-DG_THREADS_IMPL_NONE
else
THREADFLAGS=
endif

ifdef DEBUG
CXXFLAGS=-DDEBUG -g $(CXXBASEFLAGS) $(THREADFLAGS)
else
CXXFLAGS=-DNDEBUG -Os $(CXXBASEFLAGS) $(THREADFLAGS)
endif

CONFIGFLAGS:=$(shell pkg-config --cflags gtk+-2.0)
MARSHALLER=scintilla-marshal.o

.cxx.o:
	$(CC) $(CONFIGFLAGS) $(CXXFLAGS) -c $<
.c.o:
	$(CCOMP) $(CONFIGFLAGS) $(CXXFLAGS) -w -c $<

LEXOBJS:=$(addsuffix .o,$(basename $(notdir $(wildcard ../lexers/Lex*.cxx))))

all: $(COMPLIB)

clean:
	$(DEL) *.o $(COMPLIB)

deps:
	$(CC) -MM $(CONFIGFLAGS) $(CXXFLAGS) *.cxx ../src/*.cxx | sed -e 's/\/usr.* //' | grep [a-zA-Z] >deps.mak

$(COMPLIB): Accessor.o CharacterSet.o LexerBase.o LexerModule.o LexerSimple.o StyleContext.o WordList.o \
	CharClassify.o Decoration.o Document.o PerLine.o Catalogue.o CallTip.o \
	ScintillaBase.o ContractionState.o Editor.o ExternalLexer.o PropSetSimple.o PlatGTK.o \
	KeyMap.o LineMarker.o PositionCache.o ScintillaGTK.o CellBuffer.o ViewStyle.o \
	RESearch.o RunStyles.o Selection.o Style.o Indicator.o AutoComplete.o UniConversion.o XPM.o \
	$(MARSHALLER) $(LEXOBJS)
	$(AR) rc $@ $^
	$(RANLIB) $@

# Automatically generate header dependencies with "make deps"
include deps.mak
