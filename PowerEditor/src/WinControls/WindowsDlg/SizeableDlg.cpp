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

INT_PTR CALLBACK SizeableDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
	case WM_INITDIALOG :
		return onInitDialog();

	case WM_GETMINMAXINFO :
		onGetMinMaxInfo((MINMAXINFO*)lParam);
		return TRUE;

	case WM_SIZE:
		onSize(wParam, LOWORD(lParam), HIWORD(lParam));
		return TRUE;
	
	default:
		if (message == WM_WINMGR)
		{
			return (BOOL)onWinMgr(wParam, lParam);
		}
		break;
	}
	return FALSE;
}