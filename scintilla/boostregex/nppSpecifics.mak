# This makefile should be included in the main scintilla.mak file,
# just after where LOBJS is defined (around line 
# 
# The following line should be added around line 211 of scintilla.mak
# !INCLUDE nppSpecifics.mak

# Set your boost path (the root of where you've unpacked your boost zip).
# Boost is available from www.boost.org

!IF EXIST(..\boostregex\boostpath.mak)

!INCLUDE ..\boostregex\boostpath.mak

SOBJS=\
	$(SOBJS)\
	$(DIR_O)\BoostRegexSearch.obj\
	$(DIR_O)\UTF8DocumentIterator.obj
	
LOBJS=\
	$(LOBJS)\
	$(DIR_O)\BoostRegexSearch.obj\
	$(DIR_O)\UTF8DocumentIterator.obj


INCLUDEDIRS=$(INCLUDEDIRS) -I$(BOOSTPATH)

CXXFLAGS=$(CXXFLAGS) -DSCI_OWNREGEX -arch:IA32
!IFDEF DEBUG
LDFLAGS=$(LDFLAGS) -LIBPATH:$(BOOSTLIBPATH)\debug\link-static\runtime-link-static\threading-multi
!ELSE
LDFLAGS=$(LDFLAGS) -LIBPATH:$(BOOSTLIBPATH)\release\link-static\runtime-link-static\threading-multi
!ENDIF



$(DIR_O)\UTF8DocumentIterator.obj:: ../boostregex/UTF8DocumentIterator.cxx
	$(CC) $(CXXFLAGS) -c ../boostregex/UTF8DocumentIterator.cxx	
	
$(DIR_O)\BoostRegexSearch.obj:: ../boostregex/BoostRegexSearch.cxx ../src/CharClassify.h ../src/RESearch.h	
	$(CC) $(CXXFLAGS) -c ../boostregex/BoostRegexSearch.cxx

!ELSE

!IFDEF NOBOOST
!MESSAGE Note: Building without Boost-Regex support. 
!ELSE
!MESSAGE Note: It looks like you've not built boost yet.  
!MESSAGE You can build boost::regex by running BuildBoost.bat 
!MESSAGE from scintilla\BoostRegex directory with the path where 
!MESSAGE you have extracted the boost archive (from www.boost.org)
!MESSAGE e.g. 
!MESSAGE       buildboost.bat d:\libs\boost_1_48_0
!MESSAGE
!MESSAGE If you want to build scintilla without Boost (and just 
!MESSAGE use the limited  built-in regular expressions), 
!MESSAGE then run nmake again, with NOBOOST=1
!MESSAGE e.g. 
!MESSAGE       nmake NOBOOST=1 -f scintilla.mak
!MESSAGE
!ERROR Stopping build.  Either build boost or specify NOBOOST=1
!ENDIF

!ENDIF
