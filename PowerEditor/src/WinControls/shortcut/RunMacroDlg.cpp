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
		::SendDlgItemMessage(_hSelf, IDC_MACRO_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Current recorded macro"));

	for (size_t i = 0, len = macroList.size(); i < len ; ++i)
		::SendDlgItemMessage(_hSelf, IDC_MACRO_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(string2wstring(macroList[i].getName(), CP_UTF8).c_str()));

	::SendDlgItemMessage(_hSelf, IDC_MACRO_COMBO, CB_SETCURSEL, 0, 0);
	_macroIndex = 0;
}

intptr_t CALLBACK RunMacroDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{	
	switch (message)
	{
		case WM_INITDIALOG :
		{
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			initMacroList();
			::SetDlgItemInt(_hSelf, IDC_M_RUN_TIMES, _times, FALSE);
			setChecked(IDC_M_RUN_MULTI);

			::SendDlgItemMessage(_hSelf, IDC_M_RUN_TIMES, EM_LIMITTEXT, 4, 0);
			goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);

			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			return NppDarkMode::onCtlColorCtrl(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORLISTBOX:
		{
			return NppDarkMode::onCtlColorListbox(wParam, lParam);
		}

		case WM_CTLCOLORDLG:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORSTATIC:
		{
			auto hdcStatic = reinterpret_cast<HDC>(wParam);
			const auto dlgCtrlID = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam));

			const bool isStaticText = (dlgCtrlID == IDC_TIMES_STATIC);
			//set the static text colors to show enable/disable instead of ::EnableWindow which causes blurry text
			if (isStaticText)
			{
				const bool isTextEnabled = isCheckedOrNot(IDC_M_RUN_MULTI);
				return NppDarkMode::onCtlColorDlgStaticText(hdcStatic, isTextEnabled);
			}
			return NppDarkMode::onCtlColorDlg(hdcStatic);
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

		case WM_CHANGEUISTATE:
		{
			if (NppDarkMode::isEnabled())
			{
				redrawDlgItem(IDC_MACRO2RUN_STATIC);
			}

			return FALSE;
		}

		case WM_DPICHANGED:
		{
			_dpiManager.setDpiWP(wParam);
			setPositionDpi(lParam);

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_M_RUN_MULTI:
				case IDC_M_RUN_EOF:
				{
					const bool isRunMulti = (wParam == IDC_M_RUN_MULTI);
					if (isRunMulti)
					{
						::EnableWindow(::GetDlgItem(_hSelf, IDC_M_RUN_TIMES), TRUE);
						_times = ::GetDlgItemInt(_hSelf, IDC_M_RUN_TIMES, NULL, FALSE);
					}
					else
					{
						::EnableWindow(::GetDlgItem(_hSelf, IDC_M_RUN_TIMES), FALSE);
					}
					redrawDlgItem(IDC_TIMES_STATIC);

					return TRUE;
				}

				case IDCANCEL:
					::ShowWindow(_hSelf, SW_HIDE);
					return TRUE;

				case IDOK:
				{
					if (::SendDlgItemMessage(_hSelf, IDC_MACRO_COMBO, CB_GETCOUNT, 0, 0) > 0)
						::SendMessage(_hParent, WM_MACRODLGRUNMACRO, 0, 0);

					return TRUE;
				}

				case IDC_MACRO_COMBO:
				{
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						_macroIndex = static_cast<int32_t>(::SendDlgItemMessage(_hSelf, IDC_MACRO_COMBO, CB_GETCURSEL, 0, 0));
						return TRUE;
					}
					return FALSE;
				}

				case IDC_M_RUN_TIMES:
				{
					switch (HIWORD(wParam))
					{
						case EN_KILLFOCUS:
						{
							const int times = ::GetDlgItemInt(_hSelf, IDC_M_RUN_TIMES, nullptr, FALSE);
							if (times < 1)
							{
								::SetDlgItemInt(_hSelf, IDC_M_RUN_TIMES, 1, FALSE);
								return TRUE;
							}

							return FALSE;
						}

						case EN_CHANGE:
						{
							_times = std::max<int>(::GetDlgItemInt(_hSelf, IDC_M_RUN_TIMES, nullptr, FALSE), 1);
							return TRUE;
						}

						default:
						{
							return FALSE;
						}
					}
				}

				default:
				{
					return FALSE;
				}
			}
		}
	}
	return FALSE;
}

int RunMacroDlg::getMacro2Exec() const 
{
	bool isCurMacroPresent = static_cast<MacroStatus>(::SendMessage(_hParent, NPPM_GETCURRENTMACROSTATUS, 0, 0)) == MacroStatus::RecordingStopped;
	return isCurMacroPresent?(_macroIndex - 1):_macroIndex;
}
