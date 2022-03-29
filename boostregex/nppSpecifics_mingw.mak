# This makefile should be included in the main scintilla.mak file,
# just after where LOBJS is defined (around line
#
# The following line should be added around line 211 of scintilla.mak
# !INCLUDE nppSpecifics.mak

# Set your boost path (the root of where you've unpacked your boost zip).
# Boost is available from www.boost.org


SRC_OBJS+=\
	$(DIR_O)/BoostRegexSearch.o \
	$(DIR_O)/UTF8DocumentIterator.o

INCLUDES+= -I../../boostregex

CXXFLAGS+= -DSCI_OWNREGEX


$(DIR_O)/UTF8DocumentIterator.o:: ../../boostregex/UTF8DocumentIterator.cxx
	$(CXX) $(CXX_ALL_FLAGS) $(CXXFLAGS) -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS -c $< -o $@

$(DIR_O)/BoostRegexSearch.o:: ../../boostregex/BoostRegexSearch.cxx ../src/CharClassify.h ../src/RESearch.h
	$(CXX) $(CXX_ALL_FLAGS) $(CXXFLAGS) -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS -c $< -o $@

