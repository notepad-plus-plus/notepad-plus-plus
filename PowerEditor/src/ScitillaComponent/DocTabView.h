// This file is part of Notepad++ project
// Copyright (C)2020 Don HO <don.h@free.fr>
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

#include "TabBar.h"
#include "Buffer.h"

const int SAVED_IMG_INDEX = 0;
const int UNSAVED_IMG_INDEX = 1;
const int REDONLY_IMG_INDEX = 2;
const int MONITORING_IMG_INDEX = 3;

class DocTabView : public TabBarPlus
{
public :
	DocTabView():TabBarPlus(), _pView(NULL) {};
	virtual ~DocTabView(){};
	
	virtual void destroy() {
		TabBarPlus::destroy();
	};

	void init(HINSTANCE hInst, HWND parent, ScintillaEditView * pView, std::vector<IconList *> pIconListVector, unsigned char indexChoice) {
		TabBarPlus::init(hInst, parent);
		_pView = pView;

		if (!pIconListVector.empty())
		{
			_pIconListVector = pIconListVector;

			if (indexChoice >= pIconListVector.size())
				_iconListIndexChoice = 0;
			else
				_iconListIndexChoice = indexChoice;
		}

		if (_iconListIndexChoice != -1)
			TabBar::setImageList(_pIconListVector[_iconListIndexChoice]->getHandle());
		return;
	};

	void changeIcons(unsigned char choice) {
		if (choice >= _pIconListVector.size())
			return;
		_iconListIndexChoice = choice;
		TabBar::setImageList(_pIconListVector[_iconListIndexChoice]->getHandle());
	};

	void addBuffer(BufferID buffer);
	void closeBuffer(BufferID buffer);
	void bufferUpdated(Buffer * buffer, int mask);

	bool activateBuffer(BufferID buffer);

	BufferID activeBuffer();
	BufferID findBufferByName(const TCHAR * fullfilename);	//-1 if not found, something else otherwise

	int getIndexByBuffer(BufferID id);
	BufferID getBufferByIndex(size_t index);

	void setBuffer(size_t index, BufferID id);

	static bool setHideTabBarStatus(bool hideOrNot) {
		bool temp = _hideTabBarStatus;
		_hideTabBarStatus = hideOrNot;
		return temp;
	};

	static bool getHideTabBarStatus() {
		return _hideTabBarStatus;
	};

	virtual void reSizeTo(RECT & rc);

	const ScintillaEditView* getScintillaEditView() const {
		return _pView;
	};

private :
	ScintillaEditView *_pView;
	static bool _hideTabBarStatus;

	std::vector<IconList *> _pIconListVector;
	int _iconListIndexChoice = -1;
};
