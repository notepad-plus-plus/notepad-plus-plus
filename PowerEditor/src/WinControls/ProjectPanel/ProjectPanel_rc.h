// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#pragma once

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
  #define IDM_PROJECT_FINDINPROJECTSWS (IDD_PROJECTPANEL_MENUWS + 8)

#define	IDD_PROJECTPANEL_CTRL		(IDD_PROJECTPANEL + 30)
  #define	ID_PROJECTTREEVIEW    (IDD_PROJECTPANEL_CTRL + 1)
  #define	IDB_PROJECT_BTN       (IDD_PROJECTPANEL_CTRL + 2)
  #define	IDB_EDIT_BTN          (IDD_PROJECTPANEL_CTRL + 3)


#define	IDD_FILERELOCALIZER_DIALOG  3200
  #define	IDC_EDIT_FILEFULLPATHNAME  (IDD_FILERELOCALIZER_DIALOG + 1)
  

