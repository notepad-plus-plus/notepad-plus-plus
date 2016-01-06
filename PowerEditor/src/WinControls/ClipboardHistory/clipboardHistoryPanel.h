// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
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


#ifndef CLIPBOARDHISTORYPANEL_H
#define CLIPBOARDHISTORYPANEL_H

//#include <windows.h>
#ifndef DOCKINGDLGINTERFACE_H
#include "DockingDlgInterface.h"
#endif //DOCKINGDLGINTERFACE_H

#include "clipboardHistoryPanel_rc.h"
#include <vector>

#define CH_PROJECTPANELTITLE		TEXT("Clipboard History")

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
	ClipboardHistoryPanel(): DockingDlgInterface(IDD_CLIPBOARDHISTORY_PANEL), _ppEditView(NULL), _hwndNextCbViewer(NULL), _lbBgColor(-1), _lbFgColor(-1) {};

	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		DockingDlgInterface::init(hInst, hPere);
		_ppEditView = ppEditView;
	};
/*
    virtual void display(bool toShow = true) const {
        DockingDlgInterface::display(toShow);
    };
*/

    void setParent(HWND parent2set){
        _hParent = parent2set;
    };

	//void switchEncoding();
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
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	ScintillaEditView **_ppEditView;
	std::vector<ClipboardData> _clipboardDataVector;
	HWND _hwndNextCbViewer;
	int _lbBgColor;
	int _lbFgColor;

};
#endif // CLIPBOARDHISTORYPANEL_H
