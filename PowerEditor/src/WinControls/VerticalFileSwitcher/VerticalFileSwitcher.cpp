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
#include "VerticalFileSwitcher.h"

BOOL CALLBACK VerticalFileSwitcher::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG :
        {
			_fileListView.init(_hInst, _hSelf, _hImaLst);
			_fileListView.initList();
			_fileListView.display();

            return TRUE;
        }

		case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->code)
			{
				case NM_CLICK:
				{
					LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lParam;
					int i = lpnmitem->iItem;

					if (i == -1)
						return TRUE;

					activateDoc(i);
					return TRUE;
				}

				case NM_RCLICK :
				{
					// Switch to the right document
					LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lParam;
					int i = lpnmitem->iItem;
					if (i == -1)
						return TRUE;

					activateDoc(i);

					// Redirect NM_RCLICK message to Notepad_plus handle
					NMHDR	nmhdr;
					nmhdr.code = NM_RCLICK;
					nmhdr.hwndFrom = _hSelf;
					nmhdr.idFrom = ::GetDlgCtrlID(nmhdr.hwndFrom);
					::SendMessage(_hParent, WM_NOTIFY, nmhdr.idFrom, (LPARAM)&nmhdr);
					return TRUE;
				}

				case LVN_GETINFOTIP:
				{
					LPNMLVGETINFOTIP pGetInfoTip = (LPNMLVGETINFOTIP)lParam;
					int i = pGetInfoTip->iItem;
					if (i == -1)
						return TRUE;
					generic_string fn = this->getFullFilePath((size_t)i);
					lstrcpyn(pGetInfoTip->pszText, fn.c_str(), pGetInfoTip->cchTextMax);
					return TRUE;
				}

				case LVN_KEYDOWN:
				{
					switch (((LPNMLVKEYDOWN)lParam)->wVKey)
					{
						case VK_RETURN:
						{
							int i = ListView_GetSelectionMark(_fileListView.getHSelf());

							if (i == -1)
								return TRUE;

							activateDoc(i);
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
			::MoveWindow(_fileListView.getHSelf(), 0, 0, width, height, TRUE);
            break;
        }

        default :
            return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
    }
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}


void VerticalFileSwitcher::activateDoc(int i) const
{
	int view = MAIN_VIEW;
	int bufferID = _fileListView.getBufferInfoFromIndex(i, view);
	int docPosInfo = ::SendMessage(_hParent, NPPM_GETPOSFROMBUFFERID, bufferID, view);
	int view2set = docPosInfo >> 30;
	int index2Switch = (docPosInfo << 2) >> 2 ;

	::SendMessage(_hParent, NPPM_ACTIVATEDOC, view2set, index2Switch);
}

