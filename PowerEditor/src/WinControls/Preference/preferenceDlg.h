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

#include "ControlsTab.h"
#include "preference_rc.h"
#include "URLCtrl.h"
#include "Parameters.h"
#include "regExtDlg.h"
#include "WordStyleDlg.h"

class MiscSubDlg : public StaticDialog
{
public :
	MiscSubDlg() = default;

private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class GeneralSubDlg : public StaticDialog
{
public :
	GeneralSubDlg() = default;
	void setToolIconsFromStdToSmall();
	void disableTabbarAlternateIcons();

private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class EditingSubDlg : public StaticDialog
{
public :
	EditingSubDlg() = default;
	
private :
	HWND _tip = nullptr;

	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void initScintParam();
	void changeLineHiliteMode(bool enableSlider);
};

class DarkModeSubDlg : public StaticDialog
{
public:
	DarkModeSubDlg() = default;

private:
	ColourPicker* _pBackgroundColorPicker = nullptr;
	ColourPicker* _pSofterBackgroundColorPicker = nullptr;
	ColourPicker* _pHotBackgroundColorPicker = nullptr;
	ColourPicker* _pPureBackgroundColorPicker = nullptr;
	ColourPicker* _pErrorBackgroundColorPicker = nullptr;
	ColourPicker* _pTextColorPicker = nullptr;
	ColourPicker* _pDarkerTextColorPicker = nullptr;
	ColourPicker* _pDisabledTextColorPicker = nullptr;
	ColourPicker* _pEdgeColorPicker = nullptr;
	ColourPicker* _pLinkColorPicker = nullptr;
	ColourPicker* _pHotEdgeColorPicker = nullptr;
	ColourPicker* _pDisabledEdgeColorPicker = nullptr;

	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void enableCustomizedColorCtrls(bool doEnable);
	void move2CtrlLeft(int ctrlID, HWND handle2Move, int handle2MoveWidth, int handle2MoveHeight);
};

class MarginsBorderEdgeSubDlg : public StaticDialog
{
public :
	MarginsBorderEdgeSubDlg() = default;
	
private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void initScintParam();
};

struct LangID_Name
{
	LangType _id = L_TEXT;
	generic_string _name;
	LangID_Name(LangType id, const generic_string& name) : _id(id), _name(name){};
};

class NewDocumentSubDlg : public StaticDialog
{
public :
	NewDocumentSubDlg() = default;

private :
	void makeOpenAnsiAsUtf8(bool doIt){
		if (!doIt)
			::SendDlgItemMessage(_hSelf, IDC_CHECK_OPENANSIASUTF8, BM_SETCHECK, BST_UNCHECKED, 0);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_OPENANSIASUTF8), doIt);
	};
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class DefaultDirectorySubDlg : public StaticDialog
{
public :
	DefaultDirectorySubDlg() = default;

private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class RecentFilesHistorySubDlg : public StaticDialog
{
public :
	RecentFilesHistorySubDlg() = default;
	virtual void destroy() {
		_nbHistoryVal.destroy();
		_customLenVal.destroy();
	};
private :
	URLCtrl _nbHistoryVal;
	URLCtrl _customLenVal;
	void setCustomLen(int val);
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class LanguageSubDlg : public StaticDialog
{
public :
	LanguageSubDlg() = default;
	virtual void destroy() {
		_tabSizeVal.destroy();
	};

private :
    LexerStylerArray _lsArray;
	URLCtrl _tabSizeVal;
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	std::vector<LangMenuItem> _langList;
};

class HighlightingSubDlg : public StaticDialog
{
public :
	HighlightingSubDlg() = default;

private :

	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};


class SearchingSubDlg : public StaticDialog
{
public:
	SearchingSubDlg() = default;

private:
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class PrintSubDlg : public StaticDialog
{
public :
	PrintSubDlg() = default;

private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	std::vector<generic_string> varList;
	int _focusedEditCtrl = 0;
};

class BackupSubDlg : public StaticDialog
{
public :
	BackupSubDlg() = default;

private :
	void updateBackupGUI();
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};


class AutoCompletionSubDlg : public StaticDialog
{
public :
	AutoCompletionSubDlg() = default;
private :
	URLCtrl _nbCharVal;
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class MultiInstanceSubDlg : public StaticDialog
{
public :
	MultiInstanceSubDlg() = default;

private :
	const SYSTEMTIME _BTTF_time = {1985, 10, 6, 26, 16, 24, 42, 0};
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class DelimiterSubDlg : public StaticDialog
{
public :
	DelimiterSubDlg() = default;
	~DelimiterSubDlg() {
		if (_tip)
			::DestroyWindow(_tip);
	};

private :
	POINT _singleLineModePoint = {};
	POINT _multiLineModePoint = {};
	RECT _closerRect = {};
	RECT _closerLabelRect = {};
	HWND _tip = nullptr;

	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void detectSpace(const char *text2Check, int & nbSp, int & nbTab) const;
	generic_string getWarningText(size_t nbSp, size_t nbTab) const;
	void setWarningIfNeed() const;
};

class CloudAndLinkSubDlg : public StaticDialog
{
public :
	CloudAndLinkSubDlg() = default;

private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class SearchEngineSubDlg : public StaticDialog
{
public :
	SearchEngineSubDlg() = default;

private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class PreferenceDlg : public StaticDialog
{
friend class NativeLangSpeaker;
friend class Notepad_plus;
public :
	PreferenceDlg() = default;

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
	bool renameDialogTitle(const TCHAR *internalName, const TCHAR *newName);
	
	int getListSelectedIndex() const {
		return static_cast<int32_t>(::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_GETCURSEL, 0, 0));
	};

	void showDialogByName(const TCHAR *name) const;
	bool setListSelection(size_t currentSel) const;

	bool goToSection(size_t iPage, intptr_t ctrlID = -1);

	virtual void destroy();

private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void makeCategoryList();
	int32_t getIndexFromName(const TCHAR *name) const;
	void showDialogByIndex(size_t index) const;
	WindowVector _wVector;
	GeneralSubDlg _generalSubDlg;
	EditingSubDlg _editingSubDlg;
	DarkModeSubDlg _darkModeSubDlg;
	MarginsBorderEdgeSubDlg _marginsBorderEdgeSubDlg;
	MiscSubDlg _miscSubDlg;
	RegExtDlg _fileAssocDlg;
	LanguageSubDlg _languageSubDlg;
	HighlightingSubDlg _highlightingSubDlg;
	PrintSubDlg _printSubDlg;
	NewDocumentSubDlg _newDocumentSubDlg;
	DefaultDirectorySubDlg	_defaultDirectorySubDlg;
	RecentFilesHistorySubDlg _recentFilesHistorySubDlg;
	BackupSubDlg _backupSubDlg;
	AutoCompletionSubDlg _autoCompletionSubDlg;
	MultiInstanceSubDlg _multiInstanceSubDlg;
	DelimiterSubDlg _delimiterSubDlg;
	CloudAndLinkSubDlg _cloudAndLinkSubDlg;
	SearchEngineSubDlg _searchEngineSubDlg;
	SearchingSubDlg _searchingSubDlg;
};

