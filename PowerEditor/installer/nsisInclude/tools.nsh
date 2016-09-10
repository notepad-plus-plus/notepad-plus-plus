


Function LaunchNpp
  Exec '"$INSTDIR\notepad++.exe" "$INSTDIR\change.log" '
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
Var OldIconCheckboxHandle
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

	${NSD_CreateCheckbox} 0 170 100% 30u "Use the old, obsolete and monstrous icon$\nI won't blame you if you want to get the old icon back :)"
	Pop $OldIconCheckboxHandle
	${NSD_OnClick} $OldIconCheckboxHandle OnChange_OldIconCheckBox
	
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
Var isOldIconChecked


; TODO for optional arg
;Var params

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

Function OnChange_OldIconCheckBox
	${NSD_GetState} $OldIconCheckboxHandle $isOldIconChecked
FunctionEnd

Function writeInstallInfoInRegistry
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\notepad++.exe" "" "$INSTDIR\notepad++.exe"
	
	WriteRegStr HKLM "Software\${APPNAME}" "" "$INSTDIR"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "DisplayName" "${APPNAME}"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "Publisher" "Notepad++ Team"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "VersionMajor" "${VERSION_MAJOR}"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "VersionMinor" "${VERSION_MINOR}"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "MajorVersion" "${VERSION_MAJOR}"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "MinorVersion" "${VERSION_MINOR}"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "DisplayIcon" "$INSTDIR\notepad++.exe"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "DisplayVersion" "${APPVERSION}"
	WriteRegStr HKLM "${UNINSTALL_REG_KEY}" "URLInfoAbout" "${APPWEBSITE}"
	WriteUninstaller "$INSTDIR\uninstall.exe"
FunctionEnd

