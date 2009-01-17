/*
this file is part of notepad++
Copyright (C)2003 Don HO < donho@altern.org >

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef PREFERENCE_DLG_H
#define PREFERENCE_DLG_H

#include "Window.h"
#include "StaticDialog.h"
#include "ControlsTab.h"
#include "preference_rc.h"
#include "URLCtrl.h"
#include "Parameters.h"
#include "regExtDlg.h"
#include "WordStyleDlg.h"

class SettingsDlg : public StaticDialog
{
public :
	SettingsDlg() {};
	virtual void destroy() {
		_nbHistoryVal.destroy();
	};
private :
	URLCtrl _nbHistoryVal;
	bool isCheckedOrNot(int checkControlID) const {
		return (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, checkControlID), BM_GETCHECK, 0, 0));
	};
	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
};

class BarsDlg : public StaticDialog
{
public :
	BarsDlg() {};
private :
	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
};

class MarginsDlg : public StaticDialog
{
public :
	MarginsDlg() {};
	virtual void destroy() {
		_tabSizeVal.destroy();
		_verticalEdgeLineNbColVal.destroy();
	};
	
private :
	URLCtrl _tabSizeVal;
	URLCtrl _verticalEdgeLineNbColVal;
	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	void changePanelTo(int index);
};

struct LangID_Name
{
	LangType _id;
	generic_string _name;
	LangID_Name(LangType id, generic_string name) : _id(id), _name(name){};
};

class DefaultNewDocDlg : public StaticDialog
{
public :
	DefaultNewDocDlg() {};
private :
	std::vector<LangID_Name> _langList;
	void makeOpenAnsiAsUtf8(bool doIt){
		if (!doIt)
			::SendDlgItemMessage(_hSelf, IDC_CHECK_OPENANSIASUTF8, BM_SETCHECK, BST_UNCHECKED, 0);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_OPENANSIASUTF8), doIt);
	};
	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
};

class LangMenuDlg : public StaticDialog
{
public :
	LangMenuDlg() {};
private :
	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	vector<LangMenuItem> _langList;
};

class PrintSettingsDlg : public StaticDialog
{
public :
	PrintSettingsDlg() {};
private :
	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
};

class BackupDlg : public StaticDialog
{
public :
	BackupDlg() {};
private :
	URLCtrl _nbCharVal;
	void updateBackupGUI();
	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
};

struct strCouple {
	generic_string _varDesc;
	generic_string _var;
	strCouple(TCHAR *varDesc, TCHAR *var): _varDesc(varDesc), _var(var){};
};

class PrintSettings2Dlg : public StaticDialog
{
public :
	PrintSettings2Dlg():_focusedEditCtrl(0), _selStart(0), _selEnd(0){};
private :
	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	vector<strCouple> varList;
	int _focusedEditCtrl;
	DWORD _selStart;
	DWORD _selEnd;

	//ColourStaticTextHooker _colourHooker;
};

class PreferenceDlg : public StaticDialog
{
friend class Notepad_plus;

public :
	PreferenceDlg(){};

    void init(HINSTANCE hInst, HWND parent)	{
        Window::init(hInst, parent);
	};

    void doDialog(bool isRTL = false) {
    	if (!isCreated())
		{
			create(IDD_PREFERENCE_BOX, isRTL);
			goToCenter();
		}
	    display();
    };

	virtual void destroy() {
		_ctrlTab.destroy();
		_barsDlg.destroy();
		_marginsDlg.destroy();
		_settingsDlg.destroy();
		_fileAssocDlg.destroy();
		_langMenuDlg.destroy();
		_printSettingsDlg.destroy();
		_printSettings2Dlg.destroy();
		_defaultNewDocDlg.destroy();
	};
private :
	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	ControlsTab _ctrlTab;
	WindowVector _wVector;
	BarsDlg _barsDlg;
	MarginsDlg _marginsDlg;
	SettingsDlg _settingsDlg;
	RegExtDlg _fileAssocDlg;
	LangMenuDlg _langMenuDlg;
	PrintSettingsDlg _printSettingsDlg;
	PrintSettings2Dlg _printSettings2Dlg;
	DefaultNewDocDlg _defaultNewDocDlg;
	BackupDlg _backupDlg;
};



#endif //PREFERENCE_DLG_H