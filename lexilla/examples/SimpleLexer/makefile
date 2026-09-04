.PHONY: all check clean

INCLUDES = -I ../../../scintilla/include -I ../../include -I ../../lexlib

BASE_FLAGS += --std=c++17

ifdef windir
    SHAREDEXTENSION = dll
else
    ifeq ($(shell uname),Darwin)
        SHAREDEXTENSION = dylib
        BASE_FLAGS += -arch arm64 -arch x86_64
        LINK_FLAGS += -dynamiclib
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

vpath %.cxx ../../lexlib
 
LEXLIB_SOURCES := $(sort $(notdir $(wildcard ../../lexlib/*.cxx)))
LEXLIB = $(LEXLIB_SOURCES:.cxx=.o)

%.o: %.cxx
	$(CXX) $(INCLUDES) $(BASE_FLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

all: $(LIBRARY)

# make check requires CheckLexilla to have already been built
check: $(LIBRARY)
	../CheckLexilla/CheckLexilla ./$(LIBRARY)

clean:
	$(RM) *.o *obj *.lib *.exp $(LIBRARY)

$(LIBRARY): $(LEXLIB) *.cxx
	$(CXX) $(INCLUDES) $(LINK_FLAGS) $(BASE_FLAGS) -shared $(CPPFLAGS) $(CXXFLAGS) $^ $(LIBS) $(LDLIBS) -o $@
