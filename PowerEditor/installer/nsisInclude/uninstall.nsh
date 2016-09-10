
Section un.explorerContextMenu
	Exec 'regsvr32 /u /s "$INSTDIR\NppShell_01.dll"'
	Exec 'regsvr32 /u /s "$INSTDIR\NppShell_02.dll"'
	Exec 'regsvr32 /u /s "$INSTDIR\NppShell_03.dll"'
	Exec 'regsvr32 /u /s "$INSTDIR\NppShell_04.dll"'
	Exec 'regsvr32 /u /s "$INSTDIR\NppShell_05.dll"'
	Exec 'regsvr32 /u /s "$INSTDIR\NppShell_06.dll"'
	Delete "$INSTDIR\NppShell_01.dll"
	Delete "$INSTDIR\NppShell_02.dll"
	Delete "$INSTDIR\NppShell_03.dll"
	Delete "$INSTDIR\NppShell_04.dll"
	Delete "$INSTDIR\NppShell_05.dll"
	Delete "$INSTDIR\NppShell_06.dll"
SectionEnd

Section un.UnregisterFileExt
	; Remove references to "Notepad++_file"
	IntOp $1 0 + 0	; subkey index
	StrCpy $2 ""	; subkey name
Enum_HKCR_Loop:
	EnumRegKey $2 HKCR "" $1
	StrCmp $2 "" Enum_HKCR_Done
	ReadRegStr $0 HKCR $2 ""	; Read the default value
	${If} $0 == "Notepad++_file"
		ReadRegStr $3 HKCR $2 "Notepad++_backup"
		; Recover (some of) the lost original file types
		${If} $3 == "Notepad++_file"
			${If} $2 == ".ini"
				StrCpy $3 "inifile"
			${ElseIf} $2 == ".inf"
				StrCpy $3 "inffile"
			${ElseIf} $2 == ".nfo"
				StrCpy $3 "MSInfoFile"
			${ElseIf} $2 == ".txt"
				StrCpy $3 "txtfile"
			${ElseIf} $2 == ".log"
				StrCpy $3 "txtfile"
			${ElseIf} $2 == ".xml"
				StrCpy $3 "xmlfile"
			${EndIf}
		${EndIf}
		${If} $3 == "Notepad++_file"
			; File type recovering has failed. Just discard the current file extension
			DeleteRegKey HKCR $2
		${Else}
			; Restore the original file type
			WriteRegStr HKCR $2 "" $3
			DeleteRegValue HKCR $2 "Notepad++_backup"
			IntOp $1 $1 + 1
		${EndIf}
	${Else}
		IntOp $1 $1 + 1
	${EndIf}
	Goto Enum_HKCR_Loop
Enum_HKCR_Done:

	; Remove references to "Notepad++_file" from "Open with..."
	IntOp $1 0 + 0	; subkey index
	StrCpy $2 ""	; subkey name
Enum_FileExts_Loop:
	EnumRegKey $2 HKCU "SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\FileExts" $1
	StrCmp $2 "" Enum_FileExts_Done
	DeleteRegValue HKCU "SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\FileExts\$2\OpenWithProgids" "Notepad++_file"
	IntOp $1 $1 + 1
	Goto Enum_FileExts_Loop
Enum_FileExts_Done:

	; Remove "Notepad++_file" file type
	DeleteRegKey HKCR "Notepad++_file"
SectionEnd

Section un.UserManual
	RMDir /r "$INSTDIR\user.manual"
SectionEnd

Section Uninstall
	;Remove from registry...
	DeleteRegKey HKLM "${UNINSTALL_REG_KEY}"
	DeleteRegKey HKLM "SOFTWARE\${APPNAME}"
	DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\notepad++.exe"

	; Delete self
	Delete "$INSTDIR\uninstall.exe"

	; Delete Shortcuts
	Delete "$SMPROGRAMS\Notepad++\Uninstall.lnk"
	RMDir "$SMPROGRAMS\Notepad++"
	
	UserInfo::GetAccountType
	Pop $1
	StrCmp $1 "Admin" 0 +2
		SetShellVarContext all
	
	Delete "$DESKTOP\Notepad++.lnk"
	Delete "$SMPROGRAMS\Notepad++\Notepad++.lnk"
	Delete "$SMPROGRAMS\Notepad++\readme.lnk"
	

	; Clean up Notepad++
	Delete "$INSTDIR\LINEDRAW.TTF"
	Delete "$INSTDIR\SciLexer.dll"
	Delete "$INSTDIR\change.log"
	Delete "$INSTDIR\LICENSE"

	Delete "$INSTDIR\notepad++.exe"
	Delete "$INSTDIR\readme.txt"
	
	Delete "$INSTDIR\config.xml"
	Delete "$INSTDIR\config.model.xml"
	Delete "$INSTDIR\langs.xml"
	Delete "$INSTDIR\langs.model.xml"
	Delete "$INSTDIR\stylers.xml"
	Delete "$INSTDIR\stylers.model.xml"
	Delete "$INSTDIR\stylers_remove.xml"
	Delete "$INSTDIR\contextMenu.xml"
	Delete "$INSTDIR\shortcuts.xml"
	Delete "$INSTDIR\functionList.xml"
	Delete "$INSTDIR\nativeLang.xml"
	Delete "$INSTDIR\session.xml"
	Delete "$INSTDIR\localization\english.xml"
	Delete "$INSTDIR\SourceCodePro-Regular.ttf"
	Delete "$INSTDIR\SourceCodePro-Bold.ttf"
	Delete "$INSTDIR\SourceCodePro-It.ttf"
	Delete "$INSTDIR\SourceCodePro-BoldIt.ttf"
	
	SetShellVarContext current
	Delete "$APPDATA\Notepad++\langs.xml"
	Delete "$APPDATA\Notepad++\config.xml"
	Delete "$APPDATA\Notepad++\stylers.xml"
	Delete "$APPDATA\Notepad++\contextMenu.xml"
	Delete "$APPDATA\Notepad++\shortcuts.xml"
	Delete "$APPDATA\Notepad++\functionList.xml"
	Delete "$APPDATA\Notepad++\nativeLang.xml"
	Delete "$APPDATA\Notepad++\session.xml"
	Delete "$APPDATA\Notepad++\insertExt.ini"
	IfFileExists  "$INSTDIR\NppHelp.chm" 0 +2
		Delete "$INSTDIR\NppHelp.chm"

	RMDir "$APPDATA\Notepad++"
	
	StrCmp $1 "Admin" 0 +2
		SetShellVarContext all
		
	; Remove remaining directories
	RMDir /r "$INSTDIR\plugins\disabled\"
	RMDir "$INSTDIR\plugins\APIs\"
	RMDir "$INSTDIR\plugins\"
	RMDir "$INSTDIR\themes\"
	RMDir "$INSTDIR\localization\"
	RMDir "$INSTDIR\"
	RMDir "$SMPROGRAMS\Notepad++"
	RMDir "$APPDATA\Notepad++"

SectionEnd

Function un.onInit
  ;!insertmacro MUI_UNGETLANGUAGE
FunctionEnd
