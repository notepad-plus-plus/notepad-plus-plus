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

#include "DockingDlgInterface.h"
#include "documentMap_rc.h"

#define DM_PANELTITLE     TEXT("Document Map")

#define DOCUMENTMAP_SCROLL        (WM_USER + 1)
#define DOCUMENTMAP_MOUSECLICKED  (WM_USER + 2)
#define DOCUMENTMAP_MOUSEWHEEL    (WM_USER + 3)

class ScintillaEditView;
class Buffer;
struct MapPosition;

const bool moveDown = true;
const bool moveUp = false;


enum moveMode {
	perLine,
	perPage
};

class ViewZoneDlg : public StaticDialog
{
public :
	ViewZoneDlg() : StaticDialog(), _viewZoneCanvas(NULL), _canvasDefaultProc(nullptr), _higherY(0), _lowerY(0) {}

	void doDialog();

    virtual void destroy() {};

	void drawZone(long hY, long lY) {
		_higherY = hY;
		_lowerY = lY;
		if (NULL != _viewZoneCanvas)
			::InvalidateRect(_viewZoneCanvas, NULL, TRUE);
	};

	int getViewerHeight() const {
		return (_lowerY - _higherY);
	};

	int getCurrentCenterPosY() const {
		return (_lowerY - _higherY)/2 + _higherY;
	};

protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK canvasStaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK canvas_runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	void drawPreviewZone(DRAWITEMSTRUCT *pdis);

private :
	HWND _viewZoneCanvas = nullptr;
	WNDPROC _canvasDefaultProc = nullptr;
	
	long _higherY;
	long _lowerY;
};


class DocumentMap : public DockingDlgInterface {
public:
	DocumentMap(): DockingDlgInterface(IDD_DOCUMENTMAP) {};

	void create(tTbData * data, bool isRTL = false) {
		DockingDlgInterface::create(data, isRTL);
		data->pszAddInfo = id4dockingCont.c_str();
	};

	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		DockingDlgInterface::init(hInst, hPere);
		_ppEditView = ppEditView;
	};

    virtual void display(bool toShow = true) const {
        DockingDlgInterface::display(toShow);
		_vzDlg.display();
    };

	virtual void redraw(bool forceUpdate = false) const;

    void setParent(HWND parent2set){
        _hParent = parent2set;
    };

	void vzDlgDisplay(bool toShow = true) {
		_vzDlg.display(toShow);
	}

	void reloadMap();
	void showInMapTemporarily(Buffer *buf2show, ScintillaEditView *fromEditView);
	void wrapMap(const ScintillaEditView *editView = nullptr);
	void initWrapMap();
	void scrollMap();
	void scrollMap(bool direction, moveMode whichMode);
	void scrollMapWith(const MapPosition & mapPos);
	void doMove();
	void fold(size_t line, bool foldOrNot);
	void foldAll(bool mode);
	void setSyntaxHiliting();
	void changeTextDirection(bool isRTL);
	bool isTemporarilyShowing() const { return _isTemporarilyShowing; };
	void setTemporarilyShowing(bool tempShowing) { _isTemporarilyShowing = tempShowing; }

protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	bool needToRecomputeWith(const ScintillaEditView *editView = nullptr);
	int getEditorTextZoneWidth(const ScintillaEditView *editView = nullptr);

private:
	ScintillaEditView **_ppEditView = nullptr;
	ScintillaEditView *_pMapView = nullptr;
	ViewZoneDlg _vzDlg;
	HWND _hwndScintilla;
	bool _isTemporarilyShowing = false;

	// for needToRecomputeWith function
	int _displayZoom = -1;
	int _displayWidth = 0;
	generic_string id4dockingCont = DM_NOFOCUSWHILECLICKINGCAPTION;
};
