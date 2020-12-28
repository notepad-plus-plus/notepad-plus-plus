; this file is part of installer for Notepad++
; Copyright (C)2016 Don HO <don.h@free.fr>
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either
; version 2 of the License, or (at your option) any later version.
;
; Note that the GPL places important restrictions on "derived works", yet
; it does not provide a detailed definition of that term.  To avoid      
; misunderstandings, we consider an application to constitute a          
; "derivative work" for the purpose of this license if it does any of the
; following:                                                             
; 1. Integrates source code from Notepad++.
; 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
;    installer, such as those produced by InstallShield.
; 3. Links to a library or executes a program that does any of the above.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; 
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


; NSIS includes
!include "x64.nsh"       ; a few simple macros to handle installations on x64 machines
!include "MUI.nsh"       ; Modern UI
!include "nsDialogs.nsh" ; allows creation of custom pages in the installer
!include "Memento.nsh"   ; remember user selections in the installer across runs
!include "FileFunc.nsh"

Unicode true			; Generate a Unicode installer. It can only be used outside of sections and functions and before any data is compressed.
SetCompressor /SOLID lzma	; This reduces installer size by approx 30~35%
;SetCompressor /FINAL lzma	; This reduces installer size by approx 15~18%


; Installer is DPI-aware: not scaled by the DWM, no blurry text
ManifestDPIAware true

!include "nsisInclude\winVer.nsh"
!include "nsisInclude\globalDef.nsh"
!include "nsisInclude\tools.nsh"
!include "nsisInclude\uninstall.nsh"

!ifdef ARCH64
OutFile ".\build\npp.${APPVERSION}.Installer.x64.exe"
!else
OutFile ".\build\npp.${APPVERSION}.Installer.exe"
!endif

; ------------------------------------------------------------------------
; Version Information
   VIProductVersion	"${Version}"
   VIAddVersionKey	"ProductName"		"${APPNAME}"
   VIAddVersionKey	"CompanyName"		"${CompanyName}"
   VIAddVersionKey	"LegalCopyright"	"${LegalCopyright}"
   VIAddVersionKey	"FileDescription"	"${Description}"
   VIAddVersionKey	"FileVersion"		"${Version}"
   VIAddVersionKey	"ProductVersion"	"${ProdVer}"
; ------------------------------------------------------------------------
 
; Insert CheckIfRunning function as an installer and uninstaller function.
!insertmacro CheckIfRunning ""
!insertmacro CheckIfRunning "un."

; Modern interface settings
!define MUI_ICON ".\images\npp_inst.ico"
!define MUI_UNICON ".\images\npp_inst.ico"

!define MUI_WELCOMEFINISHPAGE_BITMAP ".\images\wizard.bmp"
;!define MUI_WELCOMEFINISHPAGE_BITMAP ".\images\wizard_GiletJaune.bmp"


!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP ".\images\headerLeft.bmp" ; optional
!define MUI_HEADERIMAGE_BITMAP_RTL ".\images\headerLeft_RTL.bmp" ; Header for RTL languages
!define MUI_ABORTWARNING
!define MUI_COMPONENTSPAGE_SMALLDESC ;Show components page with a small description and big box for components


!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
page Custom ExtraOptions
!define MUI_PAGE_CUSTOMFUNCTION_SHOW "CheckIfRunning"
!insertmacro MUI_PAGE_INSTFILES


!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchNpp"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!define MUI_PAGE_CUSTOMFUNCTION_SHOW "un.CheckIfRunning"
!insertmacro MUI_UNPAGE_INSTFILES


!include "nsisInclude\langs4Installer.nsh"

!include "nsisInclude\mainSectionFuncs.nsh"

Section -"setPathAndOptionsSection" setPathAndOptionsSection
	Call setPathAndOptions
SectionEnd

!include "nsisInclude\autoCompletion.nsh"

!include "nsisInclude\functionList.nsh"

!include "nsisInclude\binariesComponents.nsh"

InstType "Minimalist"


Var diffArchDir2Remove
Var noUpdater

Function .onInit

	${GetParameters} $R0 
	${GetOptions} $R0 "/noUpdater" $R1 ;case insensitive 
	IfErrors withUpdater withoutUpdater
withUpdater:
	StrCpy $noUpdater "false"
	Goto updaterDone
withoutUpdater:
	StrCpy $noUpdater "true"
updaterDone:

	${If} $noUpdater == "true"
		!insertmacro UnSelectSection ${AutoUpdater}
		SectionSetText ${AutoUpdater} ""
		!insertmacro UnSelectSection ${PluginsAdmin}
		SectionSetText ${PluginsAdmin} ""
	${EndIf}


	${If} ${SectionIsSelected} ${PluginsAdmin}
		!insertmacro SetSectionFlag ${AutoUpdater} ${SF_RO}
		!insertmacro SelectSection ${AutoUpdater}
	${Else}
		!insertmacro ClearSectionFlag ${AutoUpdater} ${SF_RO}
	${EndIf}

	Call SetRoughEstimation		; This is rough estimation of files present in function copyCommonFiles
	InitPluginsDir			; Initializes the plug-ins dir ($PLUGINSDIR) if not already initialized.
	Call preventInstallInWin9x
		
	!insertmacro MUI_LANGDLL_DISPLAY

!ifdef ARCH64
	${If} ${RunningX64}
		; disable registry redirection (enable access to 64-bit portion of registry)
		SetRegView 64
		
		; change to x64 install dir if needed
		${If} "$InstDir" != ""
			${If} "$InstDir" == "$PROGRAMFILES\${APPNAME}"
				StrCpy $INSTDIR "$PROGRAMFILES64\${APPNAME}"
			${EndIf}
			; else /D was used or last installation is not "$PROGRAMFILES\${APPNAME}"
		${Else}
			StrCpy $INSTDIR "$PROGRAMFILES64\${APPNAME}"
		${EndIf}
		
		; check if 32-bit version has been installed if yes, ask user to remove it
		IfFileExists $PROGRAMFILES\${APPNAME}\notepad++.exe 0 noDelete32
		MessageBox MB_YESNO "You are trying to install 64-bit version while 32-bit version is already installed. Would you like to remove Notepad++ 32 bit version before proceeding further?$\n(Your custom config files will be kept)" /SD IDYES IDYES doDelete32 IDNO noDelete32 ;IDYES remove
doDelete32:
		StrCpy $diffArchDir2Remove $PROGRAMFILES\${APPNAME}
noDelete32:
		
	${Else}
		MessageBox MB_OK "You cannot install Notepad++ 64-bit version on your 32-bit system.$\nPlease download and install Notepad++ 32-bit version instead."
		Abort
	${EndIf}
!else ; 32-bit installer
	${If} ${RunningX64}
		; check if 64-bit version has been installed if yes, ask user to remove it
		IfFileExists $PROGRAMFILES64\${APPNAME}\notepad++.exe 0 noDelete64
		MessageBox MB_YESNO "You are trying to install 32-bit version while 64-bit version is already installed. Would you like to remove Notepad++ 64 bit version before proceeding further?$\n(Your custom config files will be kept)"  /SD IDYES IDYES doDelete64 IDNO noDelete64
doDelete64:
		StrCpy $diffArchDir2Remove $PROGRAMFILES64\${APPNAME}
noDelete64:
	${EndIf}

!endif

	${MementoSectionRestore}

FunctionEnd


Section -"Notepad++" mainSection
	${If} $diffArchDir2Remove != ""
		!insertmacro uninstallRegKey
		!insertmacro uninstallDir $diffArchDir2Remove 
	${endIf}

	Call copyCommonFiles

	Call removeUnstablePlugins

	Call removeOldContextMenu

	Call shortcutLinkManagement

SectionEnd

; Please **DONOT** move this function (SetRoughEstimation) anywhere else
; Just keep it right after the "mainSection" section
; Otherwise rough estimation for copyCommonFiles will not be set
; which will become reason for showing 0.0KB size on components section page

Function SetRoughEstimation
	SectionSetSize ${mainSection} 4500		; This is rough estimation of files present in function copyCommonFiles
FunctionEnd


!include "nsisInclude\langs4Npp.nsh"

!include "nsisInclude\themes.nsh"

${MementoSection} "Context Menu Entry" explorerContextMenu
	SetOverwrite try
	SetOutPath "$INSTDIR\"
	
	; There is no need to keep x86 NppShell_06.dll in 64 bit installer
	; But in 32bit installer both the Dlls are required
	; 	As user can install 32bit npp version on x64 bit machine, that time x64 bit NppShell is required.
	
	!ifdef ARCH64
		File /oname=$INSTDIR\NppShell_06.dll "..\bin\NppShell64_06.dll"
	!else
		${If} ${RunningX64}
			File /oname=$INSTDIR\NppShell_06.dll "..\bin\NppShell64_06.dll"
		${Else}
			File "..\bin\NppShell_06.dll"
		${EndIf}
	!endif
	Exec 'regsvr32 /s "$INSTDIR\NppShell_06.dll"'
${MementoSectionEnd}

${MementoSectionDone}

;--------------------------------
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${explorerContextMenu} 'Explorer context menu entry for Notepad++ : Open whatever you want in Notepad++ from Windows Explorer.'
    !insertmacro MUI_DESCRIPTION_TEXT ${autoCompletionComponent} 'Install the API files you need for the auto-completion feature (Ctrl+Space).'
    !insertmacro MUI_DESCRIPTION_TEXT ${functionListComponent} 'Install the function list files you need for the function list feature (Ctrl+Space).'
    !insertmacro MUI_DESCRIPTION_TEXT ${Plugins} 'You may need these plugins to extend the capabilities of Notepad++.'
    !insertmacro MUI_DESCRIPTION_TEXT ${localization} 'To use Notepad++ in your favorite language(s), install all/desired language(s).'
    !insertmacro MUI_DESCRIPTION_TEXT ${Themes} 'The eye-candy to change visual effects. Use Theme selector to switch among them.'
    !insertmacro MUI_DESCRIPTION_TEXT ${AutoUpdater} 'Keep Notepad++ updated: Automatically download and install the latest updates.'
    !insertmacro MUI_DESCRIPTION_TEXT ${PluginsAdmin} 'Install, Update and Remove any plugin from a list by some clicks. It needs Auto-Updater installed.'
  !insertmacro MUI_FUNCTION_DESCRIPTION_END
;--------------------------------



Function .onInstSuccess
	${MementoSectionSave}
FunctionEnd


; Keep "FinishSection" section in the last so that
; writing installation info happens in the last
; Specially for writing registry "EstimatedSize"
; which is visible in control panel in column named "size"

Section -FinishSection
  Call writeInstallInfoInRegistry
SectionEnd

BrandingText "Software is like sex: It's better when it's free"

; eof
