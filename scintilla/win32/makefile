# Make file for Scintilla on Windows
# @file makefile
# Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
# The License.txt file describes the conditions under which this software may be distributed.
# This makefile assumes Mingw-w64 GCC 9.0+ is used and changes will be needed to use other compilers.
# Clang 9.0+ can be used with CLANG=1 on command line.

.PHONY: all clean analyze depend

.SUFFIXES: .cxx .c .o .h .a

DIR_O=.
DIR_BIN=../bin

COMPONENT = $(DIR_BIN)/Scintilla.dll
LIBSCI = $(DIR_BIN)/libscintilla.a

WARNINGS = -Wpedantic -Wall

ifdef CLANG
CXX = clang++
DEFINES += -D_CRT_SECURE_NO_DEPRECATE
else
# MinGW GCC
LIBSMINGW = -lstdc++
STRIPOPTION = -s
endif
ARFLAGS = rc
RANLIB ?= ranlib
WINDRES ?= windres

# Environment variable windir always defined on Win32

# Take care of changing Unix style '/' directory separator to '\' on Windows
normalize = $(if $(windir),$(subst /,\,$1),$1)

PYTHON = $(if $(windir),pyw,python3)

ifdef windir
DEL = $(if $(wildcard $(dir $(SHELL))rm.exe), $(dir $(SHELL))rm.exe -f, del /q)
else
DEL = rm -f
endif

vpath %.h ../src ../include
vpath %.cxx ../src

LDFLAGS=-shared -static -mwindows
LIBS=-lgdi32 -luser32 -limm32 -lole32 -luuid -loleaut32 -lmsimg32 $(LIBSMINGW)

INCLUDES=-I ../include -I ../src

BASE_FLAGS += $(WARNINGS)

ifdef NO_CXX11_REGEX
DEFINES += -DNO_CXX11_REGEX
endif

DEFINES += -D$(if $(DEBUG),DEBUG,NDEBUG)
BASE_FLAGS += $(if $(DEBUG),-g,-Os)

ifndef DEBUG
STRIPFLAG=$(STRIPOPTION)
endif

CXX_BASE_FLAGS =--std=c++17 $(BASE_FLAGS)
CXX_ALL_FLAGS =$(DEFINES) $(INCLUDES) $(CXX_BASE_FLAGS)

all:	$(COMPONENT) $(LIBSCI)

clean:
	$(DEL) $(call normalize, $(addprefix $(DIR_O)/, *.exe *.o *.a *.obj *.dll *.res *.map *.plist) $(COMPONENT) $(LIBSCI))

$(DIR_O)/%.o: %.cxx
	$(CXX) $(CXX_ALL_FLAGS) $(CXXFLAGS) -c $< -o $@

analyze:
	$(CXX) --analyze $(CXX_ALL_FLAGS) $(CXXFLAGS) *.cxx ../src/*.cxx

depend deps.mak:
	$(PYTHON) DepGen.py

# Required for base Scintilla
SRC_OBJS = \
	$(DIR_O)/AutoComplete.o \
	$(DIR_O)/CallTip.o \
	$(DIR_O)/CaseConvert.o \
	$(DIR_O)/CaseFolder.o \
	$(DIR_O)/CellBuffer.o \
	$(DIR_O)/CharacterCategoryMap.o \
	$(DIR_O)/CharacterType.o \
	$(DIR_O)/CharClassify.o \
	$(DIR_O)/ContractionState.o \
	$(DIR_O)/DBCS.o \
	$(DIR_O)/Decoration.o \
	$(DIR_O)/Document.o \
	$(DIR_O)/EditModel.o \
	$(DIR_O)/Editor.o \
	$(DIR_O)/EditView.o \
	$(DIR_O)/Geometry.o \
	$(DIR_O)/Indicator.o \
	$(DIR_O)/KeyMap.o \
	$(DIR_O)/LineMarker.o \
	$(DIR_O)/MarginView.o \
	$(DIR_O)/PerLine.o \
	$(DIR_O)/PositionCache.o \
	$(DIR_O)/RESearch.o \
	$(DIR_O)/RunStyles.o \
	$(DIR_O)/Selection.o \
	$(DIR_O)/Style.o \
	$(DIR_O)/UniConversion.o \
	$(DIR_O)/UniqueString.o \
	$(DIR_O)/ViewStyle.o \
	$(DIR_O)/XPM.o

COMPONENT_OBJS = \
	$(SRC_OBJS) \
	$(DIR_O)/HanjaDic.o \
	$(DIR_O)/PlatWin.o \
	$(DIR_O)/ScintillaBase.o \
	$(DIR_O)/ScintillaWin.o

SHARED_OBJS = \
	$(DIR_O)/ScintillaDLL.o \
	$(DIR_O)/ScintRes.o

$(COMPONENT): $(COMPONENT_OBJS) $(SHARED_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(STRIPFLAG) $^ $(CXXFLAGS) $(LIBS)

$(LIBSCI): $(COMPONENT_OBJS)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

# Automatically generate dependencies for most files with "make depend"
include deps.mak

$(DIR_O)/ScintRes.o: ScintRes.rc
	$(WINDRES) $< $@

