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
!define APPVERSION "5.2"
!define APPNAMEANDVERSION "Notepad++ v5.2"
!define APPWEBSITE "http://notepad-plus.sourceforge.net/"

!define VERSION_MAJOR 5
!define VERSION_MINOR 2

; Main Install settings
Name "${APPNAMEANDVERSION}"
InstallDir "$PROGRAMFILES\Notepad++"
InstallDirRegKey HKLM "Software\${APPNAME}" ""
OutFile "..\bin\npp.5.2.Installer.exe"

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

Function LaunchNpp
  Exec '"$INSTDIR\notepad++.exe" "$INSTDIR\change.log" '
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



!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\license.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN
;!define MUI_FINISHPAGE_RUN_TEXT "Run Npp"
!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchNpp"
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
  !insertmacro MUI_LANGUAGE "Thai"
  !insertmacro MUI_LANGUAGE "NorwegianNynorsk"
  !insertmacro MUI_LANGUAGE "Belarusian"
  !insertmacro MUI_LANGUAGE "Albanian"
  !insertmacro MUI_LANGUAGE "Malay"
  !insertmacro MUI_LANGUAGE "Galician"
  !insertmacro MUI_LANGUAGE "Basque"
  !insertmacro MUI_LANGUAGE "Luxembourgish"
  
  ;!insertmacro MUI_LANGUAGE "Latvian"
  ;!insertmacro MUI_LANGUAGE "Macedonian"
  ;!insertmacro MUI_LANGUAGE "Estonian"
  ; !insertmacro MUI_LANGUAGE "Mongolian"
  ;!insertmacro MUI_LANGUAGE "Breton"
  ;!insertmacro MUI_LANGUAGE "Icelandic"
  ;!insertmacro MUI_LANGUAGE "Bosnian"
  ;!insertmacro MUI_LANGUAGE "Kurdish"
  ;!insertmacro MUI_LANGUAGE "Irish"
  ;!insertmacro MUI_LANGUAGE "Uzbek"
  ;!insertmacro MUI_LANGUAGE "Afrikaans"

!insertmacro MUI_RESERVEFILE_LANGDLL

;Installer Functions


Function .onInit

	;Test if window9x
	Call GetWindowsVersion
	Pop $R0
	
	StrCmp $R0 "95" 0 +3
		MessageBox MB_OK "The installer contains only Unicode version of Notepad++, which is not compatible with your Windows 95.$\nPlease use ANSI version in zipped package, which you can download here :$\nhttps://sourceforge.net/project/showfiles.php?group_id=95717&package_id=102072"
		Abort
		
	StrCmp $R0 "98" 0 +3
		MessageBox MB_OK "The installer contains only Unicode version of Notepad++, which is not compatible with your Windows 98.$\nPlease use ANSI version in zipped package, which you can download here :$\nhttps://sourceforge.net/project/showfiles.php?group_id=95717&package_id=102072"
		Abort
		
	StrCmp $R0 "ME" 0 +3
		MessageBox MB_OK "The installer contains only Unicode version of Notepad++,, which is not compatible with your Windows ME.$\nPlease use ANSI version in zipped package, which you can download here :$\nhttps://sourceforge.net/project/showfiles.php?group_id=95717&package_id=102072"
		Abort
		
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
LangString langFileName ${LANG_THAI} "thai.xml"
LangString langFileName ${LANG_NORWEGIANNYNORSK} "nynorsk.xml"
LangString langFileName ${LANG_BELARUSIAN} "belarusian.xml"
LangString langFileName ${LANG_ALBANIAN} "albanian.xml"
LangString langFileName ${LANG_MALAY} "malay.xml"
LangString langFileName ${LANG_GALICIAN} "galician.xml"
LangString langFileName ${LANG_BASQUE} "basque.xml"
LangString langFileName ${LANG_LUXEMBOURGISH} "luxembourgish.xml"


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
	File "..\bin\NppHelp.chm"
	
	SetOutPath "$INSTDIR\localization\"
	File ".\nativeLang\"

	IfFileExists "$UPDATE_PATH\nativeLang.xml" 0 +2
		Delete "$UPDATE_PATH\nativeLang.xml"
		
	IfFileExists "$INSTDIR\nativeLang.xml" 0 +2
		Delete "$INSTDIR\nativeLang.xml"

	StrCmp $LANGUAGE ${LANG_ENGLISH} +2 0
	CopyFiles "$INSTDIR\localization\$(langFileName)" "$INSTDIR\nativeLang.xml"

	; remove all the npp shortcuts from current user
	Delete "$DESKTOP\Notepad++.lnk"
	Delete "$SMPROGRAMS\Notepad++\Notepad++.lnk"
	Delete "$SMPROGRAMS\Notepad++\readme.lnk"
	Delete "$SMPROGRAMS\Notepad++\Uninstall.lnk"
	CreateDirectory "$SMPROGRAMS\Notepad++"
	CreateShortCut "$SMPROGRAMS\Notepad++\Uninstall.lnk" "$INSTDIR\uninstall.exe"
	
	
	;clean
	Delete "$INSTDIR\plugins\NPPTextFX\AsciiToEBCDIC.bin"
	Delete "$INSTDIR\plugins\NPPTextFX\libTidy.dll"
	Delete "$INSTDIR\plugins\NPPTextFX\TIDYCFG.INI"
	Delete "$INSTDIR\plugins\NPPTextFX\W3C-CSSValidator.htm"
	Delete "$INSTDIR\plugins\NPPTextFX\W3C-HTMLValidator.htm"
	RMDir "$INSTDIR\plugins\NPPTextFX\"
	
	; remove unstable plugins
	IfFileExists "$INSTDIR\plugins\HexEditorPlugin.dll" 0 +3
		MessageBox MB_OK "Due to the problem of compability with this version,$\nHexEditorPlugin.dll is about to be deleted."
		Delete "$INSTDIR\plugins\HexEditorPlugin.dll"
/*
	IfFileExists "$INSTDIR\plugins\HexEditor.dll" 0 +3
		MessageBox MB_OK "Due to the problem of compability with this version,$\nHexEditor.dll is about to be deleted.$\nYou can download it via menu $\"?->Get more plugins$\" if you really need it."
		Delete "$INSTDIR\plugins\HexEditor.dll"
*/
	IfFileExists "$INSTDIR\plugins\MultiClipboard.dll" 0 +3	
		MessageBox MB_OK "Due to the problem of compability with this version,$\nMultiClipboard.dll is about to be deleted.$\nYou can download it via menu $\"?->Get more plugins$\" if you really need it."
		Delete "$INSTDIR\plugins\MultiClipboard.dll"
		
	Delete "$INSTDIR\plugins\NppDocShare.dll"

	IfFileExists "$INSTDIR\plugins\FunctionList.dll" 0 +3
		MessageBox MB_OK "Due to the problem of compability with this version,$\nFunctionList.dll is about to be deleted.$\nYou can download it via menu $\"?->Get more plugins$\" if you really need it."
		Delete "$INSTDIR\plugins\FunctionList.dll"
	
	IfFileExists "$INSTDIR\plugins\NPPTextFX.ini" 0 +2
		Delete "$INSTDIR\plugins\NPPTextFX.ini"
		 
	IfFileExists "$INSTDIR\plugins\NppAutoIndent.dll" 0 +3
		MessageBox MB_OK "Due to the stabilty issue,$\nNppAutoIndent.dll is about to be deleted.$\nYou can download it via menu $\"?->Get more plugins$\" if you really need it."
		Delete "$INSTDIR\plugins\NppAutoIndent.dll"
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
		File "..\bin\plugins\APIs\c.xml"
	SectionEnd
	
	Section C++
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\cpp.xml"
	SectionEnd

	Section Java
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\java.xml"
	SectionEnd
	
	Section C#
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\cs.xml"
	SectionEnd
	
	Section HTML
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\html.xml"
	SectionEnd
	
	Section RC
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\rc.xml"
	SectionEnd
	
	Section SQL
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\sql.xml"
	SectionEnd
	
	Section PHP
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\php.xml"
	SectionEnd

	Section CSS
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\css.xml"
	SectionEnd

	Section VB
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\vb.xml"
	SectionEnd

	Section Perl
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\perl.xml"
	SectionEnd
	
	Section JavaScript
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\javascript.xml"
	SectionEnd

	Section Python
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\python.xml"
	SectionEnd
	
	Section ActionScript
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\actionscript.xml"
	SectionEnd
	
	Section LISP
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\lisp.xml"
	SectionEnd
	
	Section VHDL
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\vhdl.xml"
	SectionEnd
	
	Section TeX
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\tex.xml"
	SectionEnd
	
	Section DocBook
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\xml.xml"
	SectionEnd
	
	Section NSIS
		SetOutPath "$INSTDIR\plugins\APIs"
		File "..\bin\plugins\APIs\nsis.xml"
	SectionEnd
		
SubSectionEnd

SubSection "Plugins" Plugins
	
	SetOverwrite on

	Section "NPPTextFX" NPPTextFX
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\NPPTextFX.dll"
		
		SetOutPath "$INSTDIR\plugins\Config\tidy"
		File "..\bin\plugins\Config\tidy\AsciiToEBCDIC.bin"
		File "..\bin\plugins\Config\tidy\libTidy.dll"
		File "..\bin\plugins\Config\tidy\TIDYCFG.INI"
		File "..\bin\plugins\Config\tidy\W3C-CSSValidator.htm"
		File "..\bin\plugins\Config\tidy\W3C-HTMLValidator.htm"
		
		SetOutPath "$INSTDIR\plugins\doc"
		File "..\bin\plugins\doc\NPPTextFXdemo.TXT"
	SectionEnd


	Section "NppNetNote" NppNetNote
		Delete "$INSTDIR\plugins\NppNetNote.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\NppNetNote.dll"
	SectionEnd
	

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
		File "..\bin\plugins\doc\NppExec_Guide.txt"
		File "..\bin\plugins\doc\NppExec_TechInfo.txt"
	SectionEnd

	Section "MIME Tools" MIMETools
		Delete "$INSTDIR\plugins\NppTools.dll"
		Delete "$INSTDIR\plugins\mimeTools.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\mimeTools.dll"
	SectionEnd
	
	Section "FTP synchronize" FTP_synchronize
		Delete "$INSTDIR\plugins\FTP_synchronizeA.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\FTP_synchronize.dll"
		SetOutPath "$INSTDIR\plugins\doc"
		File "..\bin\plugins\doc\FTP_synchonize.ReadMe.txt"
	SectionEnd
	
	Section "NppExport" NppExport
		Delete "$INSTDIR\plugins\NppExport.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\NppExport.dll"
	SectionEnd
	
	Section "ComparePlugin" ComparePlugin
		Delete "$INSTDIR\plugins\ComparePlugin.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\ComparePlugin.dll"
	SectionEnd
	
/*
	Section "NppAutoIndent" NppAutoIndent
		Delete "$INSTDIR\plugins\NppAutoIndent.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\NppAutoIndent.dll"
		
		StrCmp $IS_LOCAL "1" 0 NOT_LOCAL
			SetOutPath "$INSTDIR\plugins\Config\"
			goto LOCAL
	NOT_LOCAL:
			SetOutPath "$APPDATA\Notepad++\plugins\Config\"
	LOCAL:
		File "..\bin\plugins\Config\NppAutoIndent.ini"
		
	SectionEnd
*/
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
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayIcon" "$INSTDIR\notepad++.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayVersion" "${APPVERSION}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "URLInfoAbout" "${APPWEBSITE}"
	WriteUninstaller "$INSTDIR\uninstall.exe"

SectionEnd


;Uninstall section

SubSection un.autoCompletionComponent
	Section un.PHP
		Delete "$INSTDIR\plugins\APIs\php.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd

	Section un.CSS
		Delete "$INSTDIR\plugins\APIs\css.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd	
	
	Section un.HTML
		Delete "$INSTDIR\plugins\APIs\html.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd
	
	Section un.SQL
		Delete "$INSTDIR\plugins\APIs\sql.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd
	
	Section un.RC
		Delete "$INSTDIR\plugins\APIs\rc.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd

	Section un.VB
		Delete "$INSTDIR\plugins\APIs\vb.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd

	Section un.Perl
		Delete "$INSTDIR\plugins\APIs\perl.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd

	Section un.C
		Delete "$INSTDIR\plugins\APIs\c.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd
	
	Section un.C++
		Delete "$INSTDIR\plugins\APIs\cpp.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd
	
	Section un.Java
		Delete "$INSTDIR\plugins\APIs\java.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd
	
	Section un.C#
		Delete "$INSTDIR\plugins\APIs\cs.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd
	
	Section un.JavaScript
		Delete "$INSTDIR\plugins\APIs\javascript.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd

	Section un.Python
		Delete "$INSTDIR\plugins\APIs\python.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd

	Section un.ActionScript
		Delete "$INSTDIR\plugins\APIs\actionscript.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd
	
	Section un.LISP
		Delete "$INSTDIR\plugins\APIs\lisp.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd
	
	Section un.VHDL
		Delete "$INSTDIR\plugins\APIs\vhdl.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd	
	
	Section un.TeX
		Delete "$INSTDIR\plugins\APIs\tex.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd
	
	Section un.DocBook
		Delete "$INSTDIR\plugins\APIs\xml.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd
	
	Section un.NSIS
		Delete "$INSTDIR\plugins\APIs\nsis.xml"
		RMDir "$INSTDIR\plugins\APIs\"
	SectionEnd		
SubSectionEnd

SubSection un.Plugins
	Section un.NPPTextFX
		Delete "$INSTDIR\plugins\NPPTextFX.dll"
		Delete "$INSTDIR\plugins\NPPTextFX.ini"
		Delete "$APPDATA\Notepad++\NPPTextFX.ini"
		Delete "$INSTDIR\plugins\doc\NPPTextFXdemo.TXT"
		Delete "$INSTDIR\plugins\Config\tidy\AsciiToEBCDIC.bin"
		Delete "$INSTDIR\plugins\Config\tidy\libTidy.dll"
		Delete "$INSTDIR\plugins\Config\tidy\TIDYCFG.INI"
		Delete "$INSTDIR\plugins\Config\tidy\W3C-CSSValidator.htm"
		Delete "$INSTDIR\plugins\Config\tidy\W3C-HTMLValidator.htm"
		RMDir "$INSTDIR\plugins\tidy\"
		RMDir "$INSTDIR\plugins\"
  SectionEnd

	Section un.NppNetNote
		Delete "$INSTDIR\plugins\NppNetNote.dll"
		Delete "$INSTDIR\plugins\Config\NppNetNote.ini"
		RMDir "$INSTDIR\plugins\"
	SectionEnd

	Section un.NppAutoIndent
		Delete "$INSTDIR\plugins\NppAutoIndent.dll"
		Delete "$INSTDIR\plugins\Config\NppAutoIndent.ini"
		RMDir "$INSTDIR\plugins\"
	SectionEnd

	Section un.MIMETools
		Delete "$INSTDIR\plugins\NppTools.dll"
		Delete "$INSTDIR\plugins\mimeTools.dll"
		RMDir "$INSTDIR\plugins\"
	SectionEnd

	Section un.FTP_synchronize
		Delete "$INSTDIR\plugins\FTP_synchronize.dll"
		Delete "$INSTDIR\plugins\Config\FTP_synchronize.ini"
		Delete "$INSTDIR\plugins\doc\FTP_synchonize.ReadMe.txt"
		RMDir "$INSTDIR\plugins\"
	SectionEnd
	
	Section un.NppExport
		Delete "$INSTDIR\plugins\NppExport.dll"
		RMDir "$INSTDIR\plugins\"
	SectionEnd
	
	Section un.DocMonitor
		Delete "$INSTDIR\plugins\docMonitor.dll"
		Delete "$INSTDIR\plugins\Config\docMonitor.ini"
		RMDir "$INSTDIR\plugins\"
	SectionEnd	
	
	
	
	
	Section un.FileBrowserLite
		Delete "$INSTDIR\plugins\LightExplorer.dll"
		Delete "$INSTDIR\lightExplorer.ini"
		RMDir "$INSTDIR\plugins\"
	SectionEnd
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
	Section un.QuickText
		Delete "$INSTDIR\plugins\QuickText.dll"
		Delete "$INSTDIR\QuickText.ini"
		Delete "$INSTDIR\plugins\doc\quickText_README.txt"
		RMDir "$INSTDIR\plugins\"
	SectionEnd
	Section un.ComparePlugin
		Delete "$INSTDIR\plugins\ComparePlugin.dll"
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
