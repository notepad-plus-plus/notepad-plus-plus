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

#pragma comment (lib, "uxtheme")


#include "VisualFiltersDialog.h"

#include <windowsx.h>
#include <commctrl.h>
#include <uxtheme.h>

#include "NppHelpers.h"
#include "Strings.h"
#include "Tools.h"


UINT VisualFiltersDialog::doDialog(UserSettings* settings)
{
	_Settings = settings;

	return (UINT)::DialogBoxParamW(_hInst, MAKEINTRESOURCEW(IDD_VISUAL_FILTERS_DIALOG), _hParent,
			(DLGPROC)dlgProc, (LPARAM)this);
}


INT_PTR CALLBACK VisualFiltersDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			registerDlgForDarkMode(_hSelf);

			goToCenter();

			::EnableThemeDialogTexture(_hSelf, ETDT_ENABLETAB);

			// Dialog opens by default in english
			if (Strings::get().currentLocale() != "english")
				updateLocalization();

			SetParams();
		}
		return TRUE;

		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDOK:
					GetParams();
					_Settings->markAsDirty();
					::EndDialog(_hSelf, IDOK);
				return TRUE;

				case IDCANCEL:
					::EndDialog(_hSelf, IDCANCEL);
				return TRUE;

				case IDC_HIDE_ALL_DIFFS:
				{
					const bool hideAllDiffs = Button_GetCheck(::GetDlgItem(_hSelf, IDC_HIDE_ALL_DIFFS)) == BST_CHECKED;

					Button_SetCheck(::GetDlgItem(_hSelf, IDC_HIDE_NEW_LINES),
							hideAllDiffs ? BST_CHECKED : BST_UNCHECKED);
					Button_SetCheck(::GetDlgItem(_hSelf, IDC_HIDE_CHANGED_LINES),
							hideAllDiffs ? BST_CHECKED : BST_UNCHECKED);
					Button_SetCheck(::GetDlgItem(_hSelf, IDC_HIDE_MOVED_LINES),
							hideAllDiffs ? BST_CHECKED : BST_UNCHECKED);

					Button_Enable(::GetDlgItem(_hSelf, IDC_HIDE_MATCHES), !hideAllDiffs);
				}
				return TRUE;

				case IDC_HIDE_NEW_LINES:
				case IDC_HIDE_CHANGED_LINES:
				case IDC_HIDE_MOVED_LINES:
				{
					const bool hideAllDiffs =
							Button_GetCheck(::GetDlgItem(_hSelf, IDC_HIDE_NEW_LINES)) == BST_CHECKED &&
							Button_GetCheck(::GetDlgItem(_hSelf, IDC_HIDE_CHANGED_LINES)) == BST_CHECKED &&
							Button_GetCheck(::GetDlgItem(_hSelf, IDC_HIDE_MOVED_LINES)) == BST_CHECKED;

					Button_SetCheck(::GetDlgItem(_hSelf, IDC_HIDE_ALL_DIFFS),
							hideAllDiffs ? BST_CHECKED : BST_UNCHECKED);

					Button_Enable(::GetDlgItem(_hSelf, IDC_HIDE_MATCHES), !hideAllDiffs);
				}
				return TRUE;

				default:
				return FALSE;
			}
		}
		break;
	}

	return FALSE;
}


void VisualFiltersDialog::updateLocalization()
{
	const auto& str = Strings::get();

	::SetWindowTextW(_hSelf, (std::wstring(PLUGIN_NAME) + std::wstring(L"   ") + str["VISUAL_FILTERS"]).c_str());

	::SetDlgItemTextW(_hSelf, IDOK,			str["IDOK"].c_str());
	::SetDlgItemTextW(_hSelf, IDCANCEL,		str["IDCANCEL"].c_str());
	::SetDlgItemTextW(_hSelf, IDC_FILTERS,	str["IDC_FILTERS"].c_str());

	updateDlgCtrlTxt(_hSelf, IDC_NOTE,	str["IDC_NOTE"].c_str());

	updateDlgCtrlTxt(_hSelf, IDC_HIDE_MATCHES,			str["IDC_HIDE_MATCHES"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_HIDE_ALL_DIFFS,		str["IDC_HIDE_ALL_DIFFS"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_HIDE_NEW_LINES,		str["IDC_HIDE_NEW_LINES"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_HIDE_CHANGED_LINES,	str["IDC_HIDE_CHANGED_LINES"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_HIDE_MOVED_LINES,		str["IDC_HIDE_MOVED_LINES"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_SHOW_ONLY_SELECTIONS,	str["IDC_SHOW_ONLY_SELECTIONS"].c_str(), true);
}


void VisualFiltersDialog::SetParams()
{
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_HIDE_MATCHES), _Settings->HideMatches ? BST_CHECKED : BST_UNCHECKED);

	const bool hideAllDiffs = _Settings->HideNewLines && _Settings->HideChangedLines && _Settings->HideMovedLines;

	Button_SetCheck(::GetDlgItem(_hSelf, IDC_HIDE_ALL_DIFFS), hideAllDiffs ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_HIDE_NEW_LINES), _Settings->HideNewLines ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_HIDE_CHANGED_LINES),
			_Settings->HideChangedLines ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_HIDE_MOVED_LINES),
			_Settings->HideMovedLines ? BST_CHECKED : BST_UNCHECKED);

	Button_Enable(::GetDlgItem(_hSelf, IDC_HIDE_MATCHES), !hideAllDiffs);

	Button_SetCheck(::GetDlgItem(_hSelf, IDC_SHOW_ONLY_SELECTIONS),
			_Settings->ShowOnlySelections ? BST_CHECKED : BST_UNCHECKED);
}


void VisualFiltersDialog::GetParams()
{
	if (::IsWindowEnabled(::GetDlgItem(_hSelf, IDC_HIDE_MATCHES)))
		_Settings->HideMatches		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_HIDE_MATCHES)) == BST_CHECKED);
	else
		_Settings->HideMatches		= false;

	_Settings->HideNewLines			= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_HIDE_NEW_LINES)) == BST_CHECKED);
	_Settings->HideChangedLines		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_HIDE_CHANGED_LINES)) == BST_CHECKED);
	_Settings->HideMovedLines		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_HIDE_MOVED_LINES)) == BST_CHECKED);

	_Settings->ShowOnlySelections	= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_SHOW_ONLY_SELECTIONS)) == BST_CHECKED);
}
