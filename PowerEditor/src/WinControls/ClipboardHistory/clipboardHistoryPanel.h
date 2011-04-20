/*
this file is part of notepad++
Copyright (C)2003 Don HO ( donho@altern.org )

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a Copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef CLIPBOARDHISTORYPANEL_H
#define CLIPBOARDHISTORYPANEL_H

//#include <windows.h>
#ifndef DOCKINGDLGINTERFACE_H
#include "DockingDlgInterface.h"
#endif //DOCKINGDLGINTERFACE_H

#include "clipboardHistoryPanel_rc.h"
#include <vector>

typedef std::vector<unsigned char> ClipboardData;

class ScintillaEditView;

class ByteArray {
public:
	ByteArray():_pBytes(NULL), _length(0) {};
	ByteArray(ClipboardData cd);
	
	~ByteArray() {
		if (_pBytes)
			delete [] _pBytes;
		_pBytes = NULL;
		_length = 0;
	};
	const unsigned char * getPointer() const {return _pBytes;};
	size_t getLength() const {return _length;};
protected:
	unsigned char *_pBytes;
	size_t _length;
};

class StringArray : public ByteArray {
public:
	StringArray(ClipboardData cd, size_t maxLen);
};

class ClipboardHistoryPanel : public DockingDlgInterface {
public:
	ClipboardHistoryPanel(): DockingDlgInterface(IDD_CLIPBOARDHISTORY_PANEL) {};

	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		DockingDlgInterface::init(hInst, hPere);
		_ppEditView = ppEditView;
	};

    virtual void display(bool toShow = true) const {
        DockingDlgInterface::display(toShow);
    };

    void setParent(HWND parent2set){
        _hParent = parent2set;
    };

	void switchEncoding();
	ClipboardData getClipboadData();
	void addToClipboadHistory(ClipboardData cbd);

protected:
	virtual BOOL CALLBACK ClipboardHistoryPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	ScintillaEditView **_ppEditView;
	std::vector<ClipboardData> _clipboardDataVector;
};
#endif // CLIPBOARDHISTORYPANEL_H
