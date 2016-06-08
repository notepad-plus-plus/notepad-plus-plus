@echo off

cd scintilla\boostregex                                                                 || exit
buildboost.bat C:\projects\notepad-plus-plus\boost_1_55_0                               || exit
cd ..\win32                                                                             || exit
nmake -f scintilla.mak                                                                  || exit
cd ..\..\PowerEditor\visual.net                                                         || exit
msbuild notepadPlus.vcxproj /p:configuration="Unicode Debug" /p:platform=Win32          || exit

appveyor PushArtifact "Unicode Debug\Notepad++.exe" -FileName "Notepad++_debug.exe"

msbuild notepadPlus.vcxproj /p:configuration="Unicode Release" /p:platform=Win32          || exit
cd ..\bin
appveyor PushArtifact "Notepad++.exe"

