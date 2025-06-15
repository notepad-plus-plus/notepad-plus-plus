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


Function LaunchNpp
  ; Open notepad instance with same integrity level as explorer,
  ; so that drag n drop continue to function even
  ; Once npp is launched, show change.log file (this is to handle issues #2896, #2979, #3014)
  ; Caveats:
  ;		1. If launching npp takes more time (which is rare), changelog will not be shown
  ;		2. If previous npp is configured as "Always in multi-instance mode", then
  ;			a. Two npp instances will be opened which is not expected
  ;			b. Second instance may not support drag n drop if current user's integrity level is not as admin
  Exec '"$WINDIR\explorer.exe" "$INSTDIR\notepad++.exe"'

  ; Max 5 seconds wait here to open change.log
  ; If npp is not available even after 5 seconds, exit without showing change.log

  ${ForEach} $R1 1 5 + 1				; Loop to find opened Npp instance
	System::Call 'kernel32::OpenMutex(i 0x100000, b 0, t "nppInstance") i .R0'
	IntCmp $R0 0 NotYetExecuted
		System::Call 'kernel32::CloseHandle(i $R0)'
		Exec '"$INSTDIR\notepad++.exe" "$INSTDIR\change.log" '
		${Break}
	NotYetExecuted:
		Sleep 1000
  ${Next}
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
			StrCpy $runningNppDetected "true"
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
Var ShowDetailsCheckboxHandle
Var WinVer
Var noUserDataChecked
Var createShortcutChecked
Var showDetailsChecked

; The definition of "OnChange" event for checkbox
Function OnChange_NoUserDataCheckBox
	${NSD_GetState} $NoUserDataCheckboxHandle $noUserDataChecked
FunctionEnd

Function OnChange_ShortcutCheckBox
	${NSD_GetState} $ShortcutCheckboxHandle $createShortcutChecked
FunctionEnd

Function OnChange_ShowDetailsCheckbox
	${NSD_GetState} $ShowDetailsCheckboxHandle $showDetailsChecked
FunctionEnd

Function ExtraOptions
	nsDialogs::Create 1018
	Pop $Dialog

	${If} $Dialog == error
		Abort
	${EndIf}

	${NSD_CreateCheckbox} 0 0 100% 30u "Create Shortcut on Desktop"
	Pop $ShortcutCheckboxHandle
	StrCmp $WinVer "8" 0 +2
	${NSD_Check} $ShortcutCheckboxHandle
	${NSD_OnClick} $ShortcutCheckboxHandle OnChange_ShortcutCheckBox

	${NSD_CreateCheckbox} 0 80 100% 30u "Don't use %APPDATA%$\nEnable this option to make Notepad++ load/write the configuration files from/to its install directory. Check it if you use Notepad++ in a USB device."
	Pop $NoUserDataCheckboxHandle
	IfFileExists $INSTDIR\doLocalConf.xml doLocalConfExists doLocalConfDoesNotExists
	doLocalConfExists:
		 ; a previous portable N++ installation detected
		${NSD_SetState} $NoUserDataCheckboxHandle ${BST_CHECKED}
		StrCpy $noUserDataChecked ${BST_CHECKED}
	doLocalConfDoesNotExists:
	${NSD_OnClick} $NoUserDataCheckboxHandle OnChange_NoUserDataCheckBox

	${NSD_CreateCheckbox} 0 160 100% 30u "Show installation details"
	Pop $ShowDetailsCheckboxHandle
	${NSD_OnClick} $ShowDetailsCheckboxHandle OnChange_ShowDetailsCheckbox

	StrLen $0 $PROGRAMFILES
	StrCpy $1 $InstDir $0

	StrLen $0 $PROGRAMFILES64
	StrCpy $2 $InstDir $0
	${If} $1 == "$PROGRAMFILES"
	${ORIF} $2 == "$PROGRAMFILES64"
		${NSD_Uncheck} $NoUserDataCheckboxHandle
		EnableWindow $NoUserDataCheckboxHandle 0
	${Else}
		EnableWindow $NoUserDataCheckboxHandle 1
	${EndIf}
	nsDialogs::Show
FunctionEnd

Function checkCompatibility

	${GetWindowsVersion} $WinVer
	
	StrCmp $WinVer "95" 0 +3
		MessageBox MB_OK|MB_ICONSTOP "Notepad++ does not support your OS. The installation will be aborted."
		Abort
		
	StrCmp $WinVer "98" 0 +3
		MessageBox MB_OK|MB_ICONSTOP "Notepad++ does not support your OS. The installation will be aborted."
		Abort
		
	StrCmp $WinVer "ME" 0 +3
		MessageBox MB_OK|MB_ICONSTOP "Notepad++ does not support your OS. The installation will be aborted."
		Abort
		
	StrCmp $WinVer "2000" 0 +3 ; Windows 2000
		MessageBox MB_OK|MB_ICONSTOP "Notepad++ does not support your OS. The installation will be aborted."
		Abort
		
	StrCmp $WinVer "XP" 0 xp_endTest ; XP
		MessageBox MB_YESNO|MB_ICONSTOP "This version of Notepad++ doesn't support Windows XP. The installation will be aborted.$\n$\nDo you want to go to Notepad++ download page for downloading the last version which supports XP (v7.9.2)?" IDYES xp_openDlPage IDNO xp_goQuit
xp_openDlPage:
		ExecShell "open" "https://notepad-plus-plus.org/downloads/v7.9.2/"
xp_goQuit:
		Abort
xp_endTest:
		
	StrCmp $WinVer "2003" 0 ws2003_endTest ; Windows Server 2003
		MessageBox MB_YESNO|MB_ICONSTOP "This version of Notepad++ doesn't support Windows Server 2003. The installation will be aborted.$\n$\nDo you want to go to Notepad++ download page for downloading the last version which supports this OS?" IDYES ws2003_openDlPage IDNO ws2003_goQuit
ws2003_openDlPage:
		ExecShell "open" "https://notepad-plus-plus.org/downloads/v7.9.2/"
ws2003_goQuit:
		Abort
ws2003_endTest:

!ifdef ARCHARM64
	${If} ${IsNativeARM64}
		; OK
	${Else}
		; we cannot run ARM64 binaries on a x86/x64 CPU (the other way around is possible - x86 on ARM64 CPU)
		MessageBox MB_YESNO|MB_ICONSTOP "This installer contains ARM64 version of Notepad++ incompatible with your computer processor running, so the installation will be aborted.$\n$\nDo you want to go to the Notepad++ site to download a compatible (x86/x64) installer instead?" IDYES arm64_openDlPage IDNO arm64_goQuit
arm64_openDlPage:
		ExecShell "open" "https://notepad-plus-plus.org/downloads/"
arm64_goQuit:
		Abort
	${EndIf}
!endif

FunctionEnd


Function writeInstallInfoInRegistry
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\notepad++.exe" "" "$INSTDIR\notepad++.exe"
	
	WriteRegStr HKLM "Software\${APPNAME}" "" "$INSTDIR"
	!ifdef ARCH64
		WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "DisplayName" "${APPNAME} (64-bit x64)"
	!else ifdef ARCHARM64
		WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "DisplayName" "${APPNAME} (ARM 64-bit)"
	!else
		WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "DisplayName" "${APPNAME} (32-bit x86)"
	!endif
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "Publisher" "Notepad++ Team"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "MajorVersion" "${VERSION_MAJOR}"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "MinorVersion" "${VERSION_MINOR}"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
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


!define RUNPROC_WND_CLASS "Notepad++"
!define RUNPROC_WAIT_FOR_EXIT_MAX_MS 5000 ;  5 seconds max
!define RUNPROC_SYNC_TERM 0x00100001 ; dwDesiredAccess ... PROCESS_TERMINATE | SYNCHRONIZE
!include WinMessages.nsh

!macro FindAndCloseOrTerminateRunningNpp
	; to not influence the global NSIS vars used here, push them on the stack and pop them out at the end 
	Push $0 ; running process main HWND
	Push $1 ; result of the waiting for the running process object
	Push $2 ; running process HANDLE
	Push $3 ; possible WIN32 error code (the GetLastError() result)

  findRunningProcessByClassName:
	FindWindow $0 '${RUNPROC_WND_CLASS}' ''
	IntPtrCmp $0 0 processNotRunning
	IsWindow $0 0 processNotRunning

	StrCpy $runningNppDetected "true"
	IfSilent skipDetailPrint 0
	DetailPrint "Closing the ${RUNPROC_WND_CLASS} app running..."
  skipDetailPrint:

	System::Call 'user32.dll::GetWindowThreadProcessId(i r0, *i .r1) i .r2'
	System::Call 'kernel32.dll::OpenProcess(i ${RUNPROC_SYNC_TERM}, i 0, i r1) p .r2 ?e' ; ?e ... the NSIS system plugin will additionally put the GetLastError() code on top of the stack
	pop $3 ; a possible WIN32 error code will be here
	IntPtrCmp $2 0 openProcessFail

	System::Call 'user32.dll::PostMessage(i $0, i ${WM_CLOSE}, i 0, i 0)'
	System::Call 'kernel32.dll::WaitForSingleObject(i r2, i ${RUNPROC_WAIT_FOR_EXIT_MAX_MS}) i .r1'
	IntCmp $1 0 closeProcessHandle ; 0 == WAIT_OBJECT_0 (signaled state of the process to close...)

	; process could not be stopped by the usual WM_CLOSE way, so use a hard termination instead
	IfSilent terminateProcess 0
	MessageBox MB_YESNOCANCEL|MB_ICONEXCLAMATION "Installer cannot stop the running ${RUNPROC_WND_CLASS} by usual closing request.$\n$\nDo you want to forcefully terminate that process?" /SD IDYES IDYES terminateProcess IDNO closeProcessHandle
	; cancel was selected, so close the opened running process handle and quit immediately
	System::Call 'kernel32.dll::CloseHandle(i r2) i .r1'
	SetErrorLevel 5 ; set an exit code > 0 otherwise the installer returns 0 aka SUCCESS (5 == ERROR_ACCESS_DENIED)
	Quit ; installer will end

  terminateProcess:
	System::Call 'kernel32.dll::TerminateProcess(i r2, i 0) i .r1'

  closeProcessHandle:
	System::Call 'kernel32.dll::CloseHandle(i r2) i .r1'
	goto findRunningProcessByClassName ; loop, we need to check for all the possible instances of the process running

  openProcessFail:
	IfSilent skipOpenProcessFailMessage 0
	MessageBox MB_OK|MB_ICONSTOP "Installer cannot stop the running ${RUNPROC_WND_CLASS}.\n\nOpenProcess WINAPI failed! (error code: $3)"
  skipOpenProcessFailMessage:
	SetErrorLevel $3 ; set an exit code > 0 otherwise the installer returns 0 aka SUCCESS
	Quit ; installer will end

  processNotRunning:
	Pop $3
	Pop $2
	Pop $1
	Pop $0
!macroend
