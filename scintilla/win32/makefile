# Make file for Scintilla on Windows
# Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
# The License.txt file describes the conditions under which this software may be distributed.
# This makefile assumes the Mingw-w64 version of GCC 7.1+ is used and changes will
# be needed to use other compilers.

.SUFFIXES: .cxx

DIR_BIN=../bin

COMPONENT = $(DIR_BIN)/Scintilla.dll
LEXCOMPONENT = $(DIR_BIN)/SciLexer.dll
LIBSCI = $(DIR_BIN)/libscintilla.a

ifdef CLANG
CXX = clang++
# Clang doesn't like omitting braces in array initialization but they just add noise,
# __uuidof is a Microsoft extension but makes COM code neater,
# public visibility avoids warnings like 'locally defined symbol __std_terminate imported'
CLANG_OPTS = -Wno-missing-braces -Wno-language-extension-token -Xclang -flto-visibility-public-std
else
# MinGW GCC
LDMINGW = -Wl,--enable-runtime-pseudo-reloc-v2 -Wl,--add-stdcall-alias
LIBSMINGW = -lstdc++
STRIPOPTION = -s
endif

CXXSTD = c++17

ifeq ($(OS),Windows_NT)
DEL = $(if $(wildcard $(dir $(SHELL))rm.exe), $(dir $(SHELL))rm.exe -f, del /q)
else
DEL = rm -f
endif
RANLIB ?= ranlib
WINDRES ?= windres

vpath %.h ../src ../include ../lexlib
vpath %.cxx ../src ../lexlib ../lexers

LDFLAGS=-shared -static -mwindows $(LDMINGW)
LIBS=-lgdi32 -luser32 -limm32 -lole32 -luuid -loleaut32 -lmsimg32 $(LIBSMINGW)
# Add -MMD to get dependencies
INCLUDEDIRS=-I ../include -I ../src -I../lexlib

CXXBASEFLAGS=--std=$(CXXSTD) -Wall -pedantic $(INCLUDEDIRS) -D_CRT_SECURE_NO_DEPRECATE=1 $(CLANG_OPTS)

ifdef NO_CXX11_REGEX
REFLAGS=-DNO_CXX11_REGEX
endif

ifdef DEBUG
CXXFLAGS=-DDEBUG -g $(CXXBASEFLAGS)
else
CXXFLAGS=-DNDEBUG -Os $(CXXBASEFLAGS)
STRIPFLAG=$(STRIPOPTION)
endif

all:	$(COMPONENT) $(LEXCOMPONENT) $(LIBSCI)

clean:
	$(DEL) *.exe *.o *.a *.obj *.dll *.res *.map *.plist

.cxx.o:
	$(CXX) $(CXXFLAGS) $(REFLAGS) -c $<

analyze:
	$(CXX) --analyze $(CXXFLAGS) *.cxx ../src/*.cxx ../lexlib/*.cxx ../lexers/*.cxx

depend deps.mak:
	python DepGen.py

LEXOBJS:=$(addsuffix .o,$(basename $(sort $(notdir $(wildcard ../lexers/Lex*.cxx)))))

# Required for base Scintilla
BASEOBJS = \
	AutoComplete.o \
	CallTip.o \
	CaseConvert.o \
	CaseFolder.o \
	CellBuffer.o \
	CharacterCategory.o \
	CharacterSet.o \
	CharClassify.o \
	ContractionState.o \
	DBCS.o \
	Decoration.o \
	Document.o \
	EditModel.o \
	Editor.o \
	EditView.o \
	KeyMap.o \
	Indicator.o \
	LineMarker.o \
	MarginView.o \
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
	UniqueString.o \
	ViewStyle.o \
	XPM.o \
	HanjaDic.o

SOBJS = ScintillaDLL.o ScintillaWin.o ScintillaBase.o $(BASEOBJS)

# Required by lexers
LEXLIBOBJS=\
	Accessor.o \
	Catalogue.o \
	ExternalLexer.o \
	DefaultLexer.o \
	LexerBase.o \
	LexerModule.o \
	LexerSimple.o \
	StyleContext.o \
	WordList.o \

# Required by libraries and DLLs that include lexing
SCILEXOBJS=\
	$(BASEOBJS) \
	$(LEXLIBOBJS) \
	$(LEXOBJS) \
	ScintillaBaseL.o

$(COMPONENT): $(SOBJS) Scintilla.def
	$(CXX) $(LDFLAGS) -o $@ $(STRIPFLAG) $(SOBJS) $(CXXFLAGS) $(LIBS)

$(LEXCOMPONENT): ScintillaDLL.o ScintillaWinL.o $(SCILEXOBJS) Scintilla.def
	$(CXX) $(LDFLAGS) -o $@ $(STRIPFLAG) ScintillaDLL.o ScintillaWinL.o $(SCILEXOBJS) $(CXXFLAGS) $(LIBS)

$(LIBSCI): ScintillaWin.o $(SCILEXOBJS)
	$(AR) rc $@ $^
	$(RANLIB) $@

# Automatically generate dependencies for most files with "make deps"
include deps.mak

ScintillaBaseL.o:
	$(CXX) $(CXXFLAGS) -D SCI_LEXER -c $< -o $@

ScintillaWinL.o:
	$(CXX) $(CXXFLAGS) -D SCI_LEXER -c $< -o $@

ScintRes.o:	ScintRes.rc
	$(WINDRES) ScintRes.rc $@

