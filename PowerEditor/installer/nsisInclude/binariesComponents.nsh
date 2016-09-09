${MementoSection} "Context Menu Entry" explorerContextMenu
	SetOverwrite try
	SetOutPath "$INSTDIR\"
	${If} ${RunningX64}
		File /oname=$INSTDIR\NppShell_06.dll "..\bin\NppShell64_06.dll"
	${Else}
		File "..\bin\NppShell_06.dll"
	${EndIf}
	
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
