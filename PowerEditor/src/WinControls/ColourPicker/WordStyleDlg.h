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


#define WM_UPDATESCINTILLAS      (WORDSTYLE_USER + 1) //GlobalStyleDlg's msg 2 send 2 its parent
#define WM_UPDATEMAINMENUBITMAPS (WORDSTYLE_USER + 2)

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

    void init(HINSTANCE hInst, HWND parent)	{
        Window::init(hInst, parent);
	};

	virtual void create(int dialogID, bool isRTL = false, bool msgDestParent = true);

    void doDialog(bool isRTL = false) {
    	if (!isCreated())
		{
			create(IDD_STYLER_DLG, isRTL);
			prepare2Cancel();
		}

		if (!::IsWindowVisible(_hSelf))
		{
			prepare2Cancel();
		}
	    display();
    };

	void prepare2Cancel() {
		_styles2restored = (NppParameters::getInstance()).getLStylerArray();
		_gstyles2restored = (NppParameters::getInstance()).getGlobalStylers();
		_gOverride2restored = (NppParameters::getInstance()).getGlobalOverrideStyle();
	};

    virtual void redraw(bool forceUpdate = false) const {
        _pFgColour->redraw(forceUpdate);
		_pBgColour->redraw(forceUpdate);
		::InvalidateRect(_hStyleInfoStaticText, NULL, TRUE);
		::UpdateWindow(_hStyleInfoStaticText);
    };
	
	void restoreGlobalOverrideValues() {
		GlobalOverride & gOverride = (NppParameters::getInstance()).getGlobalOverrideStyle();
		gOverride = _gOverride2restored;
	};

	void apply();

	void addLastThemeEntry() {
        NppParameters& nppParamInst = NppParameters::getInstance();
        ThemeSwitcher & themeSwitcher = nppParamInst.getThemeSwitcher();
		std::pair<generic_string, generic_string> & themeInfo = themeSwitcher.getElementFromIndex(themeSwitcher.size() - 1);
	    ::SendMessage(_hSwitch2ThemeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(themeInfo.first.c_str()));
    };

	bool selectThemeByName(const TCHAR* themeName);

	bool goToSection(const TCHAR* sectionNames); // sectionNames is formed as following: "Language name:Style name"
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

	LexerStylerArray _lsArray;
    StyleArray _globalStyles;
	generic_string _themeName;

	LexerStylerArray _styles2restored;
	StyleArray _gstyles2restored;
	GlobalOverride _gOverride2restored;
	bool _restoreInvalid = false;

	ColourStaticTextHooker _colourHooker;

	bool _isDirty = false;
	bool _isThemeDirty = false;
	bool _isShownGOCtrls = false;


	intptr_t CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);


	Style & getCurrentStyler() {
		int32_t styleIndex = static_cast<int32_t>(::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETCURSEL, 0, 0));
		if (styleIndex == LB_ERR)
			styleIndex = 0;

        if (_currentLexerIndex == 0)
		{
            return _globalStyles.getStyler(styleIndex);
		}
        else
        {
		    LexerStyler & lexerStyler = _lsArray.getLexerFromIndex(_currentLexerIndex - 1);
		    return lexerStyler.getStyler(styleIndex);
        }
	};

	bool getStyleName(TCHAR *styleName, const size_t styleNameLen);

	int whichTabColourIndex();
	bool isDocumentMapStyle();
	void move2CtrlRight(int ctrlID, HWND handle2Move, int handle2MoveWidth, int handle2MoveHeight);
	void updateColour(bool which);
	void updateFontStyleStatus(fontStyleType whitchStyle);
	void updateExtension();
	void updateFontName();
	void updateFontSize();
	void updateUserKeywords();
	void switchToTheme();
	void updateThemeName(const generic_string& themeName);
	std::pair<intptr_t, intptr_t> goToPreferencesSettings();

	void loadLangListFromNppParam();

	void enableFontStyle(bool isEnable) {
		::EnableWindow(_hCheckBold, isEnable);
		::EnableWindow(_hCheckItalic, isEnable);
		::EnableWindow(_hCheckUnderline, isEnable);
	};
    long notifyDataModified() {
		_isDirty = true;
		_isThemeDirty = true;
		::EnableWindow(::GetDlgItem(_hSelf, IDC_SAVECLOSE_BUTTON), TRUE);
		return TRUE;
    };
	void setStyleListFromLexer(int index);
    void setVisualFromStyleList();

	void updateGlobalOverrideCtrls();

	void showGlobalOverrideCtrls(bool show)	{
		if (show)
		{
			updateGlobalOverrideCtrls();
		}
		::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_FG_CHECK), show?SW_SHOW:SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_BG_CHECK), show?SW_SHOW:SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_FONT_CHECK), show?SW_SHOW:SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_FONTSIZE_CHECK), show?SW_SHOW:SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_BOLD_CHECK), show?SW_SHOW:SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_ITALIC_CHECK), show?SW_SHOW:SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_UNDERLINE_CHECK), show?SW_SHOW:SW_HIDE);
		_isShownGOCtrls = show;
	};

	void applyCurrentSelectedThemeAndUpdateUI();
};
