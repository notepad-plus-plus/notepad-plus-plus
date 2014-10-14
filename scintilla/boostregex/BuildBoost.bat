@ECHO OFF
:: Perform the pre-steps to build boost and set the boost path for the build file
SETLOCAL
SET BOOSTPATH=
SET MSVCTOOLSET=
SET TOOLSETCOMMAND=
SET BOOSTVERSION=
SET WORKPATH=%~dp0%

:PARAMLOOP
IF [%1]==[] (
  GOTO PARAMCONTINUE
)

IF NOT [%1]==[--toolset] (
     SET BOOSTPATH=%1
)

IF [%1]==[--toolset] (
  SET MSVCTOOLSET=%2
  SHIFT
)



SHIFT
GOTO PARAMLOOP
:PARAMCONTINUE

IF [%BOOSTPATH%]==[] (
   GOTO USAGE
)

SET TOOLSETCOMMAND=

IF NOT [%MSVCTOOLSET%]==[] (
	SET TOOLSETCOMMAND=toolset=%MSVCTOOLSET% 
)

	

IF NOT EXIST "%BOOSTPATH%\boost\regex.hpp" (
   GOTO BOOSTNOTFOUND
)

IF NOT EXIST "%BOOSTPATH%\bjam\bin\bjam.exe" (
	ECHO Building BJAM, the boost build tool
	PUSHD %BOOSTPATH%\tools\build\v2
	CALL bootstrap.bat

	%BOOSTPATH%\tools\build\v2\b2 --prefix=%BOOSTPATH%\bjam install
	POPD
)

IF NOT ERRORLEVEL 0 (
	GOTO BUILDERROR
)
ECHO.
ECHO ***************************************************************
ECHO Building tool to check boost version
ECHO ***************************************************************
ECHO # Temporary version of auto-generated file > %WORKPATH%\boostpath.mak
ECHO # If you're seeing this version of the file, and you're not currently building boost, >> %WORKPATH%\boostpath.mak
ECHO # then your buildboost.bat is failing somewhere.  >> %WORKPATH%\boostpath.mak
ECHO # Run BuildBoost.bat [absolute_path_to_boost] to generate this file again >> %WORKPATH%\boostpath.mak
ECHO # And lookout for error messages >> %WORKPATH%\boostpath.mak
ECHO BOOSTPATH=%BOOSTPATH% >> %WORKPATH%\boostpath.mak

IF NOT EXIST bin md bin
nmake -f getboostver.mak

IF ERRORLEVEL 1 (
   ECHO ******************************
   ECHO ** ERROR building getboostver.exe
   ECHO ** Please see the error messages above, and post as much as you can to the
   ECHO ** Notepad++ Open Discussion forum
   ECHO ** http://sourceforge.net/projects/notepad-plus/forums/forum/331753
   ECHO.
   GOTO EOF
)

for /f "delims=" %%i in ('bin\getboostver.exe') do set BOOSTVERSION=%%i

IF [%BOOSTVERSION%]==[] (
   ECHO There was an error detecting the boost version.
   ECHO Please see the error messages above, and post as much as you can to the
   ECHO Notepad++ Open Discussion forum
   ECHO http://sourceforge.net/projects/notepad-plus/forums/forum/331753
   ECHO.
   GOTO EOF
)
ECHO.
ECHO ***************************************************************
ECHO Boost version in use: %BOOSTVERSION%
ECHO ***************************************************************
ECHO.

ECHO.
ECHO ***************************************************************
ECHO Building Boost::regex
ECHO ***************************************************************
ECHO.

PUSHD %BOOSTPATH%\libs\regex\build

%BOOSTPATH%\bjam\bin\bjam %TOOLSETCOMMAND% variant=release threading=multi link=static runtime-link=static
IF NOT ERRORLEVEL 0 (
	GOTO BUILDERROR
)

%BOOSTPATH%\bjam\bin\bjam %TOOLSETCOMMAND% variant=debug threading=multi link=static runtime-link=static
IF NOT ERRORLEVEL 0 (
	GOTO BUILDERROR
)

IF NOT [%MSVCTOOLSET%]==[] (
    GOTO TOOLSETKNOWN
)

:: VS2013
IF EXIST %BOOSTPATH%\bin.v2\libs\regex\build\msvc-12.0\release\link-static\runtime-link-static\threading-multi\libboost_regex-vc120-mt-s-%BOOSTVERSION%.lib (
	SET MSVCTOOLSET=msvc-12.0
)

:: VS2012
IF EXIST %BOOSTPATH%\bin.v2\libs\regex\build\msvc-11.0\release\link-static\runtime-link-static\threading-multi\libboost_regex-vc110-mt-s-%BOOSTVERSION%.lib (
	SET MSVCTOOLSET=msvc-11.0
)

:: VS2010
IF EXIST %BOOSTPATH%\bin.v2\libs\regex\build\msvc-10.0\release\link-static\runtime-link-static\threading-multi\libboost_regex-vc100-mt-s-%BOOSTVERSION%.lib (
	SET MSVCTOOLSET=msvc-10.0
)

:: VS2008
IF EXIST %BOOSTPATH%\bin.v2\libs\regex\build\msvc-9.0\release\link-static\runtime-link-static\threading-multi\libboost_regex-vc90-mt-s-%BOOSTVERSION%.lib (
	SET MSVCTOOLSET=msvc-9.0
)

:: VS2005
IF EXIST %BOOSTPATH%\bin.v2\libs\regex\build\msvc-8.0\release\link-static\runtime-link-static\threading-multi\libboost_regex-vc80-mt-s-%BOOSTVERSION%.lib (
	SET MSVCTOOLSET=msvc-8.0
)

IF [%MSVCTOOLSET%]==[] (
	ECHO No correctly built boost regex libraries could be found.  
	ECHO Try specifying the MSVC version on the command line.
	GOTO USAGE
)
ECHO ***********************************************
ECHO Assuming toolset in use is %MSVCTOOLSET%
ECHO ***********************************************
ECHO If this is not correct, specify the version on the command line with --toolset
ECHO Run buildboost.bat without parameters to see the usage.


:TOOLSETKNOWN

:: VS2013
IF [%MSVCTOOLSET%]==[msvc-12.0] (
	SET BOOSTLIBPATH=%BOOSTPATH%\bin.v2\libs\regex\build\msvc-12.0
)

:: VS2012
IF [%MSVCTOOLSET%]==[msvc-11.0] (
	SET BOOSTLIBPATH=%BOOSTPATH%\bin.v2\libs\regex\build\msvc-11.0
)

:: VS2010
IF [%MSVCTOOLSET%]==[msvc-10.0] (
	SET BOOSTLIBPATH=%BOOSTPATH%\bin.v2\libs\regex\build\msvc-10.0
)

:: VS2008
IF [%MSVCTOOLSET%]==[msvc-9.0] (
	SET BOOSTLIBPATH=%BOOSTPATH%\bin.v2\libs\regex\build\msvc-9.0
)

:: VS2005
IF [%MSVCTOOLSET%]==[msvc-8.0] (
	SET BOOSTLIBPATH=%BOOSTPATH%\bin.v2\libs\regex\build\msvc-8.0
)

:: Error case, so we try to give the user a helpful error message
IF [%BOOSTLIBPATH%] == [] (
    ECHO ****************************************
	ECHO ** ERROR
	ECHO ** Boost library could not be found.  
	ECHO ** Make sure you've specified the correct boost path on the command line, 
	ECHO ** and try adding the toolset version
	ECHO ****************************************
	GOTO USAGE
)

ECHO # Autogenerated file, run BuildBoost.bat [path_to_boost] to generate > %WORKPATH%\boostpath.mak
ECHO BOOSTPATH=%BOOSTPATH% >> %WORKPATH%\boostpath.mak
ECHO BOOSTLIBPATH=%BOOSTLIBPATH% >> %WORKPATH%\boostpath.mak
POPD
ECHO.
ECHO.
ECHO Boost::regex built.
ECHO.
ECHO Now you need to build scintilla.
ECHO.
ECHO From the scintilla\win32 directory 
ECHO.
ECHO   nmake -f scintilla.mak 
ECHO.
ECHO.

GOTO EOF

:BOOSTNOTFOUND
ECHO Boost Path not valid.  Run BuildBoost.bat with the absolute path to the directory
ECHO where you unpacked your boost zip.
ECHO.
:USAGE
ECHO.
ECHO Boost is available free from www.boost.org
ECHO.
ECHO Unzip the file downloaded from www.boost.org, and give the absolute path 
ECHO as the first parameter to buildboost.bat
ECHO.
ECHO e.g.
ECHO buildboost.bat d:\libs\boost_1_48_0

ECHO.
ECHO.
ECHO You can specify which version of the Visual Studio compiler to use
ECHO with --toolset.
ECHO Use:   
ECHO   --toolset msvc-8.0     for Visual studio 2005
ECHO   --toolset msvc-9.0     for Visual Studio 2008
ECHO   --toolset msvc-10.0    for Visual Studio 2010
ECHO   --toolset msvc-11.0    for Visual Studio 2012
ECHO   --toolset msvc-12.0    for Visual Studio 2013
ECHO.
ECHO.
ECHO e.g.  To build with boost in d:\libs\boost_1_48_0 with Visual Studio 2008
ECHO.
ECHO         buildboost.bat --toolset msvc-9.0 d:\libs\boost_1_48_0
ECHO.
GOTO EOF


:BUILDERROR
ECHO There was an error building boost.  Please see the messages above for details.
ECHO  - Have you got a clean extract from a recent boost version, such as 1.48?
ECHO  - Download a fresh copy from www.boost.org and extract it to a directory, 
ECHO    and run the batch again with the name of that directory

:EOF

ENDLOCAL
