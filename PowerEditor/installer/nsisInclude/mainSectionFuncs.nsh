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
	
	; in Silent mode we cannot use the NSIS GUI for handling the doLocalConf mode
	; but we need to directly use the previous user setting
	IfSilent 0 +3
	IfFileExists $INSTDIR\doLocalConf.xml 0 +2
	StrCpy $noUserDataChecked ${BST_CHECKED}

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
	File "..\src\tabContextMenu_example.xml"
	File "..\src\toolbarButtonsConf_example.xml"
	File "..\src\toolbarIcons.xml"

	SetOverwrite on
	SetOutPath "$INSTDIR\"
	File "..\bin\langs.model.xml"
	File "..\bin\stylers.model.xml"
	File "..\bin\contextMenu.xml"

	SetOverwrite off
	File "..\bin\shortcuts.xml"
	
	; For debug logs
	File "..\bin\nppLogNulContentCorruptionIssue.xml"

	
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

	; Copy all the Notepad++ localization files to the temp directory
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

!ifdef ARCH64 || ARCHARM64 ; x64 or ARM64

	; HexEditor makes Notepad++ x64 crash. "0.9.12" is its 1st version which contains the fix
	IfFileExists "$INSTDIR\plugins\HexEditor\HexEditor.dll" 0 HexEditorTestEnd64
		${GetFileVersion} "$INSTDIR\plugins\HexEditor\HexEditor.dll" $R0
		${VersionCompare} $R0 "0.9.12" $R1 ;   0: equal to 0.9.12   1: $R0 is newer   2: 0.9.12 is newer
		StrCmp $R1 "0" +5 0 ; if equal skip all & go to end, else go to next
		StrCmp $R1 "1" +4 0 ; if newer skip all & go to end, else older (2) then go to next
		MessageBox MB_OK "Due to HexEditor plugin's incompatibility issue in version $R0, HexEditor.dll will be deleted. Use Plugins Admin to add back (the latest version of) HexEditor." /SD IDOK
		Rename "$INSTDIR\plugins\HexEditor\HexEditor.dll" "$INSTDIR\plugins\disabled\HexEditor.dll"
		Delete "$INSTDIR\plugins\HexEditor\HexEditor.dll"
HexEditorTestEnd64:

	; ComparePlugin makes Notepad++ x64 crash. "2.0.2" is its 1st version which contains the fix
	IfFileExists "$INSTDIR\plugins\ComparePlugin\ComparePlugin.dll" 0 CompareTestEnd64
		${GetFileVersion} "$INSTDIR\plugins\ComparePlugin\ComparePlugin.dll" $R0
		${VersionCompare} $R0 "2.0.2" $R1 ;   0: equal to 2.0.2   1: $R0 is newer   2: 2.0.2 is newer
		StrCmp $R1 "0" +5 0 ; if equal skip all & go to end, else go to next
		StrCmp $R1 "1" +4 0 ; if newer skip all & go to end, else older (2) then go to next
		MessageBox MB_OK "Due to ComparePlugin plugin's incompatibility issue in version $R0, ComparePlugin.dll will be deleted. Use Plugins Admin to add back (the latest version of) ComparePlugin." /SD IDOK
		Rename "$INSTDIR\plugins\ComparePlugin\ComparePlugin.dll" "$INSTDIR\plugins\disabled\ComparePlugin.dll"
		Delete "$INSTDIR\plugins\ComparePlugin\ComparePlugin.dll"
CompareTestEnd64:

	; DSpellCheck makes Notepad++ x64 crash. "1.4.23" is its 1st version which contains the fix
	IfFileExists "$INSTDIR\plugins\DSpellCheck\DSpellCheck.dll" 0 DSpellCheckTestEnd64
		${GetFileVersion} "$INSTDIR\plugins\DSpellCheck\DSpellCheck.dll" $R0
		${VersionCompare} $R0 "1.4.23" $R1 ;   0: equal to 1.4.23   1: $R0 is newer   2: 1.4.23 is newer
		StrCmp $R1 "0" +5 0 ; if equal skip all & go to end, else go to next
		StrCmp $R1 "1" +4 0 ; if newer skip all & go to end, else older (2) then go to next
		MessageBox MB_OK "Due to DSpellCheck plugin's incompatibility issue in version $R0, DSpellCheck.dll will be deleted. Use Plugins Admin to add back (the latest version of) DSpellCheck." /SD IDOK
		Rename "$INSTDIR\plugins\DSpellCheck\DSpellCheck.dll" "$INSTDIR\plugins\disabled\DSpellCheck.dll"
		Delete "$INSTDIR\plugins\DSpellCheck\DSpellCheck.dll"
DSpellCheckTestEnd64:

	; SpeechPlugin makes Notepad++ x64 crash. "0.4.0.0" is its 1st version which contains the fix
	IfFileExists "$INSTDIR\plugins\SpeechPlugin\SpeechPlugin.dll" 0 SpeechPluginTestEnd64
		${GetFileVersion} "$INSTDIR\plugins\SpeechPlugin\SpeechPlugin.dll" $R0
		${VersionCompare} $R0 "0.4.0.0" $R1 ;   0: equal to 0.4.0.0   1: $R0 is newer   2: 0.4.0.0 is newer
		StrCmp $R1 "0" +5 0 ; if equal skip all & go to end, else go to next
		StrCmp $R1 "1" +4 0 ; if newer skip all & go to end, else older (2) then go to next
		MessageBox MB_OK "Due to SpeechPlugin plugin's incompatibility issue in version $R0, SpeechPlugin.dll will be deleted. Use Plugins Admin to add back (the latest version of) SpeechPlugin." /SD IDOK
		Rename "$INSTDIR\plugins\SpeechPlugin\SpeechPlugin.dll" "$INSTDIR\plugins\disabled\SpeechPlugin.dll"
		Delete "$INSTDIR\plugins\SpeechPlugin\SpeechPlugin.dll"
SpeechPluginTestEnd64:

	; XMLTools makes Notepad++ x64 crash. "3.1.1.12" is its 1st version which contains the fix
	IfFileExists "$INSTDIR\plugins\XMLTools\XMLTools.dll" 0 XMLToolsTestEnd64
		${GetFileVersion} "$INSTDIR\plugins\XMLTools\XMLTools.dll" $R0
		${VersionCompare} $R0 "3.1.1.12" $R1 ;   0: equal to 3.1.1.12   1: $R0 is newer   2: 3.1.1.12 is newer
		StrCmp $R1 "0" +5 0 ; if equal skip all & go to end, else go to next
		StrCmp $R1 "1" +4 0 ; if newer skip all & go to end, else older (2) then go to next
		MessageBox MB_OK "Due to XMLTools plugin's incompatibility issue in version $R0, XMLTools.dll will be deleted. Use Plugins Admin to add back (the latest version of) XMLTools." /SD IDOK
		Rename "$INSTDIR\plugins\XMLTools\XMLTools.dll" "$INSTDIR\plugins\disabled\XMLTools.dll"
		Delete "$INSTDIR\plugins\XMLTools\XMLTools.dll"
XMLToolsTestEnd64:

	; NppTaskList makes Notepad++ x64 crash. "2.4" is its 1st version which contains the fix
	IfFileExists "$INSTDIR\plugins\NppTaskList\NppTaskList.dll" 0 NppTaskListTestEnd64
		${GetFileVersion} "$INSTDIR\plugins\NppTaskList\NppTaskList.dll" $R0
		${VersionCompare} $R0 "2.4" $R1 ;   0: equal to 2.4   1: $R0 is newer   2: 2.4 is newer
		StrCmp $R1 "0" +5 0 ; if equal skip all & go to end, else go to next
		StrCmp $R1 "1" +4 0 ; if newer skip all & go to end, else older (2) then go to next
		MessageBox MB_OK "Due to NppTaskList plugin's incompatibility issue in version $R0, NppTaskList.dll will be deleted. Use Plugins Admin to add back (the latest version of) NppTaskList." /SD IDOK
		Rename "$INSTDIR\plugins\NppTaskList\NppTaskList.dll" "$INSTDIR\plugins\disabled\NppTaskList.dll"
		Delete "$INSTDIR\plugins\NppTaskList\NppTaskList.dll"
NppTaskListTestEnd64:

	; jN makes Notepad++ x64 crash. "2.2.185.7" is its 1st version which contains the fix
	IfFileExists "$INSTDIR\plugins\jN\jN.dll" 0 jN64
		${GetFileVersion} "$INSTDIR\plugins\jN\jN.dll" $R0
		${VersionCompare} $R0 "2.2.185.7" $R1 ;   0: equal to 2.2.185.7   1: $R0 is newer   2: 2.4 is newer
		StrCmp $R1 "0" +5 0 ; if equal skip all & go to end, else go to next
		StrCmp $R1 "1" +4 0 ; if newer skip all & go to end, else older (2) then go to next
		MessageBox MB_OK "Due to jN plugin's incompatibility issue in version $R0, jN.dll will be deleted. Use Plugins Admin to add back (the latest version of) jN." /SD IDOK
		Rename "$INSTDIR\plugins\jN\jN.dll" "$INSTDIR\plugins\disabled\jN.dll"
		Delete "$INSTDIR\plugins\jN\jN.dll"
jN64:


	IfFileExists "$INSTDIR\plugins\NppQCP\NppQCP.dll" 0 NppQCPTestEnd64
		MessageBox MB_OK "Due to NppQCP plugin's crash issue on Notepad++ x64 binary, NppQCP.dll will be removed." /SD IDOK
		Rename "$INSTDIR\plugins\NppQCP\NppQCP.dll" "$INSTDIR\plugins\disabled\NppQCP.dll"
		Delete "$INSTDIR\plugins\NppQCP\NppQCP.dll"
NppQCPTestEnd64:


!else ; 32-bit installer

		; https://github.com/chcg/NPP_HexEdit/issues/51
		IfFileExists "$INSTDIR\plugins\HexEditor\HexEditor.dll" 0 noDeleteHEPlugin32
			MessageBox MB_YESNO "HexEditor plugin is unstable, we suggest you to remove it.$\nRemove HexEditor plugin?" /SD IDYES IDYES doDeleteHEPlugin32 IDNO noDeleteHEPlugin32 ;IDYES remove
doDeleteHEPlugin32:
                Rename "$INSTDIR\plugins\HexEditor\HexEditor.dll" "$INSTDIR\plugins\disabled\HexEditor.dll"
                Delete "$INSTDIR\plugins\HexEditor\HexEditor.dll"
noDeleteHEPlugin32:

!endif

FunctionEnd

Function removeOldContextMenu
   ; Context Menu Management : removing old version of Context Menu module
	IfFileExists "$INSTDIR\nppcm.dll" 0 +3
		ExecWait 'regsvr32 /u /s "$INSTDIR\nppcm.dll"'
		Delete "$INSTDIR\nppcm.dll"
        
    IfFileExists "$INSTDIR\NppShell.dll" 0 +3
		ExecWait 'regsvr32 /u /s "$INSTDIR\NppShell.dll"'
		Delete "$INSTDIR\NppShell.dll"
		
    IfFileExists "$INSTDIR\NppShell_01.dll" 0 +3
		ExecWait 'regsvr32 /u /s "$INSTDIR\NppShell_01.dll"'
		Delete "$INSTDIR\NppShell_01.dll"
        
    IfFileExists "$INSTDIR\NppShell_02.dll" 0 +3
		ExecWait 'regsvr32 /u /s "$INSTDIR\NppShell_02.dll"'
		Delete "$INSTDIR\NppShell_02.dll"
		
    IfFileExists "$INSTDIR\NppShell_03.dll" 0 +3
		ExecWait 'regsvr32 /u /s "$INSTDIR\NppShell_03.dll"'
		Delete "$INSTDIR\NppShell_03.dll"
		
	IfFileExists "$INSTDIR\NppShell_04.dll" 0 +3
		ExecWait 'regsvr32 /u /s "$INSTDIR\NppShell_04.dll"'
		Delete "$INSTDIR\NppShell_04.dll"
		
	IfFileExists "$INSTDIR\NppShell_05.dll" 0 +3
		ExecWait 'regsvr32 /u /s "$INSTDIR\NppShell_05.dll"'
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


