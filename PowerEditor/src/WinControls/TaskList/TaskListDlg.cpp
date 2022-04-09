// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.



#include "TaskListDlg.h"
#include "Parameters.h"
#include "resource.h"

int TaskListDlg::_instanceCount = 0;

LRESULT CALLBACK hookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if ((nCode >= 0) && (wParam == WM_RBUTTONUP))
    {
		::PostMessage(hWndServer, WM_RBUTTONUP, 0, 0);
    }
	else if ((nCode >= 0) && (wParam == WM_MOUSEWHEEL) && windowsVersion >= WV_WIN10)
	{
		MSLLHOOKSTRUCT* pMD = (MSLLHOOKSTRUCT*)lParam;
		RECT rCtrl;
		GetWindowRect(hWndServer, &rCtrl);
		//to avoid duplicate messages, only send this message to the list control if it comes from outside the control window. if the message occurs whilst the mouse is inside the control, the control will have receive the mouse wheel message itself
		if (false == PtInRect(&rCtrl, pMD->pt))
		{
			::PostMessage(hWndServer, WM_MOUSEWHEEL, (WPARAM)pMD->mouseData, MAKELPARAM(pMD->pt.x, pMD->pt.y));
		}
	}

	return ::CallNextHookEx(hook, nCode, wParam, lParam);
}

 int TaskListDlg::doDialog(bool isRTL) 
 {
	if (isRTL)
	{
		DLGTEMPLATE *pMyDlgTemplate = NULL;
		HGLOBAL hMyDlgTemplate = makeRTLResource(IDD_VALUE_DLG, &pMyDlgTemplate);
		int result = static_cast<int32_t>(::DialogBoxIndirectParam(_hInst, pMyDlgTemplate, _hParent, dlgProc, reinterpret_cast<LPARAM>(this)));
		::GlobalFree(hMyDlgTemplate);
		return result;
	}
	return static_cast<int32_t>(::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_TASKLIST_DLG), _hParent, dlgProc, reinterpret_cast<LPARAM>(this)));
}

intptr_t CALLBACK TaskListDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG :
		{
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			::SendMessage(_hParent, WM_GETTASKLISTINFO, reinterpret_cast<WPARAM>(&_taskListInfo), 0);
			int nbTotal = static_cast<int32_t>(_taskListInfo._tlfsLst.size());

			int i2set = _taskListInfo._currentIndex + (_initDir == dirDown?1:-1);
			
			if (i2set < 0)
				i2set = nbTotal - 1;

			if (i2set > (nbTotal - 1))
				i2set = 0;

			_taskList.init(_hInst, _hSelf, _hImalist, nbTotal, i2set);
			_taskList.setFont(TEXT("Verdana"), NppParameters::getInstance()._dpiManager.scaleY(14));
			_rc = _taskList.adjustSize();

			reSizeTo(_rc);
			goToCenter();

			_taskList.display(true);
			hWndServer = _hSelf;
			windowsVersion = NppParameters::getInstance().getWinVersion();

#ifndef WH_MOUSE_LL
#define WH_MOUSE_LL 14
#endif
			_hHooker = ::SetWindowsHookEx(WH_MOUSE_LL, hookProc, _hInst, 0);
			hook = _hHooker;
			return FALSE;
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_DESTROY :
		{
			_taskList.destroy();
			::UnhookWindowsHookEx(_hHooker);
			_instanceCount--;
			return TRUE;
		}


		case WM_RBUTTONUP:
		{
			::SendMessage(_hSelf, WM_COMMAND, ID_PICKEDUP, _taskList.getCurrentIndex());
			return TRUE;
		}
		
		case WM_MOUSEWHEEL:
		{
			::SendMessage(_taskList.getHSelf(), WM_MOUSEWHEEL, wParam, lParam);
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
					LV_ITEM &lvItem = reinterpret_cast<LV_DISPINFO*>(reinterpret_cast<LV_DISPINFO FAR *>(lParam))->item;

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
					auto listIndex = lParam;
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

	const int aSpaceWidth = ListView_GetStringWidth(_taskList.getHSelf(), TEXT(" "));

	COLORREF textColor = NppDarkMode::isEnabled() ? NppDarkMode::getDarkerTextColor() : darkGrey;
	int imgStyle = ILD_SELECTED;

	if (lpDrawItemStruct->itemState & ODS_SELECTED)
	{
		imgStyle = ILD_TRANSPARENT;
		textColor = NppDarkMode::isEnabled() ? NppDarkMode::getTextColor() : black;
		::SelectObject(hDC, _taskList.GetFontSelected());
	}
	
	//
	// DRAW IMAGE
	//
	HIMAGELIST hImgLst = _taskList.getImgLst();

	IMAGEINFO info;
	::ImageList_GetImageInfo(hImgLst, iImage, &info);

	RECT & imageRect = info.rcImage;
	// center icon position, prefer bottom orientation
	imageRect.top = ((rect.bottom - rect.top) - (imageRect.bottom - imageRect.top) + 1) / 2;

	rect.left += aSpaceWidth;
	::ImageList_Draw(hImgLst, iImage, hDC, rect.left, rect.top + imageRect.top, imgStyle);
	rect.left += imageRect.right - imageRect.left + aSpaceWidth * 2;

	//
	// DRAW TEXT
	//
	::SetTextColor(hDC, textColor);
	::DrawText(hDC, label, lstrlen(label), &rect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
}
