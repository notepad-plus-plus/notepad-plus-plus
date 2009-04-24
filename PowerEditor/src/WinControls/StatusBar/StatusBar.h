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

#ifndef STATUS_BAR_H
#define STATUS_BAR_H
#include "Window.h"

#ifndef _WIN32_IE
#define _WIN32_IE	0x0600
#endif //_WIN32_IE

#include <commctrl.h>

class StatusBar : public Window
{
public :
	StatusBar() : Window(), _partWidthArray(NULL), _hloc(NULL), _lpParts(NULL) {};
	virtual ~StatusBar(){
        if (_hloc) 
        {
            ::LocalUnlock(_hloc);
            ::LocalFree(_hloc);
        }
		if (_partWidthArray)
			delete [] _partWidthArray;
    };

	virtual void init(HINSTANCE hInst, HWND hPere, int nbParts);

	bool setPartWidth(int whichPart, int width) {
		if (whichPart >= _nbParts)
			return false;

		_partWidthArray[whichPart] = width;
		return true;
	};
	virtual void destroy() {
		::DestroyWindow(_hSelf);
	};

    virtual void reSizeTo(RECT & rc) {
        ::MoveWindow(_hSelf, rc.left, rc.top, rc.right, rc.bottom, TRUE);
        adjustParts(rc.right);
        redraw();
    };


	int getHeight() const {
		if (!::IsWindowVisible(_hSelf))
			return 0;
		return Window::getHeight();
	};

    bool setText(const TCHAR *str, int whichPart) const {
        if (whichPart > _nbParts) 
            return false;
		return (::SendMessage(_hSelf, SB_SETTEXT, whichPart, (LPARAM)str) == TRUE);
    };

	void adjustParts(int clientWidth);

private :
    int _nbParts;
    int *_partWidthArray;

    HLOCAL _hloc;
    LPINT _lpParts;    
};

#endif // STATUS_BAR_H
