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

#ifndef GRIPPER_H
#define GRIPPER_H

#include "Resource.h"
#include "Docking.h"
#include "DockingCont.h"
#include "DockingManager.h"
#include "commctrl.h"


// Used by getRectAndStyle() to draw the drag rectangle
static const WORD DotPattern[] = 
{
	0x00aa, 0x0055, 0x00aa, 0x0055, 0x00aa, 0x0055, 0x00aa, 0x0055
};


#define MDLG_CLASS_NAME TEXT("moveDlg")


class Gripper
{
public:
	Gripper();
    
	void init(HINSTANCE hInst, HWND hParent) {
		_hInst   = hInst;	
		_hParent = hParent;
	};

	void startGrip(DockingCont* pCont, DockingManager* pDockMgr, void* pRes);

	~Gripper()
	{
		if (_hdc)
		{::ReleaseDC(0, _hdc);}
		if (_hbm)
		{::DeleteObject(_hbm);}
		if (_hbrush)
		{::DeleteObject(_hbrush);}
	
	}

protected :

	void create(void);

	static LRESULT CALLBACK staticWinProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(UINT Message, WPARAM wParam, LPARAM lParam);

	void onMove(void);
	void onButtonUp(void);

	void doTabReordering(POINT pt);
	void drawRectangle(POINT pt);
	void getMousePoints(POINT* pt, POINT* ptPrev);
	void getMovingRect(POINT pt, RECT *rc);
	DockingCont* contHitTest(POINT pt);
	DockingCont* workHitTest(POINT pt, RECT *rcCont = NULL);

	void initTabInformation(POINT pt);

	void CalcRectToScreen(HWND hWnd, RECT *rc) {
		ClientRectToScreenRect(hWnd, rc);
		ShrinkRcToSize(rc);
	};
	void CalcRectToClient(HWND hWnd, RECT *rc) {
		ScreenRectToClientRect(hWnd, rc);
		ShrinkRcToSize(rc);
	};
	void ShrinkRcToSize(RECT *rc) {
		rc->right	-= rc->left;
		rc->bottom	-= rc->top;
	};
	void DoCalcGripperRect(RECT* rc, RECT rcCorr, POINT pt) {
		if ((rc->left + rc->right) < pt.x)
			rc->left = pt.x - 20;
		if ((rc->top + rc->bottom) < pt.y)
			rc->top  += rcCorr.bottom - rc->bottom;
	};

private:
	/* Handle */
	HINSTANCE		_hInst;
	HWND			_hParent;
	HWND			_hSelf;

	/* data of container */
	tDockMgr		_dockData;
	DockingManager* _pDockMgr;
	DockingCont*	_pCont;

	/* mouse offset in moving rectangle */
	POINT			_ptOffset;

	/* remembers old mouse point */
	POINT			_ptOld;
	BOOL			_bPtOldValid;

	/* for sorting tabs */
	HWND			_hTab;
	HWND			_hTabSource;
	BOOL			_startMovingFromTab;
	int				_iItem;
	RECT			_rcItem;
	TCITEM			_tcItem;

	/* resource pointer of THIS class */
	void*			_pRes;

	HDC				_hdc;
	HBITMAP			_hbm;
	HBRUSH			_hbrush;

	/* is class registered */
	static BOOL		_isRegistered;
};



#endif // GRIPPER_H