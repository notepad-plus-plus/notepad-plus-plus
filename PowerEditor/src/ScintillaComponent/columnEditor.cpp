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
	switch (message)
	{
		case WM_INITDIALOG :
		{
			ColumnEditorParam colEditParam = NppParameters::getInstance()._columnEditParam;
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			::SetDlgItemText(_hSelf, IDC_COL_TEXT_EDIT, colEditParam._insertedTextContent.c_str());
			
			if (colEditParam._initialNum != -1)
				::SetDlgItemInt(_hSelf, IDC_COL_INITNUM_EDIT, colEditParam._initialNum, FALSE);
			if (colEditParam._increaseNum != -1)
				::SetDlgItemInt(_hSelf, IDC_COL_INCREASENUM_EDIT, colEditParam._increaseNum, FALSE);
			if (colEditParam._repeatNum != -1)
				::SetDlgItemInt(_hSelf, IDC_COL_REPEATNUM_EDIT, colEditParam._repeatNum, FALSE);
			
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
			if (colEditParam._formatChoice == 1)
				format = IDC_COL_HEX_RADIO;
			else if (colEditParam._formatChoice == 2)
				format = IDC_COL_OCT_RADIO;
			else if (colEditParam._formatChoice == 3)
				format = IDC_COL_BIN_RADIO;

			::SendDlgItemMessage(_hSelf, format, BM_SETCHECK,  TRUE, 0);

			switchTo(colEditParam._mainChoice);
			goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);

			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
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
				case IDCANCEL : // Close
					display(false);
					return TRUE;

				case IDOK :
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
							wchar_t *line = new wchar_t[lineAllocatedLen];

							for (size_t i = cursorLine ; i <= static_cast<size_t>(endLine); ++i)
							{
								auto lineBegin = (*_ppEditView)->execute(SCI_POSITIONFROMLINE, i);
								auto lineEnd = (*_ppEditView)->execute(SCI_GETLINEENDPOSITION, i);

								auto lineEndCol = (*_ppEditView)->execute(SCI_GETCOLUMN, lineEnd);
								auto lineLen = lineEnd - lineBegin + 1;

								if (lineLen > lineAllocatedLen)
								{
									delete [] line;
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
							delete [] line;
						}
					}
					else
					{
						size_t initialNumber = ::GetDlgItemInt(_hSelf, IDC_COL_INITNUM_EDIT, NULL, TRUE);
						size_t increaseNumber = ::GetDlgItemInt(_hSelf, IDC_COL_INCREASENUM_EDIT, NULL, TRUE);
						size_t repeat = ::GetDlgItemInt(_hSelf, IDC_COL_REPEATNUM_EDIT, NULL, TRUE);
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
								for (size_t i = 0; i < repeat; i++)
								{
									numbers.push_back(curNumber);

									if (numbers.size() >= kiMaxSize)
										break;
								}
								curNumber += increaseNumber;
							}

							constexpr int lineAllocatedLen = 1024;
							wchar_t *line = new wchar_t[lineAllocatedLen];

							UCHAR f = format & MASK_FORMAT;

							size_t base = 10;
							if (f == BASE_16)
								base = 16;
							else if (f == BASE_08)
								base = 8;
							else if (f == BASE_02)
								base = 2;

							size_t endNumber = *numbers.rbegin();
							size_t nbEnd = getNbDigits(endNumber, base);
							size_t nbInit = getNbDigits(initialNumber, base);
							size_t nb = std::max<size_t>(nbInit, nbEnd);


							for (size_t i = cursorLine ; i <= size_t(endLine) ; ++i)
							{
								auto lineBegin = (*_ppEditView)->execute(SCI_POSITIONFROMLINE, i);
								auto lineEnd = (*_ppEditView)->execute(SCI_GETLINEENDPOSITION, i);

								auto lineEndCol = (*_ppEditView)->execute(SCI_GETCOLUMN, lineEnd);
								auto lineLen = lineEnd - lineBegin + 1;

								if (lineLen > lineAllocatedLen)
								{
									delete [] line;
									line = new wchar_t[lineLen];
								}
								(*_ppEditView)->getGenericText(line, lineLen, lineBegin, lineEnd);

								wstring s2r(line);

								//
								// Calcule wstring
								//
								variedFormatNumber2String<wchar_t>(str, stringSize, numbers.at(i - cursorLine), base, nb, getLeading());

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
							delete [] line;
						}
					}
					(*_ppEditView)->execute(SCI_ENDUNDOACTION);
                    (*_ppEditView)->grabFocus();
                    return TRUE;
                }
				case IDC_COL_TEXT_RADIO :
				case IDC_COL_NUM_RADIO :
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
					colEditParam._formatChoice = 0; // dec
					if (wParam == IDC_COL_HEX_RADIO)
						colEditParam._formatChoice = 1;
					else if (wParam == IDC_COL_OCT_RADIO)
						colEditParam._formatChoice = 2;
					else if (wParam == IDC_COL_BIN_RADIO)
						colEditParam._formatChoice = 3;

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

									int num = ::GetDlgItemInt(_hSelf, LOWORD(wParam), NULL, TRUE);
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

									int num = ::GetDlgItemInt(_hSelf, LOWORD(wParam), NULL, TRUE);
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

									int num = ::GetDlgItemInt(_hSelf, LOWORD(wParam), NULL, TRUE);
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
						}
						break;
					}
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
	UCHAR f = 0; // Dec by default
	if (isCheckedOrNot(IDC_COL_HEX_RADIO))
		f = 1;
	else if (isCheckedOrNot(IDC_COL_OCT_RADIO))
		f = 2;
	else if (isCheckedOrNot(IDC_COL_BIN_RADIO))
		f = 3;
	return f;
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
