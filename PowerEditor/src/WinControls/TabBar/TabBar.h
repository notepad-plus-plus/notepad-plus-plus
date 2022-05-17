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


#pragma once

#ifndef _WIN32_IE
#define _WIN32_IE	0x0600
#endif //_WIN32_IE

#include "menuCmdID.h"
#include "resource.h"
#include <stdint.h>
#include <windows.h>
#include <commctrl.h>
#include "Window.h"

//Notification message
#define TCN_TABDROPPED (TCN_FIRST - 10)
#define TCN_TABDROPPEDOUTSIDE (TCN_FIRST - 11)
#define TCN_TABDELETE (TCN_FIRST - 12)
#define TCN_MOUSEHOVERING (TCN_FIRST - 13)
#define TCN_MOUSELEAVING (TCN_FIRST - 14)
#define TCN_MOUSEHOVERSWITCHING (TCN_FIRST - 15)

#define WM_TABSETSTYLE	(WM_APP + 0x024)

const int marge = 8;
const int nbCtrlMax = 10;

const TCHAR TABBAR_ACTIVEFOCUSEDINDCATOR[64] = TEXT("Active tab focused indicator");
const TCHAR TABBAR_ACTIVEUNFOCUSEDINDCATOR[64] = TEXT("Active tab unfocused indicator");
const TCHAR TABBAR_ACTIVETEXT[64] = TEXT("Active tab text");
const TCHAR TABBAR_INACTIVETEXT[64] = TEXT("Inactive tabs");

struct TBHDR
{
	NMHDR _hdr;
	int _tabOrigin;
};



class TabBar : public Window
{
public:
	TabBar() = default;
	virtual ~TabBar() = default;
	virtual void destroy();
	virtual void init(HINSTANCE hInst, HWND hwnd, bool isVertical = false, bool isMultiLine = false);
	virtual void reSizeTo(RECT & rc2Ajust);
	int insertAtEnd(const TCHAR *subTabName);
	void activateAt(int index) const;
	void getCurrentTitle(TCHAR *title, int titleLen);

	int32_t getCurrentTabIndex() const {
		return static_cast<int32_t>(SendMessage(_hSelf, TCM_GETCURSEL, 0, 0));
	};

	int32_t getItemCount() const {
		return static_cast<int32_t>(::SendMessage(_hSelf, TCM_GETITEMCOUNT, 0, 0));
	}

	void deletItemAt(size_t index);

	void deletAllItem() {
		::SendMessage(_hSelf, TCM_DELETEALLITEMS, 0, 0);
		_nbItem = 0;
	};

	void setImageList(HIMAGELIST himl);

    size_t nbItem() const {
        return _nbItem;
    }

	void setFont(const TCHAR *fontName, int fontSize);

	void setVertical(bool b) {
		_isVertical = b;
	};

	void setMultiLine(bool b) {
		_isMultiLine = b;
	};


protected:
	size_t _nbItem = 0;
	bool _hasImgLst = false;
	HFONT _hFont = nullptr;
	HFONT _hLargeFont = nullptr;
	HFONT _hVerticalFont = nullptr;
	HFONT _hVerticalLargeFont = nullptr;

	int _ctrlID = 0;

	bool _isVertical = false;
	bool _isMultiLine = false;

	long getRowCount() const {
		return long(::SendMessage(_hSelf, TCM_GETROWCOUNT, 0, 0));
	}
};


struct CloseButtonZone
{
	CloseButtonZone();
	bool isHit(int x, int y, const RECT & tabRect, bool isVertical) const;
	RECT getButtonRectFrom(const RECT & tabRect, bool isVertical) const;

	int _width = 0;
	int _height = 0;
};



class TabBarPlus : public TabBar
{
public :
	TabBarPlus() = default;
	enum tabColourIndex {
		activeText, activeFocusedTop, activeUnfocusedTop, inactiveText, inactiveBg
	};

	static void doDragNDrop(bool justDoIt) {
        _doDragNDrop = justDoIt;
    };

	virtual void init(HINSTANCE hInst, HWND hwnd, bool isVertical = false, bool isMultiLine = false);

	virtual void destroy();

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

	void resetDraggingPoint() {
		_draggingPoint.x = 0;
		_draggingPoint.y = 0;
	};

	static void doOwnerDrawTab();
	static void doVertical();
	static void doMultiLine();
	static bool isOwnerDrawTab() {return true;};
	static bool drawTopBar() {return _drawTopBar;};
	static bool drawInactiveTab() {return _drawInactiveTab;};
	static bool drawTabCloseButton() {return _drawTabCloseButton;};
	static bool isDbClk2Close() {return _isDbClk2Close;};
	static bool isVertical() { return _isCtrlVertical;};
	static bool isMultiLine() { return _isCtrlMultiLine;};

	static void setDrawTopBar(bool b) {
		_drawTopBar = b;
		doOwnerDrawTab();
	}

	static void setDrawInactiveTab(bool b) {
		_drawInactiveTab = b;
		doOwnerDrawTab();
	}

	static void setDrawTabCloseButton(bool b) {
		_drawTabCloseButton = b;
		doOwnerDrawTab();
	}

	static void setDbClk2Close(bool b) {
		_isDbClk2Close = b;
	}

	static void setVertical(bool b) {
		_isCtrlVertical = b;
		doVertical();
	}

	static void setMultiLine(bool b) {
		_isCtrlMultiLine = b;
		doMultiLine();
	}

	static void setColour(COLORREF colour2Set, tabColourIndex i);

protected:
    // it's the boss to decide if we do the drag N drop
    static bool _doDragNDrop;
	// drag N drop members
	bool _mightBeDragging = false;
	int _dragCount = 0;
	bool _isDragging = false;
	bool _isDraggingInside = false;
    int _nSrcTab = -1;
	int _nTabDragged = -1;
	int _previousTabSwapped = -1;
	POINT _draggingPoint = {}; // coordinate of Screen
	WNDPROC _tabBarDefaultProc = nullptr;

	RECT _currentHoverTabRect;
	int _currentHoverTabItem = -1; // -1 : no mouse on any tab

	CloseButtonZone _closeButtonZone;
	bool _isCloseHover = false;
	int _whichCloseClickDown = -1;
	bool _lmbdHit = false; // Left Mouse Button Down Hit
	HWND _tooltips = nullptr;

	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK TabBarPlus_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((TabBarPlus *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	};
	void setActiveTab(int tabIndex);
	void exchangeTabItemData(int oldTab, int newTab);
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

	void drawItem(DRAWITEMSTRUCT *pDrawItemStruct, bool isDarkMode = false);
	void draggingCursor(POINT screenPoint);

	int getTabIndexAt(const POINT & p)
	{
		return getTabIndexAt(p.x, p.y);
	}

	int32_t getTabIndexAt(int x, int y)
	{
		TCHITTESTINFO hitInfo;
		hitInfo.pt.x = x;
		hitInfo.pt.y = y;
		return static_cast<int32_t>(::SendMessage(_hSelf, TCM_HITTEST, 0, reinterpret_cast<LPARAM>(&hitInfo)));
	}

	bool isPointInParentZone(POINT screenPoint) const
	{
        RECT parentZone;
        ::GetWindowRect(_hParent, &parentZone);
	    return (((screenPoint.x >= parentZone.left) && (screenPoint.x <= parentZone.right)) &&
			    (screenPoint.y >= parentZone.top) && (screenPoint.y <= parentZone.bottom));
    }

	void notify(int notifyCode, int tabIndex);
	void trackMouseEvent(DWORD event2check);
};
