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



#include <Shlobj.h>
#include <uxtheme.h>

#include "AboutDlg.h"
#include "Parameters.h"

BOOL CALLBACK AboutDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
        case WM_INITDIALOG :
		{
			HWND compileDateHandle = ::GetDlgItem(_hSelf, IDC_BUILD_DATETIME);
			generic_string buildTime = TEXT("Build time : ");

			WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
			buildTime +=  wmc->char2wchar(__DATE__, CP_ACP);
			buildTime += TEXT(" - ");
			buildTime +=  wmc->char2wchar(__TIME__, CP_ACP);

			::SendMessage(compileDateHandle, WM_SETTEXT, 0, (LPARAM)buildTime.c_str());
			::EnableWindow(compileDateHandle, FALSE);

            HWND licenceEditHandle = ::GetDlgItem(_hSelf, IDC_LICENCE_EDIT);
            ::SendMessage(licenceEditHandle, WM_SETTEXT, 0, (LPARAM)LICENCE_TXT);

            _emailLink.init(_hInst, _hSelf);
			//_emailLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), TEXT("mailto:don.h@free.fr"));
			_emailLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), TEXT("https://notepad-plus-plus.org/contributors"));

            _pageLink.init(_hInst, _hSelf);
            _pageLink.create(::GetDlgItem(_hSelf, IDC_HOME_ADDR), TEXT("https://notepad-plus-plus.org/"));

			getClientRect(_rc);

			NppParameters *pNppParam = NppParameters::getInstance();
			ETDTProc enableDlgTheme = (ETDTProc)pNppParam->getEnableThemeDlgTexture();
			if (enableDlgTheme)
			{
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);
				redraw();
			}

			return TRUE;
		}

		case WM_DRAWITEM :
		{
			HICON hIcon = (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_CHAMELEON), IMAGE_ICON, 64, 64, LR_DEFAULTSIZE);
			//HICON hIcon = (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_JESUISCHARLIE), IMAGE_ICON, 64, 64, LR_DEFAULTSIZE);
			DRAWITEMSTRUCT *pdis = (DRAWITEMSTRUCT *)lParam;
			::DrawIconEx(pdis->hDC, 0, 0, hIcon, 64, 64, 0, NULL, DI_NORMAL);
			return TRUE;
		}

		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDCANCEL :
				case IDOK : 
					display(false);
					return TRUE;

				default :
					break;
			}
		}

		case WM_DESTROY :
		{
			return TRUE;
		}
	}
	return FALSE;	
}

void AboutDlg::doDialog()
{
	if (!isCreated())
		create(IDD_ABOUTBOX);

    // Adjust the position of AboutBox
	goToCenter();
};

