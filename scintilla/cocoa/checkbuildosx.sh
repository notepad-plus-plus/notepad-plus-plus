# Script to build Scintilla for OS X with most supported build files.
# Current directory should be scintilla/cocoa before running.

cd ../..

# ************************************************************
# Target 1: build framework and test app with Xcode targetting OS X 10.7
# Clean both then build both -- if perform clean in ScintillaTest, also cleans ScintillaFramework
# which can cause double build
cd scintilla/cocoa/ScintillaFramework
xcodebuild clean
cd ../ScintillaTest
xcodebuild clean
cd ../ScintillaFramework
xcodebuild -sdk macosx10.7
cd ../ScintillaTest
xcodebuild -sdk macosx10.7
cd ../../..

# ************************************************************
# Target 2: build framework and test app with Xcode targetting OS X 10.6
cd scintilla/cocoa/ScintillaFramework
xcodebuild clean
cd ../ScintillaTest
xcodebuild clean
cd ../ScintillaFramework
xcodebuild -sdk macosx10.6
cd ../ScintillaTest
xcodebuild -sdk macosx10.6
cd ../../..

# ************************************************************
# Target 3: build framework and test app with Xcode targetting OS X 10.5
cd scintilla/cocoa/ScintillaFramework
xcodebuild clean
cd ../ScintillaTest
xcodebuild clean
cd ../ScintillaFramework
xcodebuild -sdk macosx10.5
cd ../ScintillaTest
xcodebuild -sdk macosx10.5
cd ../../..

# ************************************************************
# Target 4: Qt builds
# Requires Qt development libraries and qmake to be installed
cd scintilla/qt
cd ScintillaEditBase
qmake
xcodebuild clean
xcodebuild
cd ..

cd ScintillaEdit
python WidgetGen.py
qmake
xcodebuild clean
xcodebuild
cd ..

cd ScintillaEditPy
python sepbuild.py
cd ..
cd ../..

# ************************************************************
# Target 5: build framework and test app with make
cd scintilla/cocoa

make -f Framework.mk clean
make -f Framework.mk all

make -f SciTest.mk all

cd ../..
