# Build the lexers test with Microsoft Visual C++ using nmake
# Tested with Visual C++ 2019

DEL = del /q
EXE = TestLexers.exe

INCLUDEDIRS = -I ../../scintilla/include -I ../include -I ../access

!IFDEF LEXILLA_STATIC
STATIC_FLAG = -D LEXILLA_STATIC
LIBS = ../bin/liblexilla.lib
!ENDIF

!IFDEF DEBUG
DEBUG_OPTIONS = -Od -MTd -DDEBUG $(STATIC_FLAG)
!ELSE
DEBUG_OPTIONS=-O1 -MT -DNDEBUG $(STATIC_FLAG) -GL
!ENDIF

CXXFLAGS = /EHsc /std:c++latest $(DEBUG_OPTIONS) $(INCLUDEDIRS)

OBJS = TestLexers.obj TestDocument.obj LexillaAccess.obj

all: $(EXE)

test: $(EXE)
	$(EXE)

clean:
	$(DEL) *.o *.obj *.exe

$(EXE): $(OBJS) $(LIBS)
	$(CXX) $(CXXFLAGS) $(LIBS) /Fe$@ $**

.cxx.obj::
	$(CXX) $(CXXFLAGS) -c $<
{..\access}.cxx.obj::
	$(CXX) $(CXXFLAGS) -c $(NAME) $<

.cxx.obj::
	$(CXX) $(CXXFLAGS) -c $<

TestLexers.obj: $*.cxx TestDocument.h
TestDocument.obj: $*.cxx $*.h
