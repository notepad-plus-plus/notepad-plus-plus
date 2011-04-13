/*
this file is part of notepad++
Copyright (C)2011 Don HO <donho@altern.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a Copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "precompiledHeaders.h"
#include "ansiCharPanel.h"
#include "ScintillaEditView.h"

BOOL CALLBACK AnsiCharPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG :
        {
            TCHAR asciiStr[2];
            asciiStr[1] = '\0';
			
			_listView.init(_hInst, _hSelf);
			int codepage = (*_ppEditView)->getCurrentBuffer()->getEncoding();
			_listView.resetValues(codepage==-1?0:codepage);
			_listView.display();

            return TRUE;
        }

		case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->code)
			{
				case NM_DBLCLK:
				{
					//printStr(TEXT("OK"));
					LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lParam;
					int i = lpnmitem->iItem;

					if (i == -1)
						return TRUE;

                    char charStr[2];
                    charStr[0] = (unsigned char)i;
                    charStr[1] = '\0';
                    wchar_t wCharStr[10];
                    char multiByteStr[10];
					int codepage = (*_ppEditView)->getCurrentBuffer()->getEncoding();
					if (codepage == -1)
					{
						multiByteStr[0] = charStr[0];
						multiByteStr[1] = charStr[1];
					}
					else
					{
						MultiByteToWideChar(codepage, 0, charStr, -1, wCharStr, sizeof(wCharStr));
						WideCharToMultiByte(CP_UTF8, 0, wCharStr, -1, multiByteStr, sizeof(multiByteStr), NULL, NULL);
					}
					(*_ppEditView)->execute(SCI_REPLACESEL, 0, (LPARAM)"");
					int len = (i < 128)?1:strlen(multiByteStr);
                    (*_ppEditView)->execute(SCI_ADDTEXT, len, (LPARAM)multiByteStr);

					return TRUE;
				}
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
