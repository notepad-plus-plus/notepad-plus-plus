// This file is part of Notepad++ project
// Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

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
#include "Gripper.h"
#include "DockingManager.h"
#include "Parameters.h"

using namespace std;

#ifndef WH_KEYBOARD_LL
#define WH_KEYBOARD_LL 13
#endif

#ifndef WH_MOUSE_LL
#define WH_MOUSE_LL 14
#endif


BOOL Gripper::_isRegistered	= FALSE;
bool Gripper::_isOverlayClassRegistered = false;

static HWND		hWndServer		= NULL;
static HHOOK	hookMouse		= NULL;
static HHOOK	hookKeyboard	= NULL;

static LRESULT CALLBACK hookProcMouse(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
		switch (wParam)
		{
			case WM_MOUSEMOVE:
			case WM_NCMOUSEMOVE:
				::SendMessage(hWndServer, static_cast<UINT>(wParam), 0, 0);
				break;

			case WM_LBUTTONUP:
			case WM_NCLBUTTONUP:
				::SendMessage(hWndServer, static_cast<UINT>(wParam), 0, 0);
				return TRUE;

			default:
				break;
		}
	}
	return ::CallNextHookEx(hookMouse, nCode, wParam, lParam);
}

static LRESULT CALLBACK hookProcKeyboard(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
		if (wParam == VK_ESCAPE)
		{
			::PostMessage(hWndServer, DMM_CANCEL_MOVE, 0, 0);
			return FALSE;
		}
	}

	return ::CallNextHookEx(hookKeyboard, nCode, wParam, lParam);
}


void Gripper::startGrip(DockingCont* pCont, DockingManager* pDockMgr)
{
	_pDockMgr   = pDockMgr;
	_pCont		= pCont;

	_pDockMgr->getDockInfo(&_dockData);

	if (!_isRegistered)
	{
		WNDCLASS clz{};

		clz.style = 0;
		clz.lpfnWndProc = staticWinProc;
		clz.cbClsExtra = 0;
		clz.cbWndExtra = 0;
		clz.hInstance = _hInst;
		clz.hIcon = NULL;
		clz.hCursor = ::LoadCursor(NULL, IDC_ARROW);

		clz.hbrBackground = NULL;
		clz.lpszMenuName = NULL;
		clz.lpszClassName = MDLG_CLASS_NAME;

		if (!::RegisterClass(&clz))
		{
			throw std::runtime_error("Gripper::startGrip : RegisterClass() function failed");
		}
		_isRegistered = TRUE;
	}

	_hSelf = ::CreateWindowEx(
					0,
					MDLG_CLASS_NAME,
					L"", 0,
					CW_USEDEFAULT, CW_USEDEFAULT,
					CW_USEDEFAULT, CW_USEDEFAULT,
					NULL,
					NULL,
					_hInst,
					(LPVOID)this);
	hWndServer = _hSelf;

	if (!_hSelf)
	{
		throw std::runtime_error("Gripper::startGrip : CreateWindowEx() function return null");
	}
}


LRESULT CALLBACK Gripper::staticWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Gripper *pDlgMoving = NULL;
	switch (message)
	{
		case WM_NCCREATE :
			pDlgMoving = static_cast<Gripper *>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
			pDlgMoving->_hSelf = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pDlgMoving));
			return TRUE;

		default :
			pDlgMoving = reinterpret_cast<Gripper *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
			if (!pDlgMoving)
				return ::DefWindowProc(hwnd, message, wParam, lParam);
			return pDlgMoving->runProc(message, wParam, lParam);
	}
}

LRESULT Gripper::runProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			create();
			break;
		}
		case WM_MOUSEMOVE:
		case WM_NCMOUSEMOVE:
		{
			onMove();
			return TRUE;
		}
		case WM_LBUTTONUP:
		case WM_NCLBUTTONUP:
		{
			/* end hooking */
			if (hookMouse)
			{
				::UnhookWindowsHookEx(hookMouse);
				::UnhookWindowsHookEx(hookKeyboard);
				hookMouse = NULL;
				hookKeyboard = NULL;
			}
			onButtonUp();
			::DestroyWindow(_hSelf);
			return TRUE;
		}
		case DMM_CANCEL_MOVE:
		{
			POINT			pt			= {0,0};
			POINT			ptBuf		= {0,0};

			::GetCursorPos(&pt);
			getMousePoints(pt, ptBuf);

			/* erase last drawn rectangle */
			drawRectangle(NULL);

			/* end hooking */
			::UnhookWindowsHookEx(hookMouse);
			::UnhookWindowsHookEx(hookKeyboard);

			::DestroyWindow(_hSelf);
			return FALSE;
		}
		case WM_DESTROY:
		{
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			::SetWindowPos(_hParent, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			_pCont->focusClient();
			delete this; // TODO: remove this line and delete this object outside of itself
			return TRUE;
		}
		default:
			break;
	}

	return ::DefWindowProc(_hSelf, message, wParam, lParam);
}


void Gripper::create()
{
	RECT		rc		= {};
	POINT		pt		= {};

	// start hooking
	::SetWindowPos(_pCont->getHSelf(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	::SetCapture(_hSelf);
	winVer ver = (NppParameters::getInstance()).getWinVersion();
	hookMouse = ::SetWindowsHookEx(WH_MOUSE_LL, hookProcMouse, _hInst, 0);

    if (!hookMouse)
    {
        DWORD dwError = ::GetLastError();
        wchar_t  str[128];
        ::wsprintf(str, L"GetLastError() returned %lu", dwError);
        ::MessageBox(NULL, str, L"SetWindowsHookEx(MOUSE) failed on Gripper::create()", MB_OK | MB_ICONERROR);
    }

	if (ver != WV_UNKNOWN && ver < WV_VISTA)
	{
		hookKeyboard = ::SetWindowsHookEx(WH_KEYBOARD_LL, hookProcKeyboard, _hInst, 0);
		if (!hookKeyboard)
		{
			DWORD dwError = ::GetLastError();
			wchar_t  str[128];
			::wsprintf(str, L"GetLastError() returned %lu", dwError);
			::MessageBox(NULL, str, L"SetWindowsHookEx(KEYBOARD) failed on Gripper::create()", MB_OK | MB_ICONERROR);
		}
	}
//  Removed regarding W9x systems
//	mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

	// calculate the mouse pt within dialog
	::GetCursorPos(&pt);

	// get tab information
	initTabInformation();

	if (_pCont->isFloating() == true)
	{
		::GetWindowRect(_pCont->getHSelf(), &rc);
	}
	else
	{
		_pCont->getClientRect(rc);
		::ScreenToClient(_pCont->getHSelf(), &pt);
	}

	_ptOffset.x	= pt.x - rc.left;
	_ptOffset.y	= pt.y - rc.top;
}


void Gripper::onMove()
{
	POINT		pt		= {0,0};
	POINT		ptBuf	= {0,0};

	::GetCursorPos(&pt);
	getMousePoints(pt, ptBuf);

	/* tab reordering only when tab was selected */
	if (_startMovingFromTab == TRUE)
	{
		doTabReordering(pt);
	}

	drawRectangle(&pt);
}


void Gripper::onButtonUp()
{
	POINT			pt			= {0,0};
	POINT			ptBuf		= {0,0};
	RECT			rc			= {};
	RECT			rcCorr		= {};

	::GetCursorPos(&pt);
	getMousePoints(pt, ptBuf);

	// do nothing, when old point is not valid
	if (_bPtOldValid == FALSE)
		return;

	// erase last drawn rectangle
	drawRectangle(NULL);

	// look if current position is within dockable area
	DockingCont*	pDockCont = contHitTest(pt);

	if (pDockCont == NULL)
	{
		pDockCont = workHitTest(pt);
	}

	/* add dependency to other container class */
	if (pDockCont == NULL)
	{
		/* calculate new position */
		rc = _pCont->getDataOfActiveTb()->rcFloat;
		_pCont->getClientRect(rcCorr);

		CalcRectToScreen(_dockData.hWnd, &rc);
		CalcRectToScreen(_dockData.hWnd, &rcCorr);

		rc.left    = pt.x - _ptOffset.x;
		rc.top     = pt.y - _ptOffset.y;

		/* correct rectangle position when mouse is not within */
		DoCalcGripperRect(&rc, rcCorr, pt);

		DockingCont* pContMove	= NULL;
		
		/* change location of toolbars */
		if (_startMovingFromTab == TRUE)
		{
			/* when tab is moved */
			if ((!_pCont->isFloating()) ||
				((_pCont->isFloating()) && (::SendMessage(_hTabSource, TCM_GETITEMCOUNT, 0, 0) > 1)))
			{
				pContMove = _pDockMgr->toggleActiveTb(_pCont, DMM_FLOAT, TRUE, &rc);
			}
		}
		else if (!_pCont->isFloating())
		{
			/* when all windows are moved */
			pContMove = _pDockMgr->toggleVisTb(_pCont, DMM_FLOAT, &rc);
		}

		/* set moving container */
		if (pContMove == NULL)
		{
			pContMove = _pCont;
		}

		/* update window position */
		::MoveWindow(pContMove->getHSelf(), rc.left, rc.top, rc.right, rc.bottom, TRUE);
		::SendMessage(pContMove->getHSelf(), WM_SIZE, 0, 0);
	}
	else if (_pCont != pDockCont)
	{
		/* change location of toolbars */
		if ((_startMovingFromTab == TRUE) && (::SendMessage(_hTabSource, TCM_GETITEMCOUNT, 0, 0) != 1))
		{
			/* when tab is moved */
			_pDockMgr->toggleActiveTb(_pCont, pDockCont);
		}
		else
		{
			/* when all windows are moved */
			_pDockMgr->toggleVisTb(_pCont, pDockCont);
		}
	}
}


void Gripper::doTabReordering(POINT pt)
{
	vector<DockingCont*>	vCont		= _pDockMgr->getContainerInfo();
	BOOL					inTab		= FALSE;
	HWND					hTab		= NULL;
	HWND					hTabOld		= _hTab;
	int						iItemOld	= _iItem;

	/* search for every tab entry */
	for (size_t iCont = 0, len = vCont.size(); iCont < len; ++iCont)
	{
		hTab = vCont[iCont]->getTabWnd();

		/* search only if container is visible */
		if (::IsWindowVisible(hTab) == TRUE)
		{
			RECT	rc		= {};

			::GetWindowRect(hTab, &rc);

			/* test if cursor points in tab window */
			if (::PtInRect(&rc, pt) == TRUE)
			{
				TCHITTESTINFO	info	= {};

				if (_hTab == NULL)
				{
					initTabInformation();
					hTabOld  = _hTab;
					iItemOld = _iItem;
				}

				// get pointed tab item
				info.pt	= pt;
				::ScreenToClient(hTab, &info.pt);
				auto iItem = ::SendMessage(hTab, TCM_HITTEST, 0, reinterpret_cast<LPARAM>(&info));

				if (iItem != -1)
				{
					// prevent flickering of tabs with different sizes
					::SendMessage(hTab, TCM_GETITEMRECT, iItem, reinterpret_cast<LPARAM>(&rc));
					ClientRectToScreenRect(hTab, &rc);

					if ((rc.left + (_rcItem.right  - _rcItem.left)) < pt.x)
					{
						return;
					}

					_iItem = static_cast<int32_t>(iItem);
				}
				else if (_hTab && ((hTab != _hTab) || (_iItem == -1)))
				{
					// test if cusor points after last tab
					auto iLastItem = ::SendMessage(hTab, TCM_GETITEMCOUNT, 0, 0) - 1;

					::SendMessage(hTab, TCM_GETITEMRECT, iLastItem, reinterpret_cast<LPARAM>(&rc));
					if ((rc.left + rc.right) < pt.x)
					{
						_iItem = static_cast<int32_t>(iLastItem) + 1;
					}
				}

				_hTab = hTab;
				inTab = TRUE;
				break;
			}
		}
	}

	// set and remove tabs correct
	if ((inTab == TRUE) && (iItemOld != _iItem))
	{
		if (_hTab == _hTabSource)
		{
			// delete item if switching back to source tab
			auto iSel = ::SendMessage(_hTab, TCM_GETCURSEL, 0, 0);
			::SendMessage(_hTab, TCM_DELETEITEM, iSel, 0);
		}
		else if (_hTab && _hTab == hTabOld)
		{
			// delete item on switch between tabs
			::SendMessage(_hTab, TCM_DELETEITEM, iItemOld, 0);
		}
	}
	else if (inTab == FALSE)
	{
		if (hTabOld != _hTabSource)
		{
			::SendMessage(hTabOld, TCM_DELETEITEM, iItemOld, 0);
		}
		_iItem = -1;
	}

	// insert new entry when mouse doesn't point to current hovered tab
	if (_hTab && ((_hTab != hTabOld) || (_iItem != iItemOld)))
	{
		_tcItem.mask	= TCIF_PARAM | (_hTab == _hTabSource ? TCIF_TEXT : 0);
		::SendMessage(_hTab, TCM_INSERTITEM, _iItem, reinterpret_cast<LPARAM>(&_tcItem));
	}

	// select the tab only in source tab window
	if ((_hTab != nullptr && _hTab == _hTabSource) && (_iItem != -1))
	{
		::SendMessage(_hTab, TCM_SETCURSEL, _iItem, 0);
	}

#if 0
	extern HWND g_hMainWnd;
	wchar_t str[128];
	wsprintf(str, L"Size: %i", vCont.size());
	::SetWindowText(g_hMainWnd, str);
#endif

	::UpdateWindow(_hParent);
}

// ============================================================================
// Overlay window for drag rectangle rendering (tk)
//
// Uses a layered window spanning the entire virtual screen with color-key
// transparency (magenta). The drag rectangle is drawn to a memory DC and
// blitted to the overlay - only the rectangle frame is visible.
//
// Coordinates: Screen coordinates are translated to overlay-local by
// subtracting the virtual screen origin (SM_XVIRTUALSCREEN, SM_YVIRTUALSCREEN).
// ============================================================================

static constexpr COLORREF clrMagenta = RGB(255, 0, 255);

LRESULT CALLBACK Gripper::overlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

bool Gripper::createOverlayWindow()
{
	if (_hOverlayWnd)
		return true;

	if (!_isOverlayClassRegistered)
	{
		WNDCLASSEX wc = {};
		wc.cbSize = sizeof(wc);
		wc.lpfnWndProc = overlayWndProc;
		wc.hInstance = _hInst;
		wc.lpszClassName = L"NppGripperOverlay";

		if (!::RegisterClassEx(&wc))
			return false;

		_isOverlayClassRegistered = true;
	}

	_xVirtScreen = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
	_yVirtScreen = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
	_overlayWidth = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
	_overlayHeight = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);

	_hOverlayWnd = ::CreateWindowEx(
		WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
		L"NppGripperOverlay",
		L"",
		WS_POPUP,
		_xVirtScreen, _yVirtScreen, _overlayWidth, _overlayHeight,
		nullptr, nullptr, _hInst, nullptr
	);

	if (!_hOverlayWnd)
		return false;

	::SetLayeredWindowAttributes(_hOverlayWnd, clrMagenta, 0, LWA_COLORKEY);

	HDC hdcScreen = ::GetDC(nullptr);
	_hdcOverlayMem = ::CreateCompatibleDC(hdcScreen);
	_hBitmapOverlay = ::CreateCompatibleBitmap(hdcScreen, _overlayWidth, _overlayHeight);
	_hOldBitmap = static_cast<HBITMAP>(::SelectObject(_hdcOverlayMem, _hBitmapOverlay));
	::ReleaseDC(nullptr, hdcScreen);

	// Fill with transparent color
	HBRUSH hBrushTransparent = ::CreateSolidBrush(clrMagenta);
	RECT rcFill = { 0, 0, _overlayWidth, _overlayHeight };
	::FillRect(_hdcOverlayMem, &rcFill, hBrushTransparent);
	::DeleteObject(hBrushTransparent);

	_hdcOverlay = ::GetDC(_hOverlayWnd);

	::ShowWindow(_hOverlayWnd, SW_SHOWNOACTIVATE);
	::BitBlt(_hdcOverlay, 0, 0, _overlayWidth, _overlayHeight, _hdcOverlayMem, 0, 0, SRCCOPY);

	return true;
}

void Gripper::destroyOverlayWindow()
{
	if (_hdcOverlay && _hOverlayWnd)
	{
		::ReleaseDC(_hOverlayWnd, _hdcOverlay);
		_hdcOverlay = nullptr;
	}
	if (_hdcOverlayMem)
	{
		if (_hOldBitmap)
			::SelectObject(_hdcOverlayMem, _hOldBitmap);
		::DeleteDC(_hdcOverlayMem);
		_hdcOverlayMem = nullptr;
		_hOldBitmap = nullptr;
	}
	if (_hBitmapOverlay)
	{
		::DeleteObject(_hBitmapOverlay);
		_hBitmapOverlay = nullptr;
	}
	if (_hOverlayWnd)
	{
		::DestroyWindow(_hOverlayWnd);
		_hOverlayWnd = nullptr;
	}
}

// Draw drag rectangle using layered overlay window
//
// Uses overlay window approach to fix clipping issues on multi-monitor setups
// where the virtual screen has negative coordinates (Issue #16805).
//
void Gripper::drawRectangle(const POINT* pPt)
{
	RECT   rcNew = {};
	RECT   rcOld = _rcPrev;

	// Create a brush with the appropriate bitmap pattern to draw our drag rectangle
	if (!_hbm)
		_hbm = ::CreateBitmap(8, 8, 1, 1, DotPattern);
	if (!_hbrush)
		_hbrush = ::CreatePatternBrush(_hbm);

	if (pPt != NULL)
	{
		// getMovingRect() returns rect where right/bottom = width/height (not coordinates)
		getMovingRect(*pPt, &rcNew);
		_rcPrev = rcNew;

		if (_bPtOldValid)
		{
			if (rcOld.left == rcNew.left && rcOld.right == rcNew.right &&
			    rcOld.top == rcNew.top && rcOld.bottom == rcNew.bottom)
				return;
		}
	}

	// Create overlay window on first draw
	if (!_hOverlayWnd)
	{
		if (!createOverlayWindow())
			return;
	}

	// Save absolute coordinates before any modifications
	RECT rcOldAbsolute = rcOld;
	RECT rcNewAbsolute = rcNew;

	// Calculate overlay-local coordinates
	LONG newOverlayX = rcNewAbsolute.left - _xVirtScreen;
	LONG newOverlayY = rcNewAbsolute.top - _yVirtScreen;
	LONG newWidth = rcNewAbsolute.right;
	LONG newHeight = rcNewAbsolute.bottom;

	// Erase old rectangle
	if (_bPtOldValid)
	{
		HBRUSH hBrushTransparent = ::CreateSolidBrush(clrMagenta);
		LONG oldOverlayX = rcOldAbsolute.left - _xVirtScreen;
		LONG oldOverlayY = rcOldAbsolute.top - _yVirtScreen;
		LONG oldWidth = rcOldAbsolute.right;
		LONG oldHeight = rcOldAbsolute.bottom;
		RECT rcClear = { oldOverlayX, oldOverlayY, oldOverlayX + oldWidth, oldOverlayY + oldHeight };
		::FillRect(_hdcOverlayMem, &rcClear, hBrushTransparent);
		::DeleteObject(hBrushTransparent);
	}

	// Draw new rectangle
	if (pPt != NULL)
	{
		HBRUSH hBrushTransparent = ::CreateSolidBrush(clrMagenta);
		RECT rcFrame = { newOverlayX, newOverlayY, newOverlayX + newWidth, newOverlayY + newHeight };
		::FillRect(_hdcOverlayMem, &rcFrame, hBrushTransparent);
		::DeleteObject(hBrushTransparent);

		HBRUSH hBrushGray = ::CreateSolidBrush(RGB(128, 128, 128));
		RECT rcTop = { newOverlayX, newOverlayY, newOverlayX + newWidth, newOverlayY + 3 };
		::FillRect(_hdcOverlayMem, &rcTop, hBrushGray);
		RECT rcBottom = { newOverlayX, newOverlayY + newHeight - 3, newOverlayX + newWidth, newOverlayY + newHeight };
		::FillRect(_hdcOverlayMem, &rcBottom, hBrushGray);
		RECT rcLeft = { newOverlayX, newOverlayY, newOverlayX + 3, newOverlayY + newHeight };
		::FillRect(_hdcOverlayMem, &rcLeft, hBrushGray);
		RECT rcRight = { newOverlayX + newWidth - 3, newOverlayY, newOverlayX + newWidth, newOverlayY + newHeight };
		::FillRect(_hdcOverlayMem, &rcRight, hBrushGray);
		::DeleteObject(hBrushGray);

		HBRUSH hOldBrush = static_cast<HBRUSH>(::SelectObject(_hdcOverlayMem, _hbrush));
		::SetBrushOrgEx(_hdcOverlayMem, rcNewAbsolute.left % 8, rcNewAbsolute.top % 8, nullptr);
		::PatBlt(_hdcOverlayMem, newOverlayX, newOverlayY, newWidth, 3, PATINVERT);
		::PatBlt(_hdcOverlayMem, newOverlayX, newOverlayY + newHeight - 3, newWidth, 3, PATINVERT);
		::PatBlt(_hdcOverlayMem, newOverlayX, newOverlayY, 3, newHeight, PATINVERT);
		::PatBlt(_hdcOverlayMem, newOverlayX + newWidth - 3, newOverlayY, 3, newHeight, PATINVERT);
		::SelectObject(_hdcOverlayMem, hOldBrush);
	}

	// Copy to overlay window (only changed region for performance)
	LONG copyMinX = newOverlayX;
	LONG copyMinY = newOverlayY;
	LONG copyMaxX = newOverlayX + newWidth;
	LONG copyMaxY = newOverlayY + newHeight;

	if (_bPtOldValid)
	{
		LONG oldOverlayX = rcOldAbsolute.left - _xVirtScreen;
		LONG oldOverlayY = rcOldAbsolute.top - _yVirtScreen;
		LONG oldWidth = rcOldAbsolute.right;
		LONG oldHeight = rcOldAbsolute.bottom;

		if (oldOverlayX < copyMinX) copyMinX = oldOverlayX;
		if (oldOverlayY < copyMinY) copyMinY = oldOverlayY;
		if (oldOverlayX + oldWidth > copyMaxX) copyMaxX = oldOverlayX + oldWidth;
		if (oldOverlayY + oldHeight > copyMaxY) copyMaxY = oldOverlayY + oldHeight;
	}

	::BitBlt(_hdcOverlay, copyMinX, copyMinY, copyMaxX - copyMinX, copyMaxY - copyMinY,
		_hdcOverlayMem, copyMinX, copyMinY, SRCCOPY);

	if (pPt == NULL)
	{
		destroyOverlayWindow();
		_bPtOldValid = FALSE;
	}
	else
	{
		_bPtOldValid = TRUE;
	}
}

void Gripper::getMousePoints(const POINT& pt, POINT& ptPrev)
{
	ptPrev	= _ptOld;
	_ptOld	= pt;
}


void Gripper::getMovingRect(POINT pt, RECT *rc)
{
	RECT			rcCorr			= {};
	DockingCont*	pContHit		= NULL;

	/* test if mouse hits a container */
	pContHit = contHitTest(pt);

	if (pContHit != NULL)
	{
		/* get rect of client */
		::GetWindowRect(pContHit->getHSelf(), rc);

		/* get rect for correction */
		if (_pCont->isFloating() == TRUE)
			rcCorr = _pCont->getDataOfActiveTb()->rcFloat;
		else
			_pCont->getClientRect(rcCorr);

		ShrinkRcToSize(rc);
		ShrinkRcToSize(&rcCorr);

		/* correct rectangle position when mouse is not within */
		DoCalcGripperRect(rc, rcCorr, pt);
	}
	else
	{
		/* test if mouse is within work area */
		pContHit = workHitTest(pt, rc);

		/* calcutlates the rect and its position */
		if (pContHit == NULL)
		{
			/* calcutlates the rect and draws it */
			if (!_pCont->isFloating())
				*rc = _pCont->getDataOfActiveTb()->rcFloat;
			else
				_pCont->getWindowRect(*rc);
			_pCont->getClientRect(rcCorr);

			CalcRectToScreen(_dockData.hWnd, rc);
			CalcRectToScreen(_dockData.hWnd, &rcCorr);

			rc->left    = pt.x - _ptOffset.x;
			rc->top     = pt.y - _ptOffset.y;

			/* correct rectangle position when mouse is not within */
			DoCalcGripperRect(rc, rcCorr, pt);
		}
	}
}


DockingCont* Gripper::contHitTest(POINT pt)
{
	vector<DockingCont*>	vCont	= _pDockMgr->getContainerInfo();
	HWND					hWnd	= ::WindowFromPoint(pt);

	for (size_t iCont = 0, len = vCont.size(); iCont < len; ++iCont)
	{
		/* test if within caption */
		if (hWnd == vCont[iCont]->getCaptionWnd())
		{
			if (vCont[iCont]->isFloating())
			{
				RECT	rc	= {};

				vCont[iCont]->getWindowRect(rc);
				if ((rc.top < pt.y) && (pt.y < (rc.top + 24)))
				{
					/* when it is the same container start moving immediately */
					if (vCont[iCont] == _pCont)
					{
						return NULL;
					}
					else
					{
						return vCont[iCont];
					}
				}
			}
			else
			{
				return vCont[iCont];
			}
		}

		/* test only tabs that are visible */
		if (::IsWindowVisible(vCont[iCont]->getTabWnd()))
		{
			/* test if within tab (rect test is used, because of drag and drop behaviour) */
			RECT		rc	= {};

			::GetWindowRect(vCont[iCont]->getTabWnd(), &rc);
			if (::PtInRect(&rc, pt))
			{
				return vCont[iCont];
			}
		}
	}

	/* doesn't hit a container */
	return NULL;
}


DockingCont* Gripper::workHitTest(POINT pt, RECT *rc)
{
	RECT					rcCont	= {};
	vector<DockingCont*>	vCont	= _pDockMgr->getContainerInfo();

	/* at first test if cursor points into a visible container */
	for (size_t iCont = 0, len = vCont.size(); iCont < len; ++iCont)
	{
		if (vCont[iCont]->isVisible())
		{
			vCont[iCont]->getWindowRect(rcCont);

			if (::PtInRect(&rcCont, pt) == TRUE)
			{
				/* when it does, return with non found docking area */
				return NULL;
			}
		}
	}

	/* now search if cusor hits a possible docking area */
	for (int iWork = 0; iWork < DOCKCONT_MAX; ++iWork)
	{
		if (!vCont[iWork]->isVisible())
		{
			rcCont = _dockData.rcRegion[iWork];
			rcCont.right  += rcCont.left;
			rcCont.bottom += rcCont.top;

			if (rc != NULL)
			{
				*rc = rcCont;
			}

			/* set fix hit test with */
			switch(iWork)
			{
				case CONT_LEFT:
					rcCont.right   = rcCont.left + HIT_TEST_THICKNESS;
					rcCont.left   -= HIT_TEST_THICKNESS;
					break;
				case CONT_RIGHT:
					rcCont.left    = rcCont.right - HIT_TEST_THICKNESS;
					rcCont.right  += HIT_TEST_THICKNESS;
					break;
				case CONT_TOP:
					rcCont.bottom  = rcCont.top + HIT_TEST_THICKNESS;
					rcCont.top    -= HIT_TEST_THICKNESS;
					break;
				case CONT_BOTTOM:
					rcCont.top     = rcCont.bottom - HIT_TEST_THICKNESS;
					rcCont.bottom += HIT_TEST_THICKNESS;
					break;
				default:
					break;
			}
			::MapWindowPoints(_dockData.hWnd, NULL, (LPPOINT)(&rcCont), 2);

			if (::PtInRect(&rcCont, pt) == TRUE)
			{
				if (rc != NULL)
				{
					::MapWindowPoints(_dockData.hWnd, NULL, (LPPOINT)(rc), 2);
					rc->right  -= rc->left;
					rc->bottom -= rc->top;
				}
				return vCont[iWork];
			}
		}
	}

	/* no docking area found */
	return NULL;
}


void Gripper::initTabInformation()
{
	/* for tab reordering */

	/* remember handle */
	_hTabSource = _pCont->getTabWnd();
	_startMovingFromTab	= _pCont->startMovingFromTab();
	if ((_startMovingFromTab == FALSE) && (::SendMessage(_hTabSource, TCM_GETITEMCOUNT, 0, 0) == 1))
	{
		_startMovingFromTab = TRUE;
		_iItem				= 0;
	}
	else
	{
		// get active tab item
		_iItem = static_cast<int32_t>(::SendMessage(_hTabSource, TCM_GETCURSEL, 0, 0));
	}

	/* get size of item */
	_hTab = _hTabSource;
	::SendMessage(_hTabSource, TCM_GETITEMRECT, _iItem, reinterpret_cast<LPARAM>(&_rcItem));

	/* store item data */
	static wchar_t	szText[64];
	_tcItem.mask		= TCIF_PARAM | TCIF_TEXT;
	_tcItem.pszText		= szText;
	_tcItem.cchTextMax	= 64;
	::SendMessage(_hTabSource, TCM_GETITEM, _iItem, reinterpret_cast<LPARAM>(&_tcItem));
}
