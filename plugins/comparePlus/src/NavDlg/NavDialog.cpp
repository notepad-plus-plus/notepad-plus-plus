/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2009
 * Copyright (C)2017-2025 Pavel Nedev (pg.nedev@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma comment (lib, "msimg32")
#pragma comment (lib, "comctl32")


#define NOMINMAX	1

#include "Compare.h"
#include "NavDialog.h"
#include "NppHelpers.h"
#include "resource.h"

#include <windowsx.h>
#include <commctrl.h>
#include <algorithm>


const int NavDialog::cSpace = 0;
const int NavDialog::cScrollerWidth = 20;


void NavDialog::NavView::init(HDC hDC)
{
	// Create bitmaps used to store graphical representation
	m_hViewDC	= ::CreateCompatibleDC(hDC);
	m_hSelDC	= ::CreateCompatibleDC(hDC);

	m_lines	= getLinesCount(m_view);

	m_hViewBMP	= ::CreateCompatibleBitmap(hDC, 1, static_cast<int>(m_lines));
	m_hSelBMP	= ::CreateCompatibleBitmap(hDC, 1, 1);

	BITMAP bmpInfo;
	::GetObjectW(m_hViewBMP, sizeof(bmpInfo), &bmpInfo);
	::GetObjectW(m_hSelBMP, sizeof(bmpInfo), &bmpInfo);

	// Attach bitmaps to the DC
	::SelectObject(m_hViewDC, m_hViewBMP);
	::SelectObject(m_hSelDC, m_hSelBMP);
}


void NavDialog::NavView::reset()
{
	m_lineMap.clear();

	if (m_hViewDC)
	{
		::DeleteDC(m_hViewDC);
		m_hViewDC = NULL;
	}

	if (m_hSelDC)
	{
		::DeleteDC(m_hSelDC);
		m_hSelDC = NULL;
	}

	if (m_hViewBMP)
	{
		::DeleteObject(m_hViewBMP);
		m_hViewBMP = NULL;
	}

	if (m_hSelBMP)
	{
		::DeleteObject(m_hSelBMP);
		m_hSelBMP = NULL;
	}
}


void NavDialog::NavView::paint(HDC hDC, int xPos, int yPos, int width, int height, int heightTotal,
	int hScale, int hOffset, bool shrinkLeftSideOfEmptyArea, int backColor)
{
	const int usefulHeight = (maxBmpLines() - hOffset) * hScale;

	int h = (height - usefulHeight) > 0 ? usefulHeight : height;
	if (h <= 0)
		return;

	RECT r;
	r.left		= xPos;
	r.top		= yPos;
	r.right		= r.left + width + 2;
	r.bottom	= r.top + h + 2;

	// Draw view border
	::Rectangle(hDC, r.left, r.top, r.right, r.bottom);

	if ((heightTotal - h) > 0)
	{
		HBRUSH bkBrush = ::CreateSolidBrush(backColor);

		RECT bkRect;

		bkRect.left		= r.left;
		bkRect.right	= r.right;
		bkRect.top		= r.bottom;
		bkRect.bottom	= r.top + heightTotal + 2;

		// Make sure the border between panes is not wiped
		if (shrinkLeftSideOfEmptyArea)
			++bkRect.left;

		::FillRect(hDC, &bkRect, bkBrush);

		::DeleteObject(bkBrush);
	}

	// Fill view
	::StretchBlt(hDC, r.left + 1, r.top + 1, width, h, m_hViewDC, 0, hOffset, 1, h / hScale, SRCCOPY);

	intptr_t firstVisible	= getDocLineFromVisible(m_view, m_firstVisible);
	intptr_t lastVisible	= m_firstVisible + CallScintilla(m_view, SCI_LINESONSCREEN, 0, 0);

	lastVisible = getDocLineFromVisible(m_view, lastVisible);

	if (firstVisible == lastVisible)
		++lastVisible;

	firstVisible	= docToBmpLine(firstVisible);
	lastVisible		= docToBmpLine(lastVisible);

	h /= hScale;

	// Selector is out of scope so don't draw it
	if (firstVisible > hOffset + h || lastVisible < hOffset)
		return;

	if (firstVisible < hOffset)
		firstVisible = hOffset;

	if (lastVisible > hOffset + h)
		lastVisible = hOffset + h;

	firstVisible	*= hScale;
	lastVisible		*= hScale;

	r.top		= static_cast<int>(firstVisible + yPos - hOffset);
	r.bottom	= static_cast<int>(lastVisible + yPos - hOffset + 2);

	::Rectangle(hDC, r.left, r.top, r.right, r.bottom);

	BLENDFUNCTION blend = { 0 };
	blend.BlendOp = AC_SRC_OVER;
	blend.SourceConstantAlpha = 80;

	::AlphaBlend(hDC, r.left + 1, r.top + 1, width, static_cast<int>(lastVisible - firstVisible),
			m_hSelDC, 0, 0, 1, 1, blend);
}


int NavDialog::NavView::docToBmpLine(intptr_t docLine) const
{
	if (m_lineMap.empty())
		return static_cast<int>(docLine);

	const int max = static_cast<int>(m_lineMap.size());

	for (int i = 0; i < max; ++i)
	{
		if (docLine <= m_lineMap[i])
			return i + 1;
	}

	return max;
}


NavDialog::NavDialog() : DockingDlgInterface(IDD_NAV_DIALOG),
	m_isDarkMode(false), m_hScroll(NULL), m_hBackBrush(NULL), m_mouseOver(false)
{
	_data.hIconTab = NULL;
}


NavDialog::~NavDialog()
{
	Hide();

	if (_data.hIconTab)
		::DestroyIcon(_data.hIconTab);
}


void NavDialog::init(HINSTANCE hInst)
{
	m_hInst = hInst;

	DockingDlgInterface::init(hInst, nppData._nppHandle);

	m_view[0].m_view = MAIN_VIEW;
	m_view[1].m_view = SUB_VIEW;

	if (isRTLwindow(nppData._nppHandle))
		std::swap(m_view[0].m_view, m_view[1].m_view);
}


void NavDialog::doDialog()
{
	if (!isCreated())
	{
		INITCOMMONCONTROLSEX icex;

		::SecureZeroMemory(&icex, sizeof(icex));
		icex.dwSize = sizeof(icex);
		icex.dwICC  = ICC_STANDARD_CLASSES;

		::InitCommonControlsEx(&icex);

		create(&_data);

		// define the default docking behaviour
		_data.uMask			= DWS_DF_CONT_RIGHT | DWS_ICONTAB;
		_data.pszName       = L"ComparePlus NavBar";
		_data.pszModuleName	= getPluginFileName();
		_data.dlgID			= CMD_NAV_BAR;
		_data.hIconTab		= (HICON)::LoadImageW(::GetModuleHandleW(L"ComparePlus.dll"),
									MAKEINTRESOURCEW(IDB_DOCKING_ICON), IMAGE_ICON, 14, 14,
									LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);

		::SendMessageW(_hParent, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&_data);
	}
}


void NavDialog::Update()
{
	if (!isVisible())
		return;

	// Bitmap needs to be recreated
	if ((m_view[0].m_lines != getLinesCount(m_view[0].m_view)) ||
		(m_view[1].m_lines != getLinesCount(m_view[1].m_view)))
	{
		Show();
	}
	else
	{
		m_view[0].updateFirstVisible();
		m_view[1].updateFirstVisible();

		updateScroll();

		::InvalidateRect(_hSelf, NULL, FALSE);
	}
}


void NavDialog::Show()
{
	HWND hwnd = ::GetFocus();

	if (hwnd == _hSelf)
		hwnd = getCurrentView();

	doDialog();

	// Free resources if needed
	m_view[0].reset();
	m_view[1].reset();

	HDC hDC = ::GetDC(_hSelf);

	m_view[0].init(hDC);
	m_view[1].init(hDC);

	// Release DC
	::ReleaseDC(_hSelf, hDC);

	createBitmaps();

	display(true);

	::SetFocus(hwnd);
}


void NavDialog::Hide()
{
	HWND hwnd = ::GetFocus();

	if (hwnd == _hSelf)
		hwnd = getCurrentView();

	display(false);

	m_view[0].reset();
	m_view[1].reset();

	if (m_hBackBrush != NULL)
	{
		::DeleteObject(m_hBackBrush);
		m_hBackBrush = NULL;
	}

	::SetFocus(hwnd);
}


void NavDialog::createBitmaps()
{
	RECT r;
	::GetClientRect(_hSelf, &r);

	const intptr_t maxLines	= std::max(m_view[0].m_lines, m_view[1].m_lines);
	const int maxHeight		= (r.bottom - r.top) - 2 * cSpace - 2;

	intptr_t reductionRatio = maxLines / maxHeight;

	if (reductionRatio && (maxLines % maxHeight))
		++reductionRatio;

	RECT bmpRect = { 0 };
	bmpRect.right = 1;

	if (m_hBackBrush != NULL)
	{
		::DeleteObject(m_hBackBrush);
		m_hBackBrush = NULL;
	}

	m_hBackBrush				= ::CreateSolidBrush(m_clr._default);
	HBRUSH hInverseBackBrush	= ::CreateSolidBrush(m_clr._default ^ 0xFFFFFF);

	for (int viewId = 0; viewId < 2; ++viewId)
	{
		bmpRect.bottom = 1;

		::FillRect(m_view[viewId].m_hSelDC, &bmpRect, hInverseBackBrush);

		bmpRect.bottom = static_cast<int>(m_view[viewId].m_lines);
		::FillRect(m_view[viewId].m_hViewDC, &bmpRect, m_hBackBrush);

		m_view[viewId].m_lineMap.clear();

		intptr_t skipLine	= reductionRatio;
		int prevMarker		= m_clr._default;
		int bmpLine			= 0;

		for (intptr_t i = 0; i < m_view[viewId].m_lines; ++i)
		{
			int marker = static_cast<int>(CallScintilla(m_view[viewId].m_view, SCI_MARKERGET, i, 0));
			if (!marker && !reductionRatio)
				continue;

			if (marker & MARKER_MASK_ADDED)			marker = m_clr.added;
			else if (marker & MARKER_MASK_REMOVED)	marker = m_clr.removed;
			else if (marker & MARKER_MASK_MOVED)	marker = m_clr.moved;
			else if (marker & MARKER_MASK_CHANGED)	marker = m_clr.changed;
			else if (reductionRatio)				marker = m_clr._default;
			else
				continue;

			if (reductionRatio)
			{
				if (prevMarker == marker)
					--skipLine;

				if (prevMarker != marker || !skipLine)
				{
					skipLine = reductionRatio;
					prevMarker = marker;

					m_view[viewId].m_lineMap.push_back(i);

					::SetPixelV(m_view[viewId].m_hViewDC, 0, bmpLine++, marker);
				}
			}
			else
			{
				::SetPixelV(m_view[viewId].m_hViewDC, 0, static_cast<int>(i), marker);
			}
		}
	}

	::DeleteObject(hInverseBackBrush);

	setScalingFactor();
}


void NavDialog::showScroller(RECT& r)
{
	const int x = r.right - cSpace - cScrollerWidth;
	const int y = cSpace;
	const int w = cScrollerWidth;
	const int h = m_navHeight;

	if (m_hScroll)
		::MoveWindow(m_hScroll, x, y, w, h, TRUE);
	else
		m_hScroll = ::CreateWindowExW(WS_EX_NOPARENTNOTIFY, WC_SCROLLBAR, NULL, WS_CHILD | SBS_VERT,
				x, y, w, h, _hSelf, NULL, m_hInst, NULL);

	if (m_isDarkMode)
		setExplorerDarkTheme(m_hScroll);
	else
		setExplorerLightTheme(m_hScroll);

	SCROLLINFO si = { 0 };
	si.cbSize	= sizeof(si);
	si.fMask	= SIF_RANGE | SIF_PAGE;
	si.nMin		= 0;
	si.nMax		= m_maxBmpLines * m_pixelsPerLine - 1;
	si.nPage	= h;

	::SetScrollInfo(m_hScroll, SB_CTL, &si, TRUE);

	m_navViewWidth1 = ((r.right - r.left) - 3 * cSpace - cScrollerWidth - 3) / 2;
	m_navViewWidth2 = (r.right - r.left) - 3 * cSpace - cScrollerWidth - 3 - m_navViewWidth1;

	::ShowScrollBar(m_hScroll, SB_CTL, TRUE);
}


void NavDialog::setScalingFactor()
{
	RECT r;
	::GetClientRect(_hSelf, &r);

	// Happens when minimizing N++ window because WM_SIZE notification is received but window is minimized?!?!
	if (r.bottom - r.top == 0)
		return;

	m_view[0].m_lines = getLinesCount(m_view[0].m_view);
	m_view[1].m_lines = getLinesCount(m_view[1].m_view);

	m_view[0].updateFirstVisible();
	m_view[1].updateFirstVisible();

	m_maxBmpLines = std::max(m_view[0].maxBmpLines(), m_view[1].maxBmpLines());
	m_syncView = (m_maxBmpLines == m_view[0].maxBmpLines()) ? &m_view[0] : &m_view[1];

	m_navViewWidth1		= ((r.right - r.left) - 2 * cSpace - 3) / 2;
	m_navViewWidth2		= (r.right - r.left) - 2 * cSpace - 3 - m_navViewWidth1;
	m_navHeightTotal	= (r.bottom - r.top) - 2 * cSpace - 2;
	m_navHeight			= m_navHeightTotal;

	m_pixelsPerLine = m_navHeight / m_maxBmpLines;

	if (m_pixelsPerLine == 0)
	{
		m_pixelsPerLine = 1;

		showScroller(r);
	}
	else
	{
		if (m_pixelsPerLine > 5)
			m_pixelsPerLine = 5;

		m_navHeight = m_pixelsPerLine * m_maxBmpLines;

		if (m_hScroll)
			::ShowScrollBar(m_hScroll, SB_CTL, FALSE);
	}

	updateScroll();
	updateDockingDlg();

	if (isVisible())
		::InvalidateRect(_hSelf, NULL, TRUE);
}


void NavDialog::setPos(int x, int y)
{
	y -= (cSpace + 1);
	if (y < 0 || x < (cSpace + 1) || x > (m_navViewWidth1 + m_navViewWidth2 + cSpace + 2))
		return;

	NavView* currentView;

	const int scrollOffset = (m_hScroll && ::IsWindowVisible(m_hScroll)) ? ::GetScrollPos(m_hScroll, SB_CTL) : 0;

	if (x < m_navViewWidth1 + cSpace + 1)
	{
		if (y > std::min((m_view[0].maxBmpLines() - scrollOffset) * m_pixelsPerLine, m_navHeight))
			return;
		currentView = &m_view[0];
	}
	else
	{
		if (y > std::min((m_view[1].maxBmpLines() - scrollOffset) * m_pixelsPerLine, m_navHeight))
			return;
		currentView = &m_view[1];
	}

	const intptr_t currentLine = currentView->bmpToDocLine((y + scrollOffset) / m_pixelsPerLine);

	if (!isLineVisible(currentView->m_view, currentLine))
		centerAt(currentView->m_view, currentLine);

	if (Settings.FollowingCaret)
	{
		::SetFocus(getView(currentView->m_view));

		if (Settings.HideMatches || Settings.HideNewLines || Settings.HideChangedLines || Settings.HideMovedLines ||
				Settings.ShowOnlySelections)
			gotoClosestUnhiddenLine(currentView->m_view, currentLine);
		else
			CallScintilla(currentView->m_view, SCI_SETEMPTYSELECTION,
					getLineStart(currentView->m_view, currentLine), 0);

		::UpdateWindow(getView(currentView->m_view));
	}
	else
	{
		::SetFocus(getCurrentView());
	}
}


void NavDialog::onMouseWheel(int rolls)
{
	const intptr_t linesOnScreen	= CallScintilla(m_syncView->m_view, SCI_LINESONSCREEN, 0, 0);
	const intptr_t lastVisible		= getVisibleFromDocLine(m_syncView->m_view, m_syncView->m_lines);

	intptr_t firstVisible = m_syncView->m_firstVisible - rolls * linesOnScreen;

	if (firstVisible < 0)
		firstVisible = 0;
	else if (firstVisible > lastVisible - linesOnScreen)
		firstVisible = lastVisible - linesOnScreen;

	CallScintilla(m_syncView->m_view, SCI_SETFIRSTVISIBLELINE, firstVisible, 0);
}


int NavDialog::updateScroll()
{
	if (m_hScroll && ::IsWindowVisible(m_hScroll))
	{
		const intptr_t firstVisible1 = getDocLineFromVisible(m_view[0].m_view, m_view[0].m_firstVisible);
		const intptr_t firstVisible2 = getDocLineFromVisible(m_view[1].m_view, m_view[1].m_firstVisible);

		intptr_t lastVisible1 = m_view[0].m_firstVisible + CallScintilla(m_view[0].m_view, SCI_LINESONSCREEN, 0, 0);
		intptr_t lastVisible2 = m_view[1].m_firstVisible + CallScintilla(m_view[1].m_view, SCI_LINESONSCREEN, 0, 0);

		lastVisible1 = getDocLineFromVisible(m_view[0].m_view, lastVisible1);
		lastVisible2 = getDocLineFromVisible(m_view[1].m_view, lastVisible2);

		const intptr_t firstVisible	= std::min(firstVisible1, firstVisible2);
		const intptr_t lastVisible	= std::max(lastVisible1, lastVisible2);

		const NavView* const firstVisibleSyncView	= (firstVisible == firstVisible1) ? &m_view[0] : &m_view[1];
		const NavView* const lastVisibleSyncView	= (lastVisible == lastVisible1) ? &m_view[0] : &m_view[1];

		int currentScroll = ::GetScrollPos(m_hScroll, SB_CTL);

		if (firstVisibleSyncView->bmpToDocLine(currentScroll) > firstVisible)
		{
			currentScroll = firstVisibleSyncView->docToBmpLine(firstVisible);

			::SetScrollPos(m_hScroll, SB_CTL, currentScroll, TRUE);
		}
		else if (lastVisibleSyncView->bmpToDocLine(currentScroll + m_navHeight - 1) < lastVisible)
		{
			currentScroll = lastVisibleSyncView->docToBmpLine(lastVisible) - m_navHeight;

			::SetScrollPos(m_hScroll, SB_CTL, currentScroll, TRUE);
		}

		return currentScroll;
	}

	return 0;
}


void NavDialog::onPaint()
{
	// If side bar is too small, don't draw anything
	if ((m_navViewWidth1 < 5) || (m_navHeight < 5))
		return;

	const int scrollOffset = (m_hScroll && ::IsWindowVisible(m_hScroll)) ? ::GetScrollPos(m_hScroll, SB_CTL) : 0;

	HPEN hPenView = ::CreatePen(PS_SOLID, 1, RGB(128, 128, 128));

	PAINTSTRUCT ps;

	// Get current DC
	HDC hDC = ::BeginPaint(_hSelf, &ps);

	::SelectObject(hDC, hPenView);
	::SelectObject(hDC, ::GetStockObject(NULL_BRUSH));

	const int maxBmpLines0 = m_view[0].maxBmpLines();
	const int maxBmpLines1 = m_view[1].maxBmpLines();

	if (maxBmpLines0 > scrollOffset)
		m_view[0].paint(hDC, cSpace, cSpace, m_navViewWidth1, m_navHeight, m_navHeightTotal,
				m_pixelsPerLine, scrollOffset, false, m_clr._default);

	if (maxBmpLines1 > scrollOffset)
		m_view[1].paint(hDC, m_navViewWidth1 + cSpace + 1, cSpace, m_navViewWidth2, m_navHeight, m_navHeightTotal,
				m_pixelsPerLine, scrollOffset, maxBmpLines0 > maxBmpLines1, m_clr._default);

	::DeleteObject(hPenView);

	::EndPaint(_hSelf, &ps);
}


void NavDialog::adjustScroll(int offset)
{
	int currentScroll = ::GetScrollPos(m_hScroll, SB_CTL) + offset;

	if (currentScroll < 0)
		currentScroll = 0;
	else if (currentScroll > m_maxBmpLines * m_pixelsPerLine - 1)
		currentScroll = m_maxBmpLines * m_pixelsPerLine - m_navHeight;

	::SetScrollPos(m_hScroll, SB_CTL, currentScroll, TRUE);
	::InvalidateRect(_hSelf, NULL, FALSE);
}


INT_PTR CALLBACK NavDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		return TRUE;

		case WM_PAINT:
			onPaint();
		return TRUE;

		case WM_SETFOCUS:
			::SetFocus(getCurrentView());
		return TRUE;

		case WM_LBUTTONDOWN:
			::SetCapture(_hSelf);
			setPos(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return TRUE;

		case WM_LBUTTONUP:
			::ReleaseCapture();
		return TRUE;

		case WM_MOUSEMOVE:
			if (::GetCapture() == _hSelf)
				setPos(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			else if (!m_mouseOver)
				m_mouseOver = true;
		return TRUE;

		case WM_MOUSELEAVE:
			if (m_mouseOver)
				m_mouseOver = false;
		return TRUE;

		case WM_MOUSEWHEEL:
			if (m_mouseOver)
				onMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam) / 120);
		return TRUE;

		case WM_VSCROLL:
		{
			switch (LOWORD(wParam))
			{
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
				{
					const int currentScroll = HIWORD(wParam);

					::SetScrollPos(m_hScroll, SB_CTL, currentScroll, TRUE);
					::InvalidateRect(_hSelf, NULL, FALSE);
				}
				return TRUE;

				case SB_PAGEDOWN:
					adjustScroll(m_navHeight);
				return TRUE;

				case SB_PAGEUP:
					adjustScroll(-m_navHeight);
				return TRUE;

				case SB_LINEDOWN:
					adjustScroll(1);
				return TRUE;

				case SB_LINEUP:
					adjustScroll(-1);
				return TRUE;
			}
		}
		break;

		case WM_SIZE:
		case WM_MOVE:
			if (isVisible())
				setScalingFactor();
		return TRUE;

		case WM_NOTIFY:
		{
			DockingDlgInterface::run_dlgProc(Message, wParam, lParam);

			LPNMHDR	pnmh = (LPNMHDR)lParam;

			if (pnmh->hwndFrom == _hParent && LOWORD(pnmh->code) == DMN_CLOSE)
			{
				ToggleNavigationBar();
				::SetFocus(getCurrentView());
				return TRUE;
			}
			else if ((pnmh->hwndFrom == _hParent && LOWORD(pnmh->code) == DMN_FLOAT) ||
					(pnmh->hwndFrom == _hParent && LOWORD(pnmh->code) == DMN_DOCK))
			{
				setScalingFactor();
				::SetFocus(getCurrentView());
				return TRUE;
			}
			else if (LOWORD(pnmh->code) == DMN_SWITCHIN)
			{
				Update();
				::SetFocus(getCurrentView());
				return TRUE;
			}
		}
		break;

		default:
			return DockingDlgInterface::run_dlgProc(Message, wParam, lParam);
	}

	return FALSE;
}
