### start defines ###

ARCH=-arch i386
CC=gcc -x c++ $(ARCH)
CO=gcc -x objective-c++ $(ARCH)

INST_NAME=-install_name @executable_path/../Frameworks/Sci.framework/Versions/A/Sci
LD=gcc $(ARCH) -dynamiclib -framework Cocoa $(INST_NAME)

gDEFs=-DSCI_NAMESPACE -DSCI_LEXER

INCS=-I../src/ -I../include/ -I.
CCX=$(CC) $(gDEFs) $(INCS)
CCO=$(CO) $(gDEFs) $(INCS)

BLD=build/framebuild

SCI_LEXERS=LexAPDL.o LexASY.o LexAU3.o LexAVE.o LexAbaqus.o LexAda.o LexAsm.o LexAsn1.o \
	LexBaan.o LexBash.o LexBasic.o LexBullant.o LexCLW.o LexCOBOL.o LexCPP.o LexCSS.o \
	LexCaml.o LexCmake.o LexConf.o LexCrontab.o LexCsound.o LexD.o LexEScript.o LexEiffel.o \
	LexErlang.o LexFlagship.o LexForth.o LexFortran.o LexGAP.o LexGui4Cli.o LexHTML.o \
	LexHaskell.o LexInno.o LexKix.o LexLisp.o LexLout.o LexLua.o LexMMIXAL.o LexMPT.o \
	LexMSSQL.o LexMagik.o LexMarkdown.o LexMatlab.o LexMetapost.o LexMySQL.o LexNimrod.o \
	LexNsis.o LexOpal.o LexOthers.o LexPB.o LexPLM.o LexPOV.o LexPS.o LexPascal.o LexPerl.o \
	LexPowerPro.o LexPowerShell.o LexProgress.o LexPython.o LexR.o LexRebol.o LexRuby.o \
	LexSML.o LexSQL.o LexScriptol.o LexSmalltalk.o LexSorcus.o LexSpecman.o LexSpice.o \
	LexTACL.o LexTADS3.o LexTAL.o LexTCL.o LexTeX.o LexVB.o LexVHDL.o LexVerilog.o LexYAML.o

SCI_OBJ=AutoComplete.o CallTip.o CellBuffer.o CharClassify.o ContractionState.o Decoration.o \
	Document.o DocumentAccessor.o Editor.o ExternalLexer.o Indicator.o KeyMap.o KeyWords.o \
	LineMarker.o PerLine.o PositionCache.o PropSet.o RESearch.o RunStyles.o ScintillaBase.o \
	Style.o StyleContext.o UniConversion.o ViewStyle.o WindowAccessor.o XPM.o Selection.o $(SCI_LEXERS)
COC_OBJ=PlatCocoa.o ScintillaCocoa.o ScintillaView.o InfoBar.o

OBJ=$(SCI_OBJ) $(COC_OBJ)
OBJS=$(addprefix $(BLD)/,$(OBJ))

TARG=$(APP)/Versions/A/Sci
APP=$(BLD)/Sci.framework
### end defines ###

### start targets ###

all: $(BLD) $(TARG)

$(APP): $(BLD)
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

$(BLD):
	-mkdir build
	-mkdir $(BLD)

clean:
	-rm -rf $(BLD)

$(BLD)/%.o : ../src/%.cxx
	$(CCX) -c ../src/$< -o $@

$(BLD)/%.o : %.mm
	$(CCO) -c $< -o $@

### get around to filling out the real dependencies later ###
$(BLD)/AutoComplete.o : ../src/AutoComplete.cxx ../src/AutoComplete.h ../include/Platform.h

$(BLD)/CallTip.o : ../src/CallTip.cxx ../src/CallTip.h ../include/Platform.h

### end targets ###