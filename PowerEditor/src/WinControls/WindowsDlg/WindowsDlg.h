// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

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

#include "SizeableDlg.h"
#include "Common.h"

class DocTabView;

typedef enum {
	WDT_ACTIVATE = 1,
	WDT_SAVE = 2,
	WDT_CLOSE = 3,
	WDT_SORT = 4,
} WinDlgNotifyType;


struct NMWINDLG : public NMHDR {

	BOOL processed;
	WinDlgNotifyType type;
	UINT curSel;
	UINT nItems;
	UINT *Items;

	// ctor: initialize to zeroes
	NMWINDLG() { memset(this,0,sizeof(NMWINDLG)); }
};

extern const UINT WDN_NOTIFY;


class WindowsDlg : public SizeableDlg
{
	typedef SizeableDlg MyBaseClass;

public :
	WindowsDlg();
	int doDialog();
	virtual void init(HINSTANCE hInst, HWND parent, DocTabView *pTab);

	void doRefresh(bool invalidate = false);

protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL onInitDialog();
	virtual void onSize(UINT nType, int cx, int cy);
	virtual void onGetMinMaxInfo(MINMAXINFO* lpMMI);
	virtual LRESULT onWinMgr(WPARAM wp, LPARAM lp);
	virtual void destroy();
	void updateColumnNames();
	void fitColumnsToSize();
	void resetSelection();
	void doSave();
	void doClose();
	void doSortToTabs();
	void updateButtonState();
	void activateCurrent();
	void doColumnSort();
	void doCount();

	HWND _hList = nullptr;
	static RECT _lastKnownLocation;
	SIZE _szMinButton;
	SIZE _szMinListCtrl;
	DocTabView *_pTab = nullptr;
	std::vector<int> _idxMap;
	int _currentColumn = -1;
	int _lastSort = -1;
	bool _reverseSort = false;

private:
	virtual void init(HINSTANCE hInst, HWND parent);	
};

class WindowsMenu
{
public:
	WindowsMenu();
	~WindowsMenu();
	void init(HINSTANCE hInst, HMENU hMainMenu, const TCHAR *translation); 
	void initPopupMenu(HMENU hMenu, DocTabView *pTab);

private:
	HMENU _hMenu = nullptr;
};

