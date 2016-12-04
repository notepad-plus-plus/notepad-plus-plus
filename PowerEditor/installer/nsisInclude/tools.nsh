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

Function LaunchNpp
  ;Exec '"$INSTDIR\notepad++.exe" "$INSTDIR\change.log" '

  ; create shortcut in temp with to launch as argument
  CreateShortCut "$TEMP\notepad++.lnk" "$INSTDIR\notepad++.exe" "$INSTDIR\change.log"

  ; Launch Notepad++ with same elevation as explorer
  ; It will solve the drag an drop issue if explorer is running with lower privilege
  Exec '"$WINDIR\explorer.exe" "$TEMP\notepad++.lnk"'

  ; Caution : Please DONOT delete $TEMP\notepad++.lnk right after Exec command
  ; In slower system it may take some time to open N++ while Exec command exist quickly
  ; Also putting sleep does not make sense here as
  ; 	- DONOT know how much to wait
  ;		- Keeps installer UI open wait time period
  ;		- If wait time is more, installer UI may become irresponsive

FunctionEnd

; Check if Notepad++ is running
; Created by Motaz Alnuweiri
; URL: http://nsis.sourceforge.net/Check_whether_your_application_is_running
;      http://nsis.sourceforge.net/Sharing_functions_between_Installer_and_Uninstaller

; Create CheckIfRunning shared function.
!macro CheckIfRunning un
	Function ${un}CheckIfRunning
		Check:
		System::Call 'kernel32::OpenMutex(i 0x100000, b 0, t "nppInstance") i .R0'
		
		IntCmp $R0 0 NotRunning
			System::Call 'kernel32::CloseHandle(i $R0)'
			MessageBox MB_RETRYCANCEL|MB_DEFBUTTON1|MB_ICONSTOP "Cannot continue the installation: Notepad++ is running.\
			          $\n$\n\
                      Please close Notepad++, then click ''Retry''." IDRETRY Retry IDCANCEL Cancel
			Retry:
				Goto Check
			
			Cancel:
				Quit
	
		NotRunning:
		
	FunctionEnd
!macroend


;Installer Functions
Var Dialog
Var NoUserDataCheckboxHandle
Var ShortcutCheckboxHandle
Var PluginLoadFromUserDataCheckboxHandle
Var WinVer

Function ExtraOptions
	nsDialogs::Create 1018
	Pop $Dialog

	${If} $Dialog == error
		Abort
	${EndIf}

	${NSD_CreateCheckbox} 0 0 100% 30u "Don't use %APPDATA%$\nEnable this option to make Notepad++ load/write the configuration files from/to its install directory. Check it if you use Notepad++ in an USB device."
	Pop $NoUserDataCheckboxHandle
	${NSD_OnClick} $NoUserDataCheckboxHandle OnChange_NoUserDataCheckBox
	
	${NSD_CreateCheckbox} 0 50 100% 30u "Allow plugins to be loaded from %APPDATA%\notepad++\plugins$\nIt could cause a security issue. Turn it on if you know what you are doing."
	Pop $PluginLoadFromUserDataCheckboxHandle
	${NSD_OnClick} $PluginLoadFromUserDataCheckboxHandle OnChange_PluginLoadFromUserDataCheckBox
	
	${NSD_CreateCheckbox} 0 110 100% 30u "Create Shortcut on Desktop"
	Pop $ShortcutCheckboxHandle
	StrCmp $WinVer "8" 0 +2
	${NSD_Check} $ShortcutCheckboxHandle
	${NSD_OnClick} $ShortcutCheckboxHandle OnChange_ShortcutCheckBox

	nsDialogs::Show
FunctionEnd

Function preventInstallInWin9x
	;Test if window9x
	${GetWindowsVersion} $WinVer
	
	StrCmp $WinVer "95" 0 +3
		MessageBox MB_OK "This version of Notepad++ does not support your OS.$\nPlease download zipped package of version 5.9 and use ANSI version. You can find v5.9 here:$\nhttp://notepad-plus-plus.org/release/5.9"
		Abort
		
	StrCmp $WinVer "98" 0 +3
		MessageBox MB_OK "This version of Notepad++ does not support your OS.$\nPlease download zipped package of version 5.9 and use ANSI version. You can find v5.9 here:$\nhttp://notepad-plus-plus.org/release/5.9"
		Abort
		
	StrCmp $WinVer "ME" 0 +3
		MessageBox MB_OK "This version of Notepad++ does not support your OS.$\nPlease download zipped package of version 5.9 and use ANSI version. You can find v5.9 here:$\nhttp://notepad-plus-plus.org/release/5.9"
		Abort
FunctionEnd

Var noUserDataChecked
Var allowPluginLoadFromUserDataChecked
Var createShortcutChecked

; The definition of "OnChange" event for checkbox
Function OnChange_NoUserDataCheckBox
	${NSD_GetState} $NoUserDataCheckboxHandle $noUserDataChecked
FunctionEnd

Function OnChange_PluginLoadFromUserDataCheckBox
	${NSD_GetState} $PluginLoadFromUserDataCheckboxHandle $allowPluginLoadFromUserDataChecked
FunctionEnd

Function OnChange_ShortcutCheckBox
	${NSD_GetState} $ShortcutCheckboxHandle $createShortcutChecked
FunctionEnd


Function writeInstallInfoInRegistry
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\notepad++.exe" "" "$INSTDIR\notepad++.exe"
	
	WriteRegStr HKLM "Software\${APPNAME}" "" "$INSTDIR"
	!ifdef ARCH64
		WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "DisplayName" "${APPNAME} (64-bit x64)"
	!else
		WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "DisplayName" "${APPNAME} (32-bit x86)"
	!endif
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "Publisher" "Notepad++ Team"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "MajorVersion" "${VERSION_MAJOR}"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "MinorVersion" "${VERSION_MINOR}"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "DisplayIcon" "$INSTDIR\notepad++.exe"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "DisplayVersion" "${APPVERSION}"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "URLInfoAbout" "${APPWEBSITE}"
	WriteRegDWORD HKLM "${UNINSTALL_REG_KEY}" "VersionMajor" ${VERSION_MAJOR}
	WriteRegDWORD HKLM "${UNINSTALL_REG_KEY}" "VersionMinor" ${VERSION_MINOR}
	WriteRegDWORD HKLM "${UNINSTALL_REG_KEY}" "NoModify" 1
	WriteRegDWORD HKLM "${UNINSTALL_REG_KEY}" "NoRepair" 1
	
	${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
	IfErrors +3 0
	IntFmt $0 "0x%08X" $0
	WriteRegDWORD HKLM "${UNINSTALL_REG_KEY}" "EstimatedSize" "$0"
	
	WriteUninstaller "$INSTDIR\uninstall.exe"
FunctionEnd

