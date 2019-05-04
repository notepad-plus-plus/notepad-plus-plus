# This makefile should be included in the main scintilla.mak file,
# just after where LOBJS is defined (around line 
# 
# The following line should be added around line 211 of scintilla.mak
# !INCLUDE nppSpecifics.mak

# Set your boost path (the root of where you've unpacked your boost zip).
# Boost is available from www.boost.org

!IFDEF BOOSTPATH
!IFDEF BOOSTREGEXLIBPATH

SOBJS=\
	$(SOBJS)\
	$(DIR_O)\BoostRegexSearch.obj\
	$(DIR_O)\UTF8DocumentIterator.obj

SCILEXOBJS=\
	$(SCILEXOBJS)\
	$(DIR_O)\BoostRegexSearch.obj\
	$(DIR_O)\UTF8DocumentIterator.obj
	
LOBJS=\
	$(LOBJS)\
	$(DIR_O)\BoostRegexSearch.obj\
	$(DIR_O)\UTF8DocumentIterator.obj

INCLUDEDIRS=$(INCLUDEDIRS) -I$(BOOSTPATH)

CXXFLAGS=$(CXXFLAGS) -DSCI_OWNREGEX -arch:IA32
LDFLAGS=$(LDFLAGS) -LIBPATH:$(BOOSTREGEXLIBPATH)


$(DIR_O)\UTF8DocumentIterator.obj:: ../boostregex/UTF8DocumentIterator.cxx
	$(CC) $(CXXFLAGS) -c ../boostregex/UTF8DocumentIterator.cxx	
	
$(DIR_O)\BoostRegexSearch.obj:: ../boostregex/BoostRegexSearch.cxx ../src/CharClassify.h ../src/RESearch.h	
	$(CC) $(CXXFLAGS) -c ../boostregex/BoostRegexSearch.cxx

!ENDIF
!ENDIF

