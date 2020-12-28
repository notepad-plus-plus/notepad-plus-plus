// This file is part of Notepad++ project
// Copyright (C)2020 Don HO <don.h@free.fr>
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


#pragma once

#include "BabyGridWrapper.h"
#include "ShortcutMapper_rc.h"
#include "shortcut.h"
#include "ContextMenu.h"

enum GridState {STATE_MENU, STATE_MACRO, STATE_USER, STATE_PLUGIN, STATE_SCINTILLA};

class ShortcutMapper : public StaticDialog {
public:
	ShortcutMapper() : _currentState(STATE_MENU), StaticDialog() {
		_shortcutFilter = TEXT("");
		_dialogInitDone = false;
	};
	~ShortcutMapper() = default;

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
			::DialogBoxIndirectParam(_hInst, pMyDlgTemplate, _hParent, dlgProc, reinterpret_cast<LPARAM>(this));
			::GlobalFree(hMyDlgTemplate);
		}
		else
			::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_SHORTCUTMAPPER_DLG), _hParent, dlgProc, reinterpret_cast<LPARAM>(this));
	};
	void getClientRect(RECT & rc) const;

	bool findKeyConflicts(__inout_opt generic_string * const keyConflictLocation,
							const KeyCombo & itemKeyCombo, const size_t & itemIndex) const;

	generic_string getTextFromCombo(HWND hCombo);
	bool isFilterValid(Shortcut);
	bool isFilterValid(PluginCmdShortcut sc);

protected :
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	BabyGridWrapper _babygrid;
	ContextMenu _rightClickMenu;

	GridState _currentState;
	HWND _hTabCtrl = nullptr;

	const static int _nbTab = 5;
	generic_string _tabNames[_nbTab];
	generic_string _shortcutFilter;
	std::vector<size_t> _shortcutIndex;

	//save/restore the last view
	std::vector<size_t> _lastHomeRow;
	std::vector<size_t> _lastCursorRow;

	generic_string _conflictInfoOk;
	generic_string _conflictInfoEditing;

	std::vector<HFONT> _hGridFonts;

	enum GridFonts : uint_fast8_t
	{
		GFONT_HEADER,
		GFONT_ROWS,
		MAX_GRID_FONTS
	};
	LONG _clientWidth;
	LONG _clientHeight;
	LONG _initClientWidth;
	LONG _initClientHeight;
	bool _dialogInitDone;

	void initTabs();
	void initBabyGrid();
	void fillOutBabyGrid();
	generic_string getTabString(size_t i) const;

	bool isConflict(const KeyCombo & lhs, const KeyCombo & rhs) const
	{
		return ( (lhs._isCtrl  == rhs._isCtrl ) &&
				 (lhs._isAlt   == rhs._isAlt  ) &&
				 (lhs._isShift == rhs._isShift) &&
				 (lhs._key	   == rhs._key	  ) );
	}
};

