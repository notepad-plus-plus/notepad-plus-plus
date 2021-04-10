# This makefile should be included in the main scintilla.mak file,
# just after where LOBJS is defined (around line 
# 
# The following line should be added around line 211 of scintilla.mak
# !INCLUDE nppSpecifics.mak

# Set your boost path (the root of where you've unpacked your boost zip).
# Boost is available from www.boost.org


SRC_OBJS=\
	$(SRC_OBJS)\
	$(DIR_O)\BoostRegexSearch.obj\
	$(DIR_O)\UTF8DocumentIterator.obj

INCLUDES=$(INCLUDES) -I../../boostregex

CXXFLAGS=$(CXXFLAGS) -DSCI_OWNREGEX


$(DIR_O)\UTF8DocumentIterator.obj:: ../../boostregex/UTF8DocumentIterator.cxx
	$(CC) $(CXXFLAGS) -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS -c ../../boostregex/UTF8DocumentIterator.cxx	
	
$(DIR_O)\BoostRegexSearch.obj:: ../../boostregex/BoostRegexSearch.cxx ../src/CharClassify.h ../src/RESearch.h	
	$(CC) $(CXXFLAGS) -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS -c ../../boostregex/BoostRegexSearch.cxx

