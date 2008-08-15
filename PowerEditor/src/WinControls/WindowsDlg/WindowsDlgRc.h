//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
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

#ifndef WINDOWSDLG_RC_H
#define WINDOWSDLG_RC_H

#ifdef __GNUC__
#define _WIN32_IE 0x0600

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
	#define  IDM_WINDOW_WINDOWS   (IDR_WINDOWS_MENU + 1)
	#define  IDM_WINDOW_MRU_FIRST (IDR_WINDOWS_MENU + 20)
	#define  IDM_WINDOW_MRU_LIMIT (IDR_WINDOWS_MENU + 29)

#endif //WINDOWSDLG_RC_H
