### start defines ###
include common.mk

INST_NAME=-install_name \
	@executable_path/../Frameworks/Sci.framework/Versions/A/Sci

LD=gcc $(ARCH) -dynamiclib -framework Cocoa $(INST_NAME)

LEXOBJS:=$(addsuffix .o,$(basename $(notdir $(wildcard ../lexers/Lex*.cxx))))

SCI_LEXERS=$(LEXOBJS) \
	LexerBase.o LexerModule.o LexerSimple.o Accessor.o

SCI_OBJ=AutoComplete.o CallTip.o CellBuffer.o CharClassify.o \
	ContractionState.o Decoration.o Document.o Editor.o \
	ExternalLexer.o Indicator.o KeyMap.o LineMarker.o PerLine.o \
	PositionCache.o PropSetSimple.o RESearch.o RunStyles.o ScintillaBase.o Style.o \
	StyleContext.o UniConversion.o ViewStyle.o XPM.o WordList.o \
	Selection.o CharacterSet.o Catalogue.o $(SCI_LEXERS)

WAH_OBJ=DocumentAccessor.o KeyWords.o WindowAccessor.o

COC_OBJ=PlatCocoa.o ScintillaCocoa.o ScintillaView.o InfoBar.o

OBJ=$(SCI_OBJ) $(UNUSED_OBJ) $(COC_OBJ)
OBJS=$(addprefix $(FRM_BLD)/,$(OBJ))

TARG=$(APP)/Versions/A/Sci
APP=$(FRM_BLD)/Sci.framework
### end defines ###

### start targets ###

all: $(FRM_BLD) $(TARG)

cleanfrm:
	-rm -rf $(FRM_BLD)

$(APP): $(FRM_BLD)
	-rm -rf $(APP)
	-mkdir $(APP)
	-mkdir $(APP)/Versions
	-mkdir $(APP)/Versions/A
	-mkdir $(APP)/Versions/A/Headers
	-mkdir $(APP)/Versions/A/Resources
	-ln -sf `pwd`/$(APP)/Versions/A `pwd`/$(APP)/Versions/Current
	-ln -sf `pwd`/$(APP)/Versions/A/Headers `pwd`/$(APP)/Headers
	-ln -sf `pwd`/$(APP)/Versions/A/Resources `pwd`/$(APP)/Resources
	-cp *.h $(APP)/Headers/
	-cp ../src/*.h $(APP)/Headers/
	-cp ../include/*.h $(APP)/Headers/
	-cp -R ScintillaFramework/English.lproj $(APP)/Resources
	-cp res/*.png $(APP)/Resources
	-cp ScintillaFramework/Info.plist $(APP)/Resources

$(TARG) : $(OBJS) $(APP)
	$(LD) $(OBJS) $(gDEFs) -o $(TARG) -lstdc++
	-ln `pwd`/$(TARG) `pwd`/$(APP)/Sci

$(FRM_BLD):
	-mkdir $(BLD)
	-mkdir $(FRM_BLD)

### get around to filling out the real dependencies later ###
#$(FRM_BLD)/AutoComplete.o : ../src/AutoComplete.cxx ../src/AutoComplete.h \
#	../include/Platform.h

#$(FRM_BLD)/CallTip.o : ../src/CallTip.cxx ../src/CallTip.h \
#	../include/Platform.h

### end targets ###
