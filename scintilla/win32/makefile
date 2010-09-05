# Make file for Scintilla on Windows
# Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
# The License.txt file describes the conditions under which this software may be distributed.
# This makefile assumes the mingw32 version of GCC 3.x or 4.x is used and changes will
# be needed to use other compilers.

.SUFFIXES: .cxx
CC = g++
DEL = del /q

COMPONENT = ../bin/Scintilla.dll
LEXCOMPONENT = ../bin/SciLexer.dll
LEXLIB = Lexers.a

vpath %.h ../src ../include ../lexlib
vpath %.cxx ../src ../lexlib ../lexers

LDFLAGS=-shared -static -Wl,--enable-runtime-pseudo-reloc-v2 -mno-cygwin -mwindows --relocatable -Wl,--add-stdcall-alias
LIBS=-lstdc++ -limm32 -lole32 -luuid
# Add -MMD to get dependencies
INCLUDEDIRS=-I ../include -I ../src -I../lexlib
CXXBASEFLAGS=-Wall -Wno-missing-braces -Wno-char-subscripts -Wno-strict-overflow -pedantic $(INCLUDEDIRS) -fno-rtti -mno-cygwin

ifdef DEBUG
CXXFLAGS=-DDEBUG -g $(CXXBASEFLAGS)
else
CXXFLAGS=-DNDEBUG -Os $(CXXBASEFLAGS)
STRIPFLAG=-s
endif

.cxx.o:
	$(CC) $(CXXFLAGS) -c $<

ALL:	$(COMPONENT) $(LEXCOMPONENT) $(LEXLIB) ScintillaWinS.o

clean:
	$(DEL) *.exe *.o *.obj *.dll *.res *.map

deps:
	$(CC) -MM $(CXXFLAGS) *.cxx ../src/*.cxx ../lexlib/*.cxx ../lexers/*.cxx >deps.mak

LEXOBJS:=$(addsuffix .o,$(basename $(notdir $(wildcard ../lexers/Lex*.cxx))))


BASEOBJS = \
	AutoComplete.o \
	CallTip.o \
	CellBuffer.o \
	CharacterSet.o \
	CharClassify.o \
	ContractionState.o \
	Decoration.o \
	Document.o \
	Editor.o \
	KeyMap.o \
	Indicator.o \
	LineMarker.o \
	PerLine.o \
	PlatWin.o \
	PositionCache.o \
	PropSetSimple.o \
	RESearch.o \
	RunStyles.o \
	ScintRes.o \
	Selection.o \
	Style.o \
	UniConversion.o \
	ViewStyle.o \
	XPM.o

SOBJS = ScintillaWin.o ScintillaBase.o $(BASEOBJS)

$(COMPONENT): $(SOBJS) Scintilla.def
	$(CC) $(LDFLAGS) -o $@ $(STRIPFLAG) $(SOBJS) $(CXXFLAGS) $(LIBS)

LOBJS = \
	Accessor.o \
	Catalogue.o \
	ExternalLexer.o \
	LexerBase.o \
	LexerModule.o \
	LexerSimple.o \
	ScintillaWinL.o \
	ScintillaBaseL.o \
	StyleContext.o \
	WordList.o \
	$(BASEOBJS) \
	$(LEXOBJS)
$(LEXCOMPONENT): $(LOBJS) Scintilla.def
	$(CC) $(LDFLAGS) -o $@ $(STRIPFLAG) $(LOBJS) $(CXXFLAGS) $(LIBS)

$(LEXLIB): $(LEXOBJS)
	$(AR) rc $@ $^
	ranlib $@

# Automatically generate dependencies for most files with "make deps"
include deps.mak

# These dependencies are maintained by hand as they do not use the default output name

ScintillaBaseL.o: ScintillaBase.cxx Platform.h \
 ILexer.h Scintilla.h SciLexer.h PropSetSimple.h \
 SplitVector.h Partitioning.h RunStyles.h \
 ContractionState.h CellBuffer.h CallTip.h \
 KeyMap.h Indicator.h XPM.h LineMarker.h \
 Style.h ViewStyle.h AutoComplete.h \
 CharClassify.h Decoration.h Document.h \
 Selection.h PositionCache.h Editor.h \
 ScintillaBase.h LexAccessor.h Accessor.h \
 LexerModule.h Catalogue.h

ScintillaWinL.o: ScintillaWin.cxx Platform.h \
 ILexer.h Scintilla.h SplitVector.h \
 Partitioning.h RunStyles.h ContractionState.h \
 CellBuffer.h CallTip.h KeyMap.h Indicator.h \
 XPM.h LineMarker.h Style.h AutoComplete.h \
 ViewStyle.h CharClassify.h Decoration.h \
 Document.h Selection.h PositionCache.h \
 Editor.h ScintillaBase.h UniConversion.h \
 LexAccessor.h Accessor.h \
 LexerModule.h Catalogue.h

ScintillaWinS.o: ScintillaWin.cxx Platform.h \
 ILexer.h Scintilla.h SplitVector.h \
 Partitioning.h RunStyles.h ContractionState.h \
 CellBuffer.h CallTip.h KeyMap.h Indicator.h \
 XPM.h LineMarker.h Style.h AutoComplete.h \
 ViewStyle.h CharClassify.h Decoration.h \
 Document.h Selection.h PositionCache.h \
 Editor.h ScintillaBase.h UniConversion.h

ScintillaBaseL.o:
	$(CC) $(CXXFLAGS) -D SCI_LEXER -c $< -o $@

ScintillaWinS.o:
	$(CC) $(CXXFLAGS) -D STATIC_BUILD -c $< -o $@

ScintillaWinL.o:
	$(CC) $(CXXFLAGS) -D SCI_LEXER -c $< -o $@

ScintRes.o:	ScintRes.rc PlatformRes.h
	windres ScintRes.rc $@

