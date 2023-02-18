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


; http://nsis.sourceforge.net/Get_Windows_version

; GetWindowsVersion 4.1.1 (2015-06-22)
;
; Based on Yazno's function, http://yazno.tripod.com/powerpimpit/
; Update by Joost Verburg
; Update (Macro, Define, Windows 7 detection) - John T. Haller of PortableApps.com - 2008-01-07
; Update (Windows 8 detection) - Marek Mizanin (Zanir) - 2013-02-07
; Update (Windows 8.1 detection) - John T. Haller of PortableApps.com - 2014-04-04
; Update (Windows 10 TP detection) - John T. Haller of PortableApps.com - 2014-10-01
; Update (Windows 10 TP4 detection, and added include guards) - Kairu - 2015-06-22
;
; Usage: ${GetWindowsVersion} $R0
;
; $R0 contains: 95, 98, ME, NT x.x, 2000, XP, 2003, Vista, 7, 8, 8.1, 10.0 or '' (for unknown)
 
!ifndef __GET_WINDOWS_VERSION_NSH
!define __GET_WINDOWS_VERSION_NSH
 
Function GetWindowsVersion
 
  Push $R0
  Push $R1
 
  ClearErrors
 
  ; check if Windows NT family
  ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
 
  IfErrors 0 lbl_winnt
 
  ; we are not NT
  ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion" VersionNumber
 
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
  StrCmp $R1 '6.0' lbl_winnt_vista
  StrCmp $R1 '6.1' lbl_winnt_7
  StrCmp $R1 '6.2' lbl_winnt_8
  StrCmp $R1 '6.3' lbl_winnt_81
  StrCmp $R1 '6.4' lbl_winnt_10 ; the early Windows 10 tech previews used version 6.4
 
  StrCpy $R1 $R0 4
 
  StrCmp $R1 '10.0' lbl_winnt_10
  Goto lbl_error
 
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
 
lbl_winnt_7:
  Strcpy $R0 '7'
  Goto lbl_done
 
lbl_winnt_8:
  Strcpy $R0 '8'
  Goto lbl_done
 
lbl_winnt_81:
lbl_winnt_10:
  ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentBuildNumber
  ${If} $R0 >= 17763 ; Windows 10 or later
      ${If} $R0 >= 22000 ; Windows 11 or later
        Strcpy $R0 '11'
        Goto lbl_done
	  ${Else}
        Strcpy $R0 '10'
        Goto lbl_done
	  ${EndIf}

  ${Else}
    Strcpy $R0 '8.1'
    Goto lbl_done
  ${EndIf}
  Goto lbl_done
 
lbl_error:
  Strcpy $R0 ''

lbl_done:
  Pop $R1
  Exch $R0
 
FunctionEnd
 
!macro GetWindowsVersion OUTPUT_VALUE
	Call GetWindowsVersion
	Pop `${OUTPUT_VALUE}`
!macroend
 
!define GetWindowsVersion '!insertmacro "GetWindowsVersion"'
 
!endif
