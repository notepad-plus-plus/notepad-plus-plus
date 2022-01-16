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

#include "StaticDialog.h"
#include "Common.h"

const int DEFAULT_NB_NUMBER = 2;
class ValueDlg : public StaticDialog
{
public :
        ValueDlg() = default;
        void init(HINSTANCE hInst, HWND parent, int valueToSet, const TCHAR *text);
        int doDialog(POINT p, bool isRTL = false);
		void setNBNumber(int nbNumber) {
			if (nbNumber > 0)
				_nbNumber = nbNumber;
		};
		int reSizeValueBox();
		void destroy() {};

protected :
	intptr_t CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM);

private :
	int _nbNumber = DEFAULT_NB_NUMBER;
    int _defaultValue = 0;
	generic_string _name;
	POINT _p = {0, 0};
};

// 0 : normal window
// 1 : fullscreen
// 2 : postit
// 4 : distractionFree
const int buttonStatus_nada = 0;            // 0000 0000
const int buttonStatus_fullscreen = 1;      // 0000 0001
const int buttonStatus_postit = 2;          // 0000 0010
const int buttonStatus_distractionFree = 4; // 0000 0100

class ButtonDlg : public StaticDialog
{
public :
    ButtonDlg() = default;
    void init(HINSTANCE hInst, HWND parent){
        Window::init(hInst, parent);
    };

    void doDialog(bool isRTL = false);
	void destroy() {};
    int getButtonStatus() const {
        return _buttonStatus;
    };
    void setButtonStatus(int buttonStatus) {
        _buttonStatus = buttonStatus;
    };

    void display(bool toShow = true) const {
        int cmdToShow = toShow?SW_SHOW:SW_HIDE;
        if (!toShow)
        {
            cmdToShow = (_buttonStatus != buttonStatus_nada)?SW_SHOW:SW_HIDE; 
        }
		::ShowWindow(_hSelf, cmdToShow);
	};

protected :
	intptr_t CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM);
    int _buttonStatus = buttonStatus_nada;

};
