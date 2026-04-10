/*
 * This file is part of ComparePlus plugin for Notepad++
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <windows.h>
#include <wchar.h>
#include <shellapi.h>
#include <cstdlib>
#include <string>

#include "AboutDialog.h"
#include "resource.h"
#include "DockingFeature/Window.h"

#include "NppHelpers.h"
#include "Tools.h"
#include "Strings.h"


static const wchar_t cAuthor[]		= L"Pavel Nedev";
static const wchar_t cDonate_URL[]	= L"https://www.paypal.com/paypalme/pnedev";
static const wchar_t cRepo_URL[]	= L"https://github.com/pnedev/comparePlus";
static const wchar_t cGuide_URL[]	= L"https://github.com/pnedev/comparePlus/blob/master/Help.md";


UINT AboutDialog::doDialog()
{
	return (UINT)::DialogBoxParamW(_hInst, MAKEINTRESOURCEW(IDD_ABOUT_DIALOG), _hParent,
			(DLGPROC)dlgProc, (LPARAM)this);
}


INT_PTR CALLBACK AboutDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM /*lParam*/)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			registerDlgForDarkMode(_hSelf);

			goToCenter();

			const auto& str = Strings::get();

			// Dialog opens by default in english
			if (str.currentLocale() != "english")
				updateLocalization();

			std::wstring txt = str["IDC_VERSION"];
			txt += L" ";
			txt += MBtoWC(TO_STR(PLUGIN_VERSION));

			::SetDlgItemTextW(_hSelf, IDC_VERSION, txt.c_str());

			txt = str["IDC_BUILD_TIME"];
			txt += L":  ";
			txt += MBtoWC(__DATE__);
			txt += L", ";
			txt += MBtoWC(__TIME__);

			::SetDlgItemTextW(_hSelf, IDC_BUILD_TIME, txt.c_str());

			txt = str["IDC_AUTHOR"];
			txt += L":  ";
			txt += cAuthor;

			::SetDlgItemTextW(_hSelf, IDC_AUTHOR, txt.c_str());

			txt = L"LibGit2 ";
			txt += str["IDC_VERSION"];
			txt += L":  ";

			if (_libGit2Ver.empty())
				txt += str["LIB_NOT_FOUND"];
			else
				txt += _libGit2Ver;

			::SetDlgItemTextW(_hSelf, IDC_GITLIB_VER, txt.c_str());

			txt = L"SQLite3 ";
			txt += str["IDC_VERSION"];
			txt += L":  ";

			if (_sqlite3Ver.empty())
				txt += str["LIB_NOT_FOUND"];
			else
				txt += _sqlite3Ver;

			::SetDlgItemTextW(_hSelf, IDC_SQLITE3_VER, txt.c_str());

			COLORREF linkColor = ::GetSysColor(COLOR_HOTLIGHT);

			if (isDarkMode())
			{
				auto dmColors = getNppDarkModeColors();
				if (dmColors)
					linkColor = dmColors->linkText;
			}

			_urlRepo.init(_hInst, _hSelf);
			_urlRepo.create(::GetDlgItem(_hSelf, IDC_REPO_URL), cRepo_URL, linkColor);
			_helpLink.init(_hInst, _hSelf);
			_helpLink.create(::GetDlgItem(_hSelf, IDC_GUIDE_URL), cGuide_URL, linkColor);

			redraw(true);

			return TRUE;
		}
		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDC_CLOSE:
				case IDCANCEL:
					::EndDialog(_hSelf, 0);
				return TRUE;

				case IDC_DONATE:
					::ShellExecuteW(NULL, L"open", cDonate_URL, NULL, NULL, SW_SHOWNORMAL);
				return TRUE;

				default:
				break;
			}
			break;
		}
	}

	return FALSE;
}


void AboutDialog::updateLocalization()
{
	const auto& str = Strings::get();

	::SetDlgItemTextW(_hSelf, IDC_CLOSE,		str["IDC_CLOSE"].c_str());
	::SetDlgItemTextW(_hSelf, IDC_DONATE,		str["IDC_DONATE"].c_str());
	::SetDlgItemTextW(_hSelf, IDC_INFO,			str["IDC_INFO"].c_str());
	::SetDlgItemTextW(_hSelf, IDC_LIBS,			str["IDC_LIBS"].c_str());
	::SetDlgItemTextW(_hSelf, IDC_LINKS,		str["IDC_LINKS"].c_str());
	::SetDlgItemTextW(_hSelf, IDC_REPO_URL,		str["IDC_REPO_URL"].c_str());
	::SetDlgItemTextW(_hSelf, IDC_GUIDE_URL,	str["IDC_GUIDE_URL"].c_str());
}
