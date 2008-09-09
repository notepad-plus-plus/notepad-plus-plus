//this file is part of docking functionality for Notepad++
//Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>
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


#include "dockingResource.h"
#include "math.h"
#include "Docking.h"
#include "Gripper.h"

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

static LRESULT CALLBACK hookProcMouse(INT nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
		switch (wParam)
		{
			case WM_MOUSEMOVE:
			case WM_NCMOUSEMOVE:
				//::PostMessage(hWndServer, wParam, 0, 0);
				::SendMessage(hWndServer, wParam, 0, 0);
				break;
			case WM_LBUTTONUP:
			case WM_NCLBUTTONUP:
				//::PostMessage(hWndServer, wParam, 0, 0);
				::SendMessage(hWndServer, wParam, 0, 0);
				return TRUE;
			default: 
				break;
		}
	}
	return ::CallNextHookEx(hookMouse, nCode, wParam, lParam);
}

static LRESULT CALLBACK hookProcKeyboard(INT nCode, WPARAM wParam, LPARAM lParam)
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

Gripper::Gripper(void)
{
	_hInst				= NULL;
	_hParent			= NULL;
	_hSelf				= NULL;

	_pDockMgr			= NULL;
	_pCont				= NULL;

	_ptOffset.x			= 0;
	_ptOffset.y			= 0;

	_ptOld.x			= 0;
	_ptOld.y			= 0;
	_bPtOldValid		= FALSE;

	_hTab				= NULL;
	_hTabSource			= NULL;
	_startMovingFromTab	= FALSE;
	_iItem				= 0;

	_hdc				= NULL;
	_hbm				= NULL;
	_hbrush				= NULL;


	memset(&_rcItem, 0, sizeof(RECT));
	memset(&_tcItem, 0, sizeof(TCITEM));
	memset(&_dockData, 0, sizeof(tDockMgr));
}


void Gripper::startGrip(DockingCont* pCont, DockingManager* pDockMgr, void* pRes)
{
	MSG			msg		= {0};
	BOOL		bIsRel  = FALSE;
	HWND		hWnd	= NULL;

	_pDockMgr   = pDockMgr;
	_pCont		= pCont;
	_pRes		= pRes;

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
			systemMessage(TEXT("System Err"));
			throw int(98);
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
		systemMessage(TEXT("System Err"));
		throw int(777);
	}
}


LRESULT CALLBACK Gripper::staticWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Gripper *pDlgMoving = NULL;
	switch (message)
	{	
		case WM_NCCREATE :
			pDlgMoving = (Gripper *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
			pDlgMoving->_hSelf = hwnd;
			::SetWindowLongPtr(hwnd, GWL_USERDATA, reinterpret_cast<LONG>(pDlgMoving));
			return TRUE;

		default :
			pDlgMoving = (Gripper *)::GetWindowLongPtr(hwnd, GWL_USERDATA);
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
			RECT			rc			= {0};

			::GetCursorPos(&pt);
			getMousePoints(&pt, &ptBuf);

			/* erase last drawn rectangle */
			drawRectangle(ptBuf);

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
			delete _pRes;
			break;
		}
		default:
			break;
	}

	return ::DefWindowProc(_hSelf, message, wParam, lParam);
}

 
void Gripper::create(void)
{
	RECT		rc		= {0};
	POINT		pt		= {0};

	/* start hooking */
	::SetWindowPos(_pCont->getHSelf(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	::SetCapture(_hSelf);

	if (GetVersion() & 0x80000000)
	{
		hookMouse	= ::SetWindowsHookEx(WH_MOUSE, (HOOKPROC)hookProcMouse, _hInst, 0);
	}
	else
	{
		hookMouse	= ::SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC)hookProcMouse, _hInst, 0);
	}

    if (!hookMouse)
    {
        DWORD dwError = ::GetLastError();
        TCHAR  str[128];
        ::wsprintf(str, TEXT("GetLastError() returned %lu"), dwError);
        ::MessageBox(NULL, str, TEXT("SetWindowsHookEx(MOUSE) failed"), MB_OK | MB_ICONERROR);
    }

	winVer winVersion = (NppParameters::getInstance())->getWinVersion();
	if (winVersion <  WV_VISTA)
	{
	hookKeyboard	= ::SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)hookProcKeyboard, _hInst, 0);
    if (!hookKeyboard)
    {
        DWORD dwError = ::GetLastError();
        TCHAR  str[128];
        ::wsprintf(str, TEXT("GetLastError() returned %lu"), dwError);
        ::MessageBox(NULL, str, TEXT("SetWindowsHookEx(KEYBOARD) failed"), MB_OK | MB_ICONERROR);
    }
	}
//  Removed regarding W9x systems
//	mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

	/* calculate the mouse pt within dialog */
	::GetCursorPos(&pt);
	
	/* get tab informations */
	initTabInformation(pt);

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


void Gripper::onMove(void)
{
	POINT		pt		= {0,0};
	POINT		ptBuf	= {0,0};

	::GetCursorPos(&pt);
	getMousePoints(&pt, &ptBuf);

	/* On first time: Do not erase previous rect, because it dosn't exist */
	if (_bPtOldValid == TRUE)
		drawRectangle(ptBuf);

	/* tab reordering only when tab was selected */
	if (_startMovingFromTab == TRUE)
	{
		doTabReordering(pt);
	}

	drawRectangle(pt);
	_bPtOldValid = TRUE;
}


void Gripper::onButtonUp(void)
{
	POINT			pt			= {0,0};
	POINT			ptBuf		= {0,0};
	RECT			rc			= {0};
	RECT			rcCorr		= {0};
	DockingCont*	pContMove	= NULL;

	::GetCursorPos(&pt);
	getMousePoints(&pt, &ptBuf);

	/* do nothing, when old point is not valid */
	if (_bPtOldValid == FALSE)
		return;

	/* erase last drawn rectangle */
	drawRectangle(ptBuf);

	/* look if current position is within dockable area */
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
	int						iItem		= -1;
	int						iItemOld	= _iItem;

	/* search for every tab entry */
	for (size_t iCont = 0; iCont < vCont.size(); iCont++)
	{
		hTab = vCont[iCont]->getTabWnd();

		/* search only if container is visible */
		if (::IsWindowVisible(hTab) == TRUE)
		{
			RECT	rc		= {0};

			::GetWindowRect(hTab, &rc);

			/* test if cursor points in tab window */
			if (::PtInRect(&rc, pt) == TRUE)
			{
				TCHITTESTINFO	info	= {0};
				TCITEM			tcItem	= {0};

				if (_hTab == NULL)
				{
					initTabInformation(pt);
					hTabOld  = _hTab;
					iItemOld = _iItem;
				}

				/* get pointed tab item */
				info.pt	= pt;
				::ScreenToClient(hTab, &info.pt);
				iItem = ::SendMessage(hTab, TCM_HITTEST, 0, (LPARAM)&info);

				if (iItem != -1)
				{
					/* prevent flickering of tabs with different sizes */
					::SendMessage(hTab, TCM_GETITEMRECT, iItem, (LPARAM)&rc);
					ClientRectToScreenRect(hTab, &rc);

					if ((rc.left + (_rcItem.right  - _rcItem.left)) < pt.x)
					{
						return;
					}

					_iItem	= iItem;
				}
				else if ((hTab != _hTab) || (_iItem == -1))
				{
					/* test if cusor points after last tab */
					int		iLastItem	= ::SendMessage(hTab, TCM_GETITEMCOUNT, 0, 0) - 1;

					::SendMessage(hTab, TCM_GETITEMRECT, iLastItem, (LPARAM)&rc);
					if ((rc.left + rc.right) < pt.x)
					{
						_iItem = iLastItem + 1;
					}
				}

				_hTab = hTab;
				inTab = TRUE;
				break;
			}
		}
	}

	/* set and remove tabs correct */
	if ((inTab == TRUE) && (iItemOld != _iItem))
	{
		if (_hTab == _hTabSource)
		{
			/* delete item if switching back to source tab */
			int iSel = ::SendMessage(_hTab, TCM_GETCURSEL, 0, 0);
			::SendMessage(_hTab, TCM_DELETEITEM, iSel, 0);
		}
		else if (_hTab == hTabOld)
		{
			/* delete item on switch between tabs */
			::SendMessage(_hTab, TCM_DELETEITEM, iItemOld, 0);
		}
		else
		{
			if (_hTab == hTabOld)
			{
				/* delete item on switch between tabs */
				::SendMessage(_hTab, TCM_DELETEITEM, iItemOld, 0);
			}
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

	/* insert new entry when mouse doesn't point to current hovered tab */
	if ((_hTab != hTabOld) || (_iItem != iItemOld))
	{
		_tcItem.mask	= TCIF_PARAM | (_hTab == _hTabSource ? TCIF_TEXT : 0);
		::SendMessage(_hTab, TCM_INSERTITEM, _iItem, (LPARAM)&_tcItem);
	}

	/* select the tab only in source tab window */
	if ((_hTab == _hTabSource) && (_iItem != -1))
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


void Gripper::drawRectangle(POINT pt)
{
	HANDLE		hbrushOrig	= NULL;
	RECT		rc			= {0};

	if (!_hdc)
		_hdc = ::GetDC(NULL);
	
	// Create a brush with the appropriate bitmap pattern to draw our drag rectangle
	if (!_hbm)
		_hbm = ::CreateBitmap(8, 8, 1, 1, DotPattern);
	if (!_hbrush)
		_hbrush = ::CreatePatternBrush(_hbm);


	// Determine whether to draw a solid drag rectangle or checkered
	getMovingRect(pt, &rc);

	::SetBrushOrgEx(_hdc, rc.left, rc.top, 0);
	hbrushOrig = ::SelectObject(_hdc, _hbrush);

	// line: left
	::PatBlt(_hdc, rc.left, rc.top, 3, rc.bottom - 3, PATINVERT);
	// line: top
	::PatBlt(_hdc, rc.left + 3, rc.top, rc.right - 3, 3, PATINVERT);
	// line: right
	::PatBlt(_hdc, rc.left + rc.right - 3, rc.top + 3, 3, rc.bottom - 3, PATINVERT);
	// line: bottom
	::PatBlt(_hdc, rc.left, rc.top + rc.bottom - 3, rc.right - 3,  3, PATINVERT);

	// destroy resources
	::SelectObject(_hdc, hbrushOrig);
	
}


void Gripper::getMousePoints(POINT* pt, POINT* ptPrev)
{
	*ptPrev	= _ptOld;
	_ptOld	= *pt;
}


void Gripper::getMovingRect(POINT pt, RECT *rc)
{
	RECT			rcCorr			= {0};
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

	for (UINT iCont = 0; iCont < vCont.size(); iCont++)
	{
		/* test if within caption */
		if (hWnd == vCont[iCont]->getCaptionWnd())
		{
			if (vCont[iCont]->isFloating())
			{
				RECT	rc	= {0};

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
			RECT		rc	= {0};

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
	RECT					rcCont	= {0};
	vector<DockingCont*>	vCont	= _pDockMgr->getContainerInfo();

	/* at first test if cursor points into a visible container */
	for (size_t iCont = 0; iCont < vCont.size(); iCont++)
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
	for (int iWork = 0; iWork < DOCKCONT_MAX; iWork++)
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
			ClientRectToScreenRect(_dockData.hWnd, &rcCont);

			if (::PtInRect(&rcCont, pt) == TRUE)
			{
				if (rc != NULL)
				{
					ClientRectToScreenRect(_dockData.hWnd, rc);
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


void Gripper::initTabInformation(POINT pt)
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
		/* get active tab item */
		_iItem	= ::SendMessage(_hTabSource, TCM_GETCURSEL, 0, 0);
	}

	/* get size of item */
	_hTab = _hTabSource;
	::SendMessage(_hTabSource, TCM_GETITEMRECT, _iItem, (LPARAM)&_rcItem);

	/* store item data */
	static TCHAR	szText[64];
	_tcItem.mask		= TCIF_PARAM | TCIF_TEXT;
	_tcItem.pszText		= szText;
	_tcItem.cchTextMax	= 64;
	::SendMessage(_hTabSource, TCM_GETITEM, _iItem, (LPARAM)&_tcItem);
}

