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
    ${If} $UPDATE_PATH == ""
        Goto initUpdatePath
	${ELSE}
        Goto alreadyDone
	${EndIf}
    
initUpdatePath:
    
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
	
	${If} $allowAppDataPluginsLoading == "true"
		File "..\bin\allowAppDataPlugins.xml"
	${ELSE}
		IfFileExists $INSTDIR\allowAppDataPlugins.xml 0 +2
		Delete $INSTDIR\allowAppDataPlugins.xml
	${EndIf}
    
alreadyDone:
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
	
	IfFileExists "$INSTDIR\plugins\HexEditorPlugin.dll" 0 +4
		MessageBox MB_OK "Due to the stability issue,$\nHexEditorPlugin.dll is about to be deleted." /SD IDOK
		Rename "$INSTDIR\plugins\HexEditorPlugin.dll" "$INSTDIR\plugins\disabled\HexEditorPlugin.dll"
		Delete "$INSTDIR\plugins\HexEditorPlugin.dll"

	IfFileExists "$INSTDIR\plugins\HexEditor.dll" 0 +4
		MessageBox MB_OK "Due to the stability issue,$\nHexEditor.dll will be moved to the directory $\"disabled$\"" /SD IDOK 
		Rename "$INSTDIR\plugins\HexEditor.dll" "$INSTDIR\plugins\disabled\HexEditor.dll" 
		Delete "$INSTDIR\plugins\HexEditor.dll"

	IfFileExists "$INSTDIR\plugins\MultiClipboard.dll" 0 +4
		MessageBox MB_OK "Due to the stability issue,$\nMultiClipboard.dll will be moved to the directory $\"disabled$\"" /SD IDOK
		Rename "$INSTDIR\plugins\MultiClipboard.dll" "$INSTDIR\plugins\disabled\MultiClipboard.dll"
		Delete "$INSTDIR\plugins\MultiClipboard.dll"
		
	Delete "$INSTDIR\plugins\NppDocShare.dll"

	IfFileExists "$INSTDIR\plugins\FunctionList.dll" 0 +4
		MessageBox MB_OK "Due to the stability issue,$\nFunctionList.dll will be moved to the directory $\"disabled$\"" /SD IDOK
		Rename "$INSTDIR\plugins\FunctionList.dll" "$INSTDIR\plugins\disabled\FunctionList.dll"
		Delete "$INSTDIR\plugins\FunctionList.dll"
	
	IfFileExists "$INSTDIR\plugins\docMonitor.unicode.dll" 0 +4
		MessageBox MB_OK "Due to the stability issue,$\ndocMonitor.unicode.dll will be moved to the directory $\"disabled$\"" /SD IDOK
		Rename "$INSTDIR\plugins\docMonitor.unicode.dll" "$INSTDIR\plugins\disabled\docMonitor.unicode.dll"
		Delete "$INSTDIR\plugins\docMonitor.unicode.dll"
		
	IfFileExists "$INSTDIR\plugins\NPPTextFX.ini" 0 +1
		Delete "$INSTDIR\plugins\NPPTextFX.ini"
		 
	IfFileExists "$INSTDIR\plugins\NppAutoIndent.dll" 0 +4
		MessageBox MB_OK "Due to the stability issue,$\nNppAutoIndent.dll will be moved to the directory $\"disabled$\"" /SD IDOK
		Rename "$INSTDIR\plugins\NppAutoIndent.dll" "$INSTDIR\plugins\disabled\NppAutoIndent.dll"
		Delete "$INSTDIR\plugins\NppAutoIndent.dll"

	IfFileExists "$INSTDIR\plugins\FTP_synchronize.dll" 0 +4
		MessageBox MB_OK "Due to the stability issue,$\nFTP_synchronize.dll will be moved to the directory $\"disabled$\"" /SD IDOK
		Rename "$INSTDIR\plugins\FTP_synchronize.dll" "$INSTDIR\plugins\disabled\FTP_synchronize.dll"
		Delete "$INSTDIR\plugins\FTP_synchronize.dll"

	IfFileExists "$INSTDIR\plugins\NppPlugin_ChangeMarker.dll" 0 +4
		MessageBox MB_OK "Due to the stability issue,$\nNppPlugin_ChangeMarker.dll will be moved to the directory $\"disabled$\"" /SD IDOK
		Rename "$INSTDIR\plugins\NppPlugin_ChangeMarker.dll" "$INSTDIR\plugins\disabled\NppPlugin_ChangeMarker.dll"
		Delete "$INSTDIR\plugins\NppPlugin_ChangeMarker.dll"
		
	IfFileExists "$INSTDIR\plugins\QuickText.UNI.dll" 0 +4
		MessageBox MB_OK "Due to the stability issue,$\nQuickText.UNI.dll will be moved to the directory $\"disabled$\"" /SD IDOK
		Rename "$INSTDIR\plugins\QuickText.UNI.dll" "$INSTDIR\plugins\disabled\QuickText.UNI.dll"
		Delete "$INSTDIR\plugins\QuickText.UNI.dll"

	IfFileExists "$INSTDIR\plugins\AHKExternalLexer.dll" 0 +4
		MessageBox MB_OK "Due to the compatibility issue,$\nAHKExternalLexer.dll will be moved to the directory $\"disabled$\"" /SD IDOK
		Rename "$INSTDIR\plugins\AHKExternalLexer.dll" "$INSTDIR\plugins\disabled\AHKExternalLexer.dll"
		Delete "$INSTDIR\plugins\AHKExternalLexer.dll"

	IfFileExists "$INSTDIR\plugins\NppExternalLexers.dll" 0 +4
		MessageBox MB_OK "Due to the compatibility issue,$\n\NppExternalLexers.dll will be moved to the directory $\"disabled$\"" /SD IDOK
		Rename "$INSTDIR\plugins\NppExternalLexers.dll" "$INSTDIR\plugins\disabled\NppExternalLexers.dll"
		Delete "$INSTDIR\plugins\NppExternalLexers.dll"

	IfFileExists "$INSTDIR\plugins\ExternalLexerKVS.dll" 0 +4
		MessageBox MB_OK "Due to the compatibility issue,$\n\ExternalLexerKVS.dll will be moved to the directory $\"disabled$\"" /SD IDOK
		Rename "$INSTDIR\plugins\ExternalLexerKVS.dll" "$INSTDIR\plugins\disabled\ExternalLexerKVS.dll"
		Delete "$INSTDIR\plugins\ExternalLexerKVS.dll"

	IfFileExists "$INSTDIR\plugins\Oberon2LexerU.dll" 0 +4
		MessageBox MB_OK "Due to the compatibility issue,$\n\Oberon2LexerU.dll will be moved to the directory $\"disabled$\"" /SD IDOK
		Rename "$INSTDIR\plugins\Oberon2LexerU.dll" "$INSTDIR\plugins\disabled\Oberon2LexerU.dll"
		Delete "$INSTDIR\plugins\Oberon2LexerU.dll"


	IfFileExists "$INSTDIR\plugins\NotepadSharp.dll" 0 +4
		MessageBox MB_OK "Due to the stability issue,$\n\NotepadSharp.dll will be moved to the directory $\"disabled$\"" /SD IDOK
		Rename "$INSTDIR\plugins\NotepadSharp.dll" "$INSTDIR\plugins\disabled\NotepadSharp.dll"
		Delete "$INSTDIR\plugins\NotepadSharp.dll"
		
	IfFileExists "$INSTDIR\plugins\PreviewHTML.dll" 0 +4
		MessageBox MB_OK "Due to the stability issue,$\nPreviewHTML.dll will be moved to the directory $\"disabled$\"" /SD IDOK
		Rename "$INSTDIR\plugins\PreviewHTML.dll" "$INSTDIR\plugins\disabled\PreviewHTML.dll"
		Delete "$INSTDIR\plugins\PreviewHTML.dll"
		
	IfFileExists "$INSTDIR\plugins\nppRegEx.dll" 0 +4
		MessageBox MB_OK "Due to the stability issue,$\nnppRegEx.dll will be moved to the directory $\"disabled$\"" /SD IDOK
		Rename "$INSTDIR\plugins\nppRegEx.dll" "$INSTDIR\plugins\disabled\nppRegEx.dll"
		Delete "$INSTDIR\plugins\nppRegEx.dll"
		
	IfFileExists "$INSTDIR\plugins\AutoSaveU.dll" 0 +4
		MessageBox MB_OK "Due to the stability issue,$\nAutoSaveU.dll will be moved to the directory $\"disabled$\"" /SD IDOK
		Rename "$INSTDIR\plugins\AutoSaveU.dll" "$INSTDIR\plugins\disabled\AutoSaveU.dll"
		Delete "$INSTDIR\plugins\AutoSaveU.dll"
		
	IfFileExists "$INSTDIR\plugins\NppQCP.dll" 0 +4
		MessageBox MB_OK "Due to the stability issue,$\nNppQCP.dll will be moved to the directory $\"disabled$\"" /SD IDOK
		Rename "$INSTDIR\plugins\NppQCP.dll" "$INSTDIR\plugins\disabled\NppQCP.dll"
		Delete "$INSTDIR\plugins\NppQCP.dll"
	/*
	IfFileExists "$INSTDIR\plugins\DSpellCheck.dll" 0 +11
		MessageBox MB_YESNOCANCEL "Due to the stability issue, DSpellCheck.dll will be moved to the directory $\"disabled$\".$\nChoose Cancel to keep it for this installation.$\nChoose No to keep it forever." /SD IDYES IDNO never IDCANCEL donothing ;IDYES remove
		Rename "$INSTDIR\plugins\DSpellCheck.dll" "$INSTDIR\plugins\disabled\DSpellCheck.dll"
		Delete "$INSTDIR\plugins\DSpellCheck.dll"
		Goto donothing
	never:
		Rename "$INSTDIR\plugins\DSpellCheck.dll" "$INSTDIR\plugins\DSpellCheck2.dll"
		Goto donothing
	donothing:
	*/
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


