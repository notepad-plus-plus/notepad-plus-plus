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
		WNDCLASS clz;

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
					TEXT(""), 0,
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
			pDlgMoving = reinterpret_cast<Gripper *>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
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
			getMousePoints(&pt, &ptBuf);

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
        TCHAR  str[128];
        ::wsprintf(str, TEXT("GetLastError() returned %lu"), dwError);
        ::MessageBox(NULL, str, TEXT("SetWindowsHookEx(MOUSE) failed on Gripper::create()"), MB_OK | MB_ICONERROR);
    }

	if (ver != WV_UNKNOWN && ver < WV_VISTA)
	{
		hookKeyboard = ::SetWindowsHookEx(WH_KEYBOARD_LL, hookProcKeyboard, _hInst, 0);
		if (!hookKeyboard)
		{
			DWORD dwError = ::GetLastError();
			TCHAR  str[128];
			::wsprintf(str, TEXT("GetLastError() returned %lu"), dwError);
			::MessageBox(NULL, str, TEXT("SetWindowsHookEx(KEYBOARD) failed on Gripper::create()"), MB_OK | MB_ICONERROR);
		}
	}
//  Removed regarding W9x systems
//	mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

	// calculate the mouse pt within dialog
	::GetCursorPos(&pt);

	// get tab informations
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
	getMousePoints(&pt, &ptBuf);

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
	getMousePoints(&pt, &ptBuf);

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
		else if (_hTab == hTabOld)
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
	TCHAR str[128];
	wsprintf(str, TEXT("Size: %i"), vCont.size());
	::SetWindowText(g_hMainWnd, str);
#endif

	::UpdateWindow(_hParent);
}

// Changed behaviour (jg): Now this function handles erasing of drag-rectangles and drawing of
// new ones within one drawing step to the desktop. This is against flickering, but also it is
// necessary for the Vista Aero style - because in this case the control is given so much to
// the graphics driver, that accesses (especially read accesses) to the desktop window become
// too expensive to access it more than absolutely necessary. Besides, usage of the function
// ::LockWindowUpdate() was added, because with often redrawn windows in the background we had
// inconsistencies while erasing our drag-rectangle (because it could already have been erased
// on some places).
//
// Parameter pPt==NULL says that only erasing is wanted and the drag-rectangle is no more needed,
// thatswhy this also leads to a call of ::LockWindowUpdate(NULL) to enable drawing by others again.
// The previously drawn rectangle is memoried within _rectPrev (and _bPtOldValid says if it already
// is valid - did not change this members name because didn't want change too much at once).
//
// I was too lazy to always draw four rectangles for the four edges of the drag-rectangle - it seems
// that drawing an outer rectangle first and then erasing the inner stuff by drawing a second,
// smaller rectangle inside seems to be not slower - wich comes not unawaited, because it is mostly
// hardware-driven and each single draw has its own fixed costs.
//
// For further solutions I think we should leave this classic way of dragging and better use
// alpha-blending and always move the whole content of the toolbars - so we could leave the
// ::LockWindowUpdate() behind us.
//
// Besides, while debugging into the dragging process please let the ::LockWindowUpdate() out,
// by #undef the USE_LOCKWINDOWUPDATE in gripper.h, because it works for your debugging window
// as well, of course. Or just try by this #define what difference it makes.
//
void Gripper::drawRectangle(const POINT* pPt)
{
	HBRUSH hbrushOrig= NULL;
	HBITMAP hbmOrig  = NULL;
	RECT   rc	 = {};
	RECT   rcNew	 = {};
	RECT   rcOld	 = _rcPrev;

	// Get a screen device context with backstage redrawing disabled - to have a consistently
	// and stable drawn rectangle while floating - keep in mind, that we must ensure, that
	// finally ::LockWindowUpdate(NULL) will be called, to enable drawing for others again.
	if (!_hdc)
	{
		HWND hWnd = ::GetDesktopWindow();
		#if defined (USE_LOCKWINDOWUPDATE)
		_hdc = ::GetDCEx(hWnd, NULL, ::LockWindowUpdate(hWnd) ? DCX_WINDOW|DCX_CACHE|DCX_LOCKWINDOWUPDATE : DCX_WINDOW|DCX_CACHE);
		#else
		_hdc = ::GetDCEx(hWnd, NULL, DCX_WINDOW|DCX_CACHE);
		#endif
	}

	// Create a brush with the appropriate bitmap pattern to draw our drag rectangle
	if (!_hbm)
		_hbm = ::CreateBitmap(8, 8, 1, 1, DotPattern);
	if (!_hbrush)
		_hbrush = ::CreatePatternBrush(_hbm);

	if (pPt != NULL)
	{
		// Determine whether to draw a solid drag rectangle or checkered
		// ???(jg) solid or checked ??? - must have been an old comment, I didn't
		// find here this difference, but at least it's a question of drag-rects size
		//
		getMovingRect(*pPt, &rcNew);
		_rcPrev= rcNew;		// save the new drawn rcNew

		// note that from here for handling purposes the right and bottom values of the rects
		// contain width and height - its handsome, but i find it dangerous, but didn't want to
		// change that already this time.

		if (_bPtOldValid)
		{
			// okay, there already a drag-rect has been drawn - and its position
			// had been saved within the rectangle _rectPrev, wich already had been
			// copied into rcOld in the beginning, and a new drag position
			// is available, too.
			// If now rcOld and rcNew are the same, just stop further handling to not
			// draw the same drag-rectangle twice (this really happens, it should be
			// better avoided anywhere earlier)
			//
			if (rcOld.left==rcNew.left && rcOld.right==rcNew.right && rcOld.top== rcNew.top && rcOld.bottom==rcNew.bottom)
				return;

			rc.left   = min(rcOld.left, rcNew.left);
			rc.top    = min(rcOld.top,  rcNew.top);
			rc.right  = max(rcOld.left + rcOld.right,  rcNew.left + rcNew.right);
			rc.bottom = max(rcOld.top  + rcOld.bottom, rcNew.top  + rcNew.bottom);
			rc.right -= rc.left;
			rc.bottom-= rc.top;
		}
		else	rc = rcNew;	// only new rect will be drawn
	}
	else	rc = rcOld;	// only old rect will be drawn - to erase it

	// now rc contains the rectangle wich encloses all needed, new and/or previous rectangle
	// because in the following we drive within a memory device context wich is limited to rc,
	// we have to localize rcNew and rcOld within rc...
	//
	rcOld.left = rcOld.left - rc.left;
	rcOld.top = rcOld.top  - rc.top;
	rcNew.left = rcNew.left - rc.left;
	rcNew.top = rcNew.top  - rc.top;

	HDC hdcMem = ::CreateCompatibleDC(_hdc);
	HBITMAP hBm = ::CreateCompatibleBitmap(_hdc, rc.right, rc.bottom);
	hbrushOrig = (HBRUSH)::SelectObject(hdcMem, hBm);

	::SetBrushOrgEx(hdcMem, rc.left%8, rc.top%8, 0);
	hbmOrig = (HBITMAP)::SelectObject(hdcMem, _hbrush);

	::BitBlt(hdcMem, 0, 0, rc.right, rc.bottom, _hdc, rc.left, rc.top, SRCCOPY);
	if (_bPtOldValid)
	{	// erase the old drag-rectangle
		::PatBlt(hdcMem, rcOld.left  , rcOld.top  , rcOld.right  , rcOld.bottom  , PATINVERT);
		::PatBlt(hdcMem, rcOld.left+3, rcOld.top+3, rcOld.right-6, rcOld.bottom-6, PATINVERT);
	}
	if (pPt != NULL)
	{	// draw the new drag-rectangle
		::PatBlt(hdcMem, rcNew.left  , rcNew.top  , rcNew.right  , rcNew.bottom  , PATINVERT);
		::PatBlt(hdcMem, rcNew.left+3, rcNew.top+3, rcNew.right-6, rcNew.bottom-6, PATINVERT);
	}
	::BitBlt(_hdc, rc.left, rc.top, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY);

	SelectObject(hdcMem, hbrushOrig);
	SelectObject(hdcMem, hbmOrig);
	DeleteObject(hBm);
	DeleteDC(hdcMem);

	if (pPt == NULL)
	{
		#if defined(USE_LOCKWINDOWUPDATE)
		::LockWindowUpdate(NULL);
		#endif
		_bPtOldValid = FALSE;
		if (_hdc)
		{
			::ReleaseDC(0, _hdc);
			_hdc = NULL;
		}
	}
	else
		_bPtOldValid = TRUE;
}


void Gripper::getMousePoints(POINT* pt, POINT* ptPrev)
{
	*ptPrev	= _ptOld;
	_ptOld	= *pt;
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
	static TCHAR	szText[64];
	_tcItem.mask		= TCIF_PARAM | TCIF_TEXT;
	_tcItem.pszText		= szText;
	_tcItem.cchTextMax	= 64;
	::SendMessage(_hTabSource, TCM_GETITEM, _iItem, reinterpret_cast<LPARAM>(&_tcItem));
}

