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
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

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
	LONG _clientWidth = 0;
	LONG _clientHeight = 0;
	LONG _initClientWidth = 0;
	LONG _initClientHeight = 0;
	bool _dialogInitDone = false;

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

