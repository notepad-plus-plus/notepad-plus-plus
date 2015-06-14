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



#include "VerticalFileSwitcher.h"
#include "menuCmdID.h"
#include "Parameters.h"
//#include "localization.h"

int CALLBACK ListViewCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	LPNMLISTVIEW pnmListView = (LPNMLISTVIEW)lParamSort;
	TCHAR str1[MAX_PATH];
	TCHAR str2[MAX_PATH];

	ListView_GetItemText(pnmListView->hdr.hwndFrom, lParam1, pnmListView->iSubItem, str1, sizeof(str1));
	ListView_GetItemText(pnmListView->hdr.hwndFrom, lParam2, pnmListView->iSubItem, str2, sizeof(str2));

	LVCOLUMN lvc;
	lvc.mask = LVCF_FMT;
	::SendMessage(pnmListView->hdr.hwndFrom, LVM_GETCOLUMN, (WPARAM)pnmListView->iSubItem, (LPARAM)&lvc);
	bool isDirectionUp = (HDF_SORTUP & lvc.fmt) != 0;

	int result = lstrcmp(str1, str2);

	if (isDirectionUp)
		return result;

	return (0 - result);
};

INT_PTR CALLBACK VerticalFileSwitcher::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
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
				case NM_DBLCLK:
				{
					LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lParam;
					int i = lpnmitem->iItem;
					if (i == -1)
					{
						::SendMessage(_hParent, WM_COMMAND, IDM_FILE_NEW, 0);
					}
					return TRUE;
				}

				case NM_CLICK:
				{
					if ((0x80 & GetKeyState(VK_CONTROL)) || (0x80 & GetKeyState(VK_SHIFT)))
						return TRUE;

					LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lParam;
					int nbItem = ListView_GetItemCount(_fileListView.getHSelf());
					int i = lpnmitem->iItem;
					if (i == -1 || i >= nbItem)
						return TRUE;

					LVITEM item;
					item.mask = LVIF_PARAM;
					item.iItem = i;	
					ListView_GetItem(((LPNMHDR)lParam)->hwndFrom, &item);
					TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;

					activateDoc(tlfs);
					return TRUE;
				}

				case NM_RCLICK :
				{
					// Switch to the right document
					LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lParam;
					int nbItem = ListView_GetItemCount(_fileListView.getHSelf());

					if (nbSelectedFiles() == 1)
					{
						int i = lpnmitem->iItem;
						if (i == -1 || i >= nbItem)
 							return TRUE;

						LVITEM item;
						item.mask = LVIF_PARAM;
						item.iItem = i;	
						ListView_GetItem(((LPNMHDR)lParam)->hwndFrom, &item);
						TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;

						activateDoc(tlfs);
					}
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
					generic_string fn = getFullFilePath((size_t)i);
					lstrcpyn(pGetInfoTip->pszText, fn.c_str(), pGetInfoTip->cchTextMax);
					return TRUE;
				}

				case LVN_COLUMNCLICK:
				{
					LPNMLISTVIEW pnmLV = (LPNMLISTVIEW)lParam;
					setHeaderOrder(pnmLV);
					ListView_SortItemsEx(pnmLV->hdr.hwndFrom, ListViewCompareProc,(LPARAM)pnmLV);
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

							LVITEM item;
							item.mask = LVIF_PARAM;
							item.iItem = i;	
							ListView_GetItem(((LPNMHDR)lParam)->hwndFrom, &item);
							TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;
							activateDoc(tlfs);
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
			_fileListView.resizeColumns(width);
            break;
        }
        
		case WM_DESTROY:
        {
			_fileListView.destroy();
            break;
        }

        default :
            return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
    }
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}


void VerticalFileSwitcher::activateDoc(TaskLstFnStatus *tlfs) const
{
	int view = tlfs->_iView;
	int bufferID = (int)tlfs->_bufID;
	
	int currentView = ::SendMessage(_hParent, NPPM_GETCURRENTVIEW, 0, 0);
	int currentBufID = ::SendMessage(_hParent, NPPM_GETCURRENTBUFFERID, 0, 0);

	if (bufferID == currentBufID && view == currentView)
		return;
	
	int docPosInfo = ::SendMessage(_hParent, NPPM_GETPOSFROMBUFFERID, bufferID, view);
	int view2set = docPosInfo >> 30;
	int index2Switch = (docPosInfo << 2) >> 2 ;

	::SendMessage(_hParent, NPPM_ACTIVATEDOC, view2set, index2Switch);
}

int VerticalFileSwitcher::setHeaderOrder(LPNMLISTVIEW pnm_list_view)
{
	HWND hListView,colHeader;
	LVCOLUMN lvc;
	int q,cols;
	int index = pnm_list_view->iSubItem;

	lvc.mask = LVCF_FMT;
	hListView = pnm_list_view->hdr.hwndFrom;
	SendMessage(hListView, LVM_GETCOLUMN, (WPARAM)index, (LPARAM)&lvc);
	if(HDF_SORTUP & lvc.fmt)
	{
		//set the opposite arrow
		lvc.fmt = lvc.fmt & (~HDF_SORTUP) | HDF_SORTDOWN; //turns off sort-up, turns on sort-down
		SendMessage(hListView, LVM_SETCOLUMN, (WPARAM) index, (LPARAM) &lvc);
		//use any sorting you would use, e.g. the LVM_SORTITEMS message
		return SORT_DIRECTION_DOWN;
	}

	if(HDF_SORTDOWN & lvc.fmt)
    {
		//the opposite
		lvc.fmt = lvc.fmt & (~HDF_SORTDOWN) | HDF_SORTUP;
		SendMessage(hListView, LVM_SETCOLUMN, (WPARAM) index, (LPARAM) &lvc);
		return SORT_DIRECTION_UP;
    }
  
	// this is the case our clicked column wasn't the one being sorted up until now
	// so first  we need to iterate through all columns and send LVM_SETCOLUMN to them with fmt set to NOT include these HDFs
	colHeader = (HWND)SendMessage(hListView,LVM_GETHEADER,0,0);
	cols = SendMessage(colHeader,HDM_GETITEMCOUNT,0,0);
	for(q=0; q<cols; ++q)
	{
		//Get current fmt
		SendMessage(hListView,LVM_GETCOLUMN,(WPARAM) q, (LPARAM) &lvc);
		//remove both sort-up and sort-down
		lvc.fmt = lvc.fmt & (~HDF_SORTUP) & (~HDF_SORTDOWN);
		SendMessage(hListView,LVM_SETCOLUMN,(WPARAM) q, (LPARAM) &lvc);
	}
	
	//read current fmt from clicked column
	SendMessage(hListView,LVM_GETCOLUMN,(WPARAM) index, (LPARAM) &lvc);
	// then set whichever arrow you feel like and send LVM_SETCOLUMN to this particular column
	lvc.fmt = lvc.fmt | HDF_SORTUP;
	SendMessage(hListView, LVM_SETCOLUMN, (WPARAM) index, (LPARAM) &lvc);

	return SORT_DIRECTION_UP;
}
