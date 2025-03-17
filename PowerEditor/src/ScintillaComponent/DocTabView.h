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
	
	void destroy() override {
		TabBarPlus::destroy();
	};

	void init(HINSTANCE hInst, HWND parent, ScintillaEditView * pView, unsigned char indexChoice, unsigned char buttonsStatus);

	void createIconSets();

	void changeIconSet(unsigned char choice) {
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
	BufferID findBufferByName(const wchar_t * fullfilename);	//-1 if not found, something else otherwise

	int getIndexByBuffer(BufferID id);
	BufferID getBufferByIndex(size_t index);

	void setBuffer(size_t index, BufferID id);

	void reSizeTo(RECT & rc) override;

	void resizeIconsDpi() {
		UINT newSize = dpiManager().scale(g_TabIconSize);
		for (const auto& i : _pIconListVector)
		{
			ImageList_SetIconSize(i->getHandle(), newSize, newSize);
		}

		createIconSets();

		if (_iconListIndexChoice < 0 || static_cast<size_t>(_iconListIndexChoice) >= _pIconListVector.size())
			_iconListIndexChoice = 0;

		TabBar::setImageList(_pIconListVector[_iconListIndexChoice]->getHandle());
	};

	const ScintillaEditView* getScintillaEditView() const {
		return _pView;
	};

	void setIndividualTabColour(BufferID bufferId, int colorId);
	int getIndividualTabColourId(int tabIndex) override;
	
	HIMAGELIST getImgLst(UINT index) {
		if (index >= _pIconListVector.size())
			index = 0;
		return _pIconListVector[index]->getHandle();
	};

private :
	ScintillaEditView *_pView = nullptr;

	IconList _docTabIconList;
	IconList _docTabIconListAlt;
	IconList _docTabIconListDarkMode;

	std::vector<IconList *> _pIconListVector;
	int _iconListIndexChoice = -1;
};
