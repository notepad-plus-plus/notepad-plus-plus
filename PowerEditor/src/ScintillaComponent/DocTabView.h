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
	ScintillaEditView *_pView = nullptr;
	static bool _hideTabBarStatus;

	std::vector<IconList *> _pIconListVector;
	int _iconListIndexChoice = -1;
};
