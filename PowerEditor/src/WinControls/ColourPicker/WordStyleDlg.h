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
#include <utility>

#include "ColourPicker.h"
#include "URLCtrl.h"
#include "Parameters.h"
#include "StaticDialog.h"
#include "resource.h"

#define WM_UPDATESCINTILLAS      (WORDSTYLE_USER + 1) // WM_UPDATESCINTILLAS (BOOL doChangePanel, 0)
#define WM_UPDATEMAINMENUBITMAPS (WORDSTYLE_USER + 2)

const wchar_t FINDDLG_STAUSNOTFOUND_COLOR[64] = L"Find status: Not found";
const wchar_t FINDDLG_STAUSMESSAGE_COLOR[64] = L"Find status: Message";
const wchar_t FINDDLG_STAUSREACHED_COLOR[64] = L"Find status: Search end reached";

enum fontStyleType {BOLD_STATUS, ITALIC_STATUS, UNDERLINE_STATUS};

class WordStyleDlg : public StaticDialog
{
public :
	WordStyleDlg() = default;
	~WordStyleDlg() override {
		if (_globalOverrideTip)
			::DestroyWindow(_globalOverrideTip);
	}

	void create(int dialogID, bool isRTL = false, bool msgDestParent = true, WORD fontSize = 8) override;
	void doDialog(bool isRTL = false);
	void destroy() override;
	void prepare2Cancel();
	void redraw(bool forceUpdate = false) const override;
	void restoreGlobalOverrideValues() const;
	void addLastThemeEntry() const;
	bool selectThemeByName(const wchar_t* themeName);
	void syncWithSelFgSingleColorCtrl();
	bool goToSection(const wchar_t* sectionNames); // sectionNames is formed as following: "Language name:Style name"
	                                               // ex: "Global Styles:EOL custom color" will set Language on "Global Styles", then set Style on "EOL custom color" if both are found.

private :
	std::unique_ptr<ColourPicker> _pFgColour = nullptr;
	std::unique_ptr<ColourPicker> _pBgColour = nullptr;

    int _currentLexerIndex = 0;
	int _currentThemeIndex = 0;

    HWND _hCheckBold = nullptr;
    HWND _hCheckItalic = nullptr;
	HWND _hCheckUnderline = nullptr;
    HWND _hFontNameCombo = nullptr;
    HWND _hFontSizeCombo = nullptr;
	HWND _hSwitch2ThemeCombo = nullptr;

	HWND _hFgColourStaticText = nullptr;
	HWND _hBgColourStaticText = nullptr;
	HWND _hFontNameStaticText = nullptr;
	HWND _hFontSizeStaticText = nullptr;
	HWND _hStyleInfoStaticText = nullptr;

	URLCtrl _goToSettings;
	URLCtrl _globalOverrideLinkTip;
	HWND _globalOverrideTip = nullptr;

	LexerStylerArray _lsArray;
    StyleArray _globalStyles;
	std::wstring _themeName;

	LexerStylerArray _styles2restored;
	StyleArray _gstyles2restored;
	GlobalOverride _gOverride2restored;
	bool _restoreInvalid = false;

	bool _isDirty = false;
	bool _isThemeDirty = false;
	bool _isShownGOCtrls = false;
	bool _isThemeChanged = false;

	std::pair<intptr_t, intptr_t> goToPreferencesSettings();

	intptr_t CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam) override;

	Style& getCurrentStyler();

	bool getStyleName(std::wstring& styleName, const size_t styleNameLenLimit = 128) const;

	int whichTabColourIndex() const;
	int whichIndividualTabColourId();
	int whichFindDlgStatusMsgColourIndex();
	void apply(int applicationInfo);
	int getApplicationInfo() const;
	bool isDocumentMapStyle();
	void move2CtrlRight(int ctrlID, HWND handle2Move, int handle2MoveWidth, int handle2MoveHeight);
	void updateColour(bool which);
	void updateFontStyleStatus(fontStyleType whitchStyle);
	void updateExtension();
	void updateFontName();
	void updateFontSize();
	void updateUserKeywords();
	void switchToTheme();
	void updateThemeName(const std::wstring& themeName);
	void loadLangListFromNppParam();
	void enableFontStyle(bool isEnable) const;
	long notifyDataModified();
	void setStyleListFromLexer(int index);
    void setVisualFromStyleList();
	void updateGlobalOverrideCtrls();
	void showGlobalOverrideCtrls(bool show);
	void applyCurrentSelectedThemeAndUpdateUI();
};
