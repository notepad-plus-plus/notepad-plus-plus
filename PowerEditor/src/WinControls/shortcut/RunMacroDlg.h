//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

// created by Daniel Volk mordorpost@volkarts.com

#ifndef RUN_MACRO_DLG_H
#define RUN_MACRO_DLG_H

#ifndef RUN_MACRO_DLG_RC_H
#include "RunMacroDlg_rc.h"
#endif //RUN_MACRO_DLG_RC_H

using namespace std;

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
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

	bool isCheckedOrNot(int checkControlID) const {
		return (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, checkControlID), BM_GETCHECK, 0, 0));
	};

	void check(int);

	int m_Mode;
	int m_Times;
	int m_macroIndex;
};

#endif //RUN_MACRO_DLG_H
