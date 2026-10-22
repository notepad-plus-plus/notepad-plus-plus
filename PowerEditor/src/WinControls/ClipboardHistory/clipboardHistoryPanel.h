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

#include <windows.h>

#include <commctrl.h>

#include <vector>

#include "DockingDlgInterface.h"
#include "ListView.h"
#include "Window.h"
#include "clipboardHistoryPanel_rc.h"

#define CH_PROJECTPANELTITLE		L"Clipboard History"

class ScintillaEditView;

struct ClipboardDataInfo {
	std::vector<unsigned char> _data;
	bool _isBinaryContained = false;
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

class StringArray : public ByteArray {
public:
	StringArray(const ClipboardDataInfo& cd, size_t maxLen);
};

class CBHListView : public ListViewBase
{
public:
	CBHListView() = default;
	~CBHListView() override = default;

	operator HWND() const noexcept {
		return _hSelf;
	}

	void init(HINSTANCE hInst, HWND hParent) override;
	void reSizeToWH(int width, int height);

private:
	using Window::reSizeToWH;
};

class ClipboardHistoryPanel : public DockingDlgInterface {
public:
	ClipboardHistoryPanel()
		: DockingDlgInterface(IDD_CLIPBOARDHISTORY_PANEL), _ppEditView(nullptr), _hwndNextCbViewer(nullptr)
	{}

	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		DockingDlgInterface::init(hInst, hPere);
		_ppEditView = ppEditView;
	}

	void setParent(HWND parent2set) {
		_hParent = parent2set;
	}

	ClipboardDataInfo getClipboadData();
	void addToClipboadHistory(const ClipboardDataInfo& cbd);
	int getClipboardDataIndex(const ClipboardDataInfo& cbd) const;

	void setBackgroundColor(COLORREF bgColour) override {
		ListView_SetBkColor(_listView, bgColour);
		ListView_SetTextBkColor(_listView, bgColour);
		::RedrawWindow(_listView, nullptr, nullptr, RDW_INVALIDATE);
	}
	void setForegroundColor(COLORREF fgColour) override {
		ListView_SetTextColor(_listView, fgColour);
		::RedrawWindow(_listView, nullptr, nullptr, RDW_INVALIDATE);
	}

	bool trackClipboardOps(bool bTrack) {
		bool bPreviousState = _isTrackingClipboardOps;
		_isTrackingClipboardOps = bTrack;
		return bPreviousState;
	}

protected:
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
	ScintillaEditView **_ppEditView = nullptr;
	std::vector<ClipboardDataInfo> _clipboardDataInfos;
	HWND _hwndNextCbViewer = nullptr;
	CBHListView _listView;
	bool _isTrackingClipboardOps = true; // false when we do not want to track & show some Clipboard operations

	using DockingDlgInterface::init;

	void onDoubleClick(HWND hList);
};
