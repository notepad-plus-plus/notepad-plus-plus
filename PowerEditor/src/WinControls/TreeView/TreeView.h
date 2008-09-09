//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
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

#ifndef TREE_VIEW_H
#define  TREE_VIEW_H

#include <windows.h>
#include "Window.h"

#ifndef _WIN32_IE
#define _WIN32_IE	0x0600
#endif //_WIN32_IE

#include <commctrl.h>

class TreeView : public Window
{
public :
	TreeView(){};
	~TreeView(){};
	virtual void init(HINSTANCE hInst, HWND pere);
	virtual void destroy() {
		::DestroyWindow(_hSelf);
	};
	
private :
	HTREEITEM insertTo(HTREEITEM parent, TCHAR *itemStr, int imgIndex);
};
	
#endif