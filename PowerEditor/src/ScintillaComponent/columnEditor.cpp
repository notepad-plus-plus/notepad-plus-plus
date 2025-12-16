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


//#include <vector>
//#include <algorithm>
#include "columnEditor.h"
#include "ScintillaEditView.h"
#include "NppConstants.h"

using namespace std;

void ColumnEditorDlg::init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView)
{
	Window::init(hInst, hPere);
	if (!ppEditView)
		throw std::runtime_error("StaticDialog::init : ppEditView is null.");
	_ppEditView = ppEditView;
}

void ColumnEditorDlg::display(bool toShow) const
{
	Window::display(toShow);
	if (toShow)
	{
		::SetFocus(::GetDlgItem(_hSelf, ID_GOLINE_EDIT));
		::SendMessageW(_hSelf, DM_REPOSITION, 0, 0);
	}
}

intptr_t CALLBACK ColumnEditorDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	static HBRUSH hRedBrush = nullptr;
	static int whichFlashRed = 0;
	constexpr COLORREF rgbRed = RGB(255, 0, 0);
	static HWND hCurrentBalloonTip = nullptr;

	switch (message)
	{
		case WM_INITDIALOG :
		{
			hRedBrush = CreateSolidBrush(rgbRed); // Create red brush once

			ColumnEditorParam colEditParam = NppParameters::getInstance()._columnEditParam;
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			::SetDlgItemText(_hSelf, IDC_COL_TEXT_EDIT, colEditParam._insertedTextContent.c_str());

			setNumericFields(colEditParam);
			
			::SendDlgItemMessage(_hSelf, IDC_COL_LEADING_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"None"));
			::SendDlgItemMessage(_hSelf, IDC_COL_LEADING_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Zeros"));
			::SendDlgItemMessage(_hSelf, IDC_COL_LEADING_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Spaces"));
			WPARAM curSel = 0;
			switch (colEditParam._leadingChoice)
			{
				case ColumnEditorParam::noneLeading: { curSel = 0; break; }
				case ColumnEditorParam::zeroLeading : { curSel = 1; break; }
				case ColumnEditorParam::spaceLeading : { curSel = 2; break; }
				default : { curSel = 0; break; }
			}
			::SendMessage(::GetDlgItem(_hSelf, IDC_COL_LEADING_COMBO), CB_SETCURSEL, curSel, 0);

			int format = IDC_COL_DEC_RADIO;
			if ((colEditParam._formatChoice == BASE_16) || (colEditParam._formatChoice == BASE_16_UPPERCASE))	// either BASE_16 or BASE_16_UC
				format = IDC_COL_HEX_RADIO;
			else if (colEditParam._formatChoice == BASE_08)
				format = IDC_COL_OCT_RADIO;
			else if (colEditParam._formatChoice == BASE_02)
				format = IDC_COL_BIN_RADIO;

			::SendDlgItemMessage(_hSelf, format, BM_SETCHECK,  TRUE, 0);

			// populate the Hex-Case dropdown and activate correct case
			::SendDlgItemMessage(_hSelf, IDC_COL_HEXUC_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"a-f"));
			::SendDlgItemMessage(_hSelf, IDC_COL_HEXUC_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"A-F"));
			UCHAR uc = (colEditParam._formatChoice == BASE_16_UPPERCASE) ? 1 : 0;
			::SendDlgItemMessage(_hSelf, IDC_COL_HEXUC_COMBO, CB_SETCURSEL, uc, 0);	// activate correct case
			EnableWindow(GetDlgItem(_hSelf, IDC_COL_HEXUC_COMBO), format == IDC_COL_HEX_RADIO);	// enable combobox only if hex is chosen

			switchTo(colEditParam._mainChoice);
			goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);

			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			int id = GetDlgCtrlID(reinterpret_cast<HWND>(lParam));
			if (id == whichFlashRed)
			{
				SetBkColor((HDC)wParam, rgbRed);
				return (LRESULT)hRedBrush;
			}
			return NppDarkMode::onCtlColorCtrl(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORLISTBOX:
		{
			return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORDLG:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORSTATIC:
		{
			auto hdcStatic = reinterpret_cast<HDC>(wParam);
			auto dlgCtrlID = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam));

			bool isStaticText = (dlgCtrlID == IDC_COL_INITNUM_STATIC ||
				dlgCtrlID == IDC_COL_INCRNUM_STATIC ||
				dlgCtrlID == IDC_COL_REPEATNUM_STATIC ||
				dlgCtrlID == IDC_COL_LEADING_STATIC);
			//set the static text colors to show enable/disable instead of ::EnableWindow which causes blurry text
			if (isStaticText)
			{
				bool isTextEnabled = isCheckedOrNot(IDC_COL_NUM_RADIO);
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
			if (NppDarkMode::isEnabled())
			{
				const ColumnEditorParam& colEditParam = NppParameters::getInstance()._columnEditParam;
				::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_FORMAT_GRP_STATIC), colEditParam._mainChoice == activeNumeric);
			}
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
				case IDCANCEL: // in case of ESC keystroke
				{
					if (hCurrentBalloonTip && IsWindowVisible(hCurrentBalloonTip)) // if current baloon tip shown, just hide it
					{
						ShowWindow(hCurrentBalloonTip, SW_HIDE);
					}
					else // if current baloon tip doesn't show, we hide Column Editor dialog
					{
						display(false);
					}

					return TRUE;
				}

				case IDOK:
				{
					(*_ppEditView)->execute(SCI_BEGINUNDOACTION);

					constexpr int stringSize = 1024;
					wchar_t str[stringSize]{};

					bool isTextMode = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_COL_TEXT_RADIO, BM_GETCHECK, 0, 0));

					if (isTextMode)
					{
						::SendDlgItemMessage(_hSelf, IDC_COL_TEXT_EDIT, WM_GETTEXT, stringSize, reinterpret_cast<LPARAM>(str));

						display(false);

						if ((*_ppEditView)->execute(SCI_SELECTIONISRECTANGLE) || (*_ppEditView)->execute(SCI_GETSELECTIONS) > 1)
						{
							ColumnModeInfos colInfos = (*_ppEditView)->getColumnModeSelectInfo();
							std::sort(colInfos.begin(), colInfos.end(), SortInPositionOrder());
							(*_ppEditView)->columnReplace(colInfos, str);
							std::sort(colInfos.begin(), colInfos.end(), SortInSelectOrder());
							(*_ppEditView)->setMultiSelections(colInfos);
						}
						else
						{
							auto cursorPos = (*_ppEditView)->execute(SCI_GETCURRENTPOS);
							auto cursorCol = (*_ppEditView)->execute(SCI_GETCOLUMN, cursorPos);
							auto cursorLine = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, cursorPos);
							auto endPos = (*_ppEditView)->execute(SCI_GETLENGTH);
							auto endLine = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, endPos);

							constexpr int lineAllocatedLen = 1024;
							wchar_t* line = new wchar_t[lineAllocatedLen];

							for (size_t i = cursorLine; i <= static_cast<size_t>(endLine); ++i)
							{
								auto lineBegin = (*_ppEditView)->execute(SCI_POSITIONFROMLINE, i);
								auto lineEnd = (*_ppEditView)->execute(SCI_GETLINEENDPOSITION, i);

								auto lineEndCol = (*_ppEditView)->execute(SCI_GETCOLUMN, lineEnd);
								auto lineLen = lineEnd - lineBegin + 1;

								if (lineLen > lineAllocatedLen)
								{
									delete[] line;
									line = new wchar_t[lineLen];
								}
								(*_ppEditView)->getGenericText(line, lineLen, lineBegin, lineEnd);
								wstring s2r(line);

								if (lineEndCol < cursorCol)
								{
									wstring s_space(cursorCol - lineEndCol, ' ');
									s2r.append(s_space);
									s2r.append(str);
								}
								else
								{
									auto posAbs2Start = (*_ppEditView)->execute(SCI_FINDCOLUMN, i, cursorCol);
									auto posRelative2Start = posAbs2Start - lineBegin;
									if (posRelative2Start > static_cast<long long>(s2r.length()))
										posRelative2Start = s2r.length();

									s2r.insert(posRelative2Start, str);
								}
								(*_ppEditView)->replaceTarget(s2r.c_str(), lineBegin, lineEnd);
							}
							delete[] line;
						}
					}
					else
					{
						ColumnEditorParam colEditParam = NppParameters::getInstance()._columnEditParam;

						::GetDlgItemText(_hSelf, IDC_COL_INITNUM_EDIT, str, stringSize);

						int initialNumber = getNumericFieldValueFromText(colEditParam._formatChoice, str, stringSize);
						if (initialNumber == -1)
						{
							whichFlashRed = sendValidationErrorMessage(IDC_COL_INITNUM_EDIT, colEditParam._formatChoice, str);
							(*_ppEditView)->execute(SCI_ENDUNDOACTION);
							return TRUE;
						}

						::GetDlgItemText(_hSelf, IDC_COL_INCREASENUM_EDIT, str, stringSize);
						int increaseNumber = getNumericFieldValueFromText(colEditParam._formatChoice, str, stringSize);
						if (increaseNumber == -1)
						{
							whichFlashRed = sendValidationErrorMessage(IDC_COL_INCREASENUM_EDIT, colEditParam._formatChoice, str);
							(*_ppEditView)->execute(SCI_ENDUNDOACTION);
							return TRUE;
						}

						::GetDlgItemText(_hSelf, IDC_COL_REPEATNUM_EDIT, str, stringSize);
						int repeat = getNumericFieldValueFromText(colEditParam._formatChoice, str, stringSize);
						if (repeat == -1)
						{
							whichFlashRed = sendValidationErrorMessage(IDC_COL_REPEATNUM_EDIT, colEditParam._formatChoice, str);
							(*_ppEditView)->execute(SCI_ENDUNDOACTION);
							return TRUE;
						}

						if (repeat == 0)
						{
							repeat = 1; // Without this we might get an infinite loop while calculating the set "numbers" below.
						}

						UCHAR format = getFormat();
						display(false);

						if ((*_ppEditView)->execute(SCI_SELECTIONISRECTANGLE) || (*_ppEditView)->execute(SCI_GETSELECTIONS) > 1)
						{
							ColumnModeInfos colInfos = (*_ppEditView)->getColumnModeSelectInfo();

							// If there is no column mode info available, no need to do anything
							// If required a message can be shown to user, that select column properly or something similar
							if (colInfos.size() > 0)
							{
								std::sort(colInfos.begin(), colInfos.end(), SortInPositionOrder());
								(*_ppEditView)->columnReplace(colInfos, initialNumber, increaseNumber, repeat, format, getLeading());
								std::sort(colInfos.begin(), colInfos.end(), SortInSelectOrder());
								(*_ppEditView)->setMultiSelections(colInfos);
							}
						}
						else
						{
							auto cursorPos = (*_ppEditView)->execute(SCI_GETCURRENTPOS);
							auto cursorCol = (*_ppEditView)->execute(SCI_GETCOLUMN, cursorPos);
							auto cursorLine = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, cursorPos);
							auto endPos = (*_ppEditView)->execute(SCI_GETLENGTH);
							auto endLine = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, endPos);

							// Compute the numbers to be placed at each column.
							std::vector<size_t> numbers;

							size_t curNumber = initialNumber;
							const size_t kiMaxSize = 1 + (size_t)endLine - (size_t)cursorLine;
							while (numbers.size() < kiMaxSize)
							{
								for (int i = 0; i < repeat; i++)
								{
									numbers.push_back(curNumber);

									if (numbers.size() >= kiMaxSize)
										break;
								}
								curNumber += increaseNumber;
							}

							constexpr int lineAllocatedLen = 1024;
							wchar_t* line = new wchar_t[lineAllocatedLen];

							size_t base = 10;
							bool useUppercase = false;
							if (format == BASE_16)
								base = 16;
							else if (format == BASE_08)
								base = 8;
							else if (format == BASE_02)
								base = 2;
							else if (format == BASE_16_UPPERCASE)
							{
								base = 16;
								useUppercase = true;
							}

							size_t endNumber = *numbers.rbegin();
							size_t nbEnd = getNbDigits(endNumber, base);
							size_t nbInit = getNbDigits(initialNumber, base);
							size_t nb = std::max<size_t>(nbInit, nbEnd);


							for (size_t i = cursorLine; i <= size_t(endLine); ++i)
							{
								auto lineBegin = (*_ppEditView)->execute(SCI_POSITIONFROMLINE, i);
								auto lineEnd = (*_ppEditView)->execute(SCI_GETLINEENDPOSITION, i);

								auto lineEndCol = (*_ppEditView)->execute(SCI_GETCOLUMN, lineEnd);
								auto lineLen = lineEnd - lineBegin + 1;

								if (lineLen > lineAllocatedLen)
								{
									delete[] line;
									line = new wchar_t[lineLen];
								}
								(*_ppEditView)->getGenericText(line, lineLen, lineBegin, lineEnd);

								wstring s2r(line);

								//
								// Calcule wstring
								//
								variedFormatNumber2String<wchar_t>(str, stringSize, numbers.at(i - cursorLine), base, useUppercase, nb, getLeading());

								if (lineEndCol < cursorCol)
								{
									wstring s_space(cursorCol - lineEndCol, ' ');
									s2r.append(s_space);
									s2r.append(str);
								}
								else
								{
									auto posAbs2Start = (*_ppEditView)->execute(SCI_FINDCOLUMN, i, cursorCol);
									auto posRelative2Start = posAbs2Start - lineBegin;
									if (posRelative2Start > static_cast<long long>(s2r.length()))
										posRelative2Start = s2r.length();

									s2r.insert(posRelative2Start, str);
								}

								(*_ppEditView)->replaceTarget(s2r.c_str(), int(lineBegin), int(lineEnd));
							}
							delete[] line;
						}
					}
					(*_ppEditView)->execute(SCI_ENDUNDOACTION);
					(*_ppEditView)->grabFocus();
					return TRUE;
				}
				case IDC_COL_TEXT_RADIO:
				case IDC_COL_NUM_RADIO:
				{
					ColumnEditorParam& colEditParam = NppParameters::getInstance()._columnEditParam;
					colEditParam._mainChoice = (wParam == IDC_COL_TEXT_RADIO) ? activeText : activeNumeric;
					switchTo(colEditParam._mainChoice);
					return TRUE;
				}

				case IDC_COL_DEC_RADIO:
				case IDC_COL_OCT_RADIO:
				case IDC_COL_HEX_RADIO:
				case IDC_COL_BIN_RADIO:
				{
					ColumnEditorParam& colEditParam = NppParameters::getInstance()._columnEditParam;
					colEditParam._formatChoice = BASE_10; // dec
					if (LOWORD(wParam) == IDC_COL_HEX_RADIO)
						colEditParam._formatChoice = getHexCase();	// will pick appropriate UC or LC version of hex
					else if (LOWORD(wParam) == IDC_COL_OCT_RADIO)
						colEditParam._formatChoice = BASE_08;
					else if (LOWORD(wParam) == IDC_COL_BIN_RADIO)
						colEditParam._formatChoice = BASE_02;

					setNumericFields(colEditParam);	// reformat the field text to be based on the new radix
					EnableWindow(GetDlgItem(_hSelf, IDC_COL_HEXUC_COMBO), LOWORD(wParam) == IDC_COL_HEX_RADIO);	// enable combobox only if hex is chosen

					return TRUE;
				}

				default :
				{
					switch (HIWORD(wParam))
					{
						case EN_CHANGE:
						{
							ColumnEditorParam& colEditParam = NppParameters::getInstance()._columnEditParam;
							constexpr int stringSize = MAX_PATH;
							wchar_t str[stringSize]{};

							switch (LOWORD(wParam))
							{
								case IDC_COL_TEXT_EDIT:
								{
									::GetDlgItemText(_hSelf, LOWORD(wParam), str, stringSize);
									colEditParam._insertedTextContent = str;
									::EnableWindow(::GetDlgItem(_hSelf, IDOK), str[0]);
									return TRUE;
								}
								case IDC_COL_INITNUM_EDIT:
								{
									::GetDlgItemText(_hSelf, LOWORD(wParam), str, stringSize);

									if (lstrcmp(str, L"") == 0)
									{
										colEditParam._initialNum = -1;
										return TRUE;
									}

									int num = getNumericFieldValueFromText(colEditParam._formatChoice, str, stringSize);
									if (num == -1)
									{
										num = colEditParam._initialNum;
										setNumericFields(colEditParam);	// reformat the strings to eliminate error
										whichFlashRed = sendValidationErrorMessage(LOWORD(wParam), colEditParam._formatChoice, str);
									}
									
									colEditParam._initialNum = num;
									return TRUE;
								}
								case IDC_COL_INCREASENUM_EDIT:
								{
									::GetDlgItemText(_hSelf, LOWORD(wParam), str, stringSize);

									if (lstrcmp(str, L"") == 0)
									{
										colEditParam._increaseNum = -1;
										return TRUE;
									}

									int num = getNumericFieldValueFromText(colEditParam._formatChoice, str, stringSize);
									if (num == -1)
									{
										num = colEditParam._increaseNum;
										setNumericFields(colEditParam);	// reformat the strings to eliminate error
										whichFlashRed = sendValidationErrorMessage(LOWORD(wParam), colEditParam._formatChoice, str);
									}

									colEditParam._increaseNum = num;
									return TRUE;
								}
								case IDC_COL_REPEATNUM_EDIT:
								{
									::GetDlgItemText(_hSelf, LOWORD(wParam), str, stringSize);

									if (lstrcmp(str, L"") == 0)
									{
										colEditParam._repeatNum = -1;
										return TRUE;
									}

									int num = getNumericFieldValueFromText(colEditParam._formatChoice, str, stringSize);
									if (num == -1)
									{
										num = colEditParam._repeatNum;
										setNumericFields(colEditParam);	// reformat the strings to eliminate error
										whichFlashRed = sendValidationErrorMessage(LOWORD(wParam), colEditParam._formatChoice, str);
									}

									colEditParam._repeatNum = num;
									return TRUE;
								}
							}
						}
						break;

						case CBN_SELCHANGE:
						{
							if (LOWORD(wParam) == IDC_COL_LEADING_COMBO)
							{
								ColumnEditorParam& colEditParam = NppParameters::getInstance()._columnEditParam;
								colEditParam._leadingChoice = getLeading();
								return TRUE;
							}
							else if(LOWORD(wParam) == IDC_COL_HEXUC_COMBO)
							{
								ColumnEditorParam& colEditParam = NppParameters::getInstance()._columnEditParam;
								if ((colEditParam._formatChoice & BASE_16) == BASE_16 )
									colEditParam._formatChoice = getHexCase();

								setNumericFields(colEditParam);	// want the GUI fields to update case when combobox changes
								return TRUE;
							}
						}
						break;
					}
					break;
				}
			}
			break;
		}

		case WM_TIMER:
		{
			static int idRedraw = 0;

			if (wParam == IDT_COL_FLASH_TIMER)
			{
				KillTimer(_hSelf, IDT_COL_FLASH_TIMER);

				idRedraw = whichFlashRed;		// keep the ID for the one whose flash is ending...
				whichFlashRed = 0;				// must be 0 before the redraw, otherwise it will maintain color
				redrawDlgItem(idRedraw, true);	// redraw the just the one that was flashed

				// Remember the latest/current baloon tip handle
				hCurrentBalloonTip = [](HWND hEditControl) -> HWND {
					HWND hTooltip = FindWindowEx(NULL, NULL, L"tooltips_class32", NULL);

					while (hTooltip)
					{
						HWND hParent = GetParent(hTooltip);
						if (hParent == hEditControl || hParent == GetParent(hEditControl))
						{
							return hTooltip;
						}
						hTooltip = FindWindowEx(NULL, hTooltip, L"tooltips_class32", NULL);
					}
					return NULL;
				}(GetDlgItem(_hSelf, idRedraw));
			}

			if (wParam == IDC_COL_BALLONTIP_TIMER)
			{
				KillTimer(_hSelf, IDC_COL_BALLONTIP_TIMER);

				SendMessage(GetDlgItem(_hSelf, idRedraw), EM_HIDEBALLOONTIP, 0, 0);
			}

			break;
		}

		case WM_DESTROY:
		{
			DeleteObject(hRedBrush);
			break;
		}

		default :
			return FALSE;
	}
	return FALSE;
}

void ColumnEditorDlg::switchTo(bool toText)
{
	HWND hText = ::GetDlgItem(_hSelf, IDC_COL_TEXT_EDIT);
	::EnableWindow(hText, toText);
	::SendDlgItemMessage(_hSelf, IDC_COL_TEXT_RADIO, BM_SETCHECK, toText, 0);

	HWND hNum = ::GetDlgItem(_hSelf, IDC_COL_INITNUM_EDIT);
	::SendDlgItemMessage(_hSelf, IDC_COL_NUM_RADIO, BM_SETCHECK, !toText, 0);
	::EnableWindow(hNum, !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_INCREASENUM_EDIT), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_REPEATNUM_EDIT), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_DEC_RADIO), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_HEX_RADIO), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_OCT_RADIO), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_BIN_RADIO), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_LEADING_COMBO), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDOK), !toText || !NppParameters::getInstance()._columnEditParam._insertedTextContent.empty());

	::SetFocus(toText?hText:hNum);

	redrawDlgItem(IDC_COL_INITNUM_STATIC);
	redrawDlgItem(IDC_COL_INCRNUM_STATIC);
	redrawDlgItem(IDC_COL_REPEATNUM_STATIC);
	redrawDlgItem(IDC_COL_LEADING_STATIC);

	if (NppDarkMode::isEnabled())
	{
		::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_FORMAT_GRP_STATIC), !toText);
		redrawDlgItem(IDC_COL_FORMAT_GRP_STATIC);
	}
}

UCHAR ColumnEditorDlg::getFormat()
{
	UCHAR f = BASE_10; // Dec by default
	if (isCheckedOrNot(IDC_COL_HEX_RADIO))
		f = getHexCase();	// will give BASE_16 or BASE_16_UC, depending on case selector
	else if (isCheckedOrNot(IDC_COL_OCT_RADIO))
		f = BASE_08;
	else if (isCheckedOrNot(IDC_COL_BIN_RADIO))
		f = BASE_02;
	return f;
}


UCHAR ColumnEditorDlg::getHexCase()
{
	int curSel = static_cast<int>(::SendDlgItemMessage(_hSelf, IDC_COL_HEXUC_COMBO, CB_GETCURSEL, 0, 0));
	return (curSel == 1) ? BASE_16_UPPERCASE : BASE_16;
}

ColumnEditorParam::leadingChoice ColumnEditorDlg::getLeading()
{
	ColumnEditorParam::leadingChoice leading = ColumnEditorParam::noneLeading;
	int curSel = static_cast<int>(::SendDlgItemMessage(_hSelf, IDC_COL_LEADING_COMBO, CB_GETCURSEL, 0, 0));
	switch (curSel)
	{
		case 0:
		default:
		{
			leading = ColumnEditorParam::noneLeading;
			break;
		}
		case 1:
		{
			leading = ColumnEditorParam::zeroLeading;
			break;
		}
		case 2:
		{
			leading = ColumnEditorParam::spaceLeading;
			break;
		}
	}
	return leading;
}

void ColumnEditorDlg::setNumericFields(const ColumnEditorParam& colEditParam)
{
	if (colEditParam._formatChoice == BASE_10)
	{
		if (colEditParam._initialNum != -1)
			::SetDlgItemInt(_hSelf, IDC_COL_INITNUM_EDIT, colEditParam._initialNum, FALSE);
		else
			::SetDlgItemText(_hSelf, IDC_COL_INITNUM_EDIT, L"");

		if (colEditParam._increaseNum != -1)
			::SetDlgItemInt(_hSelf, IDC_COL_INCREASENUM_EDIT, colEditParam._increaseNum, FALSE);
		else
			::SetDlgItemText(_hSelf, IDC_COL_INCREASENUM_EDIT, L"");

		if (colEditParam._repeatNum != -1)
			::SetDlgItemInt(_hSelf, IDC_COL_REPEATNUM_EDIT, colEditParam._repeatNum, FALSE);
		else
			::SetDlgItemText(_hSelf, IDC_COL_REPEATNUM_EDIT, L"");
	}
	else
	{
		size_t base = 10;
		switch (colEditParam._formatChoice)
		{
			case BASE_16:		// hex
			case BASE_16_UPPERCASE:	// or hex w/ uppercase A-F
				base = 16;
				break;
			case BASE_08:		// oct
				base = 8;
				break;
			case BASE_02:		// bin
				base = 2;
				break;
			default:
				base = 10;
				break;
		}
		bool useUpper = (colEditParam._formatChoice == BASE_16_UPPERCASE);

		constexpr int stringSize = 1024;
		wchar_t str[stringSize]{};

		if (colEditParam._initialNum != -1)
		{
			variedFormatNumber2String<wchar_t>(str, stringSize, colEditParam._initialNum, base, useUpper, getNbDigits(colEditParam._initialNum, base), getLeading());
			::SetDlgItemText(_hSelf, IDC_COL_INITNUM_EDIT, str);
		}
		else
			::SetDlgItemText(_hSelf, IDC_COL_INITNUM_EDIT, L"");

		if (colEditParam._increaseNum != -1)
		{
			variedFormatNumber2String<wchar_t>(str, stringSize, colEditParam._increaseNum, base, useUpper, getNbDigits(colEditParam._increaseNum, base), getLeading());
			::SetDlgItemText(_hSelf, IDC_COL_INCREASENUM_EDIT, str);
		}
		else
			::SetDlgItemText(_hSelf, IDC_COL_INCREASENUM_EDIT, L"");

		if (colEditParam._repeatNum != -1)
		{
			variedFormatNumber2String<wchar_t>(str, stringSize, colEditParam._repeatNum, base, useUpper, getNbDigits(colEditParam._repeatNum, base), getLeading());
			::SetDlgItemText(_hSelf, IDC_COL_REPEATNUM_EDIT, str);
		}
		else
			::SetDlgItemText(_hSelf, IDC_COL_REPEATNUM_EDIT, L"");

	}
	return;
}

// Convert the string to an integer, depending on base
int ColumnEditorDlg::getNumericFieldValueFromText(int formatChoice, wchar_t str[], size_t /*stringSize*/)
{
	int num = 0;
	int base = 0;

	switch (formatChoice)
	{
		case BASE_16:
		case BASE_16_UPPERCASE:
			base = 16;
			break;
		case BASE_08:
			base = 8;
			break;
		case BASE_02:
			base = 2;
			break;
		default:
			base = 10;
			break;
	}

	// convert string in base to int value; on error, return -1
	wchar_t* pEnd = nullptr;
	num = static_cast<int>(std::wcstol(str, &pEnd, base));
	if (pEnd == nullptr || *pEnd != L'\0')
	{
		return -1;
	}

	return num;
}

int ColumnEditorDlg::sendValidationErrorMessage(int whichFlashRed, int formatChoice, wchar_t str[])
{
	wchar_t wcMsg[1024];
	const wchar_t *wcRadixNote;
	EDITBALLOONTIP ebt;
	ebt.cbStruct = sizeof(EDITBALLOONTIP);
	ebt.pszTitle = L"Invalid Numeric Entry";
	switch (formatChoice)
	{
		case BASE_16:
		case BASE_16_UPPERCASE:
			wcRadixNote = L"Hex numbers use 0-9, A-F!";
			break;
		case BASE_08:
			wcRadixNote = L"Oct numbers only use 0-7!";
			break;
		case BASE_02:
			wcRadixNote = L"Bin numbers only use 0-1!";
			break;
		default:
			wcRadixNote = L"Decimal numbers only use 0-9!";
			break;
	}
	if (str[0])
	{
		swprintf_s(wcMsg, L"Entered string \"%s\":\r\n%s", str, wcRadixNote);
		ebt.pszText = wcMsg;
	}
	ebt.ttiIcon = TTI_ERROR_LARGE;    // tooltip icon
	SendMessage(GetDlgItem(_hSelf, whichFlashRed), EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt);

	SetTimer(_hSelf, IDT_COL_FLASH_TIMER, 250, NULL);
	SetTimer(_hSelf, IDC_COL_BALLONTIP_TIMER, 3500, NULL);

	redrawDlgItem(whichFlashRed);

	return whichFlashRed;
}
