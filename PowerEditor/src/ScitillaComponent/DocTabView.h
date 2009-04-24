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

#ifndef DOCTABVIEW_H
#define DOCTABVIEW_H

#include "TabBar.h"
#include "ScintillaEditView.h"
#include "ImageListSet.h"

const int SAVED_IMG_INDEX = 0;
const int UNSAVED_IMG_INDEX = 1;
const int REDONLY_IMG_INDEX = 2;

class DocTabView : public TabBarPlus
{
public :
	DocTabView():TabBarPlus(), _pView(NULL) {};
	virtual ~DocTabView(){};
	
	virtual void destroy() {
		TabBarPlus::destroy();
	};

	void init(HINSTANCE hInst, HWND parent, ScintillaEditView * pView, IconList *pIconList = NULL)
	{
		TabBarPlus::init(hInst, parent);
		_pView = pView;
		if (pIconList)
			TabBar::setImageList(pIconList->getHandle());
		return;
	};

	void addBuffer(BufferID buffer);
	void closeBuffer(BufferID buffer);
	void bufferUpdated(Buffer * buffer, int mask);

	bool activateBuffer(BufferID buffer);

	BufferID activeBuffer();
	BufferID findBufferByName(const TCHAR * fullfilename);	//-1 if not found, something else otherwise

	int getIndexByBuffer(BufferID id);
	BufferID getBufferByIndex(int index);

	void setBuffer(int index, BufferID id);

	static bool setHideTabBarStatus(bool hideOrNot) {
		bool temp = _hideTabBarStatus;
		_hideTabBarStatus = hideOrNot;
		return temp;
	};

	static bool getHideTabBarStatus() {
		return _hideTabBarStatus;
	};

	virtual void reSizeTo(RECT & rc) {
		if (_hideTabBarStatus)
		{
			RECT rcTmp = rc;
			
			TabBar::reSizeTo(rcTmp);
			_pView->reSizeTo(rc);
		}
		else
		{
			TabBar::reSizeTo(rc);
			_pView->reSizeTo(rc);
		}
	};

private :
	ScintillaEditView *_pView;
	static bool _hideTabBarStatus;
};

#endif //DOCTABVIEW_H
