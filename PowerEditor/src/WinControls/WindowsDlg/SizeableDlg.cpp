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


#include "WindowsDlg.h"
#include "WindowsDlgRc.h"

SizeableDlg::SizeableDlg(WINRECT* pWinMap)
	: MyBaseClass(), _winMgr(pWinMap)
{
}

BOOL SizeableDlg::onInitDialog()
{
	_winMgr.InitToFitSizeFromCurrent(_hSelf);
	_winMgr.CalcLayout(_hSelf);
	_winMgr.SetWindowPositions(_hSelf);
	//getClientRect(_rc);
	return TRUE;
}

void SizeableDlg::onSize(UINT, int cx, int cy)
{
	_winMgr.CalcLayout(cx,cy,_hSelf);
	_winMgr.SetWindowPositions(_hSelf);
}

void SizeableDlg::onGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	_winMgr.GetMinMaxInfo(_hSelf, lpMMI);
}

LRESULT SizeableDlg::onWinMgr(WPARAM, LPARAM)
{
	return 0;
}

intptr_t CALLBACK SizeableDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			return onInitDialog();
		}
		case WM_GETMINMAXINFO:
		{
			onGetMinMaxInfo((MINMAXINFO*)lParam);
			return TRUE;
		}
		case WM_SIZE:
		{
			onSize(static_cast<UINT>(wParam), LOWORD(lParam), HIWORD(lParam));
			return TRUE;
		}

		default:
		{
			if (message == WM_WINMGR)
				return (BOOL)onWinMgr(wParam, lParam);

			break;
		}
	}
	return FALSE;
}