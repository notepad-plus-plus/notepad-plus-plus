# Build all the unit tests with Microsoft Visual C++ using nmake
# Tested with Visual C++ 2010 and 2013

DEL = del /q
EXE = unitTest.exe

INCLUDEDIRS = /I../../include /I../../src /I../../lexlib

CXXFLAGS = /EHsc /std:c++17 /D_HAS_AUTO_PTR_ETC=1 /wd 4805 $(INCLUDEDIRS)

# Files in this directory containing tests
TESTSRC=test*.cxx
# Files being tested from scintilla/src directory
TESTEDSRC=\
 ../../lexlib/WordList.cxx \
 ../../src/CellBuffer.cxx \
 ../../src/CharClassify.cxx \
 ../../src/ContractionState.cxx \
 ../../src/Decoration.cxx \
 ../../src/RunStyles.cxx \
 ../../src/UniConversion.cxx \
 ../../src/UniqueString.cxx

TESTS=$(EXE)

all: $(TESTS)

test: $(TESTS)
	$(EXE)

clean:
	$(DEL) $(TESTS) *.o *.obj *.exe

$(EXE): $(TESTSRC) $(TESTEDSRC) $(@B).obj
	$(CXX) $(CXXFLAGS) /Fe$@ $**
