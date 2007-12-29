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

#include "TabBar.h"
#include "SysMsg.h"

const COLORREF blue      	            = RGB(0,       0, 0xFF);
const COLORREF black     	            = RGB(0,       0,    0);
const COLORREF white     	            = RGB(0xFF, 0xFF, 0xFF);
const COLORREF grey      	            = RGB(128,   128,  128);

#define	IDC_DRAG_TAB     1404
#define	IDC_DRAG_INTERDIT_TAB 1405
#define	IDC_DRAG_PLUS_TAB 1406

bool TabBarPlus::_doDragNDrop = false;

bool TabBarPlus::_drawTopBar = true;
bool TabBarPlus::_drawInactiveTab = true;
bool TabBarPlus::_drawTabCloseButton = false;
bool TabBarPlus::_isDbClk2Close = false;

HWND TabBarPlus::_hwndArray[nbCtrlMax] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
int TabBarPlus::_nbCtrl = 0;

void TabBar::init(HINSTANCE hInst, HWND parent, bool isVertical, bool isTraditional, bool isMultiLine)
{
	Window::init(hInst, parent);
	int vertical = isVertical?(TCS_VERTICAL | TCS_MULTILINE | TCS_RIGHTJUSTIFY):0;
	_isTraditional = isTraditional;
	INITCOMMONCONTROLSEX icce;
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_TAB_CLASSES;
	InitCommonControlsEx(&icce);
    int multiLine = isMultiLine?(_isTraditional?TCS_MULTILINE:0):0;

	int style = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE |\
        /* WS_BORDER |*/TCS_FOCUSNEVER | TCS_TABS | vertical | multiLine;

	_hSelf = ::CreateWindowEx(
				0/*TCS_EX_FLATSEPARATORS*/ ,
				WC_TABCONTROL,
				"Tab",
				style,
				0, 0, 0, 0,
				_hParent,
				NULL,
				_hInst,
				0);

	if (!_hSelf)
	{
		systemMessage("System Err");
		throw int(69);
	}
	/*
	if (!_isTraditional)
    {
		if (!_hwndArray[_nbCtrl])
		{
			_hwndArray[_nbCtrl] = _hSelf;
			_ctrlID = _nbCtrl;
		}
		else 
		{
			int i = 0;
			bool found = false;
			for ( ; i < nbCtrlMax && !found ; i++)
				if (!_hwndArray[i])
					found = true;
			if (!found)
			{
				_ctrlID = -1;
				::MessageBox(NULL, "The nb of Tab Control is over its limit", "Tab Control err", MB_OK);
				destroy();
				throw int(96);
			}
			_hwndArray[i] = _hSelf;
			_ctrlID = i;
		}
		_nbCtrl++;
    }
	*/
}

int TabBar::insertAtEnd(const char *subTabName)
{
	TCITEM tie; 
	tie.mask = TCIF_TEXT | TCIF_IMAGE;
	int index = -1;

	if (_hasImgLst)
		index = 0;
	tie.iImage = index; 
	tie.pszText = (char *)subTabName; 
	return int(::SendMessage(_hSelf, TCM_INSERTITEM, _nbItem++, reinterpret_cast<LPARAM>(&tie)));
}

void TabBar::getCurrentTitle(char *title, int titleLen)
{
	TCITEM tci;
	tci.mask = TCIF_TEXT;
	tci.pszText = title;     
	tci.cchTextMax = titleLen-1;
	::SendMessage(_hSelf, TCM_GETITEM, getCurrentTabIndex(), reinterpret_cast<LPARAM>(&tci));
}

void TabBar::reSizeTo(RECT & rc2Ajust)
{
	// Important to do that!
	// Otherwise, the window(s) it contains will take all the resouce of CPU
	// We don't need to resiz the contained windows if they are even invisible anyway!
	display(rc2Ajust.right > 10);
	RECT rc = rc2Ajust;
	Window::reSizeTo(rc);
	TabCtrl_AdjustRect(_hSelf, FALSE, &rc2Ajust);
}

void TabBarPlus::init(HINSTANCE hInst, HWND parent, bool isVertical, bool isTraditional, bool isMultiLine)
{
	Window::init(hInst, parent);
	int vertical = isVertical?(TCS_VERTICAL | TCS_MULTILINE | TCS_RIGHTJUSTIFY):0;
	_isTraditional = isTraditional;
	INITCOMMONCONTROLSEX icce;
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_TAB_CLASSES;
	InitCommonControlsEx(&icce);
    int multiLine = isMultiLine?(_isTraditional?TCS_MULTILINE:0):0;

	int style = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE |\
        TCS_FOCUSNEVER | TCS_TABS | vertical | multiLine;

	//if (isOwnerDrawTab() && (!_isTraditional))
	{
		style |= TCS_OWNERDRAWFIXED;
		//printStr("ownerDraw");
	}
	_hSelf = ::CreateWindowEx(
				/*TCS_EX_FLATSEPARATORS */0,
				WC_TABCONTROL,
				"Tab",
				style,
				0, 0, 0, 0,
				_hParent,
				NULL,
				_hInst,
				0);

	if (!_hSelf)
	{
		systemMessage("System Err");
		throw int(69);
	}
	if (!_isTraditional)
    {
		if (!_hwndArray[_nbCtrl])
		{
			_hwndArray[_nbCtrl] = _hSelf;
			_ctrlID = _nbCtrl;
		}
		else 
		{
			int i = 0;
			bool found = false;
			for ( ; i < nbCtrlMax && !found ; i++)
				if (!_hwndArray[i])
					found = true;
			if (!found)
			{
				_ctrlID = -1;
				::MessageBox(NULL, "The nb of Tab Control is over its limit", "Tab Control err", MB_OK);
				destroy();
				throw int(96);
			}
			_hwndArray[i] = _hSelf;
			_ctrlID = i;
		}
		_nbCtrl++;

        ::SetWindowLong(_hSelf, GWL_USERDATA, reinterpret_cast<LONG>(this));
	    _tabBarDefaultProc = reinterpret_cast<WNDPROC>(::SetWindowLong(_hSelf, GWL_WNDPROC, reinterpret_cast<LONG>(TabBarPlus_Proc)));	 
    }

	if (vertical)
	{
		_hFont = ::CreateFont( 14, 0, 0, 0,
			                   FW_NORMAL,
				               0, 0, 0, 0,
				               0, 0, 0, 0,
					           "Comic Sans MS");
		if (_hFont)
			::SendMessage(_hSelf, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), 0);
	}
/*
	if (isOwnerDrawTab() && (!_isTraditional))
		::SendMessage(_hSelf, TCM_SETPADDING, 0, MAKELPARAM(6, 5));
*/	
}

LRESULT TabBarPlus::runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_LBUTTONDOWN :
		{
			if (_drawTabCloseButton)
			{
				int xPos = LOWORD(lParam);
				int yPos = HIWORD(lParam);

				if (_closeButtonZone.isHit(xPos, yPos, _currentHoverTabRect))
				{
					_whichCloseClickDown = getTabIndexAt(xPos, yPos);
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_REFRESHTABAR, 0);
					return TRUE;
				}
			}

            ::CallWindowProc(_tabBarDefaultProc, hwnd, Message, wParam, lParam);
			if (wParam == 2)
				return TRUE;



            if (_doDragNDrop)
            {
                _nSrcTab = _nTabDragged = ::SendMessage(_hSelf, TCM_GETCURSEL, 0, 0);
        
                POINT point;
			    point.x = LOWORD(lParam);
			    point.y = HIWORD(lParam);
			    if(::DragDetect(hwnd, point)) 
			    {
				    // Yes, we're beginning to drag, so capture the mouse...
				    _isDragging = true;
				    ::SetCapture(hwnd);
				    return TRUE;
			    }
			    break;
            }
            else
                return TRUE;
		}

		case WM_MOUSEMOVE :
		{
			if (_isDragging)
			{
				POINT p;
 				p.x = LOWORD(lParam);
				p.y = HIWORD(lParam);
                exchangeItemData(p);

				// Get cursor position of "Screen"
				// For using the function "WindowFromPoint" afterward!!!
				::GetCursorPos(&_draggingPoint);
				draggingCursor(_draggingPoint);
			    return TRUE;
			}
			
			if (_drawTabCloseButton)
			{
				int xPos = LOWORD(lParam);
				int yPos = HIWORD(lParam);

				int index = getTabIndexAt(xPos, yPos);
				
				if (index != -1)
				{
					::SendMessage(_hSelf, TCM_GETITEMRECT, index, (LPARAM)&_currentHoverTabRect);
					_currentHoverTabItem = index;
				}
				bool oldVal = _isCloseHover;
				_isCloseHover = _closeButtonZone.isHit(xPos, yPos, _currentHoverTabRect);
				if (oldVal != _isCloseHover)
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_REFRESHTABAR, 0);
				
			}

			break;
		}

		case WM_LBUTTONUP :
		{
            if (_isDragging)
			{
				if(::GetCapture() == _hSelf)
					::ReleaseCapture();

				// Send a notification message to the parent with wParam = 0, lParam = 0
				// nmhdr.idFrom = this
				// destIndex = this->_nSrcTab
				// scrIndex  = this->_nTabDragged
				NMHDR nmhdr;
				nmhdr.hwndFrom = _hSelf;
				nmhdr.code = _isDraggingInside?TCN_TABDROPPED:TCN_TABDROPPEDOUTSIDE;
	            nmhdr.idFrom = reinterpret_cast<unsigned int>(this);

				::SendMessage(_hParent, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&nmhdr));
				return TRUE;				
			}

			if (_drawTabCloseButton)
			{
				int xPos = LOWORD(lParam);
				int yPos = HIWORD(lParam);

				int currentTabOn = getTabIndexAt(xPos, yPos);

				if ((_whichCloseClickDown == currentTabOn) && _closeButtonZone.isHit(xPos, yPos, _currentHoverTabRect))
				{
					NMHDR nmhdr;
					nmhdr.hwndFrom = _hSelf;
					nmhdr.code = TCN_TABDELETE;
					nmhdr.idFrom = reinterpret_cast<unsigned int>(this);

					::CallWindowProc(_tabBarDefaultProc, hwnd, WM_LBUTTONDOWN, wParam, lParam);
					::SendMessage(_hParent, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&nmhdr));

					_whichCloseClickDown = -1;	
					return TRUE;
				}
				_whichCloseClickDown = -1;
			}

			break;
		}

		case WM_CAPTURECHANGED :
		{
			if (_isDragging)
			{
				_isDragging = false;
				return TRUE;
			}
			break;
		}

		case WM_DRAWITEM :
		{
			drawItem((DRAWITEMSTRUCT *)lParam);
			return TRUE;
		}

		case WM_KEYDOWN :
		{
			if (wParam == VK_LCONTROL)
				::SetCursor(::LoadCursor(_hInst, MAKEINTRESOURCE(IDC_DRAG_PLUS_TAB)));
			return TRUE;
		}

		case WM_MBUTTONUP:
		{
			::CallWindowProc(_tabBarDefaultProc, hwnd, WM_LBUTTONDOWN, wParam, lParam);

			NMHDR nmhdr;
			nmhdr.hwndFrom = _hSelf;
			nmhdr.code = TCN_TABDELETE;
			nmhdr.idFrom = reinterpret_cast<unsigned int>(this);

			::SendMessage(_hParent, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&nmhdr));
			return TRUE;
		}

		case WM_LBUTTONDBLCLK :
		{
			::CallWindowProc(_tabBarDefaultProc, hwnd, WM_LBUTTONDOWN, wParam, lParam);
			if (_isDbClk2Close)
			{
				NMHDR nmhdr;
				nmhdr.hwndFrom = _hSelf;
				nmhdr.code = TCN_TABDELETE;
				nmhdr.idFrom = reinterpret_cast<unsigned int>(this);

				::SendMessage(_hParent, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&nmhdr));
			}
			return TRUE;
		}
	}
	return ::CallWindowProc(_tabBarDefaultProc, hwnd, Message, wParam, lParam);
}

void TabBarPlus::drawItem(DRAWITEMSTRUCT *pDrawItemStruct)
{
	RECT rect = pDrawItemStruct->rcItem;
	
	int nTab = pDrawItemStruct->itemID;
	if (nTab < 0)
	{
		::MessageBox(NULL, "nTab < 0", "", MB_OK);
		//return ::CallWindowProc(_tabBarDefaultProc, hwnd, Message, wParam, lParam);
	}
	bool isSelected = (nTab == ::SendMessage(_hSelf, TCM_GETCURSEL, 0, 0));

	char label[MAX_PATH];
	TCITEM tci;
	tci.mask = TCIF_TEXT|TCIF_IMAGE;
	tci.pszText = label;     
	tci.cchTextMax = MAX_PATH-1;

	if (!::SendMessage(_hSelf, TCM_GETITEM, nTab, reinterpret_cast<LPARAM>(&tci))) 
	{
		::MessageBox(NULL, "! TCM_GETITEM", "", MB_OK);
		//return ::CallWindowProc(_tabBarDefaultProc, hwnd, Message, wParam, lParam);
	}
	HDC hDC = pDrawItemStruct->hDC;
	
	int nSavedDC = ::SaveDC(hDC);

	// For some bizarre reason the rcItem you get extends above the actual
	// drawing area. We have to workaround this "feature".
	rect.top += ::GetSystemMetrics(SM_CYEDGE);

	::SetBkMode(hDC, TRANSPARENT);
	HBRUSH hBrush = ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
	::FillRect(hDC, &rect, hBrush);
	::DeleteObject((HGDIOBJ)hBrush);

	if (isSelected)
	{
		if (_drawTopBar)
		{
			RECT barRect = rect;
			barRect.bottom = 6;

			hBrush = ::CreateSolidBrush(RGB(250, 170, 60));
			::FillRect(hDC, &barRect, hBrush);
			::DeleteObject((HGDIOBJ)hBrush);		
		}
	}
	else
	{
		if (_drawInactiveTab)
		{
			RECT barRect = rect;

			hBrush = ::CreateSolidBrush(RGB(192, 192, 192));
			::FillRect(hDC, &barRect, hBrush);
			::DeleteObject((HGDIOBJ)hBrush);
		}
	}

	if (_drawTabCloseButton)
	{
		RECT closeButtonRect = _closeButtonZone.getButtonRectFrom(rect);
		if (isSelected)
			closeButtonRect.left -= 2;
		
		// 3 status for each inactive tab and selected tab close item :
		// normal / hover / pushed
		int idCloseImg;

		if (_isCloseHover && (_currentHoverTabItem == nTab) && (_whichCloseClickDown == -1)) // hover
			idCloseImg = IDR_CLOSETAB_HOVER;
		else if (_isCloseHover && (_currentHoverTabItem == nTab) && (_whichCloseClickDown == _currentHoverTabItem)) // pushed
			idCloseImg = IDR_CLOSETAB_PUSH;
		else
			idCloseImg = isSelected?IDR_CLOSETAB:IDR_CLOSETAB_INACT;


		HDC hdcMemory;
		hdcMemory = ::CreateCompatibleDC(hDC);
		HBITMAP hBmp = ::LoadBitmap(_hInst, MAKEINTRESOURCE(idCloseImg));
		BITMAP bmp;
		::GetObject(hBmp, sizeof(bmp), &bmp);

		::SelectObject(hdcMemory, hBmp);
		::BitBlt(hDC, closeButtonRect.left, closeButtonRect.top, bmp.bmWidth, bmp.bmHeight, hdcMemory, 0, 0, SRCCOPY);
		::DeleteDC(hdcMemory);
		::DeleteObject(hBmp);
	}

	// Draw image
	HIMAGELIST hImgLst = (HIMAGELIST)::SendMessage(_hSelf, TCM_GETIMAGELIST, 0, 0);

	SIZE charPixel;
	::GetTextExtentPoint(hDC, " ", 1, &charPixel);
	int spaceUnit = charPixel.cx;

	if (hImgLst && tci.iImage >= 0)
	{
		IMAGEINFO info;
		
		ImageList_GetImageInfo(hImgLst, tci.iImage, &info);

		RECT & imageRect = info.rcImage;
		int yPos = (rect.top + (rect.bottom - rect.top)/2 + (isSelected?0:2)) - (imageRect.bottom - imageRect.top)/2;
		
		int marge = 0;

		if (isSelected)
		{
			marge = spaceUnit*2;
		}
		else
		{
			marge = spaceUnit;
		}

		rect.left += marge;
		ImageList_Draw(hImgLst, tci.iImage, hDC, rect.left, yPos, isSelected?ILD_TRANSPARENT:ILD_SELECTED);
		rect.left += imageRect.right - imageRect.left;
	}

	if (isSelected) 
	{
		COLORREF selectedColor = ::GetSysColor(COLOR_BTNTEXT);
		::SetTextColor(hDC, selectedColor);
		rect.top -= ::GetSystemMetrics(SM_CYEDGE);
		rect.top += 1;
		rect.left += _drawTabCloseButton?spaceUnit:0;
		::DrawText(hDC, label, strlen(label), &rect, DT_SINGLELINE | DT_VCENTER | (_drawTabCloseButton?DT_LEFT:DT_CENTER));
	} 
	else 
	{
		COLORREF unselectedColor = grey;
		::SetTextColor(hDC, unselectedColor);
		rect.left += _drawTabCloseButton?spaceUnit:0;
		::DrawText(hDC, label, strlen(label), &rect, DT_SINGLELINE| DT_BOTTOM | (_drawTabCloseButton?DT_LEFT:DT_CENTER));
	}
	::RestoreDC(hDC, nSavedDC);
}


void TabBarPlus::draggingCursor(POINT screenPoint)
{
	HWND hWin = ::WindowFromPoint(screenPoint);
	if (_hSelf == hWin)
		::SetCursor(::LoadCursor(NULL, IDC_ARROW));
	else
	{
		char className[256];
		::GetClassName(hWin, className, 256);
		if ((!strcmp(className, "Scintilla")) || (!strcmp(className, WC_TABCONTROL)))
		{
			if (::GetKeyState(VK_LCONTROL) & 0x80000000)
				::SetCursor(::LoadCursor(_hInst, MAKEINTRESOURCE(IDC_DRAG_PLUS_TAB)));
			else
				::SetCursor(::LoadCursor(_hInst, MAKEINTRESOURCE(IDC_DRAG_TAB)));
		}
		else
			::SetCursor(::LoadCursor(_hInst, MAKEINTRESOURCE(IDC_DRAG_INTERDIT_TAB)));
	}
}

void TabBarPlus::exchangeItemData(POINT point)
{
	// Find the destination tab...
	int nTab = getTabIndexAt(point);

	// The position is over a tab.
	//if (hitinfo.flags != TCHT_NOWHERE)
	if (nTab != -1)
	{
		
		_isDraggingInside = true;

		if (nTab != _nTabDragged)
		{
			//1. set to focus
			::SendMessage(_hSelf, TCM_SETCURSEL, nTab, 0);

			//2. shift their data, and insert the source
			TCITEM itemData_nDraggedTab, itemData_shift;
			itemData_nDraggedTab.mask = itemData_shift.mask = TCIF_IMAGE | TCIF_TEXT;
			char str1[256];
			char str2[256];

			itemData_nDraggedTab.pszText = str1;
			itemData_nDraggedTab.cchTextMax = (sizeof(str1));

			itemData_shift.pszText = str2;
			itemData_shift.cchTextMax = (sizeof(str2));

			::SendMessage(_hSelf, TCM_GETITEM, _nTabDragged, reinterpret_cast<LPARAM>(&itemData_nDraggedTab));

			if (_nTabDragged > nTab)
			{
				for (int i = _nTabDragged ; i > nTab ; i--)
				{
					::SendMessage(_hSelf, TCM_GETITEM, i-1, reinterpret_cast<LPARAM>(&itemData_shift));
					::SendMessage(_hSelf, TCM_SETITEM, i, reinterpret_cast<LPARAM>(&itemData_shift));
				}
			}
			else
			{
				for (int i = _nTabDragged ; i < nTab ; i++)
				{
					::SendMessage(_hSelf, TCM_GETITEM, i+1, reinterpret_cast<LPARAM>(&itemData_shift));
					::SendMessage(_hSelf, TCM_SETITEM, i, reinterpret_cast<LPARAM>(&itemData_shift));
				}
			}
			//
			::SendMessage(_hSelf, TCM_SETITEM, nTab, reinterpret_cast<LPARAM>(&itemData_nDraggedTab));

			//3. update the current index
			_nTabDragged = nTab;
			
		}
	}
	else
	{
		//::SetCursor(::LoadCursor(_hInst, MAKEINTRESOURCE(IDC_DRAG_TAB)));
		_isDraggingInside = false;
	}
	
}
