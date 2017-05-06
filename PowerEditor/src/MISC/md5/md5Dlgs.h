//this file is part of notepad++
//Copyright (C)2016 Don HO <don.h@free.fr>
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

#pragma once

#include "StaticDialog.h"


class MD5FromFilesDlg : public StaticDialog
{
public :
	MD5FromFilesDlg() : StaticDialog() {};

	void doDialog(bool isRTL = false);
    virtual void destroy() {};

protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class MD5FromTextDlg : public StaticDialog
{
public :
	MD5FromTextDlg() : StaticDialog() {};

	void doDialog(bool isRTL = false);
    virtual void destroy() {};
	void generateMD5();
	void generateMD5PerLine();

protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

