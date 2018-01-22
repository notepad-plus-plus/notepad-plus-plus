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


#pragma once

#include "Common.h"
#include "StaticDialog.h"
#include "TaskListDlg_rc.h"
#include "TaskList.h"
#include "Notepad_plus_msgs.h"

#define	TASKLIST_USER    (WM_USER + 8000)
#define WM_GETTASKLISTINFO (TASKLIST_USER + 01)

struct TaskLstFnStatus {
	int _iView = -1;
	int _docIndex = 0;
	generic_string _fn;
	int _status = 0;
	void *_bufID = nullptr;
	TaskLstFnStatus(generic_string str, int status) : _fn(str), _status(status){};
	TaskLstFnStatus(int iView, int docIndex, generic_string str, int status, void *bufID) : 
	_iView(iView), _docIndex(docIndex), _fn(str), _status(status), _bufID(bufID) {};
};

struct TaskListInfo {
	std::vector<TaskLstFnStatus> _tlfsLst;
	int _currentIndex = -1;
};

static HWND hWndServer = NULL;
static HHOOK hook = NULL;
static winVer windowsVersion = WV_UNKNOWN;

static LRESULT CALLBACK hookProc(UINT nCode, WPARAM wParam, LPARAM lParam);

class TaskListDlg : public StaticDialog
{
public :
		TaskListDlg() : StaticDialog() { _instanceCount++; };
		void init(HINSTANCE hInst, HWND parent, HIMAGELIST hImgLst, bool dir) {
            Window::init(hInst, parent);
			_hImalist = hImgLst;
			_initDir = dir;
        };
        int doDialog(bool isRTL = false);
		virtual void destroy() {};

protected :
	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);

private :
	TaskList _taskList;
	TaskListInfo _taskListInfo;
	HIMAGELIST _hImalist = nullptr;
	bool _initDir = false;
	HHOOK _hHooker = nullptr;

	void drawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
public:
	static int _instanceCount;
};

