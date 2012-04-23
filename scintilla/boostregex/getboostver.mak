
# Make file for building getboostver

DIR_O=.\obj
DIR_BIN=.\bin

CC=cl
RC=rc

CXXNDEBUG=-O1 -MT -DNDEBUG -GL -nologo
LDFLAGS=-OPT:REF -LTCG -nologo
LIBS=KERNEL32.lib USER32.lib 
CXXFLAGS=$(CXXNDEBUG)

!INCLUDE boostpath.mak

LDFLAGS=$(LDFLAGS)
CXXFLAGS=$(CXXFLAGS) -I$(BOOSTPATH)

# GDI32.lib IMM32.lib OLE32.LIB


ALL: clean $(DIR_BIN)\getboostver.exe

$(DIR_BIN)\getboostver.exe:: 
	$(CC) $(CXXFLAGS)	getboostver.cpp /link $(LDFLAGS) $(LIBS) /OUT:$(DIR_BIN)\getboostver.exe 

clean:
    -del /q $(DIR_BIN)\getboostver.exe
	