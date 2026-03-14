     1|     1|; This file is part of npminmin project
     2|     2|; Copyright (C)2021 Don HO <don.h@free.fr>
     3|     3|;
     4|     4|; This program is free software: you can redistribute it and/or modify
     5|     5|; it under the terms of the GNU General Public License as published by
     6|     6|; the Free Software Foundation, either version 3 of the License, or
     7|     7|; at your option any later version.
     8|     8|;
     9|     9|; This program is distributed in the hope that it will be useful,
    10|    10|; but WITHOUT ANY WARRANTY; without even the implied warranty of
    11|    11|; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    12|    12|; GNU General Public License for more details.
    13|    13|;
    14|    14|; You should have received a copy of the GNU General Public License
    15|    15|; along with this program.  If not, see <https://www.gnu.org/licenses/>.
    16|    16|
    17|    17|
    18|    18|; NSIS includes
    19|    19|!include "x64.nsh"       ; a few simple macros to handle installations on x64 machines
    20|    20|!include "MUI.nsh"       ; Modern UI
    21|    21|!include "nsDialogs.nsh" ; allows creation of custom pages in the installer
    22|    22|!include "Memento.nsh"   ; remember user selections in the installer across runs
    23|    23|!include "FileFunc.nsh"
    24|    24|
    25|    25|Unicode true			; Generate a Unicode installer. It can only be used outside of sections and functions and before any data is compressed.
    26|    26|SetCompressor /SOLID lzma	; This reduces installer size by approx 30~35%
    27|    27|;SetCompressor /FINAL lzma	; This reduces installer size by approx 15~18%
    28|    28|
    29|    29|
    30|    30|; Installer is DPI-aware: not scaled by the DWM, no blurry text
    31|    31|ManifestDPIAware true
    32|    32|
    33|    33|Var winSysDir
    34|    34|
    35|    35|!include "nsisInclude\winVer.nsh"
    36|    36|!include "nsisInclude\globalDef.nsh"
    37|    37|!include "nsisInclude\tools.nsh"
    38|    38|!include "nsisInclude\uninstall.nsh"
    39|    39|
    40|    40|!ifdef ARCH64
    41|    41|OutFile ".\build\npp.${APPVERSION}.Installer.x64.exe"
    42|    42|!else ifdef ARCHARM64
    43|    43|OutFile ".\build\npp.${APPVERSION}.Installer.arm64.exe"
    44|    44|!else
    45|    45|OutFile ".\build\npp.${APPVERSION}.Installer.exe"
    46|    46|!endif
    47|    47|
    48|    48|; Sign uninstaller
    49|    49|!uninstfinalize  'sign-installers.bat "%1"' = 0     ; %1 is replaced by the uninstaller exe to be signed.
    50|    50|
    51|    51|; ------------------------------------------------------------------------
    52|    52|; Version Information
    53|    53|   VIProductVersion	"${Version}"
    54|    54|   VIAddVersionKey	"ProductName"		"${APPNAME}"
    55|    55|   VIAddVersionKey	"CompanyName"		"${CompanyName}"
    56|    56|   VIAddVersionKey	"LegalCopyright"	"${LegalCopyright}"
    57|    57|   VIAddVersionKey	"FileDescription"	"${Description}"
    58|    58|   VIAddVersionKey	"FileVersion"		"${Version}"
    59|    59|   VIAddVersionKey	"ProductVersion"	"${ProdVer}"
    60|    60|; ------------------------------------------------------------------------
    61|    61|
    62|    62|; Insert CheckIfRunning function as an installer and uninstaller function.
    63|    63|Var runningNppDetected
    64|    64|!insertmacro CheckIfRunning ""
    65|    65|!insertmacro CheckIfRunning "un."
    66|    66|
    67|    67|; Modern interface settings
    68|    68|!define MUI_ICON ".\images\npp_inst.ico"
    69|    69|!define MUI_UNICON ".\images\npp_inst.ico"
    70|    70|
    71|    71|!define MUI_WELCOMEFINISHPAGE_BITMAP ".\images\wizard.bmp"
    72|    72|;!define MUI_WELCOMEFINISHPAGE_BITMAP ".\images\wizard_GiletJaune.bmp"
    73|    73|
    74|    74|
    75|    75|!define MUI_HEADERIMAGE
    76|    76|!define MUI_HEADERIMAGE_BITMAP ".\images\headerLeft.bmp" ; optional
    77|    77|!define MUI_HEADERIMAGE_BITMAP_RTL ".\images\headerLeft_RTL.bmp" ; Header for RTL languages
    78|    78|!define MUI_ABORTWARNING
    79|    79|!define MUI_COMPONENTSPAGE_SMALLDESC ;Show components page with a small description and big box for components
    80|    80|
    81|    81|
    82|    82|!insertmacro MUI_PAGE_WELCOME
    83|    83|!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
    84|    84|!insertmacro MUI_PAGE_DIRECTORY
    85|    85|!insertmacro MUI_PAGE_COMPONENTS
    86|    86|page Custom ExtraOptions
    87|    87|!define MUI_PAGE_CUSTOMFUNCTION_SHOW "CheckIfRunning"
    88|    88|!insertmacro MUI_PAGE_INSTFILES
    89|    89|
    90|    90|
    91|    91|!define MUI_FINISHPAGE_RUN
    92|    92|!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchNpp"
    93|    93|!insertmacro MUI_PAGE_FINISH
    94|    94|
    95|    95|
    96|    96|!insertmacro MUI_UNPAGE_CONFIRM
    97|    97|!define MUI_PAGE_CUSTOMFUNCTION_SHOW "un.CheckIfRunning"
    98|    98|!insertmacro MUI_UNPAGE_INSTFILES
    99|    99|
   100|   100|Var diffArchDir2Remove
   101|   101|; Var noUpdater removed: no updater in npminmin
   102|   102|Var closeRunningNpp
   103|   103|Var runNppAfterSilentInstall
   104|   104|Var relaunchNppAfterSilentInstall
   105|   105|
   106|   106|
   107|   107|!include "nsisInclude\langs4Installer.nsh"
   108|   108|
   109|   109|!include "nsisInclude\mainSectionFuncs.nsh"
   110|   110|
   111|   111|Section -"setPathAndOptionsSection" setPathAndOptionsSection
   112|   112|	Call setPathAndOptions
   113|   113|SectionEnd
   114|   114|
   115|   115|!include "nsisInclude\autoCompletion.nsh"
   116|   116|
   117|   117|!include "nsisInclude\functionList.nsh"
   118|   118|
   119|   119|!include "nsisInclude\binariesComponents.nsh"
   120|   120|
   121|   121|InstType "Minimalist"
   122|   122|
   123|   123|
   124|   124|
   125|   125|!ifdef ARCH64 || ARCHARM64
   126|   126|; this is needed for the 64-bit InstallDirRegKey patch
   127|   127|!include "StrFunc.nsh"
   128|   128|${StrStr} # Supportable for Install Sections and Functions
   129|   129|!endif
   130|   130|
   131|   131|Function .onInit
   132|   132|
   133|   133|	; --- PATCH BEGIN (it can be deleted without side-effects, when the NSIS N++ x64 installer binary becomes x64 too)---
   134|   134|	;
   135|   135|	; 64-bit patch for the NSIS attribute InstallDirRegKey (used in globalDef.nsh)
   136|   136|	; - this is needed because of the NSIS binary, generated for 64-bit npminmin installations, is still a 32-bit app,
   137|   137|	;   so the InstallDirRegKey checks for the irrelevant HKLM\SOFTWARE\WOW6432Node\npminmin, explanation:
   138|   138|	;   https://nsis.sourceforge.io/Reference/SetRegView
   139|   139|	;
   140|   140|!ifdef ARCH64 || ARCHARM64	; installation of 64 bits npminmin & its 64 bits components
   141|   141|	${If} ${RunningX64} ; Windows 64 bits
   142|   142|		System::Call kernel32::GetCommandLine()t.r0 ; get the original cmdline (where a possible "/D=..." is not hidden from us by NSIS)
   143|   143|		${StrStr} $1 $0 "/D="
   144|   144|		${If} "$1" == ""
   145|   145|			; "/D=..." was NOT used for sure, so we can continue in this InstallDirRegKey x64 patch 
   146|   146|			SetRegView 64 ; disable registry redirection
   147|   147|			ReadRegStr $0 HKLM "Software\${APPNAME}" ""
   148|   148|			${If} "$0" != ""
   149|   149|				; a previous installation path has been detected, so offer that as the $INSTDIR
   150|   150|				StrCpy $INSTDIR "$0"
   151|   151|			${EndIf}
   152|   152|			SetRegView 32 ; restore the original state
   153|   153|		${EndIf}
   154|   154|	${EndIf}
   155|   155|!endif
   156|   156|	;
   157|   157|	; --- PATCH END ---
   158|   158|
   159|   159|	StrCpy $runningNppDetected "false" ; reset
   160|   160|
   161|   161|	; Begin of "/closeRunningNpp"
   162|   162|	${GetParameters} $R0 
   163|   163|	${GetOptions} $R0 "/closeRunningNpp" $R1 ; case insensitive 
   164|   164|	IfErrors 0 closeRunningNppYes
   165|   165|	StrCpy $closeRunningNpp "false"
   166|   166|	Goto closeRunningNppCheckDone
   167|   167|closeRunningNppYes:
   168|   168|	StrCpy $closeRunningNpp "true"
   169|   169|closeRunningNppCheckDone:
   170|   170|	${If} $closeRunningNpp == "true"
   171|   171|		; First try to use the usual app-closing by sending the WM_CLOSE.
   172|   172|		; If that closing fails, use the forceful TerminateProcess way.
   173|   173|		!insertmacro FindAndCloseOrTerminateRunningNpp ; this has to precede the following silent mode npminmin instance mutex check
   174|   174|	${EndIf}
   175|   175|
   176|   176|	; handle the possible Silent Mode (/S) & already running npminmin (without this an incorrect partial installation is possible)
   177|   177|	IfSilent 0 notInSilentMode
   178|   178|	System::Call 'kernel32::OpenMutex(i 0x100000, b 0, t "nppInstance") i .R0'
   179|   179|	IntCmp $R0 0 nppNotRunning
   180|   180|	StrCpy $runningNppDetected "true"
   181|   181|	System::Call 'kernel32::CloseHandle(i $R0)' ; a npminmin instance is running, tidy-up the opened mutex handle only
   182|   182|	SetErrorLevel 5 ; set an exit code > 0 otherwise the installer returns 0 aka SUCCESS ('5' means here the future ERROR_ACCESS_DENIED when trying to overwrite the npminmin.exe file...)
   183|   183|	Quit ; silent installation is silent, we cannot continue here without a user interaction (or the installation should have been launched with the "/closeRunningNpp" param)
   184|   184|nppNotRunning:
   185|   185|notInSilentMode:
   186|   186|	; End of "/closeRunningNpp"
   187|   187|
   188|   188|	; /noUpdater flag removed: npminmin ships without auto-updater
   206|   206|
   207|   207|	; Begin of "/runNppAfterSilentInstall"
   208|   208|	${GetParameters} $R0 
   209|   209|	${GetOptions} $R0 "/runNppAfterSilentInstall" $R1 ;case insensitive 
   210|   210|	IfErrors noRunNpp runNpp
   211|   211|noRunNpp:
   212|   212|	StrCpy $runNppAfterSilentInstall "false"
   213|   213|	Goto runNppDone
   214|   214|runNpp:
   215|   215|	StrCpy $runNppAfterSilentInstall "true"
   216|   216|runNppDone:
   217|   217|	; End of "/runNppAfterSilentInstall"
   218|   218|
   219|   219|	; Begin of "/relaunchNppAfterSilentInstall"
   220|   220|	${GetParameters} $R0 
   221|   221|	${GetOptions} $R0 "/relaunchNppAfterSilentInstall" $R1 ;case insensitive 
   222|   222|	IfErrors noRelaunchNpp relaunchNpp
   223|   223|noRelaunchNpp:
   224|   224|	StrCpy $relaunchNppAfterSilentInstall "false"
   225|   225|	Goto relaunchNppDone
   226|   226|relaunchNpp:
   227|   227|	StrCpy $relaunchNppAfterSilentInstall "true"
   228|   228|relaunchNppDone:
   229|   229|	; End of "/relaunchNppAfterSilentInstall"
   230|   230|
   231|   231|	${If} ${SectionIsSelected} ${PluginsAdmin}
   232|   232|		!insertmacro SetSectionFlag ${AutoUpdater} ${SF_RO}
   233|   233|		!insertmacro SelectSection ${AutoUpdater}
   234|   234|	${Else}
   235|   235|		!insertmacro ClearSectionFlag ${AutoUpdater} ${SF_RO}
   236|   236|	${EndIf}
   237|   237|
   238|   238|	Call SetRoughEstimation		; This is rough estimation of files present in function copyCommonFiles
   239|   239|	InitPluginsDir			; Initializes the plug-ins dir ($PLUGINSDIR) if not already initialized.
   240|   240|	Call checkCompatibility		; check unsupported OSes and CPUs
   241|   241|		
   242|   242|	; look for previously selected language
   243|   243|	ClearErrors
   244|   244|	Var /GLOBAL tempLng
   245|   245|	ReadRegStr $tempLng HKLM "SOFTWARE\${APPNAME}" 'InstallerLanguage'
   246|   246|	${IfNot} ${Errors}
   247|   247|		StrCpy $LANGUAGE "$tempLng" ; set default language
   248|   248|	${EndIf}
   249|   249|	
   250|   250|	!insertmacro MUI_LANGDLL_DISPLAY
   251|   251|
   252|   252|	; save selected language to registry
   253|   253|	WriteRegStr HKLM "SOFTWARE\${APPNAME}" 'InstallerLanguage' '$Language'
   254|   254|
   255|   255|!ifdef ARCH64 || ARCHARM64 ; x64 or ARM64 : installation of 64 bits npminmin & its 64 bits components
   256|   256|	StrCpy $winSysDir $WINDIR\System32
   257|   257|	${If} ${RunningX64} ; Windows 64 bits
   258|   258|		; disable registry redirection (enable access to 64-bit portion of registry)
   259|   259|		SetRegView 64
   260|   260|		
   261|   261|		; change to x64 install dir if needed
   262|   262|		${If} "$InstDir" != ""
   263|   263|			${If} "$InstDir" == "$PROGRAMFILES\${APPNAME}"
   264|   264|				StrCpy $INSTDIR "$PROGRAMFILES64\${APPNAME}"
   265|   265|			${EndIf}
   266|   266|			; else /D was used or last installation is not "$PROGRAMFILES\${APPNAME}"
   267|   267|		${Else}
   268|   268|			StrCpy $INSTDIR "$PROGRAMFILES64\${APPNAME}"
   269|   269|		${EndIf}
   270|   270|		
   271|   271|		; check if 32-bit version has been installed if yes, ask user to remove it
   272|   272|		IfFileExists $PROGRAMFILES\${APPNAME}\npminmin.exe 0 noDelete32
   273|   273|		MessageBox MB_YESNO "You are trying to install 64-bit version while 32-bit version is already installed. Would you like to remove npminmin 32 bit version before proceeding further?$\n(Your custom config files will be kept)" /SD IDYES IDYES doDelete32 IDNO noDelete32 ;IDYES remove
   274|   274|doDelete32:
   275|   275|		StrCpy $diffArchDir2Remove $PROGRAMFILES\${APPNAME}
   276|   276|noDelete32:
   277|   277|		
   278|   278|	${Else} ; Windows 32 bits
   279|   279|		MessageBox MB_OK "You cannot install npminmin 64-bit version on your 32-bit system.$\nPlease download and install npminmin 32-bit version instead."
   280|   280|		Abort
   281|   281|	${EndIf}
   282|   282|
   283|   283|!else ; installation of 32 bits npminmin & its 32 bits components
   284|   284|	StrCpy $winSysDir $WINDIR\SysWOW64
   285|   285|	${If} ${RunningX64}  ; Windows 64 bits
   286|   286|		; check if 64-bit version has been installed if yes, ask user to remove it
   287|   287|		IfFileExists $PROGRAMFILES64\${APPNAME}\npminmin.exe 0 noDelete64
   288|   288|		MessageBox MB_YESNO "You are trying to install 32-bit version while 64-bit version is already installed. Would you like to remove npminmin 64 bit version before proceeding further?$\n(Your custom config files will be kept)"  /SD IDYES IDYES doDelete64 IDNO noDelete64
   289|   289|doDelete64:
   290|   290|		StrCpy $diffArchDir2Remove $PROGRAMFILES64\${APPNAME}
   291|   291|noDelete64:
   292|   292|	${EndIf}
   293|   293|
   294|   294|!endif
   295|   295|
   296|   296|	${MementoSectionRestore}
   297|   297|
   298|   298|FunctionEnd
   299|   299|
   300|   300|
   301|   301|Section -"npminmin" mainSection
   302|   302|	${If} $showDetailsChecked == ${BST_CHECKED}
   303|   303|		SetDetailsView show
   304|   304|		SetAutoClose false
   305|   305|	${endIf}
   306|   306|
   307|   307|	${If} $diffArchDir2Remove != ""
   308|   308|		!insertmacro uninstallRegKey
   309|   309|		!insertmacro uninstallDir $diffArchDir2Remove 
   310|   310|	${endIf}
   311|   311|
   312|   312|	Call copyCommonFiles
   313|   313|
   314|   314|	Call removeUnstablePlugins
   315|   315|
   316|   316|	Call removeOldContextMenu
   317|   317|
   318|   318|	Call shortcutLinkManagement
   319|   319|
   320|   320|SectionEnd
   321|   321|
   322|   322|; Please **DONOT** move this function (SetRoughEstimation) anywhere else
   323|   323|; Just keep it right after the "mainSection" section
   324|   324|; Otherwise rough estimation for copyCommonFiles will not be set
   325|   325|; which will become reason for showing 0.0KB size on components section page
   326|   326|
   327|   327|Function SetRoughEstimation
   328|   328|	SectionSetSize ${mainSection} 4500		; This is rough estimation of files present in function copyCommonFiles
   329|   329|FunctionEnd
   330|   330|
   331|   331|
   332|   332|!include "nsisInclude\langs4Npp.nsh"
   333|   333|
   334|   334|!include "nsisInclude\themes.nsh"
   335|   335|
   336|   336|
   337|   337|${MementoSection} "Context Menu Entry" explorerContextMenu
   338|   338|
   339|   339|	SetOverwrite try
   340|   340|	SetOutPath "$INSTDIR\contextMenu\"
   341|   341|	
   342|   342|
   343|   343|	IfFileExists $INSTDIR\contextmenu\NppShell.dll 0 +2
   344|   344|		ExecWait '"$winSysDir\rundll32.exe" "$INSTDIR\contextmenu\NppShell.dll",CleanupDll'
   345|   345|
   346|   346|
   347|   347|	!ifdef ARCH64
   348|   348|		File /oname=$INSTDIR\contextMenu\NppShell.msix "..\bin64\NppShell.msix"
   349|   349|		File /oname=$INSTDIR\contextMenu\NppShell.dll "..\bin64\NppShell.x64.dll"
   350|   350|	!else ifdef ARCHARM64
   351|   351|		File /oname=$INSTDIR\contextMenu\NppShell.msix "..\binarm64\NppShell.msix"
   352|   352|		File /oname=$INSTDIR\contextMenu\NppShell.dll "..\binarm64\NppShell.arm64.dll"
   353|   353|	!else
   354|   354|		; We need to test which arch we are running on, since 32bit exe can be run on both 32bit and 64bit Windows.
   355|   355|		${If} ${RunningX64}
   356|   356|			; We are running on 64bit Windows, so we need the msix as well, since it might be Windows 11.
   357|   357|			File /oname=$INSTDIR\contextMenu\NppShell.msix "..\bin64\NppShell.msix"
   358|   358|			File /oname=$INSTDIR\contextMenu\NppShell.dll "..\bin64\NppShell.x64.dll"
   359|   359|		${Else}
   360|   360|			; We are running on 32bit Windows, so no need for the msix file, since there is no way this could even be upgraded to Windows 11.
   361|   361|			File /oname=$INSTDIR\contextMenu\NppShell.dll "..\bin\NppShell.x86.dll"
   362|   362|		${EndIf}    
   363|   363|
   364|   364|	!endif
   365|   365|	
   366|   366|	ExecWait '"$winSysDir\regsvr32.exe" /s "$INSTDIR\contextMenu\NppShell.dll"'
   367|   367|
   368|   368|${MementoSectionEnd}
   369|   369|
   370|   370|${MementoSectionDone}
   371|   371|
   372|   372|;--------------------------------
   373|   373|  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
   374|   374|    !insertmacro MUI_DESCRIPTION_TEXT ${explorerContextMenu} 'Explorer context menu entry for npminmin : Open whatever you want in npminmin from Windows Explorer.'
   375|   375|    !insertmacro MUI_DESCRIPTION_TEXT ${autoCompletionComponent} 'Install the API files you need for the auto-completion feature (Ctrl+Space).'
   376|   376|    !insertmacro MUI_DESCRIPTION_TEXT ${functionListComponent} 'Install the function list files you need for the function list feature (Ctrl+Space).'
   377|   377|    !insertmacro MUI_DESCRIPTION_TEXT ${Plugins} 'You may need these plugins to extend the capabilities of npminmin.'
   378|   378|    !insertmacro MUI_DESCRIPTION_TEXT ${NppExport} 'Copy your syntax highlighted source code as HTML/RTF into clipboard, or save them as HTML/RTF files.'
   379|   379|    !insertmacro MUI_DESCRIPTION_TEXT ${MimeTools} 'Encode/decode selected text with Base64, Quoted-printable, URL encoding, and SAML.'
   380|   380|    !insertmacro MUI_DESCRIPTION_TEXT ${Converter} 'Convert ASCII to binary, octal, hexadecimal and decimal string.'
   381|   381|    !insertmacro MUI_DESCRIPTION_TEXT ${localization} 'To use npminmin in your favorite language(s), install all/desired language(s).'
   382|   382|    !insertmacro MUI_DESCRIPTION_TEXT ${Themes} 'The eye-candy to change visual effects. Use Theme selector to switch among them.'
   383|   383|    !insertmacro MUI_DESCRIPTION_TEXT ${AutoUpdater} 'Keep npminmin updated: Automatically download and install the latest updates.'
   384|   384|    !insertmacro MUI_DESCRIPTION_TEXT ${PluginsAdmin} 'Install, Update and Remove any plugin from a list by some clicks. It needs Auto-Updater installed.'
   385|   385|  !insertmacro MUI_FUNCTION_DESCRIPTION_END
   386|   386|;--------------------------------
   387|   387|
   388|   388|
   389|   389|
   390|   390|Function .onInstSuccess
   391|   391|	${MementoSectionSave}
   392|   392|FunctionEnd
   393|   393|
   394|   394|
   395|   395|; Keep "FinishSection" section in the last so that
   396|   396|; writing installation info happens in the last
   397|   397|; Specially for writing registry "EstimatedSize"
   398|   398|; which is visible in control panel in column named "size"
   399|   399|
   400|   400|Section -FinishSection
   401|   401|	Call writeInstallInfoInRegistry
   402|   402|	IfSilent 0 theEnd
   403|   403|	${If} $runNppAfterSilentInstall == "true"
   404|   404|		Call LaunchNpp ; always launch
   405|   405|	${ElseIf} $relaunchNppAfterSilentInstall == "true"
   406|   406|		${If} $runningNppDetected == "true"
   407|   407|			Call LaunchNpp ; relaunch
   408|   408|		${EndIf}
   409|   409|	${EndIf}
   410|   410|theEnd:
   411|   411|SectionEnd
   412|   412|
   413|   413|BrandingText "The best things in life are free. npminmin is free so npminmin is the best"
   414|   414|
   415|   415|; eof
   416|   416|