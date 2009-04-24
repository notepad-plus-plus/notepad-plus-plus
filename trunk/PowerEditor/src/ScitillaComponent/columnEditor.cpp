/*
this file is part of notepad++
Copyright (C)2003 Don HO ( donho@altern.org )

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "columnEditor.h"

BOOL CALLBACK ColumnEditorDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
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
						
						if ((*_ppEditView)->execute(SCI_SELECTIONISRECTANGLE))
						{
							ColumnModeInfo colInfos = (*_ppEditView)->getColumnModeSelectInfo();
							(*_ppEditView)->columnReplace(colInfos, str);
							(*_ppEditView)->execute(SCI_SETCURRENTPOS,colInfos[colInfos.size()-1].second);
							
							//(*_ppEditView)->execute(SCI_SETSEL, colInfos[0].first, colInfos[colInfos.size()-1].second);
							//(*_ppEditView)->execute(SCI_SETSELECTIONMODE, 1);
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

							for (int i = cursorLine ; i <= endLine ; i++)
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
								(*_ppEditView)->getGenericText(line, lineBegin, lineEnd);
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
						UCHAR format = getFormat();
						display(false);
						
						if ((*_ppEditView)->execute(SCI_SELECTIONISRECTANGLE))
						{
							ColumnModeInfo colInfos = (*_ppEditView)->getColumnModeSelectInfo();
							(*_ppEditView)->columnReplace(colInfos, initialNumber, increaseNumber, format);
							(*_ppEditView)->execute(SCI_SETCURRENTPOS,colInfos[colInfos.size()-1].second);
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


							UCHAR f = format & MASK_FORMAT;
							bool isZeroLeading = (MASK_ZERO_LEADING & format) != 0;
							
							int base = 10;
							if (f == BASE_16)
								base = 16;
							else if (f == BASE_08)
								base = 8;
							else if (f == BASE_02)
								base = 2;

							int nbLine = endLine - cursorLine + 1;
							int endNumber = initialNumber + increaseNumber * (nbLine - 1);
							int nbEnd = getNbChiffre(endNumber, base);
							int nbInit = getNbChiffre(initialNumber, base);
							int nb = max(nbInit, nbEnd);


							for (int i = cursorLine ; i <= endLine ; i++)
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
								(*_ppEditView)->getGenericText(line, lineBegin, lineEnd);
								generic_string s2r(line);

								/*
								Calcule generic_string
								*/
								int2str(str, stringSize, initialNumber, base, nb, isZeroLeading);
								initialNumber += increaseNumber;

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
	return FALSE;
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
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_FORMAT_GRP_STATIC), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_DEC_RADIO), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_HEX_RADIO), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_OCT_RADIO), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_BIN_RADIO), !toText);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COL_LEADZERO_CHECK), !toText);

	::SetFocus(toText?hText:hNum);
}