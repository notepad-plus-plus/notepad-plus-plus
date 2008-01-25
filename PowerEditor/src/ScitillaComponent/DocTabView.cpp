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

#include "DocTabView.h"

#ifndef _WIN32_IE
#define _WIN32_IE	0x0600
#endif //_WIN32_IE

#include <commctrl.h>
#include <shlwapi.h>

unsigned short DocTabView::_nbNewTitle = 0;


// return the index if fn is found in DocTabView
// otherwise -1
int DocTabView::find(const char *fn) const
{
	return _pView->findDocIndexByName(fn);
}

char * DocTabView::newDocInit()
{
	// create the new entry for this doc
	char * newTitle = _pView->attatchDefaultDoc(_nbNewTitle++);

	// create a new (the first) sub tab then hightlight it
	TabBar::insertAtEnd(newTitle);
	return newTitle;
}

const char * DocTabView::newDoc(const char *fn)
{
	char *completName;
	if ((!fn) || (!strcmp(fn, "")))
		completName = _pView->createNewDoc(_nbNewTitle++);
	else
		completName = _pView->createNewDoc(fn);
	// for the title of sub tab
	fn = PathFindFileName(completName);
	char fnConformToTab[MAX_PATH];

	for (int i = 0, j = 0 ; ; i++)
	{
		fnConformToTab[j++] = fn[i];
		if (fn[i] == '&')
			fnConformToTab[j++] = '&';
		if (fn[i] == '\0')
			break;
	}
	TabBar::insertAtEnd(fnConformToTab);
	TabBar::activateAt(_nbItem - 1);

	if (_isMultiLine)
	{
		::SendMessage(_hParent, WM_SIZE, 0, 0);
	}
	return (const char *)completName;
}

const char * DocTabView::newDoc(Buffer & buf)
{
    const char *completName = buf.getFileName();
    int i = _pView->addBuffer(buf);
    _pView->activateDocAt(i);

	// for the title of sub tab
	TabBar::insertAtEnd(PathFindFileName(completName));
	TabBar::activateAt(_nbItem - 1);
	
	if (_isMultiLine)
	{
		::SendMessage(_hParent, WM_SIZE, 0, 0);
	}
	return completName;
}

//! \brief this method activates the doc and the corresponding sub tab
//! \brief return the index of previeus current doc
char * DocTabView::activate(int index)
{
	TabBar::activateAt(index);
	return _pView->activateDocAt(index);
}

// this method updates the doc when user clicks a sub tab
// return Null if the user clicks on an active sub tab,
// otherwize the name of new activated doc
char * DocTabView::clickedUpdate()
{
	int indexClicked = int(::SendMessage(_hSelf, TCM_GETCURSEL, 0, 0));
	if (indexClicked == _pView->getCurrentDocIndex()) return NULL;

	return _pView->activateDocAt(indexClicked);
}

const char * DocTabView::closeCurrentDoc()
{
	if (_nbItem == 1)
	{
        newDoc();
        closeDocAt(0);
	}
	else
	{
		int i2activate;
		int i2close = _pView->closeCurrentDoc(i2activate);

		TabBar::deletItemAt(i2close);

		if (i2activate > 1)
			TabBar::activateAt(i2activate-1);

		TabBar::activateAt(i2activate);
	}

	if (_isMultiLine)
	{
		::SendMessage(_hParent, WM_SIZE, 0, 0);
	}

	return _pView->getCurrentTitle();
}

const char * DocTabView::closeAllDocs()
{
	_pView->removeAllUnusedDocs();
	TabBar::deletAllItem();
	_nbNewTitle = 0;
	newDocInit();
	return _pView->getCurrentTitle();
}

void DocTabView::closeDocAt(int index2Close)
{
    _pView->closeDocAt(index2Close);
    TabBar::deletItemAt(index2Close);
}

void DocTabView::updateCurrentTabItem(const char *title)
{
	int currentIndex = TabCtrl_GetCurSel(_hSelf);
    
    updateTabItem(currentIndex, title);
}

void DocTabView::updateTabItem(int index, const char *title)
{
    char str[MAX_PATH];
    TCITEM tie;
	tie.mask = TCIF_TEXT | TCIF_IMAGE;
	tie.pszText = str;
	tie.cchTextMax = (sizeof(str)-1);

	TabCtrl_GetItem(_hSelf, index, &tie);
	if ((title)&&(strcmp(title, "")))
		tie.pszText = (char *)title;

	bool isDirty = (_pView->getBufferAt(index)).isDirty();//isCurrentBufReadOnly();
	bool isReadOnly = (_pView->getBufferAt(index)).isReadOnly();//getCurrentDocStat();
	tie.iImage = isReadOnly?REDONLY_IMG_INDEX:(isDirty?UNSAVED_IMG_INDEX:SAVED_IMG_INDEX);
	TabCtrl_SetItem(_hSelf, index, &tie);
}
