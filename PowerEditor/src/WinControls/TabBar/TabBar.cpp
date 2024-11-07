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
#include "Buffer.h"
#include "TabBar.h"
#include "Parameters.h"
#include "DoubleBuffer/DoubleBuffer.h"

#define	IDC_DRAG_TAB     1404
#define	IDC_DRAG_INTERDIT_TAB 1405
#define	IDC_DRAG_PLUS_TAB 1406
#define	IDC_DRAG_OUT_TAB 1407

bool TabBarPlus::_doDragNDrop = false;

bool TabBarPlus::_drawTopBar = true;
bool TabBarPlus::_drawInactiveTab = true;
bool TabBarPlus::_drawTabCloseButton = false;
bool TabBarPlus::_drawTabPinButton = true;
bool TabBarPlus::_isDbClk2Close = false;
bool TabBarPlus::_isCtrlVertical = false;
bool TabBarPlus::_isCtrlMultiLine = false;
bool TabBarPlus::_isReduced = true;

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

	INITCOMMONCONTROLSEX icce{};
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_TAB_CLASSES;
	InitCommonControlsEx(&icce);
	int multiLine = isMultiLine ? TCS_MULTILINE : 0;

	int style = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE |\
		TCS_FOCUSNEVER | TCS_TABS | WS_TABSTOP | vertical | multiLine;

	_hSelf = ::CreateWindowEx(
				0,
				WC_TABCONTROL,
				L"Tab",
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

	if (_hFont == nullptr)
	{
		const UINT dpi = DPIManagerV2::getDpiForWindow(_hParent);
		LOGFONT lf{ DPIManagerV2::getDefaultGUIFontForDpi(dpi) };
		lf.lfHeight = DPIManagerV2::scaleFont(8, dpi);
		_hFont = ::CreateFontIndirect(&lf);
		::SendMessage(_hSelf, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), 0);
	}

}

void TabBar::destroy()
{
	if (_hFont)
	{
		::DeleteObject(_hFont);
		_hFont = nullptr;
	}

	if (_hLargeFont)
	{
		::DeleteObject(_hLargeFont);
		_hLargeFont = nullptr;
	}

	if (_hVerticalFont)
	{
		::DeleteObject(_hVerticalFont);
		_hVerticalFont = nullptr;
	}

	if (_hVerticalLargeFont)
	{
		::DeleteObject(_hVerticalLargeFont);
		_hVerticalLargeFont = nullptr;
	}

	::DestroyWindow(_hSelf);
	_hSelf = nullptr;
}


int TabBar::insertAtEnd(const wchar_t *subTabName)
{
	TCITEM tie{};
	tie.mask = TCIF_TEXT | TCIF_IMAGE;
	int index = -1;

	if (_hasImgLst)
		index = 0;
	tie.iImage = index;
	tie.pszText = (wchar_t *)subTabName;
	return int(::SendMessage(_hSelf, TCM_INSERTITEM, _nbItem++, reinterpret_cast<LPARAM>(&tie)));
}


void TabBar::getCurrentTitle(wchar_t *title, int titleLen)
{
	TCITEM tci{};
	tci.mask = TCIF_TEXT;
	tci.pszText = title;
	tci.cchTextMax = titleLen-1;
	::SendMessage(_hSelf, TCM_GETITEM, getCurrentTabIndex(), reinterpret_cast<LPARAM>(&tci));
}

int TabBar::getNextOrPrevTabIdx(bool isNext) const
{
	const HWND hTab = _hSelf;
	const int lastTabIdx = TabCtrl_GetItemCount(hTab) - 1;
	int selTabIdx = TabCtrl_GetCurSel(hTab);
	if (isNext)
	{
		if (selTabIdx++ == lastTabIdx)
		{
			selTabIdx = 0;
		}
	}
	else
	{
		if (selTabIdx-- == 0)
		{
			selTabIdx = lastTabIdx;
		}
	}
	return selTabIdx;
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
}


void TabBar::deletItemAt(size_t index)
{
	if (index == _nbItem - 1)
	{
		//prevent invisible tabs. If last visible tab is removed, other tabs are put in view but not redrawn
		//Therefore, scroll one tab to the left if only one tab visible
		if (_nbItem > 1)
		{
			RECT itemRect{};
			::SendMessage(_hSelf, TCM_GETITEMRECT, index, reinterpret_cast<LPARAM>(&itemRect));
			if (itemRect.left < 5) //if last visible tab, scroll left once (no more than 5px away should be safe, usually 2px depending on the drawing)
			{
				//To scroll the tab control to the left, use the WM_HSCROLL notification
				//Doesn't really seem to be documented anywhere, but the values do match the message parameters
				//The up/down control really is just some sort of scrollbar
				//There seems to be no negative effect on any internal state of the tab control or the up/down control
				WPARAM wParam = MAKEWPARAM(SB_THUMBPOSITION, index - 1);
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
	RECT rowRect{};
	int rowCount = 0, tabsHight = 0;

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
	tabsHight += _dpiManager.getSystemMetricsForDpi(_isVertical ? SM_CXEDGE : SM_CYEDGE);

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

	if (_hCloseBtnImgLst != nullptr)
	{
		::ImageList_Destroy(_hCloseBtnImgLst);
		_hCloseBtnImgLst = nullptr;
	}

	if (_hPinBtnImgLst != nullptr)
	{
		::ImageList_Destroy(_hPinBtnImgLst);
		_hPinBtnImgLst = nullptr;
	}
}


void TabBarPlus::init(HINSTANCE hInst, HWND parent, bool isVertical, bool isMultiLine)
{
	Window::init(hInst, parent);

	const UINT dpi = DPIManagerV2::getDpiForWindow(_hParent);
	_closeButtonZone.init(_hParent, 0);
	_pinButtonZone.init(_hParent, 1);
	_dpiManager.setDpi(dpi);

	int vertical = isVertical ? (TCS_VERTICAL | TCS_MULTILINE | TCS_RIGHTJUSTIFY) : 0;
	_isVertical = isVertical;
	_isMultiLine = isMultiLine;

	INITCOMMONCONTROLSEX icce{};
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_TAB_CLASSES;
	InitCommonControlsEx(&icce);
	int multiLine = isMultiLine ? TCS_MULTILINE : 0;

	int style = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | TCS_FOCUSNEVER | TCS_TABS | vertical | multiLine;
	style |= TCS_OWNERDRAWFIXED;

	_hSelf = ::CreateWindowEx(0, WC_TABCONTROL,	L"Tab", style,	0, 0, 0, 0, _hParent, NULL, _hInst, 0);

	if (!_hSelf)
	{
		throw std::runtime_error("TabBarPlus::init : CreateWindowEx() function return null");
	}

	_tooltips = ::CreateWindowEx(0, TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP | TTS_NOPREFIX, 0, 0, 0, 0, _hParent, NULL, _hInst, 0);

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
		for (; i < nbCtrlMax && !found; ++i)
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

	DoubleBuffer::subclass(_hSelf);

	setFont();

	setCloseBtnImageList();
	setPinBtnImageList();
}

void TabBar::setFont()
{
	if (_hFont)
		::DeleteObject(_hFont);

	if (_hLargeFont)
		::DeleteObject(_hLargeFont);

	if (_hVerticalFont)
		::DeleteObject(_hVerticalFont);

	if (_hVerticalLargeFont)
		::DeleteObject(_hVerticalLargeFont);

	LOGFONT lf{ DPIManagerV2::getDefaultGUIFontForDpi(_dpiManager.getDpi()) };
	LOGFONT lfVer{ lf };
	if (_hFont != nullptr)
	{
		::DeleteObject(_hFont);
		_hFont = nullptr;
	}
	_hFont = ::CreateFontIndirect(&lf);

	lf.lfWeight = FW_HEAVY;
	lf.lfHeight = DPIManagerV2::scaleFont(10, _dpiManager.getDpi());

	_hLargeFont = ::CreateFontIndirect(&lf);

	lfVer.lfEscapement = 900;
	lfVer.lfOrientation = 900;

	_hVerticalFont = CreateFontIndirect(&lfVer);

	lfVer.lfWeight = FW_HEAVY;

	_hVerticalLargeFont = CreateFontIndirect(&lfVer);
}


void TabBarPlus::doOwnerDrawTab(TabBarPlus* tbpObj)
{
	for (int i = 0 ; i < _nbCtrl ; ++i)
	{
		if (_hwndArray[i])
		{
			LONG_PTR style = ::GetWindowLongPtr(_hwndArray[i], GWL_STYLE);
			style |= TCS_OWNERDRAWFIXED;

			::SetWindowLongPtr(_hwndArray[i], GWL_STYLE, style);
			::InvalidateRect(_hwndArray[i], NULL, TRUE);

			if (tbpObj)
			{
				int paddingSize = 0;
				if (_drawTabCloseButton && _drawTabPinButton) // 2 buttons
				{
					paddingSize = 14;
				}
				else if (!_drawTabCloseButton && !_drawTabPinButton) // no button
				{
					paddingSize = 6;
				}
				else // only 1 button
				{
					paddingSize = 10;
				}
				const int paddingSizeDynamicW = tbpObj->_dpiManager.scale(paddingSize);
				::SendMessage(_hwndArray[i], TCM_SETPADDING, 0, MAKELPARAM(paddingSizeDynamicW, 0));
			}
		}
	}
}


void TabBarPlus::setColour(COLORREF colour2Set, tabColourIndex i, TabBarPlus* tbpObj)
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
	doOwnerDrawTab(tbpObj);
}

void TabBarPlus::tabToStart(int index)
{
	if (index < 0 || index >= static_cast<int>(_nbItem))
		index = getCurrentTabIndex();

	if (index <= 0)
		return;

	for (int i = index, j = index - 1; j >= 0; --i, --j)
	{
		if (!exchangeTabItemData(i, j))
			break;
	}
}

void TabBarPlus::tabToEnd(int index)
{
	if (index < 0 || index >= static_cast<int>(_nbItem))
		index = getCurrentTabIndex();

	if (index >= static_cast<int>(_nbItem))
		return;

	for (int i = index, j = index + 1; j < static_cast<int>(_nbItem); ++i, ++j)
	{
		if (!exchangeTabItemData(i, j))
			break;
	}
}

void TabBarPlus::setCloseBtnImageList()
{
	int iconSize = 0;
	std::vector<int> ids;

	if (NppDarkMode::isEnabled())
	{
		iconSize = g_TabCloseBtnSize_DM;
		ids = { IDR_CLOSETAB_DM, IDR_CLOSETAB_INACT_DM, IDR_CLOSETAB_HOVER_DM, IDR_CLOSETAB_PUSH_DM };
	}
	else
	{
		iconSize = g_TabCloseBtnSize;
		ids = { IDR_CLOSETAB, IDR_CLOSETAB_INACT, IDR_CLOSETAB_HOVER, IDR_CLOSETAB_PUSH };
	}

	if (_hCloseBtnImgLst != nullptr)
	{
		::ImageList_Destroy(_hCloseBtnImgLst);
		_hCloseBtnImgLst = nullptr;
	}

	const int btnSize = _dpiManager.scale(iconSize);

	_hCloseBtnImgLst = ::ImageList_Create(btnSize, btnSize, ILC_COLOR32 | ILC_MASK, static_cast<int>(ids.size()), 0);

	for (const auto& id : ids)
	{
		HICON hIcon = nullptr;
		DPIManagerV2::loadIcon(_hInst, MAKEINTRESOURCE(id), btnSize, btnSize, &hIcon);
		::ImageList_AddIcon(_hCloseBtnImgLst, hIcon);
		::DestroyIcon(hIcon);
	}

	_closeButtonZone._width = btnSize;
	_closeButtonZone._height = btnSize;
}


void TabBarPlus::setPinBtnImageList()
{
	int iconSize = 0;
	std::vector<int> ids;

	if (NppDarkMode::isEnabled())
	{
		iconSize = g_TabPinBtnSize_DM;
		ids = { IDR_PINTAB_DM, IDR_PINTAB_HOVER_DM, IDR_PINTAB_PINNED_DM, IDR_PINTAB_PINNEDHOVER_DM };
	}
	else
	{
		iconSize = g_TabPinBtnSize;
		ids = { IDR_PINTAB, IDR_PINTAB_HOVER, IDR_PINTAB_PINNED, IDR_PINTAB_PINNEDHOVER };
	}

	if (_hPinBtnImgLst != nullptr)
	{
		::ImageList_Destroy(_hPinBtnImgLst);
		_hPinBtnImgLst = nullptr;
	}

	const int btnSize = _dpiManager.scale(iconSize);

	_hPinBtnImgLst = ::ImageList_Create(btnSize, btnSize, ILC_COLOR32 | ILC_MASK, static_cast<int>(ids.size()), 0);

	for (const auto& id : ids)
	{
		HICON hIcon = nullptr;
		DPIManagerV2::loadIcon(_hInst, MAKEINTRESOURCE(id), btnSize, btnSize, &hIcon);
		::ImageList_AddIcon(_hPinBtnImgLst, hIcon);
		::DestroyIcon(hIcon);
	}

	_pinButtonZone._width = btnSize;
	_pinButtonZone._height = btnSize;
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
	TBHDR nmhdr{};
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
			setCloseBtnImageList();
			setPinBtnImageList();
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
				RECT rcTabCtrl{}, rcLastTab{};
				::SendMessage(_hSelf, TCM_GETITEMRECT, lastTabIndex, reinterpret_cast<LPARAM>(&rcLastTab));
				::GetClientRect(_hSelf, &rcTabCtrl);

				// get index of the first visible tab
				TC_HITTESTINFO hti{};
				LONG xy = _dpiManager.scale(12); // an arbitrary coordinate inside the first visible tab
				hti.pt = { xy, xy };
				int scrollTabIndex = static_cast<int32_t>(::SendMessage(_hSelf, TCM_HITTEST, 0, reinterpret_cast<LPARAM>(&hti)));

				if (scrollTabIndex < 1 && (_isVertical ? rcLastTab.bottom < rcTabCtrl.bottom : rcLastTab.right < rcTabCtrl.right)) // nothing to scroll
					return TRUE;

				// maximal width/height of the msctls_updown32 class (arrow box in the tab bar), 
				// this area may hide parts of the last tab and needs to be excluded
				LONG maxLengthUpDownCtrl = _dpiManager.scale(44); // sufficient static value

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
					if (_isCloseHover || _isPinHover)
					{
						_isCloseHover = false;
						_isPinHover = false;
						::InvalidateRect(_hSelf, &_currentHoverTabRect, false);
					}

					::SendMessage(_hSelf, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, scrollTabIndex), 0);
				}
			}
			return TRUE;
		}

		case WM_LBUTTONDOWN :
		{
			int xPos = LOWORD(lParam);
			int yPos = HIWORD(lParam);

			if (::GetWindowLongPtr(_hSelf, GWL_STYLE) & TCS_BUTTONS)
			{
				int nTab = getTabIndexAt(xPos, yPos);
				if (nTab != -1 && nTab != static_cast<int32_t>(::SendMessage(_hSelf, TCM_GETCURSEL, 0, 0)))
				{
					setActiveTab(nTab);
				}
			}

			if (_drawTabCloseButton)
			{
				if (_closeButtonZone.isHit(xPos, yPos, _currentHoverTabRect, _isVertical))
				{
					_whichCloseClickDown = getTabIndexAt(xPos, yPos);
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_REFRESHTABAR, 0);
					return TRUE;
				}
			}

			if (_drawTabPinButton)
			{
				if (_pinButtonZone.isHit(xPos, yPos, _currentHoverTabRect, _isVertical))
				{
					_whichPinClickDown = getTabIndexAt(xPos, yPos);
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

			POINT p{};
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

					if (_currentHoverTabItem != -1) // tab item is being hovered
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

				if (_drawTabPinButton)
				{
					RECT currentHoverTabRectOld = _currentHoverTabRect;
					bool isPinHoverOld = _isPinHover;

					if (_currentHoverTabItem != -1) // tab item is being hovered
					{
						::SendMessage(_hSelf, TCM_GETITEMRECT, _currentHoverTabItem, reinterpret_cast<LPARAM>(&_currentHoverTabRect));
						_isPinHover = _pinButtonZone.isHit(p.x, p.y, _currentHoverTabRect, _isVertical);
						_isPinHover = _pinButtonZone.isHit(p.x, p.y, _currentHoverTabRect, _isVertical);
					}
					else
					{
						SetRectEmpty(&_currentHoverTabRect);
						_isPinHover = false;
					}

					if (isFromTabToTab || _isPinHover != isPinHoverOld)
					{
						if (isPinHoverOld && (isFromTabToTab || !_isPinHover))
							InvalidateRect(hwnd, &currentHoverTabRectOld, FALSE);

						if (_isPinHover)
							InvalidateRect(hwnd, &_currentHoverTabRect, FALSE);
					}

					if (_isPinHover)
					{
						// Mouse moves out from pin zone will send WM_MOUSELEAVE message
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
			if (_isCloseHover || _isPinHover)
				InvalidateRect(hwnd, &_currentHoverTabRect, FALSE);

			_currentHoverTabItem = -1;
			_whichCloseClickDown = -1;
			_whichPinClickDown = -1;
			SetRectEmpty(&_currentHoverTabRect);
			_isCloseHover = false;
			_isPinHover = false;

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

			if (_drawTabPinButton)
			{
				if ((_whichPinClickDown == currentTabOn) && _pinButtonZone.isHit(xPos, yPos, _currentHoverTabRect, _isVertical))
				{
					notify(TCN_TABPINNED, currentTabOn);
					_whichPinClickDown = -1;

					// Get the next tab at same position
					// If valid tab is found then
					//	 update the current hover tab RECT (_currentHoverTabRect)
					//	 update pin hover flag (_isPinHover), so that x will be highlighted or not based on new _currentHoverTabRect
					int nextTab = getTabIndexAt(xPos, yPos);
					if (nextTab != -1)
					{
						::SendMessage(_hSelf, TCM_GETITEMRECT, nextTab, reinterpret_cast<LPARAM>(&_currentHoverTabRect));
						_isPinHover = _pinButtonZone.isHit(xPos, yPos, _currentHoverTabRect, _isVertical);
					}
					return TRUE;
				}
				_whichPinClickDown = -1;
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
			if (currentTabOn != -1)
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
				break;	// Let the control paint background the default way
			}

			RECT rc{};
			::GetClientRect(hwnd, &rc);
			::FillRect(reinterpret_cast<HDC>(wParam), &rc, NppDarkMode::getDarkerBackgroundBrush());
			return TRUE;
		}

		case WM_PAINT:
		case WM_PRINTCLIENT:
		{
			LONG_PTR dwStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
			if (!NppDarkMode::isEnabled() || !(dwStyle & TCS_OWNERDRAWFIXED))
			{
				break;	// Let the control paint itself the default way
			}

			PAINTSTRUCT ps{};
			HDC hdc = (Message == WM_PAINT) ? ::BeginPaint(hwnd, &ps) : reinterpret_cast<HDC>(wParam);

			const bool hasMultipleLines = ((dwStyle & TCS_BUTTONS) == TCS_BUTTONS);

			UINT id = ::GetDlgCtrlID(hwnd);

			auto holdPen = static_cast<HPEN>(::SelectObject(hdc, NppDarkMode::getEdgePen()));

			HRGN holdClip = CreateRectRgn(0, 0, 0, 0);
			if (1 != GetClipRgn(hdc, holdClip))
			{
				DeleteObject(holdClip);
				holdClip = nullptr;
			}

			int paddingDynamicTwoX = _dpiManager.scale(2);
			int paddingDynamicTwoY = paddingDynamicTwoX;

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

				if (::RectVisible(hdc, &dis.rcItem))
				{
					if (!hasMultipleLines)
					{
						if (_isVertical)
						{
							POINT edges[] = {
								{dis.rcItem.left, dis.rcItem.bottom - 1},
								{dis.rcItem.right, dis.rcItem.bottom - 1}
							};

							if (i != nSelTab && (i != nSelTab - 1))
							{
								edges[0].x += paddingDynamicTwoX;
							}

							::Polyline(hdc, edges, _countof(edges));
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
								edges[0].y += paddingDynamicTwoY;
							}

							::Polyline(hdc, edges, _countof(edges));
							dis.rcItem.right -= 1;
						}
					}

					HRGN hClip = CreateRectRgnIndirect(&dis.rcItem);

					SelectClipRgn(hdc, hClip);

					drawItem(&dis, NppDarkMode::isEnabled());

					DeleteObject(hClip);

					SelectClipRgn(hdc, holdClip);
				}
			}

			if (!hasMultipleLines)
			{
				RECT rcFirstTab{};
				TabCtrl_GetItemRect(hwnd, 0, &rcFirstTab);

				if (_isVertical)
				{
					POINT edges[] = {
						{rcFirstTab.left, rcFirstTab.top},
						{rcFirstTab.right, rcFirstTab.top}
					};

					if (nSelTab != 0)
					{
						edges[0].x += paddingDynamicTwoX;
					}

					::Polyline(hdc, edges, _countof(edges));
				}
				else
				{
					POINT edges[] = {
						{rcFirstTab.left, rcFirstTab.top},
						{rcFirstTab.left, rcFirstTab.bottom}
					};

					if (nSelTab != 0)
					{
						edges[0].y += paddingDynamicTwoY;
					}

					::Polyline(hdc, edges, _countof(edges));
				}
			}

			SelectClipRgn(hdc, holdClip);
			if (holdClip)
			{
				DeleteObject(holdClip);
				holdClip = nullptr;
			}

			SelectObject(hdc, holdPen);

			if (Message == WM_PAINT)
			{
				::EndPaint(hwnd, &ps);
			}

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

void TabBarPlus::drawItem(DRAWITEMSTRUCT* pDrawItemStruct, bool isDarkMode)
{
	RECT rect = pDrawItemStruct->rcItem;

	int nTab = pDrawItemStruct->itemID;
	assert(nTab >= 0);

	bool isSelected = (nTab == ::SendMessage(_hSelf, TCM_GETCURSEL, 0, 0));

	wchar_t label[MAX_PATH] = { '\0' };
	TCITEM tci{};
	tci.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;
	tci.pszText = label;
	tci.cchTextMax = MAX_PATH-1;

	::SendMessage(_hSelf, TCM_GETITEM, nTab, reinterpret_cast<LPARAM>(&tci));
	
	const COLORREF colorActiveBg = isDarkMode ? NppDarkMode::getSofterBackgroundColor() : ::GetSysColor(COLOR_BTNFACE);
	const COLORREF colorInactiveBgBase = isDarkMode ? NppDarkMode::getBackgroundColor() : ::GetSysColor(COLOR_BTNFACE);
	
	COLORREF colorInactiveBg = liteGrey;
	COLORREF colorActiveText = ::GetSysColor(COLOR_BTNTEXT);
	COLORREF colorInactiveText = grey;

	if (!NppDarkMode::useTabTheme() && isDarkMode)
	{
		colorInactiveBg = NppDarkMode::getBackgroundColor();
		colorActiveText = NppDarkMode::getTextColor();
		colorInactiveText = NppDarkMode::getDarkerTextColor();
	}
	else
	{
		colorInactiveBg = _inactiveBgColour;
		colorActiveText = _activeTextColour;
		colorInactiveText = _inactiveTextColour;
	}

	HDC hDC = pDrawItemStruct->hDC;

	int nSavedDC = ::SaveDC(hDC);

	::SetBkMode(hDC, TRANSPARENT);
	HBRUSH hBrush = ::CreateSolidBrush(colorInactiveBgBase);
	::FillRect(hDC, &rect, hBrush);
	::DeleteObject(static_cast<HGDIOBJ>(hBrush));

	// equalize drawing areas of active and inactive tabs
	int paddingDynamicTwoX = _dpiManager.scale(2);
	int paddingDynamicTwoY = paddingDynamicTwoX;
	if (isSelected && !isDarkMode)
	{
		// the drawing area of the active tab extends on all borders by default
		const int xEdge = _dpiManager.getSystemMetricsForDpi(SM_CXEDGE);
		const int yEdge = _dpiManager.getSystemMetricsForDpi(SM_CYEDGE);
		::InflateRect(&rect, -xEdge, -yEdge);
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
	const bool hasMultipleLines = ((::GetWindowLongPtr(_hSelf, GWL_STYLE) & TCS_BUTTONS) == TCS_BUTTONS);
	if (hasMultipleLines)
	{
		if (_isVertical)
		{
			rect.left -= paddingDynamicTwoX;
		}
		else
		{
			rect.top -= paddingDynamicTwoY;
		}
	}

	const int individualColourId = getIndividualTabColourId(nTab);

	// draw highlights on tabs (top bar for active tab / darkened background for inactive tab)
	RECT barRect = rect;
	NppParameters& nppParam = NppParameters::getInstance();
	if (isSelected)
	{
		hBrush = ::CreateSolidBrush(colorActiveBg);
		::FillRect(hDC, &pDrawItemStruct->rcItem, hBrush);
		::DeleteObject(static_cast<HGDIOBJ>(hBrush));

		if (_drawTopBar)
		{
			int topBarHeight = _dpiManager.scale(4);
			if (_isVertical)
			{
				barRect.left -= (hasMultipleLines && isDarkMode) ? 0 : paddingDynamicTwoX;
				barRect.right = barRect.left + topBarHeight;
			}
			else
			{
				barRect.top -= (hasMultipleLines && isDarkMode) ? 0 : paddingDynamicTwoY;
				barRect.bottom = barRect.top + topBarHeight;
			}

			const bool isFocused = ::SendMessage(_hParent, NPPM_INTERNAL_ISFOCUSEDTAB, 0, reinterpret_cast<LPARAM>(_hSelf));
			COLORREF topBarColour = isFocused ? _activeTopBarFocusedColour : _activeTopBarUnfocusedColour; // #FAAA3C, #FAD296

			if (individualColourId != -1)
			{
				topBarColour = nppParam.getIndividualTabColor(individualColourId, isDarkMode, isFocused);
			}

			hBrush = ::CreateSolidBrush(topBarColour);

			::FillRect(hDC, &barRect, hBrush);
			::DeleteObject(static_cast<HGDIOBJ>(hBrush));
		}
	}
	else // inactive tabs
	{
		RECT inactiveRect = hasMultipleLines ? pDrawItemStruct->rcItem : barRect;
		COLORREF brushColour{};

		if (_drawInactiveTab && individualColourId == -1)
		{
			brushColour = colorInactiveBg;
		}
		else if (individualColourId != -1)
		{
			brushColour = nppParam.getIndividualTabColor(individualColourId, isDarkMode, false);
		}
		else
		{
			brushColour = colorActiveBg;
		}

		hBrush = ::CreateSolidBrush(brushColour);
		::FillRect(hDC, &inactiveRect, hBrush);
		::DeleteObject(static_cast<HGDIOBJ>(hBrush));
	}

	if (isDarkMode && hasMultipleLines)
	{
		::FrameRect(hDC, &pDrawItemStruct->rcItem, NppDarkMode::getEdgeBrush());
	}

	// draw close button
	if (_drawTabCloseButton && _hCloseBtnImgLst != nullptr)
	{
		// 3 status for each inactive tab and selected tab close item :
		// normal / hover / pushed
		int idxCloseImg = _closeTabIdx; // selected

		if (_isCloseHover && (_currentHoverTabItem == nTab))
		{
			if (_whichCloseClickDown == -1) // hover
			{
				idxCloseImg = _closeTabHoverIdx;
			}
			else if (_whichCloseClickDown == _currentHoverTabItem) // pushed
			{
				idxCloseImg = _closeTabPushIdx;
			}
		}
		else if (!isSelected) // inactive
		{
			idxCloseImg = _closeTabInactIdx;
		}

		RECT buttonRect = _closeButtonZone.getButtonRectFrom(rect, _isVertical);

		::ImageList_Draw(_hCloseBtnImgLst, idxCloseImg, hDC, buttonRect.left, buttonRect.top, ILD_TRANSPARENT);
	}

	// draw pin button
	if (_drawTabPinButton && _hPinBtnImgLst != nullptr)
	{
		// Each tab combined with the following stats :
		// (active / inactive) | (pinned / unpinned) | (hover / not hover / pushed)
		

		bool isPinned = reinterpret_cast<Buffer*>(tci.lParam)->isPinned();
		int idxPinImg = _unpinnedIdx; // current: upinned as default


		if (isPinned)
		{
			if (!isSelected) // inactive
			{
				if (_isPinHover && (_currentHoverTabItem == nTab))
				{
					if (_whichPinClickDown == -1) // hover
					{
						idxPinImg = _pinnedHoverIdx;
					}
					else if (_whichPinClickDown == _currentHoverTabItem) // pushed
					{
						idxPinImg = _unpinnedIdx;
					}

				}
				else // pinned inactive
				{
					idxPinImg = _pinnedIdx;
				}
			}
			else // current
			{
				if (_isPinHover && (_currentHoverTabItem == nTab)) // hover
					idxPinImg = _pinnedHoverIdx;
				else
					idxPinImg = _pinnedIdx;
			}

		}
		else // unpinned
		{
			if (!isSelected) // inactive
			{
				if (_isPinHover && (_currentHoverTabItem == nTab))
				{
					if (_whichPinClickDown == -1) // hover
					{
						idxPinImg = _unpinnedHoverIdx;
					}
					else if (_whichPinClickDown == _currentHoverTabItem) // pushed
					{
						idxPinImg = _pinnedIdx;
					}

				}
				else // unpinned inactive
				{
					idxPinImg = _unpinnedIdx;
				}
			}
			else // current
			{
				if (_isPinHover && (_currentHoverTabItem == nTab)) // hover
					idxPinImg = _unpinnedHoverIdx;
				else
					idxPinImg = _unpinnedIdx;
			}
		}

		RECT buttonRect = _pinButtonZone.getButtonRectFrom(rect, _isVertical);

		::ImageList_Draw(_hPinBtnImgLst, idxPinImg, hDC, buttonRect.left, buttonRect.top, ILD_TRANSPARENT);
	}

	// draw image
	HIMAGELIST hImgLst = (HIMAGELIST)::SendMessage(_hSelf, TCM_GETIMAGELIST, 0, 0);

	if (hImgLst && tci.iImage >= 0)
	{
		IMAGEINFO info{};
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
	SIZE charPixel{};
	::GetTextExtentPoint(hDC, L" ", 1, &charPixel);
	int spaceUnit = charPixel.cx;

	TEXTMETRIC textMetrics{};
	GetTextMetrics(hDC, &textMetrics);
	int textHeight = textMetrics.tmHeight;
	int textDescent = textMetrics.tmDescent;

	int flags = DT_SINGLELINE | DT_NOPREFIX;

	// This code will read in one character at a time and remove every first ampersand (&).
	// ex. If input "test && test &&& test &&&&" then output will be "test & test && test &&&".
	// Tab's caption must be encoded like this because otherwise tab control would make tab too small or too big for the text.
	wchar_t decodedLabel[MAX_PATH] = { '\0' };
	const wchar_t* in = label;
	wchar_t* out = decodedLabel;
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
		flags |= DT_LEFT;
		flags |= DT_BOTTOM;
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
		flags |= DT_LEFT;
		flags |= DT_TOP;

		const int paddingText = ((pDrawItemStruct->rcItem.bottom - pDrawItemStruct->rcItem.top) - (textHeight + textDescent)) / 2;
		const int paddingDescent = !hasMultipleLines ? ((textDescent + ((isDarkMode || !isSelected) ? 1 : 0)) / 2) : 0;
		rect.top = pDrawItemStruct->rcItem.top + paddingText + paddingDescent;
		rect.bottom = pDrawItemStruct->rcItem.bottom - paddingText + paddingDescent;

		if (isDarkMode || !isSelected || _drawTopBar)
		{
			rect.top += paddingDynamicTwoY;
		}

		// 1 space distance to save icon
		rect.left += spaceUnit;
	}

	COLORREF textColor = isSelected ? colorActiveText : colorInactiveText;

	::SetTextColor(hDC, textColor);

	::DrawText(hDC, decodedLabel, lstrlen(decodedLabel), &rect, flags);
	::RestoreDC(hDC, nSavedDC);
}


void TabBarPlus::draggingCursor(POINT screenPoint)
{
	HWND hWin = ::WindowFromPoint(screenPoint);
	if (_hSelf == hWin)
		::SetCursor(::LoadCursor(NULL, IDC_ARROW));
	else
	{
		wchar_t className[256] = { '\0' };
		::GetClassName(hWin, className, 256);
		if ((!lstrcmp(className, L"Scintilla")) || (!lstrcmp(className, WC_TABCONTROL)))
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

bool TabBarPlus::exchangeTabItemData(int oldTab, int newTab)
{
	//1. shift their data, and insert the source
	TCITEM itemData_nDraggedTab{}, itemData_shift{};
	itemData_nDraggedTab.mask = itemData_shift.mask = TCIF_IMAGE | TCIF_TEXT | TCIF_PARAM;
	const int stringSize = 256;
	wchar_t str1[stringSize] = { '\0' };
	wchar_t str2[stringSize] = { '\0' };

	itemData_nDraggedTab.pszText = str1;
	itemData_nDraggedTab.cchTextMax = (stringSize);

	itemData_shift.pszText = str2;
	itemData_shift.cchTextMax = (stringSize);

	::SendMessage(_hSelf, TCM_GETITEM, oldTab, reinterpret_cast<LPARAM>(&itemData_nDraggedTab));
	Buffer* chosenBuf = reinterpret_cast<Buffer*>(itemData_nDraggedTab.lParam);

	::SendMessage(_hSelf, TCM_GETITEM, newTab, reinterpret_cast<LPARAM>(&itemData_shift));
	Buffer* shiftBuf = reinterpret_cast<Buffer*>(itemData_shift.lParam);

	if (chosenBuf->isPinned() != shiftBuf->isPinned())
		return false;

	int i = oldTab;
	if (oldTab > newTab)
	{
		for (; i > newTab; i--)
		{
			::SendMessage(_hSelf, TCM_GETITEM, i - 1, reinterpret_cast<LPARAM>(&itemData_shift));
			::SendMessage(_hSelf, TCM_SETITEM, i, reinterpret_cast<LPARAM>(&itemData_shift));
		}
	}
	else
	{
		for (; i < newTab; ++i)
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

	return true;
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

			if (exchangeTabItemData(_nTabDragged, nTab))
			{
				_previousTabSwapped = _nTabDragged;
				_nTabDragged = nTab;
			}
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


bool CloseButtonZone::isHit(int x, int y, const RECT & tabRect, bool isVertical) const
{
	RECT buttonRect = getButtonRectFrom(tabRect, isVertical);

	if (x >= buttonRect.left && x <= buttonRect.right && y >= buttonRect.top && y <= buttonRect.bottom)
		return true;

	return false;
}

RECT CloseButtonZone::getButtonRectFrom(const RECT & tabRect, bool isVertical) const
{
	RECT buttonRect{};
	const int inBetween = 8;

	int fromBorder = 0;
	if (isVertical)
	{
		fromBorder = (tabRect.right - tabRect.left - _width + 1) / 2;
		if (_order == 0)
		{
			buttonRect.top = tabRect.top + fromBorder;
		}
		else if (_order == 1)
		{
			buttonRect.top = tabRect.top + fromBorder + _height + inBetween;
		}

		buttonRect.left = tabRect.left + fromBorder;
	}
	else
	{
		fromBorder = (tabRect.bottom - tabRect.top - _height + 1) / 2;
		if (_order == 0)
		{
			buttonRect.left = tabRect.right - fromBorder - _width;
		}
		else if (_order == 1)
		{
			buttonRect.left = tabRect.right - fromBorder - _width * 2 - inBetween;
		}

		buttonRect.top = tabRect.top + fromBorder;
	}
	
	buttonRect.bottom = buttonRect.top + _height;
	buttonRect.right = buttonRect.left + _width;

	return buttonRect;
}
