// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid      
// misunderstandings, we consider an application to constitute a          
// "derivative work" for the purpose of this license if it does any of the
// following:                                                             
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#define NOTEPADPLUS_USER   (WM_USER + 1000)

#define WM_GETCURRENTSCINTILLA  (NOTEPADPLUS_USER + 4)
#define WM_GETCURRENTLANGTYPE  (NOTEPADPLUS_USER + 5)
#define WM_SETCURRENTLANGTYPE  (NOTEPADPLUS_USER + 6)
#define WM_NBOPENFILES			(NOTEPADPLUS_USER + 7)
#define WM_GETOPENFILENAMES		(NOTEPADPLUS_USER + 8)

#define WM_MODELESSDIALOG		 (NOTEPADPLUS_USER + 12)
#define WM_NBSESSIONFILES (NOTEPADPLUS_USER + 13)
#define WM_GETSESSIONFILES (NOTEPADPLUS_USER + 14)
#define WM_SAVESESSION (NOTEPADPLUS_USER + 15)
#define WM_SAVECURRENTSESSION (NOTEPADPLUS_USER + 16)
#define WM_GETOPENFILENAMES_PRIMARY (NOTEPADPLUS_USER + 17)
#define WM_GETOPENFILENAMES_SECOND (NOTEPADPLUS_USER + 18)

#define WM_CREATESCINTILLAHANDLE (NOTEPADPLUS_USER + 20)
#define WM_DESTROYSCINTILLAHANDLE (NOTEPADPLUS_USER + 21)
#define WM_GETNBUSERLANG (NOTEPADPLUS_USER + 22)
#define WM_GETCURRENTDOCINDEX (NOTEPADPLUS_USER + 23)
#define WM_SETSTATUSBAR (NOTEPADPLUS_USER + 24)
#define WM_GETMENUHANDLE (NOTEPADPLUS_USER + 25)
#define WM_ENCODE_SCI (NOTEPADPLUS_USER + 26)
#define WM_DECODE_SCI (NOTEPADPLUS_USER + 27)
#define WM_ACTIVATE_DOC (NOTEPADPLUS_USER + 28)
#define WM_LAUNCH_FINDINFILESDLG (NOTEPADPLUS_USER + 29)
#define WM_DMM_SHOW (NOTEPADPLUS_USER + 30)
#define WM_DMM_HIDE	(NOTEPADPLUS_USER + 31)
#define WM_DMM_UPDATEDISPINFO (NOTEPADPLUS_USER + 32)
#define WM_DMM_REGASDCKDLG (NOTEPADPLUS_USER + 33)
#define WM_LOADSESSION (NOTEPADPLUS_USER + 34)
#define WM_DMM_VIEWOTHERTAB (NOTEPADPLUS_USER + 35)
#define WM_RELOADFILE (NOTEPADPLUS_USER + 36)
#define WM_SWITCHTOFILE (NOTEPADPLUS_USER + 37)
#define WM_SAVECURRENTFILE (NOTEPADPLUS_USER + 38)
#define WM_SAVEALLFILES	(NOTEPADPLUS_USER + 39)
#define WM_PIMENU_CHECK	(NOTEPADPLUS_USER + 40)
#define WM_ADDTOOLBARICON (NOTEPADPLUS_USER + 41)
#define WM_GETWINDOWSVERSION (NOTEPADPLUS_USER + 42)
#define WM_DMM_GETPLUGINHWNDBYNAME (NOTEPADPLUS_USER + 43)



#define	RUNCOMMAND_USER_    (WM_USER + 3000)

#define WM_GET_FULLCURRENTPATH (RUNCOMMAND_USER_ + FULL_CURRENT_PATH)
#define WM_GET_CURRENTDIRECTORY (RUNCOMMAND_USER_ + CURRENT_DIRECTORY)
#define WM_GET_FILENAME (RUNCOMMAND_USER_ + FILE_NAME)
#define WM_GET_NAMEPART (RUNCOMMAND_USER_ + NAME_PART)
#define WM_GET_EXTPART (RUNCOMMAND_USER_ + EXT_PART)
#define WM_GET_CURRENTWORD (RUNCOMMAND_USER_ + CURRENT_WORD)
#define WM_GET_NPPDIRECTORY (RUNCOMMAND_USER_ + NPP_DIRECTORY)
