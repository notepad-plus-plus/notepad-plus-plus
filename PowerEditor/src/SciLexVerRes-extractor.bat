@echo off

rem This batch file will be called from the MS Visual Studio build event
rem so the root dir for this bat-file will be the Notepad++ solution/project dir.
rem The output dir for the SciLexVerRes.txt has been selected to be discoverable by the Resource Compiler later,
rem without any explicit change to its "Additional Include Directories".

set outputResFile="..\..\PowerEditor\src\SciLexVerRes.txt"
if exist %outputResFile% del %outputResFile%

rem First "for" finds and extracts specific one line string from the resource rc-file like:
rem #define VERSION_SCINTILLA "5.5.4"
rem #define VERSION_LEXILLA "5.4.2"
rem Second "for" then uses regex to extract the right version part like "5.5.4" and "5.4.2" from it.

set sciVerStr=
for /F "delims=" %%a in ('findstr /L /C:"#define VERSION_SCINTILLA " "..\..\scintilla\win32\ScintRes.rc"') do (
	for %%b in (%%a) do (
    		echo %%b | findstr /R "[0-9]*\.[0-9]*\." > nul 2>&1 && set sciVerStr=%%b
	)
)

set lexVerStr=
for /F "delims=" %%a in ('findstr /L /C:"#define VERSION_LEXILLA " "..\..\lexilla\src\LexillaVersion.rc"') do (
	for %%b in (%%a) do (
    		echo %%b | findstr /R "[0-9]*\.[0-9]*\." > nul 2>&1 && set lexVerStr=%%b
	)
)

rem At this point we should have in the sciVerStr and lexVerStr quoted version strings like "5.5.4" and "5.4.2".
rem Now these will be finally trimmed of the quotes & merged to non-quoted resource text like:
rem 5.5.4/5.4.2

echo Scintilla version detected: %sciVerStr%
echo Lexilla version detected: %lexVerStr%

<nul set /p =%sciVerStr% > %outputResFile%
<nul set /p ="/" >> %outputResFile%
<nul set /p =%lexVerStr% >> %outputResFile%

rem One last thing - we need to suppress the error level set by the previous trick
rem so we can exit gracefully (to use in the MS Visual Studio's build events).

if %errorlevel%==1 ver > nul
