# Make file for Scintilla on Linux, macOS, or Windows
# @file makefile
# Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
# The License.txt file describes the conditions under which this software may be distributed.
# This makefile assumes GCC 9.0+ is used and changes will be needed to use other compilers.
# Clang 9.0+ can be used with CLANG=1 on command line.
# Builds for GTK+ 2 and 3. GTK 3 requires GTK3=1 on command line.
# Also works with ming32-make on Windows.

.PHONY: static shared all clean analyze depend

.SUFFIXES: .cxx .c .o .h .a .list

srcdir ?= .
basedir = $(srcdir)/..

WARNINGS = -Wpedantic -Wall
ifdef CLANG
CXX = clang++
CC = clang
WARNINGS += -Wno-deprecated-register
ifdef windir
# Turn off some warnings that occur when Clang is being used on Windows where it
# is including Microsoft headers.
# incompatible-ms-struct is because more complex structs are not quite the same as MSVC
WARNINGS += -Wno-incompatible-ms-struct
# language-extension-token is because of __int64 in glib-2.0 glibconfig.h
WARNINGS += -Wno-language-extension-token
# register may be used in glib
# This produces a warning since -Wno-register is not valid for C files but it still works
WARNINGS += -Wno-register
endif
# Can choose aspect to sanitize: address and undefined can simply change SANITIZE but for
# thread also need to create Position Independent Executable -> search online documentation
SANITIZE = address
#SANITIZE = undefined
BASE_FLAGS += -fsanitize=$(SANITIZE)
endif
ARFLAGS = rc
RANLIB ?= ranlib
PKG_CONFIG ?= pkg-config

GTK_VERSION = $(if $(GTK3),gtk+-3.0,gtk+-2.0)

# Environment variable windir always defined on Win32

# Enable Position Independent Code except on Windows where it is the default so the flag produces a warning
ifndef windir
BASE_FLAGS += -fPIC
ifeq ($(shell uname),Darwin)
LDFLAGS += -dynamiclib
endif
endif

LDFLAGS += -shared

# Take care of changing Unix style '/' directory separator to '\' on Windows
normalize = $(if $(windir),$(subst /,\,$1),$1)

PYTHON = $(if $(windir),pyw,python3)

SHAREDEXTENSION = $(if $(windir),dll,so)

ifdef windir
CC = gcc
DEL = del /q
else
DEL = rm -f
endif
COMPLIB=$(basedir)/bin/scintilla.a
COMPONENT=$(basedir)/bin/libscintilla.$(SHAREDEXTENSION)

vpath %.h $(srcdir) $(basedir)/src $(basedir)/include
vpath %.c $(srcdir)
vpath %.cxx $(srcdir) $(basedir)/src

INCLUDES=-I $(basedir)/include -I $(basedir)/src
DEFINES += -DGTK
BASE_FLAGS += $(WARNINGS)

ifdef NO_CXX11_REGEX
DEFINES += -DNO_CXX11_REGEX
endif

DEFINES += -D$(if $(DEBUG),DEBUG,NDEBUG)
BASE_FLAGS += $(if $(DEBUG),-g,-O3)

CXX_BASE_FLAGS =--std=c++17 $(BASE_FLAGS)
CXX_ALL_FLAGS =$(DEFINES) $(INCLUDES) $(CXX_BASE_FLAGS) $(CONFIG_FLAGS)

CONFIG_FLAGS:=$(shell $(PKG_CONFIG) --cflags $(GTK_VERSION))
CONFIGLIB:=$(shell $(PKG_CONFIG) --libs $(GTK_VERSION) gmodule-no-export-2.0)
MARSHALLER=scintilla-marshal.o

all: $(COMPLIB) $(COMPONENT)

static: $(COMPLIB)

shared: $(COMPONENT)

clean:
	$(DEL) *.o $(call normalize,$(COMPLIB)) $(call normalize,$(COMPONENT)) *.plist

%.o: %.cxx
	$(CXX) $(CPPFLAGS) $(CXX_ALL_FLAGS) $(CXXFLAGS) -c $<
%.o: %.c
	$(CC) $(CPPFLAGS) $(DEFINES) $(INCLUDES) $(CONFIG_FLAGS) $(BASE_FLAGS) $(CFLAGS) -w -c $<

GLIB_GENMARSHAL = glib-genmarshal
GLIB_GENMARSHAL_FLAGS = --prefix=scintilla_marshal

%.h: %.list
	$(GLIB_GENMARSHAL) --header $(GLIB_GENMARSHAL_FLAGS) $< > $@
%.c: %.list
	$(GLIB_GENMARSHAL) --body $(GLIB_GENMARSHAL_FLAGS) $< > $@

analyze:
	clang --analyze $(DEFINES) $(INCLUDES) $(CONFIG_FLAGS) $(CXX_BASE_FLAGS) $(CXXFLAGS) $(srcdir)/*.cxx $(basedir)/src/*.cxx

depend deps.mak:
	$(PYTHON) DepGen.py

# Required for base Scintilla
SRC_OBJS = \
	AutoComplete.o \
	CallTip.o \
	CaseConvert.o \
	CaseFolder.o \
	CellBuffer.o \
	ChangeHistory.o \
	CharacterCategoryMap.o \
	CharacterType.o \
	CharClassify.o \
	ContractionState.o \
	DBCS.o \
	Decoration.o \
	Document.o \
	EditModel.o \
	Editor.o \
	EditView.o \
	Geometry.o \
	Indicator.o \
	KeyMap.o \
	LineMarker.o \
	MarginView.o \
	PerLine.o \
	PositionCache.o \
	RESearch.o \
	RunStyles.o \
	Selection.o \
	Style.o \
	UndoHistory.o \
	UniConversion.o \
	UniqueString.o \
	ViewStyle.o \
	XPM.o

GTK_OBJS = \
	ScintillaBase.o \
	PlatGTK.o \
	ScintillaGTK.o \
	ScintillaGTKAccessible.o

$(COMPLIB): $(SRC_OBJS) $(GTK_OBJS) $(MARSHALLER)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

$(COMPONENT): $(SRC_OBJS) $(GTK_OBJS) $(MARSHALLER)
	$(CXX) $(CXX_ALL_FLAGS) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(CONFIGLIB)

# Automatically generate header dependencies with "make depend"
include deps.mak
