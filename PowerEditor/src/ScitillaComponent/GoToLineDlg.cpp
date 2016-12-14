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


#include "GoToLineDlg.h"
#include <codecvt>


INT_PTR CALLBACK GoToLineDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			::SendDlgItemMessage(_hSelf, IDC_RADIO_GOTOLINE, BM_SETCHECK, TRUE, 0);
			::ShowWindow(::GetDlgItem(_hSelf, ID_COLUMN_STATIC), SW_HIDE);
			::ShowWindow(::GetDlgItem(_hSelf, ID_GOCOLUMN_EDIT), SW_HIDE);
			goToCenter();
			return TRUE;
		}
		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDCANCEL : // Close
					display(false);
                    cleanLineEdit();
					return TRUE;

				case IDOK :
                {
                    int line = getLine();
                    if (line != -1)
                    {
                        display(false);
                        cleanLineEdit();
						
						if (_mode == go2line)
						{
							(*_ppEditView)->execute(SCI_ENSUREVISIBLE, line-1);
							(*_ppEditView)->execute(SCI_GOTOLINE, line-1);
						}
						else if (_mode == go2column)
						{
							int column = getColumn();
							cleanColumnEdit();
							if (column != -1)
							{
								(*_ppEditView)->execute(SCI_ENSUREVISIBLE, line - 1);
								(*_ppEditView)->execute(SCI_GOTOLINE, line - 1);
								int lineStartOffset = static_cast<unsigned int>((*_ppEditView)->execute(SCI_GETCURRENTPOS));
								(*_ppEditView)->execute(SCI_GOTOPOS, lineStartOffset + column);

							}
						}
						else
						{
							auto sci_line = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, line);
							(*_ppEditView)->execute(SCI_ENSUREVISIBLE, sci_line);
							(*_ppEditView)->execute(SCI_GOTOPOS, line);
						}
                    }

					SCNotification notification = {};
					notification.nmhdr.code = SCN_PAINTED;
					notification.nmhdr.hwndFrom = _hSelf;
					notification.nmhdr.idFrom = ::GetDlgCtrlID(_hSelf);
					::SendMessage(_hParent, WM_NOTIFY, LINKTRIGGERED, reinterpret_cast<LPARAM>(&notification));

                    (*_ppEditView)->getFocus();
                    return TRUE;
                }

				case IDC_RADIO_GOTOLINE :
				case IDC_RADIO_GOTOOFFSET :
				case IDC_RADIO_GOTOCOLUMN :	//extra case for new option
				{
					bool isLine, isOffset, isColumn;
					if (wParam == IDC_RADIO_GOTOLINE)
					{
						isLine = true;
						isOffset = false;
						isColumn = false;
						_mode = go2line;
						::ShowWindow(::GetDlgItem(_hSelf, ID_COLUMN_STATIC), SW_HIDE);
						::ShowWindow(::GetDlgItem(_hSelf, ID_GOCOLUMN_EDIT), SW_HIDE);
					}
					else if (wParam == IDC_RADIO_GOTOCOLUMN)
					{
						isLine = false;
						isOffset = false;
						isColumn = true;
						_mode = go2column;
						::ShowWindow(::GetDlgItem(_hSelf, ID_GOCOLUMN_EDIT), SW_SHOW);
						::ShowWindow(::GetDlgItem(_hSelf, ID_COLUMN_STATIC), SW_SHOW);
					}
					else
					{
						isLine = false;
						isOffset = true;
						isColumn = false;
						_mode = go2offsset;
						::ShowWindow(::GetDlgItem(_hSelf, ID_COLUMN_STATIC), SW_HIDE);
						::ShowWindow(::GetDlgItem(_hSelf, ID_GOCOLUMN_EDIT), SW_HIDE);
					}
					
					::SendDlgItemMessage(_hSelf, IDC_RADIO_GOTOLINE, BM_SETCHECK, isLine, 0);
					::SendDlgItemMessage(_hSelf, IDC_RADIO_GOTOOFFSET, BM_SETCHECK, isOffset, 0);
					::SendDlgItemMessage(_hSelf, IDC_RADIO_GOTOCOLUMN, BM_SETCHECK, isColumn, 0);
					updateLinesNumbers();
					return TRUE;
				}
				default :
				{
					switch (HIWORD(wParam))
					{
						case EN_SETFOCUS :
						case BN_SETFOCUS :
							updateLinesNumbers();
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
}

void GoToLineDlg::updateLinesNumbers() const 
{
	unsigned int current = 0;
	unsigned int limit = 0;
	unsigned int currentColumn = 0; //variable to hold column number
	unsigned int rowLimit = 0; //variable to hold limit of lines
	unsigned int columnLimit = 0; //variable to hold limit of columns on last line
	unsigned int lastLineOffset = 0; //variable which holds the character offset at the start of last line
	
	if (_mode == go2line)
	{
		current = static_cast<unsigned int>((*_ppEditView)->getCurrentLineNumber() + 1);
		limit = static_cast<unsigned int>((*_ppEditView)->execute(SCI_GETLINECOUNT));
	}
	else if (_mode == go2column)
	{
		current = static_cast<unsigned int>((*_ppEditView)->getCurrentLineNumber() + 1);
		currentColumn = static_cast<unsigned int>((*_ppEditView)->getCurrentColumnNumber());

		rowLimit = static_cast<unsigned int>((*_ppEditView)->execute(SCI_GETLINECOUNT)); //Finds the limit of how many rows in the document
		columnLimit = static_cast<unsigned int>((*_ppEditView)->getCurrentDocLen() - 1); //Finds the offset of the last character in the document
		(*_ppEditView)->execute(SCI_GOTOLINE, rowLimit - 1); //Goes to the last line
		lastLineOffset = static_cast<unsigned int>((*_ppEditView)->execute(SCI_GETCURRENTPOS)); //Finds the offset of current position, the first character on the last line
		columnLimit = columnLimit + 1 - lastLineOffset; //Finds the total number of characters on the last column, giving the user a limit.
	}
	else
	{
		current = static_cast<unsigned int>((*_ppEditView)->execute(SCI_GETCURRENTPOS));
		limit = static_cast<unsigned int>((*_ppEditView)->getCurrentDocLen() - 1);

	}
	if (_mode == go2column) //updates the current position displayed on screen when using go to column option
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::string currentPosition = "Line: " + std::to_string(current) + " Column: " + std::to_string(currentColumn);
		std::wstring currentPositionW = converter.from_bytes(currentPosition); //conversion for setDlgItemText required argument type


		std::string limitPosition = "Line: " + std::to_string(rowLimit) + " Column: " + std::to_string(columnLimit);
		std::wstring limitPositionW = converter.from_bytes(limitPosition); //conversion for setDlgItemText required argument type
		::SetDlgItemText(_hSelf, ID_CURRLINE, currentPositionW.c_str());
		::SetDlgItemText(_hSelf, ID_LASTLINE, limitPositionW.c_str());
	}
	else
	{
		::SetDlgItemInt(_hSelf, ID_CURRLINE, current, FALSE);
		::SetDlgItemInt(_hSelf, ID_LASTLINE, limit, FALSE);
	}
	
}

