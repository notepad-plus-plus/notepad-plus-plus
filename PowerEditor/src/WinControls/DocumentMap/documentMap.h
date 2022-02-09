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
#include "documentMap_rc.h"

#define DM_PANELTITLE     TEXT("Document Map")

#define DOCUMENTMAP_SCROLL        (WM_USER + 1)
#define DOCUMENTMAP_MOUSECLICKED  (WM_USER + 2)
#define DOCUMENTMAP_MOUSEWHEEL    (WM_USER + 3)

const TCHAR VIEWZONE_DOCUMENTMAP[64] = TEXT("Document map");

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

	enum class ViewZoneColorIndex {
		focus,
		frost
	};

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

	static void setColour(COLORREF colour2Set, ViewZoneColorIndex i);

protected :
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK canvasStaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK canvas_runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	static COLORREF _focus;
	static COLORREF _frost;

	void drawPreviewZone(DRAWITEMSTRUCT *pdis);

private :
	HWND _viewZoneCanvas = nullptr;
	WNDPROC _canvasDefaultProc = nullptr;
	
	long _higherY = 0;
	long _lowerY = 0;
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
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	bool needToRecomputeWith(const ScintillaEditView *editView = nullptr);

private:
	ScintillaEditView**_ppEditView = nullptr;
	ScintillaEditView*_pMapView = nullptr;
	ViewZoneDlg _vzDlg;
	HWND _hwndScintilla = nullptr;
	bool _isTemporarilyShowing = false;

	// for needToRecomputeWith function
	intptr_t _displayZoom = -1;
	intptr_t _displayWidth = 0;
	generic_string id4dockingCont = DM_NOFOCUSWHILECLICKINGCAPTION;
};
