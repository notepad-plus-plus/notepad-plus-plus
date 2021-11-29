# Make file for Lexilla
# @file makefile
# Copyright 2019 by Neil Hodgson <neilh@scintilla.org>
# The License.txt file describes the conditions under which this software may be distributed.
# This works on Windows or Linux using GCC 9.0+
# This works on Windows, Linux, or macOS using Clang 9.0+
# On Windows, it is tested with Mingw-w64 GCC and Clang.
# on macOS, it always uses Clang
# For debug versions define DEBUG on the command line:
#     make DEBUG=1
# On Windows, to build with MSVC, run lexilla.mak

.PHONY: all clean analyze depend

.SUFFIXES: .cxx

DIR_BIN=../../bin

WARNINGS = -Wpedantic -Wall -Wextra

ifdef windir
    SHARED_NAME = lexilla
    SHAREDEXTENSION = dll
    WINDRES ?= windres
    VERSION_RESOURCE = LexillaVersion.o
else
    SHARED_NAME = liblexilla
    ifeq ($(shell uname),Darwin)
        CLANG := 1
        LDFLAGS += -dynamiclib
        SHAREDEXTENSION = dylib
    else
        SHAREDEXTENSION = so
    endif
    BASE_FLAGS += -fvisibility=hidden
endif

LEXILLA=$(DIR_BIN)/$(SHARED_NAME).$(SHAREDEXTENSION)
LIBLEXILLA=$(DIR_BIN)/liblexilla.a

BASE_FLAGS += --std=c++17

ifdef CLANG
CXX = clang++
ifdef windir
# Clang on Win32 uses MSVC headers so will complain about strcpy without this
DEFINES += -D_CRT_SECURE_NO_DEPRECATE=1
endif
endif

ifdef windir
    LDFLAGS += -mwindows
else
    BASE_FLAGS += -fPIC
endif

# Take care of changing Unix style '/' directory separator to '\' on Windows
normalize = $(if $(windir),$(subst /,\,$1),$1)

PYTHON = $(if $(windir),pyw,python3)

ifdef windir
    DEL = $(if $(wildcard $(dir $(SHELL))rm.exe), $(dir $(SHELL))rm.exe -f, del /q)
else
    DEL = rm -f
endif

RANLIB ?= ranlib

vpath %.h ../../src ../../include ../../lexlib
vpath %.cxx ../../src ../../lexlib ../../lexers

DEFINES += -D$(if $(DEBUG),DEBUG,NDEBUG)
BASE_FLAGS += $(if $(DEBUG),-g,-Os)

INCLUDES = -I ../../include -I ../../src -I ../../lexlib
LDFLAGS += -shared

BASE_FLAGS += $(WARNINGS)

all:	$(LEXILLA) $(LIBLEXILLA)

clean:
	$(DEL) *.o *.obj *.a *.res *.map *.plist $(call normalize,$(LEXILLA) $(LIBLEXILLA))

%.o: %.cxx
	$(CXX) $(DEFINES) $(INCLUDES) $(BASE_FLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

%.o:	%.rc
	$(WINDRES) $< $@

analyze:
	$(CXX) --analyze $(DEFINES) $(INCLUDES) $(BASE_FLAGS) $(CXXFLAGS) *.cxx ../../lexlib/*.cxx ../../lexers/*.cxx

depend deps.mak:
	$(PYTHON) DepGen.py

LEXERS:=$(sort $(notdir $(wildcard ../../lexers/Lex*.cxx)))

OBJS = Lexilla.o

# Required by lexers
LEXLIB_OBJS=\
	Accessor.o \
	CharacterCategory.o \
	CharacterSet.o \
	DefaultLexer.o \
	LexerBase.o \
	LexerModule.o \
	LexerSimple.o \
	PropSetSimple.o \
	StyleContext.o \
	WordList.o

# Required by libraries and DLLs that include lexing
LEXILLA_OBJS=\
	$(OBJS) \
	$(LEXLIB_OBJS) \
	$(LEXERS:.cxx=.o)

$(LEXILLA): $(LEXILLA_OBJS) $(VERSION_RESOURCE)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

$(LIBLEXILLA):  $(LEXILLA_OBJS)
	$(AR) rc $@ $^
	$(RANLIB) $@

# Automatically generate dependencies for most files with "make deps"
include deps.mak
