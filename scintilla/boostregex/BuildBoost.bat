@ECHO OFF
::------------------------------------------------------------------------------
::
:: Perform the pre-steps to build boost and set the boost path for the build file
::
::------------------------------------------------------------------------------
SETLOCAL EnableExtensions EnableDelayedExpansion
SET "WORKPATH=%~dp0"

SET "BOOSTROOT="
SET "BOOSTTOOLSET="
SET "BOOSTADDRESSMODEL=32"

:PARAMLOOP
	IF [%1] EQU [] (
		GOTO :PARAMCONTINUE
	)
	IF /I [%1] EQU [--toolset] (
		SET BOOSTTOOLSET=%~2
		SHIFT
		GOTO :PARAMNEXT
	)
	IF /I [%1] EQU [-x64] (
		SET "BOOSTADDRESSMODEL=64"
		GOTO :PARAMNEXT
	)
	SET "BOOSTROOT=%~1"
:PARAMNEXT
	SHIFT
	GOTO :PARAMLOOP
:PARAMCONTINUE

IF [%BOOSTROOT%] EQU [] (
	GOTO :USAGE
)

::------------------------------------------------------------------------------

SET "BOOSTVERSION="
SET "BOOSTBIN=%BOOSTROOT%\bin.v2"
SET "BOOSTINTERMEDIATE=%BOOSTROOT%\objs"
SET "BOOSTINCLUDES=%BOOSTROOT%\boost"
SET "BOOSTSOURCE=%BOOSTROOT%\libs"
SET "BOOSTPATHMAKEFILE=%WORKPATH%boostpath.mak"
::
SET "BOOSTLIBRARY=Regex"
::
IF NOT EXIST "%BOOSTINCLUDES%\%BOOSTLIBRARY%.hpp" (
	ECHO.Not found: %BOOSTINCLUDES%\%BOOSTLIBRARY%.hpp
	GOTO :BOOSTNOTFOUND
)

IF DEFINED DEBUG (
	ECHO.&ECHO.DEBUG: Dump `BOOST...` environment variables
	SET BOOST
)

::------------------------------------------------------------------------------
::
:: Build and install the builder when applicable
:: NOTE: Boost encourages to use the name `b2` instead of `bjam`
::
SET "B2INSTALL=%BOOSTROOT%\b2"
SET "B2FULL=%B2INSTALL%\bin\b2.exe"

SET /A storeERRORLEVEL=0
IF NOT EXIST "%B2FULL%" (
	PUSHD "%BOOSTROOT%\tools\build\v2"

	ECHO.Building B2, the boost build tool
	CALL bootstrap.bat

	ECHO.Installing B2
	"b2.exe" --prefix="%B2INSTALL%" install
	SET /A storeERRORLEVEL=%ERRORLEVEL%

	POPD
)

IF %storeERRORLEVEL% NEQ 0 (
	GOTO :BUILDERROR
)

::------------------------------------------------------------------------------
::
:: Get version directly from source i.e. the version header-file.
::
SETLOCAL EnableExtensions EnableDelayedExpansion
SET "_Version="
SET "_File=%BOOSTROOT%\boost\version.hpp"
IF EXIST "%_File%" (
	FOR /F "Tokens=1,2,3*" %%A IN (
		%_File%
	) DO IF /I "%%~A" EQU "#define" IF /I "%%~B" EQU "BOOST_LIB_VERSION" (
		SET "_Version=%%~C"
	)
)
ENDLOCAL & (SET "BOOSTVERSION=%_Version%")

IF [%BOOSTVERSION%] EQU [] (
	ECHO.
	ECHO.There was an error detecting the boost version.
	ECHO.Please see the error messages above, and post as much as you can to the
	ECHO.Notepad++ Community forum
	ECHO.    https://notepad-plus-plus.org/community/
	ECHO.
	GOTO :END
) ELSE (
	ECHO.
	ECHO.***************************************************************
	ECHO.Boost version in use: %BOOSTVERSION%
	ECHO.***************************************************************
	ECHO.
)

::------------------------------------------------------------------------------

ECHO.
ECHO.***************************************************************
ECHO.Building Boost::%BOOSTLIBRARY%
ECHO.***************************************************************
ECHO.

PUSHD "%BOOSTSOURCE%\%BOOSTLIBRARY%\build"
::
:: See Boost C++ Libraries - Invocation [1] for additional properties and the
:: values allowed for the properties.
::      [1](http://www.boost.org/build/doc/html/bbv2/overview/invocation.html#bbv2.overview.invocation.targets)
::
SET B2OPTIONS=--build-dir="%BOOSTINTERMEDIATE%"
SET B2PROPERTIES="variant=debug,release" "threading=multi" "link=static" "runtime-link=static"
IF DEFINED BOOSTTOOLSET (
	SET B2PROPERTIES=%B2PROPERTIES% "toolset=%BOOSTTOOLSET%"
)
IF DEFINED BOOSTADDRESSMODEL (
	SET B2PROPERTIES=%B2PROPERTIES% "address-model=%BOOSTADDRESSMODEL%"
)
"%B2FULL%" %B2OPTIONS% %B2PROPERTIES%
SET /A storeERRORLEVEL=%ERRORLEVEL%
POPD
IF DEFINED DEBUG (
	ECHO.&ECHO.DEBUG: Dump `B2...` environment variables
	SET B2
)
IF %storeERRORLEVEL% NEQ 0 (
	GOTO :BUILDERROR
)

::------------------------------------------------------------------------------
::
:: Copy just built library files to location with shorter path
::
PUSHD "%BOOSTINTERMEDIATE%"
FOR /F "UseBackQ Tokens=1*" %%L IN (`DIR /A-D /B /OD /TW /S *.lib`) DO (
	SET "_PathSrc=%%~dpL"
	REM - e.g. `C:\Libraries\boost_1_58_0\bin.v2\libs\regex\build\msvc-10.0\release\address-model-64\link-static\runtime-link-static\threading-multi\libboost_regex-vc100-mt-s-1_58.lib`

	SET "_NrOfBits=!_PathSrc:*address-model-=!"
	IF "!_NrOfBits!" EQU "!_PathSrc!"  (
		REM - "address-model-" was not part of the path, default to 32 bit
		SET "_NrOfBits=32"
	) ELSE (
		SET "_NrOfBits=!_NrOfBits:~0,2!"
	)
	REM - `64` from example path

	SET "_ToolSet=!_PathSrc:*build\=!"
	SET "_ToolSet=!_ToolSet:\= !"
	FOR /F "Tokens=1,2* Delims= " %%V IN ("!_ToolSet!") DO SET "_ToolSet=%%V"
	REM - `msvc-10.0` from example path

	SET "_PathDst=%BOOSTBIN%\lib!_NrOfBits!-!_ToolSet!"
	IF NOT EXIST "!_PathDst!" (
		MD "!_PathDst!"
	)
	COPY "%%~L" "!_PathDst!\%%~nxL" >NUL
)
IF DEFINED DEBUG (
	ECHO.&ECHO.DEBUG: Dump `_...` environment variables
	SET _
)
:: Because of `/OD` and `/TW` options of DIR-command in FOR-loop above, the
:: `_ToolSet` variable will --at this point-- contain the most recent used toolset.
::      /OD   List by files by date/time (oldest first) order.
::      /TW   Use `Last Written` time field for sorting.
IF NOT DEFINED BOOSTTOOLSET (
	SET "BOOSTTOOLSET=%_ToolSet%"

	ECHO.
	ECHO.***********************************************
	ECHO.   Assuming toolset in use is !BOOSTTOOLSET!
	ECHO.***********************************************
	ECHO.If this is not correct, specify the version on the command line with --toolset
	ECHO.Run buildboost.bat without parameters to see the usage.
)
POPD

::------------------------------------------------------------------------------
:: Generate makefile required to build Scintilla utilizing Boost::Regex
::
IF DEFINED BOOSTADDRESSMODEL (
	SET "BOOSTLIBPATH=%BOOSTBIN%\lib%BOOSTADDRESSMODEL%-%BOOSTTOOLSET%"
) else (
	SET "BOOSTLIBPATH=%BOOSTBIN%-%BOOSTTOOLSET%"
)

IF NOT EXIST "%BOOSTLIBPATH%" (
	ECHO.***********************************************
	ECHO.** ERROR
	ECHO.** Boost library could not be found.
	ECHO.** Make sure you've specified the correct boost path on the command line,
	ECHO.** and try adding the toolset version
	ECHO.***********************************************
	GOTO :USAGE
)

(
	ECHO.# Autogenerated file, run BuildBoost.bat [path_to_boost] to generate
	ECHO.BOOSTPATH=%BOOSTROOT:\=/%
	ECHO.BOOSTLIBPATH=%BOOSTLIBPATH:\=/%
)>"%BOOSTPATHMAKEFILE%"

IF DEFINED DEBUG (
	ECHO.&ECHO.DEBUG: Dump contents of "%BOOSTPATHMAKEFILE%"
	TYPE "%BOOSTPATHMAKEFILE%"
)

::------------------------------------------------------------------------------

ECHO.
ECHO.
ECHO.Boost::Regex built.
ECHO.
ECHO.Now you need to build Scintilla.
ECHO.
ECHO.From the scintilla\win32 directory
ECHO.
ECHO.    nmake -f scintilla.mak
ECHO.
ECHO.

::------------------------------------------------------------------------------

GOTO :END

:BOOSTNOTFOUND
ECHO.Boost Path not valid.  Run BuildBoost.bat with the absolute path to the directory
ECHO.where you unpacked your boost zip.
ECHO.

:USAGE
ECHO.
ECHO.Boost is available free from www.boost.org
ECHO.
ECHO.
ECHO.usage: buildboost.bat [--toolset] [-x64] path
ECHO.
ECHO.required arguments:
ECHO.  path             Absolute path to the directory of the unpacked boost zip.
ECHO.                   Make sure it's enclosed in double quotes when it contains spaces^^!
ECHO.
ECHO.optional arguments:
ECHO.  --toolset {msvc-14.0,msvc-12.0,msvc-11.0,msvc-10.0,msvc-9.0,msvc-8.0}
ECHO.                   Specify which version of the Visual Studio compiler to use.
ECHO.                   Where:
ECHO.                       msvc-8.0  for Visual Studio 2005
ECHO.                       msvc-9.0  for Visual Studio 2008
ECHO.                       msvc-10.0 for Visual Studio 2010
ECHO.                       msvc-11.0 for Visual Studio 2012
ECHO.                       msvc-12.0 for Visual Studio 2013
ECHO.                       msvc-14.0 for Visual Studio 2015
ECHO.  -x64             To build a 64-bit version.
ECHO.
ECHO.examples:
ECHO.  buildboost.bat d:\libs\boost_1_55_0
ECHO.
ECHO.  buildboost.bat "d:\my libs path\boost_1_55_0"
ECHO.
ECHO.  buildboost.bat --toolset msvc-9.0 "d:\libs\boost_1_55_0"
ECHO.
ECHO.  buildboost.bat "d:\libs\boost_1_55_0" --toolset msvc-10.0 -x64
ECHO.
GOTO :END

:BUILDERROR
ECHO.There was an error building boost.  Please see the messages above for details.
ECHO. - Have you got a clean extract from a recent boost version, such as 1.55?
ECHO. - Download a fresh copy from www.boost.org and extract it to a directory,
ECHO.   and run the batch again with the name of that directory

:END
ENDLOCAL
