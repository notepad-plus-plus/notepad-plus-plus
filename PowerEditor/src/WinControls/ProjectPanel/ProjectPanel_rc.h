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


#ifndef PROJECTPANEL_RC_H
#define PROJECTPANEL_RC_H

#define	IDD_PROJECTPANEL		3100

#define	IDD_PROJECTPANEL_MENU		(IDD_PROJECTPANEL + 10)
  #define IDM_PROJECT_RENAME       (IDD_PROJECTPANEL_MENU + 1)
  #define IDM_PROJECT_NEWFOLDER    (IDD_PROJECTPANEL_MENU + 2)
  #define IDM_PROJECT_ADDFILES     (IDD_PROJECTPANEL_MENU + 3)
  #define IDM_PROJECT_DELETEFOLDER (IDD_PROJECTPANEL_MENU + 4)
  #define IDM_PROJECT_DELETEFILE   (IDD_PROJECTPANEL_MENU + 5)
  #define IDM_PROJECT_MODIFYFILEPATH   (IDD_PROJECTPANEL_MENU + 6)
  #define IDM_PROJECT_ADDFILESRECUSIVELY   (IDD_PROJECTPANEL_MENU + 7)
  #define IDM_PROJECT_MOVEUP       (IDD_PROJECTPANEL_MENU + 8)
  #define IDM_PROJECT_MOVEDOWN     (IDD_PROJECTPANEL_MENU + 9)

#define	IDD_PROJECTPANEL_MENUWS	(IDD_PROJECTPANEL + 20)
  #define IDM_PROJECT_NEWPROJECT    (IDD_PROJECTPANEL_MENUWS + 1)
  #define IDM_PROJECT_NEWWS         (IDD_PROJECTPANEL_MENUWS + 2)
  #define IDM_PROJECT_OPENWS        (IDD_PROJECTPANEL_MENUWS + 3)
  #define IDM_PROJECT_RELOADWS      (IDD_PROJECTPANEL_MENUWS + 4)
  #define IDM_PROJECT_SAVEWS        (IDD_PROJECTPANEL_MENUWS + 5)
  #define IDM_PROJECT_SAVEASWS      (IDD_PROJECTPANEL_MENUWS + 6)
  #define IDM_PROJECT_SAVEACOPYASWS (IDD_PROJECTPANEL_MENUWS + 7)

#define	IDD_PROJECTPANEL_CTRL		(IDD_PROJECTPANEL + 30)
  #define	ID_PROJECTTREEVIEW    (IDD_PROJECTPANEL_CTRL + 1)
  #define	IDB_PROJECT_BTN       (IDD_PROJECTPANEL_CTRL + 2)
  #define	IDB_EDIT_BTN          (IDD_PROJECTPANEL_CTRL + 3)


#define	IDD_FILERELOCALIZER_DIALOG  3200
  #define	IDC_EDIT_FILEFULLPATHNAME  (IDD_FILERELOCALIZER_DIALOG + 1)
  
#endif // PROJECTPANEL_RC_H

