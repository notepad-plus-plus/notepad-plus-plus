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


${MementoSection} "Context Menu Entry" explorerContextMenu
	SetOverwrite try
	SetOutPath "$INSTDIR\"
	
	; There is no need to keep x86 NppShell_06.dll in 64 bit installer
	; But in 32bit installer both the Dlls are required
	; 	As user can install 32bit npp version on x64 bit machine, that time x64 bit NppShell is required.
	
	!ifdef ARCH64
		File /oname=$INSTDIR\NppShell_06.dll "..\bin\NppShell64_06.dll"
	!else
		${If} ${RunningX64}
			File /oname=$INSTDIR\NppShell_06.dll "..\bin\NppShell64_06.dll"
		${Else}
			File "..\bin\NppShell_06.dll"
		${EndIf}
	!endif
	
	Exec 'regsvr32 /s "$INSTDIR\NppShell_06.dll"'
${MementoSectionEnd}

SectionGroup "Plugins" Plugins
	SetOverwrite on
!ifndef ARCH64
	${MementoSection} "NppExport" NppExport
		Delete "$INSTDIR\plugins\NppExport.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\NppExport.dll"
	${MementoSectionEnd}

	${MementoSection} "Plugin Manager" PluginManager
		Delete "$INSTDIR\plugins\PluginManager.dll"
		SetOutPath "$INSTDIR\plugins"
		File "..\bin\plugins\PluginManager.dll"
		SetOutPath "$UPDATE_PATH\plugins\Config"
		File "..\bin\plugins\Config\PluginManager.ini"
		SetOutPath "$INSTDIR\updater"
		File "..\bin\updater\gpup.exe"
	${MementoSectionEnd}
!endif

	${MementoSection} "Mime Tools" MimeTools
		Delete "$INSTDIR\plugins\mimeTools.dll"
		SetOutPath "$INSTDIR\plugins"
!ifdef ARCH64
		File "..\bin64\plugins\mimeTools.dll"
!else
		File "..\bin\plugins\mimeTools.dll"
!endif
	${MementoSectionEnd}
	
	${MementoSection} "Converter" Converter
		Delete "$INSTDIR\plugins\NppConverter.dll"
		SetOutPath "$INSTDIR\plugins"
!ifdef ARCH64
		File "..\bin64\plugins\NppConverter.dll"
!else
		File "..\bin\plugins\NppConverter.dll"
!endif
	${MementoSectionEnd}
SectionGroupEnd

${MementoSection} "Auto-Updater" AutoUpdater
	SetOverwrite on
	SetOutPath "$INSTDIR\updater"
!ifdef ARCH64
	File "..\bin64\updater\GUP.exe"
	File "..\bin64\updater\libcurl.dll"
	File "..\bin64\updater\gup.xml"
	File "..\bin64\updater\LICENSE"
	File "..\bin64\updater\gpl.txt"
	File "..\bin64\updater\README.md"
!else
	File "..\bin\updater\GUP.exe"
	File "..\bin\updater\libcurl.dll"
	File "..\bin\updater\gup.xml"
	File "..\bin\updater\LICENSE"
	File "..\bin\updater\gpl.txt"
	File "..\bin\updater\README.md"
!endif
${MementoSectionEnd}


;Uninstall section
SectionGroup un.Plugins
	Section un.NppExport
		Delete "$INSTDIR\plugins\NppExport.dll"
	SectionEnd
	
	Section un.Converter
		Delete "$INSTDIR\plugins\NppConverter.dll"
	SectionEnd
	
	Section un.MimeTools
		Delete "$INSTDIR\plugins\mimeTools.dll"
	SectionEnd

	Section un.PluginManager
		Delete "$INSTDIR\plugins\PluginManager.dll"
		Delete "$UPDATE_PATH\plugins\Config\PluginManager.ini"
		Delete "$INSTDIR\updater\gpup.exe"
		RMDir "$INSTDIR\updater\"
	SectionEnd	

SectionGroupEnd

Section un.AutoUpdater
	Delete "$INSTDIR\updater\GUP.exe"
	Delete "$INSTDIR\updater\libcurl.dll"
	Delete "$INSTDIR\updater\gup.xml"
	Delete "$INSTDIR\updater\License.txt"
	Delete "$INSTDIR\updater\LICENSE"
	Delete "$INSTDIR\updater\gpl.txt"
	Delete "$INSTDIR\updater\readme.txt"
	Delete "$INSTDIR\updater\README.md"
	Delete "$INSTDIR\updater\getDownLoadUrl.php"
	RMDir "$INSTDIR\updater\"
SectionEnd 
