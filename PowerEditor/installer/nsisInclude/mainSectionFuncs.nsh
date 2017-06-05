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

Var UPDATE_PATH
Function setPathAndOptions
	; Set Section properties
	SetOverwrite on
	StrCpy $UPDATE_PATH $INSTDIR
		
	SetOutPath "$INSTDIR\"

	${If} $noUserDataChecked == ${BST_CHECKED}
		File "..\bin\doLocalConf.xml"
	${ELSE}
		IfFileExists $INSTDIR\doLocalConf.xml 0 +2
		Delete $INSTDIR\doLocalConf.xml
		StrCpy $UPDATE_PATH "$APPDATA\${APPNAME}"
		CreateDirectory $UPDATE_PATH\plugins\config
	${EndIf}
	
	${If} $allowPluginLoadFromUserDataChecked == ${BST_CHECKED}
		File "..\bin\allowAppDataPlugins.xml"
	${ELSE}
		IfFileExists $INSTDIR\allowAppDataPlugins.xml 0 +2
		Delete $INSTDIR\allowAppDataPlugins.xml
	${EndIf}
FunctionEnd
	
Function copyCommonFiles
	SetOverwrite off
	SetOutPath "$UPDATE_PATH\"
	File "..\bin\contextMenu.xml"
	File "..\bin\functionList.xml"
	
	SetOverwrite on
	SetOutPath "$INSTDIR\"
	File "..\bin\langs.model.xml"
	File "..\bin\stylers.model.xml"
	File "..\bin\contextMenu.xml"
	File "..\bin\functionList.xml"

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

	StrCmp $LANGUAGE ${LANG_ENGLISH} +3 0
	CopyFiles "$PLUGINSDIR\nppLocalization\$(langFileName)" "$UPDATE_PATH\nativeLang.xml"
	CopyFiles "$PLUGINSDIR\nppLocalization\$(langFileName)" "$INSTDIR\localization\$(langFileName)"
FunctionEnd

	
Function removeUnstablePlugins
	; remove unstable plugins
	CreateDirectory "$INSTDIR\plugins\disabled"
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
	CreateDirectory "$SMPROGRAMS\${APPNAME}"
	CreateShortCut "$SMPROGRAMS\${APPNAME}\Notepad++.lnk" "$INSTDIR\notepad++.exe"
	${If} $createShortcutChecked == ${BST_CHECKED}
		CreateShortCut "$DESKTOP\Notepad++.lnk" "$INSTDIR\notepad++.exe"
	${EndIf}
	
	SetShellVarContext current
FunctionEnd


