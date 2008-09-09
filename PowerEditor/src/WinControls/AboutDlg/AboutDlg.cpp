//this file is part of notepad++
//Copyright (C)2003 Don HO <don.h@free.fr>
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

#ifdef UNICODE
			WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
			buildTime +=  wmc->char2wchar(__DATE__, CP_ACP);
			buildTime += TEXT(" - ");
			buildTime +=  wmc->char2wchar(__TIME__, CP_ACP);

#else
			buildTime +=  __DATE__;
			buildTime += TEXT(" - ");
			buildTime +=  __TIME__;
#endif
			::SendMessage(compileDateHandle, WM_SETTEXT, 0, (LPARAM)buildTime.c_str());
			::EnableWindow(compileDateHandle, FALSE);

            HWND licenceEditHandle = ::GetDlgItem(_hSelf, IDC_LICENCE_EDIT);
            ::SendMessage(licenceEditHandle, WM_SETTEXT, 0, (LPARAM)LICENCE_TXT);

            _emailLink.init(_hInst, _hSelf);
			_emailLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), TEXT("mailto:don.h@free.fr"));

            _pageLink.init(_hInst, _hSelf);
            _pageLink.create(::GetDlgItem(_hSelf, IDC_HOME_ADDR), TEXT("http://notepad-plus.sourceforge.net/"));

			//_onLineHelp.init(_hInst, _hSelf);
            //_onLineHelp.create(::GetDlgItem(_hSelf, IDC_ONLINEHELP_ADDR), TEXT("http://notepad-plus.sourceforge.net/uk/generalFAQ.php"));

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
			HICON hIcon = ::LoadIcon(_hInst, MAKEINTRESOURCE(IDI_M30ICON));
			DRAWITEMSTRUCT *pdis = (DRAWITEMSTRUCT *)lParam;
			::DrawIcon(pdis->hDC, 0, 0, hIcon);
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

