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

#ifdef __GNUC__
#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif

	#ifndef LVS_OWNERDATA
		#define LVS_OWNERDATA 4096
	#endif

#endif

#define IDD_WINDOWS 7000
	#define IDC_WINDOWS_LIST (IDD_WINDOWS + 1)
	#define IDC_WINDOWS_SAVE (IDD_WINDOWS + 2)
	#define IDC_WINDOWS_CLOSE (IDD_WINDOWS + 3)
	#define IDC_WINDOWS_SORT (IDD_WINDOWS + 4)

#define IDR_WINDOWS_MENU 11000
	#define  IDM_WINDOW_WINDOWS   	(IDR_WINDOWS_MENU + 1)
	#define  IDM_WINDOW_SORT_FN_ASC	(IDR_WINDOWS_MENU + 2)
	#define  IDM_WINDOW_SORT_FN_DSC	(IDR_WINDOWS_MENU + 3)
	#define  IDM_WINDOW_SORT_FP_ASC	(IDR_WINDOWS_MENU + 4)
	#define  IDM_WINDOW_SORT_FP_DSC	(IDR_WINDOWS_MENU + 5)
	#define  IDM_WINDOW_SORT_FT_ASC	(IDR_WINDOWS_MENU + 6)
	#define  IDM_WINDOW_SORT_FT_DSC	(IDR_WINDOWS_MENU + 7)
	#define  IDM_WINDOW_SORT_FS_ASC	(IDR_WINDOWS_MENU + 8)
	#define  IDM_WINDOW_SORT_FS_DSC	(IDR_WINDOWS_MENU + 9)
	#define  IDM_WINDOW_MRU_FIRST 	(IDR_WINDOWS_MENU + 20)
	#define  IDM_WINDOW_MRU_LIMIT 	(IDR_WINDOWS_MENU + 29)
	#define  IDM_WINDOW_COPY_NAME 	(IDM_WINDOW_MRU_LIMIT + 1)
	#define  IDM_WINDOW_COPY_PATH 	(IDM_WINDOW_MRU_LIMIT + 2)
