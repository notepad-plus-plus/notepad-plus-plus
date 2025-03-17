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
#include "dpiManagerV2.h"

//Notification message
#define TCN_TABDROPPED (TCN_FIRST - 10)
#define TCN_TABDROPPEDOUTSIDE (TCN_FIRST - 11)
#define TCN_TABDELETE (TCN_FIRST - 12)
#define TCN_MOUSEHOVERING (TCN_FIRST - 13)
#define TCN_MOUSELEAVING (TCN_FIRST - 14)
#define TCN_MOUSEHOVERSWITCHING (TCN_FIRST - 15)
#define TCN_TABPINNED (TCN_FIRST - 16)

#define WM_TABSETSTYLE	(WM_APP + 0x024)

const int marge = 8;
const int nbCtrlMax = 10;

const wchar_t TABBAR_ACTIVEFOCUSEDINDCATOR[64] = L"Active tab focused indicator";
const wchar_t TABBAR_ACTIVEUNFOCUSEDINDCATOR[64] = L"Active tab unfocused indicator";
const wchar_t TABBAR_ACTIVETEXT[64] = L"Active tab text";
const wchar_t TABBAR_INACTIVETEXT[64] = L"Inactive tabs";

const wchar_t TABBAR_INDIVIDUALCOLOR_1[64] = L"Tab color 1";
const wchar_t TABBAR_INDIVIDUALCOLOR_2[64] = L"Tab color 2";
const wchar_t TABBAR_INDIVIDUALCOLOR_3[64] = L"Tab color 3";
const wchar_t TABBAR_INDIVIDUALCOLOR_4[64] = L"Tab color 4";
const wchar_t TABBAR_INDIVIDUALCOLOR_5[64] = L"Tab color 5";

const wchar_t TABBAR_INDIVIDUALCOLOR_DM_1[64] = L"Tab color dark mode 1";
const wchar_t TABBAR_INDIVIDUALCOLOR_DM_2[64] = L"Tab color dark mode 2";
const wchar_t TABBAR_INDIVIDUALCOLOR_DM_3[64] = L"Tab color dark mode 3";
const wchar_t TABBAR_INDIVIDUALCOLOR_DM_4[64] = L"Tab color dark mode 4";
const wchar_t TABBAR_INDIVIDUALCOLOR_DM_5[64] = L"Tab color dark mode 5";

constexpr int g_TabIconSize = 16;
constexpr int g_TabHeight = 22;
constexpr int g_TabHeightLarge = 25;
constexpr int g_TabWidth = 45;
constexpr int g_TabWidthButton = 60;
constexpr int g_TabCloseBtnSize = 11;
constexpr int g_TabPinBtnSize = 11;
constexpr int g_TabCloseBtnSize_DM = 16;
constexpr int g_TabPinBtnSize_DM = 16;

struct TBHDR
{
	NMHDR _hdr{};
	int _tabOrigin = 0;
};



class TabBar : public Window
{
public:
	TabBar() = default;
	virtual ~TabBar() = default;
	void destroy() override;
	virtual void init(HINSTANCE hInst, HWND hwnd, bool isVertical = false, bool isMultiLine = false);
	void reSizeTo(RECT& rc2Ajust) override;
	int insertAtEnd(const wchar_t *subTabName);
	void activateAt(int index) const;
	void getCurrentTitle(wchar_t *title, int titleLen);

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

	void setFont();

	HFONT& getFont(bool isReduced = true) {
		return isReduced ? _hFont : _hLargeFont;
	}

	int getNextOrPrevTabIdx(bool isNext) const;

	DPIManagerV2& dpiManager() { return _dpiManager; };

protected:
	size_t _nbItem = 0;
	bool _hasImgLst = false;

	HFONT _hFont = nullptr;
	HFONT _hLargeFont = nullptr;
	HFONT _hVerticalFont = nullptr;
	HFONT _hVerticalLargeFont = nullptr;


	DPIManagerV2 _dpiManager;

	long getRowCount() const {
		return long(::SendMessage(_hSelf, TCM_GETROWCOUNT, 0, 0));
	}
};


struct TabButtonZone
{
	void init(HWND parent, int order) {
		_parent = parent;
		_order = order;
	}

	bool isHit(int x, int y, const RECT & tabRect, bool isVertical) const;
	RECT getButtonRectFrom(const RECT & tabRect, bool isVertical) const;
	void setOrder(int newOrder) { _order = newOrder; };

	HWND _parent = nullptr;
	int _width = 0;
	int _height = 0;
	int _order = -1; // from right to left: 0, 1
};



class TabBarPlus : public TabBar
{
public :
	TabBarPlus() = default;
	enum tabColourIndex {
		activeText, activeFocusedTop, activeUnfocusedTop, inactiveText, inactiveBg
	};

	enum individualTabColourId {
		id0, id1, id2, id3, id4, id5, id6, id7, id8, id9
	};

	void init(HINSTANCE hInst, HWND hwnd, bool isVertical, bool isMultiLine, unsigned char buttonsStatus = 0);

	void destroy() override;

	POINT getDraggingPoint() const {
		return _draggingPoint;
	};

	void resetDraggingPoint() {
		_draggingPoint.x = 0;
		_draggingPoint.y = 0;
	};

	static void triggerOwnerDrawTabbar(DPIManagerV2* pDPIManager);
	static void doVertical();
	static void doMultiLine();

	static void setColour(COLORREF colour2Set, tabColourIndex i, DPIManagerV2* pDPIManager);
	virtual int getIndividualTabColourId(int tabIndex) = 0;

	void tabToStart(int index = -1);
	void tabToEnd(int index = -1);

	void setCloseBtnImageList();
	void setPinBtnImageList();

	void setTabPinButtonOrder(int newOrder) {
		_pinButtonZone.setOrder(newOrder);
	}

	void setTabCloseButtonOrder(int newOrder) {
		_closeButtonZone.setOrder(newOrder);
	}

	// Hack for forcing the tab width change
	// ref: https://github.com/notepad-plus-plus/notepad-plus-plus/pull/15781#issuecomment-2469387409
	void refresh() {
		int index = insertAtEnd(L"");
		deletItemAt(index);
	}

protected:
	// drag & drop members
	bool _mightBeDragging = false;
	int _dragCount = 0;
	bool _isDragging = false;
	bool _isDraggingInside = false;
    int _nSrcTab = -1;
	int _nTabDragged = -1;
	int _previousTabSwapped = -1;
	POINT _draggingPoint{}; // coordinate of Screen
	WNDPROC _tabBarDefaultProc = nullptr;

	RECT _currentHoverTabRect{};
	int _currentHoverTabItem = -1; // -1 : no mouse on any tab

	TabButtonZone _closeButtonZone;
	TabButtonZone _pinButtonZone;

	HIMAGELIST _hCloseBtnImgLst = nullptr;
	const int _closeTabIdx = 0;
	const int _closeTabInactIdx = 1;
	const int _closeTabHoverInIdx = 2; // hover inside of box
	const int _closeTabHoverOnTabIdx = 3; // hover on the tab, but outside of box
	const int _closeTabPushIdx = 4;

	HIMAGELIST _hPinBtnImgLst = nullptr;
	const int _unpinnedIdx = 0;
	const int _unpinnedInactIdx = 1;
	const int _unpinnedHoverInIdx = 2; // hover inside of box
	const int _unpinnedHoverOnTabIdx = 3; // hover on the tab, but outside of box
	const int _pinnedIdx = 4;
	const int _pinnedHoverIdx = 5;

	bool _isCloseHover = false;
	bool _isPinHover = false;
	int _whichCloseClickDown = -1;
	int _whichPinClickDown = -1;
	bool _lmbdHit = false; // Left Mouse Button Down Hit
	HWND _tooltips = nullptr;

	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK TabBarPlus_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((TabBarPlus *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	};
	void setActiveTab(int tabIndex);
	bool exchangeTabItemData(int oldTab, int newTab);
	void exchangeItemData(POINT point);

	static COLORREF _activeTextColour;
	static COLORREF _activeTopBarFocusedColour;
	static COLORREF _activeTopBarUnfocusedColour;
	static COLORREF _inactiveTextColour;
	static COLORREF _inactiveBgColour;

	static int _nbCtrl;
	static HWND _tabbrPlusInstanceHwndArray[nbCtrlMax];

	void drawItem(DRAWITEMSTRUCT *pDrawItemStruct, bool isDarkMode = false);
	void draggingCursor(POINT screenPoint);

	int getTabIndexAt(const POINT & p) const {
		return getTabIndexAt(p.x, p.y);
	}

	int32_t getTabIndexAt(int x, int y) const {
		TCHITTESTINFO hitInfo{};
		hitInfo.pt.x = x;
		hitInfo.pt.y = y;
		return static_cast<int32_t>(::SendMessage(_hSelf, TCM_HITTEST, 0, reinterpret_cast<LPARAM>(&hitInfo)));
	}

	bool isPointInParentZone(POINT screenPoint) const {
		RECT parentZone{};
        ::GetWindowRect(_hParent, &parentZone);
	    return (((screenPoint.x >= parentZone.left) && (screenPoint.x <= parentZone.right)) &&
			    (screenPoint.y >= parentZone.top) && (screenPoint.y <= parentZone.bottom));
    }

	void notify(int notifyCode, int tabIndex);
	void trackMouseEvent(DWORD event2check);
};
