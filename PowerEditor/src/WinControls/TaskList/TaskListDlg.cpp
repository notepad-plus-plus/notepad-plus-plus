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



#include "TaskListDlg.h"
#include "Parameters.h"
#include "resource.h"

LRESULT CALLBACK hookProc(UINT nCode, WPARAM wParam, LPARAM lParam)
{
	if ((nCode >= 0) && (wParam == WM_RBUTTONUP))
    {
		::PostMessage(hWndServer, WM_RBUTTONUP, 0, 0);
    }        
	
	return ::CallNextHookEx(hook, nCode, wParam, lParam);
}

 int TaskListDlg::doDialog(bool isRTL) 
 {
	if (isRTL)
	{
		DLGTEMPLATE *pMyDlgTemplate = NULL;
		HGLOBAL hMyDlgTemplate = makeRTLResource(IDD_VALUE_DLG, &pMyDlgTemplate);
		int result = ::DialogBoxIndirectParam(_hInst, pMyDlgTemplate, _hParent,  dlgProc, (LPARAM)this);
		::GlobalFree(hMyDlgTemplate);
		return result;
	}
	return ::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_TASKLIST_DLG), _hParent,  dlgProc, (LPARAM)this);
}

INT_PTR CALLBACK TaskListDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG :
		{
			::SendMessage(_hParent, WM_GETTASKLISTINFO, (WPARAM)&_taskListInfo, 0);
			int nbTotal = _taskListInfo._tlfsLst.size();

			int i2set = _taskListInfo._currentIndex + (_initDir == dirDown?1:-1);
			
			if (i2set < 0)
				i2set = nbTotal - 1;

			if (i2set > (nbTotal - 1))
				i2set = 0;

			_taskList.init(_hInst, _hSelf, _hImalist, nbTotal, i2set);
			_taskList.setFont(TEXT("Verdana"), 14);
			_rc = _taskList.adjustSize();

			reSizeTo(_rc);
			goToCenter();

			_taskList.display(true);
			hWndServer = _hSelf;

#ifndef WH_MOUSE_LL
#define WH_MOUSE_LL 14
#endif
			_hHooker = ::SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC)hookProc, _hInst, 0);
			hook = _hHooker;
			return FALSE;
		}

		case WM_DESTROY :
		{
			_taskList.destroy();
			::UnhookWindowsHookEx(_hHooker);
			return TRUE;
		}


		case WM_RBUTTONUP:
		{
			::SendMessage(_hSelf, WM_COMMAND, ID_PICKEDUP, _taskList.getCurrentIndex());
			return TRUE;
		}
		

		case WM_DRAWITEM :
		{
			drawItem((DRAWITEMSTRUCT *)lParam);
			return TRUE;
		}

		case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->code)
			{
				case LVN_GETDISPINFO:
				{
					LV_ITEM &lvItem = reinterpret_cast<LV_DISPINFO*>((LV_DISPINFO FAR *)lParam)->item;

					TaskLstFnStatus & fileNameStatus = _taskListInfo._tlfsLst[lvItem.iItem];

					lvItem.pszText = (TCHAR *)fileNameStatus._fn.c_str();
					lvItem.iImage = fileNameStatus._status;

					return TRUE;
				}
		
				case NM_CLICK :
				case NM_RCLICK :
				{
					::SendMessage(_hSelf, WM_COMMAND, ID_PICKEDUP, _taskList.updateCurrentIndex());
					return TRUE;
				}

				default:
					break;
			}
			break;
		}

		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case ID_PICKEDUP :
				{
					int listIndex = lParam;
					int view2set = _taskListInfo._tlfsLst[listIndex]._iView;
					int index2Switch = _taskListInfo._tlfsLst[listIndex]._docIndex;
					::SendMessage(_hParent, NPPM_ACTIVATEDOC, view2set, index2Switch);
					::EndDialog(_hSelf, -1);
					return TRUE;
				}

				default:
					return FALSE;
			}
		}

		default :
			return FALSE;
	}

	return FALSE;
}

void TaskListDlg::drawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	RECT rect = lpDrawItemStruct->rcItem;
	HDC hDC = lpDrawItemStruct->hDC;
	int nItem = lpDrawItemStruct->itemID;
	const TCHAR *label = _taskListInfo._tlfsLst[nItem]._fn.c_str();
	int iImage = _taskListInfo._tlfsLst[nItem]._status;
	
	COLORREF textColor = darkGrey;
	int imgStyle = ILD_SELECTED;

	if (lpDrawItemStruct->itemState & ODS_SELECTED)
	{
		imgStyle = ILD_TRANSPARENT;
		textColor = black;
		::SelectObject(hDC, _taskList.GetFontSelected());
	}
	
	//
	// DRAW IMAGE
	//
	HIMAGELIST hImgLst = _taskList.getImgLst();

	IMAGEINFO info;
	ImageList_GetImageInfo(hImgLst, iImage, &info);

	RECT & imageRect = info.rcImage;
	//int yPos = (rect.top + (rect.bottom - rect.top)/2 + (isSelected?0:2)) - (imageRect.bottom - imageRect.top)/2;
	
	SIZE charPixel;
	::GetTextExtentPoint(hDC, TEXT(" "), 1, &charPixel);
	int spaceUnit = charPixel.cx;
	int marge = spaceUnit;

	rect.left += marge;
	ImageList_Draw(hImgLst, iImage, hDC, rect.left, rect.top, imgStyle);
	rect.left += imageRect.right - imageRect.left + spaceUnit * 2;

	//
	// DRAW TEXT
	//
	::SetTextColor(hDC, textColor);
	rect.top -= ::GetSystemMetrics(SM_CYEDGE);
		
	::DrawText(hDC, label, lstrlen(label), &rect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
}
