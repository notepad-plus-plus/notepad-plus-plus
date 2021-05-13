# This makefile should be included in the main scintilla.mak file,
# just after where LOBJS is defined (around line 
# 
# The following line should be added around line 211 of scintilla.mak
# !INCLUDE nppSpecifics.mak

# Set your boost path (the root of where you've unpacked your boost zip).
# Boost is available from www.boost.org


SRC_OBJS+=\
	BoostRegexSearch.o \
	UTF8DocumentIterator.o

INCLUDES+= -I../../boostregex

CXXFLAGS+= -DSCI_OWNREGEX


UTF8DocumentIterator.o:: ../../boostregex/UTF8DocumentIterator.cxx
	$(CXX) $(CXX_ALL_FLAGS) $(CXXFLAGS) -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS -c ../../boostregex/UTF8DocumentIterator.cxx	

BoostRegexSearch.o:: ../../boostregex/BoostRegexSearch.cxx ../src/CharClassify.h ../src/RESearch.h	
	$(CXX) $(CXX_ALL_FLAGS) $(CXXFLAGS) -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS -c ../../boostregex/BoostRegexSearch.cxx

