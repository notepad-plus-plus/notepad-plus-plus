echo off
rem This file is part of Notepad++ project
rem Copyright (C)2021 Don HO <don.h@free.fr>
rem
rem This program is free software: you can redistribute it and/or modify
rem it under the terms of the GNU General Public License as published by
rem the Free Software Foundation, either version 3 of the License, or
rem at your option any later version.
rem
rem This program is distributed in the hope that it will be useful,
rem but WITHOUT ANY WARRANTY; without even the implied warranty of
rem MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
rem GNU General Public License for more details.
rem
rem You should have received a copy of the GNU General Public License
rem along with this program.  If not, see <https://www.gnu.org/licenses/>.

echo on

if %SIGN% == 0 goto NoSign

set signtoolWin11="C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe"
set signBinary=%signtoolWin11% sign /fd SHA256 /tr http://timestamp.digicert.com /td sha256 /a /f %NPP_CERT% /p %NPP_CERT_PWD% /d "Notepad++" /du https://notepad-plus-plus.org/


%signBinary% ..\bin\notepad++.exe
If ErrorLevel 1 goto End
%signBinary% ..\bin64\notepad++.exe
If ErrorLevel 1 goto End
%signBinary% ..\binarm64\notepad++.exe
If ErrorLevel 1 goto End

%signBinary% ..\bin\NppShell.x86.dll
If ErrorLevel 1 goto End
%signBinary% ..\bin64\NppShell.msix
If ErrorLevel 1 goto End
%signBinary% ..\bin64\NppShell.x64.dll
If ErrorLevel 1 goto End
%signBinary% ..\binarm64\NppShell.msix
If ErrorLevel 1 goto End
%signBinary% ..\binarm64\NppShell.arm64.dll
If ErrorLevel 1 goto End

%signBinary% ..\bin\plugins\Config\nppPluginList.dll
If ErrorLevel 1 goto End
%signBinary% ..\bin64\plugins\Config\nppPluginList.dll
If ErrorLevel 1 goto End
%signBinary% ..\binarm64\plugins\Config\nppPluginList.dll
If ErrorLevel 1 goto End

%signBinary% ..\bin\updater\GUP.exe
If ErrorLevel 1 goto End
%signBinary% ..\bin64\updater\GUP.exe
If ErrorLevel 1 goto End
%signBinary% ..\binarm64\updater\GUP.exe
If ErrorLevel 1 goto End

%signBinary% ..\bin\updater\libcurl.dll
If ErrorLevel 1 goto End
%signBinary% ..\bin64\updater\libcurl.dll
If ErrorLevel 1 goto End
%signBinary% ..\binarm64\updater\libcurl.dll
If ErrorLevel 1 goto End

%signBinary% ..\bin\plugins\NppExport\NppExport.dll
If ErrorLevel 1 goto End
%signBinary% ..\bin64\plugins\NppExport\NppExport.dll
If ErrorLevel 1 goto End
%signBinary% ..\binarm64\plugins\NppExport\NppExport.dll
If ErrorLevel 1 goto End

%signBinary% ..\bin\plugins\mimeTools\mimeTools.dll
If ErrorLevel 1 goto End
%signBinary% ..\bin64\plugins\mimeTools\mimeTools.dll
If ErrorLevel 1 goto End
%signBinary% ..\binarm64\plugins\mimeTools\mimeTools.dll
If ErrorLevel 1 goto End

%signBinary% ..\bin\plugins\NppConverter\NppConverter.dll
If ErrorLevel 1 goto End
%signBinary% ..\bin64\plugins\NppConverter\NppConverter.dll
If ErrorLevel 1 goto End
%signBinary% ..\binarm64\plugins\NppConverter\NppConverter.dll
If ErrorLevel 1 goto End

:NoSign


rmdir /S /Q .\build
mkdir .\build

rem Notepad++ minimalist package
rmdir /S /Q .\minimalist
mkdir .\minimalist
mkdir .\minimalist\userDefineLangs
mkdir .\minimalist\themes

copy /Y ..\..\LICENSE .\minimalist\license.txt
If ErrorLevel 1 goto End
copy /Y ..\bin\readme.txt .\minimalist\
If ErrorLevel 1 goto End
copy /Y ..\bin\change.log .\minimalist\
If ErrorLevel 1 goto End
copy /Y "..\bin\userDefineLangs\markdown._preinstalled.udl.xml" .\minimalist\userDefineLangs\
If ErrorLevel 1 goto End
copy /Y ..\src\langs.model.xml .\minimalist\
If ErrorLevel 1 goto End
copy /Y ..\src\stylers.model.xml .\minimalist\
If ErrorLevel 1 goto End
copy /Y ..\src\contextMenu.xml .\minimalist\
If ErrorLevel 1 goto End
copy /Y ..\src\tabContextMenu_example.xml .\minimalist\
If ErrorLevel 1 goto End
copy /Y ..\src\toolbarButtonsConf_example.xml .\minimalist\
If ErrorLevel 1 goto End
copy /Y ..\src\shortcuts.xml .\minimalist\
If ErrorLevel 1 goto End
copy /Y ..\bin\doLocalConf.xml .\minimalist\
If ErrorLevel 1 goto End
copy /Y ..\bin\nppLogNulContentCorruptionIssue.xml .\minimalist\
If ErrorLevel 1 goto End
copy /Y ..\bin\"notepad++.exe" .\minimalist\
If ErrorLevel 1 goto End
copy /Y ".\themes\DarkModeDefault.xml" .\minimalist\themes\
If ErrorLevel 1 goto End
copy /Y ..\src\toolbarIcons.xml .\minimalist\
If ErrorLevel 1 goto End


rmdir /S /Q .\minimalist64
mkdir .\minimalist64
mkdir .\minimalist64\userDefineLangs
mkdir .\minimalist64\themes

copy /Y ..\..\LICENSE .\minimalist64\license.txt
If ErrorLevel 1 goto End
copy /Y ..\bin\readme.txt .\minimalist64\
If ErrorLevel 1 goto End
copy /Y ..\bin\change.log .\minimalist64\
If ErrorLevel 1 goto End
copy /Y "..\bin\userDefineLangs\markdown._preinstalled.udl.xml" .\minimalist64\userDefineLangs\
If ErrorLevel 1 goto End
copy /Y ..\src\langs.model.xml .\minimalist64\
If ErrorLevel 1 goto End
copy /Y ..\src\stylers.model.xml .\minimalist64\
If ErrorLevel 1 goto End
copy /Y ..\src\contextMenu.xml .\minimalist64\
If ErrorLevel 1 goto End
copy /Y ..\src\tabContextMenu_example.xml .\minimalist64\
If ErrorLevel 1 goto End
copy /Y ..\src\toolbarButtonsConf_example.xml .\minimalist64\
If ErrorLevel 1 goto End
copy /Y ..\src\shortcuts.xml .\minimalist64\
If ErrorLevel 1 goto End
copy /Y ..\bin\doLocalConf.xml .\minimalist64\
If ErrorLevel 1 goto End
copy /Y ..\bin\nppLogNulContentCorruptionIssue.xml .\minimalist64\
If ErrorLevel 1 goto End
copy /Y ..\bin64\"notepad++.exe" .\minimalist64\
If ErrorLevel 1 goto End
copy /Y ".\themes\DarkModeDefault.xml" .\minimalist64\themes\
If ErrorLevel 1 goto End
copy /Y ..\src\toolbarIcons.xml .\minimalist64\
If ErrorLevel 1 goto End


rmdir /S /Q .\minimalistArm64
mkdir .\minimalistArm64
mkdir .\minimalistArm64\userDefineLangs
mkdir .\minimalistArm64\themes

copy /Y ..\..\LICENSE .\minimalistArm64\license.txt
If ErrorLevel 1 goto End
copy /Y ..\bin\readme.txt .\minimalistArm64\
If ErrorLevel 1 goto End
copy /Y ..\bin\change.log .\minimalistArm64\
If ErrorLevel 1 goto End
copy /Y "..\bin\userDefineLangs\markdown._preinstalled.udl.xml" .\minimalistArm64\userDefineLangs\
If ErrorLevel 1 goto End
copy /Y ..\src\langs.model.xml .\minimalistArm64\
If ErrorLevel 1 goto End
copy /Y ..\src\stylers.model.xml .\minimalistArm64\
If ErrorLevel 1 goto End
copy /Y ..\src\contextMenu.xml .\minimalistArm64\
If ErrorLevel 1 goto End
copy /Y ..\src\tabContextMenu_example.xml .\minimalistArm64\
If ErrorLevel 1 goto End
copy /Y ..\src\toolbarButtonsConf_example.xml .\minimalistArm64\
If ErrorLevel 1 goto End
copy /Y ..\src\shortcuts.xml .\minimalistArm64\
If ErrorLevel 1 goto End
copy /Y ..\bin\doLocalConf.xml .\minimalistArm64\
If ErrorLevel 1 goto End
copy /Y ..\bin\nppLogNulContentCorruptionIssue.xml .\minimalistArm64\
If ErrorLevel 1 goto End
copy /Y ..\binarm64\"notepad++.exe" .\minimalistArm64\
If ErrorLevel 1 goto End
copy /Y ".\themes\DarkModeDefault.xml" .\minimalistArm64\themes\
If ErrorLevel 1 goto End
copy /Y ..\src\toolbarIcons.xml .\minimalistArm64\
If ErrorLevel 1 goto End


rem Remove old built Notepad++ 32-bit package
rmdir /S /Q .\zipped.package.release

rem Re-build Notepad++ 32-bit package folders
mkdir .\zipped.package.release
mkdir .\zipped.package.release\updater
mkdir .\zipped.package.release\localization
mkdir .\zipped.package.release\themes
mkdir .\zipped.package.release\autoCompletion
mkdir .\zipped.package.release\functionList
mkdir .\zipped.package.release\userDefineLangs
mkdir .\zipped.package.release\plugins
mkdir .\zipped.package.release\plugins\NppExport
mkdir .\zipped.package.release\plugins\mimeTools
mkdir .\zipped.package.release\plugins\NppConverter
mkdir .\zipped.package.release\plugins\Config
mkdir .\zipped.package.release\plugins\doc


rem Remove old built Notepad++ 64-bit package
rmdir /S /Q .\zipped.package.release64

rem Re-build Notepad++ 64-bit package folders
mkdir .\zipped.package.release64
mkdir .\zipped.package.release64\updater
mkdir .\zipped.package.release64\localization
mkdir .\zipped.package.release64\themes
mkdir .\zipped.package.release64\autoCompletion
mkdir .\zipped.package.release64\functionList
mkdir .\zipped.package.release64\userDefineLangs
mkdir .\zipped.package.release64\plugins
mkdir .\zipped.package.release64\plugins\NppExport
mkdir .\zipped.package.release64\plugins\mimeTools
mkdir .\zipped.package.release64\plugins\NppConverter
mkdir .\zipped.package.release64\plugins\Config
mkdir .\zipped.package.release64\plugins\doc


rem Remove old built Notepad++ ARM64-bit package
rmdir /S /Q .\zipped.package.releaseArm64

rem Re-build Notepad++ ARM64-bit package folders
mkdir .\zipped.package.releaseArm64
mkdir .\zipped.package.releaseArm64\updater
mkdir .\zipped.package.releaseArm64\localization
mkdir .\zipped.package.releaseArm64\themes
mkdir .\zipped.package.releaseArm64\autoCompletion
mkdir .\zipped.package.releaseArm64\functionList
mkdir .\zipped.package.releaseArm64\userDefineLangs
mkdir .\zipped.package.releaseArm64\plugins
mkdir .\zipped.package.releaseArm64\plugins\NppExport
mkdir .\zipped.package.releaseArm64\plugins\mimeTools
mkdir .\zipped.package.releaseArm64\plugins\NppConverter
mkdir .\zipped.package.releaseArm64\plugins\Config
mkdir .\zipped.package.releaseArm64\plugins\doc


rem Basic: Copy needed files into Notepad++ 32-bit package folders
copy /Y ..\..\LICENSE .\zipped.package.release\license.txt
If ErrorLevel 1 goto End
copy /Y ..\bin\readme.txt .\zipped.package.release\
If ErrorLevel 1 goto End
copy /Y ..\bin\change.log .\zipped.package.release\
If ErrorLevel 1 goto End
copy /Y ..\src\langs.model.xml .\zipped.package.release\
If ErrorLevel 1 goto End
copy /Y ..\src\stylers.model.xml .\zipped.package.release\
If ErrorLevel 1 goto End
copy /Y ..\src\contextMenu.xml .\zipped.package.release\
If ErrorLevel 1 goto End
copy /Y ..\src\tabContextMenu_example.xml .\zipped.package.release\
If ErrorLevel 1 goto End
copy /Y ..\src\toolbarButtonsConf_example.xml .\zipped.package.release\
If ErrorLevel 1 goto End
copy /Y ..\src\shortcuts.xml .\zipped.package.release\
If ErrorLevel 1 goto End
copy /Y ..\bin\doLocalConf.xml .\zipped.package.release\
If ErrorLevel 1 goto End
copy /Y ..\bin\nppLogNulContentCorruptionIssue.xml .\zipped.package.release\
If ErrorLevel 1 goto End
copy /Y ..\bin\"notepad++.exe" .\zipped.package.release\
If ErrorLevel 1 goto End
copy /Y ..\src\toolbarIcons.xml .\zipped.package.release\
If ErrorLevel 1 goto End



rem Basic Copy needed files into Notepad++ 64-bit package folders
copy /Y ..\..\LICENSE .\zipped.package.release64\license.txt
If ErrorLevel 1 goto End
copy /Y ..\bin\readme.txt .\zipped.package.release64\
If ErrorLevel 1 goto End
copy /Y ..\bin\change.log .\zipped.package.release64\
If ErrorLevel 1 goto End
copy /Y ..\src\langs.model.xml .\zipped.package.release64\
If ErrorLevel 1 goto End
copy /Y ..\src\stylers.model.xml .\zipped.package.release64\
If ErrorLevel 1 goto End
copy /Y ..\src\contextMenu.xml .\zipped.package.release64\
If ErrorLevel 1 goto End
copy /Y ..\src\tabContextMenu_example.xml .\zipped.package.release64\
If ErrorLevel 1 goto End
copy /Y ..\src\toolbarButtonsConf_example.xml .\zipped.package.release64\
If ErrorLevel 1 goto End
copy /Y ..\src\shortcuts.xml .\zipped.package.release64\
If ErrorLevel 1 goto End
copy /Y ..\bin\doLocalConf.xml .\zipped.package.release64\
If ErrorLevel 1 goto End
copy /Y ..\bin\nppLogNulContentCorruptionIssue.xml .\zipped.package.release64\
If ErrorLevel 1 goto End
copy /Y ..\bin64\"notepad++.exe" .\zipped.package.release64\
If ErrorLevel 1 goto End
copy /Y ..\src\toolbarIcons.xml .\zipped.package.release64\
If ErrorLevel 1 goto End


rem Basic Copy needed files into Notepad++ ARM64 package folders
copy /Y ..\..\LICENSE .\zipped.package.releaseArm64\license.txt
If ErrorLevel 1 goto End
copy /Y ..\bin\readme.txt .\zipped.package.releaseArm64\
If ErrorLevel 1 goto End
copy /Y ..\bin\change.log .\zipped.package.releaseArm64\
If ErrorLevel 1 goto End
copy /Y ..\src\langs.model.xml .\zipped.package.releaseArm64\
If ErrorLevel 1 goto End
copy /Y ..\src\stylers.model.xml .\zipped.package.releaseArm64\
If ErrorLevel 1 goto End
copy /Y ..\src\contextMenu.xml .\zipped.package.releaseArm64\
If ErrorLevel 1 goto End
copy /Y ..\src\tabContextMenu_example.xml .\zipped.package.releaseArm64\
If ErrorLevel 1 goto End
copy /Y ..\src\toolbarButtonsConf_example.xml .\zipped.package.releaseArm64\
If ErrorLevel 1 goto End
copy /Y ..\src\shortcuts.xml .\zipped.package.releaseArm64\
If ErrorLevel 1 goto End
copy /Y ..\bin\doLocalConf.xml .\zipped.package.releaseArm64\
If ErrorLevel 1 goto End
copy /Y ..\bin\nppLogNulContentCorruptionIssue.xml .\zipped.package.releaseArm64\
If ErrorLevel 1 goto End
copy /Y ..\binarm64\"notepad++.exe" .\zipped.package.releaseArm64\
If ErrorLevel 1 goto End
copy /Y ..\src\toolbarIcons.xml .\zipped.package.releaseArm64\
If ErrorLevel 1 goto End


rem Plugins: Copy needed files into Notepad++ 32-bit package folders
copy /Y "..\bin\plugins\NppExport\NppExport.dll" .\zipped.package.release\plugins\NppExport\
If ErrorLevel 1 goto End
copy /Y "..\bin\plugins\mimeTools\mimeTools.dll" .\zipped.package.release\plugins\mimeTools\
If ErrorLevel 1 goto End
copy /Y "..\bin\plugins\NppConverter\NppConverter.dll" .\zipped.package.release\plugins\NppConverter\
If ErrorLevel 1 goto End

rem Plugins: Copy needed files into Notepad++ 64-bit package folders
copy /Y "..\bin64\plugins\NppExport\NppExport.dll" .\zipped.package.release64\plugins\NppExport\
If ErrorLevel 1 goto End
copy /Y "..\bin64\plugins\mimeTools\mimeTools.dll" .\zipped.package.release64\plugins\mimeTools\
If ErrorLevel 1 goto End
copy /Y "..\bin64\plugins\NppConverter\NppConverter.dll" .\zipped.package.release64\plugins\NppConverter\
If ErrorLevel 1 goto End

rem Plugins: Copy needed files into Notepad++ 64-bit package folders
copy /Y "..\binarm64\plugins\NppExport\NppExport.dll" .\zipped.package.releaseArm64\plugins\NppExport\
If ErrorLevel 1 goto End
copy /Y "..\binarm64\plugins\mimeTools\mimeTools.dll" .\zipped.package.releaseArm64\plugins\mimeTools\
If ErrorLevel 1 goto End
copy /Y "..\binarm64\plugins\NppConverter\NppConverter.dll" .\zipped.package.releaseArm64\plugins\NppConverter\
If ErrorLevel 1 goto End


rem localizations: Copy all files into Notepad++ 32-bit/64-bit package folders
copy /Y ".\nativeLang\*.xml" .\zipped.package.release\localization\
If ErrorLevel 1 goto End
copy /Y ".\nativeLang\*.xml" .\zipped.package.release64\localization\
If ErrorLevel 1 goto End
copy /Y ".\nativeLang\*.xml" .\zipped.package.releaseArm64\localization\
If ErrorLevel 1 goto End

rem files API: Copy all files into Notepad++ 32-bit/64-bit package folders
copy /Y ".\APIs\*.xml" .\zipped.package.release\autoCompletion\
If ErrorLevel 1 goto End
copy /Y ".\APIs\*.xml" .\zipped.package.release64\autoCompletion\
If ErrorLevel 1 goto End
copy /Y ".\APIs\*.xml" .\zipped.package.releaseArm64\autoCompletion\
If ErrorLevel 1 goto End

rem FunctionList files: Copy all files into Notepad++ 32-bit/64-bit package folders
copy /Y ".\functionList\*.xml" .\zipped.package.release\functionList\
If ErrorLevel 1 goto End
copy /Y ".\functionList\*.xml" .\zipped.package.release64\functionList\
If ErrorLevel 1 goto End
copy /Y ".\functionList\*.xml" .\zipped.package.releaseArm64\functionList\
If ErrorLevel 1 goto End

rem Markdown as UserDefineLanguge: Markdown syntax highlighter into Notepad++ 32-bit/64-bit package folders
copy /Y "..\bin\userDefineLangs\markdown._preinstalled.udl.xml" .\zipped.package.release\userDefineLangs\
If ErrorLevel 1 goto End
copy /Y "..\bin\userDefineLangs\markdown._preinstalled.udl.xml" .\zipped.package.release64\userDefineLangs\
If ErrorLevel 1 goto End
copy /Y "..\bin\userDefineLangs\markdown._preinstalled.udl.xml" .\zipped.package.releaseArm64\userDefineLangs\
If ErrorLevel 1 goto End
copy /Y "..\bin\userDefineLangs\markdown._preinstalled_DM.udl.xml" .\zipped.package.release\userDefineLangs\
If ErrorLevel 1 goto End
copy /Y "..\bin\userDefineLangs\markdown._preinstalled_DM.udl.xml" .\zipped.package.release64\userDefineLangs\
If ErrorLevel 1 goto End
copy /Y "..\bin\userDefineLangs\markdown._preinstalled_DM.udl.xml" .\zipped.package.releaseArm64\userDefineLangs\
If ErrorLevel 1 goto End

rem theme: Copy all files into Notepad++ 32-bit/64-bit package folders
copy /Y ".\themes\*.xml" .\zipped.package.release\themes\
If ErrorLevel 1 goto End
copy /Y ".\themes\*.xml" .\zipped.package.release64\themes\
If ErrorLevel 1 goto End
copy /Y ".\themes\*.xml" .\zipped.package.releaseArm64\themes\
If ErrorLevel 1 goto End

rem Plugins Admin
rem for disabling auto-updater
copy /Y ..\src\config.4zipPackage.xml .\zipped.package.release\config.xml
If ErrorLevel 1 goto End
copy /Y ..\bin\plugins\Config\nppPluginList.dll .\zipped.package.release\plugins\Config\
If ErrorLevel 1 goto End
copy /Y ..\bin\updater\GUP.exe .\zipped.package.release\updater\
If ErrorLevel 1 goto End
copy /Y ..\bin\updater\libcurl.dll .\zipped.package.release\updater\
If ErrorLevel 1 goto End
copy /Y ..\bin\updater\gup.xml .\zipped.package.release\updater\
If ErrorLevel 1 goto End
copy /Y ..\bin\updater\LICENSE .\zipped.package.release\updater\
If ErrorLevel 1 goto End
copy /Y ..\bin\updater\README.md .\zipped.package.release\updater\
If ErrorLevel 1 goto End
copy /Y ..\bin\updater\updater.ico .\zipped.package.release\updater\
If ErrorLevel 1 goto End

rem For disabling auto-updater
copy /Y ..\src\config.4zipPackage.xml .\zipped.package.release64\config.xml
If ErrorLevel 1 goto End
copy /Y ..\bin64\plugins\Config\nppPluginList.dll .\zipped.package.release64\plugins\Config\
If ErrorLevel 1 goto End
copy /Y ..\bin64\updater\GUP.exe .\zipped.package.release64\updater\
If ErrorLevel 1 goto End
copy /Y ..\bin64\updater\libcurl.dll .\zipped.package.release64\updater\
If ErrorLevel 1 goto End
copy /Y ..\bin64\updater\gup.xml .\zipped.package.release64\updater\
If ErrorLevel 1 goto End
copy /Y ..\bin64\updater\LICENSE .\zipped.package.release64\updater\
If ErrorLevel 1 goto End
copy /Y ..\bin64\updater\README.md .\zipped.package.release64\updater\
If ErrorLevel 1 goto End
copy /Y ..\bin64\updater\updater.ico .\zipped.package.release64\updater\
If ErrorLevel 1 goto End

rem For disabling auto-updater
copy /Y ..\src\config.4zipPackage.xml .\zipped.package.releaseArm64\config.xml
If ErrorLevel 1 goto End
copy /Y ..\binarm64\plugins\Config\nppPluginList.dll .\zipped.package.releaseArm64\plugins\Config\
If ErrorLevel 1 goto End
copy /Y ..\binarm64\updater\GUP.exe .\zipped.package.releaseArm64\updater\
If ErrorLevel 1 goto End
copy /Y ..\binarm64\updater\libcurl.dll .\zipped.package.releaseArm64\updater\
If ErrorLevel 1 goto End
copy /Y ..\binarm64\updater\gup.xml .\zipped.package.releaseArm64\updater\
If ErrorLevel 1 goto End
copy /Y ..\binarm64\updater\LICENSE .\zipped.package.releaseArm64\updater\
If ErrorLevel 1 goto End
copy /Y ..\binarm64\updater\README.md .\zipped.package.releaseArm64\updater\
If ErrorLevel 1 goto End
copy /Y ..\binarm64\updater\updater.ico .\zipped.package.releaseArm64\updater\
If ErrorLevel 1 goto End



"C:\Program Files\7-Zip\7z.exe" a -r .\build\npp.portable.minimalist.7z .\minimalist\*
If ErrorLevel 1 goto End
"C:\Program Files\7-Zip\7z.exe" a -r .\build\npp.portable.minimalist.x64.7z .\minimalist64\*
If ErrorLevel 1 goto End
"C:\Program Files\7-Zip\7z.exe" a -r .\build\npp.portable.minimalist.arm64.7z .\minimalistArm64\*
If ErrorLevel 1 goto End


"C:\Program Files\7-Zip\7z.exe" a -tzip -r .\build\npp.portable.zip .\zipped.package.release\*
If ErrorLevel 1 goto End
"C:\Program Files\7-Zip\7z.exe" a -r .\build\npp.portable.7z .\zipped.package.release\*
If ErrorLevel 1 goto End
rem IF EXIST "%PROGRAMFILES(X86)%" ("%PROGRAMFILES(x86)%\NSIS\Unicode\makensis.exe" nppSetup.nsi) ELSE ("%PROGRAMFILES%\NSIS\Unicode\makensis.exe" nppSetup.nsi)
IF EXIST "%PROGRAMFILES(X86)%" ("%PROGRAMFILES(x86)%\NSIS\makensis.exe" nppSetup.nsi) ELSE ("%PROGRAMFILES%\NSIS\makensis.exe" nppSetup.nsi)
IF EXIST "%PROGRAMFILES(X86)%" ("%PROGRAMFILES(x86)%\NSIS\makensis.exe" -DARCH64 nppSetup.nsi) ELSE ("%PROGRAMFILES%\NSIS\makensis.exe" -DARCH64 nppSetup.nsi)
IF EXIST "%PROGRAMFILES(X86)%" ("%PROGRAMFILES(x86)%\NSIS\makensis.exe" -DARCHARM64 nppSetup.nsi) ELSE ("%PROGRAMFILES%\NSIS\makensis.exe" -DARCHARM64 nppSetup.nsi)

rem Remove old build
rmdir /S /Q .\zipped.package.release

rem 
"C:\Program Files\7-Zip\7z.exe" a -tzip -r .\build\npp.portable.x64.zip .\zipped.package.release64\*
If ErrorLevel 1 goto End

"C:\Program Files\7-Zip\7z.exe" a -r .\build\npp.portable.x64.7z .\zipped.package.release64\*
If ErrorLevel 1 goto End

rem 
"C:\Program Files\7-Zip\7z.exe" a -tzip -r .\build\npp.portable.arm64.zip .\zipped.package.releaseArm64\*
If ErrorLevel 1 goto End

"C:\Program Files\7-Zip\7z.exe" a -r .\build\npp.portable.arm64.7z .\zipped.package.releaseArm64\*
If ErrorLevel 1 goto End


rem set var locally in this batch file
setlocal enableDelayedExpansion 

cd .\build\


for %%a in (npp.*.Installer.exe) do (
  rem echo a = %%a
  set nppInstallerVar=%%a
  set nppInstallerVar64=!nppInstallerVar:Installer.exe=Installer.x64.exe!
  set nppInstallerVarArm64=!nppInstallerVar:Installer.exe=Installer.arm64.exe!

  rem nppInstallerVar should be the version for example: 6.9
  rem we put npp.6.9. + (portable.zip instead of Installer.exe) into var zipvar
  set zipvar=!nppInstallerVar:Installer.exe=portable.zip!

  set zipvar64=!nppInstallerVar:Installer.exe=portable.x64.zip!
  set zipvarArm64=!nppInstallerVar:Installer.exe=portable.arm64.zip!
  set 7zvar=!nppInstallerVar:Installer.exe=portable.7z!
  set 7zvar64=!nppInstallerVar:Installer.exe=portable.x64.7z!
  set 7zvarArm64=!nppInstallerVar:Installer.exe=portable.arm64.7z!
  set 7zvarMin=!nppInstallerVar:Installer.exe=portable.minimalist.7z!
  set 7zvarMin64=!nppInstallerVar:Installer.exe=portable.minimalist.x64.7z!
  set 7zvarMinArm64=!nppInstallerVar:Installer.exe=portable.minimalist.arm64.7z!
)

rem echo zipvar=!zipvar!
rem echo zipvar64=!zipvar64!
rem echo 7zvar=!7zvar!
rem echo 7zvar64=!7zvar64!
rem echo 7zvarMin=!7zvarMin!
rem echo 7zvarMin64=!7zvarMin64!
ren npp.portable.zip !zipvar!
ren npp.portable.x64.zip !zipvar64!
ren npp.portable.arm64.zip !zipvarArm64!
ren npp.portable.7z !7zvar!
ren npp.portable.x64.7z !7zvar64!
ren npp.portable.arm64.7z !7zvarArm64!
ren npp.portable.minimalist.7z !7zvarMin!
ren npp.portable.minimalist.x64.7z !7zvarMin64!
ren npp.portable.minimalist.arm64.7z !7zvarMinArm64!


cd ..

endlocal

:End