//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef TASKLISTDLG_H
#define TASKLISTDLG_H

#include "StaticDialog.h"
#include "TaskListDlg_rc.h"
#include "TaskList.h"
#include "ImageListSet.h"
#include "Notepad_plus_msgs.h"

const bool dirUp = true;
const bool dirDown = false;

#define	TASKLIST_USER    (WM_USER + 8000)
	#define WM_GETTASKLISTINFO (TASKLIST_USER + 01)

struct TaskLstFnStatus {
	int _iView;
	int _docIndex;
	generic_string _fn;
	int _status;
	TaskLstFnStatus(generic_string str, int status) : _fn(str), _status(status){};
	TaskLstFnStatus(int iView, int docIndex, generic_string str, int status) : _iView(iView), _docIndex(docIndex), _fn(str), _status(status) {};
};

struct TaskListInfo {
	vector<TaskLstFnStatus> _tlfsLst;
	int _currentIndex;
};

static HWND hWndServer = NULL;
static HHOOK hook = NULL;

static LRESULT CALLBACK hookProc(UINT nCode, WPARAM wParam, LPARAM lParam)
{
	if ((nCode >= 0) && (wParam == WM_RBUTTONUP))
    {
		::PostMessage(hWndServer, WM_RBUTTONUP, 0, 0);
    }        
	
	return ::CallNextHookEx(hook, nCode, wParam, lParam);
};

class TaskListDlg : public StaticDialog
{
public :	
        TaskListDlg() : StaticDialog() {};
		void init(HINSTANCE hInst, HWND parent, HIMAGELIST hImgLst, bool dir) {
            Window::init(hInst, parent);
			_hImalist = hImgLst;
			_initDir = dir;
        };

        int doDialog(bool isRTL = false) {
			if (isRTL)
			{
				DLGTEMPLATE *pMyDlgTemplate = NULL;
				HGLOBAL hMyDlgTemplate = makeRTLResource(IDD_VALUE_DLG, &pMyDlgTemplate);
				int result = ::DialogBoxIndirectParam(_hInst, pMyDlgTemplate, _hParent,  (DLGPROC)dlgProc, (LPARAM)this);
				::GlobalFree(hMyDlgTemplate);
				return result;
			}
			return ::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_TASKLIST_DLG), _hParent,  (DLGPROC)dlgProc, (LPARAM)this);
        };

		virtual void destroy() {};

protected :
	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam) {

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
				winVer ver = (NppParameters::getInstance())->getWinVersion();
				_hHooker = ::SetWindowsHookEx(ver >= WV_W2K?WH_MOUSE_LL:WH_MOUSE, (HOOKPROC)hookProc, _hInst, 0);
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
	};

private :
	TaskList _taskList;
	TaskListInfo _taskListInfo;
	HIMAGELIST _hImalist;
	bool _initDir;
	HHOOK _hHooker;

	void drawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
	{
		RECT rect = lpDrawItemStruct->rcItem;
		HDC hDC = lpDrawItemStruct->hDC;
		int nItem = lpDrawItemStruct->itemID;
		const TCHAR *label = _taskListInfo._tlfsLst[nItem]._fn.c_str();
		int iImage = _taskListInfo._tlfsLst[nItem]._status;
		
		COLORREF textColor = darkGrey;
		int imgStyle = ILD_SELECTED;

		if (lpDrawItemStruct->itemState && ODS_SELECTED)
		{
			imgStyle = ILD_TRANSPARENT;
			textColor = black;

			HFONT selectedFont = (HFONT)::GetStockObject(SYSTEM_FONT);
			::SelectObject(hDC, selectedFont);
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
	};

};



#endif // TASKLISTDLG_H
