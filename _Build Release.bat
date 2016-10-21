@echo off

cd /d "%~dp0"

set BUILD_SOLUTION=%CD%\PowerEditor\visual.net\notepadPlus.vcxproj

:COMPILER
if defined PROGRAMFILES(X86) set PF=%PROGRAMFILES(X86)%
if not defined PROGRAMFILES(X86) set PF=%PROGRAMFILES%

set VCVARSALL=%PF%\Microsoft Visual Studio 14.0\VC\VcVarsAll.bat
set BUILD_PLATFORMTOOLSET=v140_xp
if exist "%VCVARSALL%" goto :COMPILER_END

set VCVARSALL=%PF%\Microsoft Visual Studio 12.0\VC\VcVarsAll.bat
set BUILD_PLATFORMTOOLSET=v120_xp
if exist "%VCVARSALL%" goto :COMPILER_END

set VCVARSALL=%PF%\Microsoft Visual Studio 11.0\VC\VcVarsAll.bat
set BUILD_PLATFORMTOOLSET=v110_xp
if exist "%VCVARSALL%" goto :COMPILER_END

set VCVARSALL=%PF%\Microsoft Visual Studio 10.0\VC\VcVarsAll.bat
set BUILD_PLATFORMTOOLSET=v100
if exist "%VCVARSALL%" goto :COMPILER_END

echo ERROR: Can't find Visual Studio 2010/2012/2013/2015
pause
goto :EOF
:COMPILER_END


:BUILD_X86
call "%VCVARSALL%" x86

msbuild /m /t:build "%BUILD_SOLUTION%" "/p:Configuration=Unicode Release" /p:Platform=Win32 /p:PlatformToolset=%BUILD_PLATFORMTOOLSET% /nologo /verbosity:normal
if %ERRORLEVEL% neq 0 ( echo ERRORLEVEL = %ERRORLEVEL% && pause && goto :EOF )

xcopy "scintilla" "scintilla32" /QEIYD
pushd "scintilla32\win32"
nmake NOBOOST=1 -f scintilla.mak
popd
if %ERRORLEVEL% neq 0 ( echo ERRORLEVEL = %ERRORLEVEL% && pause && goto :EOF )


:BUILD_X64
call "%VCVARSALL%" x86_amd64

msbuild /m /t:build "%BUILD_SOLUTION%" "/p:Configuration=Unicode Release" /p:Platform=x64 /p:PlatformToolset=%BUILD_PLATFORMTOOLSET% /nologo /verbosity:normal
if %ERRORLEVEL% neq 0 ( echo ERRORLEVEL = %ERRORLEVEL% && pause && goto :EOF )

xcopy "scintilla" "scintilla64" /QEIYD
pushd "scintilla64\win32"
nmake NOBOOST=1 -f scintilla.mak
popd
if %ERRORLEVEL% neq 0 ( echo ERRORLEVEL = %ERRORLEVEL% && pause && goto :EOF )


:COLLECT
xcopy "scintilla32\bin\SciLexer.dll"   "PowerEditor\bin\" /YFD
xcopy "scintilla64\bin\SciLexer.dll"   "PowerEditor\bin64\" /YFD

xcopy "PowerEditor\bin\NppShell.dll"   "PowerEditor\bin\NppShell_06.dll"
xcopy "PowerEditor\bin\NppShell64.dll" "PowerEditor\bin\NppShell64_06.dll"

:INSTALLER
pushd PowerEditor\installer
set SIGN=0
call packageAll.bat
popd

pause