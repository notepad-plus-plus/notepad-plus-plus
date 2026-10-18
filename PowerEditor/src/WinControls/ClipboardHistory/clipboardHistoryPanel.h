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
#include <string>

#define CH_PROJECTPANELTITLE		L"Clipboard History"

class ScintillaEditView;

struct ClipboardDataInfo {
	std::vector<unsigned char> _data;
	bool _isBinaryContained = false;

	mutable std::wstring _cleanedFullStr;
	mutable bool _isCleaned = false;
	mutable int _cachedWidth = -1;
	mutable std::wstring _cachedDisplayStr;

	void clearDisplayCache() noexcept {
		_cachedWidth = -1;
		_cachedDisplayStr.clear();
	}
};

class ByteArray {
public:
	ByteArray() = default;
	explicit ByteArray(const ClipboardDataInfo& cd);
	
	~ByteArray() {
		if (_pBytes)
			delete [] _pBytes;
	}
	const unsigned char* getPointer() const { return _pBytes; }
	size_t getLength() const { return _length; }
protected:
	unsigned char *_pBytes = nullptr;
	size_t _length = 0;
};

class ClipboardHistoryPanel : public DockingDlgInterface {
public:
	ClipboardHistoryPanel(): DockingDlgInterface(IDD_CLIPBOARDHISTORY_PANEL), _ppEditView(nullptr), _hwndNextCbViewer(nullptr), _lbBgColor(-1), _lbFgColor(-1) {}

	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		DockingDlgInterface::init(hInst, hPere);
		_ppEditView = ppEditView;
	}

	void setParent(HWND parent2set) {
		_hParent = parent2set;
	}

	ClipboardDataInfo getClipboadData();
	void addToClipboadHistory(ClipboardDataInfo cbd);
	int getClipboardDataIndex(const ClipboardDataInfo& cbd);

	void setBackgroundColor(COLORREF bgColour) override {
		_lbBgColor = bgColour;
	}
	void setForegroundColor(COLORREF fgColour) override {
		_lbFgColor = fgColour;
	}

	void drawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	bool trackClipboardOps(bool bTrack) {
		bool bPreviousState = _isTrackingClipboardOps;
		_isTrackingClipboardOps = bTrack;
		return bPreviousState;
	}

protected:
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
	void destroyListBoxFont() noexcept;
	void createListBoxFontAndHeight();

	const std::wstring& getDisplayString(HDC hdc, const ClipboardDataInfo& cbd, int maxPixelWidth) const;

	ScintillaEditView **_ppEditView = nullptr;
	std::vector<ClipboardDataInfo> _clipboardDataInfos;
	HWND _hwndNextCbViewer = nullptr;
	HFONT _hListBoxFont = nullptr;
	int _lbBgColor = -1;
	int _lbFgColor= -1;
	bool _isTrackingClipboardOps = true; // false when we do not want to track & show some Clipboard operations
	int _lastClientWidth = -1;
};
