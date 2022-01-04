.PHONY: all check clean

INCLUDES = -I ../../../scintilla/include -I ../../include -I ../../lexlib

BASE_FLAGS += --std=c++17 -shared

ifdef windir
    SHAREDEXTENSION = dll
else
    ifeq ($(shell uname),Darwin)
        SHAREDEXTENSION = dylib
        BASE_FLAGS += -dynamiclib -arch arm64 -arch x86_64
    else
        BASE_FLAGS += -fPIC
        SHAREDEXTENSION = so
    endif
    BASE_FLAGS += -fvisibility=hidden
endif

ifdef windir
	RM = $(if $(wildcard $(dir $(SHELL))rm.exe), $(dir $(SHELL))rm.exe -f, del /q)
	CXX = g++
endif

LIBRARY = SimpleLexer.$(SHAREDEXTENSION)
LEXLIB = ../../lexlib/*.cxx

all: $(LIBRARY)

# make check requires CheckLexilla to have already been built
check: $(LIBRARY)
	../CheckLexilla/CheckLexilla ./$(LIBRARY)

clean:
	$(RM) *.o *obj *.lib *.exp $(LIBRARY)

$(LIBRARY): *.cxx
	$(CXX) $(INCLUDES) $(BASE_FLAGS) $(CPPFLAGS) $(CXXFLAGS) $^ $(LEXLIB) $(LIBS) $(LDLIBS) -o $@
