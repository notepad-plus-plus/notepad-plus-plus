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


#include "columnEditor.h"

#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <Scintilla.h>

#include "Common.h"
#include "NppConstants.h"
#include "NppDarkMode.h"
#include "Parameters.h"
#include "ScintillaEditView.h"
#include "Window.h"
#include "columnEditor_rc.h"
#include "resource.h"

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
	static constexpr COLORREF rgbRed = RGB(255, 0, 0);

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
				using enum ColumnEditorParam::leadingChoice;
				case noneLeading: { curSel = 0; break; }
				case zeroLeading: { curSel = 1; break; }
				case spaceLeading: { curSel = 2; break; }
				default : { curSel = 0; break; }
			}
			::SendMessage(::GetDlgItem(_hSelf, IDC_COL_LEADING_COMBO), CB_SETCURSEL, curSel, 0);

			int format = IDC_COL_DEC_RADIO;
			using enum NumBase;
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
			WPARAM uc = (colEditParam._formatChoice == BASE_16_UPPERCASE) ? 1 : 0;
			::SendDlgItemMessage(_hSelf, IDC_COL_HEXUC_COMBO, CB_SETCURSEL, uc, 0);	// activate correct case

			switchTo(colEditParam._mainChoice);
			goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);

			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			int id = GetDlgCtrlID(reinterpret_cast<HWND>(lParam));
			if (id == whichFlashRed)
			{
				::SetBkColor(reinterpret_cast<HDC>(wParam), rgbRed);
				return reinterpret_cast<LRESULT>(hRedBrush);
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
					if (_hCurrentBalloonTip && IsWindowVisible(_hCurrentBalloonTip)) // if current baloon tip shown, just hide it
					{
						::ShowWindow(_hCurrentBalloonTip, SW_HIDE);
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

					static constexpr int stringSize = 1024;

					bool isTextMode = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_COL_TEXT_RADIO, BM_GETCHECK, 0, 0));

					if (isTextMode)
					{
						wchar_t strW[stringSize]{};
						::SendDlgItemMessage(_hSelf, IDC_COL_TEXT_EDIT, WM_GETTEXT, stringSize, reinterpret_cast<LPARAM>(strW));

						const std::string str = wstring2string(strW);

						display(false);

						if ((*_ppEditView)->execute(SCI_SELECTIONISRECTANGLE) || (*_ppEditView)->execute(SCI_GETSELECTIONS) > 1)
						{
							ColumnModeInfos colInfos = (*_ppEditView)->getColumnModeSelectInfo();
							std::sort(colInfos.begin(), colInfos.end(), SortInPositionOrder());
							(*_ppEditView)->columnReplace(colInfos, str.c_str());
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

							static constexpr int lineAllocatedLen = 1024;
							auto line = std::make_unique<char[]>(lineAllocatedLen);

							for (size_t i = cursorLine; i <= static_cast<size_t>(endLine); ++i)
							{
								auto lineBegin = (*_ppEditView)->execute(SCI_POSITIONFROMLINE, i);
								auto lineEnd = (*_ppEditView)->execute(SCI_GETLINEENDPOSITION, i);

								auto lineEndCol = (*_ppEditView)->execute(SCI_GETCOLUMN, lineEnd);
								auto lineLen = lineEnd - lineBegin + 1;

								if (lineLen > lineAllocatedLen)
								{
									line.reset(new char[lineLen]);
								}
								(*_ppEditView)->getGenericText(line.get(), lineLen, lineBegin, lineEnd);
								std::string s2r(line.get());

								if (lineEndCol < cursorCol)
								{
									std::string s_space(cursorCol - lineEndCol, ' ');
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
								(*_ppEditView)->replaceTarget(s2r, lineBegin, lineEnd);
							}
						}
					}
					else
					{
						ColumnEditorParam colEditParam = NppParameters::getInstance()._columnEditParam;

						wchar_t str[stringSize]{};
						::GetDlgItemText(_hSelf, IDC_COL_INITNUM_EDIT, str, stringSize);

						const int initialNumber = getNumericFieldValueFromText(colEditParam._formatChoice, str);
						if (initialNumber == -1)
						{
							whichFlashRed = sendValidationErrorMessage(IDC_COL_INITNUM_EDIT, colEditParam._formatChoice, str);
							(*_ppEditView)->execute(SCI_ENDUNDOACTION);
							return TRUE;
						}

						::GetDlgItemText(_hSelf, IDC_COL_INCREASENUM_EDIT, str, stringSize);
						const int increaseNumber = getNumericFieldValueFromText(colEditParam._formatChoice, str);
						if (increaseNumber == -1)
						{
							whichFlashRed = sendValidationErrorMessage(IDC_COL_INCREASENUM_EDIT, colEditParam._formatChoice, str);
							(*_ppEditView)->execute(SCI_ENDUNDOACTION);
							return TRUE;
						}

						::GetDlgItemText(_hSelf, IDC_COL_REPEATNUM_EDIT, str, stringSize);
						int repeat = getNumericFieldValueFromText(colEditParam._formatChoice, str);
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

						const NumBase format = getFormat();
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
							const size_t kiMaxSize = 1 + static_cast<size_t>(endLine) - static_cast<size_t>(cursorLine);
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

							static constexpr int lineAllocatedLen = 1024;
							auto line = std::make_unique<wchar_t[]>(lineAllocatedLen);

							size_t base = 10;
							bool useUppercase = false;
							using enum NumBase;
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

							for (size_t i = cursorLine; i <= static_cast<size_t>(endLine); ++i)
							{
								auto lineBegin = (*_ppEditView)->execute(SCI_POSITIONFROMLINE, i);
								auto lineEnd = (*_ppEditView)->execute(SCI_GETLINEENDPOSITION, i);

								auto lineEndCol = (*_ppEditView)->execute(SCI_GETCOLUMN, lineEnd);
								auto lineLen = lineEnd - lineBegin + 1;

								if (lineLen > lineAllocatedLen)
								{
									line.reset(new wchar_t[lineLen]);
								}
								(*_ppEditView)->getGenericText(line.get(), lineLen, lineBegin, lineEnd);

								std::wstring s2r(line.get());

								//
								// Calcule wstring
								//
								variedFormatNumber2String<wchar_t>(str, stringSize, numbers.at(i - cursorLine), base, useUppercase, nb, getLeading());

								if (lineEndCol < cursorCol)
								{
									std::wstring s_space(cursorCol - lineEndCol, ' ');
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

								(*_ppEditView)->replaceTarget(s2r.c_str(), static_cast<int>(lineBegin), static_cast<int>(lineEnd));
							}
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
					using enum NumBase;
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
							static constexpr int stringSize = MAX_PATH;
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

									int num = getNumericFieldValueFromText(colEditParam._formatChoice, str);
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

									int num = getNumericFieldValueFromText(colEditParam._formatChoice, str);
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

									int num = getNumericFieldValueFromText(colEditParam._formatChoice, str);
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
								using enum NumBase;
								ColumnEditorParam& colEditParam = NppParameters::getInstance()._columnEditParam;
								if (colEditParam._formatChoice == BASE_16 || colEditParam._formatChoice == BASE_16_UPPERCASE)
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

	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_HEXUC_COMBO), !toText && isCheckedOrNot(IDC_COL_HEX_RADIO));

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

NumBase ColumnEditorDlg::getFormat()
{
	using enum NumBase;
	NumBase f = BASE_10; // Dec by default
	if (isCheckedOrNot(IDC_COL_HEX_RADIO))
		f = getHexCase();	// will give BASE_16 or BASE_16_UC, depending on case selector
	else if (isCheckedOrNot(IDC_COL_OCT_RADIO))
		f = BASE_08;
	else if (isCheckedOrNot(IDC_COL_BIN_RADIO))
		f = BASE_02;
	return f;
}


NumBase ColumnEditorDlg::getHexCase()
{
	using enum NumBase;
	int curSel = static_cast<int>(::SendDlgItemMessage(_hSelf, IDC_COL_HEXUC_COMBO, CB_GETCURSEL, 0, 0));
	return (curSel == 1) ? BASE_16_UPPERCASE : BASE_16;
}

ColumnEditorParam::leadingChoice ColumnEditorDlg::getLeading()
{
	using enum ColumnEditorParam::leadingChoice;
	ColumnEditorParam::leadingChoice leading = noneLeading;
	int curSel = static_cast<int>(::SendDlgItemMessage(_hSelf, IDC_COL_LEADING_COMBO, CB_GETCURSEL, 0, 0));
	switch (curSel)
	{
		case 0:
		default:
		{
			leading = noneLeading;
			break;
		}
		case 1:
		{
			leading = zeroLeading;
			break;
		}
		case 2:
		{
			leading = spaceLeading;
			break;
		}
	}
	return leading;
}

void ColumnEditorDlg::setNumericFields(const ColumnEditorParam& colEditParam)
{
	using enum NumBase;
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

		static constexpr int stringSize = 1024;
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
int ColumnEditorDlg::getNumericFieldValueFromText(NumBase formatChoice, const std::wstring& str)
{
	int base = 0;

	switch (formatChoice)
	{
		using enum NumBase;
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

	size_t end = 0;
	// convert string in base to int value; on error, return -1
	try
	{
		const int num = std::stoi(str, &end, base);
		if (end != str.length())
			return -1;
		return num;
	}
	catch (std::invalid_argument const&)
	{
		return -1;
	}
	catch (std::out_of_range const&)
	{
		return -1;
	}
}

int ColumnEditorDlg::sendValidationErrorMessage(int whichFlashRed, NumBase formatChoice, wchar_t str[])
{
	wchar_t wcMsg[1024];
	const wchar_t* wcRadixNote = nullptr;
	EDITBALLOONTIP ebt{};
	ebt.cbStruct = sizeof(EDITBALLOONTIP);
	ebt.pszTitle = L"Invalid Numeric Entry";
	switch (formatChoice)
	{
		using enum NumBase;
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
	::SendMessage(::GetDlgItem(_hSelf, whichFlashRed), EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&ebt));

	_hCurrentBalloonTip = [](HWND hEditControl) -> HWND
	{
		HWND hTooltip = ::FindWindowEx(nullptr, nullptr, TOOLTIPS_CLASS, nullptr);

		while (hTooltip)
		{
			HWND hParent = ::GetParent(hTooltip);
			if (hParent == hEditControl || hParent == ::GetParent(hEditControl))
			{
				NppDarkMode::setDarkTooltips(hTooltip, NppDarkMode::ToolTipsType::tooltip);
				return hTooltip;
			}
			hTooltip = ::FindWindowEx(nullptr, hTooltip, TOOLTIPS_CLASS, nullptr);
		}
		return nullptr;
	}(::GetDlgItem(_hSelf, whichFlashRed));

	SetTimer(_hSelf, IDT_COL_FLASH_TIMER, 250, NULL);
	SetTimer(_hSelf, IDC_COL_BALLONTIP_TIMER, 3500, NULL);

	redrawDlgItem(whichFlashRed);

	return whichFlashRed;
}
