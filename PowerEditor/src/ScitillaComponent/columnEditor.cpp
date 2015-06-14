// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid      
// misunderstandings, we consider an application to constitute a          
// "derivative work" for the purpose of this license if it does any of the
// following:                                                             
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include <vector>
#include <algorithm>
#include <Shlobj.h>
#include <uxtheme.h>
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

INT_PTR CALLBACK ColumnEditorDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			switchTo(activeText);
			::SendDlgItemMessage(_hSelf, IDC_COL_DEC_RADIO, BM_SETCHECK, TRUE, 0);
			goToCenter();

			NppParameters *pNppParam = NppParameters::getInstance();
			ETDTProc enableDlgTheme = (ETDTProc)pNppParam->getEnableThemeDlgTexture();
			if (enableDlgTheme)
			{
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);
				redraw();
			}
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
						::SendDlgItemMessage(_hSelf, IDC_COL_TEXT_EDIT, WM_GETTEXT, stringSize, (LPARAM)str);

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
							int cursorPos = (*_ppEditView)->execute(SCI_GETCURRENTPOS);
							int cursorCol = (*_ppEditView)->execute(SCI_GETCOLUMN, cursorPos);
							int cursorLine = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, cursorPos);
							int endPos = (*_ppEditView)->execute(SCI_GETLENGTH);
							int endLine = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, endPos);

							int lineAllocatedLen = 1024;
							TCHAR *line = new TCHAR[lineAllocatedLen];

							for (int i = cursorLine ; i <= endLine ; ++i)
							{
								int lineBegin = (*_ppEditView)->execute(SCI_POSITIONFROMLINE, i);
								int lineEnd = (*_ppEditView)->execute(SCI_GETLINEENDPOSITION, i);

								int lineEndCol = (*_ppEditView)->execute(SCI_GETCOLUMN, lineEnd);
								int lineLen = lineEnd - lineBegin + 1;

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
									int posAbs2Start = (*_ppEditView)->execute(SCI_FINDCOLUMN, i, cursorCol);
									int posRelative2Start = posAbs2Start - lineBegin;
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
							std::sort(colInfos.begin(), colInfos.end(), SortInPositionOrder());
							(*_ppEditView)->columnReplace(colInfos, initialNumber, increaseNumber, repeat, format);
							std::sort(colInfos.begin(), colInfos.end(), SortInSelectOrder());
							(*_ppEditView)->setMultiSelections(colInfos);
						}
						else
						{
							int cursorPos = (*_ppEditView)->execute(SCI_GETCURRENTPOS);
							int cursorCol = (*_ppEditView)->execute(SCI_GETCOLUMN, cursorPos);
							int cursorLine = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, cursorPos);
							int endPos = (*_ppEditView)->execute(SCI_GETLENGTH);
							int endLine = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, endPos);

							// Compute the numbers to be placed at each column.
							std::vector<int> numbers;
							{
								int curNumber = initialNumber;
								const unsigned int kiMaxSize = 1 + (unsigned int)endLine - (unsigned int)cursorLine;
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


							for (int i = cursorLine ; i <= endLine ; ++i)
							{
								int lineBegin = (*_ppEditView)->execute(SCI_POSITIONFROMLINE, i);
								int lineEnd = (*_ppEditView)->execute(SCI_GETLINEENDPOSITION, i);

								int lineEndCol = (*_ppEditView)->execute(SCI_GETCOLUMN, lineEnd);
								int lineLen = lineEnd - lineBegin + 1;

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
									int posAbs2Start = (*_ppEditView)->execute(SCI_FINDCOLUMN, i, cursorCol);
									int posRelative2Start = posAbs2Start - lineBegin;
									s2r.insert(posRelative2Start, str);
								}

								(*_ppEditView)->replaceTarget(s2r.c_str(), lineBegin, lineEnd);
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
	//return FALSE;
}

void ColumnEditorDlg::switchTo(bool toText)
{
	HWND hText = ::GetDlgItem(_hSelf, IDC_COL_TEXT_EDIT);
	::EnableWindow(hText, toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_TEXT_GRP_STATIC), toText);
	::SendDlgItemMessage(_hSelf, IDC_COL_TEXT_RADIO, BM_SETCHECK, toText, 0);

	HWND hNum = ::GetDlgItem(_hSelf, IDC_COL_INITNUM_EDIT);
	::SendDlgItemMessage(_hSelf, IDC_COL_NUM_RADIO, BM_SETCHECK, !toText, 0);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_NUM_GRP_STATIC), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_INITNUM_STATIC), !toText);
	::EnableWindow(hNum, !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_INCRNUM_STATIC), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_INCREASENUM_EDIT), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_REPEATNUM_STATIC), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_REPEATNUM_EDIT), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_FORMAT_GRP_STATIC), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_DEC_RADIO), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_HEX_RADIO), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_OCT_RADIO), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_BIN_RADIO), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_LEADZERO_CHECK), !toText);

	::SetFocus(toText?hText:hNum);
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
