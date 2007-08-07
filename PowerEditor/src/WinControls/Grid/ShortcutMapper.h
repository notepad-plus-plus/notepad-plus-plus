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

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef SHORTCUTMAPPER
#define SHORTCUTMAPPER

#include "BabyGridWrapper.h"
#include "ShortcutMapper_rc.h"
#include "shortcut.h"
#include "ContextMenu.h"

class ShortcutMapper : public StaticDialog {
public:
	ShortcutMapper() : StaticDialog() {};
	~ShortcutMapper() {};
	//void init(HINSTANCE hInst, HWND parent) {};
	void destroy() {};
	void doDialog(bool isRTL = false) {
		if (isRTL)
		{
			DLGTEMPLATE *pMyDlgTemplate = NULL;
			HGLOBAL hMyDlgTemplate = makeRTLResource(IDD_SHORTCUTMAPPER_DLG, &pMyDlgTemplate);
			::DialogBoxIndirectParam(_hInst, pMyDlgTemplate, _hParent,  (DLGPROC)dlgProc, (LPARAM)this);
			::GlobalFree(hMyDlgTemplate);
		}
		else
			::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_SHORTCUTMAPPER_DLG), _hParent, (DLGPROC)dlgProc, (LPARAM)this);
	};
	void getClientRect(RECT & rc) const {
		Window::getClientRect(rc);
		rc.bottom -= 20;
	};

protected :
	BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	BabyGridWrapper _babygrid;
	Accelerator *_pAccel;
	ContextMenu _rightClickMenu;

	void initBabyGrid();
	void fillOutBabyGrid();
};

#endif //SHORTCUTMAPPER
