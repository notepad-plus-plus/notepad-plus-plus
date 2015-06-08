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


#include "ansiCharPanel.h"
#include "ScintillaEditView.h"

void AnsiCharPanel::switchEncoding()
{
	int codepage = (*_ppEditView)->getCurrentBuffer()->getEncoding();
	_listView.resetValues(codepage);
}

INT_PTR CALLBACK AnsiCharPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG :
        {
			_listView.init(_hInst, _hSelf);
			int codepage = (*_ppEditView)->getCurrentBuffer()->getEncoding();
			_listView.setValues(codepage==-1?0:codepage);
			_listView.display();

            return TRUE;
        }

		case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->code)
			{
				case NM_DBLCLK:
				{
					LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lParam;
					int i = lpnmitem->iItem;

					if (i == -1)
						return TRUE;

					insertChar((unsigned char)i);
					return TRUE;
				}

				case LVN_KEYDOWN:
				{
					switch (((LPNMLVKEYDOWN)lParam)->wVKey)
					{
						case VK_RETURN:
						{
							int i = ListView_GetSelectionMark(_listView.getHSelf());

							if (i == -1)
								return TRUE;

							insertChar((unsigned char)i);
							return TRUE;
						}
						default:
							break;
					}
				}
				break;

				default:
					break;
			}
		}
		return TRUE;

        case WM_SIZE:
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
			::MoveWindow(_listView.getHSelf(), 0, 0, width, height, TRUE);
            break;
        }

        default :
            return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
    }
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}

void AnsiCharPanel::insertChar(unsigned char char2insert) const
{
    char charStr[2];
    charStr[0] = char2insert;
    charStr[1] = '\0';
    wchar_t wCharStr[10];
    char multiByteStr[10];
	int codepage = (*_ppEditView)->getCurrentBuffer()->getEncoding();
	if (codepage == -1)
	{
		bool isUnicode = ((*_ppEditView)->execute(SCI_GETCODEPAGE) == SC_CP_UTF8);
		if (isUnicode)
		{
			MultiByteToWideChar(0, 0, charStr, -1, wCharStr, sizeof(wCharStr));
			WideCharToMultiByte(CP_UTF8, 0, wCharStr, -1, multiByteStr, sizeof(multiByteStr), NULL, NULL);
		}
		else // ANSI
		{
			multiByteStr[0] = charStr[0];
			multiByteStr[1] = charStr[1];
		}
	}
	else
	{
		MultiByteToWideChar(codepage, 0, charStr, -1, wCharStr, sizeof(wCharStr));
		WideCharToMultiByte(CP_UTF8, 0, wCharStr, -1, multiByteStr, sizeof(multiByteStr), NULL, NULL);
	}
	(*_ppEditView)->execute(SCI_REPLACESEL, 0, (LPARAM)"");
	int len = (char2insert < 128)?1:strlen(multiByteStr);
    (*_ppEditView)->execute(SCI_ADDTEXT, len, (LPARAM)multiByteStr);
	(*_ppEditView)->getFocus();
}