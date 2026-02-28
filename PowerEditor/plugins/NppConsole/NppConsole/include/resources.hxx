/*
This file is part of Notepad++ console plugin.
Copyright ©2011 Mykhajlo Pobojnyj <mpoboyny@web.de>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

// resources.hxx

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

/* VERSION DEFINITIONS */
#define VER_MAJOR   1
#define VER_MINOR   2
#define VER_RELEASE 8
#define VER_BUILD   1
#define VER_STRING  STR(VER_MAJOR) "." STR(VER_MINOR) "." STR(VER_RELEASE) "." STR(VER_BUILD)

#define IDI_APPICON                     101
#define IDD_DIALOG_ABOUT                102
#define IDB_TLB_IMG                     103

#define IDC_EDIT_ABOUT					1001
#define IDC_EDIT_LINE					1003
#define IDC_RADIO_IGN					1005
#define IDC_RADIO_RESTR					1006
#define IDC_RADIO_PROCESS				1007
#define IDC_CBO_COMMAND                 1008
#define IDC_STC_VER                     1009
#define IDC_CHK_PANELTOGGLE             1010

#define IDC_STATIC -1
