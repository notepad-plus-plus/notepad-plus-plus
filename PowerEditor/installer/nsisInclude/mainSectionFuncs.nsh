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

Var UPDATE_PATH
Var PLUGIN_INST_PATH
Var USER_PLUGIN_CONF_PATH
Var ALLUSERS_PLUGIN_CONF_PATH
Function setPathAndOptions
    ${If} $UPDATE_PATH == ""
	${OrIf} $PLUGIN_INST_PATH == ""
	${OrIf} $USER_PLUGIN_CONF_PATH == ""
	${OrIf} $ALLUSERS_PLUGIN_CONF_PATH == ""
        Goto initUpdatePath
	${ELSE}
        Goto alreadyDone
	${EndIf}
	
initUpdatePath:
    
	; Set Section properties
	SetOverwrite on
	StrCpy $UPDATE_PATH $INSTDIR
		
	SetOutPath "$INSTDIR\"

	StrCpy $PLUGIN_INST_PATH "$INSTDIR\plugins"
	StrCpy $ALLUSERS_PLUGIN_CONF_PATH "$PLUGIN_INST_PATH\Config"
	
	${If} $noUserDataChecked == ${BST_CHECKED}

		File "..\bin\doLocalConf.xml"
		StrCpy $USER_PLUGIN_CONF_PATH "$ALLUSERS_PLUGIN_CONF_PATH"
		CreateDirectory $PLUGIN_INST_PATH\config
	${ELSE}
	
		IfFileExists $INSTDIR\doLocalConf.xml 0 +2
		Delete $INSTDIR\doLocalConf.xml
		
		StrCpy $USER_PLUGIN_CONF_PATH "$APPDATA\${APPNAME}\plugins\Config"
		StrCpy $UPDATE_PATH "$APPDATA\${APPNAME}"
		CreateDirectory $UPDATE_PATH\plugins\config
	${EndIf}
	
	; WriteIniStr "$INSTDIR\uninstall.ini" "Uninstall" "UPDATE_PATH" $UPDATE_PATH 
	; WriteIniStr "$INSTDIR\uninstall.ini" "Uninstall" "PLUGIN_INST_PATH" $PLUGIN_INST_PATH 
	; WriteIniStr "$INSTDIR\uninstall.ini" "Uninstall" "USER_PLUGIN_CONF_PATH" $USER_PLUGIN_CONF_PATH 
	; WriteIniStr "$INSTDIR\uninstall.ini" "Uninstall" "ALLUSERS_PLUGIN_CONF_PATH" $ALLUSERS_PLUGIN_CONF_PATH 

alreadyDone:
FunctionEnd

Function un.setPathAndOptions
    ReadINIStr $UPDATE_PATH "$INSTDIR\uninstall.ini" "Uninstall" "UPDATE_PATH" 
    ReadINIStr $PLUGIN_INST_PATH "$INSTDIR\uninstall.ini" "Uninstall" "PLUGIN_INST_PATH" 
    ReadINIStr $USER_PLUGIN_CONF_PATH "$INSTDIR\uninstall.ini" "Uninstall" "USER_PLUGIN_CONF_PATH" 
    ReadINIStr $ALLUSERS_PLUGIN_CONF_PATH "$INSTDIR\uninstall.ini" "Uninstall" "ALLUSERS_PLUGIN_CONF_PATH" 
FunctionEnd


Function copyCommonFiles
	SetOverwrite off
	SetOutPath "$UPDATE_PATH\"
	File "..\bin\contextMenu.xml"
	
	SetOverwrite on
	SetOutPath "$INSTDIR\"
	File "..\bin\langs.model.xml"
	File "..\bin\stylers.model.xml"
	File "..\bin\contextMenu.xml"

	SetOverwrite off
	File "..\bin\shortcuts.xml"
	

	
	; Set Section Files and Shortcuts
	SetOverwrite on
	File "..\..\LICENSE"
	File "..\bin\change.log"
	File "..\bin\readme.txt"
	
!ifdef ARCH64
	File "..\bin64\notepad++.exe"
!else ifdef ARCHARM64
	File "..\binarm64\notepad++.exe"
!else
	File "..\bin\notepad++.exe"
!endif

	; Markdown in user defined languages
	SetOutPath "$UPDATE_PATH\userDefineLangs\"
	Delete "$UPDATE_PATH\userDefineLangs\userDefinedLang-markdown.default.modern.xml"
	File "..\bin\userDefineLangs\markdown._preinstalled.udl.xml"
	File "..\bin\userDefineLangs\markdown._preinstalled_DM.udl.xml"

	; Localization
	; Default language English 
	SetOutPath "$INSTDIR\localization\"
	File ".\nativeLang\english.xml"

	; Copy all the language files to the temp directory
	; than make them installed via option
	SetOutPath "$PLUGINSDIR\nppLocalization\"
	File ".\nativeLang\"

	IfFileExists "$UPDATE_PATH\nativeLang.xml" 0 +2
		Delete "$UPDATE_PATH\nativeLang.xml"
		
	IfFileExists "$INSTDIR\nativeLang.xml" 0 +2
		Delete "$INSTDIR\nativeLang.xml"

	StrCmp $LANGUAGE ${LANG_ENGLISH} +5 0
	CopyFiles "$PLUGINSDIR\nppLocalization\$(langFileName)" "$UPDATE_PATH\nativeLang.xml"
	CopyFiles "$PLUGINSDIR\nppLocalization\$(langFileName)" "$INSTDIR\localization\$(langFileName)"
	IfFileExists "$PLUGINSDIR\gupLocalization\$(langFileName)" 0 +2
		CopyFiles "$PLUGINSDIR\gupLocalization\$(langFileName)" "$INSTDIR\updater\nativeLang.xml"
FunctionEnd

; Source from: https://nsis.sourceforge.io/VersionCompare
Function VersionCompare
	!define VersionCompare `!insertmacro VersionCompareCall`
 
	!macro VersionCompareCall _VER1 _VER2 _RESULT
		Push `${_VER1}`
		Push `${_VER2}`
		Call VersionCompare
		Pop ${_RESULT}
	!macroend
 
	Exch $1
	Exch
	Exch $0
	Exch
	Push $2
	Push $3
	Push $4
	Push $5
	Push $6
	Push $7
 
	begin:
	StrCpy $2 -1
	IntOp $2 $2 + 1
	StrCpy $3 $0 1 $2
	StrCmp $3 '' +2
	StrCmp $3 '.' 0 -3
	StrCpy $4 $0 $2
	IntOp $2 $2 + 1
	StrCpy $0 $0 '' $2
 
	StrCpy $2 -1
	IntOp $2 $2 + 1
	StrCpy $3 $1 1 $2
	StrCmp $3 '' +2
	StrCmp $3 '.' 0 -3
	StrCpy $5 $1 $2
	IntOp $2 $2 + 1
	StrCpy $1 $1 '' $2
 
	StrCmp $4$5 '' equal
 
	StrCpy $6 -1
	IntOp $6 $6 + 1
	StrCpy $3 $4 1 $6
	StrCmp $3 '0' -2
	StrCmp $3 '' 0 +2
	StrCpy $4 0
 
	StrCpy $7 -1
	IntOp $7 $7 + 1
	StrCpy $3 $5 1 $7
	StrCmp $3 '0' -2
	StrCmp $3 '' 0 +2
	StrCpy $5 0
 
	StrCmp $4 0 0 +2
	StrCmp $5 0 begin newer2
	StrCmp $5 0 newer1
	IntCmp $6 $7 0 newer1 newer2
 
	StrCpy $4 '1$4'
	StrCpy $5 '1$5'
	IntCmp $4 $5 begin newer2 newer1
 
	equal:
	StrCpy $0 0
	goto end
	newer1:
	StrCpy $0 1
	goto end
	newer2:
	StrCpy $0 2
 
	end:
	Pop $7
	Pop $6
	Pop $5
	Pop $4
	Pop $3
	Pop $2
	Pop $1
	Exch $0
FunctionEnd

Function removeUnstablePlugins
	; remove unstable plugins
	CreateDirectory "$INSTDIR\plugins\disabled"
	
	; NppSaveAsAdmin makes Notepad++ crash. "1.0.211.0" is its 1st version which contains the fix
	IfFileExists "$INSTDIR\plugins\NppSaveAsAdmin\NppSaveAsAdmin.dll" 0 NppSaveAsAdminTestEnd
		${GetFileVersion} "$INSTDIR\plugins\NppSaveAsAdmin\NppSaveAsAdmin.dll" $R0
		${VersionCompare} $R0 "1.0.211.0" $R1 ;   0: equal to 1.0.211.0   1: $R0 is newer   2: 1.0.211.0 is newer
		StrCmp $R1 "0" +5 0 ; if equal skip all & go to end, else go to next
		StrCmp $R1 "1" +4 0 ; if newer skip all & go to end, else older (2) then go to next
		MessageBox MB_OK "Due to NppSaveAsAdmin plugin's incompatibility issue in version $R0, NppSaveAsAdmin.dll will be deleted. Use Plugins Admin to add back (the latest version of) NppSaveAsAdmin." /SD IDOK
		Rename "$INSTDIR\plugins\NppSaveAsAdmin\NppSaveAsAdmin.dll" "$INSTDIR\plugins\disabled\NppSaveAsAdmin.dll"
		Delete "$INSTDIR\plugins\NppSaveAsAdmin\NppSaveAsAdmin.dll"
	NppSaveAsAdminTestEnd:
	
	; https://github.com/chcg/NPP_HexEdit/issues/51
	IfFileExists "$INSTDIR\plugins\HexEditor\HexEditor.dll" 0 HexEditorTestEnd
		MessageBox MB_OK "Due to HexEditor plugin's crash issue on Notepad++ v8 (and later versions), HexEditor.dll will be removed." /SD IDOK
		Rename "$INSTDIR\plugins\HexEditor\HexEditor.dll" "$INSTDIR\plugins\disabled\HexEditor.dll"
		Delete "$INSTDIR\plugins\HexEditor\HexEditor.dll"
	HexEditorTestEnd:
	
FunctionEnd

Function removeOldContextMenu
   ; Context Menu Management : removing old version of Context Menu module
	IfFileExists "$INSTDIR\nppcm.dll" 0 +3
		Exec 'regsvr32 /u /s "$INSTDIR\nppcm.dll"'
		Delete "$INSTDIR\nppcm.dll"
        
    IfFileExists "$INSTDIR\NppShell.dll" 0 +3
		Exec 'regsvr32 /u /s "$INSTDIR\NppShell.dll"'
		Delete "$INSTDIR\NppShell.dll"
		
    IfFileExists "$INSTDIR\NppShell_01.dll" 0 +3
		Exec 'regsvr32 /u /s "$INSTDIR\NppShell_01.dll"'
		Delete "$INSTDIR\NppShell_01.dll"
        
    IfFileExists "$INSTDIR\NppShell_02.dll" 0 +3
		Exec 'regsvr32 /u /s "$INSTDIR\NppShell_02.dll"'
		Delete "$INSTDIR\NppShell_02.dll"
		
    IfFileExists "$INSTDIR\NppShell_03.dll" 0 +3
		Exec 'regsvr32 /u /s "$INSTDIR\NppShell_03.dll"'
		Delete "$INSTDIR\NppShell_03.dll"
		
	IfFileExists "$INSTDIR\NppShell_04.dll" 0 +3
		Exec 'regsvr32 /u /s "$INSTDIR\NppShell_04.dll"'
		Delete "$INSTDIR\NppShell_04.dll"
		
	IfFileExists "$INSTDIR\NppShell_05.dll" 0 +3
		Exec 'regsvr32 /u /s "$INSTDIR\NppShell_05.dll"'
		Delete "$INSTDIR\NppShell_05.dll"
FunctionEnd

Function shortcutLinkManagement
	; remove all the npp shortcuts from current user
	Delete "$DESKTOP\Notepad++.lnk"
	Delete "$SMPROGRAMS\Notepad++.lnk"
	Delete "$SMPROGRAMS\${APPNAME}\Notepad++.lnk"
	Delete "$SMPROGRAMS\${APPNAME}\readme.lnk"
	Delete "$SMPROGRAMS\${APPNAME}\Uninstall.lnk"
	RMDir "$SMPROGRAMS\${APPNAME}"
		
	; detect the right of 
	UserInfo::GetAccountType
	Pop $1
	StrCmp $1 "Admin" 0 +2
	SetShellVarContext all
	
	; set the shortcuts working directory
	; http://nsis.sourceforge.net/Docs/Chapter4.html#createshortcut
	SetOutPath "$INSTDIR\"
	
	; add all the npp shortcuts for all user or current user
	CreateShortCut "$SMPROGRAMS\Notepad++.lnk" "$INSTDIR\notepad++.exe"
	${If} $createShortcutChecked == ${BST_CHECKED}
		CreateShortCut "$DESKTOP\Notepad++.lnk" "$INSTDIR\notepad++.exe"
	${EndIf}
	
	SetShellVarContext current
FunctionEnd


