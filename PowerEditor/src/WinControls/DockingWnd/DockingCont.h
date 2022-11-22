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


#pragma once
#include "resource.h"
#include "Docking.h"
#include <vector>
#include "StaticDialog.h"
#include "Common.h"


// window styles
#define POPUP_STYLES		(WS_POPUP|WS_CLIPSIBLINGS|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MAXIMIZEBOX)
#define POPUP_EXSTYLES		(WS_EX_CONTROLPARENT|WS_EX_WINDOWEDGE|WS_EX_TOOLWINDOW)
#define CHILD_STYLES		(WS_CHILD)
#define CHILD_EXSTYLES		(0x00000000L)


enum eMousePos {
	posOutside,
	posCaption,
	posClose
};

// some fix modify values for GUI
#define	HIGH_CAPTION		18
#define CAPTION_GAP			2
#define CLOSEBTN_POS_LEFT	3
#define CLOSEBTN_POS_TOP	3


class DockingCont : public StaticDialog
{
public:
	DockingCont();
	~DockingCont();

	HWND getTabWnd() {
		return _hContTab;
	};
	HWND getCaptionWnd() { 
		if (_isFloating == false)
			return _hCaption;
		else
			return _hSelf;
	};

	tTbData* createToolbar(tTbData data);
	void	 removeToolbar(tTbData data);
	tTbData* findToolbarByWnd(HWND hClient);
	tTbData* findToolbarByName(TCHAR* pszName);

	void showToolbar(tTbData *pTbData, BOOL state);

	BOOL updateInfo(HWND hClient) {
		for (size_t iTb = 0; iTb < _vTbData.size(); ++iTb)
		{
			if (_vTbData[iTb]->hClient == hClient)
			{
				updateCaption();
				return TRUE;
			}
		}
		return FALSE;
	};

	void setActiveTb(tTbData* pTbData);
	void setActiveTb(int iItem);
	int getActiveTb();
	tTbData * getDataOfActiveTb();
	std::vector<tTbData *> getDataOfAllTb() {
		return _vTbData;
	};
	std::vector<tTbData *> getDataOfVisTb();
	bool isTbVis(tTbData* data);

	void doDialog(bool willBeShown = true, bool isFloating = false);

	bool isFloating() {
		return _isFloating;
	}

	size_t getElementCnt() {
		return _vTbData.size();
	}

	// interface function for gripper
	BOOL startMovingFromTab() {
		BOOL	dragFromTabTemp = _dragFromTab;
		_dragFromTab = FALSE;
		return dragFromTabTemp;
	};

	void setCaptionTop(BOOL isTopCaption) {
		_isTopCaption = (isTopCaption == CAPTION_TOP);
		onSize();
	};

	void focusClient();

	void SetActive(BOOL bState) {
		_isActive = bState;
		updateCaption();
	};

	void setTabStyle(const BOOL & bDrawOgLine) {
		_bDrawOgLine = bDrawOgLine;
		RedrawWindow(_hContTab, NULL, NULL, 0);
	};

    virtual void destroy() {
		for (int iTb = static_cast<int>(_vTbData.size()); iTb > 0; iTb--)
		{
			delete _vTbData[iTb-1];
		}
		::DestroyWindow(_hSelf);
	};

protected :

	// Subclassing caption
	LRESULT runProcCaption(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndCaptionProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((DockingCont *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProcCaption(hwnd, Message, wParam, lParam));
	};

	// Subclassing tab
	LRESULT runProcTab(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndTabProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((DockingCont *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProcTab(hwnd, Message, wParam, lParam));
	};

    virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

	// drawing functions
	void drawCaptionItem(DRAWITEMSTRUCT *pDrawItemStruct);
	void drawTabItem(DRAWITEMSTRUCT *pDrawItemStruct);
	void onSize();

	// functions for caption handling and drawing
	eMousePos isInRect(HWND hwnd, int x, int y);

	// handling of toolbars
	void doClose(BOOL closeAll);

	// return new item
	int  searchPosInTab(tTbData* pTbData);
	void selectTab(int iTab);

	int  hideToolbar(tTbData* pTbData, BOOL hideClient = TRUE);
	void viewToolbar(tTbData *pTbData);
	int  removeTab(tTbData* pTbData) {
		return hideToolbar(pTbData, FALSE);
	};

	bool updateCaption();
	LPARAM NotifyParent(UINT message);

private:
	// handles
	BOOL _isActive = FALSE;
	bool _isFloating = FALSE;
	HWND _hCaption = nullptr;
	HWND _hContTab = nullptr;
	HWND _hTabUpdown = nullptr;

	// horizontal font for caption and tab
	HFONT _hFont = nullptr;

	// caption params
	BOOL _isTopCaption = FALSE;
	generic_string _pszCaption;

	BOOL _isMouseDown = FALSE;
	BOOL _isMouseClose = FALSE;
	BOOL _isMouseOver = FALSE;
	RECT _rcCaption = {};
	
	// tab style
	BOOL _bDrawOgLine = FALSE;

	// Important value for DlgMoving class
	BOOL _dragFromTab = FALSE;

	// subclassing handle for caption
	WNDPROC _hDefaultCaptionProc = nullptr;

	// subclassing handle for tab
	WNDPROC _hDefaultTabProc = nullptr;

	// for moving and reordering
	UINT _prevItem = 0;
	BOOL _beginDrag = FALSE;

	// Is tooltip
	BOOL _bTabTTHover = FALSE;
	INT _iLastHovered = 0;

	BOOL _bCaptionTT = FALSE;
	BOOL _bCapTTHover = FALSE;
	eMousePos _hoverMPos = posOutside;

	int _captionHeightDynamic = HIGH_CAPTION;
	int _captionGapDynamic = CAPTION_GAP;
	int _closeButtonPosLeftDynamic = CLOSEBTN_POS_LEFT;
	int _closeButtonPosTopDynamic = CLOSEBTN_POS_TOP;
	int _closeButtonWidth = 12;
	int _closeButtonHeight = 12;

	// data of added windows
	std::vector<tTbData *> _vTbData;
};

