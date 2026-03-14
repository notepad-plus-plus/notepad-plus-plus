     1|     1|echo off
     2|     2|rem This file is part of npminmin project
     3|     3|rem Copyright (C)2025 Don HO <don.h@free.fr>
     4|     4|rem
     5|     5|rem This program is free software: you can redistribute it and/or modify
     6|     6|rem it under the terms of the GNU General Public License as published by
     7|     7|rem the Free Software Foundation, either version 3 of the License, or
     8|     8|rem at your option any later version.
     9|     9|rem
    10|    10|rem This program is distributed in the hope that it will be useful,
    11|    11|rem but WITHOUT ANY WARRANTY; without even the implied warranty of
    12|    12|rem MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    13|    13|rem GNU General Public License for more details.
    14|    14|rem
    15|    15|rem You should have received a copy of the GNU General Public License
    16|    16|rem along with this program.  If not, see <https://www.gnu.org/licenses/>.
    17|    17|
    18|    18|echo on
    19|    19|
    20|    20|if %SIGN% == 0 goto NoSign
    21|    21|
    22|    22|REM commands to sign
    23|    23|
    24|    24|set signtoolWin11="C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe"
    25|    25|
    26|    26|set Sign_by_GlobalSignCert=%signtoolWin11% sign /n "NOTEPAD++" /tr http://timestamp.globalsign.com/tsa/r6advanced1 /td SHA256 /fd SHA256
    27|    27|
    28|    28|set DOUBLE_SIGNING=/as
    29|    29|
    30|    30|REM files to be signed
    31|    31|
    32|    32|set nppBinaries=..\bin\npminmin.exe ..\bin64\npminmin.exe ..\binarm64\npminmin.exe
    33|    33|
    34|REM REMOVED: 34|set componentsBinaries=..\bin\plugins\Config\nppPluginList.dll ..\bin64\plugins\Config\nppPluginList.dll ..\binarm64\plugins\Config\nppPluginList.dll ..\bin\updater\GUP.exe ..\bin64\updater\GUP.exe ..\binarm64\updater\GUP.exe  :: npminmin - no auto-updater
    35|    35|
    36|    36|set pluginBinaries=..\bin\plugins\NppExport\NppExport.dll ..\bin64\plugins\NppExport\NppExport.dll ..\binarm64\plugins\NppExport\NppExport.dll ..\bin\plugins\mimeTools\mimeTools.dll ..\bin64\plugins\mimeTools\mimeTools.dll ..\binarm64\plugins\mimeTools\mimeTools.dll ..\bin\plugins\NppConverter\NppConverter.dll ..\bin64\plugins\NppConverter\NppConverter.dll ..\binarm64\plugins\NppConverter\NppConverter.dll
    37|    37|
    38|    38|
    39|    39|REM macro is used to sign NppShell.dll & NppShell.msix with hash algorithm SHA256, due to signtool.exe bug:
    40|    40|REM "error 0x8007000B: The signature hash method specified (SHA512) must match the hash method used in the app package block map (SHA256)."
    41|    41|REM "The hashAlgorithm specified in the /fd parameter is incorrect. Rerun SignTool using hashAlgorithm that matches the app package block map (used to create the app package)"
    42|    42|REM Note that Publisher in Packaging/AppxManifest.xml‎ should match with the Subject of certificate.
    43|    43|REM https://learn.microsoft.com/en-us/windows/msix/package/signing-known-issues
    44|    44|set nppShellBinaries=..\bin\NppShell.x86.dll  ..\bin64\NppShell.msix  ..\bin64\NppShell.x64.dll ..\binarm64\NppShell.msix ..\binarm64\NppShell.arm64.dll
    45|    45|
    46|    46|
    47|    47|%Sign_by_GlobalSignCert% %nppBinaries% %componentsBinaries% %pluginBinaries% %nppShellBinaries%
    48|    48|If ErrorLevel 1 goto End
    49|    49|
    50|    50|
    51|    51|
    52|    52|:NoSign
    53|    53|
    54|    54|
    55|    55|rmdir /S /Q .\build
    56|    56|mkdir .\build
    57|    57|
    58|    58|rem npminmin minimalist package
    59|    59|rmdir /S /Q .\minimalist
    60|    60|mkdir .\minimalist
    61|    61|mkdir .\minimalist\userDefineLangs
    62|    62|mkdir .\minimalist\themes
    63|    63|
    64|    64|copy /Y ..\..\LICENSE .\minimalist\license.txt
    65|    65|If ErrorLevel 1 goto End
    66|    66|copy /Y ..\bin\readme.txt .\minimalist\
    67|    67|If ErrorLevel 1 goto End
    68|    68|copy /Y ..\bin\change.log .\minimalist\
    69|    69|If ErrorLevel 1 goto End
    70|    70|copy /Y "..\bin\userDefineLangs\markdown._preinstalled.udl.xml" .\minimalist\userDefineLangs\
    71|    71|If ErrorLevel 1 goto End
    72|    72|copy /Y ..\src\langs.model.xml .\minimalist\
    73|    73|If ErrorLevel 1 goto End
    74|    74|copy /Y ..\src\stylers.model.xml .\minimalist\
    75|    75|If ErrorLevel 1 goto End
    76|    76|copy /Y ..\src\contextMenu.xml .\minimalist\
    77|    77|If ErrorLevel 1 goto End
    78|    78|copy /Y ..\src\tabContextMenu_example.xml .\minimalist\
    79|    79|If ErrorLevel 1 goto End
    80|    80|copy /Y ..\src\toolbarButtonsConf_example.xml .\minimalist\
    81|    81|If ErrorLevel 1 goto End
    82|    82|copy /Y ..\src\shortcuts.xml .\minimalist\
    83|    83|If ErrorLevel 1 goto End
    84|    84|copy /Y ..\bin\doLocalConf.xml .\minimalist\
    85|    85|If ErrorLevel 1 goto End
    86|    86|copy /Y ..\bin\nppLogNulContentCorruptionIssue.xml .\minimalist\
    87|    87|If ErrorLevel 1 goto End
    88|    88|copy /Y ..\bin\"npminmin.exe" .\minimalist\
    89|    89|If ErrorLevel 1 goto End
    90|    90|copy /Y ".\themes\DarkModeDefault.xml" .\minimalist\themes\
    91|    91|If ErrorLevel 1 goto End
    92|    92|
    93|    93|
    94|    94|rmdir /S /Q .\minimalist64
    95|    95|mkdir .\minimalist64
    96|    96|mkdir .\minimalist64\userDefineLangs
    97|    97|mkdir .\minimalist64\themes
    98|    98|
    99|    99|copy /Y ..\..\LICENSE .\minimalist64\license.txt
   100|   100|If ErrorLevel 1 goto End
   101|   101|copy /Y ..\bin\readme.txt .\minimalist64\
   102|   102|If ErrorLevel 1 goto End
   103|   103|copy /Y ..\bin\change.log .\minimalist64\
   104|   104|If ErrorLevel 1 goto End
   105|   105|copy /Y "..\bin\userDefineLangs\markdown._preinstalled.udl.xml" .\minimalist64\userDefineLangs\
   106|   106|If ErrorLevel 1 goto End
   107|   107|copy /Y ..\src\langs.model.xml .\minimalist64\
   108|   108|If ErrorLevel 1 goto End
   109|   109|copy /Y ..\src\stylers.model.xml .\minimalist64\
   110|   110|If ErrorLevel 1 goto End
   111|   111|copy /Y ..\src\contextMenu.xml .\minimalist64\
   112|   112|If ErrorLevel 1 goto End
   113|   113|copy /Y ..\src\tabContextMenu_example.xml .\minimalist64\
   114|   114|If ErrorLevel 1 goto End
   115|   115|copy /Y ..\src\toolbarButtonsConf_example.xml .\minimalist64\
   116|   116|If ErrorLevel 1 goto End
   117|   117|copy /Y ..\src\shortcuts.xml .\minimalist64\
   118|   118|If ErrorLevel 1 goto End
   119|   119|copy /Y ..\bin\doLocalConf.xml .\minimalist64\
   120|   120|If ErrorLevel 1 goto End
   121|   121|copy /Y ..\bin\nppLogNulContentCorruptionIssue.xml .\minimalist64\
   122|   122|If ErrorLevel 1 goto End
   123|   123|copy /Y ..\bin64\"npminmin.exe" .\minimalist64\
   124|   124|If ErrorLevel 1 goto End
   125|   125|copy /Y ".\themes\DarkModeDefault.xml" .\minimalist64\themes\
   126|   126|If ErrorLevel 1 goto End
   127|   127|
   128|   128|
   129|   129|rmdir /S /Q .\minimalistArm64
   130|   130|mkdir .\minimalistArm64
   131|   131|mkdir .\minimalistArm64\userDefineLangs
   132|   132|mkdir .\minimalistArm64\themes
   133|   133|
   134|   134|copy /Y ..\..\LICENSE .\minimalistArm64\license.txt
   135|   135|If ErrorLevel 1 goto End
   136|   136|copy /Y ..\bin\readme.txt .\minimalistArm64\
   137|   137|If ErrorLevel 1 goto End
   138|   138|copy /Y ..\bin\change.log .\minimalistArm64\
   139|   139|If ErrorLevel 1 goto End
   140|   140|copy /Y "..\bin\userDefineLangs\markdown._preinstalled.udl.xml" .\minimalistArm64\userDefineLangs\
   141|   141|If ErrorLevel 1 goto End
   142|   142|copy /Y ..\src\langs.model.xml .\minimalistArm64\
   143|   143|If ErrorLevel 1 goto End
   144|   144|copy /Y ..\src\stylers.model.xml .\minimalistArm64\
   145|   145|If ErrorLevel 1 goto End
   146|   146|copy /Y ..\src\contextMenu.xml .\minimalistArm64\
   147|   147|If ErrorLevel 1 goto End
   148|   148|copy /Y ..\src\tabContextMenu_example.xml .\minimalistArm64\
   149|   149|If ErrorLevel 1 goto End
   150|   150|copy /Y ..\src\toolbarButtonsConf_example.xml .\minimalistArm64\
   151|   151|If ErrorLevel 1 goto End
   152|   152|copy /Y ..\src\shortcuts.xml .\minimalistArm64\
   153|   153|If ErrorLevel 1 goto End
   154|   154|copy /Y ..\bin\doLocalConf.xml .\minimalistArm64\
   155|   155|If ErrorLevel 1 goto End
   156|   156|copy /Y ..\bin\nppLogNulContentCorruptionIssue.xml .\minimalistArm64\
   157|   157|If ErrorLevel 1 goto End
   158|   158|copy /Y ..\binarm64\"npminmin.exe" .\minimalistArm64\
   159|   159|If ErrorLevel 1 goto End
   160|   160|copy /Y ".\themes\DarkModeDefault.xml" .\minimalistArm64\themes\
   161|   161|If ErrorLevel 1 goto End
   162|   162|
   163|   163|
   164|   164|rem Remove old built npminmin 32-bit package
   165|   165|rmdir /S /Q .\zipped.package.release
   166|   166|
   167|   167|rem Re-build npminmin 32-bit package folders
   168|   168|mkdir .\zipped.package.release
   169|REM REMOVED: 169|mkdir .\zipped.package.release\updater  :: npminmin - no auto-updater
   170|   170|mkdir .\zipped.package.release\localization
   171|   171|mkdir .\zipped.package.release\themes
   172|   172|mkdir .\zipped.package.release\autoCompletion
   173|   173|mkdir .\zipped.package.release\functionList
   174|   174|mkdir .\zipped.package.release\userDefineLangs
   175|   175|mkdir .\zipped.package.release\plugins
   176|   176|mkdir .\zipped.package.release\plugins\NppExport
   177|   177|mkdir .\zipped.package.release\plugins\mimeTools
   178|   178|mkdir .\zipped.package.release\plugins\NppConverter
   179|   179|mkdir .\zipped.package.release\plugins\Config
   180|   180|mkdir .\zipped.package.release\plugins\doc
   181|   181|
   182|   182|
   183|   183|rem Remove old built npminmin 64-bit package
   184|   184|rmdir /S /Q .\zipped.package.release64
   185|   185|
   186|   186|rem Re-build npminmin 64-bit package folders
   187|   187|mkdir .\zipped.package.release64
   188|REM REMOVED: 188|mkdir .\zipped.package.release64\updater  :: npminmin - no auto-updater
   189|   189|mkdir .\zipped.package.release64\localization
   190|   190|mkdir .\zipped.package.release64\themes
   191|   191|mkdir .\zipped.package.release64\autoCompletion
   192|   192|mkdir .\zipped.package.release64\functionList
   193|   193|mkdir .\zipped.package.release64\userDefineLangs
   194|   194|mkdir .\zipped.package.release64\plugins
   195|   195|mkdir .\zipped.package.release64\plugins\NppExport
   196|   196|mkdir .\zipped.package.release64\plugins\mimeTools
   197|   197|mkdir .\zipped.package.release64\plugins\NppConverter
   198|   198|mkdir .\zipped.package.release64\plugins\Config
   199|   199|mkdir .\zipped.package.release64\plugins\doc
   200|   200|
   201|   201|
   202|   202|rem Remove old built npminmin ARM64-bit package
   203|   203|rmdir /S /Q .\zipped.package.releaseArm64
   204|   204|
   205|   205|rem Re-build npminmin ARM64-bit package folders
   206|   206|mkdir .\zipped.package.releaseArm64
   207|REM REMOVED: 207|mkdir .\zipped.package.releaseArm64\updater  :: npminmin - no auto-updater
   208|   208|mkdir .\zipped.package.releaseArm64\localization
   209|   209|mkdir .\zipped.package.releaseArm64\themes
   210|   210|mkdir .\zipped.package.releaseArm64\autoCompletion
   211|   211|mkdir .\zipped.package.releaseArm64\functionList
   212|   212|mkdir .\zipped.package.releaseArm64\userDefineLangs
   213|   213|mkdir .\zipped.package.releaseArm64\plugins
   214|   214|mkdir .\zipped.package.releaseArm64\plugins\NppExport
   215|   215|mkdir .\zipped.package.releaseArm64\plugins\mimeTools
   216|   216|mkdir .\zipped.package.releaseArm64\plugins\NppConverter
   217|   217|mkdir .\zipped.package.releaseArm64\plugins\Config
   218|   218|mkdir .\zipped.package.releaseArm64\plugins\doc
   219|   219|
   220|   220|
   221|   221|rem Basic: Copy needed files into npminmin 32-bit package folders
   222|   222|copy /Y ..\..\LICENSE .\zipped.package.release\license.txt
   223|   223|If ErrorLevel 1 goto End
   224|   224|copy /Y ..\bin\readme.txt .\zipped.package.release\
   225|   225|If ErrorLevel 1 goto End
   226|   226|copy /Y ..\bin\change.log .\zipped.package.release\
   227|   227|If ErrorLevel 1 goto End
   228|   228|copy /Y ..\src\langs.model.xml .\zipped.package.release\
   229|   229|If ErrorLevel 1 goto End
   230|   230|copy /Y ..\src\stylers.model.xml .\zipped.package.release\
   231|   231|If ErrorLevel 1 goto End
   232|   232|copy /Y ..\src\contextMenu.xml .\zipped.package.release\
   233|   233|If ErrorLevel 1 goto End
   234|   234|copy /Y ..\src\tabContextMenu_example.xml .\zipped.package.release\
   235|   235|If ErrorLevel 1 goto End
   236|   236|copy /Y ..\src\toolbarButtonsConf_example.xml .\zipped.package.release\
   237|   237|If ErrorLevel 1 goto End
   238|   238|copy /Y ..\src\shortcuts.xml .\zipped.package.release\
   239|   239|If ErrorLevel 1 goto End
   240|   240|copy /Y ..\bin\doLocalConf.xml .\zipped.package.release\
   241|   241|If ErrorLevel 1 goto End
   242|   242|copy /Y ..\bin\nppLogNulContentCorruptionIssue.xml .\zipped.package.release\
   243|   243|If ErrorLevel 1 goto End
   244|   244|copy /Y ..\bin\"npminmin.exe" .\zipped.package.release\
   245|   245|If ErrorLevel 1 goto End
   246|   246|
   247|   247|
   248|   248|
   249|   249|rem Basic Copy needed files into npminmin 64-bit package folders
   250|   250|copy /Y ..\..\LICENSE .\zipped.package.release64\license.txt
   251|   251|If ErrorLevel 1 goto End
   252|   252|copy /Y ..\bin\readme.txt .\zipped.package.release64\
   253|   253|If ErrorLevel 1 goto End
   254|   254|copy /Y ..\bin\change.log .\zipped.package.release64\
   255|   255|If ErrorLevel 1 goto End
   256|   256|copy /Y ..\src\langs.model.xml .\zipped.package.release64\
   257|   257|If ErrorLevel 1 goto End
   258|   258|copy /Y ..\src\stylers.model.xml .\zipped.package.release64\
   259|   259|If ErrorLevel 1 goto End
   260|   260|copy /Y ..\src\contextMenu.xml .\zipped.package.release64\
   261|   261|If ErrorLevel 1 goto End
   262|   262|copy /Y ..\src\tabContextMenu_example.xml .\zipped.package.release64\
   263|   263|If ErrorLevel 1 goto End
   264|   264|copy /Y ..\src\toolbarButtonsConf_example.xml .\zipped.package.release64\
   265|   265|If ErrorLevel 1 goto End
   266|   266|copy /Y ..\src\shortcuts.xml .\zipped.package.release64\
   267|   267|If ErrorLevel 1 goto End
   268|   268|copy /Y ..\bin\doLocalConf.xml .\zipped.package.release64\
   269|   269|If ErrorLevel 1 goto End
   270|   270|copy /Y ..\bin\nppLogNulContentCorruptionIssue.xml .\zipped.package.release64\
   271|   271|If ErrorLevel 1 goto End
   272|   272|copy /Y ..\bin64\"npminmin.exe" .\zipped.package.release64\
   273|   273|If ErrorLevel 1 goto End
   274|   274|
   275|   275|
   276|   276|rem Basic Copy needed files into npminmin ARM64 package folders
   277|   277|copy /Y ..\..\LICENSE .\zipped.package.releaseArm64\license.txt
   278|   278|If ErrorLevel 1 goto End
   279|   279|copy /Y ..\bin\readme.txt .\zipped.package.releaseArm64\
   280|   280|If ErrorLevel 1 goto End
   281|   281|copy /Y ..\bin\change.log .\zipped.package.releaseArm64\
   282|   282|If ErrorLevel 1 goto End
   283|   283|copy /Y ..\src\langs.model.xml .\zipped.package.releaseArm64\
   284|   284|If ErrorLevel 1 goto End
   285|   285|copy /Y ..\src\stylers.model.xml .\zipped.package.releaseArm64\
   286|   286|If ErrorLevel 1 goto End
   287|   287|copy /Y ..\src\contextMenu.xml .\zipped.package.releaseArm64\
   288|   288|If ErrorLevel 1 goto End
   289|   289|copy /Y ..\src\tabContextMenu_example.xml .\zipped.package.releaseArm64\
   290|   290|If ErrorLevel 1 goto End
   291|   291|copy /Y ..\src\toolbarButtonsConf_example.xml .\zipped.package.releaseArm64\
   292|   292|If ErrorLevel 1 goto End
   293|   293|copy /Y ..\src\shortcuts.xml .\zipped.package.releaseArm64\
   294|   294|If ErrorLevel 1 goto End
   295|   295|copy /Y ..\bin\doLocalConf.xml .\zipped.package.releaseArm64\
   296|   296|If ErrorLevel 1 goto End
   297|   297|copy /Y ..\bin\nppLogNulContentCorruptionIssue.xml .\zipped.package.releaseArm64\
   298|   298|If ErrorLevel 1 goto End
   299|   299|copy /Y ..\binarm64\"npminmin.exe" .\zipped.package.releaseArm64\
   300|   300|If ErrorLevel 1 goto End
   301|   301|
   302|   302|
   303|   303|rem Plugins: Copy needed files into npminmin 32-bit package folders
   304|   304|copy /Y "..\bin\plugins\NppExport\NppExport.dll" .\zipped.package.release\plugins\NppExport\
   305|   305|If ErrorLevel 1 goto End
   306|   306|copy /Y "..\bin\plugins\mimeTools\mimeTools.dll" .\zipped.package.release\plugins\mimeTools\
   307|   307|If ErrorLevel 1 goto End
   308|   308|copy /Y "..\bin\plugins\NppConverter\NppConverter.dll" .\zipped.package.release\plugins\NppConverter\
   309|   309|If ErrorLevel 1 goto End
   310|   310|
   311|   311|rem Plugins: Copy needed files into npminmin 64-bit package folders
   312|   312|copy /Y "..\bin64\plugins\NppExport\NppExport.dll" .\zipped.package.release64\plugins\NppExport\
   313|   313|If ErrorLevel 1 goto End
   314|   314|copy /Y "..\bin64\plugins\mimeTools\mimeTools.dll" .\zipped.package.release64\plugins\mimeTools\
   315|   315|If ErrorLevel 1 goto End
   316|   316|copy /Y "..\bin64\plugins\NppConverter\NppConverter.dll" .\zipped.package.release64\plugins\NppConverter\
   317|   317|If ErrorLevel 1 goto End
   318|   318|
   319|   319|rem Plugins: Copy needed files into npminmin 64-bit package folders
   320|   320|copy /Y "..\binarm64\plugins\NppExport\NppExport.dll" .\zipped.package.releaseArm64\plugins\NppExport\
   321|   321|If ErrorLevel 1 goto End
   322|   322|copy /Y "..\binarm64\plugins\mimeTools\mimeTools.dll" .\zipped.package.releaseArm64\plugins\mimeTools\
   323|   323|If ErrorLevel 1 goto End
   324|   324|copy /Y "..\binarm64\plugins\NppConverter\NppConverter.dll" .\zipped.package.releaseArm64\plugins\NppConverter\
   325|   325|If ErrorLevel 1 goto End
   326|   326|
   327|   327|
   328|   328|rem localizations: Copy all files into npminmin 32-bit/64-bit package folders
   329|   329|copy /Y ".\nativeLang\*.xml" .\zipped.package.release\localization\
   330|   330|If ErrorLevel 1 goto End
   331|   331|copy /Y ".\nativeLang\*.xml" .\zipped.package.release64\localization\
   332|   332|If ErrorLevel 1 goto End
   333|   333|copy /Y ".\nativeLang\*.xml" .\zipped.package.releaseArm64\localization\
   334|   334|If ErrorLevel 1 goto End
   335|   335|
   336|   336|rem files API: Copy all files into npminmin 32-bit/64-bit package folders
   337|   337|copy /Y ".\APIs\*.xml" .\zipped.package.release\autoCompletion\
   338|   338|If ErrorLevel 1 goto End
   339|   339|copy /Y ".\APIs\*.xml" .\zipped.package.release64\autoCompletion\
   340|   340|If ErrorLevel 1 goto End
   341|   341|copy /Y ".\APIs\*.xml" .\zipped.package.releaseArm64\autoCompletion\
   342|   342|If ErrorLevel 1 goto End
   343|   343|
   344|   344|rem FunctionList files: Copy all files into npminmin 32-bit/64-bit package folders
   345|   345|copy /Y ".\functionList\*.xml" .\zipped.package.release\functionList\
   346|   346|If ErrorLevel 1 goto End
   347|   347|copy /Y ".\functionList\*.xml" .\zipped.package.release64\functionList\
   348|   348|If ErrorLevel 1 goto End
   349|   349|copy /Y ".\functionList\*.xml" .\zipped.package.releaseArm64\functionList\
   350|   350|If ErrorLevel 1 goto End
   351|   351|
   352|   352|rem Markdown as UserDefineLanguge: Markdown syntax highlighter into npminmin 32-bit/64-bit package folders
   353|   353|copy /Y "..\bin\userDefineLangs\markdown._preinstalled.udl.xml" .\zipped.package.release\userDefineLangs\
   354|   354|If ErrorLevel 1 goto End
   355|   355|copy /Y "..\bin\userDefineLangs\markdown._preinstalled.udl.xml" .\zipped.package.release64\userDefineLangs\
   356|   356|If ErrorLevel 1 goto End
   357|   357|copy /Y "..\bin\userDefineLangs\markdown._preinstalled.udl.xml" .\zipped.package.releaseArm64\userDefineLangs\
   358|   358|If ErrorLevel 1 goto End
   359|   359|copy /Y "..\bin\userDefineLangs\markdown._preinstalled_DM.udl.xml" .\zipped.package.release\userDefineLangs\
   360|   360|If ErrorLevel 1 goto End
   361|   361|copy /Y "..\bin\userDefineLangs\markdown._preinstalled_DM.udl.xml" .\zipped.package.release64\userDefineLangs\
   362|   362|If ErrorLevel 1 goto End
   363|   363|copy /Y "..\bin\userDefineLangs\markdown._preinstalled_DM.udl.xml" .\zipped.package.releaseArm64\userDefineLangs\
   364|   364|If ErrorLevel 1 goto End
   365|   365|
   366|   366|rem theme: Copy all files into npminmin 32-bit/64-bit package folders
   367|   367|copy /Y ".\themes\*.xml" .\zipped.package.release\themes\
   368|   368|If ErrorLevel 1 goto End
   369|   369|copy /Y ".\themes\*.xml" .\zipped.package.release64\themes\
   370|   370|If ErrorLevel 1 goto End
   371|   371|copy /Y ".\themes\*.xml" .\zipped.package.releaseArm64\themes\
   372|   372|If ErrorLevel 1 goto End
   373|   373|
   374|   374|rem Plugins Admin
   375|   375|If ErrorLevel 1 goto End
   376|   376|copy /Y ..\bin\plugins\Config\nppPluginList.dll .\zipped.package.release\plugins\Config\
   377|   377|If ErrorLevel 1 goto End
   378|REM REMOVED: 378|copy /Y ..\bin\updater\GUP.exe .\zipped.package.release\updater\  :: npminmin - no auto-updater
   379|   379|If ErrorLevel 1 goto End
   380|REM REMOVED: 380|copy /Y ..\bin\updater\gup.xml .\zipped.package.release\updater\  :: npminmin - no auto-updater
   381|   381|If ErrorLevel 1 goto End
   382|REM REMOVED: 382|copy /Y ..\bin\updater\LICENSE .\zipped.package.release\updater\  :: npminmin - no auto-updater
   383|   383|If ErrorLevel 1 goto End
   384|REM REMOVED: 384|copy /Y ..\bin\updater\README.md .\zipped.package.release\updater\  :: npminmin - no auto-updater
   385|   385|If ErrorLevel 1 goto End
   386|REM REMOVED: 386|copy /Y ..\bin\updater\updater.ico .\zipped.package.release\updater\  :: npminmin - no auto-updater
   387|   387|If ErrorLevel 1 goto End
   388|   388|
   389|   389|
   390|   390|If ErrorLevel 1 goto End
   391|   391|copy /Y ..\bin64\plugins\Config\nppPluginList.dll .\zipped.package.release64\plugins\Config\
   392|   392|If ErrorLevel 1 goto End
   393|REM REMOVED: 393|copy /Y ..\bin64\updater\GUP.exe .\zipped.package.release64\updater\  :: npminmin - no auto-updater
   394|   394|If ErrorLevel 1 goto End
   395|REM REMOVED: 395|copy /Y ..\bin64\updater\gup.xml .\zipped.package.release64\updater\  :: npminmin - no auto-updater
   396|   396|If ErrorLevel 1 goto End
   397|REM REMOVED: 397|copy /Y ..\bin64\updater\LICENSE .\zipped.package.release64\updater\  :: npminmin - no auto-updater
   398|   398|If ErrorLevel 1 goto End
   399|REM REMOVED: 399|copy /Y ..\bin64\updater\README.md .\zipped.package.release64\updater\  :: npminmin - no auto-updater
   400|   400|If ErrorLevel 1 goto End
   401|REM REMOVED: 401|copy /Y ..\bin64\updater\updater.ico .\zipped.package.release64\updater\  :: npminmin - no auto-updater
   402|   402|If ErrorLevel 1 goto End
   403|   403|
   404|   404|
   405|   405|If ErrorLevel 1 goto End
   406|   406|copy /Y ..\binarm64\plugins\Config\nppPluginList.dll .\zipped.package.releaseArm64\plugins\Config\
   407|   407|If ErrorLevel 1 goto End
   408|REM REMOVED: 408|copy /Y ..\binarm64\updater\GUP.exe .\zipped.package.releaseArm64\updater\  :: npminmin - no auto-updater
   409|   409|If ErrorLevel 1 goto End
   410|REM REMOVED: 410|copy /Y ..\binarm64\updater\gup.xml .\zipped.package.releaseArm64\updater\  :: npminmin - no auto-updater
   411|   411|If ErrorLevel 1 goto End
   412|REM REMOVED: 412|copy /Y ..\binarm64\updater\LICENSE .\zipped.package.releaseArm64\updater\  :: npminmin - no auto-updater
   413|   413|If ErrorLevel 1 goto End
   414|REM REMOVED: 414|copy /Y ..\binarm64\updater\README.md .\zipped.package.releaseArm64\updater\  :: npminmin - no auto-updater
   415|   415|If ErrorLevel 1 goto End
   416|REM REMOVED: 416|copy /Y ..\binarm64\updater\updater.ico .\zipped.package.releaseArm64\updater\  :: npminmin - no auto-updater
   417|   417|If ErrorLevel 1 goto End
   418|   418|
   419|   419|
   420|   420|
   421|   421|"C:\Program Files\7-Zip\7z.exe" a -r .\build\npminmin.portable.minimalist.7z .\minimalist\*
   422|   422|If ErrorLevel 1 goto End
   423|   423|"C:\Program Files\7-Zip\7z.exe" a -r .\build\npminmin.portable.minimalist.x64.7z .\minimalist64\*
   424|   424|If ErrorLevel 1 goto End
   425|   425|"C:\Program Files\7-Zip\7z.exe" a -r .\build\npminmin.portable.minimalist.arm64.7z .\minimalistArm64\*
   426|   426|If ErrorLevel 1 goto End
   427|   427|
   428|   428|
   429|   429|"C:\Program Files\7-Zip\7z.exe" a -tzip -r .\build\npminmin.portable.zip .\zipped.package.release\*
   430|   430|If ErrorLevel 1 goto End
   431|   431|"C:\Program Files\7-Zip\7z.exe" a -r .\build\npminmin.portable.7z .\zipped.package.release\*
   432|   432|If ErrorLevel 1 goto End
   433|   433|rem IF EXIST "%PROGRAMFILES(X86)%" ("%PROGRAMFILES(x86)%\NSIS\Unicode\makensis.exe" nppSetup.nsi) ELSE ("%PROGRAMFILES%\NSIS\Unicode\makensis.exe" nppSetup.nsi)
   434|   434|IF EXIST "%PROGRAMFILES(X86)%" ("%PROGRAMFILES(x86)%\NSIS\makensis.exe" nppSetup.nsi) ELSE ("%PROGRAMFILES%\NSIS\makensis.exe" nppSetup.nsi)
   435|   435|IF EXIST "%PROGRAMFILES(X86)%" ("%PROGRAMFILES(x86)%\NSIS\makensis.exe" -DARCH64 nppSetup.nsi) ELSE ("%PROGRAMFILES%\NSIS\makensis.exe" -DARCH64 nppSetup.nsi)
   436|   436|IF EXIST "%PROGRAMFILES(X86)%" ("%PROGRAMFILES(x86)%\NSIS\makensis.exe" -DARCHARM64 nppSetup.nsi) ELSE ("%PROGRAMFILES%\NSIS\makensis.exe" -DARCHARM64 nppSetup.nsi)
   437|   437|
   438|   438|rem Remove old build
   439|   439|rmdir /S /Q .\zipped.package.release
   440|   440|
   441|   441|rem 
   442|   442|"C:\Program Files\7-Zip\7z.exe" a -tzip -r .\build\npminmin.portable.x64.zip .\zipped.package.release64\*
   443|   443|If ErrorLevel 1 goto End
   444|   444|
   445|   445|"C:\Program Files\7-Zip\7z.exe" a -r .\build\npminmin.portable.x64.7z .\zipped.package.release64\*
   446|   446|If ErrorLevel 1 goto End
   447|   447|
   448|   448|rem 
   449|   449|"C:\Program Files\7-Zip\7z.exe" a -tzip -r .\build\npminmin.portable.arm64.zip .\zipped.package.releaseArm64\*
   450|   450|If ErrorLevel 1 goto End
   451|   451|
   452|   452|"C:\Program Files\7-Zip\7z.exe" a -r .\build\npminmin.portable.arm64.7z .\zipped.package.releaseArm64\*
   453|   453|If ErrorLevel 1 goto End
   454|   454|
   455|   455|rem set var locally in this batch file
   456|   456|setlocal 
   457|   457|
   458|   458|cd build
   459|   459|
   460|   460|:: Get npminmin.6.9.Installer.exe in %nppInstallerVar%
   461|   461|for %%f in (npminmin.*.Installer.exe) do set "nppInstallerVar=%%f"
   462|   462|
   463|   463|
   464|   464|rem get the version string "6.9" in %VERSION%
   465|   465|set "VERSION=%nppInstallerVar:npminmin.=%"
   466|   466|rem replace "npminmin." with nothing in "npminmin.6.9.Installer.exe" - now VERSION is "6.9.Installer.exe"
   467|   467|
   468|   468|rem echo %VERSION%
   469|   469|
   470|   470|set "VERSION=%VERSION:.Installer.exe=%"
   471|   471|rem replace ".Installer.exe" with nothing in "6.9.Installer.exe" - now VERSION is "6.9"
   472|   472|
   473|   473|rem echo %VERSION%
   474|   474|
   475|   475|cd ..\msi\
   476|   476|dotnet build -c release -p:OutputPath=..\build\ -p:DefineConstants=Version=%VERSION%
   477|   477|If ErrorLevel 1 goto End
   478|   478|
   479|   479|cd ..\build\
   480|   480|
   481|   481|
   482|   482|ren npminmin.portable.zip npminmin.%VERSION%.portable.zip
   483|   483|If ErrorLevel 1 goto End
   484|   484|
   485|   485|ren npminmin.portable.x64.zip npminmin.%VERSION%.portable.x64.zip
   486|   486|If ErrorLevel 1 goto End
   487|   487|
   488|   488|ren npminmin.portable.arm64.zip npminmin.%VERSION%.portable.arm64.zip
   489|   489|If ErrorLevel 1 goto End
   490|   490|
   491|   491|ren npminmin.portable.7z npminmin.%VERSION%.portable.7z
   492|   492|If ErrorLevel 1 goto End
   493|   493|
   494|   494|ren npminmin.portable.x64.7z npminmin.%VERSION%.portable.x64.7z
   495|   495|If ErrorLevel 1 goto End
   496|   496|
   497|   497|ren npminmin.portable.arm64.7z npminmin.%VERSION%.portable.arm64.7z
   498|   498|If ErrorLevel 1 goto End
   499|   499|
   500|   500|ren npminmin.portable.minimalist.7z npminmin.%VERSION%.portable.minimalist.7z
   501|