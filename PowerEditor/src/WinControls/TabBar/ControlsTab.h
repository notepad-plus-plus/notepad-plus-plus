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

#include "TabBar.h"
#include "StaticDialog.h"
//#include "SplitterContainer.h"
#include <vector>

struct DlgInfo {
    Window *_dlg;
    TCHAR _name[64];
	TCHAR _internalName[32];

	DlgInfo(Window *dlg, TCHAR *name, TCHAR *internalName = NULL): _dlg(dlg) {
		lstrcpy(_name, name);
		if (!internalName)
			_internalName[0] = '\0';
		else
			lstrcpy(_internalName, internalName);
	};
};

typedef std::vector<DlgInfo> WindowVector;

class ControlsTab : public TabBar
{
public :
	ControlsTab() : TabBar(), _pWinVector(NULL), _current(0), _isVertical(false) {};
	~ControlsTab(){};
	//void init(HINSTANCE hInst, HWND pere, bool isVertical, WindowVector & winVector);
	virtual void init(HINSTANCE hInst, HWND hwnd, bool isVertical = false, bool isTraditional = false, bool isMultiLine = false) {
		_isVertical = isVertical;
		//TabBar::init(hInst, hwnd, false, true);
		TabBar::init(hInst, hwnd, false, isTraditional, isMultiLine);
	};
	void ControlsTab::createTabs(WindowVector & winVector);

	void destroy() {
		TabBar::destroy();
	};
	
	virtual void reSizeTo(RECT & rc);
	
	void activateWindowAt(int index)
	{
        if (index == _current)  return;
		(*_pWinVector)[_current]._dlg->display(false);
		(*_pWinVector)[index]._dlg->display(true);
		_current = index;
	};

	void clickedUpdate()
	{
		int indexClicked = int(::SendMessage(_hSelf, TCM_GETCURSEL, 0, 0));
		activateWindowAt(indexClicked);
	};

	void renameTab(int index, const TCHAR *newName) {
		TCITEM tie;
		tie.mask = TCIF_TEXT;
		tie.pszText = (TCHAR *)newName;
		TabCtrl_SetItem(_hSelf, index, &tie);
	};

	bool renameTab(const TCHAR *internalName, const TCHAR *newName) {
		bool foundIt = false;
		size_t i = 0;
		for ( ; i < _pWinVector->size() ; i++)
		{
			if (!lstrcmp((*_pWinVector)[i]._internalName, internalName))
			{
				foundIt = true;
				break;
			}
		}
		if (!foundIt)
			return false;

		renameTab(i, newName);
		return true;
	};

private :
	WindowVector *_pWinVector;
    int _current;
    bool _isVertical;
};



#endif //CONTROLS_TAB_H
