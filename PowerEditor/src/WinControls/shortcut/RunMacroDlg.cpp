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

// created by Daniel Volk mordorpost@volkarts.com


#include "RunMacroDlg.h"
#include "ScintillaEditView.h"
#include "Notepad_plus_msgs.h"

void RunMacroDlg::initMacroList()
{
	if (!isCreated()) return;

	NppParameters& nppParam = NppParameters::getInstance();
	std::vector<MacroShortcut> & macroList = nppParam.getMacroList();

	::SendDlgItemMessage(_hSelf, IDC_MACRO_COMBO, CB_RESETCONTENT, 0, 0);

	if (static_cast<MacroStatus>(::SendMessage(_hParent, NPPM_GETCURRENTMACROSTATUS, 0, 0)) == MacroStatus::RecordingStopped)
		::SendDlgItemMessage(_hSelf, IDC_MACRO_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("Current recorded macro")));

	for (size_t i = 0, len = macroList.size(); i < len ; ++i)
		::SendDlgItemMessage(_hSelf, IDC_MACRO_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(macroList[i].getName()));

	::SendDlgItemMessage(_hSelf, IDC_MACRO_COMBO, CB_SETCURSEL, 0, 0);
	_macroIndex = 0;
}

intptr_t CALLBACK RunMacroDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{	
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			initMacroList();
			::SetDlgItemInt(_hSelf, IDC_M_RUN_TIMES, _times, FALSE);
			switch (_mode)
			{
				case RM_RUN_MULTI:
					check(IDC_M_RUN_MULTI);
					break;
				case RM_RUN_EOF:
					check(IDC_M_RUN_EOF);
					break;
			}
			::SendDlgItemMessage(_hSelf, IDC_M_RUN_TIMES, EM_LIMITTEXT, 4, 0);
			goToCenter();

			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORLISTBOX:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::autoThemeChildControls(_hSelf);
			return TRUE;
		}

		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				switch (LOWORD(wParam))
				{	
					case IDC_M_RUN_TIMES:
						check(IDC_M_RUN_MULTI);
						return TRUE;
					
					default:
						return FALSE;
				}
			}
			
			switch (wParam)
			{
				case IDCANCEL :
					::ShowWindow(_hSelf, SW_HIDE);
					return TRUE;

				case IDOK :
					if ( isCheckedOrNot(IDC_M_RUN_MULTI) )
					{
						_mode = RM_RUN_MULTI;
						_times = ::GetDlgItemInt(_hSelf, IDC_M_RUN_TIMES, NULL, FALSE);
					}
					else if ( isCheckedOrNot(IDC_M_RUN_EOF) )
					{
						_mode = RM_RUN_EOF;
					}

					if (::SendDlgItemMessage(_hSelf, IDC_MACRO_COMBO, CB_GETCOUNT, 0, 0))
						::SendMessage(_hParent, WM_MACRODLGRUNMACRO, 0, 0);

					return TRUE;

				default:
					if ((HIWORD(wParam) == CBN_SELCHANGE) && (LOWORD(wParam) == IDC_MACRO_COMBO))
					{
						_macroIndex = static_cast<int32_t>(::SendDlgItemMessage(_hSelf, IDC_MACRO_COMBO, CB_GETCURSEL, 0, 0));
						return TRUE;
					}
			}
		}
	}
	return FALSE;
}

void RunMacroDlg::check(int id)
{
	// IDC_M_RUN_MULTI
	if ( id == IDC_M_RUN_MULTI )
		::SendDlgItemMessage(_hSelf, IDC_M_RUN_MULTI, BM_SETCHECK, BST_CHECKED, 0);
	else
		::SendDlgItemMessage(_hSelf, IDC_M_RUN_MULTI, BM_SETCHECK, BST_UNCHECKED, 0);

	// IDC_M_RUN_EOF
	if ( id == IDC_M_RUN_EOF )
		::SendDlgItemMessage(_hSelf, IDC_M_RUN_EOF, BM_SETCHECK, BST_CHECKED, 0);
	else
		::SendDlgItemMessage(_hSelf, IDC_M_RUN_EOF, BM_SETCHECK, BST_UNCHECKED, 0);
}

int RunMacroDlg::getMacro2Exec() const 
{
	bool isCurMacroPresent = static_cast<MacroStatus>(::SendMessage(_hParent, NPPM_GETCURRENTMACROSTATUS, 0, 0)) == MacroStatus::RecordingStopped;
	return isCurMacroPresent?(_macroIndex - 1):_macroIndex;
}
