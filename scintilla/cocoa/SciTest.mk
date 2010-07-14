### start defines ###
NAME=Editor

ARCH=-arch i386
CC=gcc -x c++ $(ARCH)
CO=gcc -x objective-c++ $(ARCH)
LD=gcc $(ARCH) -framework Cocoa

gDEFs=-DSCI_NAMESPACE -DSCI_LEXER

INCS=-I../src/ -I../include/ -I.
CCX=$(CC) $(gDEFs) $(INCS)
CCO=$(CO) $(gDEFs) $(INCS)

BLD=build/SciAppBuild
TARG=$(APP)/Contents/MacOS/$(NAME)
APP=$(BLD)/$(NAME).app

all: $(BLD) $(TARG)

clean:
	-rm -rf $(BLD)

$(APP):
	-rm -rf $(APP)
	-mkdir $(APP)
	-mkdir $(APP)/Contents/
	-mkdir $(APP)/Contents/Frameworks/
	-mkdir $(APP)/Contents/MacOS/
	-mkdir $(APP)/Contents/Resources/
	-cp ScintillaTest/Info.plist $(APP)/Contents/Info.plist.bak
	-sed "s/\$${EXECUTABLE_NAME}/$(NAME)/g" < $(APP)/Contents/Info.plist.bak > $(APP)/Contents/Info.plist.bak2
	-sed "s/\$${PRODUCT_NAME}/$(NAME)/g" < $(APP)/Contents/Info.plist.bak2 > $(APP)/Contents/Info.plist
	-rm $(APP)/Contents/Info.plist.bak $(APP)/Contents/Info.plist.bak2
	-cp -r ScintillaTest/English.lproj $(APP)/Contents/Resources/
	/Developer/usr/bin/ibtool --errors --warnings --notices --output-format human-readable-text \
	--compile $(APP)/Contents/Resources/English.lproj/MainMenu.nib ScintillaTest/English.lproj/MainMenu.xib
	-cp ScintillaTest/TestData.sql $(APP)/Contents/Resources/
	-make -f Framework.mk all

$(TARG) : $(BLD)/main.o $(BLD)/AppController.o $(APP)
	-cp -R build/framebuild/Sci.framework $(APP)/Contents/Frameworks/
	$(LD) $(BLD)/main.o $(BLD)/AppController.o $(APP)/Contents/Frameworks/Sci.framework/Sci -o $(TARG) -lstdc++


$(BLD) :
	-mkdir build
	-mkdir $(BLD)

$(BLD)/%.o : ScintillaTest/%.mm
	$(CCO) -c $< -o $@

$(BLD)/%.o : ScintillaTest/%.m
	$(CCO) -c $< -o $@
