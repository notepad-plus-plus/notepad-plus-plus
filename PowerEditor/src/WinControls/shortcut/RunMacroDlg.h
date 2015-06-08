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

// created by Daniel Volk mordorpost@volkarts.com

#ifndef RUN_MACRO_DLG_H
#define RUN_MACRO_DLG_H

#ifndef RUN_MACRO_DLG_RC_H
#include "RunMacroDlg_rc.h"
#endif //RUN_MACRO_DLG_RC_H

#include "StaticDialog.h"

#define RM_CANCEL -1
#define RM_RUN_MULTI 1
#define RM_RUN_EOF 2

class RunMacroDlg : public StaticDialog
{
public :
	RunMacroDlg() : StaticDialog(), m_Mode(RM_RUN_MULTI), m_Times(1) {};
	~RunMacroDlg() {
	};

	void init(HINSTANCE hInst, HWND hPere/*, ScintillaEditView **ppEditView*/) {
		Window::init(hInst, hPere);
	};

	void doDialog(bool isRTL = false) {
		if (!isCreated())
			create(IDD_RUN_MACRO_DLG, isRTL);
		else
			::ShowWindow(_hSelf, SW_SHOW);
	};

	void initMacroList();

	int getMode() const {return m_Mode;};
	int getTimes() const {return m_Times;};
	int getMacro2Exec() const;

private :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void check(int);

	int m_Mode;
	int m_Times;
	int m_macroIndex;
};

#endif //RUN_MACRO_DLG_H
