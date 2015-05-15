### start defines ###
include common.mk

NAME=Demo

LD=gcc $(ARCH) -framework Cocoa

TARG=$(APP)/Contents/MacOS/$(NAME)
APP=$(APP_BLD)/$(NAME).app

all: $(APP_BLD) $(TARG)

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

$(TARG) : $(APP_BLD)/main.o $(APP_BLD)/AppController.o $(APP)
	-cp -R $(FRM_BLD)/Sci.framework $(APP)/Contents/Frameworks/
	$(LD) $(APP_BLD)/main.o $(APP_BLD)/AppController.o $(APP)/Contents/Frameworks/Sci.framework/Sci -o $(TARG) -lstdc++

$(APP_BLD) :
	-mkdir $(BLD)
	-mkdir $(APP_BLD)
