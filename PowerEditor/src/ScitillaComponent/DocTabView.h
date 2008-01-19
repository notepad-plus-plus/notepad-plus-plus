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
	DocTabView():TabBarPlus(), _pView(NULL), _hideTabBarStatus(false){};
	virtual ~DocTabView(){};
	
	virtual void destroy() {
		TabBarPlus::destroy();
	};

	char * init(HINSTANCE hInst, HWND parent, ScintillaEditView *pView, IconList *pIconList = NULL)
	{
		TabBarPlus::init(hInst, parent);
		_pView = pView;

		if (pIconList)
			TabBar::setImageList(pIconList->getHandle());
		return newDocInit();
	};

	char * newDocInit();
	int find(const char *) const;
	char * activate(int index);
	
    const char * newDoc(const char *fn = NULL);
    const char * newDoc(Buffer & buf);

	char * clickedUpdate();

	const char * closeCurrentDoc();
	const char * closeAllDocs();
    void closeDocAt(int index);

	//void setCurrentTabItem(const char *title, bool isDirty);
	void updateCurrentTabItem(const char *title = NULL);
    void updateTabItem(int index, const char *title = NULL);

	bool setHideTabBarStatus(bool hideOrNot){
		bool temp = _hideTabBarStatus;
		_hideTabBarStatus = hideOrNot;
		return temp;
	};

	bool getHideTabBarStatus() const {
		return _hideTabBarStatus;
	};

	virtual void reSizeTo(RECT & rc) {
		if (!_hideTabBarStatus)
		{
			TabBar::reSizeTo(rc);
			//rc.top += 2;
			//rc.right -= 4;
			//rc.bottom -= 26;
		}

		_pView->reSizeTo(rc);
	};

private :
	static unsigned short _nbNewTitle;
	ScintillaEditView *_pView;
	bool _hideTabBarStatus;
};

#endif //DOCTABVIEW_H
