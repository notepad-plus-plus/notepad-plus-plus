// This file is part of Notepad++ project
// Copyright (C)2003-2017 Don HO <don.h@free.fr>
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

#include "documentSnapshot_rc.h"
#include "StaticDialog.h"

class ScintillaEditView;
class Buffer;
struct MapPosition;


class DocumentPeeker : public StaticDialog {
public:
	DocumentPeeker(): StaticDialog() {};

	void init(HINSTANCE hInst, HWND hPere) {
		Window::init(hInst, hPere);
	};

	void doDialog(POINT p, Buffer *buf, ScintillaEditView & scintSource);
	
	void syncDisplay(Buffer *buf, ScintillaEditView & scintSource);


    void setParent(HWND parent2set){
        _hParent = parent2set;
    };

	void scrollSnapshotWith(const MapPosition & mapPos);
	void saveCurrentSnapshot(ScintillaEditView & editView);

protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void goTo(POINT p);

private:
	ScintillaEditView *_pPeekerView = nullptr;
};
