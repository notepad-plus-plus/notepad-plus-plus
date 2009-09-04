/*
this file is part of notepad++
Copyright (C)2003 Don HO <donho@altern.org>

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

#ifndef SIZABLE_DLG_H
#define SIZABLE_DLG_H

#ifndef WINDOWS_DLG_RC_H
#include "WindowsDlgRc.h"
#endif //WINDOWS_DLG_RC_H

#ifndef WINMGR_H
#include "WinMgr.h"
#endif //WINMGR_H

class SizeableDlg : public StaticDialog {
	typedef StaticDialog MyBaseClass;
public:
	SizeableDlg(WINRECT* pWinMap);

protected:
	CWinMgr _winMgr;	  // window manager

	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL onInitDialog();
	virtual void onSize(UINT nType, int cx, int cy);
	virtual void onGetMinMaxInfo(MINMAXINFO* lpMMI);
	virtual LRESULT onWinMgr(WPARAM wp, LPARAM lp);
};

#endif //SIZABLE_DLG_H
