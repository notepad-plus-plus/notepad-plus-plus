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
	ShortcutMapper() : StaticDialog(), _currentState(STATE_MENU) {
		_shortcutFilter = std::vector<std::wstring>();
		_dialogInitDone = false;
	};
	~ShortcutMapper() = default;

	void init(HINSTANCE hInst, HWND parent, GridState initState = STATE_MENU) {
		Window::init(hInst, parent);
		_currentState = initState;
	};

	void destroy() override {};
	void doDialog(bool isRTL = false) {
		StaticDialog::myCreateDialogBoxIndirectParam(IDD_SHORTCUTMAPPER_DLG, isRTL);
	}
	void getClientRect(RECT & rc) const override;

	bool findKeyConflicts(__inout_opt std::wstring * const keyConflictLocation,
							const KeyCombo & itemKeyCombo, const size_t & itemIndex) const;

	std::wstring getTextFromCombo(HWND hCombo);
	bool isFilterValid(Shortcut sc);
	bool isFilterValid(PluginCmdShortcut sc);
	bool isFilterValid(ScintillaKeyMap sc);

protected:
	void resizeDialogElements();
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
	BabyGridWrapper _babygrid;
	ContextMenu _rightClickMenu;

	GridState _currentState;
	HWND _hTabCtrl = nullptr;

	const static int _nbTab = 5;
	std::wstring _tabNames[_nbTab];
	std::vector<std::wstring> _shortcutFilter;
	std::vector<size_t> _shortcutIndex;

	//save/restore the last view
	std::vector<size_t> _lastHomeRow;
	std::vector<size_t> _lastCursorRow;

	std::wstring _conflictInfoOk;
	std::wstring _conflictInfoEditing;

	std::vector<HFONT> _hGridFonts;

	enum GridFonts : uint_fast8_t
	{
		GFONT_HEADER,
		GFONT_ROWS,
		MAX_GRID_FONTS
	};

	SIZE _szMinDialog{};
	SIZE _szBorder{};
	bool _dialogInitDone = false;

	void initTabs();
	void initBabyGrid();
	void fillOutBabyGrid();
	std::wstring getTabString(size_t i) const;

	bool isConflict(const KeyCombo & lhs, const KeyCombo & rhs) const
	{
		return ( (lhs._isCtrl  == rhs._isCtrl ) &&
				 (lhs._isAlt   == rhs._isAlt  ) &&
				 (lhs._isShift == rhs._isShift) &&
				 (lhs._key	   == rhs._key	  ) );
	}
};
