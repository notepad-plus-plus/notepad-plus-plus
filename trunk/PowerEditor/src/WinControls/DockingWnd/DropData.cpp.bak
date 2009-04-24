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

#include "DropData.h"
#include <Commctrl.h>
#include <shlobj.h>
#include "dockingResource.h"

DropData::DropData(HWND hWnd, DockingCont* pCont)
{
	_lRefCount		= 1;
	_pCont			= pCont;
	_hWnd			= hWnd;
	_pAreasData		= NULL;
	_hTab			= _pCont->getTabWnd();
	_hCaption		= _pCont->getCaptionWnd();
}

DropData::~DropData()
{
	if (_pAreasData != NULL)
	{
		delete [] _pAreasData;
	}
}

HRESULT __stdcall DropData::QueryInterface (REFIID iid, void ** ppvObject)
{
	if(iid == IID_IDropTarget || iid == IID_IUnknown)
	{
		AddRef();
		*ppvObject = this;
		return S_OK;
	}
	else
	{
		*ppvObject = 0;
		return E_NOINTERFACE;
	}
}

ULONG __stdcall DropData::AddRef(void)
{
	return InterlockedIncrement(&_lRefCount);
}	

ULONG __stdcall DropData::Release(void)
{
	LONG count = InterlockedDecrement(&_lRefCount);
		
	if(count == 0)
	{
		delete this;
		return 0;
	}
	else
	{
		return count;
	}
}

DWORD DropData::DropEffect(DWORD grfKeyState, POINTL ptl, DWORD dwAllowed)
{
	DWORD		dwEffect	= DROPEFFECT_NONE;
	POINT		pt			= {ptl.x, ptl.y};

	for (int iElem = 0; iElem < _iElemCnt; iElem++)
	{
		/* test if cursor points in a rect */
		if (::PtInRect(&_pAreasData[iElem].rcDropArea, pt) == TRUE)
		{
			dwEffect = DROPEFFECT_COPY;
			break;
		}
	}
	
	return dwEffect;
}

HRESULT __stdcall DropData::DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL ptl, DWORD * pdwEffect)
{
	int			iItem		= 0;
	int			iItemCnt	= 0;
	TCITEM		tcItem		= {0};

	/* initial element count */
	_iElemCnt				= 0;

	/* get amount of element in tab and add caption */
	iItemCnt = ::SendMessage(_hTab, TCM_GETITEMCOUNT, 0, 0) + 1;

	/* allocate resources */
	_pAreasData = (tAreaData*) new tAreaData[iItemCnt];

	/* get allowed areas */
	tcItem.mask		= TCIF_PARAM;

	if (::IsWindowVisible(_hTab) == TRUE)
	{
		/* get possible areas for all tabs */
		for (iItem = 0; iItem < (iItemCnt - 1); iItem++)
		{
			::SendMessage(_hTab, TCM_GETITEM, iItem, (LPARAM)&tcItem);
			
			if (((tTbData*)tcItem.lParam)->uMask & DWS_ACCEPTDATA)
			{
				if (pDataObject->QueryGetData(((tTbData*)tcItem.lParam)->pFETC) == S_OK)
				{
					_pAreasData[_iElemCnt].targetWnd = ((tTbData*)tcItem.lParam)->hClient;
					::SendMessage(_hTab, TCM_GETITEMRECT, iItem, (LPARAM)&_pAreasData[_iElemCnt].rcDropArea);
					ClientToScreen(_hTab, &_pAreasData[_iElemCnt++].rcDropArea);
				}
			}
		}
	}

	/* get caption area when current selected tab allowes data drop */
	iItem = ::SendMessage(_hTab, TCM_GETCURSEL, 0, 0);
	::SendMessage(_hTab, TCM_GETITEM, iItem, (LPARAM)&tcItem);
	if (((tTbData*)tcItem.lParam)->uMask & DWS_ACCEPTDATA)
	{
		if (pDataObject->QueryGetData(((tTbData*)tcItem.lParam)->pFETC) == S_OK)
		{
			_pAreasData[_iElemCnt].targetWnd = ((tTbData*)tcItem.lParam)->hClient;

			if (_pCont->isFloating())
			{
				_pCont->getWindowRect(_pAreasData[_iElemCnt].rcDropArea);
				_pAreasData[_iElemCnt].rcDropArea.bottom = _pAreasData[_iElemCnt].rcDropArea.top + 24;
			}
			else
			{
				::GetWindowRect(_hCaption, &_pAreasData[_iElemCnt].rcDropArea);
			}
			_iElemCnt++;
		}
	}

	*pdwEffect = DropEffect(grfKeyState, ptl, *pdwEffect);

	return S_OK;
}

HRESULT __stdcall DropData::DragOver(DWORD grfKeyState, POINTL ptl, DWORD * pdwEffect)
{
	*pdwEffect = DropEffect(grfKeyState, ptl, *pdwEffect);

	return S_OK;
}

HRESULT __stdcall DropData::DragLeave(void)
{
	return S_OK;
}

HRESULT __stdcall DropData::Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL ptl, DWORD * pdwEffect)
{
	POINT		pt	= {ptl.x, ptl.y};

	/* initial elements */
	*pdwEffect = DROPEFFECT_NONE;

	for (int iElem = 0; iElem < _iElemCnt; iElem++)
	{
		/* test if cursor points in a rect */
		if (::PtInRect(&_pAreasData[iElem].rcDropArea, (POINT)pt) == TRUE)
		{
			/* notify child windows */
			::SendMessage(_pAreasData[iElem].targetWnd, LMM_DROPDATA, 0, (LPARAM)pDataObject);

			*pdwEffect = DROPEFFECT_COPY;
			break;
		}
	}

	return S_OK;
}

void RegisterDropWindow(HWND hwnd, DockingCont* pCont, IDropTarget **ppDropTarget)
{
	DropData *pDropTarget = new DropData(hwnd, pCont);

	// tell OLE that the window is a drop target
	::RegisterDragDrop(hwnd, pDropTarget);

	*ppDropTarget = pDropTarget;
}

void UnregisterDropWindow(HWND hwnd, IDropTarget *pDropTarget)
{
	// remove drag+drop
	::RevokeDragDrop(hwnd);

	// remove the strong lock
	CoLockObjectExternal(pDropTarget, FALSE, TRUE);

	// release our own reference
	pDropTarget->Release();
}

