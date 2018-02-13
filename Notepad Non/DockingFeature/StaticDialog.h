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

#ifndef STATIC_DIALOG_H
#define STATIC_DIALOG_H

//#include "resource.h"
#include "Window.h"
#include "Notepad_plus_msgs.h"

enum PosAlign{ALIGNPOS_LEFT, ALIGNPOS_RIGHT, ALIGNPOS_TOP, ALIGNPOS_BOTTOM};

struct DLGTEMPLATEEX {
      WORD   dlgVer;
      WORD   signature;
      DWORD  helpID;
      DWORD  exStyle;
      DWORD  style; 
      WORD   cDlgItems;
      short  x;
      short  y;    
      short  cx;
      short  cy;
      // The structure has more fields but are variable length
} ;

class StaticDialog : public Window
{
public :
	StaticDialog() : Window() {};
	~StaticDialog(){
		if (isCreated()) {
			::SetWindowLongPtr(_hSelf, GWL_USERDATA, (long)NULL);	//Prevent run_dlgProc from doing anything, since its virtual
			destroy();
		}
	};
	virtual void create(int dialogID, bool isRTL = false);

    virtual bool isCreated() const {
		return (_hSelf != NULL);
	};

	void goToCenter();
    void destroy() {
		::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (WPARAM)_hSelf);
		::DestroyWindow(_hSelf);
	};

protected :
	RECT _rc;
	static BOOL CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) = 0;

    void alignWith(HWND handle, HWND handle2Align, PosAlign pos, POINT & point);
	HGLOBAL makeRTLResource(int dialogID, DLGTEMPLATE **ppMyDlgTemplate);
};

#endif //STATIC_DIALOG_H
