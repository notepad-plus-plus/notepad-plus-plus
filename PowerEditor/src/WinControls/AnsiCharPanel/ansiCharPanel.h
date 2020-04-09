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
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	ScintillaEditView **_ppEditView = nullptr;
	AsciiListView _listView;
};

