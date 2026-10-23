; This file is part of Notepad++ project
; Copyright (C)2021 Don HO <don.h@free.fr>
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; at your option any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.


; NSIS includes
!include "x64.nsh"       ; a few simple macros to handle installations on x64 machines
!include "MUI.nsh"       ; Modern UI
!include "nsDialogs.nsh" ; allows creation of custom pages in the installer
!include "Memento.nsh"   ; remember user selections in the installer across runs
!include "FileFunc.nsh"
!include "WinVer.nsh"

Unicode true			; Generate a Unicode installer. It can only be used outside of sections and functions and before any data is compressed.
SetCompressor /SOLID lzma	; This reduces installer size by approx 30~35%
;SetCompressor /FINAL lzma	; This reduces installer size by approx 15~18%


; Installer is DPI-aware: not scaled by the DWM, no blurry text
ManifestDPIAware true

; Never request elevation at startup, so the installer can run for any user without UAC.
; If the user picks the "all users" install mode on the InstallModePage, we relaunch
; ourselves with ExecShell "runas" (which triggers UAC) and pass /AllUsers so the
; elevated instance remembers the choice. See InstallModePageLeave for the relaunch logic.
RequestExecutionLevel user

Var winSysDir

; Per-user vs all-users install support.
; Declared here (before the includes below) because tools.nsh and uninstall.nsh use them.
Var MultiUserInstallMode	; "AllUsers" or "CurrentUser"
Var IsElevated			; "true" if the installer process runs elevated (admin)
Var InstModeRadioAllUsers
Var InstModeRadioCurrentUser

!include "nsisInclude\winVer.nsh"
!include "nsisInclude\globalDef.nsh"
!include "nsisInclude\tools.nsh"
!include "nsisInclude\uninstall.nsh"

!ifdef ARCH64
OutFile ".\build\npp.${APPVERSION}.Installer.x64.exe"
!else ifdef ARCHARM64
OutFile ".\build\npp.${APPVERSION}.Installer.arm64.exe"
!else
OutFile ".\build\npp.${APPVERSION}.Installer.exe"
!endif

; Sign uninstaller
!uninstfinalize  'sign-installers.bat "%1"' = 0     ; %1 is replaced by the uninstaller exe to be signed.

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
Var runningNppDetected
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
Page custom InstallModePageCreate InstallModePageLeave
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

Var diffArchDir2Remove
Var noUpdater
Var closeRunningNpp
Var runNppAfterSilentInstall
Var relaunchNppAfterSilentInstall


!include "nsisInclude\langs4Installer.nsh"

!include "nsisInclude\mainSectionFuncs.nsh"

Section -"setPathAndOptionsSection" setPathAndOptionsSection
	Call setPathAndOptions
SectionEnd

!include "nsisInclude\autoCompletion.nsh"

!include "nsisInclude\functionList.nsh"

!include "nsisInclude\binariesComponents.nsh"

InstType "Minimalist"



; StrStr is used in .onInit to detect a user-provided "/D=" on the command line
!include "StrFunc.nsh"
${StrStr} # Supportable for Install Sections and Functions

; Apply the shell-var context (all/current) and registry view that match $MultiUserInstallMode.
; SHCTX-based registry access and $LOCALAPPDATA/$APPDATA then resolve to the right place.
Function applyInstallModeContext
	${If} $MultiUserInstallMode == "AllUsers"
		SetShellVarContext all
	${Else}
		SetShellVarContext current
	${EndIf}
!ifdef ARCH64 || ARCHARM64
	${If} ${RunningX64}
		SetRegView 64 ; 64-bit components: always use the non-redirected hive
	${EndIf}
!endif
FunctionEnd

; Decide the initial install mode from the elevation state, with optional /AllUsers and
; /CurrentUser command-line overrides (mostly useful for silent installations).
Function initInstallMode
	UserInfo::GetAccountType
	Pop $0
	${If} $0 == "Admin"
		StrCpy $IsElevated "true"
		StrCpy $MultiUserInstallMode "AllUsers"
	${Else}
		StrCpy $IsElevated "false"
		StrCpy $MultiUserInstallMode "CurrentUser"
	${EndIf}

	${GetParameters} $R0
	ClearErrors
	${GetOptions} $R0 "/CurrentUser" $R1
	${IfNot} ${Errors}
		StrCpy $MultiUserInstallMode "CurrentUser"
	${EndIf}
	ClearErrors
	${GetOptions} $R0 "/AllUsers" $R1
	${IfNot} ${Errors}
		${If} $IsElevated == "true"
			StrCpy $MultiUserInstallMode "AllUsers"
		${ElseIf} ${Silent}
			; A silent install has no page on which to offer the elevation that the GUI
			; does, so honouring /AllUsers here would quietly turn into a per-user install
			; while still reporting success. Fail with ERROR_ELEVATION_REQUIRED instead, so
			; deployment scripts that only check the exit code do not get a false positive.
			SetErrorLevel 740
			Quit
		${EndIf}
	${EndIf}

	Call applyInstallModeContext
FunctionEnd

; When the user switches mode, swap $INSTDIR to the new mode's default - but only if it still
; holds the other mode's default path (a /D= or restored previous path is left untouched).
Function updateDefaultInstDir
!ifdef ARCH64 || ARCHARM64
	StrCpy $R8 "$PROGRAMFILES64\${APPNAME}"
!else
	StrCpy $R8 "$PROGRAMFILES\${APPNAME}"
!endif
	StrCpy $R9 "$LOCALAPPDATA\${APPNAME}"
	${If} $MultiUserInstallMode == "CurrentUser"
		${If} "$INSTDIR" == "$R8"
			StrCpy $INSTDIR "$R9"
		${EndIf}
	${Else}
		${If} "$INSTDIR" == "$R9"
			StrCpy $INSTDIR "$R8"
		${EndIf}
	${EndIf}
FunctionEnd

; Custom page: let the user pick all-users vs per-user installation.
Function InstallModePageCreate
	!insertmacro MUI_HEADER_TEXT "Choose Installation Type" "Choose for which users you want to install Notepad++."
	nsDialogs::Create 1018
	Pop $0
	${If} $0 == error
		Abort
	${EndIf}

	${NSD_CreateLabel} 0 0 100% 24u "Install Notepad++ for all users of this computer (requires administrator rights), or only for you (no administrator rights needed)."
	Pop $0

	${NSD_CreateRadioButton} 10u 34u 100% 12u "Install for &all users (requires administrator rights)"
	Pop $InstModeRadioAllUsers
	${NSD_CreateRadioButton} 10u 50u 100% 12u "Install just for &me (into %LOCALAPPDATA%, no administrator rights needed)"
	Pop $InstModeRadioCurrentUser

	${If} $MultiUserInstallMode == "AllUsers"
		${NSD_Check} $InstModeRadioAllUsers
	${Else}
		${NSD_Check} $InstModeRadioCurrentUser
	${EndIf}

	; Both options are always selectable. If the user picks "for all users" from a
	; non-elevated process, InstallModePageLeave triggers a UAC prompt via
	; ExecShell "runas" and relaunches the installer elevated. Windows handles
	; the case where the account has no admin rights at all (will prompt for an
	; admin password) - which is friendlier than disabling the option outright.

	nsDialogs::Show
FunctionEnd

Function InstallModePageLeave
	${NSD_GetState} $InstModeRadioAllUsers $0
	${If} $0 == ${BST_CHECKED}
		StrCpy $MultiUserInstallMode "AllUsers"
		; All-users install needs admin rights. If this process is not elevated, relaunch
		; ourselves elevated (UAC prompt) and pass /AllUsers so the new instance picks up
		; the same choice. initInstallMode applies /AllUsers AFTER /CurrentUser, so a
		; pre-existing /CurrentUser on the cmdline is harmlessly overridden in the new
		; (elevated) process. The current (non-elevated) process then quits.
		${If} $IsElevated != "true"
			${GetParameters} $R0
			ExecShell "runas" "$EXEPATH" "/AllUsers $R0"
			Quit	; if the user cancels UAC, the install simply aborts (standard NSIS pattern)
		${EndIf}
	${Else}
		StrCpy $MultiUserInstallMode "CurrentUser"
	${EndIf}
	Call applyInstallModeContext
	Call updateDefaultInstDir
FunctionEnd

Function .onInit

	; Decide whether we install for all users (HKLM, Program Files) or just for the
	; current user (HKCU, %LOCALAPPDATA%). This sets $MultiUserInstallMode, the shell-var
	; context and the registry view, so all the SHCTX-based registry writes land in the
	; right hive and no admin rights are required for a per-user install.
	Call initInstallMode

	; Read back a previous installation directory (replaces the former InstallDirRegKey).
	; The root follows the install mode via SHCTX, and is skipped when the user passes "/D=".
	; (The NSIS x64 installer binary is still a 32-bit app, so SetRegView - done in
	;  initInstallMode - is required to read the non-redirected hive.)
	System::Call kernel32::GetCommandLine()t.r0 ; original cmdline ("/D=..." is not hidden from us by NSIS here)
	${StrStr} $1 $0 "/D="
	${If} "$1" == ""
		ReadRegStr $0 SHCTX "Software\${APPNAME}" ""
		${If} "$0" != ""
			; a previous installation path has been detected, so offer that as the $INSTDIR
			StrCpy $INSTDIR "$0"
		${EndIf}
	${EndIf}

	StrCpy $runningNppDetected "false" ; reset

	; Begin of "/closeRunningNpp"
	${GetParameters} $R0 
	${GetOptions} $R0 "/closeRunningNpp" $R1 ; case insensitive 
	IfErrors 0 closeRunningNppYes
	StrCpy $closeRunningNpp "false"
	Goto closeRunningNppCheckDone
closeRunningNppYes:
	StrCpy $closeRunningNpp "true"
closeRunningNppCheckDone:
	${If} $closeRunningNpp == "true"
		; First try to use the usual app-closing by sending the WM_CLOSE.
		; If that closing fails, use the forceful TerminateProcess way.
		!insertmacro FindAndCloseOrTerminateRunningNpp ; this has to precede the following silent mode Notepad++ instance mutex check
	${EndIf}

	; handle the possible Silent Mode (/S) & already running Notepad++ (without this an incorrect partial installation is possible)
	; A per-user install writes only into $LOCALAPPDATA\Notepad++ and never touches an
	; all-users notepad++.exe, so this mutex check only matters for an all-users install.
	IfSilent 0 notInSilentMode
	${If} $MultiUserInstallMode != "AllUsers"
		Goto notInSilentMode
	${EndIf}
	System::Call 'kernel32::OpenMutex(i 0x100000, b 0, t "nppInstance") i .R0'
	IntCmp $R0 0 nppNotRunning
	StrCpy $runningNppDetected "true"
	System::Call 'kernel32::CloseHandle(i $R0)' ; a Notepad++ instance is running, tidy-up the opened mutex handle only
	SetErrorLevel 5 ; set an exit code > 0 otherwise the installer returns 0 aka SUCCESS ('5' means here the future ERROR_ACCESS_DENIED when trying to overwrite the notepad++.exe file...)
	Quit ; silent installation is silent, we cannot continue here without a user interaction (or the installation should have been launched with the "/closeRunningNpp" param)
nppNotRunning:
notInSilentMode:
	; End of "/closeRunningNpp"

	; Begin of "/noUpdater"
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
	; End of "/noUpdater"

	; Begin of "/runNppAfterSilentInstall"
	${GetParameters} $R0 
	${GetOptions} $R0 "/runNppAfterSilentInstall" $R1 ;case insensitive 
	IfErrors noRunNpp runNpp
noRunNpp:
	StrCpy $runNppAfterSilentInstall "false"
	Goto runNppDone
runNpp:
	StrCpy $runNppAfterSilentInstall "true"
runNppDone:
	; End of "/runNppAfterSilentInstall"

	; Begin of "/relaunchNppAfterSilentInstall"
	${GetParameters} $R0 
	${GetOptions} $R0 "/relaunchNppAfterSilentInstall" $R1 ;case insensitive 
	IfErrors noRelaunchNpp relaunchNpp
noRelaunchNpp:
	StrCpy $relaunchNppAfterSilentInstall "false"
	Goto relaunchNppDone
relaunchNpp:
	StrCpy $relaunchNppAfterSilentInstall "true"
relaunchNppDone:
	; End of "/relaunchNppAfterSilentInstall"

	${If} ${SectionIsSelected} ${PluginsAdmin}
		!insertmacro SetSectionFlag ${AutoUpdater} ${SF_RO}
		!insertmacro SelectSection ${AutoUpdater}
	${Else}
		!insertmacro ClearSectionFlag ${AutoUpdater} ${SF_RO}
	${EndIf}

	Call SetRoughEstimation		; This is rough estimation of files present in function copyCommonFiles
	InitPluginsDir			; Initializes the plug-ins dir ($PLUGINSDIR) if not already initialized.
	Call checkCompatibility		; check unsupported OSes and CPUs
		
	; look for previously selected language (SHCTX follows the install mode: HKLM all-users / HKCU per-user)
	ClearErrors
	Var /GLOBAL tempLng
	ReadRegStr $tempLng SHCTX "SOFTWARE\${APPNAME}" 'InstallerLanguage'
	${IfNot} ${Errors}
		StrCpy $LANGUAGE "$tempLng" ; set default language
	${EndIf}
	
	!insertmacro MUI_LANGDLL_DISPLAY

	; save selected language to registry
	WriteRegStr SHCTX "SOFTWARE\${APPNAME}" 'InstallerLanguage' '$Language'

!ifdef ARCH64 || ARCHARM64 ; x64 or ARM64 : installation of 64 bits Notepad++ & its 64 bits components
	StrCpy $winSysDir $WINDIR\System32
	${If} ${RunningX64} ; Windows 64 bits
		; By default, regView value is 32.
		; But while installing Notepad++ x64 on 64-bits OS,
		; we disable registry redirection (enable access to 64-bit portion of registry)
		; regView value is set to 64 once for all.
		SetRegView 64

		; Pick the default install dir for the chosen mode, but only when $INSTDIR is still
		; the generic globalDef.nsh default (i.e. neither /D= nor a restored previous path).
		${If} "$InstDir" == "$PROGRAMFILES\${APPNAME}"
		${OrIf} "$InstDir" == ""
			${If} $MultiUserInstallMode == "CurrentUser"
				StrCpy $INSTDIR "$LOCALAPPDATA\${APPNAME}"
			${Else}
				StrCpy $INSTDIR "$PROGRAMFILES64\${APPNAME}"
			${EndIf}
		${EndIf}
		
		; check if 32-bit version has been installed if yes, ask user to remove it
		IfFileExists $PROGRAMFILES\${APPNAME}\notepad++.exe 0 noDelete32
		MessageBox MB_YESNO "You are trying to install 64-bit version while 32-bit version is already installed. Would you like to remove Notepad++ 32 bit version before proceeding further?$\n(Your custom config files will be kept)" /SD IDYES IDYES doDelete32 IDNO noDelete32 ;IDYES remove
doDelete32:
		StrCpy $diffArchDir2Remove $PROGRAMFILES\${APPNAME}
noDelete32:
		
	${Else} ; Windows 32 bits
		MessageBox MB_OK "You cannot install Notepad++ 64-bit version on your 32-bit system.$\nPlease download and install Notepad++ 32-bit version instead."
		Abort
	${EndIf}

!else ; installation of 32 bits Notepad++ & its 32 bits components
	StrCpy $winSysDir $WINDIR\SysWOW64
	; For a per-user install, default to a writable location instead of Program Files.
	${If} $MultiUserInstallMode == "CurrentUser"
		${If} "$InstDir" == "$PROGRAMFILES\${APPNAME}"
		${OrIf} "$InstDir" == ""
			StrCpy $INSTDIR "$LOCALAPPDATA\${APPNAME}"
		${EndIf}
	${EndIf}
	${If} ${RunningX64}  ; Windows 64 bits
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
	${If} $showDetailsChecked == ${BST_CHECKED}
		SetDetailsView show
		SetAutoClose false
	${endIf}

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
	SetOutPath "$INSTDIR\contextMenu\"
	
	IfFileExists $INSTDIR\contextmenu\NppShell.dll 0 +2
		ExecWait '"$winSysDir\rundll32.exe" "$INSTDIR\contextmenu\NppShell.dll",CleanupDll'

!ifdef ARCH64
	File /oname=$INSTDIR\contextMenu\NppShell.msix "..\bin64\NppShell.msix"
	File /oname=$INSTDIR\contextMenu\NppShell.dll "..\bin64\NppShell.x64.dll"
!else ifdef ARCHARM64
	File /oname=$INSTDIR\contextMenu\NppShell.msix "..\binarm64\NppShell.msix"
	File /oname=$INSTDIR\contextMenu\NppShell.dll "..\binarm64\NppShell.arm64.dll"
!else
	; We need to test which arch we are running on, since 32bit exe can be run on both 32bit and 64bit Windows.
	${If} ${RunningX64}
		; We are running on 64bit Windows, so we need the msix as well, since it might be Windows 11.
		File /oname=$INSTDIR\contextMenu\NppShell.msix "..\bin64\NppShell.msix"
		File /oname=$INSTDIR\contextMenu\NppShell.dll "..\bin64\NppShell.x64.dll"
	${Else}
		; We are running on 32bit Windows, so no need for the msix file, since there is no way this could even be upgraded to Windows 11.
		File /oname=$INSTDIR\contextMenu\NppShell.dll "..\bin\NppShell.x86.dll"
	${EndIf}
!endif

	; Shell context menu entry
	; HKCR == HKLM\Software\Classes; using SHCTX\Software\Classes routes to HKLM for an
	; all-users install and to HKCU\Software\Classes for a per-user install (no admin needed).
	WriteRegStr SHCTX "Software\Classes\*\shell\ANotepad++64" "" "Notepad++ Context menu"
	WriteRegStr SHCTX "Software\Classes\*\shell\ANotepad++64" "ExplorerCommandHandler" "{B298D29A-A6ED-11DE-BA8C-A68E55D89593}"
	WriteRegStr SHCTX "Software\Classes\*\shell\ANotepad++64" "NeverDefault" ""

	; CLSID registration
	WriteRegStr SHCTX "Software\Classes\CLSID\{B298D29A-A6ED-11DE-BA8C-A68E55D89593}" "" "notepad++"
	WriteRegStr SHCTX "Software\Classes\CLSID\{B298D29A-A6ED-11DE-BA8C-A68E55D89593}\InProcServer32" "" "$INSTDIR\contextMenu\NppShell.dll"
	WriteRegStr SHCTX "Software\Classes\CLSID\{B298D29A-A6ED-11DE-BA8C-A68E55D89593}\InProcServer32" "ThreadingModel" "Apartment"

	; Register MSIX for Windows 11 modern context menu
	; Skip only for x86 Notepad++ installation on Windows 32 system
!ifdef ARCH64 || ARCHARM64 ; x64 installer
	Call RegisterMSIX
!else ; 32 bits installer
	; NppShell's DllRegisterServer writes its keys under HKLM, so regsvr32 needs admin
	; rights and can only be used for an all-users install. A per-user install does not
	; need it: the entries above are written through SHCTX, which routes them to HKCU.
	${If} $MultiUserInstallMode == "AllUsers"
		ExecWait '"$winSysDir\regsvr32.exe" /s "$INSTDIR\contextMenu\NppShell.dll"'
	${EndIf}
!endif

${MementoSectionEnd}

${MementoSectionDone}

; Helper function for registering MSIX (Include the ExternalLocation flag for Sparse Packages)
Function RegisterMSIX
	; Windows 11 (build 22000+) is required for modern context menu via MSIX
	${If} ${AtLeastWin11}
		; Get PowerShell path from the Registry
		ReadRegStr $0 HKLM "SOFTWARE\Microsoft\PowerShell\1\ShellIds\Microsoft.PowerShell" "Path"

        ; Pass INSTDIR as an environment variable
        System::Call 'kernel32::SetEnvironmentVariableW(w "NPP_INSTDIR", w "$INSTDIR")'

		; Use PowerShell to register a modern Windows 11 right-click context menu silently
        nsExec::ExecToLog '"$0" -Command "Add-AppxPackage -Path \"$$env:NPP_INSTDIR\contextMenu\NppShell.msix\" -ExternalLocation \"$$env:NPP_INSTDIR\contextMenu\""'

		; Wait 2 seconds for the AppX service to finish indexing the new identity
		Sleep 2000

		; Notify the Shell (Association Change + Interrupt)
		System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
		System::Call 'shell32::SHChangeNotify(i 0x00008000, i 0, p 0, p 0)'

		; Broadcast the change
		SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:ShellState" /TIMEOUT=2000
	${EndIf}
FunctionEnd


;--------------------------------
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${explorerContextMenu} 'Explorer context menu entry for Notepad++ : Open whatever you want in Notepad++ from Windows Explorer.'
    !insertmacro MUI_DESCRIPTION_TEXT ${autoCompletionComponent} 'Install the API files you need for the auto-completion feature (Ctrl+Space).'
    !insertmacro MUI_DESCRIPTION_TEXT ${functionListComponent} 'Install the function list files you need for the function list feature (Ctrl+Space).'
    !insertmacro MUI_DESCRIPTION_TEXT ${Plugins} 'You may need these plugins to extend the capabilities of Notepad++.'
    !insertmacro MUI_DESCRIPTION_TEXT ${NppExport} 'Copy your syntax highlighted source code as HTML/RTF into clipboard, or save them as HTML/RTF files.'
    !insertmacro MUI_DESCRIPTION_TEXT ${MimeTools} 'Encode/decode selected text with Base64, Quoted-printable, URL encoding, and SAML.'
    !insertmacro MUI_DESCRIPTION_TEXT ${Converter} 'Convert ASCII to binary, octal, hexadecimal and decimal string.'
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
	IfSilent 0 theEnd
	${If} $runNppAfterSilentInstall == "true"
		Call LaunchNpp ; always launch
	${ElseIf} $relaunchNppAfterSilentInstall == "true"
		${If} $runningNppDetected == "true"
			Call LaunchNpp ; relaunch
		${EndIf}
	${EndIf}
theEnd:
SectionEnd

BrandingText "The best things in life are free. Notepad++ is free so Notepad++ is the best"

; eof
