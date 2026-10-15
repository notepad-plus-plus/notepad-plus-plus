// This file is part of Notepad++ project
// Copyright (c) 2026 LucaSorvillo and Notepad++ team

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

// Vertical tab bar with horizontal titles (Preferences > Tab Bar: "Vertical" and "Horizontal titles").
// How it works ("list mode"):
// - the tab control is a horizontal multi-line one (no TCS_VERTICAL) with fixed-width tabs as wide as the
//   column, so there is one tab per row and the titles are horizontal. Click, close/pin buttons, tooltips,
//   drag and drop and dark mode painting keep using the native geometry, so they keep working;
// - TCS_BUTTONS keeps the rows in order (no row shuffling when a tab is selected);
// - the control is as tall as all its tabs, moved up by the scroll offset and clipped to the visible column
//   with a window region (no reparenting: the notifications are unchanged);
// - the column width comes from the Preferences and can be changed by dragging the right edge: the strip
//   between the tabs and the editor, or the left border line of the editor (a subclass of the editor windows
//   passes that drag to the column).
// TabBar.cpp treats list mode as a horizontal tab bar (its isVertical checks call isListMode()).
#include "VerticalTabs.h"

#include <windowsx.h>
#include <commctrl.h>

#include <algorithm>
#include <cwchar>
#include <map>

#include "NppConstants.h"
#include "Parameters.h"
#include "TabBar.h"
#include "dpiManagerV2.h"
#include "resource.h"

namespace
{
	constexpr UINT_PTR subclassID = 1;
	constexpr UINT_PTR editorSubclassID = 1;

	struct TabState
	{
		bool listMode = false;     // list mode applied to the control
		RECT slot{};               // area given by the layout, Notepad++ convention: left, top, width, height
		int width = 0;             // column width (pixels): the space taken from the editor
		int gap = 0;               // space DocTabView leaves before the editor (border width): covered by the column
		int contentHeight = 0;     // bottom of the last tab
		int scroll = 0;            // vertical scroll offset (pixels)
		int wheelRemainder = 0;    // touchpads send small wheel deltas
		int requestedCx = 0;       // last TCM_SETITEMSIZE asked by Notepad++
		int requestedCy = 0;
		bool resizing = false;     // dragging the right edge
		int resizeStartX = 0;
		int resizeStartWidth = 0;
	};

	std::map<HWND, TabState> g_states;

	UINT dpiOf(HWND hwnd)
	{
		return DPIManagerV2::getDpiForWindow(hwnd);
	}

	int scaled(HWND hwnd, int value)
	{
		return DPIManagerV2::scale(value, dpiOf(hwnd));
	}

	int itemWidthFor(HWND hwnd, const TabState& st)
	{
		return std::max(scaled(hwnd, 40), st.width - scaled(hwnd, 6));
	}

	int rowHeightOf(HWND hwnd)
	{
		RECT rc{};
		if (TabCtrl_GetItemCount(hwnd) > 0 && TabCtrl_GetItemRect(hwnd, 0, &rc))
			return std::max(1, static_cast<int>(rc.bottom - rc.top));
		return scaled(hwnd, 22);
	}

	void setListStyle(HWND hwnd, bool listMode)
	{
		LONG_PTR style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
		const LONG_PTR oldStyle = style;
		if (listMode)
		{
			style &= ~static_cast<LONG_PTR>(TCS_VERTICAL | TCS_RIGHTJUSTIFY);
			style |= TCS_MULTILINE | TCS_FIXEDWIDTH | TCS_BUTTONS;
		}
		else
		{
			// the styles of the original code (TabBarPlus::init, doVertical, doMultiLine);
			// TCS_BUTTONS is managed by the original TabBar::reSizeTo()
			const int tabStatus = NppParameters::getInstance().getNppGUI()._tabStatus;
			style &= ~static_cast<LONG_PTR>(TCS_FIXEDWIDTH | TCS_VERTICAL | TCS_RIGHTJUSTIFY | TCS_MULTILINE);
			if (tabStatus & TAB_VERTICAL)
				style |= TCS_VERTICAL | TCS_MULTILINE | TCS_RIGHTJUSTIFY;
			if (tabStatus & TAB_MULTILINE)
				style |= TCS_MULTILINE;
		}

		if (style != oldStyle)
			::SetWindowLongPtr(hwnd, GWL_STYLE, style);
	}

	// Moves the (tall) control by the scroll offset and clips it to the visible column
	void applyScroll(HWND hwnd, TabState& st)
	{
		const int slotHeight = std::max(0, static_cast<int>(st.slot.bottom));
		const int maxScroll = std::max(0, st.contentHeight - slotHeight);
		st.scroll = std::clamp(st.scroll, 0, maxScroll);

		const int totalHeight = std::max(slotHeight, st.contentHeight);
		const int windowWidth = st.width + st.gap;
		::MoveWindow(hwnd, st.slot.left, st.slot.top - st.scroll, windowWidth, totalHeight, TRUE);
		::SetWindowRgn(hwnd, ::CreateRectRgn(0, st.scroll, windowWidth, st.scroll + slotHeight), TRUE);
	}

	void layoutColumn(HWND hwnd, TabState& st)
	{
		// The rows only depend on the width: the control gets the column width before the tabs are measured.
		// It is not moved for that: a move without repainting, followed by a move back that changes nothing,
		// would leave the pixels of the old position on the screen (tabs missing until the mouse hovers them).
		const int windowWidth = st.width + st.gap;
		RECT rcWindow{};
		::GetWindowRect(hwnd, &rcWindow);
		if (rcWindow.right - rcWindow.left != windowWidth)
			::SetWindowPos(hwnd, nullptr, 0, 0, windowWidth, rcWindow.bottom - rcWindow.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

		st.contentHeight = 0;
		const int count = TabCtrl_GetItemCount(hwnd);
		RECT rcLast{};
		if (count > 0 && TabCtrl_GetItemRect(hwnd, count - 1, &rcLast))
			st.contentHeight = rcLast.bottom + scaled(hwnd, 2);

		applyScroll(hwnd, st);
	}

	void ensureVisible(HWND hwnd, TabState& st, int index)
	{
		RECT rc{};
		if (index < 0 || !TabCtrl_GetItemRect(hwnd, index, &rc))
			return;

		const int slotHeight = static_cast<int>(st.slot.bottom);
		if (rc.top < st.scroll)
			st.scroll = rc.top;
		else if (rc.bottom > st.scroll + slotHeight)
			st.scroll = rc.bottom - slotHeight;
		else
			return;

		applyScroll(hwnd, st);
	}

	// Grab area to resize the column: from the end of the tabs up to the editor
	bool isInResizeZone(HWND hwnd, const TabState& st, int clientX)
	{
		return clientX >= itemWidthFor(hwnd, st) && clientX < st.width + st.gap;
	}

	void beginResize(HWND hwnd, TabState& st, int clientX)
	{
		// client coordinates: the left edge of the column does not move while it is resized
		st.resizing = true;
		st.resizeStartX = clientX;
		st.resizeStartWidth = st.width;
		::SetCapture(hwnd);
	}

	void enterListMode(HWND hwnd, TabState& st)
	{
		st.listMode = true;
		st.scroll = 0;
		setListStyle(hwnd, true);
	}

	void leaveListMode(HWND hwnd, TabState& st)
	{
		st.listMode = false;
		st.scroll = 0;
		st.resizing = false;
		::SetWindowRgn(hwnd, nullptr, TRUE);
		setListStyle(hwnd, false);
		if (st.requestedCx > 0 && st.requestedCy > 0)
			::SendMessage(hwnd, TCM_SETITEMSIZE, 0, MAKELPARAM(st.requestedCx, st.requestedCy));
	}

	LRESULT CALLBACK tabBarSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR idSubclass, DWORD_PTR /*refData*/)
	{
		auto it = g_states.find(hwnd);
		if (it == g_states.end() || message == WM_NCDESTROY)
		{
			if (message == WM_NCDESTROY)
			{
				::RemoveWindowSubclass(hwnd, tabBarSubclassProc, idSubclass);
				if (it != g_states.end())
					g_states.erase(it);
			}
			return ::DefSubclassProc(hwnd, message, wParam, lParam);
		}

		TabState& st = it->second;
		switch (message)
		{
			case TCM_SETITEMSIZE:
			{
				// Notepad++ sets the tab size (init, DPI change, "Reduce"...): in list mode the width is the column one
				st.requestedCx = LOWORD(lParam);
				st.requestedCy = HIWORD(lParam);
				if (st.listMode)
					lParam = MAKELPARAM(itemWidthFor(hwnd, st), st.requestedCy);
				break;
			}

			case TCM_SETCURSEL:
			case TCM_SETCURFOCUS:
			{
				const LRESULT result = ::DefSubclassProc(hwnd, message, wParam, lParam);
				if (st.listMode)
					ensureVisible(hwnd, st, static_cast<int>(wParam));
				return result;
			}

			case WM_MOUSEWHEEL:
			{
				// plain wheel scrolls the column; Ctrl / Shift + wheel keep their original meaning (switch / move tabs)
				if (st.listMode && !(GET_KEYSTATE_WPARAM(wParam) & (MK_CONTROL | MK_SHIFT)))
				{
					st.wheelRemainder += GET_WHEEL_DELTA_WPARAM(wParam);
					const int pixels = ::MulDiv(st.wheelRemainder, 3 * rowHeightOf(hwnd), WHEEL_DELTA);
					if (pixels != 0)
					{
						st.wheelRemainder -= ::MulDiv(pixels, WHEEL_DELTA, 3 * rowHeightOf(hwnd));
						st.scroll -= pixels;
						applyScroll(hwnd, st);
					}
					return 0;
				}
				break;
			}

			case WM_NCHITTEST:
			{
				// outside its tabs the native control answers HTTRANSPARENT (the mouse goes to the main window):
				// the grab area must get the mouse
				if (st.listMode)
				{
					POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
					::ScreenToClient(hwnd, &pt);
					if (isInResizeZone(hwnd, st, pt.x))
						return HTCLIENT;
				}
				break;
			}

			case WM_SETCURSOR:
			{
				if (st.listMode && LOWORD(lParam) == HTCLIENT)
				{
					POINT pt{};
					::GetCursorPos(&pt);
					::ScreenToClient(hwnd, &pt);
					if (st.resizing || isInResizeZone(hwnd, st, pt.x))
					{
						::SetCursor(::LoadCursor(nullptr, IDC_SIZEWE));
						return TRUE;
					}
				}
				break;
			}

			case WM_LBUTTONDOWN:
			case WM_LBUTTONDBLCLK: // a double click in the grab area must not open a new document
			{
				if (st.listMode && isInResizeZone(hwnd, st, GET_X_LPARAM(lParam)))
				{
					beginResize(hwnd, st, GET_X_LPARAM(lParam));
					return 0;
				}
				break;
			}

			case WM_MOUSEMOVE:
			{
				if (st.resizing)
				{
					const int newWidth = st.resizeStartWidth + (GET_X_LPARAM(lParam) - st.resizeStartX);
					const int dpi = static_cast<int>(dpiOf(hwnd));
					const int newSetting = std::clamp(::MulDiv(newWidth, USER_DEFAULT_SCREEN_DPI, dpi), VERTICAL_TAB_LIST_WIDTH_MIN, VERTICAL_TAB_LIST_WIDTH_MAX);

					NppGUI& nppGUI = NppParameters::getInstance().getNppGUI();
					if (newSetting != nppGUI._verticalTabListWidth)
					{
						nppGUI._verticalTabListWidth = newSetting;
						::SendMessage(::GetParent(hwnd), WM_SIZE, 0, 0); // relayout of both views
						::SendMessage(::GetParent(hwnd), NPPM_INTERNAL_VERTICALTABWIDTHCHANGED, 0, 0); // for Preferences > Tab Bar
					}
					return 0;
				}

				// dragging a tab near the top or the bottom of the column scrolls it
				if (st.listMode && ::GetCapture() == hwnd && (wParam & MK_LBUTTON))
				{
					const int y = GET_Y_LPARAM(lParam);
					const int rowHeight = rowHeightOf(hwnd);
					const int slotHeight = static_cast<int>(st.slot.bottom);
					if (y < st.scroll + rowHeight / 2 && st.scroll > 0)
					{
						st.scroll -= rowHeight / 2;
						applyScroll(hwnd, st);
					}
					else if (y > st.scroll + slotHeight - rowHeight / 2)
					{
						st.scroll += rowHeight / 2;
						applyScroll(hwnd, st);
					}
				}
				break;
			}

			case WM_LBUTTONUP:
			{
				if (st.resizing)
				{
					st.resizing = false;
					::ReleaseCapture();
					return 0;
				}
				break;
			}

			case WM_CAPTURECHANGED:
			{
				st.resizing = false;
				break;
			}

			default:
				break;
		}
		return ::DefSubclassProc(hwnd, message, wParam, lParam);
	}

	// --- the left border line of the editors (non-client area of the editor window: HTBORDER) ---

	// The column in list mode on the left of an editor window (the column reaches the editor)
	HWND columnLeftOf(HWND editor)
	{
		RECT rcEditor{};
		::GetWindowRect(editor, &rcEditor);
		const LONG middleY = (rcEditor.top + rcEditor.bottom) / 2;
		for (const auto& [tab, st] : g_states)
		{
			if (!st.listMode || !::IsWindowVisible(tab))
				continue;

			RECT rcTab{};
			::GetWindowRect(tab, &rcTab);
			const LONG visibleTop = rcTab.top + st.scroll;
			if (rcTab.right == rcEditor.left && middleY >= visibleTop && middleY < visibleTop + st.slot.bottom)
				return tab;
		}
		return nullptr;
	}

	bool isOnLeftBorder(HWND editor, POINT screenPt)
	{
		RECT rc{};
		::GetWindowRect(editor, &rc);
		return screenPt.x < rc.left + DPIManagerV2::getSystemMetricsForDpi(SM_CXEDGE, dpiOf(editor));
	}

	LRESULT CALLBACK editorSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR idSubclass, DWORD_PTR /*refData*/)
	{
		switch (message)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hwnd, editorSubclassProc, idSubclass);
				break;
			}

			case WM_SETCURSOR:
			{
				POINT pt{};
				::GetCursorPos(&pt);
				if (LOWORD(lParam) == HTBORDER && isOnLeftBorder(hwnd, pt) && columnLeftOf(hwnd))
				{
					::SetCursor(::LoadCursor(nullptr, IDC_SIZEWE));
					return TRUE;
				}
				break;
			}

			case WM_NCLBUTTONDOWN:
			case WM_NCLBUTTONDBLCLK:
			{
				POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				if (wParam == HTBORDER && isOnLeftBorder(hwnd, pt))
				{
					if (const HWND tab = columnLeftOf(hwnd))
					{
						// the column takes the drag over (mouse capture)
						::ScreenToClient(tab, &pt);
						beginResize(tab, g_states[tab], pt.x);
						return 0;
					}
				}
				break;
			}

			default:
				break;
		}
		return ::DefSubclassProc(hwnd, message, wParam, lParam);
	}

	BOOL CALLBACK attachEditor(HWND child, LPARAM parent)
	{
		wchar_t className[16]{};
		if (::GetParent(child) == reinterpret_cast<HWND>(parent) && ::GetClassName(child, className, _countof(className)) && wcscmp(className, L"Scintilla") == 0)
			::SetWindowSubclass(child, editorSubclassProc, editorSubclassID, 0); // once: same procedure and ID
		return TRUE;
	}

	// The editors are the Scintilla windows next to the tab bars, children of the main window
	void attachEditors(HWND tabBar)
	{
		const HWND parent = ::GetParent(tabBar);
		::EnumChildWindows(parent, attachEditor, reinterpret_cast<LPARAM>(parent));
	}
}

namespace VerticalTabs
{
	bool isListMode()
	{
		const NppGUI& nppGUI = NppParameters::getInstance().getNppGUI();
		return (nppGUI._tabStatus & TAB_VERTICAL) && nppGUI._isVerticalTabListMode;
	}

	void attach(HWND hTabBar)
	{
		g_states[hTabBar] = TabState{};
		::SetWindowSubclass(hTabBar, tabBarSubclassProc, subclassID, 0);
	}

	bool reSizeTo(TabBar& tabBar, RECT& rc2Adjust)
	{
		const HWND hwnd = tabBar.getHSelf();
		auto it = g_states.find(hwnd);
		if (it == g_states.end())
			return false; // not a document tab bar

		TabState& st = it->second;
		if (!isListMode())
		{
			if (st.listMode)
				leaveListMode(hwnd, st);
			return false; // original layout
		}

		const bool isEntering = !st.listMode;
		if (isEntering)
		{
			enterListMode(hwnd, st);
			attachEditors(hwnd);
		}

		tabBar.display(rc2Adjust.right > 10);

		// the column never takes the whole width: keep some room for the editor
		const int minEditorWidth = scaled(hwnd, 100);
		const int wanted = scaled(hwnd, NppParameters::getInstance().getNppGUI()._verticalTabListWidth);
		st.width = std::max(scaled(hwnd, 40), std::min(wanted, static_cast<int>(rc2Adjust.right) - minEditorWidth));
		st.slot = rc2Adjust;
		st.gap = std::max(0, NppParameters::getInstance().getSVP()._borderWidth); // DocTabView leaves it before the editor

		// doVertical() / doMultiLine() may have changed the styles meanwhile
		setListStyle(hwnd, true);
		if (st.requestedCy > 0)
			::SendMessage(hwnd, TCM_SETITEMSIZE, 0, MAKELPARAM(st.requestedCx, st.requestedCy)); // width replaced by the subclass

		layoutColumn(hwnd, st);

		// list mode just switched on (at startup or from the preferences): show the active tab
		if (isEntering)
			ensureVisible(hwnd, st, TabCtrl_GetCurSel(hwnd));

		rc2Adjust.left += st.width;
		rc2Adjust.right -= st.width;
		return true;
	}
}
