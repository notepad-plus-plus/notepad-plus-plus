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


#include "FindCharsInRange.h"
#include "findCharsInRange_rc.h"
#include "Parameters.h"
#include "localization.h"

intptr_t CALLBACK FindCharsInRangeDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG:
		{
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			::SendDlgItemMessage(_hSelf, IDC_RANGESTART_EDIT, EM_LIMITTEXT, 3, 0);
			::SendDlgItemMessage(_hSelf, IDC_RANGEEND_EDIT, EM_LIMITTEXT, 3, 0);
			::SendDlgItemMessage(_hSelf, IDC_NONASCCI_RADIO, BM_SETCHECK, TRUE, 0);
			::SendDlgItemMessage(_hSelf, ID_FINDCHAR_DIRDOWN, BM_SETCHECK, TRUE, 0);

			::SetDlgItemInt(_hSelf, IDC_RANGESTART_EDIT, 0, FALSE);
			::SetDlgItemInt(_hSelf, IDC_RANGEEND_EDIT, 255, FALSE);

			::EnableWindow(::GetDlgItem(_hSelf, IDC_RANGESTART_EDIT), FALSE);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_RANGEEND_EDIT), FALSE);

			goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);

			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			return NppDarkMode::onCtlColorCtrl(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORDLG:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORSTATIC:
		{
			const auto hdcStatic = reinterpret_cast<HDC>(wParam);
			const auto dlgCtrlID = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam));

			//set the static text colors to show enable/disable instead of ::EnableWindow which causes blurry text
			if (dlgCtrlID == IDC_STATIC)
			{
				const bool isTextEnabled = isCheckedOrNot(IDC_MYRANGE_RADIO);
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
				case IDC_NONASCCI_RADIO:
				case IDC_ASCCI_RADIO:
				{
					::EnableWindow(::GetDlgItem(_hSelf, IDC_RANGESTART_EDIT), FALSE);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_RANGEEND_EDIT), FALSE);
					redrawDlgItem(IDC_STATIC);

					return TRUE;
				}

				case IDC_MYRANGE_RADIO:
				{
					::EnableWindow(::GetDlgItem(_hSelf, IDC_RANGESTART_EDIT), TRUE);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_RANGEEND_EDIT), TRUE);
					redrawDlgItem(IDC_STATIC);

					return TRUE;
				}

				case IDCANCEL: // Close
					display(false);
					return TRUE;

				case ID_FINDCHAR_NEXT:
				{
					intptr_t currentPos = (*_ppEditView)->execute(SCI_GETCURRENTPOS);
					unsigned char startRange = 0;
					unsigned char endRange = 255;
					bool direction = dirDown;
					bool isWrap = true;
					if (!getRangeFromUI(startRange, endRange))
					{
						//STOP!
						NppParameters::getInstance().getNativeLangSpeaker()->messageBox("FindCharRangeValueError",
							_hSelf,
							L"You should type between 0 and 255.",
							L"Range Value problem",
							MB_OK);
						return TRUE;
					}
					getDirectionFromUI(direction, isWrap);
					findCharInRange(startRange, endRange, currentPos, direction, isWrap);
					return TRUE;
				}

				default :
				{
					break;
				}
			}
			break;
		}
		default :
			return FALSE;
	}
	return FALSE;
}

bool FindCharsInRangeDlg::findCharInRange(unsigned char beginRange, unsigned char endRange, intptr_t startPos, bool direction, bool wrap)
{
	size_t totalSize = (*_ppEditView)->getCurrentDocLen();
	if (startPos == -1)
		startPos = direction == dirDown ? 0 : totalSize - 1;
	if (startPos > static_cast<intptr_t>(totalSize))
		return false;

	char *content = new char[totalSize + 1];
	(*_ppEditView)->getText(content, 0, totalSize);

	bool isFound = false;
	size_t found = 0;

	for (int64_t i = startPos - (direction == dirDown ? 0 : 1);
		(direction == dirDown) ? i < static_cast<int64_t>(totalSize) : i >= 0 ;
		(direction == dirDown) ? (++i) : (--i))
	{
		if (static_cast<unsigned char>(content[i]) >= beginRange && static_cast<unsigned char>(content[i]) <= endRange)
		{
			found = static_cast<size_t>(i);
			isFound = true;
			break;
		}
	}
	
	if (!isFound)
	{
		if (wrap)
		{
			for (int64_t i = (direction == dirDown ? 0: totalSize - 1); 
				(direction == dirDown) ? i < static_cast<int64_t>(totalSize) : i >= 0 ;
				(direction == dirDown) ? (++i) : (--i))
			{
				if (static_cast<unsigned char>(content[i]) >= beginRange && static_cast<unsigned char>(content[i]) <= endRange)
				{
					found = static_cast<size_t>(i);
					isFound = true;
					break;
				}
			}
		}
	}

	if (isFound)
	{
		//printInt(found);
		auto sci_line = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, found);
		(*_ppEditView)->execute(SCI_ENSUREVISIBLE, sci_line);
		(*_ppEditView)->execute(SCI_GOTOPOS, found);
		(*_ppEditView)->execute(SCI_SETSEL, (direction == dirDown)? found : found+1, (direction == dirDown) ? found + 1 : found);
	}
	delete [] content;
	return isFound;
}

void FindCharsInRangeDlg::getDirectionFromUI(bool & whichDirection, bool & isWrap)
{
	whichDirection = isCheckedOrNot(ID_FINDCHAR_DIRUP);
	isWrap = isCheckedOrNot(ID_FINDCHAR_WRAP);
}

bool FindCharsInRangeDlg::getRangeFromUI(unsigned char & startRange, unsigned char & endRange)
{
	if (isCheckedOrNot(IDC_NONASCCI_RADIO))
	{
		startRange = 128;
		endRange = 255;
		return true;
	}

	if (isCheckedOrNot(IDC_ASCCI_RADIO))
	{
		startRange = 0;
		endRange = 127;
		return true;
	}
	
	if (isCheckedOrNot(IDC_MYRANGE_RADIO))
	{
		BOOL startBool = FALSE;
		BOOL endBool = FALSE;
		int start = ::GetDlgItemInt(_hSelf, IDC_RANGESTART_EDIT, &startBool, FALSE);
		int end = ::GetDlgItemInt(_hSelf, IDC_RANGEEND_EDIT, &endBool, FALSE);

		if (!startBool || !endBool)
			return false;
		if (start > 255 || end > 255)
			return false;
		if (start > end)
			return false;
		startRange = static_cast<unsigned char>(start);
		endRange = static_cast<unsigned char>(end);
		return true;
	}
	
	return false;
}
