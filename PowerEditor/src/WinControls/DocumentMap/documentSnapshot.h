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

#include "documentSnapshot_rc.h"
#include "StaticDialog.h"

class ScintillaEditView;
class Buffer;
struct MapPosition;


class DocumentPeeker : public StaticDialog {
public:
	DocumentPeeker() = default;

	void init(HINSTANCE hInst, HWND hPere) {
		Window::init(hInst, hPere);
	};

	void doDialog(POINT p, Buffer *buf, ScintillaEditView & scintSource);
	void syncDisplay(Buffer *buf, ScintillaEditView & scintSource);

    void setParent(HWND parent2set){
        _hParent = parent2set;
    };

	void scrollSnapshotWith(const MapPosition & mapPos, int textZoneWidth);
	void saveCurrentSnapshot(ScintillaEditView & editView);

protected:
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void goTo(POINT p);

private:
	ScintillaEditView *_pPeekerView = nullptr;
};
