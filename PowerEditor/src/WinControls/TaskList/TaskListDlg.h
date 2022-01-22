// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


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
	TaskLstFnStatus(const generic_string& str, int status) : _fn(str), _status(status){};
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

static LRESULT CALLBACK hookProc(int nCode, WPARAM wParam, LPARAM lParam);

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
	intptr_t CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);

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
