@echo off

REM this script need some environment variables :
REM    platform: x64 or Win32
REM    configuration: Unicode Release or Unicode Debug
REM    boost_path: C:\projects\notepad-plus-plus\boost_1_55_0 when using AppVeyor.
REM example:
REM set platform=x64
REM set configuration=Unicode Release
REM set boost_path=C:\dev\boost_1_55_0


REM set boost variable
if "%platform%"=="x64" (set boost_platform=-x64)

REM Set the BOOST debug option
if "%configuration%"=="Unicode Debug" (
   set boost_debug=DEBUG=1
) else (set boost_debug= )

REM Build the boost regular expression
cd scintilla\boostregex

call buildboost.bat %boost_path% %boost_platform%

REM Build scintilla project
cd ..\..
cd scintilla\win32
nmake %boost_debug% -f scintilla.mak clean || exit
nmake %boost_debug% -f scintilla.mak || exit

REM Build Power Editor aka Notepad++
cd ..\..
cd PowerEditor\visual.net
msbuild notepadPlus.vcxproj /p:configuration="%configuration%" /p:platform="%platform%" || exit

REM go back in the initial directory
cd ..\..
