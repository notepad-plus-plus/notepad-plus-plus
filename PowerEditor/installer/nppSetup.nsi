;this file is part of installer for Notepad++
;Copyright (C)2006 Don HO <don.h@free.fr>
;
;This program is free software; you can redistribute it and/or
;modify it under the terms of the GNU General Public License
;as published by the Free Software Foundation; either
;version 2 of the License, or (at your option) any later version.
;
;This program is distributed in the hope that it will be useful,
;but WITHOUT ANY WARRANTY; without even the implied warranty of
;MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;GNU General Public License for more details.
;
;You should have received a copy of the GNU General Public License
;along with this program; if not, write to the Free Software
;Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

; Define the application name
!define APPNAME "Notepad++"
!define APPNAMEANDVERSION "Notepad++ v4.9"

!define VERSION_MAJOR 4
!define VERSION_MINOR 9

; Main Install settings
Name "${APPNAMEANDVERSION}"
InstallDir "$PROGRAMFILES\Notepad++"
InstallDirRegKey HKLM "Software\${APPNAME}" ""
OutFile "..\bin\npp.4.9.Installer.exe"

; GetWindowsVersion
 ;
 ; Based on Yazno's function, http://yazno.tripod.com/powerpimpit/
 ; Updated by Joost Verburg
 ;
 ; Returns on top of stack
 ;
 ; Windows Version (95, 98, ME, NT x.x, 2000, XP, 2003, Vista)
 ; or
 ; '' (Unknown Windows Version)
 ;
 ; Usage:
 ;   Call GetWindowsVersion
 ;   Pop $R0
 ;   ; at this point $R0 is "NT 4.0" or whatnot
 
 Function GetWindowsVersion
 
   Push $R0
   Push $R1
 
   ClearErrors
 
   ReadRegStr $R0 HKLM \
   "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion

   IfErrors 0 lbl_winnt
   
   ; we are not NT
   ReadRegStr $R0 HKLM \
   "SOFTWARE\Microsoft\Windows\CurrentVersion" VersionNumber
 
   StrCpy $R1 $R0 1
   StrCmp $R1 '4' 0 lbl_error
 
   StrCpy $R1 $R0 3
 
   StrCmp $R1 '4.0' lbl_win32_95
   StrCmp $R1 '4.9' lbl_win32_ME lbl_win32_98
 
   lbl_win32_95:
     StrCpy $R0 '95'
   Goto lbl_done
 
   lbl_win32_98:
     StrCpy $R0 '98'
   Goto lbl_done
 
   lbl_win32_ME:
     StrCpy $R0 'ME'
   Goto lbl_done
 
   lbl_winnt:
 
   StrCpy $R1 $R0 1
 
   StrCmp $R1 '3' lbl_winnt_x
   StrCmp $R1 '4' lbl_winnt_x
 
   StrCpy $R1 $R0 3
 
   StrCmp $R1 '5.0' lbl_winnt_2000
   StrCmp $R1 '5.1' lbl_winnt_XP
   StrCmp $R1 '5.2' lbl_winnt_2003
   StrCmp $R1 '6.0' lbl_winnt_vista lbl_error
 
   lbl_winnt_x:
     StrCpy $R0 "NT $R0" 6
   Goto lbl_done
 
   lbl_winnt_2000:
     Strcpy $R0 '2000'
   Goto lbl_done
 
   lbl_winnt_XP:
     Strcpy $R0 'XP'
   Goto lbl_done
 
   lbl_winnt_2003:
     Strcpy $R0 '2003'
   Goto lbl_done
 
   lbl_winnt_vista:
     Strcpy $R0 'Vista'
   Goto lbl_done
 
   lbl_error:
     Strcpy $R0 ''
   lbl_done:
 
   Pop $R1
   Exch $R0
 
 FunctionEnd


; Modern interface settings
!include "MUI.nsh"
!include "x64.nsh"

!define MUI_ICON ".\images\npp_inst.ico"

!define MUI_WELCOMEFINISHPAGE_BITMAP ".\images\wizard.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP_NOSTRETCH

!define MUI_HEADERIMAGE
;!define MUI_HEADERIMAGE_RIGHT
;!define MUI_HEADERIMAGE_BITMAP ".\images\headerRight.bmp" ; optional
!define MUI_HEADERIMAGE_BITMAP ".\images\headerLeft.bmp" ; optional
!define MUI_ABORTWARNING

!define MUI_FINISHPAGE_RUN "$INSTDIR\notepad++.exe"


!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\license.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Set languages (first is default language)
;!insertmacro MUI_LANGUAGE "English"

;Languages

  !insertmacro MUI_LANGUAGE "English"
  !insertmacro MUI_LANGUAGE "French"
  !insertmacro MUI_LANGUAGE "TradChinese"
  !insertmacro MUI_LANGUAGE "Spanish"
  !insertmacro MUI_LANGUAGE "Hungarian"
  !insertmacro MUI_LANGUAGE "Russian"
  !insertmacro MUI_LANGUAGE "German"
  !insertmacro MUI_LANGUAGE "Dutch"
  !insertmacro MUI_LANGUAGE "SimpChinese"
  !insertmacro MUI_LANGUAGE "Italian"
  !insertmacro MUI_LANGUAGE "Danish"
  !insertmacro MUI_LANGUAGE "Polish"
  !insertmacro MUI_LANGUAGE "Czech"
  !insertmacro MUI_LANGUAGE "Slovenian"
  !insertmacro MUI_LANGUAGE "Slovak"
  !insertmacro MUI_LANGUAGE "Swedish"
  !insertmacro MUI_LANGUAGE "Norwegian"
  !insertmacro MUI_LANGUAGE "PortugueseBR"
  !insertmacro MUI_LANGUAGE "Ukrainian"
  !insertmacro MUI_LANGUAGE "Turkish"
  !insertmacro MUI_LANGUAGE "Catalan"
  !insertmacro MUI_LANGUAGE "Arabic"
  !insertmacro MUI_LANGUAGE "Lithuanian"
	!insertmacro MUI_LANGUAGE "Finnish"
	!insertmacro MUI_LANGUAGE "Greek"
	!insertmacro MUI_LANGUAGE "Romanian"
	!insertmacro MUI_LANGUAGE "Korean"
	!insertmacro MUI_LANGUAGE "Hebrew"
	!insertmacro MUI_LANGUAGE "Portuguese"
  !insertmacro MUI_LANGUAGE "Farsi"
  !insertmacro MUI_LANGUAGE "Bulgarian"
	!insertmacro MUI_LANGUAGE "Indonesian"
  !insertmacro MUI_LANGUAGE "Japanese"
  !insertmacro MUI_LANGUAGE "Croatian"
  !insertmacro MUI_LANGUAGE "Serbian"
	
  ;!insertmacro MUI_LANGUAGE "Thai"
  ;!insertmacro MUI_LANGUAGE "Latvian"
  ;!insertmacro MUI_LANGUAGE "Macedonian"
  ;!insertmacro MUI_LANGUAGE "Estonian"
  ;

!insertmacro MUI_RESERVEFILE_LANGDLL

;Installer Functions


Function .onInit

  !insertmacro MUI_LANGDLL_DISPLAY
	# the plugins dir is automatically deleted when the installer exits
	;InitPluginsDir
	;File /oname=$PLUGINSDIR\splash.bmp ".\images\splash.bmp"
	#optional
	#File /oname=$PLUGINSDIR\splash.wav "C:\myprog\sound.wav"

	;splash::show 1000 $PLUGINSDIR\splash

	;Pop $0 ; $0 has '1' if the user closed the splash screen early,
			; '0' if everything closed normally, and '-1' if some error occurred.

FunctionEnd

LangString langFileName ${LANG_ENGLISH} "english.xml"
LangString langFileName ${LANG_FRENCH} "french.xml"
LangString langFileName ${LANG_TRADCHINESE} "chinese.xml"
LangString langFileName ${LANG_GERMAN} "german.xml"
LangString langFileName ${LANG_SPANISH} "spanish.xml"
LangString langFileName ${LANG_HUNGARIAN} "hungarian.xml"
LangString langFileName ${LANG_RUSSIAN} "russian.xml"
LangString langFileName ${LANG_DUTCH} "dutch.xml"
LangString langFileName ${LANG_SIMPCHINESE} "chineseSimplified.xml"
LangString langFileName ${LANG_ITALIAN} "italian.xml"
LangString langFileName ${LANG_DANISH} "danish.xml"
LangString langFileName ${LANG_POLISH} "polish.xml"
LangString langFileName ${LANG_CZECH} "czech.xml"
LangString langFileName ${LANG_SLOVENIAN} "slovenian.xml"
LangString langFileName ${LANG_SLOVAK} "slovak.xml"
LangString langFileName ${LANG_SWEDISH} "swedish.xml"
LangString langFileName ${LANG_NORWEGIAN} "norwegian.xml"
LangString langFileName ${LANG_PORTUGUESEBR} "brazilian_portuguese.xml"
LangString langFileName ${LANG_UKRAINIAN} "ukrainian.xml"
LangString langFileName ${LANG_TURKISH} "turkish.xml"
LangString langFileName ${LANG_CATALAN} "catalan.xml"
LangString langFileName ${LANG_ARABIC} "arabic.xml"
LangString langFileName ${LANG_LITHUANIAN} "lithuanian.xml"
LangString langFileName ${LANG_FINNISH} "finnish.xml"
LangString langFileName ${LANG_GREEK} "greek.xml"
LangString langFileName ${LANG_ROMANIAN} "romanian.xml"
LangString langFileName ${LANG_KOREAN} "korean.xml"
LangString langFileName ${LANG_HEBREW} "hebrew.xml"
LangString langFileName ${LANG_PORTUGUESE} "portuguese.xml"
LangString langFileName ${LANG_FARSI} "farsi.xml"
LangString langFileName ${LANG_BULGARIAN} "bulgarian.xml"
LangString langFileName ${LANG_INDONESIAN} "indonesian.xml"
LangString langFileName ${LANG_JAPANESE} "japanese.xml"
LangString langFileName ${LANG_CROATIAN} "croatian.xml"
LangString langFileName ${LANG_SERBIAN} "serbian.xml"

;--------------------------------
;Variables
  Var IS_LOCAL
;--------------------------------

Section /o "Don't use %APPDATA%" makeLocal
	StrCpy $IS_LOCAL "1"
SectionEnd

Var UPDATE_PATH

Section -"Notepad++" mainSection

	; Set Section properties
	SetOverwrite on

	StrCpy $UPDATE_PATH $INSTDIR
	
	;SetOutPath "$TEMP\"
	File /oname=$TEMP\xmlUpdater.exe ".\bin\xmlUpdater.exe"
		
	SetOutPath "$INSTDIR\"
	
	;Test if window9x
	Call GetWindowsVersion
	Pop $R0
	
	StrCmp $R0 "95" 0 +2
		StrCpy $IS_LOCAL "1"
		
	StrCmp $R0 "98" 0 +2
		StrCpy $IS_LOCAL "1"
		
	StrCmp $R0 "ME" 0 +2
		StrCpy $IS_LOCAL "1"
	
	
	; if isLocal -> copy file "doLocalConf.xml"
	StrCmp $IS_LOCAL "1" 0 IS_NOT_LOCAL
		File "..\bin\doLocalConf.xml"
		goto GLOBAL_INST
	
IS_NOT_LOCAL:
	IfFileExists $INSTDIR\doLocalConf.xml 0 +2
		Delete $INSTDIR\doLocalConf.xml
	
	StrCpy $UPDATE_PATH "$APPDATA\Notepad++"
	CreateDirectory $UPDATE_PATH\plugins\config
	
GLOBAL_INST:
	SetOutPath "$TEMP\"
	File "langsModel.xml"
	File "configModel.xml"
	File "stylesLexerModel.xml"
	File "stylesGlobalModel.xml"

	File "..\bin\langs.model.xml"
	File "..\bin\config.model.xml"
	File "..\bin\stylers.model.xml"

	;UPGRATE $INSTDIR\langs.xml
	nsExec::ExecToStack '"$TEMP\xmlUpdater.exe" "$TEMP\langsModel.xml" "$TEMP\langs.model.xml" "$INSTDIR\langs.xml"'
	nsExec::ExecToStack '"$TEMP\xmlUpdater.exe" "$TEMP\configModel.xml" "$TEMP\config.model.xml" "$UPDATE_PATH\config.xml"'
	nsExec::ExecToStack '"$TEMP\xmlUpdater.exe" "$TEMP\stylesLexerModel.xml" "$TEMP\stylers.model.xml" "$UPDATE_PATH\stylers.xml"'
	nsExec::ExecToStack '"$TEMP\xmlUpdater.exe" "$TEMP\stylesGlobalModel.xml" "$TEMP\stylers.model.xml" "$UPDATE_PATH\stylers.xml"'

	SetOutPath "$INSTDIR\"
	File "..\bin\langs.model.xml"
	File "..\bin\config.model.xml"
	File "..\bin\stylers.model.xml"

	SetOverwrite off
	File /oname=$INSTDIR\langs.xml "..\bin\langs.model.xml"
	File "..\bin\contextMenu.xml"
	File "..\bin\shortcuts.xml"
	
	; Set Section Files and Shortcuts
	
	SetOverwrite on
	File "..\license.txt"
	File "..\bin\LINEDRAW.TTF"
	File "..\bin\SciLexer.dll"
	File "..\bin\change.log"
	File "..\bin\notepad++.exe"
	File "..\bin\readme.txt"
	
	StrCmp $LANGUAGE ${LANG_ENGLISH} noLang 0
	StrCmp $LANGUAGE ${LANG_FRENCH} 0 +3
		File ".\nativeLang\french.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_TRADCHINESE} 0 +3
		File ".\nativeLang\chinese.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_SPANISH} 0 +3
		File ".\nativeLang\spanish.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_HUNGARIAN} 0 +3
		File ".\nativeLang\hungarian.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_RUSSIAN} 0 +3
		File ".\nativeLang\russian.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_GERMAN} 0 +3
		File ".\nativeLang\german.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_DUTCH} 0 +3
		File ".\nativeLang\dutch.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_SIMPCHINESE} 0 +3
		File ".\nativeLang\chineseSimplified.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_ITALIAN} 0 +3
		File ".\nativeLang\italian.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_DANISH} 0 +3
		File ".\nativeLang\danish.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_POLISH} 0 +3
		File ".\nativeLang\polish.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_CZECH} 0 +3
		File ".\nativeLang\czech.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_SLOVENIAN} 0 +3
		File ".\nativeLang\slovenian.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_SLOVAK} 0 +3
		File ".\nativeLang\slovak.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_SWEDISH} 0 +3
		File ".\nativeLang\swedish.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_NORWEGIAN} 0 +3
		File ".\nativeLang\norwegian.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_PORTUGUESEBR} 0 +3
		File ".\nativeLang\brazilian_portuguese.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_UKRAINIAN} 0 +3
		File ".\nativeLang\ukrainian.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_TURKISH} 0 +3
		File ".\nativeLang\turkish.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_CATALAN} 0 +3
		File ".\nativeLang\catalan.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_ARABIC} 0 +3
		File ".\nativeLang\arabic.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_LITHUANIAN} 0 +3
		File ".\nativeLang\lithuanian.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_FINNISH} 0 +3
		File ".\nativeLang\finnish.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_GREEK} 0 +3
		File ".\nativeLang\greek.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_ROMANIAN} 0 +3
		File ".\nativeLang\romanian.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_KOREAN} 0 +3
		File ".\nativeLang\korean.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_HEBREW} 0 +3
		File ".\nativeLang\hebrew.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_PORTUGUESE} 0 +3
		File ".\nativeLang\portuguese.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_FARSI} 0 +3
		File ".\nativeLang\farsi.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_BULGARIAN} 0 +3
		File ".\nativeLang\bulgarian.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_INDONESIAN} 0 +3
		File ".\nativeLang\indonesian.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_JAPANESE} 0 +3
		File ".\nativeLang\japanese.xml"
		Goto finLang
	StrCmp $LANGUAGE ${LANG_CROATIAN} 0 +3
		File ".\nativeLang\croatian.xml"
		Goto finLang
	finLang:
	
	IfFileExists "$UPDATE_PATH\nativeLang.xml" 0 +2
		Delete "$UPDATE_PATH\nativeLang.xml"
		
	IfFileExists "$INSTDIR\nativeLang.xml" 0 +2
		Delete "$INSTDIR\nativeLang.xml"

	Rename "$INSTDIR\$(langFileName)" "$INSTDIR\nativeLang.xml"
	Goto commun

	noLang:
	IfFileExists "$UPDATE_PATH\nativeLang.xml" 0 +2
		Delete "$UPDATE_PATH\nativeLang.xml"
		
	commun:
	
	; remove all the npp shortcuts from current user
	Delete "$DESKTOP\Notepad++.lnk"
	Delete "$SMPROGRAMS\Notepad++\Notepad++.lnk"
	Delete "$SMPROGRAMS\Notepad++\readme.lnk"
	Delete "$SMPROGRAMS\Notepad++\Uninstall.lnk"
	CreateDirectory "$SMPROGRAMS\Notepad++"
	CreateShortCut "$SMPROGRAMS\Notepad++\Uninstall.lnk" "$INSTDIR\uninstall.exe"
	
	; remove unstable plugins
	IfFileExists "$INSTDIR\plugins\HexEditorPlugin.dll" 0 +3
		MessageBox MB_OK "Due to the problem of compability with this version,$\nHexEditorPlugin.dll is about to be deleted."
		Delete "$INSTDIR\plugins\HexEditorPlugin.dll"
	
	IfFileExists "$INSTDIR\plugins\HexEditor.dll" 0 +3
		MessageBox MB_OK "Due to the problem of compability with this version,$\nHexEditor.dll is about to be deleted.$\nYou can download it via menu $\"?->Get more plugins$\" if you really need it."
		Delete "$INSTDIR\plugins\HexEditor.dll"
	
	IfFileExists "$INSTDIR\plugins\MultiClipboard.dll" 0 +3	
		MessageBox MB_OK "Due to the problem of compability with this version,$\nMultiClipboard.dll is about to be deleted.$\nYou can download it via menu $\"?->Get more plugins$\" if you really need it."
		Delete "$INSTDIR\plugins\MultiClipboard.dll"
	
	; detect the right of 
	UserInfo::GetAccountType
	Pop $1
	StrCmp $1 "Admin" 0 +2
	
	SetShellVarContext all
	; add all the npp shortcuts for all user or current user
	CreateDirectory "$SMPROGRAMS\Notepad++"
	CreateShortCut "$DESKTOP\Notepad++.lnk" "$INSTDIR\notepad++.exe"
	CreateShortCut "$SMPROGRAMS\Notepad++\Notepad++.lnk" "$INSTDIR\notepad++.exe"
	CreateShortCut "$SMPROGRAMS\Notepad++\readme.lnk" "$INSTDIR\readme.txt"
	SetShellVarContext current
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\notepad++.exe" "" "$INSTDIR\notepad++.exe"
SectionEnd

Section "Context Menu Entry" explorerContextMenu
	SetOverwrite try
	SetOutPath "$INSTDIR\"
	${If} ${RunningX64}
		File /oname=$INSTDIR\nppcm.dll "..\bin\nppcm64.dll"
	${Else}
		File "..\bin\nppcm.dll"
	${EndIf}
	
	Exec 'regsvr32 /s "$INSTDIR\nppcm.dll"'
	Exec 'regsvr32 /u /s "$INSTDIR\nppshellext.dll"'
	Delete "$INSTDIR\nppshellext.dll"
SectionEnd

SubSection "Auto-completion Files" autoCompletionComponent
	SetOverwrite off
	
	Section C
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\c.api"
	SectionEnd
	
	Section C++
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\cpp.api"
	SectionEnd

	Section Java
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\java.api"
	SectionEnd
	
	Section C#
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\cs.api"
	SectionEnd
	
	Section PHP
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\php.api"
	SectionEnd

	Section CSS
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\css.api"
	SectionEnd

	Section VB
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\vb.api"
	SectionEnd

	Section Perl
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\perl.api"
	SectionEnd
	
	Section JavaScript
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\javascript.api"
	SectionEnd

	Section Python
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\python.api"
	SectionEnd
	
	Section ActionScript
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\flash.api"
	SectionEnd
	
	Section LISP
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\lisp.api"
	SectionEnd
	
	Section VHDL
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\vhdl.api"
	SectionEnd
	
	Section TeX
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\tex.api"
	SectionEnd
	
	Section DocBook
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\xml.api"
	SectionEnd
	
	Section NSIS
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\nsis.api"
	SectionEnd
		
SubSectionEnd

SubSection "Plugins" Plugins
	
	SetOverwrite on
	
	Section "NPPTextFX" NPPTextFX
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\NPPTextFX.dll"
		StrCmp $IS_LOCAL "1" 0 NOT_LOCAL
			SetOutPath "$INSTDIR\plugins"
			goto LOCAL
	NOT_LOCAL:
			SetOutPath "$APPDATA\Notepad++"
	LOCAL:
		File "..\bin\plugins\NPPTextFX.ini"
		
		SetOutPath "$INSTDIR\plugins\NPPTextFX"
		File "..\bin\plugins\NPPTextFX\AsciiToEBCDIC.bin"
		File "..\bin\plugins\NPPTextFX\libTidy.dll"
		File "..\bin\plugins\NPPTextFX\W3C-CSSValidator.htm"
		File "..\bin\plugins\NPPTextFX\W3C-HTMLValidator.htm"
		
		SetOutPath "$INSTDIR\plugins\doc"
		File "..\bin\plugins\doc\NPPTextFXdemo.TXT"
	SectionEnd
/*
	Section "Function List" FunctionList
		Delete "$INSTDIR\plugins\FunctionListPlugin.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\FunctionList.dll"
	SectionEnd

	Section "File Browser" FileBrowser
		Delete "$INSTDIR\plugins\Explorer.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\Explorer.dll"
	SectionEnd
*/	
	Section "Light Explorer" FileBrowserLite
		Delete "$INSTDIR\plugins\LightExplorer.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\LightExplorer.dll"
	SectionEnd
/*	
	Section "Hex Editor" HexEditor
		Delete "$INSTDIR\plugins\HexEditorPlugin.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\HexEditor.dll"
	SectionEnd	

	Section "ConvertExt" ConvertExt
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\ConvertExt.dll"
		
		StrCmp $IS_LOCAL "1" 0 NOT_LOCAL2
			SetOutPath "$INSTDIR"
			goto LOCAL2
	NOT_LOCAL2:
			SetOutPath "$APPDATA\Notepad++"
	LOCAL2:
		File "..\bin\ConvertExt.ini"
		File "..\bin\ConvertExt.enc"
		File "..\bin\ConvertExt.lng"
	SectionEnd
*/

	Section "Spell-Checker" SpellChecker
		Delete "$INSTDIR\plugins\SpellChecker.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\SpellChecker.dll"
	SectionEnd

	Section "NppExec" NppExec
		Delete "$INSTDIR\plugins\NppExec.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\NppExec.dll"
		SetOutPath "$INSTDIR\plugins\doc"
		File "..\bin\plugins\doc\NppExec.txt"
		File "..\bin\plugins\doc\NppExec_TechInfo.txt"
	SectionEnd
/*
	Section "QuickText" QuickText
		Delete "$INSTDIR\plugins\QuickText.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\QuickText.dll"
		SetOutPath "$INSTDIR\"
		File "..\bin\QuickText.ini"
		SetOutPath "$INSTDIR\plugins\doc"
		File "..\bin\plugins\doc\quickText_README.txt"
	SectionEnd
*/
	Section "MIME Tools" MIMETools
		Delete "$INSTDIR\plugins\NppTools.dll"
		Delete "$INSTDIR\plugins\mimeTools.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\mimeTools.dll"
	SectionEnd
	
	Section "FTP synchronize" FTP_synchronize
		Delete "$INSTDIR\plugins\FTP_synchronizeA.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\FTP_synchronizeA.dll"
		SetOutPath "$INSTDIR\plugins\doc"
		File "..\bin\plugins\doc\FTP_synchonize.ReadMe.txt"
	SectionEnd
	
	Section "NppExport" NppExport
		Delete "$INSTDIR\plugins\NppExport.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\NppExport.dll"
	SectionEnd
	
	Section "Compare Plugin" ComparePlugin
		Delete "$INSTDIR\plugins\ComparePlugin.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\ComparePlugin.dll"
	SectionEnd
	
	Section "Document Monitor" DocMonitor
		Delete "$INSTDIR\plugins\docMonitor.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\docMonitor.dll"
	SectionEnd	
SubSectionEnd

Section /o "As default html viewer" htmlViewer
	SetOutPath "$INSTDIR\"
	File "..\bin\nppIExplorerShell.exe"
	WriteRegStr HKLM "SOFTWARE\Microsoft\Internet Explorer\View Source Editor\Editor Name" "" "$INSTDIR\nppIExplorerShell.exe"
SectionEnd

InstType "o"

Section "Auto-Updater" AutoUpdater
	SetOutPath "$INSTDIR\updater"
	File "..\bin\updater\GUP.exe"
	File "..\bin\updater\libcurl.dll"
	File "..\bin\updater\gup.xml"
	File "..\bin\updater\License.txt"
	File "..\bin\updater\gpl.txt"
	File "..\bin\updater\readme.txt"
	File "..\bin\updater\getDownLoadUrl.php"
SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  
  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${makeLocal} 'Enable this option to make Notepad++ load/write the configuration files from/to its install directory. Check it if you use Notepad++ in an USB device.'
    !insertmacro MUI_DESCRIPTION_TEXT ${explorerContextMenu} 'Explorer context menu entry for Notepad++ : Open whatever you want in Notepad++ from Windows Explorer.'
    !insertmacro MUI_DESCRIPTION_TEXT ${autoCompletionComponent} 'Install the API files you need for the auto-completion feature (Ctrl+Space).'
    !insertmacro MUI_DESCRIPTION_TEXT ${Plugins} 'You may need those plugins to extend the capacity of Notepad++.'
    !insertmacro MUI_DESCRIPTION_TEXT ${htmlViewer} 'Open the html file in Notepad++ while you choose <view source> from IE.'
    !insertmacro MUI_DESCRIPTION_TEXT ${AutoUpdater} 'Keep your Notepad++ update: Check this option to install an update module which searches Notepad++ update on Internet and install it for you.'
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------

Section -FinishSection

	WriteRegStr HKLM "Software\${APPNAME}" "" "$INSTDIR"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayName" "${APPNAME}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteUninstaller "$INSTDIR\uninstall.exe"

SectionEnd


;Uninstall section

SubSection un.autoCompletionComponent
	Section un.PHP
		Delete "$INSTDIR\plugins\APIs\php.api"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd

	Section un.CSS
		Delete "$INSTDIR\plugins\APIs\css.api"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd

	Section un.VB
		Delete "$INSTDIR\plugins\APIs\vb.api"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd

	Section un.Perl
		Delete "$INSTDIR\plugins\APIs\perl.api"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd

	Section un.C
		Delete "$INSTDIR\plugins\APIs\c.api"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd
	
	Section un.C++
		Delete "$INSTDIR\plugins\APIs\cpp.api"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd
	
	Section un.Java
		Delete "$INSTDIR\plugins\APIs\java.api"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd
	
	Section un.C#
		Delete "$INSTDIR\plugins\APIs\cs.api"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd
	
	Section un.JavaScript
		Delete "$INSTDIR\plugins\APIs\javascript.api"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd

	Section un.Python
		Delete "$INSTDIR\plugins\APIs\python.api"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd

	Section un.ActionScript
		Delete "$INSTDIR\plugins\APIs\flash.api"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd
	
	Section un.LISP
		Delete "$INSTDIR\plugins\APIs\lisp.api"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd
	
	Section un.VHDL
		Delete "$INSTDIR\plugins\APIs\vhdl.api"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd	
	
	Section un.TeX
		Delete "$INSTDIR\plugins\APIs\tex.api"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd
	
	Section un.DocBook
		Delete "$INSTDIR\plugins\APIs\xml.api"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd
	
	Section un.NSIS
		Delete "$INSTDIR\plugins\APIs\nsis.api"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd		
SubSectionEnd

SubSection un.Plugins
	Section un.NPPTextFX
		Delete "$INSTDIR\plugins\NPPTextFX.dll"
		Delete "$INSTDIR\plugins\NPPTextFX.ini"
		Delete "$APPDATA\Notepad++\NPPTextFX.ini"
		Delete "$INSTDIR\plugins\NPPTextFX\AsciiToEBCDIC.bin"
		Delete "$INSTDIR\plugins\NPPTextFX\libTidy.dll"
		Delete "$INSTDIR\plugins\doc\NPPTextFXdemo.TXT"
		Delete "$INSTDIR\plugins\NPPTextFX\W3C-CSSValidator.htm"
		Delete "$INSTDIR\plugins\NPPTextFX\W3C-HTMLValidator.htm"
		
		RMDir "$INSTDIR\plugins\NPPTextFX\"
		RMDir "$INSTDIR\plugins\"
  SectionEnd
/*
	Section un.FunctionList
		Delete "$INSTDIR\plugins\FunctionList.dll"
		RMDir "$INSTDIR\plugins\"
	SectionEnd

	Section un.FileBrowser
		Delete "$INSTDIR\plugins\Explorer.dll"
		Delete "$INSTDIR\Explorer.ini"
		RMDir "$INSTDIR\plugins\"
	SectionEnd
*/	
	Section un.FileBrowserLite
		Delete "$INSTDIR\plugins\LightExplorer.dll"
		Delete "$INSTDIR\lightExplorer.ini"
		RMDir "$INSTDIR\plugins\"
	SectionEnd
/*	
	Section un.HexEditor
		Delete "$INSTDIR\plugins\HexEditor.dll"
		RMDir "$INSTDIR\plugins\"
	SectionEnd

	Section un.ConvertExt
		Delete "$INSTDIR\plugins\ConvertExt.dll"

		Delete "$APPDATA\Notepad++\ConvertExt.ini"
		Delete "$APPDATA\Notepad++\ConvertExt.enc"
		Delete "$APPDATA\Notepad++\ConvertExt.lng"
		Delete "$INSTDIR\ConvertExt.ini"
		Delete "$INSTDIR\ConvertExt.enc"
		Delete "$INSTDIR\ConvertExt.lng"
		
		RMDir "$INSTDIR\plugins\"
	SectionEnd
*/

	Section un.SpellChecker
		Delete "$INSTDIR\plugins\SpellChecker.dll"
		RMDir "$INSTDIR\plugins\"
	SectionEnd

	Section un.NppExec
		Delete "$INSTDIR\plugins\NppExec.dll"
		Delete "$INSTDIR\plugins\doc\NppExec.txt"
		Delete "$INSTDIR\plugins\doc\NppExec_TechInfo.txt"
		Delete "$INSTDIR\plugins\Config\NppExec.ini"
		RMDir "$INSTDIR\plugins\"
		RMDir "$INSTDIR\plugins\doc\"
	SectionEnd
/*
	Section un.QuickText
		Delete "$INSTDIR\plugins\QuickText.dll"
		Delete "$INSTDIR\QuickText.ini"
		Delete "$INSTDIR\plugins\doc\quickText_README.txt"
		RMDir "$INSTDIR\plugins\"
	SectionEnd
*/
	Section un.NppTools
		Delete "$INSTDIR\plugins\NppTools.dll"
		RMDir "$INSTDIR\plugins\"
	SectionEnd
	
	Section un.FTP_synchronize
		Delete "$INSTDIR\plugins\FTP_synchronizeA.dll"
		Delete "$INSTDIR\plugins\doc\FTP_synchonize.ReadMe.txt"
		RMDir "$INSTDIR\plugins\"
	SectionEnd
	
	Section un.NppExport
		Delete "$INSTDIR\plugins\NppExport.dll"
		RMDir "$INSTDIR\plugins\"
	SectionEnd
	
	Section un.ComparePlugin
		Delete "$INSTDIR\plugins\ComparePlugin.dll"
		RMDir "$INSTDIR\plugins\"
	SectionEnd
	
	Section un.DocMonitor
		Delete "$INSTDIR\plugins\docMonitor.dll"
		RMDir "$INSTDIR\plugins\"
	SectionEnd	
SubSectionEnd

Section un.htmlViewer
	DeleteRegKey HKLM "SOFTWARE\Microsoft\Internet Explorer\View Source Editor"
	Delete "$INSTDIR\nppIExplorerShell.exe"
SectionEnd

Section un.AutoUpdater
	Delete "$INSTDIR\updater\GUP.exe"
	Delete "$INSTDIR\updater\libcurl.dll"
	Delete "$INSTDIR\updater\gup.xml"
	Delete "$INSTDIR\updater\License.txt"
	Delete "$INSTDIR\updater\gpl.txt"
	Delete "$INSTDIR\updater\readme.txt"
	Delete "$INSTDIR\updater\getDownLoadUrl.php"
	RMDir "$INSTDIR\updater\"
SectionEnd  

Section un.explorerContextMenu
	Exec 'regsvr32 /u /s "$INSTDIR\nppcm.dll"'
	Delete "$INSTDIR\nppcm.dll"
SectionEnd

Section Uninstall
	;Remove from registry...
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
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
	Delete "$INSTDIR\license.txt"

	Delete "$INSTDIR\notepad++.exe"
	Delete "$INSTDIR\readme.txt"
	
	
	Delete "$INSTDIR\config.xml"
	Delete "$INSTDIR\config.model.xml"
	Delete "$INSTDIR\langs.xml"
	Delete "$INSTDIR\langs.model.xml"
	Delete "$INSTDIR\stylers.xml"
	Delete "$INSTDIR\stylers.model.xml"
	Delete "$INSTDIR\contextMenu.xml"
	Delete "$INSTDIR\shortcuts.xml"
	Delete "$INSTDIR\nativeLang.xml"
	Delete "$INSTDIR\session.xml"
	
	SetShellVarContext current
	Delete "$APPDATA\Notepad++\config.xml"
	Delete "$APPDATA\Notepad++\stylers.xml"
	Delete "$APPDATA\Notepad++\contextMenu.xml"
	Delete "$APPDATA\Notepad++\shortcuts.xml"
	Delete "$APPDATA\Notepad++\nativeLang.xml"
	Delete "$APPDATA\Notepad++\session.xml"
	Delete "$APPDATA\Notepad++\insertExt.ini"
	
	RMDir "$APPDATA\Notepad++"
	
	StrCmp $1 "Admin" 0 +2
		SetShellVarContext all
		
	; Remove remaining directories
	RMDir "$SMPROGRAMS\Notepad++"
	RMDir "$INSTDIR\"
	RMDir "$APPDATA\Notepad++"

SectionEnd

Function un.onInit

  !insertmacro MUI_UNGETLANGUAGE

FunctionEnd

BrandingText "Don HO"

; eof