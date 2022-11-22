# Build all the unit tests using GNU make and either g++ or clang
# Should be run using mingw32-make on Windows, not nmake
# On Windows g++ is used, on macOS clang, and on Linux G++ is used by default
# but clang can be used by defining CLANG when invoking make
# clang works only with libc++, not libstdc++
# Tested with clang 9 and g++ 9

CXXSTD=c++17

ifndef windir
ifeq ($(shell uname),Darwin)
# On macOS always use clang as g++ is old version
CLANG = 1
USELIBCPP = 1
endif
endif

CXXFLAGS += --std=$(CXXSTD)

ifdef CLANG
CXX = clang++
ifdef USELIBCPP
# macOS, use libc++ but don't have sanitizers
CXXFLAGS += --stdlib=libc++
LINKFLAGS = -lc++
else
# Linux, have sanitizers
SANITIZE = -fsanitize=address,undefined
CXXFLAGS += $(SANITIZE)
endif
else
CXX = g++
endif

ifdef windir
DEL = del /q
EXE = unitTest.exe
else
DEL = rm -f
EXE = unitTest
endif

INCLUDEDIRS = -I ../../include -I ../../src -I../../lexlib -I../../../scintilla/include

CPPFLAGS += $(INCLUDEDIRS)
CXXFLAGS += -Wall -Wextra

# Files in this directory containing tests
TESTSRC=test*.cxx
# Files being tested from scintilla/src directory
TESTEDSRC=\
 ../../lexlib/Accessor.cxx \
 ../../lexlib/CharacterSet.cxx \
 ../../lexlib/LexerBase.cxx \
 ../../lexlib/LexerModule.cxx \
 ../../lexlib/LexerSimple.cxx \
 ../../lexlib/PropSetSimple.cxx \
 ../../lexlib/WordList.cxx

TESTS=$(EXE)

all: $(TESTS)

test: $(TESTS)
	./$(EXE)

clean:
	$(DEL) $(TESTS) *.o *.obj *.exe

$(EXE): $(TESTSRC) $(TESTEDSRC) unitTest.cxx
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LINKFLAGS) $^ -o $@
