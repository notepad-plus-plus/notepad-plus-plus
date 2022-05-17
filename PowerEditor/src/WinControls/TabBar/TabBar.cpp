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

#include <stdexcept>
#include "TabBar.h"
#include "Parameters.h"

#define	IDC_DRAG_TAB     1404
#define	IDC_DRAG_INTERDIT_TAB 1405
#define	IDC_DRAG_PLUS_TAB 1406
#define	IDC_DRAG_OUT_TAB 1407

bool TabBarPlus::_doDragNDrop = false;

bool TabBarPlus::_drawTopBar = true;
bool TabBarPlus::_drawInactiveTab = true;
bool TabBarPlus::_drawTabCloseButton = false;
bool TabBarPlus::_isDbClk2Close = false;
bool TabBarPlus::_isCtrlVertical = false;
bool TabBarPlus::_isCtrlMultiLine = false;

COLORREF TabBarPlus::_activeTextColour = ::GetSysColor(COLOR_BTNTEXT);
COLORREF TabBarPlus::_activeTopBarFocusedColour = RGB(250, 170, 60);
COLORREF TabBarPlus::_activeTopBarUnfocusedColour = RGB(250, 210, 150);
COLORREF TabBarPlus::_inactiveTextColour = grey;
COLORREF TabBarPlus::_inactiveBgColour = RGB(192, 192, 192);

HWND TabBarPlus::_hwndArray[nbCtrlMax] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
int TabBarPlus::_nbCtrl = 0;

void TabBar::init(HINSTANCE hInst, HWND parent, bool isVertical, bool isMultiLine)
{
	Window::init(hInst, parent);
	int vertical = isVertical?(TCS_VERTICAL | TCS_MULTILINE | TCS_RIGHTJUSTIFY):0;

	_isVertical = isVertical;
	_isMultiLine = isMultiLine;

	INITCOMMONCONTROLSEX icce;
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_TAB_CLASSES;
	InitCommonControlsEx(&icce);
	int multiLine = isMultiLine ? TCS_MULTILINE : 0;

	int style = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE |\
		TCS_FOCUSNEVER | TCS_TABS | WS_TABSTOP | vertical | multiLine;

	_hSelf = ::CreateWindowEx(
				0,
				WC_TABCONTROL,
				TEXT("Tab"),
				style,
				0, 0, 0, 0,
				_hParent,
				NULL,
				_hInst,
				0);

	if (!_hSelf)
	{
		throw std::runtime_error("TabBar::init : CreateWindowEx() function return null");
	}
}


void TabBar::destroy()
{
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
}


int TabBar::insertAtEnd(const TCHAR *subTabName)
{
	TCITEM tie;
	tie.mask = TCIF_TEXT | TCIF_IMAGE;
	int index = -1;

	if (_hasImgLst)
		index = 0;
	tie.iImage = index;
	tie.pszText = (TCHAR *)subTabName;
	return int(::SendMessage(_hSelf, TCM_INSERTITEM, _nbItem++, reinterpret_cast<LPARAM>(&tie)));
}


void TabBar::getCurrentTitle(TCHAR *title, int titleLen)
{
	TCITEM tci;
	tci.mask = TCIF_TEXT;
	tci.pszText = title;
	tci.cchTextMax = titleLen-1;
	::SendMessage(_hSelf, TCM_GETITEM, getCurrentTabIndex(), reinterpret_cast<LPARAM>(&tci));
}


void TabBar::setFont(const TCHAR *fontName, int fontSize)
{
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
}


void TabBar::activateAt(int index) const
{
	if (getCurrentTabIndex() != index)
	{
		// TCM_SETCURFOCUS is busted on WINE/ReactOS for single line (non-TCS_BUTTONS) tabs...
		// We need it on Windows for multi-line tabs or multiple tabs can appear pressed.
		if (::GetWindowLongPtr(_hSelf, GWL_STYLE) & TCS_BUTTONS)
		{
			::SendMessage(_hSelf, TCM_SETCURFOCUS, index, 0);
		}

		::SendMessage(_hSelf, TCM_SETCURSEL, index, 0);
	}

	TBHDR nmhdr;
	nmhdr._hdr.hwndFrom = _hSelf;
	nmhdr._hdr.code = TCN_SELCHANGE;
	nmhdr._hdr.idFrom = reinterpret_cast<UINT_PTR>(this);
	nmhdr._tabOrigin = index;
}


void TabBar::deletItemAt(size_t index)
{
	if (index == _nbItem - 1)
	{
		//prevent invisible tabs. If last visible tab is removed, other tabs are put in view but not redrawn
		//Therefore, scroll one tab to the left if only one tab visible
		if (_nbItem > 1)
		{
			RECT itemRect;
			::SendMessage(_hSelf, TCM_GETITEMRECT, index, reinterpret_cast<LPARAM>(&itemRect));
			if (itemRect.left < 5) //if last visible tab, scroll left once (no more than 5px away should be safe, usually 2px depending on the drawing)
			{
				//To scroll the tab control to the left, use the WM_HSCROLL notification
				//Doesn't really seem to be documented anywhere, but the values do match the message parameters
				//The up/down control really is just some sort of scrollbar
				//There seems to be no negative effect on any internal state of the tab control or the up/down control
				int wParam = MAKEWPARAM(SB_THUMBPOSITION, index - 1);
				::SendMessage(_hSelf, WM_HSCROLL, wParam, 0);

				wParam = MAKEWPARAM(SB_ENDSCROLL, index - 1);
				::SendMessage(_hSelf, WM_HSCROLL, wParam, 0);
			}
		}
	}
	::SendMessage(_hSelf, TCM_DELETEITEM, index, 0);
	_nbItem--;
}


void TabBar::setImageList(HIMAGELIST himl)
{
	_hasImgLst = true;
	::SendMessage(_hSelf, TCM_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(himl));
}


void TabBar::reSizeTo(RECT & rc2Ajust)
{
	RECT rowRect;
	int rowCount, tabsHight;

	// Important to do that!
	// Otherwise, the window(s) it contains will take all the resouce of CPU
	// We don't need to resize the contained windows if they are even invisible anyway
	display(rc2Ajust.right > 10);
	RECT rc = rc2Ajust;
	Window::reSizeTo(rc);

	// Do our own calculations because TabCtrl_AdjustRect doesn't work
	// on vertical or multi-lined tab controls

	rowCount = TabCtrl_GetRowCount(_hSelf);
	TabCtrl_GetItemRect(_hSelf, 0, &rowRect);

	int larger = _isVertical ? rowRect.right : rowRect.bottom;
	int smaller = _isVertical ? rowRect.left : rowRect.top;
	int marge = 0;

	LONG_PTR style = ::GetWindowLongPtr(_hSelf, GWL_STYLE);
	if (rowCount == 1)
	{
		style &= ~TCS_BUTTONS;
	}
	else // (rowCount >= 2)
	{
		style |= TCS_BUTTONS;
		marge = (rowCount - 2) * 3; // in TCS_BUTTONS mode, each row has few pixels higher
	}

	::SetWindowLongPtr(_hSelf, GWL_STYLE, style);
	tabsHight = rowCount * (larger - smaller) + marge;
	tabsHight += GetSystemMetrics(_isVertical ? SM_CXEDGE : SM_CYEDGE);

	if (_isVertical)
	{
		rc2Ajust.left += tabsHight;
		rc2Ajust.right -= tabsHight;
	}
	else
	{
		rc2Ajust.top += tabsHight;
		rc2Ajust.bottom -= tabsHight;
	}
}


void TabBarPlus::destroy()
{
	TabBar::destroy();
	::DestroyWindow(_tooltips);
	_tooltips = NULL;
}


void TabBarPlus::init(HINSTANCE hInst, HWND parent, bool isVertical, bool isMultiLine)
{
	Window::init(hInst, parent);
	int vertical = isVertical?(TCS_VERTICAL | TCS_MULTILINE | TCS_RIGHTJUSTIFY):0;
	_isVertical = isVertical;
	_isMultiLine = isMultiLine;

	INITCOMMONCONTROLSEX icce;
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_TAB_CLASSES;
	InitCommonControlsEx(&icce);
	int multiLine = isMultiLine ? TCS_MULTILINE : 0;

	int style = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | TCS_FOCUSNEVER | TCS_TABS | vertical | multiLine;

	style |= TCS_OWNERDRAWFIXED;

	_hSelf = ::CreateWindowEx(
				0,
				WC_TABCONTROL,
				TEXT("Tab"),
				style,
				0, 0, 0, 0,
				_hParent,
				NULL,
				_hInst,
				0);

	if (!_hSelf)
	{
		throw std::runtime_error("TabBarPlus::init : CreateWindowEx() function return null");
	}

	_tooltips = ::CreateWindowEx(
		0,
		TOOLTIPS_CLASS,
		NULL,
		TTS_ALWAYSTIP | TTS_NOPREFIX,
		0, 0, 0, 0,
		_hParent,
		NULL,
		_hInst,
		0);

	if (!_tooltips)
	{
		throw std::runtime_error("TabBarPlus::init : tooltip CreateWindowEx() function return null");
	}

	NppDarkMode::setDarkTooltips(_tooltips, NppDarkMode::ToolTipsType::tooltip);

	::SendMessage(_hSelf, TCM_SETTOOLTIPS, reinterpret_cast<WPARAM>(_tooltips), 0);

	if (!_hwndArray[_nbCtrl])
	{
		_hwndArray[_nbCtrl] = _hSelf;
		_ctrlID = _nbCtrl;
	}
	else
	{
		int i = 0;
		bool found = false;
		for ( ; i < nbCtrlMax && !found ; ++i)
			if (!_hwndArray[i])
				found = true;
		if (!found)
		{
			_ctrlID = -1;
			destroy();
			throw std::runtime_error("TabBarPlus::init : Tab Control error - Tab Control # is over its limit");
		}
		_hwndArray[i] = _hSelf;
		_ctrlID = i;
	}
	++_nbCtrl;

	::SetWindowLongPtr(_hSelf, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	_tabBarDefaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSelf, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(TabBarPlus_Proc)));

	LOGFONT LogFont;

	_hFont = (HFONT)::SendMessage(_hSelf, WM_GETFONT, 0, 0);

	if (_hFont == NULL)
		_hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);

	if (_hLargeFont == NULL)
		_hLargeFont = (HFONT)::GetStockObject(SYSTEM_FONT);

	if (::GetObject(_hFont, sizeof(LOGFONT), &LogFont) != 0)
	{
		LogFont.lfEscapement  = 900;
		LogFont.lfOrientation = 900;
		_hVerticalFont = CreateFontIndirect(&LogFont);

		LogFont.lfWeight = 900;
		_hVerticalLargeFont = CreateFontIndirect(&LogFont);
	}
}


void TabBarPlus::doOwnerDrawTab()
{
	::SendMessage(_hwndArray[0], TCM_SETPADDING, 0, MAKELPARAM(6, 0));
	for (int i = 0 ; i < _nbCtrl ; ++i)
	{
		if (_hwndArray[i])
		{
			LONG_PTR style = ::GetWindowLongPtr(_hwndArray[i], GWL_STYLE);
			if (isOwnerDrawTab())
				style |= TCS_OWNERDRAWFIXED;
			else
				style &= ~TCS_OWNERDRAWFIXED;

			::SetWindowLongPtr(_hwndArray[i], GWL_STYLE, style);
			::InvalidateRect(_hwndArray[i], NULL, TRUE);

			const int paddingSizeDynamicW = NppParameters::getInstance()._dpiManager.scaleX(6);
			const int paddingSizePlusClosebuttonDynamicW = NppParameters::getInstance()._dpiManager.scaleX(9);
			::SendMessage(_hwndArray[i], TCM_SETPADDING, 0, MAKELPARAM(_drawTabCloseButton ? paddingSizePlusClosebuttonDynamicW : paddingSizeDynamicW, 0));
		}
	}
}


void TabBarPlus::setColour(COLORREF colour2Set, tabColourIndex i)
{
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
}


void TabBarPlus::doVertical()
{
	for (int i = 0 ; i < _nbCtrl ; ++i)
	{
		if (_hwndArray[i])
			SendMessage(_hwndArray[i], WM_TABSETSTYLE, isVertical(), TCS_VERTICAL);
	}
}


void TabBarPlus::doMultiLine()
{
	for (int i = 0 ; i < _nbCtrl ; ++i)
	{
		if (_hwndArray[i])
			SendMessage(_hwndArray[i], WM_TABSETSTYLE, isMultiLine(), TCS_MULTILINE);
	}
}

void TabBarPlus::notify(int notifyCode, int tabIndex)
{
	TBHDR nmhdr;
	nmhdr._hdr.hwndFrom = _hSelf;
	nmhdr._hdr.code = notifyCode;
	nmhdr._hdr.idFrom = reinterpret_cast<UINT_PTR>(this);
	nmhdr._tabOrigin = tabIndex;
	::SendMessage(_hParent, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&nmhdr));
}

void TabBarPlus::trackMouseEvent(DWORD event2check)
{
	TRACKMOUSEEVENT tme = {};
	tme.cbSize = sizeof(tme);
	tme.dwFlags = event2check;
	tme.hwndTrack = _hSelf;
	TrackMouseEvent(&tme);
}

LRESULT TabBarPlus::runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		// Custom window message to change tab control style on the fly
		case WM_TABSETSTYLE:
		{
			LONG_PTR style = ::GetWindowLongPtr(hwnd, GWL_STYLE);

			if (wParam > 0)
				style |= lParam;
			else
				style &= ~lParam;

			_isVertical  = ((style & TCS_VERTICAL) != 0);
			_isMultiLine = ((style & TCS_MULTILINE) != 0);

			::SetWindowLongPtr(hwnd, GWL_STYLE, style);
			::InvalidateRect(hwnd, NULL, TRUE);

			return TRUE;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::setDarkTooltips(hwnd, NppDarkMode::ToolTipsType::tabbar);
			return TRUE;
		}

		case WM_MOUSEWHEEL:
		{
			// ..............................................................................
			// MOUSEWHEEL:
			// will scroll the tab bar area (similar to Firefox's tab scrolling),
			// it only happens if not in multi-line mode and at least one tab is hidden
			// ..............................................................................
			// CTRL + MOUSEWHEEL:
			// will do previous/next tab WITH scroll wrapping (endless loop)
			// ..............................................................................
			// SHIFT + MOUSEWHEEL:
			// if _doDragNDrop is enabled, then moves the tab, otherwise switches 
			// to previous/next tab WITHOUT scroll wrapping (stops at first/last tab)
			// ..............................................................................
			// CTRL + SHIFT + MOUSEWHEEL:
			// will switch to the first/last tab
			// ..............................................................................

			if (_isDragging)
				return TRUE;

			const bool isForward = ((short)HIWORD(wParam)) < 0; // wheel rotation towards the user will be considered as forward direction
			const int lastTabIndex = static_cast<int32_t>(::SendMessage(_hSelf, TCM_GETITEMCOUNT, 0, 0) - 1);

			if ((wParam & MK_CONTROL) && (wParam & MK_SHIFT))
			{
				setActiveTab((isForward ? lastTabIndex : 0));
			}
			else if ((wParam & MK_SHIFT) && _doDragNDrop)
			{
				int oldTabIndex = static_cast<int32_t>(::SendMessage(_hSelf, TCM_GETCURSEL, 0, 0));
				int newTabIndex = oldTabIndex + (isForward ? 1 : -1);

				if (newTabIndex < 0)
				{
					newTabIndex = lastTabIndex; // wrap scrolling
				}
				else if (newTabIndex > lastTabIndex)
				{
					newTabIndex = 0; // wrap scrolling
				}

				if (oldTabIndex != newTabIndex)
				{
					exchangeTabItemData(oldTabIndex, newTabIndex);
				}
			}
			else if (wParam & (MK_CONTROL | MK_SHIFT))
			{
				int tabIndex = static_cast<int32_t>(::SendMessage(_hSelf, TCM_GETCURSEL, 0, 0) + (isForward ? 1 : -1));
				if (tabIndex < 0)
				{
					if (wParam & MK_CONTROL)
						tabIndex = lastTabIndex; // wrap scrolling
					else
						return TRUE;
				}
				else if (tabIndex > lastTabIndex)
				{
					if (wParam & MK_CONTROL)
						tabIndex = 0; // wrap scrolling
					else
						return TRUE;
				}
				setActiveTab(tabIndex);
			}
			else if (!_isMultiLine) // don't scroll if in multi-line mode
			{
				RECT rcTabCtrl, rcLastTab;
				::SendMessage(_hSelf, TCM_GETITEMRECT, lastTabIndex, reinterpret_cast<LPARAM>(&rcLastTab));
				::GetClientRect(_hSelf, &rcTabCtrl);

				// get index of the first visible tab
				TC_HITTESTINFO hti;
				LONG xy = NppParameters::getInstance()._dpiManager.scaleX(12); // an arbitrary coordinate inside the first visible tab
				hti.pt = { xy, xy };
				int scrollTabIndex = static_cast<int32_t>(::SendMessage(_hSelf, TCM_HITTEST, 0, reinterpret_cast<LPARAM>(&hti)));

				if (scrollTabIndex < 1 && (_isVertical ? rcLastTab.bottom < rcTabCtrl.bottom : rcLastTab.right < rcTabCtrl.right)) // nothing to scroll
					return TRUE;

				// maximal width/height of the msctls_updown32 class (arrow box in the tab bar), 
				// this area may hide parts of the last tab and needs to be excluded
				LONG maxLengthUpDownCtrl = NppParameters::getInstance()._dpiManager.scaleX(44); // sufficient static value

				// scroll forward as long as the last tab is hidden; scroll backward till the first tab
				if ((_isVertical ? ((rcTabCtrl.bottom - rcLastTab.bottom) < maxLengthUpDownCtrl) : ((rcTabCtrl.right - rcLastTab.right) < maxLengthUpDownCtrl)) || !isForward)
				{
					if (isForward)
						++scrollTabIndex;
					else
						--scrollTabIndex;

					if (scrollTabIndex < 0 || scrollTabIndex > lastTabIndex)
						return TRUE;

					// clear hover state of the close button,
					// WM_MOUSEMOVE won't handle this properly since the tab position will change
					if (_isCloseHover)
					{
						_isCloseHover = false;
						::InvalidateRect(_hSelf, &_currentHoverTabRect, false);
					}

					::SendMessage(_hSelf, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, scrollTabIndex), 0);
				}
			}
			return TRUE;
		}

		case WM_LBUTTONDOWN :
		{
			if (::GetWindowLongPtr(_hSelf, GWL_STYLE) & TCS_BUTTONS)
			{
				int nTab = getTabIndexAt(LOWORD(lParam), HIWORD(lParam));
				if (nTab != -1 && nTab != static_cast<int32_t>(::SendMessage(_hSelf, TCM_GETCURSEL, 0, 0)))
				{
					setActiveTab(nTab);
				}
			}

			if (_drawTabCloseButton)
			{
				int xPos = LOWORD(lParam);
				int yPos = HIWORD(lParam);

				if (_closeButtonZone.isHit(xPos, yPos, _currentHoverTabRect, _isVertical))
				{
					_whichCloseClickDown = getTabIndexAt(xPos, yPos);
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_REFRESHTABAR, 0);
					return TRUE;
				}
			}

			::CallWindowProc(_tabBarDefaultProc, hwnd, Message, wParam, lParam);
			int currentTabOn = static_cast<int32_t>(::SendMessage(_hSelf, TCM_GETCURSEL, 0, 0));

			if (wParam == 2)
				return TRUE;

			if (_doDragNDrop)
			{
				_mightBeDragging = true;
			}

			notify(NM_CLICK, currentTabOn);

			return TRUE;
		}

		case WM_RBUTTONDOWN :	//rightclick selects tab aswell
		{
			// TCS_BUTTONS doesn't select the tab
			if (::GetWindowLongPtr(_hSelf, GWL_STYLE) & TCS_BUTTONS)
			{
				int nTab = getTabIndexAt(LOWORD(lParam), HIWORD(lParam));
				if (nTab != -1 && nTab != static_cast<int32_t>(::SendMessage(_hSelf, TCM_GETCURSEL, 0, 0)))
				{
					setActiveTab(nTab);
				}
			}

			::CallWindowProc(_tabBarDefaultProc, hwnd, WM_LBUTTONDOWN, wParam, lParam);
			return TRUE;
		}

		case WM_MOUSEMOVE :
		{
			if (_mightBeDragging && !_isDragging)
			{
				// Grrr! Who has stolen focus and eaten the WM_LBUTTONUP?!
				if (GetKeyState(VK_LBUTTON) >= 0)
				{
					_mightBeDragging = false;
					_dragCount = 0;
				}
				else if (++_dragCount > 2)
				{
					int tabSelected = static_cast<int32_t>(::SendMessage(_hSelf, TCM_GETCURSEL, 0, 0));

					if (tabSelected >= 0)
					{
						_nSrcTab = _nTabDragged = tabSelected;
						_isDragging = true;

						// TLS_BUTTONS is already captured on Windows and will break on ::SetCapture
						// However, this is not the case for WINE/ReactOS and must ::SetCapture
						if (::GetCapture() != _hSelf)
						{
							::SetCapture(hwnd);
						}
					}
				}
			}

			POINT p;
			p.x = LOWORD(lParam);
			p.y = HIWORD(lParam);

			if (_isDragging)
			{
				exchangeItemData(p);

				// Get cursor position of "Screen"
				// For using the function "WindowFromPoint" afterward!!!
				::GetCursorPos(&_draggingPoint);
				draggingCursor(_draggingPoint);
				return TRUE;
			}
			else
			{
				bool isFromTabToTab = false;

				int iTabNow = getTabIndexAt(p.x, p.y); // _currentHoverTabItem keeps previous value, and it need to be updated

				if (_currentHoverTabItem == iTabNow && _currentHoverTabItem != -1) // mouse moves arround in the same tab
				{
					// do nothing
				}
				else if (iTabNow == -1 && _currentHoverTabItem != -1) // mouse is no more on any tab, set hover -1
				{
					_currentHoverTabItem = -1;

					// send mouse leave notif
					notify(TCN_MOUSELEAVING, -1);
				}
				else if (iTabNow != -1 && _currentHoverTabItem == -1) // mouse is just entered in a tab zone
				{
					_currentHoverTabItem = iTabNow;

					notify(TCN_MOUSEHOVERING, _currentHoverTabItem);
				}
				else if (iTabNow != -1 && _currentHoverTabItem != -1 && _currentHoverTabItem != iTabNow) // mouse is being moved from a tab and entering into another tab
				{
					isFromTabToTab = true;
					_whichCloseClickDown = -1;

					// set current hovered
					_currentHoverTabItem = iTabNow;

					// send mouse enter notif
					notify(TCN_MOUSEHOVERSWITCHING, _currentHoverTabItem);
				}
				else if (iTabNow == -1 && _currentHoverTabItem == -1) // mouse is already outside
				{
					// do nothing
				}

				if (_drawTabCloseButton)
				{
					RECT currentHoverTabRectOld = _currentHoverTabRect;
					bool isCloseHoverOld = _isCloseHover;

					if (_currentHoverTabItem != -1) // is hovering
					{
						::SendMessage(_hSelf, TCM_GETITEMRECT, _currentHoverTabItem, reinterpret_cast<LPARAM>(&_currentHoverTabRect));
						_isCloseHover = _closeButtonZone.isHit(p.x, p.y, _currentHoverTabRect, _isVertical);
					}
					else
					{
						SetRectEmpty(&_currentHoverTabRect);
						_isCloseHover = false;
					}

					if (isFromTabToTab || _isCloseHover != isCloseHoverOld)
					{
						if (isCloseHoverOld && (isFromTabToTab || !_isCloseHover))
							InvalidateRect(hwnd, &currentHoverTabRectOld, FALSE);

						if (_isCloseHover)
							InvalidateRect(hwnd, &_currentHoverTabRect, FALSE);
					}

					if (_isCloseHover)
					{
						// Mouse moves out from close zone will send WM_MOUSELEAVE message
						trackMouseEvent(TME_LEAVE);
					}
				}
				// Mouse moves out from tab zone will send WM_MOUSELEAVE message
				// but it doesn't track mouse moving from a tab to another
				trackMouseEvent(TME_LEAVE);
			}

			break;
		}

		case WM_MOUSELEAVE:
		{
			if (_isCloseHover)
				InvalidateRect(hwnd, &_currentHoverTabRect, FALSE);

			_currentHoverTabItem = -1;
			_whichCloseClickDown = -1;
			SetRectEmpty(&_currentHoverTabRect);
			_isCloseHover = false;

			notify(TCN_MOUSELEAVING, _currentHoverTabItem);
			break;
		}

		case WM_LBUTTONUP :
		{
			_mightBeDragging = false;
			_dragCount = 0;

			int xPos = LOWORD(lParam);
			int yPos = HIWORD(lParam);
			int currentTabOn = getTabIndexAt(xPos, yPos);
			if (_isDragging)
			{
				if (::GetCapture() == _hSelf)
				{
					::ReleaseCapture();
				}
				else
				{
					_isDragging = false;
				}

				notify(_isDraggingInside?TCN_TABDROPPED:TCN_TABDROPPEDOUTSIDE, currentTabOn);
				return TRUE;
			}

			if (_drawTabCloseButton)
			{
				if ((_whichCloseClickDown == currentTabOn) && _closeButtonZone.isHit(xPos, yPos, _currentHoverTabRect, _isVertical))
				{
					notify(TCN_TABDELETE, currentTabOn);
					_whichCloseClickDown = -1;

					// Get the next tab at same position
					// If valid tab is found then
					//	 update the current hover tab RECT (_currentHoverTabRect)
					//	 update close hover flag (_isCloseHover), so that x will be highlighted or not based on new _currentHoverTabRect
					int nextTab = getTabIndexAt(xPos, yPos);
					if (nextTab != -1)
					{
						::SendMessage(_hSelf, TCM_GETITEMRECT, nextTab, reinterpret_cast<LPARAM>(&_currentHoverTabRect));
						_isCloseHover = _closeButtonZone.isHit(xPos, yPos, _currentHoverTabRect, _isVertical);
					}
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
			int xPos = LOWORD(lParam);
			int yPos = HIWORD(lParam);
			int currentTabOn = getTabIndexAt(xPos, yPos);
			notify(TCN_TABDELETE, currentTabOn);
			return TRUE;
		}

		case WM_LBUTTONDBLCLK:
		{
			if (_isDbClk2Close)
			{
				int xPos = LOWORD(lParam);
				int yPos = HIWORD(lParam);
				int currentTabOn = getTabIndexAt(xPos, yPos);
				notify(TCN_TABDELETE, currentTabOn);
			}
			return TRUE;
		}

		case WM_ERASEBKGND:
		{
			if (!NppDarkMode::isEnabled())
			{
				break;
			}

			RECT rc = {};
			GetClientRect(hwnd, &rc);
			FillRect((HDC)wParam, &rc, NppDarkMode::getDarkerBackgroundBrush());

			return 1;
		}

		case WM_PAINT:
		{
			if (!NppDarkMode::isEnabled())
			{
				break;
			}

			LONG_PTR dwStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
			if (!(dwStyle & TCS_OWNERDRAWFIXED))
			{
				break;
			}

			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			FillRect(hdc, &ps.rcPaint, NppDarkMode::getDarkerBackgroundBrush());

			UINT id = ::GetDlgCtrlID(hwnd);

			auto holdPen = static_cast<HPEN>(::SelectObject(hdc, NppDarkMode::getEdgePen()));

			HRGN holdClip = CreateRectRgn(0, 0, 0, 0);
			if (1 != GetClipRgn(hdc, holdClip))
			{
				DeleteObject(holdClip);
				holdClip = nullptr;
			}

			int topBarHeight = NppParameters::getInstance()._dpiManager.scaleX(4);

			int nTabs = TabCtrl_GetItemCount(hwnd);
			int nFocusTab = TabCtrl_GetCurFocus(hwnd);
			int nSelTab = TabCtrl_GetCurSel(hwnd);
			for (int i = 0; i < nTabs; ++i)
			{
				DRAWITEMSTRUCT dis = { ODT_TAB, id, (UINT)i, ODA_DRAWENTIRE, ODS_DEFAULT, hwnd, hdc, {}, 0 };
				TabCtrl_GetItemRect(hwnd, i, &dis.rcItem);

				if (i == nFocusTab)
				{
					dis.itemState |= ODS_FOCUS;
				}
				if (i == nSelTab)
				{
					dis.itemState |= ODS_SELECTED;
				}

				dis.itemState |= ODS_NOFOCUSRECT; // maybe, does it handle it already?

				RECT rcIntersect = {};
				if (IntersectRect(&rcIntersect, &ps.rcPaint, &dis.rcItem))
				{
					if (dwStyle & TCS_VERTICAL)
					{
						POINT edges[] = {
							{dis.rcItem.left, dis.rcItem.bottom - 1},
							{dis.rcItem.right, dis.rcItem.bottom - 1}
						};
						if (i != nSelTab && (i != nSelTab - 1))
						{
							edges[0].x += topBarHeight;
						}
						Polyline(hdc, edges, _countof(edges));
						dis.rcItem.bottom -= 1;
					}
					else
					{
						POINT edges[] = {
							{dis.rcItem.right - 1, dis.rcItem.top},
							{dis.rcItem.right - 1, dis.rcItem.bottom}
						};
						if (i != nSelTab && (i != nSelTab - 1))
						{
							edges[0].y += topBarHeight;
						}
						Polyline(hdc, edges, _countof(edges));
						dis.rcItem.right -= 1;
					}

					HRGN hClip = CreateRectRgnIndirect(&dis.rcItem);

					SelectClipRgn(hdc, hClip);

					drawItem(&dis, true);

					DeleteObject(hClip);

					SelectClipRgn(hdc, holdClip);
				}
			}

			SelectClipRgn(hdc, holdClip);
			if (holdClip)
			{
				DeleteObject(holdClip);
				holdClip = nullptr;
			}

			SelectObject(hdc, holdPen);

			EndPaint(hwnd, &ps);
			return 0;
		}

		case WM_PARENTNOTIFY:
		{
			switch (LOWORD(wParam))
			{
				case WM_CREATE:
				{
					auto hwndUpdown = reinterpret_cast<HWND>(lParam);
					if (NppDarkMode::subclassTabUpDownControl(hwndUpdown))
					{
						return 0;
					}
					break;
				}
			}
			return 0;
		}
	}

	return ::CallWindowProc(_tabBarDefaultProc, hwnd, Message, wParam, lParam);
}


void TabBarPlus::drawItem(DRAWITEMSTRUCT *pDrawItemStruct, bool isDarkMode)
{
	RECT rect = pDrawItemStruct->rcItem;

	int nTab = pDrawItemStruct->itemID;
	if (nTab < 0)
	{
		::MessageBox(NULL, TEXT("nTab < 0"), TEXT(""), MB_OK);
	}
	bool isSelected = (nTab == ::SendMessage(_hSelf, TCM_GETCURSEL, 0, 0));

	TCHAR label[MAX_PATH];
	TCITEM tci;
	tci.mask = TCIF_TEXT|TCIF_IMAGE;
	tci.pszText = label;
	tci.cchTextMax = MAX_PATH-1;

	if (!::SendMessage(_hSelf, TCM_GETITEM, nTab, reinterpret_cast<LPARAM>(&tci)))
	{
		::MessageBox(NULL, TEXT("! TCM_GETITEM"), TEXT(""), MB_OK);
	}
	HDC hDC = pDrawItemStruct->hDC;

	int nSavedDC = ::SaveDC(hDC);

	::SetBkMode(hDC, TRANSPARENT);
	HBRUSH hBrush = ::CreateSolidBrush(!isDarkMode ? ::GetSysColor(COLOR_BTNFACE) : NppDarkMode::getBackgroundColor());
	::FillRect(hDC, &rect, hBrush);
	::DeleteObject((HGDIOBJ)hBrush);
	
	// equalize drawing areas of active and inactive tabs
	int paddingDynamicTwoX = NppParameters::getInstance()._dpiManager.scaleX(2);
	int paddingDynamicTwoY = NppParameters::getInstance()._dpiManager.scaleY(2);
	if (isSelected && !isDarkMode)
	{
		// the drawing area of the active tab extends on all borders by default
		rect.top += ::GetSystemMetrics(SM_CYEDGE);
		rect.bottom -= ::GetSystemMetrics(SM_CYEDGE);
		rect.left += ::GetSystemMetrics(SM_CXEDGE);
		rect.right -= ::GetSystemMetrics(SM_CXEDGE);
		// the active tab is also slightly higher by default (use this to shift the tab cotent up bx two pixels if tobBar is not drawn)
		if (_isVertical)
		{
			rect.left += _drawTopBar ? paddingDynamicTwoX : 0;
			rect.right -= _drawTopBar ? 0 : paddingDynamicTwoX;
		}
		else
		{
			rect.top += _drawTopBar ? paddingDynamicTwoY : 0;
			rect.bottom -= _drawTopBar ? 0 : paddingDynamicTwoY;
		}
	}
	else
	{
		if (_isVertical)
		{
			rect.left += paddingDynamicTwoX;
			rect.right += paddingDynamicTwoX;
			rect.top -= paddingDynamicTwoY;
			rect.bottom += paddingDynamicTwoY;
		}
		else
		{
			rect.left -= paddingDynamicTwoX;
			rect.right += paddingDynamicTwoX;
			rect.top += paddingDynamicTwoY;
			rect.bottom += paddingDynamicTwoY;
		}
	}
	
	// the active tab's text with TCS_BUTTONS is lower than normal and gets clipped
	if (::GetWindowLongPtr(_hSelf, GWL_STYLE) & TCS_BUTTONS)
	{
		if (_isVertical)
		{
			rect.left -= 2;
		}
		else
		{
			rect.top -= 2;
		}
	}

	// draw highlights on tabs (top bar for active tab / darkened background for inactive tab)
	RECT barRect = rect;
	if (isSelected)
	{
		if (isDarkMode)
		{
			::FillRect(hDC, &pDrawItemStruct->rcItem, NppDarkMode::getSofterBackgroundBrush());
		}
		if (_drawTopBar)
		{
			int topBarHeight = NppParameters::getInstance()._dpiManager.scaleX(4);
			if (_isVertical)
			{
				barRect.left -= NppParameters::getInstance()._dpiManager.scaleX(2);
				barRect.right = barRect.left + topBarHeight;
			}
			else
			{
				barRect.top -= NppParameters::getInstance()._dpiManager.scaleY(2);
				barRect.bottom = barRect.top + topBarHeight;
			}

			if (::SendMessage(_hParent, NPPM_INTERNAL_ISFOCUSEDTAB, 0, reinterpret_cast<LPARAM>(_hSelf)))
				hBrush = ::CreateSolidBrush(_activeTopBarFocusedColour); // #FAAA3C
			else
				hBrush = ::CreateSolidBrush(_activeTopBarUnfocusedColour); // #FAD296

			::FillRect(hDC, &barRect, hBrush);
			::DeleteObject((HGDIOBJ)hBrush);
		}
	}
	else
	{
		if (_drawInactiveTab)
		{
			hBrush = ::CreateSolidBrush(!isDarkMode ? _inactiveBgColour : NppDarkMode::getBackgroundColor());
			::FillRect(hDC, &barRect, hBrush);
			::DeleteObject((HGDIOBJ)hBrush);
		}
	}

	// draw close button
	if (_drawTabCloseButton)
	{
		// 3 status for each inactive tab and selected tab close item :
		// normal / hover / pushed
		int idCloseImg;

		if (_isCloseHover && (_currentHoverTabItem == nTab) && (_whichCloseClickDown == -1)) // hover
			idCloseImg = isDarkMode ? IDR_CLOSETAB_HOVER_DM : IDR_CLOSETAB_HOVER;
		else if (_isCloseHover && (_currentHoverTabItem == nTab) && (_whichCloseClickDown == _currentHoverTabItem)) // pushed
			idCloseImg = isDarkMode ? IDR_CLOSETAB_PUSH_DM : IDR_CLOSETAB_PUSH;
		else
			idCloseImg = isSelected ? (isDarkMode ? IDR_CLOSETAB_DM : IDR_CLOSETAB) : (isDarkMode ? IDR_CLOSETAB_INACT_DM : IDR_CLOSETAB_INACT);

		HDC hdcMemory;
		hdcMemory = ::CreateCompatibleDC(hDC);
		HBITMAP hBmp = ::LoadBitmap(_hInst, MAKEINTRESOURCE(idCloseImg));
		BITMAP bmp;
		::GetObject(hBmp, sizeof(bmp), &bmp);

		int bmDpiDynamicalWidth = NppParameters::getInstance()._dpiManager.scaleX(bmp.bmWidth);
		int bmDpiDynamicalHeight = NppParameters::getInstance()._dpiManager.scaleY(bmp.bmHeight);

		RECT buttonRect = _closeButtonZone.getButtonRectFrom(rect, _isVertical);

		::SelectObject(hdcMemory, hBmp);
		::StretchBlt(hDC, buttonRect.left, buttonRect.top, bmDpiDynamicalWidth, bmDpiDynamicalHeight, hdcMemory, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
		::DeleteDC(hdcMemory);
		::DeleteObject(hBmp);
	}

	// draw image
	HIMAGELIST hImgLst = (HIMAGELIST)::SendMessage(_hSelf, TCM_GETIMAGELIST, 0, 0);

	if (hImgLst && tci.iImage >= 0)
	{
		IMAGEINFO info;
		ImageList_GetImageInfo(hImgLst, tci.iImage, &info);

		RECT& imageRect = info.rcImage;

		int fromBorder;
		int xPos, yPos;
		if (_isVertical)
		{
			fromBorder = (rect.right - rect.left - (imageRect.right - imageRect.left) + 1) / 2;
			xPos = rect.left + fromBorder;
			yPos = rect.bottom - fromBorder - (imageRect.bottom - imageRect.top);
			rect.bottom -= fromBorder + (imageRect.bottom - imageRect.top);
		}
		else
		{
			fromBorder = (rect.bottom - rect.top - (imageRect.bottom - imageRect.top) + 1) / 2;
			yPos = rect.top + fromBorder;
			xPos = rect.left + fromBorder;
			rect.left += fromBorder + (imageRect.right - imageRect.left);
		}
		ImageList_Draw(hImgLst, tci.iImage, hDC, xPos, yPos, isSelected ? ILD_TRANSPARENT : ILD_SELECTED);
	}

	// draw text
	bool isStandardSize = (::SendMessage(_hParent, NPPM_INTERNAL_ISTABBARREDUCED, 0, 0) == TRUE);

	if (isStandardSize)
	{
		if (_isVertical)
			SelectObject(hDC, _hVerticalFont);
		else
			SelectObject(hDC, _hFont);
	}
	else
	{
		if (_isVertical)
			SelectObject(hDC, _hVerticalLargeFont);
		else
			SelectObject(hDC, _hLargeFont);
	}
	SIZE charPixel;
	::GetTextExtentPoint(hDC, TEXT(" "), 1, &charPixel);
	int spaceUnit = charPixel.cx;

	TEXTMETRIC textMetrics;
	GetTextMetrics(hDC, &textMetrics);
	int textHeight = textMetrics.tmHeight;
	int textDescent = textMetrics.tmDescent;

	int Flags = DT_SINGLELINE | DT_NOPREFIX;

	// This code will read in one character at a time and remove every first ampersand (&).
	// ex. If input "test && test &&& test &&&&" then output will be "test & test && test &&&".
	// Tab's caption must be encoded like this because otherwise tab control would make tab too small or too big for the text.
	TCHAR decodedLabel[MAX_PATH];
	const TCHAR* in = label;
	TCHAR* out = decodedLabel;
	while (*in != 0)
		if (*in == '&')
			while (*(++in) == '&')
				*out++ = *in;
		else
			*out++ = *in++;
	*out = '\0';

	if (_isVertical)
	{
		// center text horizontally (rotated text is positioned as if it were unrotated, therefore manual positioning is necessary)
		Flags |= DT_LEFT;
		Flags |= DT_BOTTOM;
		rect.left += (rect.right - rect.left - textHeight) / 2;
		rect.bottom += textHeight;

		// ignoring the descent when centering (text elements below the base line) is more pleasing to the eye
		rect.left += textDescent / 2;
		rect.right += textDescent / 2;

		// 1 space distance to save icon
		rect.bottom -= spaceUnit;
	}
	else
	{
		// center text vertically
		Flags |= DT_LEFT;
		Flags |= DT_VCENTER;

		// ignoring the descent when centering (text elements below the base line) is more pleasing to the eye
		rect.top += textDescent / 2;
		rect.bottom += textDescent / 2;

		// 1 space distance to save icon
		rect.left += spaceUnit;
	}

	COLORREF textColor = isSelected ? _activeTextColour : _inactiveTextColour;
	if (isDarkMode)
	{
		textColor = NppDarkMode::invertLightnessSofter(textColor);
	}

	::SetTextColor(hDC, textColor);

	::DrawText(hDC, decodedLabel, lstrlen(decodedLabel), &rect, Flags);
	::RestoreDC(hDC, nSavedDC);
}


void TabBarPlus::draggingCursor(POINT screenPoint)
{
	HWND hWin = ::WindowFromPoint(screenPoint);
	if (_hSelf == hWin)
		::SetCursor(::LoadCursor(NULL, IDC_ARROW));
	else
	{
		TCHAR className[256];
		::GetClassName(hWin, className, 256);
		if ((!lstrcmp(className, TEXT("Scintilla"))) || (!lstrcmp(className, WC_TABCONTROL)))
		{
			if (::GetKeyState(VK_LCONTROL) & 0x80000000)
				::SetCursor(::LoadCursor(_hInst, MAKEINTRESOURCE(IDC_DRAG_PLUS_TAB)));
			else
				::SetCursor(::LoadCursor(_hInst, MAKEINTRESOURCE(IDC_DRAG_TAB)));
		}
		else if (isPointInParentZone(screenPoint))
			::SetCursor(::LoadCursor(_hInst, MAKEINTRESOURCE(IDC_DRAG_INTERDIT_TAB)));
		else // drag out of application
			::SetCursor(::LoadCursor(_hInst, MAKEINTRESOURCE(IDC_DRAG_OUT_TAB)));
	}
}

void TabBarPlus::setActiveTab(int tabIndex)
{
	// TCM_SETCURFOCUS is busted on WINE/ReactOS for single line (non-TCS_BUTTONS) tabs...
	// We need it on Windows for multi-line tabs or multiple tabs can appear pressed.
	if (::GetWindowLongPtr(_hSelf, GWL_STYLE) & TCS_BUTTONS)
	{
		::SendMessage(_hSelf, TCM_SETCURFOCUS, tabIndex, 0);
	}

	::SendMessage(_hSelf, TCM_SETCURSEL, tabIndex, 0);
	notify(TCN_SELCHANGE, tabIndex);
}

void TabBarPlus::exchangeTabItemData(int oldTab, int newTab)
{
	//1. shift their data, and insert the source
	TCITEM itemData_nDraggedTab, itemData_shift;
	itemData_nDraggedTab.mask = itemData_shift.mask = TCIF_IMAGE | TCIF_TEXT | TCIF_PARAM;
	const int stringSize = 256;
	TCHAR str1[stringSize];
	TCHAR str2[stringSize];

	itemData_nDraggedTab.pszText = str1;
	itemData_nDraggedTab.cchTextMax = (stringSize);

	itemData_shift.pszText = str2;
	itemData_shift.cchTextMax = (stringSize);

	::SendMessage(_hSelf, TCM_GETITEM, oldTab, reinterpret_cast<LPARAM>(&itemData_nDraggedTab));

	if (oldTab > newTab)
	{
		for (int i = oldTab; i > newTab; i--)
		{
			::SendMessage(_hSelf, TCM_GETITEM, i - 1, reinterpret_cast<LPARAM>(&itemData_shift));
			::SendMessage(_hSelf, TCM_SETITEM, i, reinterpret_cast<LPARAM>(&itemData_shift));
		}
	}
	else
	{
		for (int i = oldTab; i < newTab; ++i)
		{
			::SendMessage(_hSelf, TCM_GETITEM, i + 1, reinterpret_cast<LPARAM>(&itemData_shift));
			::SendMessage(_hSelf, TCM_SETITEM, i, reinterpret_cast<LPARAM>(&itemData_shift));
		}
	}
	::SendMessage(_hSelf, TCM_SETITEM, newTab, reinterpret_cast<LPARAM>(&itemData_nDraggedTab));

	// Tell Notepad_plus to notifiy plugins that a D&D operation was done (so doc index has been changed)
	::SendMessage(_hParent, NPPM_INTERNAL_DOCORDERCHANGED, 0, oldTab);

	//2. set to focus
	setActiveTab(newTab);
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
			if (_previousTabSwapped == nTab)
			{
				return;
			}

			exchangeTabItemData(_nTabDragged, nTab);
			_previousTabSwapped = _nTabDragged;
			_nTabDragged = nTab;
		}
		else
		{
			_previousTabSwapped = -1;
		}
	}
	else
	{
		//::SetCursor(::LoadCursor(_hInst, MAKEINTRESOURCE(IDC_DRAG_TAB)));
		_previousTabSwapped = -1;
		_isDraggingInside = false;
	}

}


CloseButtonZone::CloseButtonZone()
{
	// TODO: get width/height of close button dynamically
	_width = NppParameters::getInstance()._dpiManager.scaleX(11);
	_height = NppParameters::getInstance()._dpiManager.scaleY(11);
}

bool CloseButtonZone::isHit(int x, int y, const RECT & tabRect, bool isVertical) const
{
	RECT buttonRect = getButtonRectFrom(tabRect, isVertical);

	if (x >= buttonRect.left && x <= buttonRect.right && y >= buttonRect.top && y <= buttonRect.bottom)
		return true;

	return false;
}

RECT CloseButtonZone::getButtonRectFrom(const RECT & tabRect, bool isVertical) const
{
	RECT buttonRect;

	int fromBorder;
	if (isVertical)
	{
		fromBorder = (tabRect.right - tabRect.left - _width + 1) / 2;
		buttonRect.left = tabRect.left + fromBorder;
	}
	else
	{
		fromBorder = (tabRect.bottom - tabRect.top - _height + 1) / 2;
		buttonRect.left = tabRect.right - fromBorder - _width;
	}
	buttonRect.top = tabRect.top + fromBorder;
	buttonRect.bottom = buttonRect.top + _height;
	buttonRect.right = buttonRect.left + _width;

	return buttonRect;
}
