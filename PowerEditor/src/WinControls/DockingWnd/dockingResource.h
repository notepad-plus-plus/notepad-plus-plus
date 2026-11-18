// This file is part of Notepad++ project
// Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

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

// Contribution by Don HO <don.h@free.fr>
// Date of Contribution: 2025-05-14
// Description of Contribution: Minor change

#pragma once

#define DM_NOFOCUSWHILECLICKINGCAPTION L"NOFOCUSWHILECLICKINGCAPTION"

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
	//#define DMM_GETIMAGELIST            (DMM_MSG + 8)
	//#define DMM_GETICONPOS              (DMM_MSG + 9)
	#define DMM_DROPDATA                (DMM_MSG + 10)
	#define DMM_MOVE_SPLITTER           (DMM_MSG + 11)
	#define DMM_CANCEL_MOVE             (DMM_MSG + 12)
	#define DMM_LBUTTONUP               (DMM_MSG + 13)

#define DMN_FIRST 1050
	#define DMN_CLOSE                   (DMN_FIRST + 1)
	#define DMN_DOCK                    (DMN_FIRST + 2)
	#define DMN_FLOAT                   (DMN_FIRST + 3)
	#define DMN_SWITCHIN                (DMN_FIRST + 4)
	#define DMN_SWITCHOFF               (DMN_FIRST + 5)
	#define DMN_FLOATDROPPED            (DMN_FIRST + 6)

