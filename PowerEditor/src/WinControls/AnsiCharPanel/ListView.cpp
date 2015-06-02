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



#include "ListView.h"
#include "Parameters.h"
#include "localization.h"

void ListView::init(HINSTANCE hInst, HWND parent)
{
	Window::init(hInst, parent);
    INITCOMMONCONTROLSEX icex;
    
    // Ensure that the common control DLL is loaded. 
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);
    
    // Create the list-view window in report view with label editing enabled.
	int listViewStyles = LVS_REPORT | LVS_NOSORTHEADER\
						| LVS_SINGLESEL | LVS_AUTOARRANGE\
						| LVS_SHAREIMAGELISTS | LVS_SHOWSELALWAYS;

	_hSelf = ::CreateWindow(WC_LISTVIEW, 
                                TEXT(""), 
                                WS_CHILD | listViewStyles,
                                0,
                                0, 
                                0,
                                0,
                                _hParent, 
                                (HMENU) NULL, 
                                hInst,
                                NULL);
	if (!_hSelf)
	{
		throw std::runtime_error("ListView::init : CreateWindowEx() function return null");
	}

	::SetWindowLongPtr(_hSelf, GWLP_USERDATA, (LONG_PTR)this);
	_defaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSelf, GWLP_WNDPROC, (LONG_PTR)staticProc));

	DWORD exStyle = ListView_GetExtendedListViewStyle(_hSelf);
	exStyle |= LVS_EX_FULLROWSELECT | LVS_EX_BORDERSELECT ;
	ListView_SetExtendedListViewStyle(_hSelf, exStyle);

	LVCOLUMN lvColumn;
	lvColumn.mask = LVCF_TEXT|LVCF_WIDTH;

	NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance())->getNativeLangSpeaker();
	generic_string valStr = pNativeSpeaker->getAttrNameStr(TEXT("Value"), "AsciiInsertion", "ColumnVal");
	generic_string hexStr = pNativeSpeaker->getAttrNameStr(TEXT("Hex"), "AsciiInsertion", "ColumnHex");
	generic_string charStr = pNativeSpeaker->getAttrNameStr(TEXT("Character"), "AsciiInsertion", "ColumnChar");

	lvColumn.cx = 45;
	lvColumn.pszText = (TCHAR *)valStr.c_str();
	ListView_InsertColumn(_hSelf, 0, &lvColumn);
	
	lvColumn.cx = 45;
	lvColumn.pszText = (TCHAR *)hexStr.c_str();
	ListView_InsertColumn(_hSelf, 1, &lvColumn);

	lvColumn.cx = 70;
	lvColumn.pszText = (TCHAR *)charStr.c_str();
	ListView_InsertColumn(_hSelf, 2, &lvColumn);
}

void ListView::resetValues(int codepage)
{
	if (codepage == -1)
		codepage = 0;

	if (_codepage == codepage)
		return;

	ListView_DeleteAllItems(_hSelf);
	setValues(codepage);
}

generic_string ListView::getAscii(unsigned char value)
{
	switch (value)
	{
		case 0:
			return TEXT("NULL");
		case 1:
			return TEXT("SOH");
		case 2:
			return TEXT("STX");
		case 3:
			return TEXT("ETX");
		case 4:
			return TEXT("EOT");
		case 5:
			return TEXT("ENQ");
		case 6:
			return TEXT("ACK");
		case 7:
			return TEXT("BEL");
		case 8:
			return TEXT("BS");
		case 9:
			return TEXT("TAB");
		case 10:
			return TEXT("LF");
		case 11:
			return TEXT("VT");
		case 12:
			return TEXT("FF");
		case 13:
			return TEXT("CR");
		case 14:
			return TEXT("SO");
		case 15:
			return TEXT("SI");
		case 16:
			return TEXT("DLE");
		case 17:
			return TEXT("DC1");
		case 18:
			return TEXT("DC2");
		case 19:
			return TEXT("DC3");
		case 20:
			return TEXT("DC4");
		case 21:
			return TEXT("NAK");
		case 22:
			return TEXT("SYN");
		case 23:
			return TEXT("ETB");
		case 24:
			return TEXT("CAN");
		case 25:
			return TEXT("EM");
		case 26:
			return TEXT("SUB");
		case 27:
			return TEXT("ESC");
		case 28:
			return TEXT("FS");
		case 29:
			return TEXT("GS");
		case 30:
			return TEXT("RS");
		case 31:
			return TEXT("US");
		case 32:
			return TEXT("Space");
		case 127:
			return TEXT("DEL");
		default:
		{
			TCHAR charStr[10];
			char ascii[2];
			ascii[0] = value;
			ascii[1] = '\0';
			MultiByteToWideChar(_codepage, 0, ascii, -1, charStr, sizeof(charStr));
			return charStr;
		}

	}
}

void ListView::setValues(int codepage)
{
	_codepage = codepage;
	
	for (int i = 0 ; i < 256 ; ++i)
	{
		LVITEM item;
		item.mask = LVIF_TEXT;
		TCHAR dec[8];
		TCHAR hex[8];
		generic_sprintf(dec, TEXT("%d"), i);
		generic_sprintf(hex, TEXT("%02X"), i);
		item.pszText = dec;
		item.iItem = i;
		item.iSubItem = 0;
		ListView_InsertItem(_hSelf, &item);

		ListView_SetItemText(_hSelf, i, 1, (LPTSTR)hex);

		generic_string s = getAscii((unsigned char)i);
		ListView_SetItemText(_hSelf, i, 2, (LPTSTR)s.c_str());
	}
}


void ListView::destroy()
{
	::DestroyWindow(_hSelf);
	_hSelf = NULL;
}


LRESULT ListView::runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	return ::CallWindowProc(_defaultProc, hwnd, Message, wParam, lParam);
}

