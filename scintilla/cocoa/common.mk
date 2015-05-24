### shared variables and targets between Framework.mk and SciTest.mk ###

# build directories
BLD=build
APP_BLD=$(BLD)/Application
FRM_BLD=$(BLD)/Framework

ifdef DBG
CFLAGS=-g -O0
else
CFLAGS=-Os
endif

# compiler and compiler options
ARCH=-arch i386 $(CFLAGS)
CC=gcc -x c++ $(ARCH)
CO=gcc -x objective-c++ $(ARCH)
CCX=$(CC) $(gDEFs) $(INCS)
CCO=$(CO) $(gDEFs) $(INCS)

# include directories and global #define
gDEFs=-DSCI_NAMESPACE -DSCI_LEXER

# source directories
SRC_DIRS=../src ./ScintillaFramework ./ScintillaTest ./ \
	../lexers ../lexlib

INC_DIRS=$(SRC_DIRS) ../include

INCS=$(addprefix -I,$(INC_DIRS))

vpath %.m $(SRC_DIRS)
vpath %.mm $(SRC_DIRS)
vpath %.cpp $(SRC_DIRS)
vpath %.cxx $(SRC_DIRS)
vpath %.c $(SRC_DIRS)
vpath %.h $(INC_DIRS)

# clean everything
clean:
	-rm -rf $(BLD)

# build application objective-c++ files
$(APP_BLD)/%.o : %.mm
	$(CCO) -c $< -o $@

# build application objective-c files
$(APP_BLD)/%.o : %.m
	$(CCO) -c $< -o $@

# build framework c++ files
$(FRM_BLD)/%.o : %.cxx
	$(CCX) -c $< -o $@

# build framework objective-c++ files
$(FRM_BLD)/%.o : %.mm
	$(CCO) -c $< -o $@