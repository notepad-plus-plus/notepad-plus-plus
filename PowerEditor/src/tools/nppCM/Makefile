TARGETOS = BOTH
!include <win32.mak>

lflags  = /NODEFAULTLIB /INCREMENTAL:NO /RELEASE /NOLOGO
dlllflags = $(lflags) -entry:_DllMainCRTStartup$(DLLENTRY) -dll

all: nppcm.dll

nppcm.dll: nppcm.obj nppcm.res
	$(implib) -machine:$(CPU) -def:nppcm.def $** -out:nppcm.lib
	$(link) $(dlllflags) -base:0x1C000000 -out:$*.dll $** $(olelibsdll) shell32.lib nppcm.lib comctl32.lib nppcm.exp

.cpp.obj:
	$(cc) $(cflags) $(cvarsdll) $*.cpp

nppcm.res: nppcm.rc
	$(rc) $(rcflags) $(rcvars) nppcm.rc

clean:
	-1 del nppcm.dll nppcm.obj nppcm.exp nppcm.res

zip:
	-1 del *.zip
	perl abpack.pl