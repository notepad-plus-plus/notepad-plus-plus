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


#ifndef DROP_TARGET_H
#define DROP_TARGET_H

#include "DockingCont.h"
#include <windows.h>


void RegisterDropWindow(HWND hwnd, DockingCont* pCont, IDropTarget **ppDropTarget);
void UnregisterDropWindow(HWND hwnd, IDropTarget *pDropTarget);


typedef struct {
	HWND		targetWnd;
	RECT		rcDropArea;
} tAreaData;


class DropData : public IDropTarget
{
public:
	/* IUnknown implementation */
	HRESULT __stdcall QueryInterface(REFIID iid, void** ppvObject);
	ULONG	__stdcall AddRef(void);
	ULONG	__stdcall Release(void);

	/* IDropTarget implementation */
	HRESULT __stdcall DragEnter(IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
	HRESULT __stdcall DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
	HRESULT __stdcall DragLeave(void);
	HRESULT __stdcall Drop(IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);

	// Constructor
	DropData(HWND hwnd, DockingCont* pCont);
	~DropData();

private:
	/* internal helper function */
	DWORD DropEffect(DWORD grfKeyState, POINTL pt, DWORD dwAllowed);
	bool  QueryDataObject(IDataObject *pDataObject);


private:
	LONG				_lRefCount;

	HWND				_hWnd;
	HWND				_hCaption;
	HWND				_hTab;

	DockingCont*		_pCont;

	tAreaData*			_pAreasData;
	int					_iElemCnt;
};


#endif	// DROP_TARGET_H