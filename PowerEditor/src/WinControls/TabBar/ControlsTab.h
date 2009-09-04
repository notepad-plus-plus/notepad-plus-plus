/*
this file is part of notepad++
Copyright (C)2003 Don HO <donho@altern.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef CONTROLS_TAB_H
#define CONTROLS_TAB_H

#ifndef TAB_BAR_H
#include "TabBar.h"
#endif //TAB_BAR_H

struct DlgInfo {
    Window *_dlg;
    generic_string _name;
	generic_string _internalName;

	DlgInfo(Window *dlg, TCHAR *name, TCHAR *internalName = NULL): _dlg(dlg), _name(name), _internalName(internalName?internalName:TEXT("")) {};
};

typedef std::vector<DlgInfo> WindowVector;

class ControlsTab : public TabBar
{
public :
	ControlsTab() : TabBar(), _pWinVector(NULL), _current(0), _isVertical(false) {};
	~ControlsTab(){};

	virtual void init(HINSTANCE hInst, HWND hwnd, bool isVertical = false, bool isTraditional = false, bool isMultiLine = false) {
		_isVertical = isVertical;
		TabBar::init(hInst, hwnd, false, isTraditional, isMultiLine);
	};
	void ControlsTab::createTabs(WindowVector & winVector);

	void destroy() {
		TabBar::destroy();
	};
	
	virtual void reSizeTo(RECT & rc);
	void activateWindowAt(int index);

	void clickedUpdate()
	{
		int indexClicked = int(::SendMessage(_hSelf, TCM_GETCURSEL, 0, 0));
		activateWindowAt(indexClicked);
	};
	void renameTab(int index, const TCHAR *newName);
	bool renameTab(const TCHAR *internalName, const TCHAR *newName);

private :
	WindowVector *_pWinVector;
    int _current;
    bool _isVertical;
};



#endif //CONTROLS_TAB_H
