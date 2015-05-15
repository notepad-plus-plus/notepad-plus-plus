# Build all the unit tests
# Should be run using mingw32-make on Windows

.SUFFIXES: .cxx

GTEST_DIR = ../../../../gtest-1.5.0

ifdef windir

DEL = del /q
# Find Google Test headers.
CPPFLAGS += -I$(GTEST_DIR)/include
GTEST_ALL =  gtest-all.o
LINKFLAGS = $(CPPFLAGS) $(CXXFLAGS)
EXE = unitTest.exe

else

DEL = rm -f
CPPFLAGS = $(shell gtest-config --cppflags)
CXXFLAGS = $(shell gtest-config --cxxflags)
LINKFLAGS = $(shell gtest-config --ldflags --libs)
EXE = unitTest

# For coverage testing with gcov
#CPPFLAGS += -fprofile-arcs -ftest-coverage
#LINKFLAGS += -fprofile-arcs -ftest-coverage

endif

#vpath %.cxx ../src ../lexlib ../lexers
vpath %.cxx ../../src


INCLUDEDIRS = -I ../../include -I ../../src -I../../lexlib


# Find headers of test code.
CPPFLAGS += $(INCLUDEDIRS)

CXXFLAGS += -g -Wall -Wextra -Wno-unused-function
#~ CXXFLAGS += -g -Wall

CASES:=$(addsuffix .o,$(basename $(notdir $(wildcard test*.cxx))))
TESTEDOBJS=ContractionState.o RunStyles.o CharClassify.o

TESTS=$(EXE)

GTEST_HEADERS=$(GTEST_DIR)/include/gtest/*.h $(GTEST_DIR)/include/gtest/internal/*.h

all: $(TESTS)

clean:
	$(DEL) $(TESTS) *.a *.o *.exe *.gcov *.gcda *.gcno

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

gtest-all.o: $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -I$(GTEST_DIR) -c \
            $(GTEST_DIR)/src/gtest-all.cc

.cxx.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS)  -c $<

$(EXE): $(CASES) $(TESTEDOBJS) unitTest.o $(GTEST_ALL)
	$(CXX) $(LINKFLAGS) $^ -o $@
