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


#include "GoToLineDlg.h"


intptr_t CALLBACK GoToLineDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG :
		{
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			setChecked(IDC_RADIO_GOTOLINE);
			goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);
			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_ERASEBKGND:
		{
			if (NppDarkMode::isEnabled())
			{
				RECT rc{};
				getClientRect(rc);
				::FillRect(reinterpret_cast<HDC>(wParam), &rc, NppDarkMode::getDarkerBackgroundBrush());
				return TRUE;
			}
			break;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::autoThemeChildControls(_hSelf);
			return TRUE;
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
				case IDCANCEL : // Close
					display(false);
					return TRUE;

				case IDOK :
                {
					(*_ppEditView)->execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF);
                    long long line = getLine();
                    if (line != -1)
                    {
                        display(false);
						if (_mode == go2line)
						{
							(*_ppEditView)->execute(SCI_ENSUREVISIBLE, static_cast<WPARAM>(line - 1));
							(*_ppEditView)->execute(SCI_GOTOLINE, static_cast<WPARAM>(line - 1));
						}
						else
						{
							size_t posToGoto = 0;
							if (line > 0)
							{
								// make sure not jumping into the middle of a multibyte character
								// or into the middle of a CR/LF pair for Windows files
								auto before = (*_ppEditView)->execute(SCI_POSITIONBEFORE, static_cast<WPARAM>(line));
								posToGoto = (*_ppEditView)->execute(SCI_POSITIONAFTER, before);
							}
							auto sci_line = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, posToGoto);
							(*_ppEditView)->execute(SCI_ENSUREVISIBLE, sci_line);
							(*_ppEditView)->execute(SCI_GOTOPOS, posToGoto);
						}
					}
					(*_ppEditView)->execute(SCI_SETMODEVENTMASK, MODEVENTMASK_ON);

					SCNotification notification{};
					notification.nmhdr.code = SCN_PAINTED;
					notification.nmhdr.hwndFrom = _hSelf;
					notification.nmhdr.idFrom = ::GetDlgCtrlID(_hSelf);
					::SendMessage(_hParent, WM_NOTIFY, LINKTRIGGERED, reinterpret_cast<LPARAM>(&notification));

                    (*_ppEditView)->grabFocus();
                    return TRUE;
                }

				case IDC_RADIO_GOTOLINE:
				case IDC_RADIO_GOTOOFFSET:
				{
					if (wParam == IDC_RADIO_GOTOLINE)
					{
						_mode = go2line;
					}
					else
					{
						_mode = go2offsset;
					}

					updateLinesNumbers();
					return TRUE;
				}

				default:
				{
					break;
				}
			}
			return FALSE;
		}

		default :
			return FALSE;
	}
	return FALSE;
}

void GoToLineDlg::updateLinesNumbers() const
{
	size_t current = 0;
	size_t limit = 0;
	
	if (_mode == go2line)
	{
		current = (*_ppEditView)->getCurrentLineNumber() + 1;
		limit = (*_ppEditView)->execute(SCI_GETLINECOUNT);
	}
	else
	{
		current = (*_ppEditView)->execute(SCI_GETCURRENTPOS);
		size_t currentDocLength = (*_ppEditView)->getCurrentDocLen();
		limit = (currentDocLength > 0 ? currentDocLength - 1 : 0);
	}

	::SetDlgItemTextA(_hSelf, ID_CURRLINE_EDIT, std::to_string(current).c_str());
	::SetDlgItemTextA(_hSelf, ID_LASTLINE, std::to_string(limit).c_str());
}
