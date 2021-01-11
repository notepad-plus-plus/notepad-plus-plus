// This file is part of Notepad++ project
// Copyright (C)2020 Don HO <don.h@free.fr>
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


INT_PTR CALLBACK GoToLineDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			::SendDlgItemMessage(_hSelf, IDC_RADIO_GOTOLINE, BM_SETCHECK, TRUE, 0);
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
						else
						{
							int posToGoto = 0;
							if (line > 0)
							{
								// make sure not jumping into the middle of a multibyte character
								// or into the middle of a CR/LF pair for Windows files
								auto before = (*_ppEditView)->execute(SCI_POSITIONBEFORE, line);
								posToGoto = static_cast<int>((*_ppEditView)->execute(SCI_POSITIONAFTER, before));
							}
							auto sci_line = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, posToGoto);
							(*_ppEditView)->execute(SCI_ENSUREVISIBLE, sci_line);
							(*_ppEditView)->execute(SCI_GOTOPOS, posToGoto);
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
				{
				
					bool isLine, isOffset;
					if (wParam == IDC_RADIO_GOTOLINE)
					{
						isLine = true;
						isOffset = false;
						_mode = go2line;
					}
					else
					{
						isLine = false;
						isOffset = true;
						_mode = go2offsset;
					}
					::SendDlgItemMessage(_hSelf, IDC_RADIO_GOTOLINE, BM_SETCHECK, isLine, 0);
					::SendDlgItemMessage(_hSelf, IDC_RADIO_GOTOOFFSET, BM_SETCHECK, isOffset, 0);
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
	
	if (_mode == go2line)
	{
		current = static_cast<unsigned int>((*_ppEditView)->getCurrentLineNumber() + 1);
		limit = static_cast<unsigned int>((*_ppEditView)->execute(SCI_GETLINECOUNT));
	}
	else
	{
		current = static_cast<unsigned int>((*_ppEditView)->execute(SCI_GETCURRENTPOS));
		int currentDocLength = (*_ppEditView)->getCurrentDocLen();
		limit = static_cast<unsigned int>(currentDocLength > 0 ? currentDocLength - 1 : 0);
	}
    ::SetDlgItemInt(_hSelf, ID_CURRLINE, current, FALSE);
    ::SetDlgItemInt(_hSelf, ID_LASTLINE, limit, FALSE);
}

