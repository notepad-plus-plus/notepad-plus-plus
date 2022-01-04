# Script to build Scintilla for macOS with most supported build files.
# Current directory should be scintilla/cocoa before running.

cd ../..

# ************************************************************
# Target 1: Unit tests

echo Unit tests

cd scintilla/test/unit
make clean
make test
cd ../../..

# ************************************************************
# Target 2: build framework and test app with Xcode targeting macOS 10.n with n from 9 to 5
# Only SDK versions that are installed will be built
# Clean both then build both -- if perform clean in ScintillaTest, also cleans ScintillaFramework
# which can cause double build

echo Building Cocoa-native ScintillaFramework and ScintillaTest
for sdk in macosx10.15 macosx10.14
do
    xcodebuild -showsdks | grep $sdk
    if [ "$(xcodebuild -showsdks | grep $sdk)" != "" ]
    then
        echo Building with $sdk
        cd scintilla/cocoa/ScintillaFramework
        xcodebuild clean
        cd ../ScintillaTest
        xcodebuild clean
        cd ../ScintillaFramework
        xcodebuild -sdk $sdk
        cd ../ScintillaTest
        xcodebuild -sdk $sdk
        cd ../../..
    else
        echo Warning $sdk not available
    fi
done

# ************************************************************
# Target 3: Qt builds
# Requires Qt development libraries and qmake to be installed

echo Building Qt and PySide

cd scintilla/qt
cd ScintillaEditBase
qmake -spec macx-xcode
xcodebuild clean
xcodebuild
cd ..

cd ScintillaEdit
python3 WidgetGen.py
qmake -spec macx-xcode
xcodebuild clean
xcodebuild
cd ..

cd ScintillaEditPy
python2 sepbuild.py
cd ..
cd ../..
