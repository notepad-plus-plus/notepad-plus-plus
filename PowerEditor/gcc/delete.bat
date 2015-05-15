@echo off
FOR /F "tokens=*" %%G IN ('dir /b /s ..\src\*.o') DO del "%%G"
del resources.res
@echo on