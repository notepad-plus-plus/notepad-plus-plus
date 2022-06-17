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
#include "DockingManager.h"
#include "DockingSplitter.h"
#include "DockingCont.h"
#include "Gripper.h"
#include "Parameters.h"

using namespace std;

BOOL DockingManager::_isRegistered = FALSE;

//Window of event handling DockingManager (can only be one)
static	HWND			hWndServer	= NULL;
//Next hook in line
static	HHOOK			gWinCallHook = NULL;
LRESULT CALLBACK focusWndProc(int nCode, WPARAM wParam, LPARAM lParam);

// Callback function that handles messages (to test focus)
LRESULT CALLBACK focusWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION && hWndServer)
	{
		DockingManager *pDockingManager = (DockingManager *)::GetWindowLongPtr(hWndServer, GWLP_USERDATA);
		if (pDockingManager)
		{
			vector<DockingCont*> & vcontainer = pDockingManager->getContainerInfo();
			CWPSTRUCT * pCwp = (CWPSTRUCT*)lParam;
			if (pCwp->message == WM_KILLFOCUS)
			{
				for (int i = 0; i < DOCKCONT_MAX; ++i)
				{
					vcontainer[i]->SetActive(FALSE);	//deactivate all containers
				}
			}
			else if (pCwp->message == WM_SETFOCUS)
			{
				for (int i = 0; i < DOCKCONT_MAX; ++i)
				{
					vcontainer[i]->SetActive(IsChild(vcontainer[i]->getHSelf(), pCwp->hwnd));	//activate the container that contains the window with focus, this can be none
				}
			}
		}
	}
	return CallNextHookEx(gWinCallHook, nCode, wParam, lParam);
}

DockingManager::DockingManager()
{
	memset(_iContMap, -1, CONT_MAP_MAX * sizeof(int));

	_iContMap[0] = CONT_LEFT;
	_iContMap[1] = CONT_RIGHT;
	_iContMap[2] = CONT_TOP;
	_iContMap[3] = CONT_BOTTOM;

	// create four containers with splitters
	for (int i = 0; i < DOCKCONT_MAX; ++i)
	{
		DockingCont *_pDockCont = new DockingCont;
		_vContainer.push_back(_pDockCont);

		DockingSplitter *_pSplitter = new DockingSplitter;
		_vSplitter.push_back(_pSplitter);
	}
}

DockingManager::~DockingManager()
{
	// delete 4 splitters
	for (int i = 0; i < DOCKCONT_MAX; ++i)
	{
		delete _vSplitter[i];
	}
}

void DockingManager::init(HINSTANCE hInst, HWND hWnd, Window ** ppWin)
{
	Window::init(hInst, hWnd);

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
		clz.lpszClassName = DSPC_CLASS_NAME;

		if (!::RegisterClass(&clz))
		{
			throw std::runtime_error("DockingManager::init : RegisterClass() function failed");
		}
		_isRegistered = TRUE;
	}

	_hSelf = ::CreateWindowEx(
					0,
					DSPC_CLASS_NAME,
					TEXT(""),
					WS_CHILD | WS_CLIPCHILDREN,
					CW_USEDEFAULT, CW_USEDEFAULT,
					CW_USEDEFAULT, CW_USEDEFAULT,
					_hParent,
					NULL,
					_hInst,
					(LPVOID)this);

	if (!_hSelf)
	{
		throw std::runtime_error("DockingManager::init : CreateWindowEx() function return null");
	}

	setClientWnd(ppWin);

	// create docking container
	for (int iCont = 0; iCont < DOCKCONT_MAX; ++iCont)
	{
		_vContainer[iCont]->init(_hInst, _hSelf);
		_vContainer[iCont]->doDialog(false);
		::SetParent(_vContainer[iCont]->getHSelf(), _hParent);

		if ((iCont == CONT_TOP) || (iCont == CONT_BOTTOM))
			_vSplitter[iCont]->init(_hInst, _hParent, _hSelf, DMS_HORIZONTAL);
		else
			_vSplitter[iCont]->init(_hInst, _hParent, _hSelf, DMS_VERTICAL);
	}
	// register window event hooking
	if (!hWndServer)
		hWndServer = _hSelf;
	CoInitialize(NULL);
	if (!gWinCallHook)	//only set if not already done
		gWinCallHook = ::SetWindowsHookEx(WH_CALLWNDPROC, focusWndProc, hInst, GetCurrentThreadId());

	if (!gWinCallHook)
	{
		throw std::runtime_error("DockingManager::init : SetWindowsHookEx() function return null");
	}

	_dockData.hWnd = _hSelf;

	_isInitialized = TRUE;
}

void DockingManager::destroy()
{
	::DestroyWindow(_hSelf);
}

LRESULT CALLBACK DockingManager::staticWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DockingManager *pDockingManager = NULL;
	switch (message)
	{
		case WM_NCCREATE :
			pDockingManager = reinterpret_cast<DockingManager *>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
			pDockingManager->_hSelf = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pDockingManager));
			return TRUE;

		default :
			pDockingManager = reinterpret_cast<DockingManager *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
			if (!pDockingManager)
				return ::DefWindowProc(hwnd, message, wParam, lParam);
			return pDockingManager->runProc(hwnd, message, wParam, lParam);
	}
}

void DockingManager::updateContainerInfo(HWND hClient)
{
	for (size_t iCont = 0, len = _vContainer.size(); iCont < len; ++iCont)
	{
		if (_vContainer[iCont]->updateInfo(hClient) == TRUE)
		{
			break;
		}
	}
}

void DockingManager::showFloatingContainers(bool show)
{
	for (size_t i=0; i < _vContainer.size(); i++)
	{
		size_t iElementCnt = _vContainer[i]->getElementCnt();
		if (iElementCnt > 0)
		{
			if (0 < ::SendMessage(_vContainer[i]->getTabWnd(), TCM_GETITEMCOUNT, 0, 0)) // any real item(s)?
				_vContainer[i]->display(show);
		}
	}
}

LRESULT DockingManager::runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_NCACTIVATE:
		{
			// activate/deactivate titlebar of toolbars
			for (size_t iCont = DOCKCONT_MAX, len = _vContainer.size(); iCont < len; ++iCont)
			{
				::SendMessage(_vContainer[iCont]->getHSelf(), WM_NCACTIVATE, wParam, static_cast<LPARAM>(-1));
			}

			if (static_cast<int>(lParam) != -1)
			{
				::SendMessage(_hParent, WM_NCACTIVATE, wParam, static_cast<LPARAM>(-1));
			}
			break;
		}
		case WM_MOVE:
		case WM_SIZE:
		{
			resize();
			break;
		}
		case WM_DESTROY:
		{
			// unregister window event hooking BEFORE EVERYTHING ELSE
			if (hWndServer == hwnd)
			{
				UnhookWindowsHookEx(gWinCallHook);
				gWinCallHook = NULL;
				hWndServer = NULL;
			}

			// destroy imagelist if it exists
			if (_hImageList != NULL)
			{
				::ImageList_Destroy(_hImageList);
			}

			// destroy containers
			for (int32_t i = static_cast<int32_t>(_vContainer.size()); i > 0; i--)
			{
				_vContainer[i-1]->destroy();
				delete _vContainer[i-1];
			}
			CoUninitialize();
			break;
		}
		case DMM_LBUTTONUP:	//is this message still relevant?
		{
			if (::GetActiveWindow() != _hParent)
				break;

			// set respective activate state
			for (int i = 0; i < DOCKCONT_MAX; ++i)
			{
				_vContainer[i]->SetActive(IsChild(_vContainer[i]->getHSelf(), ::GetFocus()));
			}
			return TRUE;
		}

		case DMM_MOVE:
		{
			Gripper *pGripper = new Gripper;
			pGripper->init(_hInst, _hParent);
			pGripper->startGrip(reinterpret_cast<DockingCont*>(lParam), this);
			break;
		}

		case DMM_MOVE_SPLITTER:
		{
			int offset = static_cast<int32_t>(wParam);

			for (int iCont = 0; iCont < DOCKCONT_MAX; ++iCont)
			{
				if (_vSplitter[iCont]->getHSelf() == reinterpret_cast<HWND>(lParam))
				{
					switch (iCont)
					{
						case CONT_TOP:
							_dockData.rcRegion[iCont].bottom -= offset;
							if (_dockData.rcRegion[iCont].bottom < 0)
							{
								_dockData.rcRegion[iCont].bottom = 0;
							}
							if ((_rcWork.bottom < (-SPLITTER_WIDTH)) && (offset < 0))
							{
								_dockData.rcRegion[iCont].bottom += offset;
							}
							break;
						case CONT_BOTTOM:
							_dockData.rcRegion[iCont].bottom   += offset;
							if (_dockData.rcRegion[iCont].bottom < 0)
							{
								_dockData.rcRegion[iCont].bottom   = 0;
							}
							if ((_rcWork.bottom < (-SPLITTER_WIDTH)) && (offset > 0))
							{
								_dockData.rcRegion[iCont].bottom -= offset;
							}
							break;
						case CONT_LEFT:
							_dockData.rcRegion[iCont].right    -= offset;
							if (_dockData.rcRegion[iCont].right < 0)
							{
								_dockData.rcRegion[iCont].right = 0;
							}
							if ((_rcWork.right < SPLITTER_WIDTH) && (offset < 0))
							{
								_dockData.rcRegion[iCont].right += offset;
							}
							break;
						case CONT_RIGHT:
							_dockData.rcRegion[iCont].right    += offset;
							if (_dockData.rcRegion[iCont].right < 0)
							{
								_dockData.rcRegion[iCont].right = 0;
							}
							if ((_rcWork.right < SPLITTER_WIDTH) && (offset > 0))
							{
								_dockData.rcRegion[iCont].right -= offset;
							}
							break;
					}
					resize();
					break;
				}
			}
			break;
		}
		case DMM_DOCK:
		case DMM_FLOAT:
		{
			toggleActiveTb(reinterpret_cast<DockingCont*>(lParam), message);
			return FALSE;
		}
		case DMM_CLOSE:
		{
			tTbData	TbData	= *(reinterpret_cast<DockingCont*>(lParam))->getDataOfActiveTb();
			LRESULT res = SendNotify(TbData.hClient, DMN_CLOSE);	// Be sure the active item is OK with closing
			return res;
		}
		case DMM_FLOATALL:
		{
			toggleVisTb(reinterpret_cast<DockingCont*>(lParam), DMM_FLOAT);
			return FALSE;
		}
		case DMM_DOCKALL:
		{
			toggleVisTb(reinterpret_cast<DockingCont*>(lParam), DMM_DOCK);
			return FALSE;
		}
		case DMM_GETIMAGELIST:
		{
			return reinterpret_cast<LPARAM>(_hImageList);
		}
		case DMM_GETICONPOS:
		{
			for (size_t uImageCnt = 0, len = _vImageList.size(); uImageCnt < len; ++uImageCnt)
			{
				if (reinterpret_cast<HWND>(lParam) == _vImageList[uImageCnt])
				{
					return uImageCnt;
				}
			}
			return -1;
		}
		default :
			break;
	}

	return ::DefWindowProc(_hSelf, message, wParam, lParam);
}

void DockingManager::resize()
{
    reSizeTo(_rect);
}

void DockingManager::reSizeTo(RECT & rc)
{
	// store current size of client area
	_rect = rc;

	// prepare size of work area
	_rcWork	= rc;

	if (_isInitialized == FALSE)
		return;

	// set top container
	_dockData.rcRegion[CONT_TOP].left      = rc.left;
	_dockData.rcRegion[CONT_TOP].top       = rc.top;
	_dockData.rcRegion[CONT_TOP].right     = rc.right-rc.left;

	_vSplitter[CONT_TOP]->display(false);

	if (_vContainer[CONT_TOP]->isVisible())
	{
		_rcWork.top		+= _dockData.rcRegion[CONT_TOP].bottom + SPLITTER_WIDTH;
		_rcWork.bottom	-= _dockData.rcRegion[CONT_TOP].bottom + SPLITTER_WIDTH;

		// set size of splitter
		RECT	rc = {_dockData.rcRegion[CONT_TOP].left  ,
					  _dockData.rcRegion[CONT_TOP].top + _dockData.rcRegion[CONT_TOP].bottom,
					  _dockData.rcRegion[CONT_TOP].right ,
					  SPLITTER_WIDTH};
		_vSplitter[CONT_TOP]->reSizeTo(rc);
	}

	// set bottom container
	_dockData.rcRegion[CONT_BOTTOM].left   = rc.left;
	_dockData.rcRegion[CONT_BOTTOM].top    = rc.top + rc.bottom - _dockData.rcRegion[CONT_BOTTOM].bottom;
	_dockData.rcRegion[CONT_BOTTOM].right  = rc.right-rc.left;

	// create temporary rect for bottom container
	RECT		rcBottom	= _dockData.rcRegion[CONT_BOTTOM];

	_vSplitter[CONT_BOTTOM]->display(false);

	if (_vContainer[CONT_BOTTOM]->isVisible())
	{
		_rcWork.bottom	-= _dockData.rcRegion[CONT_BOTTOM].bottom + SPLITTER_WIDTH;

		// correct the visibility of bottom container when height is NULL
		if (_rcWork.bottom < rc.top)
		{
			rcBottom.top     = _rcWork.top + rc.top + SPLITTER_WIDTH;
			rcBottom.bottom += _rcWork.bottom - rc.top;
			_rcWork.bottom = rc.top;
		}
		if ((rcBottom.bottom + SPLITTER_WIDTH) < 0)
		{
			_rcWork.bottom = rc.bottom - _dockData.rcRegion[CONT_TOP].bottom;
		}

		// set size of splitter
		RECT	rc = {rcBottom.left,
					  rcBottom.top - SPLITTER_WIDTH,
					  rcBottom.right,
					  SPLITTER_WIDTH};
		_vSplitter[CONT_BOTTOM]->reSizeTo(rc);
	}

	// set left container
	_dockData.rcRegion[CONT_LEFT].left     = rc.left;
	_dockData.rcRegion[CONT_LEFT].top      = _rcWork.top;
	_dockData.rcRegion[CONT_LEFT].bottom   = _rcWork.bottom;

	_vSplitter[CONT_LEFT]->display(false);

	if (_vContainer[CONT_LEFT]->isVisible())
	{
		_rcWork.left		+= _dockData.rcRegion[CONT_LEFT].right + SPLITTER_WIDTH;
		_rcWork.right	-= _dockData.rcRegion[CONT_LEFT].right + SPLITTER_WIDTH;

		// set size of splitter
		RECT	rc = {_dockData.rcRegion[CONT_LEFT].right,
					  _dockData.rcRegion[CONT_LEFT].top,
					  SPLITTER_WIDTH,
					  _dockData.rcRegion[CONT_LEFT].bottom};
		_vSplitter[CONT_LEFT]->reSizeTo(rc);
	}

	// set right container
	_dockData.rcRegion[CONT_RIGHT].left    = rc.right - _dockData.rcRegion[CONT_RIGHT].right;
	_dockData.rcRegion[CONT_RIGHT].top     = _rcWork.top;
	_dockData.rcRegion[CONT_RIGHT].bottom  = _rcWork.bottom;

	// create temporary rect for right container
	RECT		rcRight		= _dockData.rcRegion[CONT_RIGHT];

	_vSplitter[CONT_RIGHT]->display(false);
	if (_vContainer[CONT_RIGHT]->isVisible())
	{
		_rcWork.right	-= _dockData.rcRegion[CONT_RIGHT].right + SPLITTER_WIDTH;

		// correct the visibility of right container when width is NULL
		if (_rcWork.right < 15)
		{
			rcRight.left    = _rcWork.left + 15 + SPLITTER_WIDTH;
			rcRight.right  += _rcWork.right - 15;
			_rcWork.right	= 15;
		}

		// set size of splitter
		RECT	rc = {rcRight.left - SPLITTER_WIDTH,
					  rcRight.top,
					  SPLITTER_WIDTH,
					  rcRight.bottom};
		_vSplitter[CONT_RIGHT]->reSizeTo(rc);
	}

	// set window positions of container
	if (_vContainer[CONT_BOTTOM]->isVisible())
	{
		::SetWindowPos(_vContainer[CONT_BOTTOM]->getHSelf(), NULL,
					   rcBottom.left  ,
					   rcBottom.top   ,
					   rcBottom.right ,
					   rcBottom.bottom,
					   SWP_NOZORDER);
		_vSplitter[CONT_BOTTOM]->display();
	}

	if (_vContainer[CONT_TOP]->isVisible())
	{
		::SetWindowPos(_vContainer[CONT_TOP]->getHSelf(), NULL,
					   _dockData.rcRegion[CONT_TOP].left  ,
					   _dockData.rcRegion[CONT_TOP].top   ,
					   _dockData.rcRegion[CONT_TOP].right ,
					   _dockData.rcRegion[CONT_TOP].bottom,
					   SWP_NOZORDER);
		_vSplitter[CONT_TOP]->display();
	}

	if (_vContainer[CONT_RIGHT]->isVisible())
	{
		::SetWindowPos(_vContainer[CONT_RIGHT]->getHSelf(), NULL,
					   rcRight.left  ,
					   rcRight.top   ,
					   rcRight.right ,
					   rcRight.bottom,
					   SWP_NOZORDER);
		_vSplitter[CONT_RIGHT]->display();
	}

	if (_vContainer[CONT_LEFT]->isVisible())
	{
		::SetWindowPos(_vContainer[CONT_LEFT]->getHSelf(), NULL,
					   _dockData.rcRegion[CONT_LEFT].left  ,
					   _dockData.rcRegion[CONT_LEFT].top   ,
					   _dockData.rcRegion[CONT_LEFT].right ,
					   _dockData.rcRegion[CONT_LEFT].bottom,
					   SWP_NOZORDER);
		_vSplitter[CONT_LEFT]->display();
	}
	(*_ppMainWindow)->reSizeTo(_rcWork);
}

void DockingManager::createDockableDlg(tTbData data, int iCont, bool isVisible)
{
	// add icons
	if ((data.uMask & DWS_ICONTAB) && data.hIconTab != NULL)
	{
		// create image list if not exist
		if (_hImageList == NULL)
		{
			int iconDpiDynamicalSize = NppParameters::getInstance()._dpiManager.scaleY(12) + 2;
			_hImageList = ::ImageList_Create(iconDpiDynamicalSize, iconDpiDynamicalSize, ILC_COLOR32 | ILC_MASK, 0, 0);
		}

		// add icon
		::ImageList_AddIcon(_hImageList, data.hIconTab);

		// do the reference here to find later the correct position
		_vImageList.push_back(data.hClient);
	}

	if ((data.uMask & DWS_USEOWNDARKMODE) != DWS_USEOWNDARKMODE && NppDarkMode::isEnabledForPlugins())
	{
		NppDarkMode::autoSubclassAndThemePluginDockWindow(data.hClient);
	}

	// create additional containers if necessary
	RECT				rc			= {0,0,0,0};
	DockingCont*		pCont		= NULL;

	// if floated rect not set
	if (memcmp(&data.rcFloat, &rc, sizeof(RECT)) == 0)
	{
		// set default rect state
		::GetWindowRect(data.hClient, &data.rcFloat);

		// test if dialog is first time created
		if (iCont == -1)
		{
			// set default visible state
			isVisible = (::IsWindowVisible(data.hClient) == TRUE);

			if (data.uMask & DWS_DF_FLOATING)
			{
				// create new container
				pCont = new DockingCont;
				_vContainer.push_back(pCont);

				// initialize
				pCont->init(_hInst, _hSelf);
				pCont->doDialog(isVisible, true);

				// get previous position and set container id
				data.iPrevCont = (data.uMask & 0x30000000) >> 28;
				iCont = static_cast<int32_t>(_vContainer.size()) - 1;
			}
			else
			{
				// set position
				iCont = (data.uMask & 0x30000000) >> 28;

				// previous container is not selected
				data.iPrevCont = -1;
			}
		}
	}
	// if one of the container was not created before
	else if ((iCont >= DOCKCONT_MAX) || (data.iPrevCont >= DOCKCONT_MAX))
	{
        // test if current container is in floating state
		if (iCont >= DOCKCONT_MAX)
		{
			// no mapping for available store mapping
			if (_iContMap[iCont] == -1)
			{
				// create new container
				pCont = new DockingCont;
				_vContainer.push_back(pCont);

				// initialize and map container id
				pCont->init(_hInst, _hSelf);
				pCont->doDialog(isVisible, true);
				_iContMap[iCont] = static_cast<int32_t>(_vContainer.size()) - 1;
			}

			// get current container from map
			iCont = _iContMap[iCont];
		}
		// previous container is in floating state
		else
		{
			// no mapping for available store mapping
			if (_iContMap[data.iPrevCont] == -1)
			{
				// create new container
				pCont = new DockingCont;
				_vContainer.push_back(pCont);

				// initialize and map container id
				pCont->init(_hInst, _hSelf);
				pCont->doDialog(false, true);
				pCont->reSizeToWH(data.rcFloat);
				_iContMap[data.iPrevCont] = static_cast<int32_t>(_vContainer.size()) - 1;
			}
			data.iPrevCont = _iContMap[data.iPrevCont];
		}
	}

	// attach toolbar
	if (_vContainer.size() > (size_t)iCont && _vContainer[iCont] != NULL)
		_vContainer[iCont]->createToolbar(data);

	// notify client app
	if (iCont < DOCKCONT_MAX)
		SendNotify(data.hClient, MAKELONG(DMN_DOCK, iCont));
	else
		SendNotify(data.hClient, MAKELONG(DMN_FLOAT, iCont));
}

void DockingManager::setActiveTab(int iCont, int iItem)
{
	if ((iCont == -1) || (_iContMap[iCont] == -1))
		return;

	_vContainer[_iContMap[iCont]]->setActiveTb(iItem);
}

void DockingManager::showDockableDlg(HWND hDlg, BOOL view)
{
	for (size_t i = 0, len = _vContainer.size(); i < len; ++i)
	{
		tTbData *pTbData = _vContainer[i]->findToolbarByWnd(hDlg);
		if (pTbData != NULL)
		{
			_vContainer[i]->showToolbar(pTbData, view);
			return;
		}
	}
}

void DockingManager::showDockableDlg(TCHAR* pszName, BOOL view)
{
	for (size_t i = 0, len = _vContainer.size(); i < len; ++i)
	{
		tTbData *pTbData = _vContainer[i]->findToolbarByName(pszName);
		if (pTbData != NULL)
		{
			_vContainer[i]->showToolbar(pTbData, view);
			return;
		}
	}
}

LRESULT DockingManager::SendNotify(HWND hWnd, UINT message)
{
	NMHDR	nmhdr;
	nmhdr.code		= message;
	nmhdr.hwndFrom	= _hParent;
	nmhdr.idFrom	= ::GetDlgCtrlID(_hParent);
	::SendMessage(hWnd, WM_NOTIFY, nmhdr.idFrom, reinterpret_cast<LPARAM>(&nmhdr));
	return ::GetWindowLongPtr(hWnd, DWLP_MSGRESULT);
}

void DockingManager::setDockedContSize(int iCont, int iSize)
{
	if ((iCont == CONT_TOP) || (iCont == CONT_BOTTOM))
		_dockData.rcRegion[iCont].bottom = iSize;
	else if ((iCont == CONT_LEFT) || (iCont == CONT_RIGHT))
		_dockData.rcRegion[iCont].right = iSize;
	else
		return;
	resize();
}

int DockingManager::getDockedContSize(int iCont)
{
	if ((iCont == CONT_TOP) || (iCont == CONT_BOTTOM))
		return _dockData.rcRegion[iCont].bottom;
	else if ((iCont == CONT_LEFT) || (iCont == CONT_RIGHT))
		return _dockData.rcRegion[iCont].right;
	else
		return -1;
}

DockingCont* DockingManager::toggleActiveTb(DockingCont* pContSrc, UINT message, BOOL bNew, LPRECT prcFloat)
{
	tTbData			TbData		= *pContSrc->getDataOfActiveTb();
	int				iContSrc	= GetContainer(pContSrc);
	int				iContPrev	= TbData.iPrevCont;
	BOOL			isCont		= ContExists(iContPrev);
	DockingCont*	pContTgt	= NULL;

	// if new float position is given
	if (prcFloat != NULL)
	{
		TbData.rcFloat = *prcFloat;
	}

	if ((isCont == FALSE) || (bNew == TRUE))
	{
		// find an empty container
		int	iContNew = FindEmptyContainer();

		if (iContNew == -1)
		{
			// if no free container is available create a new one
			pContTgt = new DockingCont;
			pContTgt->init(_hInst, _hSelf);
			pContTgt->doDialog(true, true);

			// change only on toggling
			if ((bNew == FALSE) || (!pContSrc->isFloating()))
				TbData.iPrevCont = iContSrc;

			pContTgt->createToolbar(TbData);
			_vContainer.push_back(pContTgt);
		}
		else
		{
			// set new target
			pContTgt = _vContainer[iContNew];

			// change only on toggling
			if ((pContSrc->isFloating()) != (pContTgt->isFloating()))
                TbData.iPrevCont = iContSrc;

			pContTgt->createToolbar(TbData);
		}
	}
	else
	{
		// set new target
		pContTgt = _vContainer[iContPrev];

		// change data normaly
		TbData.iPrevCont = iContSrc;
		pContTgt->createToolbar(TbData);
	}

	// notify client app
	SendNotify(TbData.hClient, MAKELONG(message==DMM_DOCK?DMN_DOCK:DMN_FLOAT, GetContainer(pContTgt)));

	// remove toolbar from source
	_vContainer[iContSrc]->removeToolbar(TbData);

	return pContTgt;
}

DockingCont* DockingManager::toggleVisTb(DockingCont* pContSrc, UINT message, LPRECT prcFloat)
{
	vector<tTbData*>	vTbData		= pContSrc->getDataOfVisTb();
	tTbData*			pTbData		= pContSrc->getDataOfActiveTb();

	int					iContSrc	= GetContainer(pContSrc);
	int					iContPrev	= pTbData->iPrevCont;
	BOOL				isCont		= ContExists(iContPrev);
	DockingCont*		pContTgt	= NULL;

	// at first hide container and resize
	pContSrc->doDialog(false);
	resize();

	for (size_t iTb = 0, len = vTbData.size(); iTb < len; ++iTb)
	{
		// get data one by another
		tTbData		TbData = *vTbData[iTb];

		// if new float position is given
		if (prcFloat != NULL)
		{
			TbData.rcFloat = *prcFloat;
		}

		if (isCont == FALSE)
		{
            // create new container
			pContTgt = new DockingCont;
			pContTgt->init(_hInst, _hSelf);
			pContTgt->doDialog(true, true);

			TbData.iPrevCont = iContSrc;
			pContTgt->createToolbar(TbData);
			_vContainer.push_back(pContTgt);

			// now container exists
			isCont	= TRUE;
			iContPrev = GetContainer(pContTgt);
		}
		else
		{
			// set new target
			pContTgt = _vContainer[iContPrev];

			TbData.iPrevCont = iContSrc;
			pContTgt->createToolbar(TbData);
		}

		SendNotify(TbData.hClient, MAKELONG(message==DMM_DOCK?DMN_DOCK:DMN_FLOAT, GetContainer(pContTgt)));

		// remove toolbar from anywhere
		_vContainer[iContSrc]->removeToolbar(TbData);
	}

	_vContainer[iContPrev]->setActiveTb(pTbData);
	return pContTgt;
}

void DockingManager::toggleActiveTb(DockingCont* pContSrc, DockingCont* pContTgt)
{
	tTbData		TbData		= *pContSrc->getDataOfActiveTb();

	toggleTb(pContSrc, pContTgt, TbData);
}

void DockingManager::toggleVisTb(DockingCont* pContSrc, DockingCont* pContTgt)
{
	vector<tTbData*>	vTbData		= pContSrc->getDataOfVisTb();
	tTbData*			pTbData		= pContSrc->getDataOfActiveTb();

	// at first hide container and resize
	pContSrc->doDialog(false);
	resize();

	for (size_t iTb = 0, len = vTbData.size(); iTb < len; ++iTb)
	{
		// get data one by another
		tTbData		TbData = *vTbData[iTb];
		toggleTb(pContSrc, pContTgt, TbData);
	}
	pContTgt->setActiveTb(pTbData);
}

void DockingManager::toggleTb(DockingCont* pContSrc, DockingCont* pContTgt, tTbData TbData)
{
	int					iContSrc	= GetContainer(pContSrc);
	int					iContTgt	= GetContainer(pContTgt);

	// test if container state changes from docking to floating or vice versa
	if (((iContSrc <  DOCKCONT_MAX) && (iContTgt >= DOCKCONT_MAX)) ||
		((iContSrc >= DOCKCONT_MAX) && (iContTgt <  DOCKCONT_MAX)))
	{
		// change states
		TbData.iPrevCont = iContSrc;
	}

	// notify client app
	if (iContTgt < DOCKCONT_MAX)
		SendNotify(TbData.hClient, MAKELONG(DMN_DOCK, iContTgt));
	else
		SendNotify(TbData.hClient, MAKELONG(DMN_FLOAT, iContTgt));

	// create new toolbar
	pContTgt->createToolbar(TbData);

	// remove toolbar from source
	_vContainer[iContSrc]->removeToolbar(TbData);
}

BOOL DockingManager::ContExists(size_t iCont)
{
	BOOL	bRet = FALSE;

	if (iCont < _vContainer.size())
	{
		bRet = TRUE;
	}

	return bRet;
}

int DockingManager::GetContainer(DockingCont* pCont)
{
	int iRet = -1;
	for (size_t iCont = 0, len = _vContainer.size(); iCont < len; ++iCont)
	{
		if (_vContainer[iCont] == pCont)
		{
			iRet = static_cast<int32_t>(iCont);
			break;
		}
	}

	return iRet;
}

int DockingManager::FindEmptyContainer()
{
    int      iRetCont       = -1;
    BOOL*    pPrevDockList  = (BOOL*) new BOOL[_vContainer.size()+1];
    BOOL*    pArrayPos      = &pPrevDockList[1];

    // delete all entries
    for (size_t iCont = 0, len = _vContainer.size()+1; iCont < len; ++iCont)
    {
        pPrevDockList[iCont] = FALSE;
    }

    // search for used floated containers
    for (size_t iCont = 0; iCont < DOCKCONT_MAX; ++iCont)
    {
        vector<tTbData*>    vTbData = _vContainer[iCont]->getDataOfAllTb();

        for (size_t iTb = 0, len = vTbData.size(); iTb < len; ++iTb)
        {
            pArrayPos[vTbData[iTb]->iPrevCont] = TRUE;
        }
    }

    // find free container
    for (size_t iCont = DOCKCONT_MAX, len = _vContainer.size(); iCont < len; ++iCont)
    {
        if (pArrayPos[iCont] == FALSE)
        {
            // and test if container is hidden
            if (!_vContainer[iCont]->isVisible())
            {
				iRetCont = static_cast<int32_t>(iCont);
                break;
            }
        }
    }

    delete [] pPrevDockList;

    // search for empty arrays
    return iRetCont;
}
