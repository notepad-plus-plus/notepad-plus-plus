echo off
rem this file is part of installer for Notepad++
rem Copyright (C)2006 Don HO <don.h@free.fr>
rem 
rem This program is free software; you can redistribute it and/or
rem modify it under the terms of the GNU General Public License
rem as published by the Free Software Foundation; either
rem version 2 of the License, or (at your option) any later version.
rem 
rem This program is distributed in the hope that it will be useful,
rem but WITHOUT ANY WARRANTY; without even the implied warranty of
rem MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
rem GNU General Public License for more details.
rem 
rem You should have received a copy of the GNU General Public License
rem along with this program; if not, write to the Free Software
rem Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

echo on

rmdir /S /Q .\build
mkdir .\build

rem Notepad++ minimalist package
rmdir /S /Q .\minimalist
mkdir .\minimalist

copy /Y ..\bin\license.txt .\minimalist\
If ErrorLevel 1 PAUSE
copy /Y ..\bin\readme.txt .\minimalist\
If ErrorLevel 1 PAUSE
copy /Y ..\bin\change.log .\minimalist\
If ErrorLevel 1 PAUSE
copy /Y ..\src\config.model.xml .\minimalist\
If ErrorLevel 1 PAUSE
copy /Y ..\src\langs.model.xml .\minimalist\
If ErrorLevel 1 PAUSE
copy /Y ..\src\stylers.model.xml .\minimalist\
If ErrorLevel 1 PAUSE
copy /Y ..\src\contextMenu.xml .\minimalist\
If ErrorLevel 1 PAUSE
copy /Y ..\src\shortcuts.xml .\minimalist\
If ErrorLevel 1 PAUSE
copy /Y ..\bin\doLocalConf.xml .\minimalist\
If ErrorLevel 1 PAUSE
copy /Y ..\bin\"notepad++.exe" .\minimalist\
If ErrorLevel 1 PAUSE
copy /Y ..\bin\SciLexer.dll .\minimalist\
If ErrorLevel 1 PAUSE


rem Notepad++ Unicode package
rmdir /S /Q .\zipped.package.release

mkdir .\zipped.package.release
mkdir .\zipped.package.release\updater
mkdir .\zipped.package.release\localization
mkdir .\zipped.package.release\themes
mkdir .\zipped.package.release\plugins
mkdir .\zipped.package.release\plugins\APIs
mkdir .\zipped.package.release\plugins\Config
mkdir .\zipped.package.release\plugins\Config\Hunspell
mkdir .\zipped.package.release\plugins\doc


copy /Y ..\bin\license.txt .\zipped.package.release\
If ErrorLevel 1 PAUSE
copy /Y ..\bin\readme.txt .\zipped.package.release\
If ErrorLevel 1 PAUSE
copy /Y ..\bin\change.log .\zipped.package.release\
If ErrorLevel 1 PAUSE
copy /Y ..\src\config.model.xml .\zipped.package.release\
If ErrorLevel 1 PAUSE
copy /Y ..\src\langs.model.xml .\zipped.package.release\
If ErrorLevel 1 PAUSE
copy /Y ..\src\stylers.model.xml .\zipped.package.release\
If ErrorLevel 1 PAUSE
copy /Y ..\src\contextMenu.xml .\zipped.package.release\
If ErrorLevel 1 PAUSE
copy /Y ..\src\shortcuts.xml .\zipped.package.release\
If ErrorLevel 1 PAUSE
copy /Y ..\src\functionList.xml .\zipped.package.release\
If ErrorLevel 1 PAUSE
copy /Y ..\bin\doLocalConf.xml .\zipped.package.release\
If ErrorLevel 1 PAUSE
copy /Y ..\bin\"notepad++.exe" .\zipped.package.release\
If ErrorLevel 1 PAUSE
copy /Y ..\bin\SciLexer.dll .\zipped.package.release\
If ErrorLevel 1 PAUSE

rem Plugins
copy /Y "..\bin\plugins\DSpellCheck.dll" .\zipped.package.release\plugins\
If ErrorLevel 1 PAUSE
copy /Y "..\bin\plugins\Config\Hunspell\dictionary.lst" .\zipped.package.release\plugins\Config\Hunspell\
If ErrorLevel 1 PAUSE
copy /Y "..\bin\plugins\Config\Hunspell\en_GB.dic" .\zipped.package.release\plugins\Config\Hunspell\
If ErrorLevel 1 PAUSE
copy /Y "..\bin\plugins\Config\Hunspell\en_GB.aff" .\zipped.package.release\plugins\Config\Hunspell\
If ErrorLevel 1 PAUSE
copy /Y "..\bin\plugins\Config\Hunspell\README_en_GB.txt" .\zipped.package.release\plugins\Config\Hunspell\
If ErrorLevel 1 PAUSE
copy /Y "..\bin\plugins\Config\Hunspell\en_US.dic" .\zipped.package.release\plugins\Config\Hunspell\
If ErrorLevel 1 PAUSE
copy /Y "..\bin\plugins\Config\Hunspell\en_US.aff" .\zipped.package.release\plugins\Config\Hunspell\
If ErrorLevel 1 PAUSE
copy /Y "..\bin\plugins\Config\Hunspell\README_en_US.txt" .\zipped.package.release\plugins\Config\Hunspell\
If ErrorLevel 1 PAUSE

copy /Y "..\bin\plugins\NppFTP.dll" .\zipped.package.release\plugins\
If ErrorLevel 1 PAUSE
copy /Y "..\bin\plugins\NppExport.dll" .\zipped.package.release\plugins\
If ErrorLevel 1 PAUSE
copy /Y "..\bin\plugins\mimeTools.dll" .\zipped.package.release\plugins\
If ErrorLevel 1 PAUSE
copy /Y "..\bin\plugins\NppConverter.dll" .\zipped.package.release\plugins\
If ErrorLevel 1 PAUSE

rem plugins manager and its updater 
copy /Y "..\bin\plugins\PluginManager.dll" .\zipped.package.release\plugins\
If ErrorLevel 1 PAUSE
copy /Y "..\bin\updater\gpup.exe" .\zipped.package.release\updater\
If ErrorLevel 1 PAUSE


rem localizations
copy /Y ".\nativeLang\*.xml" .\zipped.package.release\localization\
If ErrorLevel 1 PAUSE

rem files API
copy /Y ".\APIs\*.xml" .\zipped.package.release\plugins\APIs\
If ErrorLevel 1 PAUSE

rem theme
copy /Y ".\themes\*.xml" .\zipped.package.release\themes\
If ErrorLevel 1 PAUSE




"C:\Program Files\7-Zip\7z.exe" a -r .\build\npp.bin.minimalist.7z .\minimalist\*
If ErrorLevel 1 PAUSE
"C:\Program Files\7-Zip\7z.exe" a -tzip -r .\build\npp.bin.zip .\zipped.package.release\*
If ErrorLevel 1 PAUSE
"C:\Program Files\7-Zip\7z.exe" a -r .\build\npp.bin.7z .\zipped.package.release\*
If ErrorLevel 1 PAUSE
"C:\Program Files (x86)\NSIS\Unicode\makensis.exe" nppSetup.nsi


@echo off

setlocal enableDelayedExpansion 

cd .\build\

for %%a in (npp.*.Installer.exe) do (
  rem echo a = %%a
  set nppInstallerVar=%%a
  set zipvar=!nppInstallerVar:Installer.exe=bin.zip!
  set 7zvar=!nppInstallerVar:Installer.exe=bin.7z!
  set 7zvarMin=!nppInstallerVar:Installer.exe=bin.minimalist.7z!
  rem set md5var=!nppInstallerVar:Installer.exe=release.md5!
)
ren npp.bin.zip !zipvar!
ren npp.bin.7z !7zvar!
ren npp.bin.minimalist.7z !7zvarMin!
rem ..\externalTools\md5.exe -o!md5var! !nppInstallerVar! !zipvar! !7zvar!

cd ..

endlocal
