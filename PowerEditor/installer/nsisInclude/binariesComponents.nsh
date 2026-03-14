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
    18|    18|SectionGroup "Plugins" Plugins
    19|    19|	SetOverwrite on
    20|    20|	
    21|    21|	${MementoSection} "NppExport" NppExport
    22|    22|		Delete "$INSTDIR\plugins\NppExport.dll"
    23|    23|		Delete "$INSTDIR\plugins\NppExport\NppExport.dll"
    24|    24|		Delete "$PLUGIN_INST_PATH\NppExport\NppExport.dll"
    25|    25|		
    26|    26|		SetOutPath "$PLUGIN_INST_PATH\NppExport"
    27|    27|!ifdef ARCH64
    28|    28|		File "..\bin64\plugins\NppExport\NppExport.dll"
    29|    29|!else ifdef ARCHARM64
    30|    30|		File "..\binarm64\plugins\NppExport\NppExport.dll"
    31|    31|!else
    32|    32|		File "..\bin\plugins\NppExport\NppExport.dll"
    33|    33|!endif
    34|    34|	${MementoSectionEnd}
    35|    35|
    36|    36|
    37|    37|	${MementoSection} "Mime Tools" MimeTools
    38|    38|		Delete "$INSTDIR\plugins\mimeTools.dll"
    39|    39|		Delete "$INSTDIR\plugins\mimeTools\mimeTools.dll"
    40|    40|		Delete "$PLUGIN_INST_PATH\mimeTools\mimeTools.dll"
    41|    41|		
    42|    42|		SetOutPath "$PLUGIN_INST_PATH\mimeTools"
    43|    43|!ifdef ARCH64
    44|    44|		File "..\bin64\plugins\mimeTools\mimeTools.dll"
    45|    45|!else ifdef ARCHARM64
    46|    46|		File "..\binarm64\plugins\mimeTools\mimeTools.dll"
    47|    47|!else
    48|    48|		File "..\bin\plugins\mimeTools\mimeTools.dll"
    49|    49|!endif
    50|    50|	${MementoSectionEnd}
    51|    51|	
    52|    52|	${MementoSection} "Converter" Converter
    53|    53|		Delete "$INSTDIR\plugins\NppConverter.dll"
    54|    54|		Delete "$INSTDIR\plugins\NppConverter\NppConverter.dll"
    55|    55|		Delete "$PLUGIN_INST_PATH\NppConverter\NppConverter.dll"
    56|    56|		
    57|    57|		SetOutPath "$PLUGIN_INST_PATH\NppConverter"
    58|    58|!ifdef ARCH64
    59|    59|		File "..\bin64\plugins\NppConverter\NppConverter.dll"
    60|    60|!else ifdef ARCHARM64
    61|    61|		File "..\binarm64\plugins\NppConverter\NppConverter.dll"
    62|    62|!else
    63|    63|		File "..\bin\plugins\NppConverter\NppConverter.dll"
    64|    64|!endif
    65|    65|	${MementoSectionEnd}
    66|    66|
    67|    67|SectionGroupEnd
    68|    68|
    69|    69|; Auto-updater section removed: npminmin ships without WinGup/GUP
    70|    99|
    71|   100|${MementoSection} "Plugins Admin" PluginsAdmin
    72|   101|	SetOverwrite on
    73|   102|	SetOutPath $ALLUSERS_PLUGIN_CONF_PATH
    74|   103|!ifdef ARCH64
    75|   104|	File "..\bin64\plugins\Config\nppPluginList.dll"
    76|   105|!else ifdef ARCHARM64
    77|   106|	File "..\binarm64\plugins\Config\nppPluginList.dll"
    78|   107|!else
    79|   108|	File "..\bin\plugins\Config\nppPluginList.dll"
    80|   109|!endif
    81|   110|${MementoSectionEnd}
    82|   111|
    83|   112|;Uninstall section
    84|   113|SectionGroup un.Plugins
    85|   114|	Section un.NppExport
    86|   115|		Delete "$INSTDIR\plugins\NppExport.dll"
    87|   116|		Delete "$INSTDIR\plugins\NppExport\NppExport.dll"
    88|   117|		RMDir "$INSTDIR\plugins\NppExport"
    89|   118|
    90|   119|		Delete "$PLUGIN_INST_PATH\NppExport\NppExport.dll"
    91|   120|		RMDir "$PLUGIN_INST_PATH\NppExport"
    92|   121|	SectionEnd
    93|   122|	
    94|   123|	Section un.Converter
    95|   124|		Delete "$INSTDIR\plugins\NppConverter.dll"
    96|   125|		Delete "$INSTDIR\plugins\NppConverter\NppConverter.dll"
    97|   126|		RMDir "$INSTDIR\plugins\NppConverter"
    98|   127|		Delete "$PLUGIN_INST_PATH\NppConverter\NppConverter.dll"
    99|   128|		RMDir "$PLUGIN_INST_PATH\NppConverter"
   100|   129|	SectionEnd
   101|   130|	
   102|   131|	Section un.MimeTools
   103|   132|		Delete "$INSTDIR\plugins\mimeTools.dll"
   104|   133|		Delete "$INSTDIR\plugins\mimeTools\mimeTools.dll"
   105|   134|		RMDir "$INSTDIR\plugins\mimeTools"
   106|   135|		Delete "$PLUGIN_INST_PATH\mimeTools\mimeTools.dll"
   107|   136|		RMDir "$PLUGIN_INST_PATH\mimeTools"
   108|   137|	SectionEnd
   109|   138|
   110|   139| 	Section un.DSpellCheck
   111|   140|
   112|   141|		Delete "$INSTDIR\plugins\DSpellCheck.dll"
   113|   142|		Delete "$INSTDIR\plugins\DSpellCheck\DSpellCheck.dll"
   114|   143|		Delete "$PLUGIN_INST_PATH\DSpellCheck\DSpellCheck.dll"
   115|   144|		Delete "$UPDATE_PATH\plugins\Config\DSpellCheck.ini"
   116|   145|		Delete "$ALLUSERS_PLUGIN_CONF_PATH\DSpellCheck.ini"
   117|   146|		Delete "$INSTDIR\plugins\Config\Hunspell\en_US.aff"
   118|   147|		Delete "$USER_PLUGIN_CONF_PATH\Hunspell\en_US.aff"
   119|   148|		Delete "$INSTDIR\plugins\Config\Hunspell\en_US.dic"
   120|   149|		Delete "$USER_PLUGIN_CONF_PATH\Hunspell\en_US.dic"
   121|   150|		RMDir /r "$INSTDIR\plugins\Config"			; Remove Config folder recursively only if empty
   122|   151|		RMDir /r "$ALLUSERS_PLUGIN_CONF_PATH\Config"			; Remove Config folder recursively only if empty
   123|   152|		RMDir "$INSTDIR\plugins\DSpellCheck"
   124|   153|	SectionEnd
   125|   154|
   126|   155|SectionGroupEnd
   127|   156|
   142|   171|
   143|   172|Function .onSelChange
   144|   173|${If} ${SectionIsSelected} ${PluginsAdmin}
   147|   176|${Else}
   149|   178|${EndIf}
   150|   179|FunctionEnd
   151|   180|
   152|   181|Section un.PluginsAdmin
   153|   182|	Delete "$USER_PLUGIN_CONF_PATH\nppPluginList.dll" ; delete 7.6 version's left
   154|   183|	Delete "$ALLUSERS_PLUGIN_CONF_PATH\nppPluginList.dll"
   155|   184|SectionEnd
   156|   185|