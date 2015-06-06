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


#include "RunMacroDlg.h"
#include "ScintillaEditView.h"
#include "Notepad_plus_msgs.h"

void RunMacroDlg::initMacroList()
{
	if (!isCreated()) return;

	NppParameters *pNppParam = NppParameters::getInstance();
	std::vector<MacroShortcut> & macroList = pNppParam->getMacroList();

	::SendDlgItemMessage(_hSelf, IDC_MACRO_COMBO, CB_RESETCONTENT, 0, 0);

	if (::SendMessage(_hParent, WM_GETCURRENTMACROSTATUS, 0, 0) == MACRO_RECORDING_HAS_STOPPED)
		::SendDlgItemMessage(_hSelf, IDC_MACRO_COMBO, CB_ADDSTRING, 0, (LPARAM)TEXT("Current recorded macro"));

	for (size_t i = 0, len = macroList.size(); i < len ; ++i)
		::SendDlgItemMessage(_hSelf, IDC_MACRO_COMBO, CB_ADDSTRING, 0, (LPARAM)macroList[i].getName());

	::SendDlgItemMessage(_hSelf, IDC_MACRO_COMBO, CB_SETCURSEL, 0, 0);
	m_macroIndex = 0;
}

INT_PTR CALLBACK RunMacroDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{	
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			initMacroList();
			::SetDlgItemInt(_hSelf, IDC_M_RUN_TIMES, m_Times, FALSE);
			switch ( m_Mode )
			{
				case RM_RUN_MULTI:
					check(IDC_M_RUN_MULTI);
					break;
				case RM_RUN_EOF:
					check(IDC_M_RUN_EOF);
					break;
			}
			::SendDlgItemMessage(_hSelf, IDC_M_RUN_TIMES, EM_LIMITTEXT, 4, 0);
			goToCenter();

			return TRUE;
		}
		
		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				switch (LOWORD(wParam))
				{	
					case IDC_M_RUN_TIMES:
						check(IDC_M_RUN_MULTI);
						return TRUE;
					
					default:
						return FALSE;
				}
			}
			
			switch (wParam)
			{
				case IDCANCEL :
					::ShowWindow(_hSelf, SW_HIDE);
					return TRUE;

				case IDOK :
					if ( isCheckedOrNot(IDC_M_RUN_MULTI) )
					{
						m_Mode = RM_RUN_MULTI;
						m_Times = ::GetDlgItemInt(_hSelf, IDC_M_RUN_TIMES, NULL, FALSE);
					}
					else if ( isCheckedOrNot(IDC_M_RUN_EOF) )
					{
						m_Mode = RM_RUN_EOF;
					}

					if (::SendDlgItemMessage(_hSelf, IDC_MACRO_COMBO, CB_GETCOUNT, 0, 0))
						::SendMessage(_hParent, WM_MACRODLGRUNMACRO, 0, 0);

					return TRUE;

				default:
					if ((HIWORD(wParam) == CBN_SELCHANGE) && (LOWORD(wParam) == IDC_MACRO_COMBO))
					{
						m_macroIndex = ::SendDlgItemMessage(_hSelf, IDC_MACRO_COMBO, CB_GETCURSEL, 0, 0);
						return TRUE;
					}
			}
		}
	}
	return FALSE;
}

void RunMacroDlg::check(int id)
{
	// IDC_M_RUN_MULTI
	if ( id == IDC_M_RUN_MULTI )
		::SendDlgItemMessage(_hSelf, IDC_M_RUN_MULTI, BM_SETCHECK, BST_CHECKED, 0);
	else
		::SendDlgItemMessage(_hSelf, IDC_M_RUN_MULTI, BM_SETCHECK, BST_UNCHECKED, 0);

	// IDC_M_RUN_EOF
	if ( id == IDC_M_RUN_EOF )
		::SendDlgItemMessage(_hSelf, IDC_M_RUN_EOF, BM_SETCHECK, BST_CHECKED, 0);
	else
		::SendDlgItemMessage(_hSelf, IDC_M_RUN_EOF, BM_SETCHECK, BST_UNCHECKED, 0);
}

int RunMacroDlg::getMacro2Exec() const 
{
	bool isCurMacroPresent = ::SendMessage(_hParent, WM_GETCURRENTMACROSTATUS, 0, 0) == MACRO_RECORDING_HAS_STOPPED;
	return isCurMacroPresent?(m_macroIndex - 1):m_macroIndex;
}
