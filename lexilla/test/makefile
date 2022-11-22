# Build all the lexer tests using GNU make and either g++ or Clang
# @file makefile
# Copyright 2019 by Neil Hodgson <neilh@scintilla.org>
# The License.txt file describes the conditions under which this software may be distributed.
# Should be run using mingw32-make on Windows, not nmake
# On Windows g++ is used, on macOS clang, and on Linux g++ is used by default
# but clang can be used by defining CLANG when invoking make
# clang works only with libc++, not libstdc++

.PHONY: all test clean

.SUFFIXES: .cxx

WARNINGS = -Wpedantic -Wall -Wextra

ifndef windir
LIBS += -ldl
ifeq ($(shell uname),Darwin)
# On macOS always use Clang
CLANG = 1
endif
endif

EXE = $(if $(windir),TestLexers.exe,TestLexers)

BASE_FLAGS += --std=c++2a

ifdef CLANG
    CXX = clang++
    BASE_FLAGS += -fsanitize=address
endif

ifdef LEXILLA_STATIC
    DEFINES += -D LEXILLA_STATIC
    LIBS += ../bin/liblexilla.a
endif

ifdef windir
    DEL = $(if $(wildcard $(dir $(SHELL))rm.exe), $(dir $(SHELL))rm.exe -f, del /q)
else
    DEL = rm -f
endif

vpath %.cxx ../access

DEFINES += -D$(if $(DEBUG),DEBUG,NDEBUG)
BASE_FLAGS += $(if $(DEBUG),-g,-Os)

INCLUDES = -I ../../scintilla/include -I ../include -I ../access
BASE_FLAGS += $(WARNINGS)

all: $(EXE)

test: $(EXE)
	./$(EXE)

clean:
	$(DEL) *.o *.obj $(EXE)

%.o: %.cxx
	$(CXX) $(DEFINES) $(INCLUDES) $(BASE_FLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

OBJS = TestLexers.o TestDocument.o LexillaAccess.o

$(EXE): $(OBJS)
	$(CXX) $(BASE_FLAGS) $(CPPFLAGS) $(CXXFLAGS) $^ $(LIBS) $(LDLIBS) -o $@

TestLexers.o: TestLexers.cxx TestDocument.h
TestDocument.o: TestDocument.cxx TestDocument.h
