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


#include "CompareOptionsDialog.h"

#include <string>
#include <windowsx.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <boost/regex.hpp>

#include "NppHelpers.h"
#include "Strings.h"
#include "Tools.h"


UINT CompareOptionsDialog::doDialog(UserSettings* settings)
{
	_Settings = settings;

	return (UINT)::DialogBoxParamW(_hInst, MAKEINTRESOURCEW(IDD_COMPARE_OPTIONS_DIALOG), _hParent,
			(DLGPROC)dlgProc, (LPARAM)this);
}


INT_PTR CALLBACK CompareOptionsDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM)
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
					if (GetParams())
					{
						_Settings->markAsDirty();
						::EndDialog(_hSelf, IDOK);

						return TRUE;
					}
				break;

				case IDCANCEL:
					::EndDialog(_hSelf, IDCANCEL);
				return TRUE;

				case IDC_DETECT_SUB_BLOCK_DIFFS:
				{
					bool subBlockDiffs =
							(Button_GetCheck(::GetDlgItem(_hSelf, IDC_DETECT_SUB_BLOCK_DIFFS)) == BST_CHECKED);

					Button_Enable(::GetDlgItem(_hSelf, IDC_DETECT_SUB_LINE_MOVES), subBlockDiffs);
					Button_Enable(::GetDlgItem(_hSelf, IDC_DETECT_CHAR_DIFFS), subBlockDiffs);
				}
				return TRUE;

				case IDC_IGNORE_CHANGED_SPACES:
				{
					bool ignoreChangedSpaces =
							(Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_CHANGED_SPACES)) == BST_CHECKED);

					if (ignoreChangedSpaces)
						Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_ALL_SPACES), BST_UNCHECKED);
				}
				return TRUE;

				case IDC_IGNORE_ALL_SPACES:
				{
					bool ignoreAllSpaces =
							(Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_ALL_SPACES)) == BST_CHECKED);

					if (ignoreAllSpaces)
						Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_CHANGED_SPACES), BST_UNCHECKED);
				}
				return TRUE;

				case IDC_IGNORE_REGEX:
				{
					const bool ignoreRegex = (Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_REGEX)) == BST_CHECKED);

					Button_Enable(::GetDlgItem(_hSelf, IDC_HIGHLIGHT_REGEX_IGNORES),	ignoreRegex);
					Button_Enable(::GetDlgItem(_hSelf, IDC_REGEX_MODE_IGNORE),			ignoreRegex);
					Button_Enable(::GetDlgItem(_hSelf, IDC_REGEX_MODE_MATCH),			ignoreRegex);
					Button_Enable(::GetDlgItem(_hSelf, IDC_REGEX_INCL_NOMATCH_LINES),	ignoreRegex &&
							(Button_GetCheck(::GetDlgItem(_hSelf, IDC_REGEX_MODE_MATCH)) == BST_CHECKED));

					ComboBox_Enable(::GetDlgItem(_hSelf, IDC_IGNORE_REGEX_STR), ignoreRegex);
				}
				return TRUE;

				case IDC_REGEX_MODE_IGNORE:
				case IDC_REGEX_MODE_MATCH:
					Button_Enable(::GetDlgItem(_hSelf, IDC_REGEX_INCL_NOMATCH_LINES),
							(Button_GetCheck(::GetDlgItem(_hSelf, IDC_REGEX_MODE_MATCH)) == BST_CHECKED));
				return TRUE;

				default:
				return FALSE;
			}
		}
		break;
	}

	return FALSE;
}


void CompareOptionsDialog::updateLocalization()
{
	const auto& str = Strings::get();

	::SetWindowTextW(_hSelf, (std::wstring(PLUGIN_NAME) + std::wstring(L"   ") + str["COMPARE_OPTIONS"]).c_str());

	::SetDlgItemTextW(_hSelf, IDOK,			str["IDOK"].c_str());
	::SetDlgItemTextW(_hSelf, IDCANCEL,		str["IDCANCEL"].c_str());
	::SetDlgItemTextW(_hSelf, IDC_DETECT,	str["IDC_DETECT"].c_str());
	::SetDlgItemTextW(_hSelf, IDC_IGNORE,	str["IDC_IGNORE"].c_str());
	::SetDlgItemTextW(_hSelf, IDC_REGEX,	str["IDC_REGEX"].c_str());

	updateDlgCtrlTxt(_hSelf, IDC_DETECT_MOVES,				str["IDC_DETECT_MOVES"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_DETECT_SUB_BLOCK_DIFFS,	str["IDC_DETECT_SUB_BLOCK_DIFFS"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_DETECT_SUB_LINE_MOVES,		str["IDC_DETECT_SUB_LINE_MOVES"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_DETECT_CHAR_DIFFS,			str["IDC_DETECT_CHAR_DIFFS"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_IGNORE_EMPTY_LINES,		str["IDC_IGNORE_EMPTY_LINES"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_IGNORE_FOLDED_LINES,		str["IDC_IGNORE_FOLDED_LINES"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_IGNORE_HIDDEN_LINES,		str["IDC_IGNORE_HIDDEN_LINES"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_IGNORE_CHANGED_SPACES,		str["IDC_IGNORE_CHANGED_SPACES"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_IGNORE_ALL_SPACES,			str["IDC_IGNORE_ALL_SPACES"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_IGNORE_EOL,				str["IDC_IGNORE_EOL"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_IGNORE_CASE,				str["IDC_IGNORE_CASE"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_IGNORE_REGEX,				str["IDC_IGNORE_REGEX"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_REGEX_MODE_IGNORE,			str["IDC_REGEX_MODE_IGNORE"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_REGEX_MODE_MATCH,			str["IDC_REGEX_MODE_MATCH"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_REGEX_INCL_NOMATCH_LINES,	str["IDC_REGEX_INCL_NOMATCH_LINES"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_HIGHLIGHT_REGEX_IGNORES,	str["IDC_HIGHLIGHT_REGEX_IGNORES"].c_str(), true);
}


void CompareOptionsDialog::SetParams()
{
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_DETECT_MOVES), _Settings->DetectMoves ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_DETECT_SUB_BLOCK_DIFFS),
			_Settings->DetectSubBlockDiffs ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_DETECT_SUB_LINE_MOVES),
			_Settings->DetectSubLineMoves ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_DETECT_CHAR_DIFFS),
			_Settings->DetectCharDiffs ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_EMPTY_LINES),
			_Settings->IgnoreEmptyLines ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_FOLDED_LINES),
			_Settings->IgnoreFoldedLines ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_HIDDEN_LINES),
			_Settings->IgnoreHiddenLines ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_CHANGED_SPACES),
			!_Settings->IgnoreAllSpaces && _Settings->IgnoreChangedSpaces ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_ALL_SPACES),
			_Settings->IgnoreAllSpaces ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_EOL),	_Settings->IgnoreEOL ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_CASE),  _Settings->IgnoreCase  ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_REGEX), _Settings->IgnoreRegex ? BST_CHECKED : BST_UNCHECKED);

	Button_SetCheck(::GetDlgItem(_hSelf, IDC_REGEX_MODE_IGNORE), !_Settings->InvertRegex ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_REGEX_MODE_MATCH),   _Settings->InvertRegex ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_REGEX_INCL_NOMATCH_LINES),
			_Settings->InclRegexNomatchLines ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_HIGHLIGHT_REGEX_IGNORES),
			_Settings->HighlightRegexIgnores ? BST_CHECKED : BST_UNCHECKED);

	Button_Enable(::GetDlgItem(_hSelf, IDC_DETECT_SUB_LINE_MOVES),		_Settings->DetectSubBlockDiffs);
	Button_Enable(::GetDlgItem(_hSelf, IDC_DETECT_CHAR_DIFFS),			_Settings->DetectSubBlockDiffs);
	Button_Enable(::GetDlgItem(_hSelf, IDC_REGEX_MODE_IGNORE),			_Settings->IgnoreRegex);
	Button_Enable(::GetDlgItem(_hSelf, IDC_REGEX_MODE_MATCH),			_Settings->IgnoreRegex);
	Button_Enable(::GetDlgItem(_hSelf, IDC_REGEX_INCL_NOMATCH_LINES),
			_Settings->IgnoreRegex && _Settings->InvertRegex);
	Button_Enable(::GetDlgItem(_hSelf, IDC_HIGHLIGHT_REGEX_IGNORES),	_Settings->IgnoreRegex);

	HWND hCtrl = ::GetDlgItem(_hSelf, IDC_IGNORE_REGEX_STR);

	for (const auto& regexStr : _Settings->IgnoreRegexStr)
	{
		if (regexStr.empty())
			break;

		ComboBox_AddString(hCtrl, regexStr.c_str());
	}

	ComboBox_LimitText(hCtrl, UserSettings::cMaxRegexLen);
	ComboBox_SetText(hCtrl, _Settings->IgnoreRegexStr[0].c_str());
	ComboBox_Enable(hCtrl, _Settings->IgnoreRegex);
}


bool CompareOptionsDialog::GetParams()
{
	_Settings->DetectMoves			= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_DETECT_MOVES)) == BST_CHECKED);
	_Settings->DetectSubBlockDiffs	=
			(Button_GetCheck(::GetDlgItem(_hSelf, IDC_DETECT_SUB_BLOCK_DIFFS)) == BST_CHECKED);
	_Settings->DetectSubLineMoves	=
			(Button_GetCheck(::GetDlgItem(_hSelf, IDC_DETECT_SUB_LINE_MOVES)) == BST_CHECKED);
	_Settings->DetectCharDiffs		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_DETECT_CHAR_DIFFS)) == BST_CHECKED);
	_Settings->IgnoreEmptyLines		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_EMPTY_LINES)) == BST_CHECKED);
	_Settings->IgnoreFoldedLines	= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_FOLDED_LINES)) == BST_CHECKED);
	_Settings->IgnoreHiddenLines	= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_HIDDEN_LINES)) == BST_CHECKED);
	_Settings->IgnoreChangedSpaces	= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_CHANGED_SPACES)) == BST_CHECKED);
	_Settings->IgnoreAllSpaces		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_ALL_SPACES)) == BST_CHECKED);
	_Settings->IgnoreEOL			= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_EOL)) == BST_CHECKED);
	_Settings->IgnoreCase			= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_CASE)) == BST_CHECKED);
	_Settings->IgnoreRegex			= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_IGNORE_REGEX)) == BST_CHECKED);

	if (_Settings->IgnoreRegex)
	{
		HWND hCtrl = ::GetDlgItem(_hSelf, IDC_IGNORE_REGEX_STR);

		int len = ComboBox_GetTextLength(hCtrl);

		std::wstring newRegex;

		if (len > 0)
		{
			newRegex.resize(len);

			ComboBox_GetText(hCtrl, &newRegex[0], len + 1);

			if (!isRegexValid(newRegex.c_str()))
			{
				::SetFocus(hCtrl);

				return false;
			}

			_Settings->moveRegexToHistory(std::move(newRegex));
		}
	}

	_Settings->InvertRegex = (Button_GetCheck(::GetDlgItem(_hSelf, IDC_REGEX_MODE_MATCH)) == BST_CHECKED);
	_Settings->InclRegexNomatchLines =
				(Button_GetCheck(::GetDlgItem(_hSelf, IDC_REGEX_INCL_NOMATCH_LINES)) == BST_CHECKED);
	_Settings->HighlightRegexIgnores =
			(Button_GetCheck(::GetDlgItem(_hSelf, IDC_HIGHLIGHT_REGEX_IGNORES)) == BST_CHECKED);

	return true;
}


bool CompareOptionsDialog::isRegexValid(const wchar_t* regexStr)
{
	try
	{
		boost::wregex testRegex(regexStr, boost::regex::perl);
	}
	catch (boost::regex_error& err)
	{
		::MessageBoxA(nppData._nppHandle, err.what(), "ComparePlus Bad Regex", MB_OK | MB_ICONWARNING);

		return false;
	}

	return true;
}
