//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
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

#ifndef WINDOWS_DLG_H
#define WINDOWS_DLG_H

#include "SizeableDlg.h"
#include "WindowsDlgRc.h"
#include "Parameters.h"
#include <vector>
#include <string>

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

	class CachedValue
	{
		std::generic_string fullname;
		int index;
	};

public :
	WindowsDlg();
	int doDialog(TiXmlNodeA *dlgNode);
	virtual void init(HINSTANCE hInst, HWND parent, DocTabView *pTab);

	void doRefresh(bool invalidate = false);
	bool changeDlgLang();

protected :
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
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
	TCHAR *buildFileName(TCHAR *buffer, int len, int pos, const TCHAR *filename);
	HMENU _hMenu;
};


#endif //WINDOWS_DLG_H
