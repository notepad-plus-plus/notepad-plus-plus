# Build all the unit tests with Microsoft Visual C++ using nmake
# Tested with Visual C++ 2019

DEL = del /q
EXE = unitTest.exe

INCLUDEDIRS = /I../../include /I../../src

CXXFLAGS = /MP /EHsc /std:c++17 $(OPTIMIZATION) /nologo /D_HAS_AUTO_PTR_ETC=1 /wd 4805 $(INCLUDEDIRS)

# Files in this directory containing tests
TESTSRC=test*.cxx
# Files being tested from scintilla/src directory
TESTEDSRC=\
 ../../src/CaseConvert.cxx \
 ../../src/CaseFolder.cxx \
 ../../src/CellBuffer.cxx \
 ../../src/ChangeHistory.cxx \
 ../../src/CharacterCategoryMap.cxx \
 ../../src/CharClassify.cxx \
 ../../src/ContractionState.cxx \
 ../../src/Decoration.cxx \
 ../../src/Document.cxx \
 ../../src/Geometry.cxx \
 ../../src/PerLine.cxx \
 ../../src/RESearch.cxx \
 ../../src/RunStyles.cxx \
 ../../src/UndoHistory.cxx \
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
