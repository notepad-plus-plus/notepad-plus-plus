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

#define DM_PANELTITLE     L"Document Map"

const wchar_t VIEWZONE_DOCUMENTMAP[64] = L"Document map";

class ScintillaEditView;
class Buffer;
struct MapPosition;

enum moveMode {
	perLine,
	perPage
};

class ViewZoneDlg : public StaticDialog
{
public :
	ViewZoneDlg() : StaticDialog(), _viewZoneCanvas(nullptr), _higherY(0), _lowerY(0) {}

	enum class ViewZoneColorIndex {
		focus,
		frost
	};

	void doDialog();

	void destroy() override {}

	void drawZone(long hY, long lY) {
		_higherY = hY;
		_lowerY = lY;
		if (nullptr != _viewZoneCanvas)
			::InvalidateRect(_viewZoneCanvas, nullptr, TRUE);
	}

	int getViewerHeight() const {
		return (_lowerY - _higherY);
	}

	int getCurrentCenterPosY() const {
		return (_lowerY - _higherY)/2 + _higherY;
	}

	static void setColour(COLORREF colour2Set, ViewZoneColorIndex i);

protected :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

	static COLORREF _focus;
	static COLORREF _frost;

	void drawPreviewZone(DRAWITEMSTRUCT *pdis);

private :
	HWND _viewZoneCanvas = nullptr;
	
	long _higherY = 0;
	long _lowerY = 0;

	static LRESULT CALLBACK CanvasProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
};


class DocumentMap : public DockingDlgInterface {
public:
	DocumentMap(): DockingDlgInterface(IDD_DOCUMENTMAP) {}

	void create(tTbData * data, bool isRTL = false) override {
		DockingDlgInterface::create(data, isRTL);
		data->pszAddInfo = id4dockingCont.c_str();
	}

	void create(tTbData* data, std::array<int, 3> iconIDs, bool isRTL = false) override {
		DockingDlgInterface::create(data, iconIDs, isRTL);
		data->pszAddInfo = id4dockingCont.c_str();
	}

	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		DockingDlgInterface::init(hInst, hPere);
		_ppEditView = ppEditView;
	}

	void display(bool toShow = true) const override {
		DockingDlgInterface::display(toShow);
		_vzDlg.display();
	}

	void redraw(bool forceUpdate = false) const override;

	void setParent(HWND parent2set) {
		_hParent = parent2set;
	}

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
	bool isTemporarilyShowing() const { return _isTemporarilyShowing; }
	void setTemporarilyShowing(bool tempShowing) { _isTemporarilyShowing = tempShowing; }

protected:
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
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
	std::wstring id4dockingCont = DM_NOFOCUSWHILECLICKINGCAPTION;

	using DockingDlgInterface::init;
};
