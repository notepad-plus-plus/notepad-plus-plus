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

#ifndef TASKLISTDLG_H
#define TASKLISTDLG_H

#ifndef TASKLISTDLGRC_H
#include "TaskListDlg_rc.h"
#endif //TASKLISTDLGRC_H

#ifndef TASKLIST_H
#include "TaskList.h"
#endif //TASKLIST_H
/*
#ifndef IMAGE_LIST_H
#include "ImageListSet.h"
#endif //IMAGE_LIST_H
*/
#ifndef NOTEPAD_PLUS_MSGS_H
#include "Notepad_plus_msgs.h"
#endif //NOTEPAD_PLUS_MSGS_H

#define	TASKLIST_USER    (WM_USER + 8000)
	#define WM_GETTASKLISTINFO (TASKLIST_USER + 01)

struct TaskLstFnStatus {
	int _iView;
	int _docIndex;
	generic_string _fn;
	int _status;
	void *_bufID;
	TaskLstFnStatus(generic_string str, int status) : _fn(str), _status(status){};
	TaskLstFnStatus(int iView, int docIndex, generic_string str, int status, void *bufID) : 
	_iView(iView), _docIndex(docIndex), _fn(str), _status(status), _bufID(bufID) {};
};

struct TaskListInfo {
	std::vector<TaskLstFnStatus> _tlfsLst;
	int _currentIndex;
};

static HWND hWndServer = NULL;
static HHOOK hook = NULL;

static LRESULT CALLBACK hookProc(UINT nCode, WPARAM wParam, LPARAM lParam);

class TaskListDlg : public StaticDialog
{
public :	
        TaskListDlg() : StaticDialog() {};
		void init(HINSTANCE hInst, HWND parent, HIMAGELIST hImgLst, bool dir) {
            Window::init(hInst, parent);
			_hImalist = hImgLst;
			_initDir = dir;
        };
        int doDialog(bool isRTL = false);
		virtual void destroy() {};

protected :
	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);

private :
	TaskList _taskList;
	TaskListInfo _taskListInfo;
	HIMAGELIST _hImalist;
	bool _initDir;
	HHOOK _hHooker;

	void drawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
};


#endif // TASKLISTDLG_H
