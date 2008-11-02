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

#ifndef TAB_BAR_H
#define TAB_BAR_H

#include "Window.h"

#ifndef _WIN32_IE
#define _WIN32_IE	0x0600
#endif //_WIN32_IE

//Notification message
#define TCN_TABDROPPED (TCN_FIRST - 10)
#define TCN_TABDROPPEDOUTSIDE (TCN_FIRST - 11)
#define TCN_TABDELETE (TCN_FIRST - 12)

#define WM_TABSETSTYLE	(WM_APP + 0x024)

const int marge = 8;
const int nbCtrlMax = 10;

#include <commctrl.h>
#include "menuCmdID.h"
#include "resource.h"

const TCHAR TABBAR_ACTIVEFOCUSEDINDCATOR[64] = TEXT("Active tab focused indicator");
const TCHAR TABBAR_ACTIVEUNFOCUSEDINDCATOR[64] = TEXT("Active tab unfocused indicator");
const TCHAR TABBAR_ACTIVETEXT[64] = TEXT("Active tab text");
const TCHAR TABBAR_INACTIVETEXT[64] = TEXT("Inactive tabs");

struct TBHDR {
	NMHDR hdr;
	int tabOrigin;
};

class TabBar : public Window
{
public:
	TabBar() : Window(), _nbItem(0), _hasImgLst(false), _hFont(NULL){};

	virtual ~TabBar() {};

	virtual void destroy(){
		if (_hFont)
			DeleteObject(_hFont);

		if (_hLargeFont)
			DeleteObject(_hLargeFont);

		if (_hVerticalFont)
			DeleteObject(_hVerticalFont);

		if (_hVerticalLargeFont)
			DeleteObject(_hVerticalLargeFont);

		::DestroyWindow(_hSelf);
		_hSelf = NULL;
	};

	virtual void init(HINSTANCE hInst, HWND hwnd, bool isVertical = false, bool isTraditional = false, bool isMultiLine = false);

	virtual void reSizeTo(RECT & rc2Ajust);
	
	int insertAtEnd(const TCHAR *subTabName);

	void activateAt(int index) const {
		if (getCurrentTabIndex() != index) {
			::SendMessage(_hSelf, TCM_SETCURSEL, index, 0);}
			TBHDR nmhdr;
			nmhdr.hdr.hwndFrom = _hSelf;
			nmhdr.hdr.code = TCN_SELCHANGE;
			nmhdr.hdr.idFrom = reinterpret_cast<unsigned int>(this);
			nmhdr.tabOrigin = index;
		
	};
	void getCurrentTitle(TCHAR *title, int titleLen);

	int getCurrentTabIndex() const {
		return ::SendMessage(_hSelf, TCM_GETCURSEL, 0, 0);
	};
	void deletItemAt(int index);

	void deletAllItem() {
		::SendMessage(_hSelf, TCM_DELETEALLITEMS, 0, 0);
		_nbItem = 0;
	};

	void setImageList(HIMAGELIST himl) {
		_hasImgLst = true;
		::SendMessage(_hSelf, TCM_SETIMAGELIST, 0, (LPARAM)himl);
	};
    
    int nbItem() const {
        return _nbItem;
    };

	void setFont(TCHAR *fontName, size_t fontSize) {
		if (_hFont)
			::DeleteObject(_hFont);

		_hFont = ::CreateFont( fontSize, 0, 
							  (_isVertical) ? 900:0,
							  (_isVertical) ? 900:0,
			                   FW_NORMAL,
				               0, 0, 0, 0,
				               0, 0, 0, 0,
					           fontName);
		if (_hFont)
			::SendMessage(_hSelf, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), 0);
	};
		
	void setVertical(bool b) {
		_isVertical = b;
	};
	
	void setMultiLine(bool b) {
		_isMultiLine = b;
	};


protected:
	size_t _nbItem;
	bool _hasImgLst;
	HFONT _hFont;
	HFONT _hLargeFont;
	HFONT _hVerticalFont;
	HFONT _hVerticalLargeFont;

	int _ctrlID;
	bool _isTraditional;

	bool _isVertical;
	bool _isMultiLine;
	
	long getRowCount() const {
		return long(::SendMessage(_hSelf, TCM_GETROWCOUNT, 0, 0));
	};
};


struct CloseButtonZone {

	CloseButtonZone(): _width(11), _hight(11), _fromTop(5), _fromRight(3){};

	bool isHit(int x, int y, const RECT & testZone) const {
		if (((x + _width + _fromRight) < testZone.right) || (x > (testZone.right - _fromRight)))
			return false;

		if (((y - _hight - _fromTop) > testZone.top) || (y < (testZone.top + _fromTop)))
			return false;

		return true;
	};

	RECT getButtonRectFrom(const RECT & tabItemRect) const {
		RECT rect;
		rect.right = tabItemRect.right - _fromRight;
		rect.left = rect.right - _width;
		rect.top = tabItemRect.top + _fromTop;
		rect.bottom = rect.top + _hight;

		return rect;
	};

	int _width;
	int _hight;

	int _fromTop; // distance from top in pixzl
	int _fromRight; // distance from right in pixzl
};

class TabBarPlus : public TabBar
{
public :

	TabBarPlus() : TabBar(), _isDragging(false), _tabBarDefaultProc(NULL), _currentHoverTabItem(-1),\
		_isCloseHover(false), _whichCloseClickDown(-1), _lmbdHit(false) {};
	enum tabColourIndex {
		activeText, activeFocusedTop, activeUnfocusedTop, inactiveText, inactiveBg
	};

	static void doDragNDrop(bool justDoIt) {
        _doDragNDrop = justDoIt;
    };

	virtual void init(HINSTANCE hInst, HWND hwnd, bool isVertical = false, bool isTraditional = false, bool isMultiLine = false);

    static bool doDragNDropOrNot() {
        return _doDragNDrop;
    };

	int getSrcTabIndex() const {
        return _nSrcTab;
    };

    int getTabDraggedIndex() const {
        return _nTabDragged;
    };

	POINT getDraggingPoint() const {
		return _draggingPoint;
	};

	static void doOwnerDrawTab() {
		::SendMessage(_hwndArray[0], TCM_SETPADDING, 0, MAKELPARAM(6, 0));
		for (int i = 0 ; i < _nbCtrl ; i++)
		{
			if (_hwndArray[i])
			{
				DWORD style = ::GetWindowLongPtr(_hwndArray[i], GWL_STYLE);
				if (isOwnerDrawTab())
					style |= TCS_OWNERDRAWFIXED;
				else
					style &= ~TCS_OWNERDRAWFIXED;

				::SetWindowLongPtr(_hwndArray[i], GWL_STYLE, style);
				::InvalidateRect(_hwndArray[i], NULL, TRUE);

				const int base = 6;
				::SendMessage(_hwndArray[i], TCM_SETPADDING, 0, MAKELPARAM(_drawTabCloseButton?base+3:base, 0));
			}
		}
	};

	static void doVertical() {
		for (int i = 0 ; i < _nbCtrl ; i++)
		{
			if (_hwndArray[i])
				SendMessage(_hwndArray[i], WM_TABSETSTYLE, isVertical(), TCS_VERTICAL);
		}
	};

	static void doMultiLine() {
		for (int i = 0 ; i < _nbCtrl ; i++)
		{
			if (_hwndArray[i])
				SendMessage(_hwndArray[i], WM_TABSETSTYLE, isMultiLine(), TCS_MULTILINE);
		}
	};

	static bool isOwnerDrawTab() {return true;};//(_drawInactiveTab || _drawTopBar || _drawTabCloseButton);};
	static bool drawTopBar() {return _drawTopBar;};
	static bool drawInactiveTab() {return _drawInactiveTab;};
	static bool drawTabCloseButton() {return _drawTabCloseButton;};
	static bool isDbClk2Close() {return _isDbClk2Close;};
	static bool isVertical() { return _isCtrlVertical;};
	static bool isMultiLine() { return _isCtrlMultiLine;};

	static void setDrawTopBar(bool b) {
		_drawTopBar = b;
		doOwnerDrawTab();
	};
	static void setDrawInactiveTab(bool b) {
		_drawInactiveTab = b;
		doOwnerDrawTab();
	};
	static void setDrawTabCloseButton(bool b) {
		_drawTabCloseButton = b;
		doOwnerDrawTab();
	};

	static void setDbClk2Close(bool b) {
		_isDbClk2Close = b;
	};

	static void setVertical(bool b) {
		_isCtrlVertical = b;
		doVertical();
	};

	static void setMultiLine(bool b) {
		_isCtrlMultiLine = b;
		doMultiLine();
	};

	static void setColour(COLORREF colour2Set, tabColourIndex i) {
		switch (i)
		{
			case activeText:
				_activeTextColour = colour2Set;
				break;
			case activeFocusedTop:
				_activeTopBarFocusedColour = colour2Set;
				break;
			case activeUnfocusedTop:
				_activeTopBarUnfocusedColour = colour2Set;
				break;
			case inactiveText:
				_inactiveTextColour = colour2Set;
				break;
			case inactiveBg :
				_inactiveBgColour = colour2Set;
				break;
			default :
				return;
		}
		doOwnerDrawTab();
	};

protected:
    // it's the boss to decide if we do the drag N drop
    static bool _doDragNDrop;
	// drag N drop members
	bool _isDragging;
	bool _isDraggingInside;
    int _nSrcTab;
	int _nTabDragged;
	POINT _draggingPoint; // coordinate of Screen
	WNDPROC _tabBarDefaultProc;

	RECT _currentHoverTabRect;
	int _currentHoverTabItem;

	CloseButtonZone _closeButtonZone;
	bool _isCloseHover;
	int _whichCloseClickDown;
	bool _lmbdHit; // Left Mouse Button Down Hit

	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK TabBarPlus_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((TabBarPlus *)(::GetWindowLongPtr(hwnd, GWL_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	};
	void exchangeItemData(POINT point);


	// it's the boss to decide if we do the ownerDraw style tab
	static bool _drawInactiveTab;
	static bool _drawTopBar;
	static bool _drawTabCloseButton;
	static bool _isDbClk2Close;
	static bool _isCtrlVertical;
	static bool _isCtrlMultiLine;

	static COLORREF _activeTextColour;
	static COLORREF _activeTopBarFocusedColour;
	static COLORREF _activeTopBarUnfocusedColour;
	static COLORREF _inactiveTextColour;
	static COLORREF _inactiveBgColour;

	static int _nbCtrl;
	static HWND _hwndArray[nbCtrlMax];

	void drawItem(DRAWITEMSTRUCT *pDrawItemStruct);
	void draggingCursor(POINT screenPoint);

	int getTabIndexAt(const POINT & p) {
		return getTabIndexAt(p.x, p.y);
	};

	int getTabIndexAt(int x, int y) {
		TCHITTESTINFO hitInfo;
		hitInfo.pt.x = x;
		hitInfo.pt.y = y;

		return ::SendMessage(_hSelf, TCM_HITTEST, 0, (LPARAM)&hitInfo);
	};
};

#endif // TAB_BAR_H
