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


#ifndef SHORTCUTMAPPER
#define SHORTCUTMAPPER

#ifndef BABYGRIDWRAPPER
#include "BabyGridWrapper.h"
#endif// BABYGRIDWRAPPER

#ifndef SHORTCUTMAPPER_RC_H
#include "ShortcutMapper_rc.h"
#endif //SHORTCUTMAPPER_RC_H

#ifndef SHORTCUTS_H
#include "shortcut.h"
#endif// SHORTCUTS_H

#ifndef CONTEXTMENU_H
#include "ContextMenu.h"
#endif// CONTEXTMENU_H

enum GridState {STATE_MENU, STATE_MACRO, STATE_USER, STATE_PLUGIN, STATE_SCINTILLA};

class ShortcutMapper : public StaticDialog {
public:
	ShortcutMapper() : _currentState(STATE_MENU), StaticDialog() {
		generic_strncpy(tabNames[0], TEXT("Main menu"), maxTabName);
		generic_strncpy(tabNames[1], TEXT("Macros"), maxTabName);
		generic_strncpy(tabNames[2], TEXT("Run commands"), maxTabName);
		generic_strncpy(tabNames[3], TEXT("Plugin commands"), maxTabName);
		generic_strncpy(tabNames[4], TEXT("Scintilla commands"), maxTabName);
	};
	~ShortcutMapper() {};

	void init(HINSTANCE hInst, HWND parent, GridState initState = STATE_MENU) {
        Window::init(hInst, parent);
        _currentState = initState;
    };

	void destroy() {};
	void doDialog(bool isRTL = false) {
		if (isRTL)
		{
			DLGTEMPLATE *pMyDlgTemplate = NULL;
			HGLOBAL hMyDlgTemplate = makeRTLResource(IDD_SHORTCUTMAPPER_DLG, &pMyDlgTemplate);
			::DialogBoxIndirectParam(_hInst, pMyDlgTemplate, _hParent,  dlgProc, (LPARAM)this);
			::GlobalFree(hMyDlgTemplate);
		}
		else
			::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_SHORTCUTMAPPER_DLG), _hParent, dlgProc, (LPARAM)this);
	};
	void getClientRect(RECT & rc) const;
	void translateTab(int index, const TCHAR * newname);

protected :
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	static const int maxTabName = 64;
	BabyGridWrapper _babygrid;
	ContextMenu _rightClickMenu;

	GridState _currentState;
	HWND _hTabCtrl;

	TCHAR tabNames[5][maxTabName];

	void initTabs();
	void initBabyGrid();
	void fillOutBabyGrid();
};

#endif //SHORTCUTMAPPER
