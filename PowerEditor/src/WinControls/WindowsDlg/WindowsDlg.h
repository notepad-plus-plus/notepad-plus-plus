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
#include "ContextMenu.h"

class DocTabView;
class Buffer;

typedef enum {
	WDT_ACTIVATE = 1,
	WDT_SAVE = 2,
	WDT_CLOSE = 3,
	WDT_SORT = 4,
} WinDlgNotifyType;


struct NMWINDLG : public NMHDR {

	BOOL processed = FALSE;
	WinDlgNotifyType type = WDT_ACTIVATE;
	UINT curSel = 0;
	UINT nItems = 0;
	UINT *Items = 0;

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
	void doSortToTabs();
	void doSort();
	void sort(int columnID, bool reverseSort);
	void sortFileNameASC();
	void sortFileNameDSC();
	void sortFilePathASC();
	void sortFilePathDSC();
	void sortFileTypeASC();
	void sortFileTypeDSC();
	void sortFileSizeASC();
	void sortFileSizeDSC();
	void doRefresh(bool invalidate = false);

protected :
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
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
	void updateButtonState();
	void activateCurrent();
	void doColumnSort();
	void doCount();
	void refreshMap();
	void putItemsToClipboard(bool isFullPath);
	Buffer* getBuffer(int index) const;

	static LONG_PTR originalListViewProc;
	static LRESULT CALLBACK listViewProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	HWND _hList = nullptr;
	static RECT _lastKnownLocation;
	SIZE _szMinButton = {};
	SIZE _szMinListCtrl = {};
	DocTabView* _pTab = nullptr;
	std::vector<int> _idxMap;
	int _currentColumn = -1;
	int _lastSort = -1;
	bool _reverseSort = false;
	ContextMenu _listMenu;

private:
	virtual void init(HINSTANCE hInst, HWND parent);	
};

class WindowsMenu
{
public:
	WindowsMenu() {};
	~WindowsMenu() {};
	void init(HMENU hMainMenu); 
	void initPopupMenu(HMENU hMenu, DocTabView *pTab);

private:
	HMENU _hMenu = nullptr;
	HMENU _hMenuList = nullptr;
	UINT _limitPrev = 0;
};
