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

#include <windows.h>

#include "TabBar.h"
#include "Window.h"

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
	if (index == _current) return;
	(*_pWinVector)[_current]._dlg->display(false);
	(*_pWinVector)[index]._dlg->display(true);
	_current = index;
}

void ControlsTab::reSizeToWH(RECT& rc)
{
	Window::reSizeToWH(rc);

	RECT rcTab{};
	TabCtrl_GetItemRect(_hSelf, 0, &rcTab);
	const LONG tabHeight = (rcTab.bottom - rcTab.top) + _dpiManager.getSystemMetricsForDpi(SM_CYEDGE);

	::GetClientRect(_hSelf, &rc);
	::MapWindowPoints(_hSelf, _hParent, reinterpret_cast<LPPOINT>(&rc), 2);

	rc.top += tabHeight;

	const LONG padding = _dpiManager.scale(3);
	::InflateRect(&rc, -padding, -padding);

	for (const auto& dlgInfo : *_pWinVector)
	{
		dlgInfo._dlg->Window::reSizeToWH(rc);
	}

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
	TCITEM tie{};
	tie.mask = TCIF_TEXT;
	tie.pszText = const_cast<wchar_t*>(newName);
	TabCtrl_SetItem(_hSelf, index, &tie);
}
