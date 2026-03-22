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

#include <memory>
#include <string>
#include <vector>

#include "ColourPicker.h"
#include "Common.h"
#include "ContextMenu.h"
#include "ControlsTab.h"
#include "Notepad_plus_msgs.h"
#include "Parameters.h"
#include "StaticDialog.h"
#include "dpiManagerV2.h"
#include "preference_rc.h"
#include "regExtDlg.h"

class MiscSubDlg : public StaticDialog
{
friend class PreferenceDlg;
public :
	MiscSubDlg() = default;
	~MiscSubDlg() override {
		if (_tipScintillaRenderingTechnology)
		{
			::DestroyWindow(_tipScintillaRenderingTechnology);
			_tipScintillaRenderingTechnology = nullptr;
		}
	}

private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	HWND _tipScintillaRenderingTechnology = nullptr;
};

class GeneralSubDlg : public StaticDialog
{
public :
	GeneralSubDlg() = default;

private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
};

class ToolbarSubDlg : public StaticDialog
{
friend class PreferenceDlg;
public:
	ToolbarSubDlg() = default;

private:
	HWND _accentTip = nullptr;
	std::unique_ptr<ColourPicker> _pIconColorPicker = nullptr;

	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	UINT getToolbarIconSetMsg(int* iconSetID);
	void move2CtrlLeft(int ctrlID, HWND handle2Move, int handle2MoveWidth, int handle2MoveHeight);
	void enableIconColorPicker(bool enable, bool useDark);
};

class TabbarSubDlg : public StaticDialog
{
friend class PreferenceDlg;
public:
	TabbarSubDlg() = default;
	void setTabbarAlternateIcons(bool enable = false);

private:
	HWND _tabCompactLabelLenTip = nullptr;

	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
};

class EditingSubDlg : public StaticDialog
{
friend class PreferenceDlg;
public :
	EditingSubDlg() = default;
	~EditingSubDlg() override = default;
	
private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	void initScintParam();
	void changeLineHiliteMode(bool enableSlider);
};

class Editing2SubDlg : public StaticDialog
{
friend class PreferenceDlg;
public :
	Editing2SubDlg() = default;
	~Editing2SubDlg() override {
		if (_tip != nullptr)
		{
			::DestroyWindow(_tip);
			_tip = nullptr;
		}

		for (auto& tip : _tips)
		{
			if (tip != nullptr)
			{
				::DestroyWindow(tip);
				tip = nullptr;
			}
		}
	}

private:
	HWND _tip = nullptr;
	HWND _tipNote = nullptr;
	HWND _tipAbb = nullptr;
	HWND _tipCodepoint = nullptr;
	HWND _tipNpcColor = nullptr;
	HWND _tipNpcInclude = nullptr;

	std::vector<HWND> _tips;
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
};

class DarkModeSubDlg : public StaticDialog
{
public:
	DarkModeSubDlg() = default;

	void destroyResetMenu() {
		_resetPopupMenu.destroy();
	}

private:
	std::unique_ptr<ColourPicker> _pBackgroundColorPicker = nullptr;
	std::unique_ptr<ColourPicker> _pCtrlBackgroundColorPicker = nullptr;
	std::unique_ptr<ColourPicker> _pHotBackgroundColorPicker = nullptr;
	std::unique_ptr<ColourPicker> _pDlgBackgroundColorPicker = nullptr;
	std::unique_ptr<ColourPicker> _pErrorBackgroundColorPicker = nullptr;
	std::unique_ptr<ColourPicker> _pTextColorPicker = nullptr;
	std::unique_ptr<ColourPicker> _pDarkerTextColorPicker = nullptr;
	std::unique_ptr<ColourPicker> _pDisabledTextColorPicker = nullptr;
	std::unique_ptr<ColourPicker> _pEdgeColorPicker = nullptr;
	std::unique_ptr<ColourPicker> _pLinkColorPicker = nullptr;
	std::unique_ptr<ColourPicker> _pHotEdgeColorPicker = nullptr;
	std::unique_ptr<ColourPicker> _pDisabledEdgeColorPicker = nullptr;

	ContextMenu _resetPopupMenu;

	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	void enableCustomizedColorCtrls(bool doEnable);
	void move2CtrlLeft(int ctrlID, HWND handle2Move, int handle2MoveWidth, int handle2MoveHeight);
};

class MarginsBorderEdgeSubDlg : public StaticDialog
{
friend class PreferenceDlg;
public :
	MarginsBorderEdgeSubDlg() = default;
	~MarginsBorderEdgeSubDlg() override {
		if (_verticalEdgeTip != nullptr)
		{
			::DestroyWindow(_verticalEdgeTip);
			_verticalEdgeTip = nullptr;
		}
	}

private :
	HWND _verticalEdgeTip = nullptr;

	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	void initScintParam();
};

struct LangID_Name
{
	LangType _id = L_TEXT;
	std::wstring _name;
	LangID_Name(LangType id, const std::wstring& name) : _id(id), _name(name) {}
};

class NewDocumentSubDlg : public StaticDialog
{
public :
	NewDocumentSubDlg() = default;

private :
	void makeOpenAnsiAsUtf8(bool doIt) const;
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
};

class DefaultDirectorySubDlg : public StaticDialog
{
public :
	DefaultDirectorySubDlg() = default;

private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
};

class RecentFilesHistorySubDlg : public StaticDialog
{
public :
	RecentFilesHistorySubDlg() = default;
private :
	void setCustomLen(int val);
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
};

class LanguageSubDlg : public StaticDialog
{
public :
	LanguageSubDlg() = default;

private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	std::vector<LangMenuItem> _langList;
};

class IndentationSubDlg : public StaticDialog
{
friend class PreferenceDlg;
public :
	IndentationSubDlg() = default;
	~IndentationSubDlg() override {
		if (_tipAutoIndentBasic != nullptr)
		{
			::DestroyWindow(_tipAutoIndentBasic);
			_tipAutoIndentBasic = nullptr;
		}

		if (_tipAutoIndentAdvanced != nullptr)
		{
			::DestroyWindow(_tipAutoIndentAdvanced);
			_tipAutoIndentAdvanced = nullptr;
		}
	}

private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	HWND _tipAutoIndentBasic = nullptr;
	HWND _tipAutoIndentAdvanced = nullptr;
};

class HighlightingSubDlg : public StaticDialog
{
public :
	HighlightingSubDlg() = default;

private :

	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
};


class SearchingSubDlg : public StaticDialog
{
friend class PreferenceDlg;
public:
	SearchingSubDlg() = default;
	~SearchingSubDlg() override {
		if (_tipInSelThresh != nullptr)
		{
			::DestroyWindow(_tipInSelThresh);
			_tipInSelThresh = nullptr;
		}
	}

private:
	HWND _tipInSelThresh = nullptr;
	HWND _tipFillFindWhatThresh = nullptr;
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
};

class PrintSubDlg : public StaticDialog
{
public :
	PrintSubDlg() = default;

private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	std::vector<std::wstring> varList;
	int _focusedEditCtrl = 0;
};

class BackupSubDlg : public StaticDialog
{
public :
	BackupSubDlg() = default;

private :
	void updateBackupSessionGUI();
	void updateBackupOnSaveGUI();
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
};


class AutoCompletionSubDlg : public StaticDialog
{
public :
	AutoCompletionSubDlg() = default;
private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
};

class MultiInstanceSubDlg : public StaticDialog
{
public :
	MultiInstanceSubDlg() = default;

private :
	const SYSTEMTIME _BTTF_time = {1985, 10, 6, 26, 16, 24, 42, 0};
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
};

class DelimiterSubDlg : public StaticDialog
{
friend class PreferenceDlg;
public :
	DelimiterSubDlg() = default;
	~DelimiterSubDlg() override {
		if (_tip)
			::DestroyWindow(_tip);
	}

private :
	LONG _gapEditHor = 0;
	LONG _gapEditVer = 0;
	LONG _gapText = 0;
	HWND _tip = nullptr;

	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	void detectSpace(const char *text2Check, int & nbSp, int & nbTab) const;
	std::wstring getWarningText(size_t nbSp, size_t nbTab) const;
	void setWarningIfNeed() const;
	void calcCtrlsPos();
	void setCtrlsPos(bool isMultiline);
};

class CloudAndLinkSubDlg : public StaticDialog
{
public :
	CloudAndLinkSubDlg() = default;

private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
};

class SearchEngineSubDlg : public StaticDialog
{
public :
	SearchEngineSubDlg() = default;

private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
};

class PerformanceSubDlg : public StaticDialog
{
friend class PreferenceDlg;
public :
	PerformanceSubDlg() = default;
	~PerformanceSubDlg() override {
		if (_largeFileRestrictionTip)
			::DestroyWindow(_largeFileRestrictionTip);
	}

private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

	HWND _largeFileRestrictionTip = nullptr;
};

class PreferenceDlg : public StaticDialog
{
friend class NativeLangSpeaker;
friend class Notepad_plus;
public :
	PreferenceDlg() = default;

	void doDialog(bool isRTL = false) {
		if (!isCreated())
		{
			const auto dpiContext = DPIManagerV2::setThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
			create(IDD_PREFERENCE_BOX, isRTL);
			goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);
			DPIManagerV2::setThreadDpiAwarenessContext(dpiContext);
		}
		display();
	}

	bool renameDialogTitle(const wchar_t *internalName, const wchar_t *newName);
	
	int getListSelectedIndex() const {
		return static_cast<int>(::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_GETCURSEL, 0, 0));
	}

	void showDialogByName(const wchar_t *name) const;
	bool setListSelection(size_t currentSel) const;

	bool goToSection(size_t iPage, intptr_t ctrlID = -1);

	void destroy() override;

private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	void makeCategoryList();
	int getIndexFromName(const wchar_t* name) const;
	void showDialogByIndex(size_t index) const;
	WindowVector _wVector;
	GeneralSubDlg _generalSubDlg;
	ToolbarSubDlg _toolbarSubDlg;
	TabbarSubDlg _tabbarSubDlg;
	EditingSubDlg _editingSubDlg;
	Editing2SubDlg _editing2SubDlg;
	DarkModeSubDlg _darkModeSubDlg;
	MarginsBorderEdgeSubDlg _marginsBorderEdgeSubDlg;
	MiscSubDlg _miscSubDlg;
	RegExtDlg _fileAssocDlg;
	LanguageSubDlg _languageSubDlg;
	IndentationSubDlg _indentationSubDlg;
	HighlightingSubDlg _highlightingSubDlg;
	PrintSubDlg _printSubDlg;
	NewDocumentSubDlg _newDocumentSubDlg;
	DefaultDirectorySubDlg	_defaultDirectorySubDlg;
	RecentFilesHistorySubDlg _recentFilesHistorySubDlg;
	BackupSubDlg _backupSubDlg;
	AutoCompletionSubDlg _autoCompletionSubDlg;
	MultiInstanceSubDlg _multiInstanceSubDlg;
	DelimiterSubDlg _delimiterSubDlg;
	PerformanceSubDlg _performanceSubDlg;
	CloudAndLinkSubDlg _cloudAndLinkSubDlg;
	SearchEngineSubDlg _searchEngineSubDlg;
	SearchingSubDlg _searchingSubDlg;

	ControlInfoTip _gotoTip;
};
