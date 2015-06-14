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


#ifndef WORD_STYLE_H
#define WORD_STYLE_H

#include "ColourPicker.h"
#include "WordStyleDlgRes.h"
#include "Parameters.h"


#define WM_UPDATESCINTILLAS			(WORDSTYLE_USER + 1) //GlobalStyleDlg's msg 2 send 2 its parent

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
		::SetWindowLongPtr(staticHandle, GWLP_USERDATA, (LONG_PTR)this);
		_oldProc = (WNDPROC)::SetWindowLongPtr(staticHandle, GWLP_WNDPROC, (LONG_PTR)staticProc);
	};
private :
	COLORREF _colour;
	WNDPROC _oldProc;

	static LRESULT CALLBACK staticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
		ColourStaticTextHooker *pColourStaticTextHooker = reinterpret_cast<ColourStaticTextHooker *>(::GetWindowLongPtr(hwnd, GWL_USERDATA));
		return pColourStaticTextHooker->colourStaticProc(hwnd, message, wParam, lParam);
	}; 
	LRESULT CALLBACK colourStaticProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
};

class WordStyleDlg : public StaticDialog
{
public :
	WordStyleDlg():_isDirty(false), _isThemeDirty(false), _restoreInvalid(false), /*_isSync(true),*/ _isShownGOCtrls(false){};

    void init(HINSTANCE hInst, HWND parent)	{
        Window::init(hInst, parent);
	};

	virtual void create(int dialogID, bool isRTL = false);

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
		_styles2restored = (NppParameters::getInstance())->getLStylerArray();
		_gstyles2restored = (NppParameters::getInstance())->getGlobalStylers();
		_gOverride2restored = (NppParameters::getInstance())->getGlobalOverrideStyle();
	};

    virtual void redraw() const {
        _pFgColour->redraw();
        _pBgColour->redraw();
		::InvalidateRect(_hStyleInfoStaticText, NULL, TRUE);
		::UpdateWindow(_hStyleInfoStaticText);
    };
	
	void restoreGlobalOverrideValues() {
		GlobalOverride & gOverride = (NppParameters::getInstance())->getGlobalOverrideStyle();
		gOverride = _gOverride2restored;
	};

	void apply();

	void addLastThemeEntry() {
        NppParameters *nppParamInst = NppParameters::getInstance();
        ThemeSwitcher & themeSwitcher = nppParamInst->getThemeSwitcher();
		std::pair<generic_string, generic_string> & themeInfo = themeSwitcher.getElementFromIndex(themeSwitcher.size() - 1);
	    ::SendMessage(_hSwitch2ThemeCombo, CB_ADDSTRING, 0, (LPARAM)themeInfo.first.c_str());
    };



private :
    ColourPicker *_pFgColour;
    ColourPicker *_pBgColour;

    int _currentLexerIndex;
	int _currentThemeIndex;

    HWND _hCheckBold;
    HWND _hCheckItalic;
	HWND _hCheckUnderline;
    HWND _hFontNameCombo;
    HWND _hFontSizeCombo;
	HWND _hSwitch2ThemeCombo;

	HWND _hFgColourStaticText;
	HWND _hBgColourStaticText;
	HWND _hFontNameStaticText;
	HWND _hFontSizeStaticText;
	HWND _hStyleInfoStaticText;
	//TCHAR _originalWarning[256];

	LexerStylerArray _lsArray;
    StyleArray _globalStyles;
	generic_string _themeName;

	LexerStylerArray _styles2restored;
	StyleArray _gstyles2restored;
	GlobalOverride _gOverride2restored;
	bool _restoreInvalid;

	ColourStaticTextHooker colourHooker;

	bool _isDirty;
	bool _isThemeDirty;
    //bool _isSync;
	bool _isShownGOCtrls;

	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);


	Style & getCurrentStyler() {
		int styleIndex = ::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETCURSEL, 0, 0);
		if (styleIndex == LB_ERR) styleIndex = 0;

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

	int whichTabColourIndex();

	void updateColour(bool which);
	void updateFontStyleStatus(fontStyleType whitchStyle);
	void updateExtension();
	void updateFontName();
	void updateFontSize();
	void updateUserKeywords();
	void switchToTheme();
	void updateThemeName(generic_string themeName);

	void loadLangListFromNppParam();

	void enableFg(bool isEnable) {
		::EnableWindow(_pFgColour->getHSelf(), isEnable);
		::EnableWindow(_hFgColourStaticText, isEnable);
	};

	void enableBg(bool isEnable) {
		::EnableWindow(_pBgColour->getHSelf(), isEnable);
		::EnableWindow(_hBgColourStaticText, isEnable);
	};

	void enableFontName(bool isEnable) {
		::EnableWindow(_hFontNameCombo, isEnable);
		::EnableWindow(_hFontNameStaticText, isEnable);
	};

	void enableFontSize(bool isEnable) {
		::EnableWindow(_hFontSizeCombo, isEnable);
		::EnableWindow(_hFontSizeStaticText, isEnable);
	};

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
};

#endif //WORD_STYLE_H
