REM To add vista style icon, you need ChangeIcon.exe in the same directory.

REM %1 should be the path of ChangeIcon.exe
REM %2 should be the path of notepad++.exe

if exist %1 (
%1 "..\src\icons\nppNewIcon.ico" %2 100 1033
)

