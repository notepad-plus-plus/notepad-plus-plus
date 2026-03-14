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
    17|
    18|; Define the application name
    19|!define APPNAME "npminmin"
    20|
    21|; ------------------------------------------------------------------------
    22|; Get npminmin version from the npminmin binary
    23|
    24|!ifdef ARCH64
    25|	!getdllversion "..\bin64\npminmin.exe" nppVer_
    26|!else ifdef ARCHARM64
    27|	!getdllversion "..\binarm64\npminmin.exe" nppVer_
    28|!else
    29|	!getdllversion "..\bin\npminmin.exe" nppVer_
    30|!endif
    31|
    32|!define APPVERSION		${nppVer_1}.${nppVer_2}		; 7.5
    33|!define VERSION_MAJOR		${nppVer_1}			; 7
    34|!define VERSION_MINOR		${nppVer_2}			; 5
    35|
    36|!if ${nppVer_3} != 0
    37|	!undef APPVERSION
    38|	!define APPVERSION	${nppVer_1}.${nppVer_2}.${nppVer_3}	; 7.5.1
    39|
    40|	!undef VERSION_MINOR
    41|	!define VERSION_MINOR	${nppVer_2}${nppVer_3}			; 51
    42|!endif
    43|
    44|!if ${nppVer_4} != 0
    45|	!undef APPVERSION
    46|	!define APPVERSION	${nppVer_1}.${nppVer_2}.${nppVer_3}.${nppVer_4}	; 7.5.1.3
    47|
    48|	!undef VERSION_MINOR
    49|	!define VERSION_MINOR	${nppVer_2}${nppVer_3}${nppVer_4}		; 513
    50|!endif
    51|
    52|; ------------------------------------------------------------------------
    53|
    54|!define APPNAMEANDVERSION	"${APPNAME} v${APPVERSION}"
    55|!define CompanyName		"Don HO don.h@free.fr"
    56|!define Description		"npminmin : a secure, portable source code editor"
    57|!define Version		"${nppVer_1}.${nppVer_2}.${nppVer_3}.${nppVer_4}"
    58|!define ProdVer		"${VERSION_MAJOR}.${VERSION_MINOR}"
    59|!define LegalCopyright		"npminmin contributors"
    60|
    61|!define APPWEBSITE "https://github.com/ridermw/np-minus-minus"
    62|
    63|!define UNINSTALL_REG_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
    64|!define MEMENTO_REGISTRY_ROOT HKLM
    65|!define MEMENTO_REGISTRY_KEY ${UNINSTALL_REG_KEY}
    66|
    67|; Main Install settings
    68|Name "${APPNAMEANDVERSION}"
    69|
    70|InstallDir "$PROGRAMFILES\${APPNAME}"
    71|
    72|InstallDirRegKey HKLM "Software\${APPNAME}" ""
    73|