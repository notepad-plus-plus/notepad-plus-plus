@echo off
rem Promote new result files.
rem Find all the *.new files under test\examples and copy them to their expected name without ".new".
rem Run after RunTest.bat if ".new" result files are correct.
pushd ..\test\examples
for /R %%f in (*.new) do (call :moveFile %%f)
popd
goto :eof

:moveFile
set pathWithNew=%1
set directory=%~dp1
set fileWithNew=%~nx1
set fileNoNew=%~n1
set pathNoNew=%pathWithNew:~0,-4%

if exist %pathNoNew% (
   echo Move %fileWithNew% to %fileNoNew% in %directory%
) else (
   echo New %fileWithNew% to %fileNoNew% in %directory%
)
move %pathWithNew% %pathNoNew%
goto :eof
