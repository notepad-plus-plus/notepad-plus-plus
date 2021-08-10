; This file is part of Notepad++ project
; Copyright (C)2021 Don HO <don.h@free.fr>
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; at your option any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.


; Define the application name
!define APPNAME "Notepad++"

; ------------------------------------------------------------------------
; Get Notepad++ version from the notepad++ binary

!ifdef ARCH64
	!getdllversion "..\bin64\notepad++.exe" nppVer_
!else ifdef ARCHARM64
	!getdllversion "..\binarm64\notepad++.exe" nppVer_
!else
	!getdllversion "..\bin\notepad++.exe" nppVer_
!endif

!define APPVERSION		${nppVer_1}.${nppVer_2}		; 7.5
!define VERSION_MAJOR		${nppVer_1}			; 7
!define VERSION_MINOR		${nppVer_2}			; 5

!if ${nppVer_3} != 0
	!undef APPVERSION
	!define APPVERSION	${nppVer_1}.${nppVer_2}.${nppVer_3}	; 7.5.1

	!undef VERSION_MINOR
	!define VERSION_MINOR	${nppVer_2}${nppVer_3}			; 51
!endif

!if ${nppVer_4} != 0
	!undef APPVERSION
	!define APPVERSION	${nppVer_1}.${nppVer_2}.${nppVer_3}.${nppVer_4}	; 7.5.1.3

	!undef VERSION_MINOR
	!define VERSION_MINOR	${nppVer_2}${nppVer_3}${nppVer_4}		; 513
!endif

; ------------------------------------------------------------------------

!define APPNAMEANDVERSION	"${APPNAME} v${APPVERSION}"
!define CompanyName		"Don HO don.h@free.fr"
!define Description		"Notepad++ : a free (GNU) source code editor"
!define Version		"${nppVer_1}.${nppVer_2}.${nppVer_3}.${nppVer_4}"
!define ProdVer		"${VERSION_MAJOR}.${VERSION_MINOR}"
!define LegalCopyright		"Copyleft 1998-2017 by Don HO"

!define APPWEBSITE "https://notepad-plus-plus.org/"

!define UNINSTALL_REG_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
!define MEMENTO_REGISTRY_ROOT HKLM
!define MEMENTO_REGISTRY_KEY ${UNINSTALL_REG_KEY}

; Main Install settings
Name "${APPNAMEANDVERSION}"

InstallDir "$PROGRAMFILES\${APPNAME}"

InstallDirRegKey HKLM "Software\${APPNAME}" ""
