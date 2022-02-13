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

#include "DockingDlgInterface.h"
#include "clipboardHistoryPanel_rc.h"
#include <vector>

#define CH_PROJECTPANELTITLE		TEXT("Clipboard History")

typedef std::vector<unsigned char> ClipboardData;

class ScintillaEditView;

class ByteArray {
public:
	ByteArray() = default;
	explicit ByteArray(ClipboardData cd);
	
	~ByteArray() {
		if (_pBytes)
			delete [] _pBytes;
	};
	const unsigned char * getPointer() const {return _pBytes;};
	size_t getLength() const {return _length;};
protected:
	unsigned char *_pBytes = nullptr;
	size_t _length = 0;
};

class StringArray : public ByteArray {
public:
	StringArray(ClipboardData cd, size_t maxLen);
};

class ClipboardHistoryPanel : public DockingDlgInterface {
public:
	ClipboardHistoryPanel(): DockingDlgInterface(IDD_CLIPBOARDHISTORY_PANEL), _ppEditView(NULL), _hwndNextCbViewer(NULL), _lbBgColor(-1), _lbFgColor(-1) {};

	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		DockingDlgInterface::init(hInst, hPere);
		_ppEditView = ppEditView;
	};

    void setParent(HWND parent2set){
        _hParent = parent2set;
    };

	ClipboardData getClipboadData();
	void addToClipboadHistory(ClipboardData cbd);
	int getClipboardDataIndex(ClipboardData cbd);

	virtual void setBackgroundColor(COLORREF bgColour) {
		_lbBgColor = bgColour;
    };
	virtual void setForegroundColor(COLORREF fgColour) {
		_lbFgColor = fgColour;
    };

	void drawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

protected:
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	ScintillaEditView **_ppEditView = nullptr;
	std::vector<ClipboardData> _clipboardDataVector;
	HWND _hwndNextCbViewer = nullptr;
	int _lbBgColor = -1;
	int _lbFgColor= -1;

};

