//this file is part of docking functionality for Notepad++
//Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef DOCKING_RESOURCE_H
#define DOCKING_RESOURCE_H

#define IDD_PLUGIN_DLG                  103
#define IDC_EDIT1                       1000


#define IDB_CLOSE_DOWN                  137
#define IDB_CLOSE_UP                    138
#define IDD_CONTAINER_DLG               139

#define IDC_TAB_CONT                    1027
#define IDC_CLIENT_TAB                  1028
#define IDC_BTN_CAPTION                 1050

#define DMM_MSG                         0x5000
    #define DMM_CLOSE                   (DMM_MSG + 1)
    #define DMM_DOCK                    (DMM_MSG + 2)
    #define DMM_FLOAT                   (DMM_MSG + 3)
    #define DMM_DOCKALL                 (DMM_MSG + 4)
    #define DMM_FLOATALL                (DMM_MSG + 5)
    #define DMM_MOVE                    (DMM_MSG + 6)
    #define DMM_UPDATEDISPINFO          (DMM_MSG + 7)
    #define DMM_GETIMAGELIST			(DMM_MSG + 8)
    #define DMM_GETICONPOS              (DMM_MSG + 9)
    #define DMM_DROPDATA				(DMM_MSG + 10)
    #define DMM_MOVE_SPLITTER		    (DMM_MSG + 11)
	#define DMM_CANCEL_MOVE				(DMM_MSG + 12)
	#define DMM_LBUTTONUP				(DMM_MSG + 13)

#define DMN_FIRST 1050
	#define DMN_CLOSE					(DMN_FIRST + 1)
	//nmhdr.code = DWORD(DMN_CLOSE, 0));
	//nmhdr.hwndFrom = hwndNpp;
	//nmhdr.idFrom = ctrlIdNpp;

	#define DMN_DOCK		            (DMN_FIRST + 2)
    #define DMN_FLOAT					(DMN_FIRST + 3)
	//nmhdr.code = DWORD(DMN_XXX, int newContainer);
	//nmhdr.hwndFrom = hwndNpp;
	//nmhdr.idFrom = ctrlIdNpp;



#endif //DOCKING_RESOURCE_H

