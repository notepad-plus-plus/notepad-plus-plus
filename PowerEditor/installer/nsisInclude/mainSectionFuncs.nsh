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
	File "..\bin64\SciLexer.dll"
	File "..\bin64\notepad++.exe"
!else
	File "..\bin\SciLexer.dll"
	File "..\bin\notepad++.exe"
!endif

	; Markdown in user defined languages
	SetOutPath "$UPDATE_PATH\userDefineLangs\"
	Delete "$UPDATE_PATH\userDefineLangs\userDefinedLang-markdown.default.modern.xml"
	File "..\bin\userDefineLangs\markdown._preinstalled.udl.xml"

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

	
Function removeUnstablePlugins

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


