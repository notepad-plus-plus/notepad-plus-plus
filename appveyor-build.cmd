@echo off

cd PowerEditor\visual.net                                                               || exit
msbuild notepadPlus.vcxproj /p:configuration="Unicode Debug" /p:platform=Win32          || exit
