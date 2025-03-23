@echo off

rem If this batch file is called from the Microsoft Visual Studio pre-build event,
rem then the current dir for this bat-file is set to Notepad++ solution/project dir.
rem Because in this batch file we have to go two dir-levels higher (for finding the files
rem from the external libraries) from both the solution/project dir and from the dir,
rem where this batch file is located, we can run this batch file ok also manually
rem and not only from that MSVS event.
set outputFile="..\..\PowerEditor\src\NppLibsVersion.h"

if exist %outputFile% del %outputFile%

rem First "for" finds and extracts specific one line string like "#define VERSION_SCINTILLA "5.5.4"".
rem Second "for" then uses regex to extract only the "..." version part substring from it.

set sciVerStr="N/A"
for /F "delims=" %%a in ('findstr /L /C:"#define VERSION_SCINTILLA " "..\..\scintilla\win32\ScintRes.rc"') do (
	for %%b in (%%a) do (
			echo %%b | findstr /R "[0-9]*\.[0-9]*\." > nul 2>&1 && set sciVerStr=%%b
	)
)

set lexVerStr="N/A"
for /F "delims=" %%a in ('findstr /L /C:"#define VERSION_LEXILLA " "..\..\lexilla\src\LexillaVersion.rc"') do (
	for %%b in (%%a) do (
			echo %%b | findstr /R "[0-9]*\.[0-9]*\." > nul 2>&1 && set lexVerStr=%%b
	)
)

set boostRegexVerStr="N/A"
for /F "delims=" %%a in ('findstr /L /C:"#define BOOST_LIB_VERSION " "..\..\boostregex\boost\version.hpp"') do (
	for %%b in (%%a) do (
			echo %%b | findstr /R "[0-9]*_[0-9]*\" > nul 2>&1 && set boostRegexVerStr=%%b
	)
)

rem At this point, we should have the quoted version strings collected:
echo Scintilla version detected: %sciVerStr%
echo Lexilla version detected: %lexVerStr%
echo Boost Regex version detected: %boostRegexVerStr%

rem And finally create the output file:
echo // NppLibsVersion.h>%outputFile%
echo // - maintained by NppLibsVersionH-generator.bat>>%outputFile%
echo #define NPP_SCINTILLA_VERSION %sciVerStr%>>%outputFile%
echo #define NPP_LEXILLA_VERSION %lexVerStr%>>%outputFile%
echo #define NPP_BOOST_REGEX_VERSION %boostRegexVerStr%>>%outputFile%
