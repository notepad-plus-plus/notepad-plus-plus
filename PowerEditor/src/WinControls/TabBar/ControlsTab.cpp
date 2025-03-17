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
	rc.bottom -= 55;
	rc.right -= 20;

	(*_pWinVector)[_current]._dlg->reSizeTo(rc);
	(*_pWinVector)[_current]._dlg->redraw();

}

bool ControlsTab::renameTab(const wchar_t *internalName, const wchar_t *newName)
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

void ControlsTab::renameTab(size_t index, const wchar_t *newName)
{
	TCITEM tie;
	tie.mask = TCIF_TEXT;
	tie.pszText = (wchar_t *)newName;
	TabCtrl_SetItem(_hSelf, index, &tie);
}