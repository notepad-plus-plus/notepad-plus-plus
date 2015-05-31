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


#include "ControlsTab.h"

void ControlsTab::createTabs(WindowVector & winVector)
{
	_pWinVector = &winVector;

	for (size_t i = 0, len = winVector.size(); i < len; ++i)
		TabBar::insertAtEnd(winVector[i]._name.c_str());

	TabBar::activateAt(0);
	activateWindowAt(0);
}

void ControlsTab::activateWindowAt(int index)
{
    if (index == _current)  return;
	(*_pWinVector)[_current]._dlg->display(false);
	(*_pWinVector)[index]._dlg->display(true);
	_current = index;
}

void ControlsTab::reSizeTo(RECT & rc)
{
	TabBar::reSizeTo(rc);
	rc.left += marge;
	rc.top += marge;
	
	//-- We do those dirty things 
	//-- because it's a "vertical" tab control
    if (_isVertical)
    {
	    rc.right -= 40;
	    rc.bottom -= 20;
	    if (getRowCount() == 2)
	    {
		    rc.right -= 20;
	    }
    }
	//-- end of dirty things
	rc.bottom -= 55;
	rc.right -= 20;

	(*_pWinVector)[_current]._dlg->reSizeTo(rc);
	(*_pWinVector)[_current]._dlg->redraw();

}

bool ControlsTab::renameTab(const TCHAR *internalName, const TCHAR *newName)
{
	bool foundIt = false;
	size_t i = 0;
	for (size_t len = _pWinVector->size(); i < len; ++i)
	{
		if ((*_pWinVector)[i]._internalName == internalName)
		{
			foundIt = true;
			break;
		}
	}
	if (!foundIt)
		return false;

	renameTab(i, newName);
	return true;
}

void ControlsTab::renameTab(int index, const TCHAR *newName)
{
	TCITEM tie;
	tie.mask = TCIF_TEXT;
	tie.pszText = (TCHAR *)newName;
	TabCtrl_SetItem(_hSelf, index, &tie);
}