REM To add vista style icon, you need ReplaceVistaIcon.exe in the same directory. You can find it on:
REM http://www.rw-designer.com/compile-vista-icon

REM %1 should be the path of ReplaceVistaIcon.exe
REM %2 should be the path of notepad++.exe

if exist %1 (
%1 %2 "..\src\icons\nppNewIcon.ico"
)

