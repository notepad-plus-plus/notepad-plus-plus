# Build all the unit tests with Microsoft Visual C++ using nmake
# Tested with Visual C++ 2019

DEL = del /q
EXE = unitTest.exe

INCLUDEDIRS = /I../../include /I../../src /I../../lexlib /I../../../scintilla/include

CXXFLAGS = /EHsc /std:c++17 /D_HAS_AUTO_PTR_ETC=1 /wd 4805 $(INCLUDEDIRS)

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
	$(EXE)

clean:
	$(DEL) $(TESTS) *.o *.obj *.exe

$(EXE): $(TESTSRC) $(TESTEDSRC) $(@B).obj
	$(CXX) $(CXXFLAGS) /Fe$@ $**
