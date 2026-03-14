     1|; This file is part of npminmin project
     2|; Copyright (C)2021 Don HO <don.h@free.fr>
     3|;
     4|; This program is free software: you can redistribute it and/or modify
     5|; it under the terms of the GNU General Public License as published by
     6|; the Free Software Foundation, either version 3 of the License, or
     7|; at your option any later version.
     8|;
     9|; This program is distributed in the hope that it will be useful,
    10|; but WITHOUT ANY WARRANTY; without even the implied warranty of
    11|; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    12|; GNU General Public License for more details.
    13|;
    14|; You should have received a copy of the GNU General Public License
    15|; along with this program.  If not, see <https://www.gnu.org/licenses/>.
    16|
    17|Var UPDATE_PATH
    18|Var PLUGIN_INST_PATH
    19|Var USER_PLUGIN_CONF_PATH
    20|Var ALLUSERS_PLUGIN_CONF_PATH
    21|Function setPathAndOptions
    22|    ${If} $UPDATE_PATH == ""
    23|	${OrIf} $PLUGIN_INST_PATH == ""
    24|	${OrIf} $USER_PLUGIN_CONF_PATH == ""
    25|	${OrIf} $ALLUSERS_PLUGIN_CONF_PATH == ""
    26|        Goto initUpdatePath
    27|	${ELSE}
    28|        Goto alreadyDone
    29|	${EndIf}
    30|	
    31|initUpdatePath:
    32|    
    33|	; Set Section properties
    34|	SetOverwrite on
    35|	StrCpy $UPDATE_PATH $INSTDIR
    36|		
    37|	SetOutPath "$INSTDIR\"
    38|
    39|	StrCpy $PLUGIN_INST_PATH "$INSTDIR\plugins"
    40|	StrCpy $ALLUSERS_PLUGIN_CONF_PATH "$PLUGIN_INST_PATH\Config"
    41|	
    42|	; in Silent mode we cannot use the NSIS GUI for handling the doLocalConf mode
    43|	; but we need to directly use the previous user setting
    44|	IfSilent 0 +3
    45|	IfFileExists $INSTDIR\doLocalConf.xml 0 +2
    46|	StrCpy $noUserDataChecked ${BST_CHECKED}
    47|
    48|	${If} $noUserDataChecked == ${BST_CHECKED}
    49|
    50|		File "..\bin\doLocalConf.xml"
    51|		StrCpy $USER_PLUGIN_CONF_PATH "$ALLUSERS_PLUGIN_CONF_PATH"
    52|		CreateDirectory $PLUGIN_INST_PATH\config
    53|	${ELSE}
    54|	
    55|		IfFileExists $INSTDIR\doLocalConf.xml 0 +2
    56|		Delete $INSTDIR\doLocalConf.xml
    57|		
    58|		StrCpy $USER_PLUGIN_CONF_PATH "$APPDATA\${APPNAME}\plugins\Config"
    59|		StrCpy $UPDATE_PATH "$APPDATA\${APPNAME}"
    60|		CreateDirectory $UPDATE_PATH\plugins\config
    61|	${EndIf}
    62|	
    63|	; WriteIniStr "$INSTDIR\uninstall.ini" "Uninstall" "UPDATE_PATH" $UPDATE_PATH 
    64|	; WriteIniStr "$INSTDIR\uninstall.ini" "Uninstall" "PLUGIN_INST_PATH" $PLUGIN_INST_PATH 
    65|	; WriteIniStr "$INSTDIR\uninstall.ini" "Uninstall" "USER_PLUGIN_CONF_PATH" $USER_PLUGIN_CONF_PATH 
    66|	; WriteIniStr "$INSTDIR\uninstall.ini" "Uninstall" "ALLUSERS_PLUGIN_CONF_PATH" $ALLUSERS_PLUGIN_CONF_PATH 
    67|
    68|alreadyDone:
    69|FunctionEnd
    70|
    71|Function un.setPathAndOptions
    72|    ReadINIStr $UPDATE_PATH "$INSTDIR\uninstall.ini" "Uninstall" "UPDATE_PATH" 
    73|    ReadINIStr $PLUGIN_INST_PATH "$INSTDIR\uninstall.ini" "Uninstall" "PLUGIN_INST_PATH" 
    74|    ReadINIStr $USER_PLUGIN_CONF_PATH "$INSTDIR\uninstall.ini" "Uninstall" "USER_PLUGIN_CONF_PATH" 
    75|    ReadINIStr $ALLUSERS_PLUGIN_CONF_PATH "$INSTDIR\uninstall.ini" "Uninstall" "ALLUSERS_PLUGIN_CONF_PATH" 
    76|FunctionEnd
    77|
    78|
    79|Function copyCommonFiles
    80|	SetOverwrite off
    81|	SetOutPath "$UPDATE_PATH\"
    82|	File "..\bin\contextMenu.xml"
    83|	File "..\src\tabContextMenu_example.xml"
    84|	File "..\src\toolbarButtonsConf_example.xml"
    85|
    86|	SetOverwrite on
    87|	SetOutPath "$INSTDIR\"
    88|	File "..\bin\langs.model.xml"
    89|	File "..\bin\stylers.model.xml"
    90|	File "..\bin\contextMenu.xml"
    91|
    92|	SetOverwrite off
    93|	File "..\bin\shortcuts.xml"
    94|	
    95|	; For debug logs
    96|	File "..\bin\nppLogNulContentCorruptionIssue.xml"
    97|
    98|	
    99|	; Set Section Files and Shortcuts
   100|	SetOverwrite on
   101|	File "..\..\LICENSE"
   102|	File "..\bin\change.log"
   103|	File "..\bin\readme.txt"
   104|	
   105|!ifdef ARCH64
   106|	File "..\bin64\npminmin.exe"
   107|!else ifdef ARCHARM64
   108|	File "..\binarm64\npminmin.exe"
   109|!else
   110|	File "..\bin\npminmin.exe"
   111|!endif
   112|
   113|	; Markdown in user defined languages
   114|	SetOutPath "$UPDATE_PATH\userDefineLangs\"
   115|	Delete "$UPDATE_PATH\userDefineLangs\userDefinedLang-markdown.default.modern.xml"
   116|	File "..\bin\userDefineLangs\markdown._preinstalled.udl.xml"
   117|	File "..\bin\userDefineLangs\markdown._preinstalled_DM.udl.xml"
   118|
   119|	; Localization
   120|	; Default language English 
   121|	SetOutPath "$INSTDIR\localization\"
   122|	File ".\nativeLang\english.xml"
   123|
   124|	; Copy all the npminmin localization files to the temp directory
   125|	; than make them installed via option
   126|	SetOutPath "$PLUGINSDIR\nppLocalization\"
   127|	File ".\nativeLang\"
   128|
   129|	IfFileExists "$UPDATE_PATH\nativeLang.xml" 0 +2
   130|		Delete "$UPDATE_PATH\nativeLang.xml"
   131|		
   132|	IfFileExists "$INSTDIR\nativeLang.xml" 0 +2
   133|		Delete "$INSTDIR\nativeLang.xml"
   134|
   135|	StrCmp $LANGUAGE ${LANG_ENGLISH} +5 0
   136|	CopyFiles "$PLUGINSDIR\nppLocalization\$(langFileName)" "$UPDATE_PATH\nativeLang.xml"
   137|	CopyFiles "$PLUGINSDIR\nppLocalization\$(langFileName)" "$INSTDIR\localization\$(langFileName)"
	; removed: auto-updater localization (npminmin has no updater)
	; removed: auto-updater localization (npminmin has no updater)
   140|
   141|FunctionEnd
   142|
   143|; Source from: https://nsis.sourceforge.io/VersionCompare
   144|Function VersionCompare
   145|	!define VersionCompare `!insertmacro VersionCompareCall`
   146| 
   147|	!macro VersionCompareCall _VER1 _VER2 _RESULT
   148|		Push `${_VER1}`
   149|		Push `${_VER2}`
   150|		Call VersionCompare
   151|		Pop ${_RESULT}
   152|	!macroend
   153| 
   154|	Exch $1
   155|	Exch
   156|	Exch $0
   157|	Exch
   158|	Push $2
   159|	Push $3
   160|	Push $4
   161|	Push $5
   162|	Push $6
   163|	Push $7
   164| 
   165|	begin:
   166|	StrCpy $2 -1
   167|	IntOp $2 $2 + 1
   168|	StrCpy $3 $0 1 $2
   169|	StrCmp $3 '' +2
   170|	StrCmp $3 '.' 0 -3
   171|	StrCpy $4 $0 $2
   172|	IntOp $2 $2 + 1
   173|	StrCpy $0 $0 '' $2
   174| 
   175|	StrCpy $2 -1
   176|	IntOp $2 $2 + 1
   177|	StrCpy $3 $1 1 $2
   178|	StrCmp $3 '' +2
   179|	StrCmp $3 '.' 0 -3
   180|	StrCpy $5 $1 $2
   181|	IntOp $2 $2 + 1
   182|	StrCpy $1 $1 '' $2
   183| 
   184|	StrCmp $4$5 '' equal
   185| 
   186|	StrCpy $6 -1
   187|	IntOp $6 $6 + 1
   188|	StrCpy $3 $4 1 $6
   189|	StrCmp $3 '0' -2
   190|	StrCmp $3 '' 0 +2
   191|	StrCpy $4 0
   192| 
   193|	StrCpy $7 -1
   194|	IntOp $7 $7 + 1
   195|	StrCpy $3 $5 1 $7
   196|	StrCmp $3 '0' -2
   197|	StrCmp $3 '' 0 +2
   198|	StrCpy $5 0
   199| 
   200|	StrCmp $4 0 0 +2
   201|	StrCmp $5 0 begin newer2
   202|	StrCmp $5 0 newer1
   203|	IntCmp $6 $7 0 newer1 newer2
   204| 
   205|	StrCpy $4 '1$4'
   206|	StrCpy $5 '1$5'
   207|	IntCmp $4 $5 begin newer2 newer1
   208| 
   209|	equal:
   210|	StrCpy $0 0
   211|	goto end
   212|	newer1:
   213|	StrCpy $0 1
   214|	goto end
   215|	newer2:
   216|	StrCpy $0 2
   217| 
   218|	end:
   219|	Pop $7
   220|	Pop $6
   221|	Pop $5
   222|	Pop $4
   223|	Pop $3
   224|	Pop $2
   225|	Pop $1
   226|	Exch $0
   227|FunctionEnd
   228|
   229|Function removeUnstablePlugins
   230|	; remove unstable plugins
   231|	CreateDirectory "$INSTDIR\plugins\disabled"
   232|	
   233|	; NppSaveAsAdmin makes npminmin crash. "1.0.211.0" is its 1st version which contains the fix
   234|	IfFileExists "$INSTDIR\plugins\NppSaveAsAdmin\NppSaveAsAdmin.dll" 0 NppSaveAsAdminTestEnd
   235|		${GetFileVersion} "$INSTDIR\plugins\NppSaveAsAdmin\NppSaveAsAdmin.dll" $R0
   236|		${VersionCompare} $R0 "1.0.211.0" $R1 ;   0: equal to 1.0.211.0   1: $R0 is newer   2: 1.0.211.0 is newer
   237|		StrCmp $R1 "0" +5 0 ; if equal skip all & go to end, else go to next
   238|		StrCmp $R1 "1" +4 0 ; if newer skip all & go to end, else older (2) then go to next
   239|		MessageBox MB_OK "Due to NppSaveAsAdmin plugin's incompatibility issue in version $R0, NppSaveAsAdmin.dll will be deleted. Use Plugins Admin to add back (the latest version of) NppSaveAsAdmin." /SD IDOK
   240|		Rename "$INSTDIR\plugins\NppSaveAsAdmin\NppSaveAsAdmin.dll" "$INSTDIR\plugins\disabled\NppSaveAsAdmin.dll"
   241|		Delete "$INSTDIR\plugins\NppSaveAsAdmin\NppSaveAsAdmin.dll"
   242|NppSaveAsAdminTestEnd:
   243|
   244|!ifdef ARCH64 || ARCHARM64 ; x64 or ARM64
   245|
   246|	; HexEditor makes npminmin x64 crash. "0.9.12" is its 1st version which contains the fix
   247|	IfFileExists "$INSTDIR\plugins\HexEditor\HexEditor.dll" 0 HexEditorTestEnd64
   248|		${GetFileVersion} "$INSTDIR\plugins\HexEditor\HexEditor.dll" $R0
   249|		${VersionCompare} $R0 "0.9.12" $R1 ;   0: equal to 0.9.12   1: $R0 is newer   2: 0.9.12 is newer
   250|		StrCmp $R1 "0" +5 0 ; if equal skip all & go to end, else go to next
   251|		StrCmp $R1 "1" +4 0 ; if newer skip all & go to end, else older (2) then go to next
   252|		MessageBox MB_OK "Due to HexEditor plugin's incompatibility issue in version $R0, HexEditor.dll will be deleted. Use Plugins Admin to add back (the latest version of) HexEditor." /SD IDOK
   253|		Rename "$INSTDIR\plugins\HexEditor\HexEditor.dll" "$INSTDIR\plugins\disabled\HexEditor.dll"
   254|		Delete "$INSTDIR\plugins\HexEditor\HexEditor.dll"
   255|HexEditorTestEnd64:
   256|
   257|	; ComparePlugin makes npminmin x64 crash. "2.0.2" is its 1st version which contains the fix
   258|	IfFileExists "$INSTDIR\plugins\ComparePlugin\ComparePlugin.dll" 0 CompareTestEnd64
   259|		${GetFileVersion} "$INSTDIR\plugins\ComparePlugin\ComparePlugin.dll" $R0
   260|		${VersionCompare} $R0 "2.0.2" $R1 ;   0: equal to 2.0.2   1: $R0 is newer   2: 2.0.2 is newer
   261|		StrCmp $R1 "0" +5 0 ; if equal skip all & go to end, else go to next
   262|		StrCmp $R1 "1" +4 0 ; if newer skip all & go to end, else older (2) then go to next
   263|		MessageBox MB_OK "Due to ComparePlugin plugin's incompatibility issue in version $R0, ComparePlugin.dll will be deleted. Use Plugins Admin to add back (the latest version of) ComparePlugin." /SD IDOK
   264|		Rename "$INSTDIR\plugins\ComparePlugin\ComparePlugin.dll" "$INSTDIR\plugins\disabled\ComparePlugin.dll"
   265|		Delete "$INSTDIR\plugins\ComparePlugin\ComparePlugin.dll"
   266|CompareTestEnd64:
   267|
   268|	; DSpellCheck makes npminmin x64 crash. "1.4.23" is its 1st version which contains the fix
   269|	IfFileExists "$INSTDIR\plugins\DSpellCheck\DSpellCheck.dll" 0 DSpellCheckTestEnd64
   270|		${GetFileVersion} "$INSTDIR\plugins\DSpellCheck\DSpellCheck.dll" $R0
   271|		${VersionCompare} $R0 "1.4.23" $R1 ;   0: equal to 1.4.23   1: $R0 is newer   2: 1.4.23 is newer
   272|		StrCmp $R1 "0" +5 0 ; if equal skip all & go to end, else go to next
   273|		StrCmp $R1 "1" +4 0 ; if newer skip all & go to end, else older (2) then go to next
   274|		MessageBox MB_OK "Due to DSpellCheck plugin's incompatibility issue in version $R0, DSpellCheck.dll will be deleted. Use Plugins Admin to add back (the latest version of) DSpellCheck." /SD IDOK
   275|		Rename "$INSTDIR\plugins\DSpellCheck\DSpellCheck.dll" "$INSTDIR\plugins\disabled\DSpellCheck.dll"
   276|		Delete "$INSTDIR\plugins\DSpellCheck\DSpellCheck.dll"
   277|DSpellCheckTestEnd64:
   278|
   279|	; SpeechPlugin makes npminmin x64 crash. "0.4.0.0" is its 1st version which contains the fix
   280|	IfFileExists "$INSTDIR\plugins\SpeechPlugin\SpeechPlugin.dll" 0 SpeechPluginTestEnd64
   281|		${GetFileVersion} "$INSTDIR\plugins\SpeechPlugin\SpeechPlugin.dll" $R0
   282|		${VersionCompare} $R0 "0.4.0.0" $R1 ;   0: equal to 0.4.0.0   1: $R0 is newer   2: 0.4.0.0 is newer
   283|		StrCmp $R1 "0" +5 0 ; if equal skip all & go to end, else go to next
   284|		StrCmp $R1 "1" +4 0 ; if newer skip all & go to end, else older (2) then go to next
   285|		MessageBox MB_OK "Due to SpeechPlugin plugin's incompatibility issue in version $R0, SpeechPlugin.dll will be deleted. Use Plugins Admin to add back (the latest version of) SpeechPlugin." /SD IDOK
   286|		Rename "$INSTDIR\plugins\SpeechPlugin\SpeechPlugin.dll" "$INSTDIR\plugins\disabled\SpeechPlugin.dll"
   287|		Delete "$INSTDIR\plugins\SpeechPlugin\SpeechPlugin.dll"
   288|SpeechPluginTestEnd64:
   289|
   290|	; XMLTools makes npminmin x64 crash. "3.1.1.12" is its 1st version which contains the fix
   291|	IfFileExists "$INSTDIR\plugins\XMLTools\XMLTools.dll" 0 XMLToolsTestEnd64
   292|		${GetFileVersion} "$INSTDIR\plugins\XMLTools\XMLTools.dll" $R0
   293|		${VersionCompare} $R0 "3.1.1.12" $R1 ;   0: equal to 3.1.1.12   1: $R0 is newer   2: 3.1.1.12 is newer
   294|		StrCmp $R1 "0" +5 0 ; if equal skip all & go to end, else go to next
   295|		StrCmp $R1 "1" +4 0 ; if newer skip all & go to end, else older (2) then go to next
   296|		MessageBox MB_OK "Due to XMLTools plugin's incompatibility issue in version $R0, XMLTools.dll will be deleted. Use Plugins Admin to add back (the latest version of) XMLTools." /SD IDOK
   297|		Rename "$INSTDIR\plugins\XMLTools\XMLTools.dll" "$INSTDIR\plugins\disabled\XMLTools.dll"
   298|		Delete "$INSTDIR\plugins\XMLTools\XMLTools.dll"
   299|XMLToolsTestEnd64:
   300|
   301|	; NppTaskList makes npminmin x64 crash. "2.4" is its 1st version which contains the fix
   302|	IfFileExists "$INSTDIR\plugins\NppTaskList\NppTaskList.dll" 0 NppTaskListTestEnd64
   303|		${GetFileVersion} "$INSTDIR\plugins\NppTaskList\NppTaskList.dll" $R0
   304|		${VersionCompare} $R0 "2.4" $R1 ;   0: equal to 2.4   1: $R0 is newer   2: 2.4 is newer
   305|		StrCmp $R1 "0" +5 0 ; if equal skip all & go to end, else go to next
   306|		StrCmp $R1 "1" +4 0 ; if newer skip all & go to end, else older (2) then go to next
   307|		MessageBox MB_OK "Due to NppTaskList plugin's incompatibility issue in version $R0, NppTaskList.dll will be deleted. Use Plugins Admin to add back (the latest version of) NppTaskList." /SD IDOK
   308|		Rename "$INSTDIR\plugins\NppTaskList\NppTaskList.dll" "$INSTDIR\plugins\disabled\NppTaskList.dll"
   309|		Delete "$INSTDIR\plugins\NppTaskList\NppTaskList.dll"
   310|NppTaskListTestEnd64:
   311|
   312|	; jN makes npminmin x64 crash. "2.2.185.7" is its 1st version which contains the fix
   313|	IfFileExists "$INSTDIR\plugins\jN\jN.dll" 0 jN64
   314|		${GetFileVersion} "$INSTDIR\plugins\jN\jN.dll" $R0
   315|		${VersionCompare} $R0 "2.2.185.7" $R1 ;   0: equal to 2.2.185.7   1: $R0 is newer   2: 2.4 is newer
   316|		StrCmp $R1 "0" +5 0 ; if equal skip all & go to end, else go to next
   317|		StrCmp $R1 "1" +4 0 ; if newer skip all & go to end, else older (2) then go to next
   318|		MessageBox MB_OK "Due to jN plugin's incompatibility issue in version $R0, jN.dll will be deleted. Use Plugins Admin to add back (the latest version of) jN." /SD IDOK
   319|		Rename "$INSTDIR\plugins\jN\jN.dll" "$INSTDIR\plugins\disabled\jN.dll"
   320|		Delete "$INSTDIR\plugins\jN\jN.dll"
   321|jN64:
   322|
   323|
   324|	IfFileExists "$INSTDIR\plugins\NppQCP\NppQCP.dll" 0 NppQCPTestEnd64
   325|		MessageBox MB_OK "Due to NppQCP plugin's crash issue on npminmin x64 binary, NppQCP.dll will be removed." /SD IDOK
   326|		Rename "$INSTDIR\plugins\NppQCP\NppQCP.dll" "$INSTDIR\plugins\disabled\NppQCP.dll"
   327|		Delete "$INSTDIR\plugins\NppQCP\NppQCP.dll"
   328|NppQCPTestEnd64:
   329|
   330|
   331|!else ; 32-bit installer
   332|
   333|		; https://github.com/chcg/NPP_HexEdit/issues/51
   334|		IfFileExists "$INSTDIR\plugins\HexEditor\HexEditor.dll" 0 noDeleteHEPlugin32
   335|			MessageBox MB_YESNO "HexEditor plugin is unstable, we suggest you to remove it.$\nRemove HexEditor plugin?" /SD IDYES IDYES doDeleteHEPlugin32 IDNO noDeleteHEPlugin32 ;IDYES remove
   336|doDeleteHEPlugin32:
   337|                Rename "$INSTDIR\plugins\HexEditor\HexEditor.dll" "$INSTDIR\plugins\disabled\HexEditor.dll"
   338|                Delete "$INSTDIR\plugins\HexEditor\HexEditor.dll"
   339|noDeleteHEPlugin32:
   340|
   341|!endif
   342|
   343|FunctionEnd
   344|
   345|Function removeOldContextMenu
   346|   ; Context Menu Management : removing old version of Context Menu module
   347|	IfFileExists "$INSTDIR\nppcm.dll" 0 +3
   348|		ExecWait '"$winSysDir\regsvr32.exe" /u /s "$INSTDIR\nppcm.dll"'
   349|		Delete "$INSTDIR\nppcm.dll"
   350|        
   351|    IfFileExists "$INSTDIR\NppShell.dll" 0 +3
   352|		ExecWait '"$winSysDir\regsvr32.exe" /u /s "$INSTDIR\NppShell.dll"'
   353|		Delete "$INSTDIR\NppShell.dll"
   354|		
   355|    IfFileExists "$INSTDIR\NppShell_01.dll" 0 +3
   356|		ExecWait '"$winSysDir\regsvr32.exe" /u /s "$INSTDIR\NppShell_01.dll"'
   357|		Delete "$INSTDIR\NppShell_01.dll"
   358|        
   359|    IfFileExists "$INSTDIR\NppShell_02.dll" 0 +3
   360|		ExecWait '"$winSysDir\regsvr32.exe" /u /s "$INSTDIR\NppShell_02.dll"'
   361|		Delete "$INSTDIR\NppShell_02.dll"
   362|		
   363|    IfFileExists "$INSTDIR\NppShell_03.dll" 0 +3
   364|		ExecWait '"$winSysDir\regsvr32.exe" /u /s "$INSTDIR\NppShell_03.dll"'
   365|		Delete "$INSTDIR\NppShell_03.dll"
   366|		
   367|	IfFileExists "$INSTDIR\NppShell_04.dll" 0 +3
   368|		ExecWait '"$winSysDir\regsvr32.exe" /u /s "$INSTDIR\NppShell_04.dll"'
   369|		Delete "$INSTDIR\NppShell_04.dll"
   370|		
   371|	IfFileExists "$INSTDIR\NppShell_05.dll" 0 +3
   372|		ExecWait '"$winSysDir\regsvr32.exe" /u /s "$INSTDIR\NppShell_05.dll"'
   373|		Delete "$INSTDIR\NppShell_05.dll"
   374|FunctionEnd
   375|
   376|Function shortcutLinkManagement
   377|	; remove all the npp shortcuts from current user
   378|	Delete "$DESKTOP\npminmin.lnk"
   379|	Delete "$SMPROGRAMS\npminmin.lnk"
   380|	Delete "$SMPROGRAMS\${APPNAME}\npminmin.lnk"
   381|	Delete "$SMPROGRAMS\${APPNAME}\readme.lnk"
   382|	Delete "$SMPROGRAMS\${APPNAME}\Uninstall.lnk"
   383|	RMDir "$SMPROGRAMS\${APPNAME}"
   384|		
   385|	; detect the right of 
   386|	UserInfo::GetAccountType
   387|	Pop $1
   388|	StrCmp $1 "Admin" 0 +2
   389|	SetShellVarContext all
   390|	
   391|	; set the shortcuts working directory
   392|	; http://nsis.sourceforge.net/Docs/Chapter4.html#createshortcut
   393|	SetOutPath "$INSTDIR\"
   394|	
   395|	; add all the npp shortcuts for all user or current user
   396|	CreateShortCut "$SMPROGRAMS\npminmin.lnk" "$INSTDIR\npminmin.exe"
   397|	${If} $createShortcutChecked == ${BST_CHECKED}
   398|		CreateShortCut "$DESKTOP\npminmin.lnk" "$INSTDIR\npminmin.exe"
   399|	${EndIf}
   400|	
   401|	SetShellVarContext current
   402|FunctionEnd
   403|
   404|
   405|