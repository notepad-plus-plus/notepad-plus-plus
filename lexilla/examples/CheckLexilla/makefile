.PHONY: all check clean

INCLUDES = -I ../../include
EXE = $(if $(windir),CheckLexilla.exe,CheckLexilla)

ifdef windir
	RM = $(if $(wildcard $(dir $(SHELL))rm.exe), $(dir $(SHELL))rm.exe -f, del /q)
	CC = gcc
else
	LIBS += -ldl
endif

all: $(EXE)

check: $(EXE)
	./$(EXE)

clean:
	$(RM) $(EXE)

$(EXE): *.c
	$(CC) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) $^ $(LIBS) $(LDLIBS) -o $@
