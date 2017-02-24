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



#include <shlobj.h>
#include <uxtheme.h>

#include "AboutDlg.h"
#include "Parameters.h"

INT_PTR CALLBACK AboutDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
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

			NppParameters *pNppParam = NppParameters::getInstance();
			LPCTSTR bitness = pNppParam ->isx64() ? TEXT("(64-bit)") : TEXT("(32-bit)");
			::SetDlgItemText(_hSelf, IDC_VERSION_BIT, bitness);

			::SendMessage(compileDateHandle, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buildTime.c_str()));
			::EnableWindow(compileDateHandle, FALSE);

            HWND licenceEditHandle = ::GetDlgItem(_hSelf, IDC_LICENCE_EDIT);
			::SendMessage(licenceEditHandle, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(LICENCE_TXT));

            _emailLink.init(_hInst, _hSelf);
			//_emailLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), TEXT("mailto:don.h@free.fr"));
			_emailLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), TEXT("https://notepad-plus-plus.org/contributors"));

            _pageLink.init(_hInst, _hSelf);
            _pageLink.create(::GetDlgItem(_hSelf, IDC_HOME_ADDR), TEXT("https://notepad-plus-plus.org/"));

			getClientRect(_rc);

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
}


INT_PTR CALLBACK DebugInfoDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			NppParameters *pNppParam = NppParameters::getInstance();

			// Notepad++ version
			_debugInfoStr = NOTEPAD_PLUS_VERSION;
			_debugInfoStr += pNppParam->isx64() ? TEXT("   (64-bit)") : TEXT("   (32-bit)");
			_debugInfoStr += TEXT("\r\n");

			// Build time
			_debugInfoStr += TEXT("Build time : ");
			generic_string buildTime;
			WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
			buildTime += wmc->char2wchar(__DATE__, CP_ACP);
			buildTime += TEXT(" - ");
			buildTime += wmc->char2wchar(__TIME__, CP_ACP);
			_debugInfoStr += buildTime;
			_debugInfoStr += TEXT("\r\n");

			// Binary path
			_debugInfoStr += TEXT("Path : ");
			TCHAR nppFullPath[MAX_PATH];
			::GetModuleFileName(NULL, nppFullPath, MAX_PATH);
			_debugInfoStr += nppFullPath;
			_debugInfoStr += TEXT("\r\n");

			// Administrator mode
			_debugInfoStr += TEXT("Admin mode : ");
			_debugInfoStr += (_isAdmin ? TEXT("ON") : TEXT("OFF"));
			_debugInfoStr += TEXT("\r\n");

			// local conf
			_debugInfoStr += TEXT("Local Conf mode : ");
			bool doLocalConf = (NppParameters::getInstance())->isLocal();
			_debugInfoStr += (doLocalConf ? TEXT("ON") : TEXT("OFF"));
			_debugInfoStr += TEXT("\r\n");

			// OS version
			_debugInfoStr += TEXT("OS : ");
			_debugInfoStr += (NppParameters::getInstance())->getWinVersionStr();
			_debugInfoStr += TEXT(" (");
			_debugInfoStr += (NppParameters::getInstance())->getWinVerBitStr();
			_debugInfoStr += TEXT(")");
			_debugInfoStr += TEXT("\r\n");

			// Plugins
			_debugInfoStr += TEXT("Plugins : ");
			_debugInfoStr += _loadedPlugins.length() == 0 ? TEXT("none") : _loadedPlugins;
			_debugInfoStr += TEXT("\r\n");

			::SetDlgItemText(_hSelf, IDC_DEBUGINFO_EDIT, _debugInfoStr.c_str());

			_copyToClipboardLink.init(_hInst, _hSelf);
			_copyToClipboardLink.create(::GetDlgItem(_hSelf, IDC_DEBUGINFO_COPYLINK), IDC_DEBUGINFO_COPYLINK);

			getClientRect(_rc);
			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDCANCEL:
				case IDOK:
					display(false);
					return TRUE;

				case IDC_DEBUGINFO_COPYLINK:
				{
					if ((GetKeyState(VK_LBUTTON) & 0x100) != 0)
					{
						// Visual effect
						::SendDlgItemMessage(_hSelf, IDC_DEBUGINFO_EDIT, EM_SETSEL, 0, _debugInfoStr.length() - 1);

						// Copy to clipboard
						str2Clipboard(_debugInfoStr, _hSelf);
					}
					return TRUE;
				}
				default:
					break;
			}
		}

		case WM_DESTROY:
		{
			return TRUE;
		}
	}
	return FALSE;
}

void DebugInfoDlg::doDialog()
{
	if (!isCreated())
		create(IDD_DEBUGINFOBOX);

	// Adjust the position of AboutBox
	goToCenter();
}

