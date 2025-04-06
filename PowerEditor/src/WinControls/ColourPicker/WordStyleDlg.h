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

#include "ColourPicker.h"
#include "URLCtrl.h"
#include "WordStyleDlgRes.h"
#include "Parameters.h"


#define WM_UPDATESCINTILLAS      (WORDSTYLE_USER + 1) // WM_UPDATESCINTILLAS (BOOL doChangePanel, 0)
#define WM_UPDATEMAINMENUBITMAPS (WORDSTYLE_USER + 2)

// The following parameters are for apply() method which will re-initialize the followings GUI with modified styler:
// 2 Scintilla edit zones, Search result (displayed by Sintilla), Notepad++ GUI & components concerning theme
#define NO_VISUAL_CHANGE            0x00  // No need to apply visual effect - User ext.
#define GENERAL_CHANGE              0x01  // For Sintilla zones & Notepad++ GUI (Tabbar, Find dialog, etc...)
#define THEME_CHANGE                0x02  // For the components concerning theme, for example the background color of dockable panels 
#define COLOR_CHANGE_4_MENU         0x04  // For the color items displayed on the menu

const wchar_t FINDDLG_STAUSNOTFOUND_COLOR[64] = L"Find status: Not found";
const wchar_t FINDDLG_STAUSMESSAGE_COLOR[64] = L"Find status: Message";
const wchar_t FINDDLG_STAUSREACHED_COLOR[64] = L"Find status: Search end reached";

enum fontStyleType {BOLD_STATUS, ITALIC_STATUS, UNDERLINE_STATUS};

const bool C_FOREGROUND = false;
const bool C_BACKGROUND = true;


class ColourStaticTextHooker {
public :
	ColourStaticTextHooker() : _colour(RGB(0x00, 0x00, 0x00)) {};

	COLORREF setColour(COLORREF colour2Set) {
		COLORREF oldColour = _colour;
		_colour = colour2Set;
		return oldColour;
	};
	void hookOn(HWND staticHandle) {
		::SetWindowLongPtr(staticHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		_oldProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(staticHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(staticProc)));
	};
private :
	COLORREF _colour = RGB(0xFF, 0xFF, 0xFF);
	WNDPROC _oldProc = nullptr;

	static LRESULT CALLBACK staticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
		ColourStaticTextHooker *pColourStaticTextHooker = reinterpret_cast<ColourStaticTextHooker *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
		return pColourStaticTextHooker->colourStaticProc(hwnd, message, wParam, lParam);
	}; 
	LRESULT CALLBACK colourStaticProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
};

class WordStyleDlg : public StaticDialog
{
public :
	WordStyleDlg() = default;
	~WordStyleDlg() {
		_goToSettings.destroy();
		_globalOverrideLinkTip.destroy();

		if (_globalOverrideTip)
			::DestroyWindow(_globalOverrideTip);
	};

	void create(int dialogID, bool isRTL = false, bool msgDestParent = true) override;
    void doDialog(bool isRTL = false);
	void destroy() override;
	void prepare2Cancel();
	void redraw(bool forceUpdate = false) const override;
	void restoreGlobalOverrideValues();
	void addLastThemeEntry();
	bool selectThemeByName(const wchar_t* themeName);
	void syncWithSelFgSingleColorCtrl();
	bool goToSection(const wchar_t* sectionNames); // sectionNames is formed as following: "Language name:Style name"
	                                               // ex: "Global Styles:EOL custom color" will set Language on "Global Styles", then set Style on "EOL custom color" if both are found.

private :
    ColourPicker *_pFgColour = nullptr;
    ColourPicker *_pBgColour = nullptr;

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

	ColourStaticTextHooker _colourHooker;

	bool _isDirty = false;
	bool _isThemeDirty = false;
	bool _isShownGOCtrls = false;
	bool _isThemeChanged = false;

	std::pair<intptr_t, intptr_t> goToPreferencesSettings();

	intptr_t CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam) override;

	Style& getCurrentStyler();

	bool getStyleName(wchar_t *styleName, const size_t styleNameLen) const;

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
	void enableFontStyle(bool isEnable);
	long notifyDataModified();
	void setStyleListFromLexer(int index);
    void setVisualFromStyleList();
	void updateGlobalOverrideCtrls();
	void showGlobalOverrideCtrls(bool show);
	void applyCurrentSelectedThemeAndUpdateUI();
};
