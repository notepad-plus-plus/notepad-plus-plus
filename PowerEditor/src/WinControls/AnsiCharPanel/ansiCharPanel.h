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

#include <windows.h>
#include <commctrl.h>

#include "DockingDlgInterface.h"
#include "ansiCharPanel_rc.h"
#include "ListView.h"
#include "asciiListView.h"

#define AI_PROJECTPANELTITLE		TEXT("ASCII Codes Insertion Panel")

class ScintillaEditView;

class AnsiCharPanel : public DockingDlgInterface {
public:
	AnsiCharPanel(): DockingDlgInterface(IDD_ANSIASCII_PANEL) {};

	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		DockingDlgInterface::init(hInst, hPere);
		_ppEditView = ppEditView;
	};

    void setParent(HWND parent2set){
        _hParent = parent2set;
    };

	void switchEncoding();
	void insertChar(unsigned char char2insert) const;
	void insertString(LPWSTR string2insert) const;

	virtual void setBackgroundColor(int bgColour) const {
		ListView_SetBkColor(_listView.getHSelf(), bgColour);
		ListView_SetTextBkColor(_listView.getHSelf(), bgColour);
		_listView.redraw(true);
    };
	virtual void setForegroundColor(int fgColour) const {
		ListView_SetTextColor(_listView.getHSelf(), fgColour);
		_listView.redraw(true);
    };
	
protected:
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	ScintillaEditView **_ppEditView = nullptr;
	AsciiListView _listView;
};

