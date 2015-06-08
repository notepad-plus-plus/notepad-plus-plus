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


#ifndef SIZE_DLG_H
#define SIZE_DLG_H

#include "StaticDialog.h"
#include "Common.h"

const int DEFAULT_NB_NUMBER = 2;
class ValueDlg : public StaticDialog
{
public :
        ValueDlg() : StaticDialog(), _nbNumber(DEFAULT_NB_NUMBER) {};
        void init(HINSTANCE hInst, HWND parent, int valueToSet, const TCHAR *text);
        int doDialog(POINT p, bool isRTL = false);
		void setNBNumber(int nbNumber) {
			if (nbNumber > 0)
				_nbNumber = nbNumber;
		};
		int reSizeValueBox();
		void destroy() {};

protected :
	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM);

private :
	int _nbNumber;
    int _defaultValue;
	generic_string _name;
	POINT _p;
};

// 0 : sans fullscreen
// 1 : fullscreen
// 2 : postit
const int buttonStatus_nada = 0;
const int buttonStatus_fullscreen = 1;
const int buttonStatus_postit = 2;

class ButtonDlg : public StaticDialog
{
public :
    ButtonDlg() : StaticDialog(), _buttonStatus(buttonStatus_nada) {};
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
	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM);
    int _buttonStatus;

};
#endif //TABSIZE_DLG_H
