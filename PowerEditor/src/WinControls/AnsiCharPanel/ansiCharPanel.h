/*
this file is part of notepad++
Copyright (C)2003 Don HO ( donho@altern.org )

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a Copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef ANSICHARPANEL_H
#define  ANSICHARPANEL_H

//#include <windows.h>
#ifndef DOCKINGDLGINTERFACE_H
#include "DockingDlgInterface.h"
#endif //DOCKINGDLGINTERFACE_H

#include "ansiCharPanel_rc.h"
#include "ListView.h"

class ScintillaEditView;

class AnsiCharPanel : public DockingDlgInterface {
public:
	AnsiCharPanel(): DockingDlgInterface(IDD_ANSIASCII_PANEL) {};

	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		DockingDlgInterface::init(hInst, hPere);
		_ppEditView = ppEditView;
	};

    virtual void display(bool toShow = true) const {
        DockingDlgInterface::display(toShow);
    };

    void setParent(HWND parent2set){
        _hParent = parent2set;
    };

	void switchEncoding();
	void insertChar(unsigned char char2insert) const;

protected:
	virtual BOOL CALLBACK AnsiCharPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	ScintillaEditView **_ppEditView;
	ListView _listView;
};
#endif // ANSICHARPANEL_H
