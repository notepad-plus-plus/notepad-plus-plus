// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid      
// misunderstandings, we consider an application to constitute a          
// "derivative work" for the purpose of this license if it does any of the
// following:                                                             
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#ifndef WINDOWS_DLG_H
#define WINDOWS_DLG_H

#include "SizeableDlg.h"
#include "Common.h"

class DocTabView;
class TiXmlNodeA;

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

	class CachedValue
	{
		generic_string fullname;
		int index;
	};

public :
	WindowsDlg();
	int doDialog(TiXmlNodeA *dlgNode);
	virtual void init(HINSTANCE hInst, HWND parent, DocTabView *pTab);

	void doRefresh(bool invalidate = false);
	bool changeDlgLang();

protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL onInitDialog();
	virtual void onSize(UINT nType, int cx, int cy);
	virtual void onGetMinMaxInfo(MINMAXINFO* lpMMI);
	virtual LRESULT onWinMgr(WPARAM wp, LPARAM lp);
	virtual void destroy();
	void fitColumnsToSize();
	void resetSelection();
	void doSave();
	void doClose();
	void doSortToTabs();
	void updateButtonState();
	void activateCurrent();

	HWND _hList;
	static RECT _lastKnownLocation;
	SIZE _szMinButton;
	SIZE _szMinListCtrl;
	DocTabView *_pTab;
	std::vector<int> _idxMap;
	int _lastSort;
	bool _isSorted;
	TiXmlNodeA *_dlgNode;

private:
	virtual void init(HINSTANCE hInst, HWND parent);	
};

class WindowsMenu
{
public:
	WindowsMenu();
	~WindowsMenu();
	void init(HINSTANCE hInst, HMENU hMainMenu, const TCHAR *translation); 
	//void initMenu(HMENU hMenu, ScintillaEditView *pView);
	void initPopupMenu(HMENU hMenu, DocTabView *pTab);
	//void uninitPopupMenu(HMENU hMenu, ScintillaEditView *pView);
private:
	HMENU _hMenu;
};


#endif //WINDOWS_DLG_H
