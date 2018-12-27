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

enum hashType {hash_md5, hash_sha256};

class HashFromFilesDlg : public StaticDialog
{
public :
	HashFromFilesDlg() : StaticDialog() {};

	void doDialog(bool isRTL = false);
    virtual void destroy() {};
	void setHashType(hashType hashType2set);

protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	hashType _ht = hash_md5;
};

class HashFromTextDlg : public StaticDialog
{
public :
	HashFromTextDlg() : StaticDialog() {};

	void doDialog(bool isRTL = false);
    virtual void destroy() {};
	void generateHash();
	void generateHashPerLine();
	void setHashType(hashType hashType2set);

protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	hashType _ht = hash_md5;
};

