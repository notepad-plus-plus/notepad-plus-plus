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



#include "lesDlgs.h"
#include "resource.h"
#include "menuCmdID.h"

void ValueDlg::init(HINSTANCE hInst, HWND parent, int valueToSet, const TCHAR *text) 
{
	Window::init(hInst, parent);
	_defaultValue = valueToSet;
	_name = text;
}

int ValueDlg::doDialog(POINT p, bool isRTL) 
{
	_p = p;
	if (isRTL)
	{
		DLGTEMPLATE *pMyDlgTemplate = NULL;
		HGLOBAL hMyDlgTemplate = makeRTLResource(IDD_VALUE_DLG, &pMyDlgTemplate);
		int result = ::DialogBoxIndirectParam(_hInst, pMyDlgTemplate, _hParent,  dlgProc, (LPARAM)this);
		::GlobalFree(hMyDlgTemplate);
		return result;
	}
	return ::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_VALUE_DLG), _hParent,  dlgProc, (LPARAM)this);
}


int ValueDlg::reSizeValueBox()
{
	if (_nbNumber == DEFAULT_NB_NUMBER) return 0;
	RECT rect;
	POINT p;

	HWND hEdit = ::GetDlgItem(_hSelf, IDC_VALUE_EDIT);

	//get screen coordonnees (x,y)
	::GetWindowRect(hEdit, &rect);
	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top;

	p.x = rect.left;
	p.y = rect.top;

	// convert screen coordonnees to client coordonnees
	::ScreenToClient(_hSelf, &p);

	int unit = w / (DEFAULT_NB_NUMBER + 2);
	int extraSize = (_nbNumber-DEFAULT_NB_NUMBER)*unit;
	::MoveWindow(hEdit, p.x, p.y, w + extraSize, h, FALSE);

	return extraSize;
}

INT_PTR CALLBACK ValueDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM) 
{
	switch (Message)
	{
		case WM_INITDIALOG :
		{
			::SetDlgItemText(_hSelf, IDC_VALUE_STATIC, _name.c_str());
			::SetDlgItemInt(_hSelf, IDC_VALUE_EDIT, _defaultValue, FALSE);

			RECT rc;
			::GetClientRect(_hSelf, &rc);
			int size = reSizeValueBox();
			::MoveWindow(_hSelf, _p.x, _p.y, rc.right - rc.left + size, rc.bottom - rc.top + 30, TRUE);
			
			return TRUE;
		}

		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDOK :
				{
					int i = ::GetDlgItemInt(_hSelf, IDC_VALUE_EDIT, NULL, FALSE);
					::EndDialog(_hSelf, i);
					return TRUE;
				}

				case IDCANCEL :
					::EndDialog(_hSelf, -1);
					return TRUE;

				default:
					return FALSE;
			}
		}
		default :
			return FALSE;
	}
}



INT_PTR CALLBACK ButtonDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM) 
{
	switch (Message)
	{
		case WM_INITDIALOG :
		{
			return TRUE;
		}

		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDC_RESTORE_BUTTON :
				{
                    int bs = getButtonStatus();
                    bool isFullScreen = (bs & buttonStatus_fullscreen) != 0;
                    bool isPostIt = (bs & buttonStatus_postit) != 0;
                    int cmd = 0;
                    if (isFullScreen && isPostIt)
                    {
                        // remove postit firstly
                        cmd = IDM_VIEW_POSTIT;
                    }
                    else if (isFullScreen)
                    {
                        cmd = IDM_VIEW_FULLSCREENTOGGLE;
                    }
                    else if (isPostIt)
                    {
                        cmd = IDM_VIEW_POSTIT;
                    }
                    ::SendMessage(_hParent, WM_COMMAND, cmd, 0);
					display(false);
					return TRUE;
				}

				default:
					return FALSE;
			}
		}
		default :
			return FALSE;
	}
}




void ButtonDlg::doDialog(bool isRTL) 
{
    if (!isCreated())
			create(IDD_BUTTON_DLG, isRTL);
	display();
}
