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



#include <shlobj.h>
#include <uxtheme.h>

#include "AboutDlg.h"
#include "Parameters.h"
#include "localization.h"

#pragma warning(disable : 4996) // for GetVersion()

INT_PTR CALLBACK AboutDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
        case WM_INITDIALOG :
		{
			HWND compileDateHandle = ::GetDlgItem(_hSelf, IDC_BUILD_DATETIME);
			generic_string buildTime = TEXT("Build time : ");

			WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
			buildTime +=  wmc.char2wchar(__DATE__, CP_ACP);
			buildTime += TEXT(" - ");
			buildTime +=  wmc.char2wchar(__TIME__, CP_ACP);

			NppParameters& nppParam = NppParameters::getInstance();
			LPCTSTR bitness = nppParam.isx64() ? TEXT("(64-bit)") : TEXT("(32-bit)");
			::SetDlgItemText(_hSelf, IDC_VERSION_BIT, bitness);

			::SendMessage(compileDateHandle, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buildTime.c_str()));
			::EnableWindow(compileDateHandle, FALSE);

            HWND licenceEditHandle = ::GetDlgItem(_hSelf, IDC_LICENCE_EDIT);
			::SendMessage(licenceEditHandle, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(LICENCE_TXT));

            //_emailLink.init(_hInst, _hSelf);
			//_emailLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), TEXT("mailto:don.h@free.fr"));
			//_emailLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), TEXT("https://notepad-plus-plus.org/news/v781-free-uyghur-edition/"));
			//_emailLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), TEXT("https://notepad-plus-plus.org/news/v792-stand-with-hong-kong/"));
			//_emailLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), TEXT("https://notepad-plus-plus.org/news/v791-pour-samuel-paty/"));

            _pageLink.init(_hInst, _hSelf);
            _pageLink.create(::GetDlgItem(_hSelf, IDC_HOME_ADDR), TEXT("https://notepad-plus-plus.org/"));

			getClientRect(_rc);

			ETDTProc enableDlgTheme = (ETDTProc)nppParam.getEnableThemeDlgTexture();
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
			//HICON hIcon = (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_GILETJAUNE), IMAGE_ICON, 64, 64, LR_DEFAULTSIZE);
			//HICON hIcon = (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_SAMESEXMARRIAGE), IMAGE_ICON, 64, 64, LR_DEFAULTSIZE);
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
			NppParameters& nppParam = NppParameters::getInstance();

			// Notepad++ version
			_debugInfoStr = NOTEPAD_PLUS_VERSION;
			_debugInfoStr += nppParam.isx64() ? TEXT("   (64-bit)") : TEXT("   (32-bit)");
			_debugInfoStr += TEXT("\r\n");

			// Build time
			_debugInfoStr += TEXT("Build time : ");
			generic_string buildTime;
			WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
			buildTime += wmc.char2wchar(__DATE__, CP_ACP);
			buildTime += TEXT(" - ");
			buildTime += wmc.char2wchar(__TIME__, CP_ACP);
			_debugInfoStr += buildTime;
			_debugInfoStr += TEXT("\r\n");

			// Binary path
			_debugInfoStr += TEXT("Path : ");
			TCHAR nppFullPath[MAX_PATH];
			::GetModuleFileName(NULL, nppFullPath, MAX_PATH);
			_debugInfoStr += nppFullPath;
			_debugInfoStr += TEXT("\r\n");

			// Command line as specified for program launch
			_debugInfoStr += TEXT("Command Line : ");
			_debugInfoStr += nppParam.getCmdLineString();
			_debugInfoStr += TEXT("\r\n");

			// Administrator mode
			_debugInfoStr += TEXT("Admin mode : ");
			_debugInfoStr += (_isAdmin ? TEXT("ON") : TEXT("OFF"));
			_debugInfoStr += TEXT("\r\n");

			// local conf
			_debugInfoStr += TEXT("Local Conf mode : ");
			bool doLocalConf = (NppParameters::getInstance()).isLocal();
			_debugInfoStr += (doLocalConf ? TEXT("ON") : TEXT("OFF"));
			_debugInfoStr += TEXT("\r\n");

			// Cloud config directory
			_debugInfoStr += TEXT("Cloud Config : ");
			const generic_string& cloudPath = nppParam.getNppGUI()._cloudPath;
			_debugInfoStr += cloudPath.empty() ? _T("OFF") : cloudPath;
			_debugInfoStr += TEXT("\r\n");

			// OS information
			HKEY hKey;
			DWORD dataSize = 0;
			
			TCHAR szProductName[96] = {'\0'};
			TCHAR szCurrentBuildNumber[32] = {'\0'};
			TCHAR szReleaseId[32] = {'\0'};
			DWORD dwUBR = 0;
			TCHAR szUBR[12] = TEXT("0");

			// NOTE: RegQueryValueExW is not guaranteed to return null-terminated strings
			if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
			{
				dataSize = sizeof(szProductName);
				RegQueryValueExW(hKey, TEXT("ProductName"), NULL, NULL, reinterpret_cast<LPBYTE>(szProductName), &dataSize);
				szProductName[sizeof(szProductName) / sizeof(TCHAR) - 1] = '\0';

				dataSize = sizeof(szReleaseId);
				RegQueryValueExW(hKey, TEXT("ReleaseId"), NULL, NULL, reinterpret_cast<LPBYTE>(szReleaseId), &dataSize);
				szReleaseId[sizeof(szReleaseId) / sizeof(TCHAR) - 1] = '\0';
				
				dataSize = sizeof(szCurrentBuildNumber);
				RegQueryValueExW(hKey, TEXT("CurrentBuildNumber"), NULL, NULL, reinterpret_cast<LPBYTE>(szCurrentBuildNumber), &dataSize);
				szCurrentBuildNumber[sizeof(szCurrentBuildNumber) / sizeof(TCHAR) - 1] = '\0';
				
				dataSize = sizeof(DWORD);
				if (RegQueryValueExW(hKey, TEXT("UBR"), NULL, NULL, reinterpret_cast<LPBYTE>(&dwUBR), &dataSize) == ERROR_SUCCESS)
				{
					generic_sprintf(szUBR, TEXT("%u"), dwUBR);
				}
				
				RegCloseKey(hKey);
			}

			// Get alternative OS information
			if (szProductName[0] == '\0')
			{
				generic_sprintf(szProductName, TEXT("%s"), (NppParameters::getInstance()).getWinVersionStr().c_str());
			}
			if (szCurrentBuildNumber[0] == '\0')
			{
				DWORD dwVersion = GetVersion();
				if (dwVersion < 0x80000000)
				{
					generic_sprintf(szCurrentBuildNumber, TEXT("%u"), HIWORD(dwVersion));
				}
			}
			
			_debugInfoStr += TEXT("OS Name : ");
			_debugInfoStr += szProductName;
			_debugInfoStr += TEXT(" (");
			_debugInfoStr += (NppParameters::getInstance()).getWinVerBitStr();
			_debugInfoStr += TEXT(") ");
			_debugInfoStr += TEXT("\r\n");
			
			if (szReleaseId[0] != '\0')
			{
				_debugInfoStr += TEXT("OS Version : ");
				_debugInfoStr += szReleaseId;
				_debugInfoStr += TEXT("\r\n");
			}

			if (szCurrentBuildNumber[0] != '\0')
			{
				_debugInfoStr += TEXT("OS Build : ");
				_debugInfoStr += szCurrentBuildNumber;
				_debugInfoStr += TEXT(".");
				_debugInfoStr += szUBR;
				_debugInfoStr += TEXT("\r\n");
			}

			{
				TCHAR szACP[32];
				generic_sprintf(szACP, TEXT("%u"), ::GetACP());
				_debugInfoStr += TEXT("Current ANSI codepage : ");
 				_debugInfoStr += szACP;
				_debugInfoStr += TEXT("\r\n");
			}

			// Detect WINE
			PWINEGETVERSION pWGV = (PWINEGETVERSION)GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), "wine_get_version");
			if (pWGV != NULL)
			{
				TCHAR szWINEVersion[32];
				generic_sprintf(szWINEVersion, TEXT("%hs"), pWGV());

				_debugInfoStr += TEXT("WINE : ");
				_debugInfoStr += szWINEVersion;
				_debugInfoStr += TEXT("\r\n");
			}

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

void DoSaveOrNotBox::doDialog(bool isRTL)
{
	
	if (isRTL)
	{
		DLGTEMPLATE *pMyDlgTemplate = NULL;
		HGLOBAL hMyDlgTemplate = makeRTLResource(IDD_DOSAVEORNOTBOX, &pMyDlgTemplate);
		::DialogBoxIndirectParam(_hInst, pMyDlgTemplate, _hParent, dlgProc, reinterpret_cast<LPARAM>(this));
		::GlobalFree(hMyDlgTemplate);
	}
	else
		::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_DOSAVEORNOTBOX), _hParent, dlgProc, reinterpret_cast<LPARAM>(this));
}

void DoSaveOrNotBox::changeLang()
{
	generic_string msg;
	generic_string defaultMessage = TEXT("Save file \"$STR_REPLACE$\" ?");
	NativeLangSpeaker* nativeLangSpeaker = NppParameters::getInstance().getNativeLangSpeaker();

	if (nativeLangSpeaker->changeDlgLang(_hSelf, "DoSaveOrNot"))
	{
		const unsigned char len = 255;
		TCHAR text[len];
		::GetDlgItemText(_hSelf, IDC_DOSAVEORNOTTEX, text, len);
		msg = text;
	}

	if (msg.empty())
		msg = defaultMessage;

	msg = stringReplace(msg, TEXT("$STR_REPLACE$"), _fn);
	::SetDlgItemText(_hSelf, IDC_DOSAVEORNOTTEX, msg.c_str());
}

INT_PTR CALLBACK DoSaveOrNotBox::run_dlgProc(UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	switch (message)
	{
		case WM_INITDIALOG :
		{
			changeLang();
			::EnableWindow(::GetDlgItem(_hSelf, IDRETRY), _isMulti);
			::EnableWindow(::GetDlgItem(_hSelf, IDIGNORE), _isMulti);
			goToCenter();
			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
				{
					::EndDialog(_hSelf, -1);
					clickedButtonId = IDCANCEL;
					return TRUE;
				}

				case IDYES:
				{
					::EndDialog(_hSelf, 0);
					clickedButtonId = IDYES;
					return TRUE;
				}

				case IDNO:
				{
					::EndDialog(_hSelf, 0);
					clickedButtonId = IDNO;
					return TRUE;
				}

				case IDIGNORE:
				{
					::EndDialog(_hSelf, 0);
					clickedButtonId = IDIGNORE;
					return TRUE;
				}

				case IDRETRY:
				{
					::EndDialog(_hSelf, 0);
					clickedButtonId = IDRETRY;
					return TRUE;
				}
			}
		}
		default:
			return FALSE;
	}
}