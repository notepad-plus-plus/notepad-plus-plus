// This file is part of Notepad++ project
// Copyright (C)2024 Don HO <don.h@free.fr>

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
#include "dpiManagerV2.h"
#include "Notepad_plus_msgs.h"
#include "Window.h"

typedef HRESULT (WINAPI * ETDTProc) (HWND, DWORD);

enum class PosAlign { left, right, top, bottom };

#pragma pack(push, 1)
struct DLGTEMPLATEEX
{
	WORD   dlgVer = 0;
	WORD   signature = 0;
	DWORD  helpID = 0;
	DWORD  exStyle = 0;
	DWORD  style = 0;
	WORD   cDlgItems = 0;
	short  x = 0;
	short  y = 0;
	short  cx = 0;
	short  cy = 0;
	// The structure has more fields but are variable length
	//sz_Or_Ord menu;
	//sz_Or_Ord windowClass;
	//WCHAR  title[titleLen];
	//WORD   pointsize;
	//WORD   weight;
	//BYTE   italic;
	//BYTE   charset;
	//WCHAR  typeface[stringLen];
};
#pragma pack(pop)

class StaticDialog : public Window
{
public :
	~StaticDialog() override;

	virtual void create(int dialogID, bool isRTL = false, bool msgDestParent = true, WORD fontSize = 8);

	virtual bool isCreated() const {
		return (_hSelf != nullptr);
	}

	void getMappedChildRect(HWND hChild, RECT& rcChild) const;
	void getMappedChildRect(int idChild, RECT& rcChild) const;
	void redrawDlgItem(const int nIDDlgItem, bool forceUpdate = false) const;

	void goToCenter(UINT swpFlags = SWP_SHOWWINDOW);
	bool moveForDpiChange();

	void display(bool toShow = true, bool enhancedPositioningCheckWhenShowing = false) const;

	RECT getViewablePositionRect(RECT testRc) const;

	POINT getTopPoint(HWND hwnd, bool isLeft = true) const;

	bool isCheckedOrNot(int checkControlID) const
	{
		return (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, checkControlID), BM_GETCHECK, 0, 0));
	}

	void setChecked(int checkControlID, bool checkOrNot = true) const
	{
		::SendDlgItemMessage(_hSelf, checkControlID, BM_SETCHECK, checkOrNot ? BST_CHECKED : BST_UNCHECKED, 0);
	}

	void setDpi() {
		_dpiManager.setDpi(_hSelf);
	}

	void setPositionDpi(LPARAM lParam, UINT flags = SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE) {
		DPIManagerV2::setPositionDpi(lParam, _hSelf, flags);
	}

	void destroy() override;

	DPIManagerV2& dpiManager() { return _dpiManager; }

protected:
	RECT _rc{};
	DPIManagerV2 _dpiManager;

	static intptr_t CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) = 0;

	HWND myCreateDialogIndirectParam(int dialogID, bool isRTL, WORD fontSize, DLGPROC myDlgProc = StaticDialog::dlgProc);
	INT_PTR myCreateDialogBoxIndirectParam(int dialogID, bool isRTL, WORD fontSize = 8);
};
