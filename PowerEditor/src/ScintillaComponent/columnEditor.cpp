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


#include <vector>
#include <algorithm>
#include "columnEditor.h"
#include "ScintillaEditView.h"


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
        ::SetFocus(::GetDlgItem(_hSelf, ID_GOLINE_EDIT));
}

intptr_t CALLBACK ColumnEditorDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			switchTo(activeText);
			::SendDlgItemMessage(_hSelf, IDC_COL_DEC_RADIO, BM_SETCHECK, TRUE, 0);
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

		case WM_CTLCOLORDLG:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORSTATIC:
		{
			auto hdcStatic = reinterpret_cast<HDC>(wParam);
			auto dlgCtrlID = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam));

			bool isStaticText = (dlgCtrlID == IDC_COL_INITNUM_STATIC ||
				dlgCtrlID == IDC_COL_INCRNUM_STATIC ||
				dlgCtrlID == IDC_COL_REPEATNUM_STATIC);
			//set the static text colors to show enable/disable instead of ::EnableWindow which causes blurry text
			if (isStaticText)
			{
				bool isTextEnabled = isCheckedOrNot(IDC_COL_NUM_RADIO);
				return NppDarkMode::onCtlColorDarkerBGStaticText(hdcStatic, isTextEnabled);
			}

			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(hdcStatic);
			}
			return FALSE;
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
				RECT rc = {};
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

		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDCANCEL : // Close
					display(false);
					return TRUE;

				case IDOK :
                {
					(*_ppEditView)->execute(SCI_BEGINUNDOACTION);
					
					const int stringSize = 1024;
					TCHAR str[stringSize];
					
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

							int lineAllocatedLen = 1024;
							TCHAR *line = new TCHAR[lineAllocatedLen];

							for (size_t i = cursorLine ; i <= static_cast<size_t>(endLine); ++i)
							{
								auto lineBegin = (*_ppEditView)->execute(SCI_POSITIONFROMLINE, i);
								auto lineEnd = (*_ppEditView)->execute(SCI_GETLINEENDPOSITION, i);

								auto lineEndCol = (*_ppEditView)->execute(SCI_GETCOLUMN, lineEnd);
								auto lineLen = lineEnd - lineBegin + 1;

								if (lineLen > lineAllocatedLen)
								{
									delete [] line;
									line = new TCHAR[lineLen];
								}
								(*_ppEditView)->getGenericText(line, lineLen, lineBegin, lineEnd);
								generic_string s2r(line);

								if (lineEndCol < cursorCol)
								{
									generic_string s_space(cursorCol - lineEndCol, ' ');
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
						int initialNumber = ::GetDlgItemInt(_hSelf, IDC_COL_INITNUM_EDIT, NULL, TRUE);
						int increaseNumber = ::GetDlgItemInt(_hSelf, IDC_COL_INCREASENUM_EDIT, NULL, TRUE);
						int repeat = ::GetDlgItemInt(_hSelf, IDC_COL_REPEATNUM_EDIT, NULL, TRUE);
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
								(*_ppEditView)->columnReplace(colInfos, initialNumber, increaseNumber, repeat, format);
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
							std::vector<int> numbers;
							{
								int curNumber = initialNumber;
								const size_t kiMaxSize = 1 + (size_t)endLine - (size_t)cursorLine;
								while (numbers.size() < kiMaxSize)
								{
									for (int i = 0; i < repeat; i++)
									{
										numbers.push_back(curNumber);
										if (numbers.size() >= kiMaxSize)
										{
											break;
										}
									}
									curNumber += increaseNumber;
								}
							}
							assert(numbers.size() > 0);

							int lineAllocatedLen = 1024;
							TCHAR *line = new TCHAR[lineAllocatedLen];


							UCHAR f = format & MASK_FORMAT;
							bool isZeroLeading = (MASK_ZERO_LEADING & format) != 0;
							
							int base = 10;
							if (f == BASE_16)
								base = 16;
							else if (f == BASE_08)
								base = 8;
							else if (f == BASE_02)
								base = 2;

							int endNumber = *numbers.rbegin();
							int nbEnd = getNbDigits(endNumber, base);
							int nbInit = getNbDigits(initialNumber, base);
							int nb = max(nbInit, nbEnd);


							for (size_t i = cursorLine ; i <= size_t(endLine) ; ++i)
							{
								auto lineBegin = (*_ppEditView)->execute(SCI_POSITIONFROMLINE, i);
								auto lineEnd = (*_ppEditView)->execute(SCI_GETLINEENDPOSITION, i);

								auto lineEndCol = (*_ppEditView)->execute(SCI_GETCOLUMN, lineEnd);
								auto lineLen = lineEnd - lineBegin + 1;

								if (lineLen > lineAllocatedLen)
								{
									delete [] line;
									line = new TCHAR[lineLen];
								}
								(*_ppEditView)->getGenericText(line, lineLen, lineBegin, lineEnd);
								generic_string s2r(line);

								//
								// Calcule generic_string
								//
								int2str(str, stringSize, numbers.at(i - cursorLine), base, nb, isZeroLeading);

								if (lineEndCol < cursorCol)
								{
									generic_string s_space(cursorCol - lineEndCol, ' ');
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
                    (*_ppEditView)->getFocus();
                    return TRUE;
                }
				case IDC_COL_TEXT_RADIO :
				case IDC_COL_NUM_RADIO :
				{
					switchTo((wParam == IDC_COL_TEXT_RADIO)? activeText : activeNumeric);
					return TRUE;
				}

				default :
				{
					switch (HIWORD(wParam))
					{
						case EN_SETFOCUS :
						case BN_SETFOCUS :
							//updateLinesNumbers();
							return TRUE;
						default :
							return TRUE;
					}
					break;
				}
			}
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
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_LEADZERO_CHECK), !toText);

	::SetFocus(toText?hText:hNum);

	redraw();
}

UCHAR ColumnEditorDlg::getFormat() 
{
	bool isLeadingZeros = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_COL_LEADZERO_CHECK, BM_GETCHECK, 0, 0));
	UCHAR f = 0; // Dec by default
	if (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_COL_HEX_RADIO, BM_GETCHECK, 0, 0))
		f = 1;
	else if (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_COL_OCT_RADIO, BM_GETCHECK, 0, 0))
		f = 2;
	else if (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_COL_BIN_RADIO, BM_GETCHECK, 0, 0))
		f = 3;
	return (f | (isLeadingZeros?MASK_ZERO_LEADING:0));
}
