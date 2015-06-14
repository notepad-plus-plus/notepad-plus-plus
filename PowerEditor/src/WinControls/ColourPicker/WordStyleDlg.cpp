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


#include <Shlobj.h>
#include <shlwapi.h>
#include <uxtheme.h>
#include "WordStyleDlg.h"
#include "ScintillaEditView.h"

using namespace std;

LRESULT CALLBACK ColourStaticTextHooker::colourStaticProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
    {
        case WM_PAINT:
        {
	        RECT rect;
            ::GetClientRect(hwnd, &rect);

            PAINTSTRUCT ps;
            HDC hdc = ::BeginPaint(hwnd, &ps);
    		
            ::SetTextColor(hdc, _colour);

		    // Get the default GUI font
            HFONT hf = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);

			HANDLE hOld = SelectObject(hdc, hf);

		    // Draw the text!
            TCHAR text[MAX_PATH];
            ::GetWindowText(hwnd, text, MAX_PATH);
            ::DrawText(hdc, text, -1, &rect, DT_LEFT);
    		
            ::SelectObject(hdc, hOld);

            ::EndPaint(hwnd, &ps);

		    return TRUE;
        }
    }
    return ::CallWindowProc(_oldProc, hwnd, Message, wParam, lParam);
}
void WordStyleDlg::updateGlobalOverrideCtrls()
{
	const NppGUI & nppGUI = (NppParameters::getInstance())->getNppGUI();
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_FG_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableFg, 0);
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_BG_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableBg, 0);
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_FONT_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableFont, 0);
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_FONTSIZE_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableFontSize, 0);
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_BOLD_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableBold, 0);
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_ITALIC_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableItalic, 0);
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_UNDERLINE_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableUnderLine, 0);
}

INT_PTR CALLBACK WordStyleDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG :
		{
			NppParameters *nppParamInst = NppParameters::getInstance();

            _hCheckBold = ::GetDlgItem(_hSelf, IDC_BOLD_CHECK);
            _hCheckItalic = ::GetDlgItem(_hSelf, IDC_ITALIC_CHECK);
			_hCheckUnderline = ::GetDlgItem(_hSelf, IDC_UNDERLINE_CHECK);
			_hFontNameCombo = ::GetDlgItem(_hSelf, IDC_FONT_COMBO);
			_hFontSizeCombo = ::GetDlgItem(_hSelf, IDC_FONTSIZE_COMBO);
			_hSwitch2ThemeCombo = ::GetDlgItem(_hSelf, IDC_SWITCH2THEME_COMBO);

			_hFgColourStaticText = ::GetDlgItem(_hSelf, IDC_FG_STATIC);
			_hBgColourStaticText = ::GetDlgItem(_hSelf, IDC_BG_STATIC);
			_hFontNameStaticText = ::GetDlgItem(_hSelf, IDC_FONTNAME_STATIC);
			_hFontSizeStaticText = ::GetDlgItem(_hSelf, IDC_FONTSIZE_STATIC);
			_hStyleInfoStaticText = ::GetDlgItem(_hSelf, IDC_STYLEDESCRIPTION_STATIC);

			colourHooker.setColour(RGB(0xFF, 0x00, 0x00));
			colourHooker.hookOn(_hStyleInfoStaticText);

			_currentThemeIndex = -1;
			int defaultThemeIndex = 0;
			ThemeSwitcher & themeSwitcher = nppParamInst->getThemeSwitcher();
			for(size_t i = 0 ; i < themeSwitcher.size() ; ++i)
			{
				pair<generic_string, generic_string> & themeInfo = themeSwitcher.getElementFromIndex(i);
				int j = ::SendMessage(_hSwitch2ThemeCombo, CB_ADDSTRING, 0, (LPARAM)themeInfo.first.c_str());
				if (! themeInfo.second.compare( nppParamInst->getNppGUI()._themeName ) ) 
				{
					_currentThemeIndex = j;
					_themeName.assign(themeInfo.second);
				}
				if (! themeInfo.first.compare(TEXT("Default")) ) 
				{
					defaultThemeIndex = j;
				}
			}
			if (_currentThemeIndex == -1)
			{
				_currentThemeIndex = defaultThemeIndex;
			}
			::SendMessage(_hSwitch2ThemeCombo, CB_SETCURSEL, _currentThemeIndex, 0);

			for(int i = 0 ; i < sizeof(fontSizeStrs)/(3*sizeof(TCHAR)) ; ++i)
				::SendMessage(_hFontSizeCombo, CB_ADDSTRING, 0, (LPARAM)fontSizeStrs[i]);

			const std::vector<generic_string> & fontlist = (NppParameters::getInstance())->getFontList();
			for (size_t i = 0, len = fontlist.size() ; i < len ; ++i)
			{
				int j = ::SendMessage(_hFontNameCombo, CB_ADDSTRING, 0, (LPARAM)fontlist[i].c_str());
				::SendMessage(_hFontNameCombo, CB_SETITEMDATA, j, (LPARAM)fontlist[i].c_str());
			}

			_pFgColour = new ColourPicker;
			_pBgColour = new ColourPicker;
			_pFgColour->init(_hInst, _hSelf);
			_pBgColour->init(_hInst, _hSelf);

            POINT p1, p2;

            alignWith(_hFgColourStaticText, _pFgColour->getHSelf(), ALIGNPOS_RIGHT, p1);
            alignWith(_hBgColourStaticText, _pBgColour->getHSelf(), ALIGNPOS_RIGHT, p2);

            p1.x = p2.x = ((p1.x > p2.x)?p1.x:p2.x) + 10;
            p1.y -= 4; p2.y -= 4;

            ::MoveWindow((HWND)_pFgColour->getHSelf(), p1.x, p1.y, 25, 25, TRUE);
            ::MoveWindow((HWND)_pBgColour->getHSelf(), p2.x, p2.y, 25, 25, TRUE);
			_pFgColour->display();
			_pBgColour->display();


			::EnableWindow(::GetDlgItem(_hSelf, IDOK), _isDirty);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_SAVECLOSE_BUTTON), FALSE/*!_isSync*/);

			ETDTProc enableDlgTheme = (ETDTProc)nppParamInst->getEnableThemeDlgTexture();
			if (enableDlgTheme)
			{
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);
				redraw();
			}

			updateGlobalOverrideCtrls();
			setVisualFromStyleList();
			goToCenter();

			loadLangListFromNppParam();

			return TRUE;
		}

		case WM_DESTROY:
		{
			_pFgColour->destroy();
			_pBgColour->destroy();
			delete _pFgColour;
			delete _pBgColour;
			return TRUE;
		}

		case WM_HSCROLL :
		{
			if ((HWND)lParam == ::GetDlgItem(_hSelf, IDC_SC_PERCENTAGE_SLIDER))
			{
				int percent = ::SendDlgItemMessage(_hSelf, IDC_SC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0);
				(NppParameters::getInstance())->SetTransparent(_hSelf, percent);
			}
			return TRUE;
		}

		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == EN_CHANGE)
            {
				int editID = LOWORD(wParam);
				if (editID == IDC_USER_KEYWORDS_EDIT)
				{
					updateUserKeywords();
					notifyDataModified();
					apply();
				}
				else if (editID == IDC_USER_EXT_EDIT)
				{
					updateExtension();
					notifyDataModified();
					apply();
				}
			}
			else
			{
				switch (wParam)
				{
					case IDC_BOLD_CHECK :
						updateFontStyleStatus(BOLD_STATUS);
						notifyDataModified();
						apply();
						break;

					case IDC_ITALIC_CHECK :
						updateFontStyleStatus(ITALIC_STATUS);
						notifyDataModified();
						apply();
						break;

					case IDC_UNDERLINE_CHECK :
						updateFontStyleStatus(UNDERLINE_STATUS);
						notifyDataModified();
						apply();
						break;

					case IDCANCEL :
						//::MessageBox(NULL, TEXT("cancel"), TEXT(""), MB_OK);
						if (_isDirty)
						{
							NppParameters *nppParamInst = NppParameters::getInstance();
							if (_restoreInvalid) 
							{	
								generic_string str( nppParamInst->getNppGUI()._themeName );
								nppParamInst->reloadStylers( &str[0] );
							}

							LexerStylerArray & lsArray = nppParamInst->getLStylerArray();
							StyleArray & globalStyles = nppParamInst->getGlobalStylers();
							
							if (_restoreInvalid) 
							{
								_lsArray = _styles2restored = lsArray;
								_globalStyles = _gstyles2restored = globalStyles;
							}
							else 
							{
								globalStyles = _globalStyles = _gstyles2restored;
								lsArray = _lsArray = _styles2restored;
							}

							restoreGlobalOverrideValues();

							_restoreInvalid = false;
							_isDirty = false;
							_isThemeDirty = false;
							setVisualFromStyleList();

							
							//(nppParamInst->getNppGUI())._themeName
							::SendMessage(_hSwitch2ThemeCombo, CB_SETCURSEL, _currentThemeIndex, 0);
							::SendMessage(_hParent, WM_UPDATESCINTILLAS, 0, 0);
						}
						::EnableWindow(::GetDlgItem(_hSelf, IDC_SAVECLOSE_BUTTON), FALSE/*!_isSync*/);
						display(false);
						return TRUE;

					case IDC_SAVECLOSE_BUTTON :
					{
						if (_isDirty)
						{
							LexerStylerArray & lsa = (NppParameters::getInstance())->getLStylerArray();
							StyleArray & globalStyles = (NppParameters::getInstance())->getGlobalStylers();

							_lsArray = lsa;
							_globalStyles = globalStyles;
							updateThemeName(_themeName);
							_restoreInvalid = false;

							_currentThemeIndex = ::SendMessage(_hSwitch2ThemeCombo, CB_GETCURSEL, 0, 0);
							::EnableWindow(::GetDlgItem(_hSelf, IDOK), FALSE);
							_isDirty = false;
						}
						_isThemeDirty = false;
						(NppParameters::getInstance())->writeStyles(_lsArray, _globalStyles);
						::EnableWindow(::GetDlgItem(_hSelf, IDC_SAVECLOSE_BUTTON), FALSE);
						//_isSync = true;
						display(false);
						::SendMessage(_hParent, WM_UPDATESCINTILLAS, 0, 0);
						return TRUE;
					}
					
					case IDC_SC_TRANSPARENT_CHECK :
					{
						bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_SC_TRANSPARENT_CHECK, BM_GETCHECK, 0, 0));
						if (isChecked)
						{
							int percent = ::SendDlgItemMessage(_hSelf, IDC_SC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0);
							(NppParameters::getInstance())->SetTransparent(_hSelf, percent);
						}
						else
							(NppParameters::getInstance())->removeTransparent(_hSelf);

						::EnableWindow(::GetDlgItem(_hSelf, IDC_SC_PERCENTAGE_SLIDER), isChecked);
						return TRUE;
					}

					case IDC_GLOBAL_FG_CHECK :
					{
						GlobalOverride & glo = (NppParameters::getInstance())->getGlobalOverrideStyle();
						glo.enableFg = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply();
						return TRUE;
					}

					case  IDC_GLOBAL_BG_CHECK:
					{
						GlobalOverride & glo = (NppParameters::getInstance())->getGlobalOverrideStyle();
						glo.enableBg = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply();
						return TRUE;
					}

					case IDC_GLOBAL_FONT_CHECK :
					{
						GlobalOverride & glo = (NppParameters::getInstance())->getGlobalOverrideStyle();
						glo.enableFont = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply();
						return TRUE;
					}
					case IDC_GLOBAL_FONTSIZE_CHECK :
					{
						GlobalOverride & glo = (NppParameters::getInstance())->getGlobalOverrideStyle();
						glo.enableFontSize = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply();
						return TRUE;
					}
					case IDC_GLOBAL_BOLD_CHECK :
					{
						GlobalOverride & glo = (NppParameters::getInstance())->getGlobalOverrideStyle();
						glo.enableBold = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply();
						return TRUE;
					}
					
					case IDC_GLOBAL_ITALIC_CHECK :
					{
						GlobalOverride & glo = (NppParameters::getInstance())->getGlobalOverrideStyle();
						glo.enableItalic = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply();
						return TRUE;
					}
					case IDC_GLOBAL_UNDERLINE_CHECK :
					{
						GlobalOverride & glo = (NppParameters::getInstance())->getGlobalOverrideStyle();
						glo.enableUnderLine = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply();
						return TRUE;
					}

					default:
						switch (HIWORD(wParam))
						{
							case CBN_SELCHANGE : // == case LBN_SELCHANGE :
							{
								switch (LOWORD(wParam))
								{
									case IDC_FONT_COMBO :
										updateFontName();
										notifyDataModified();
										apply();
										break;
									case IDC_FONTSIZE_COMBO :
										updateFontSize();
										notifyDataModified();
										apply();
										break;
									case IDC_LANGUAGES_LIST :
									{
										int i = ::SendDlgItemMessage(_hSelf, LOWORD(wParam), LB_GETCURSEL, 0, 0);
										if (i != LB_ERR)
										{
											bool prevThemeState = _isThemeDirty;
											setStyleListFromLexer(i);
											_isThemeDirty = prevThemeState;
										}
										break;
									}
									case IDC_STYLES_LIST :
										setVisualFromStyleList();
										break;

									case IDC_SWITCH2THEME_COMBO :
										switchToTheme();
										setVisualFromStyleList();
										notifyDataModified();
										_isThemeDirty = false;
										apply();
										break;
								}
								return TRUE;
							}

							case CPN_COLOURPICKED:	
							{
								if ((HWND)lParam == _pFgColour->getHSelf())
								{
									updateColour(C_FOREGROUND);
									notifyDataModified();
									int tabColourIndex;
									if ((tabColourIndex = whichTabColourIndex()) != -1)
									{
										TabBarPlus::setColour(_pFgColour->getColour(), (TabBarPlus::tabColourIndex)tabColourIndex);
									}
									apply();
									return TRUE;
								}
								else if ((HWND)lParam == _pBgColour->getHSelf())
								{
									updateColour(C_BACKGROUND);
									notifyDataModified();
									int tabColourIndex;
									if ((tabColourIndex = whichTabColourIndex()) != -1)
									{
										tabColourIndex = (int)tabColourIndex == TabBarPlus::inactiveText? TabBarPlus::inactiveBg : tabColourIndex;
										TabBarPlus::setColour(_pBgColour->getColour(), (TabBarPlus::tabColourIndex)tabColourIndex);
									}
									apply();
									return TRUE;
								}
								else
									return FALSE;
							}

							default :
							{
								return FALSE;
							}
						}
						//return TRUE;
				}
			}

		}
		default :
			return FALSE;
	}
	//return FALSE;
}

void WordStyleDlg::loadLangListFromNppParam()
{
	NppParameters *nppParamInst = NppParameters::getInstance();
	_lsArray = nppParamInst->getLStylerArray();
    _globalStyles = nppParamInst->getGlobalStylers();

	// Clean up Language List
	::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_LIST, LB_RESETCONTENT, 0, 0);

	::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_LIST, LB_ADDSTRING, 0, (LPARAM)TEXT("Global Styles"));
	// All the lexers
    for (int i = 0, nb = _lsArray.getNbLexer() ; i < nb ; ++i)
    {
		::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_LIST, LB_ADDSTRING, 0, (LPARAM)_lsArray.getLexerDescFromIndex(i));
    }

	const int index2Begin = 0;
	::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_LIST, LB_SETCURSEL, 0, index2Begin);
	setStyleListFromLexer(index2Begin);
}

void WordStyleDlg::updateThemeName(generic_string themeName)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )pNppParam->getNppGUI();
	nppGUI._themeName.assign( themeName );
}

int WordStyleDlg::whichTabColourIndex() 
{
	int i = ::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETCURSEL, 0, 0);
	if (i == LB_ERR)
		return -1;
	TCHAR styleName[128];
	::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETTEXT, i, (LPARAM)styleName);

	if (lstrcmp(styleName, TABBAR_ACTIVEFOCUSEDINDCATOR) == 0)
		return (int)TabBarPlus::activeFocusedTop;

	if (lstrcmp(styleName, TABBAR_ACTIVEUNFOCUSEDINDCATOR) == 0)
		return (int)TabBarPlus::activeUnfocusedTop;

	if (lstrcmp(styleName, TABBAR_ACTIVETEXT) == 0)
		return (int)TabBarPlus::activeText;

	if (lstrcmp(styleName, TABBAR_INACTIVETEXT) == 0)
		return (int)TabBarPlus::inactiveText;

	return -1;
}

void WordStyleDlg::updateColour(bool which)
{
	Style & style = getCurrentStyler();
	if (which == C_FOREGROUND)
	{
		style._fgColor = _pFgColour->getColour();
		if (_pFgColour->isEnabled())
			style._colorStyle |= COLORSTYLE_FOREGROUND;
		else
			style._colorStyle &= ~COLORSTYLE_FOREGROUND;
	}
	else //(which == C_BACKGROUND)
	{
		style._bgColor = _pBgColour->getColour();
		if (_pBgColour->isEnabled())
			style._colorStyle |= COLORSTYLE_BACKGROUND;
		else
			style._colorStyle &= ~COLORSTYLE_BACKGROUND;
	}
}

void WordStyleDlg::updateFontSize()
{
	Style & style = getCurrentStyler();
	int iFontSizeSel = ::SendMessage(_hFontSizeCombo, CB_GETCURSEL, 0, 0);

	TCHAR intStr[5];
	if (iFontSizeSel != 0)
	{
		::SendMessage(_hFontSizeCombo, CB_GETLBTEXT, iFontSizeSel, (LPARAM)intStr);
		if (!intStr[0])
			style._fontSize = STYLE_NOT_USED;
		else
		{
			TCHAR *finStr;
			style._fontSize = generic_strtol(intStr, &finStr, 10);
			if (*finStr != '\0')
				style._fontSize = STYLE_NOT_USED;
		}
	}
	else
		style._fontSize = 0;
}

void WordStyleDlg::updateExtension()
{
	const int NB_MAX = 256;
	TCHAR ext[NB_MAX];
	::SendDlgItemMessage(_hSelf, IDC_USER_EXT_EDIT, WM_GETTEXT, NB_MAX, (LPARAM)ext);
	_lsArray.getLexerFromIndex(_currentLexerIndex - 1).setLexerUserExt(ext);
}

void WordStyleDlg::updateUserKeywords()
{
	Style & style = getCurrentStyler();
	//const int NB_MAX = 2048;
	//TCHAR kw[NB_MAX];
	int len = ::SendDlgItemMessage(_hSelf, IDC_USER_KEYWORDS_EDIT, WM_GETTEXTLENGTH, 0, 0);
	len +=1;
	TCHAR *kw = new TCHAR[len];
	::SendDlgItemMessage(_hSelf, IDC_USER_KEYWORDS_EDIT, WM_GETTEXT, len, (LPARAM)kw);
	style.setKeywords(kw);

	delete [] kw;
}

void WordStyleDlg::updateFontName()
{
    Style & style = getCurrentStyler();
	int iFontSel = ::SendMessage(_hFontNameCombo, CB_GETCURSEL, 0, 0);
	TCHAR *fnStr = (TCHAR *)::SendMessage(_hFontNameCombo, CB_GETITEMDATA, iFontSel, 0);
	style._fontName = fnStr;
}

void WordStyleDlg::updateFontStyleStatus(fontStyleType whitchStyle)
{
    Style & style = getCurrentStyler();
	if (style._fontStyle == STYLE_NOT_USED)
		style._fontStyle = FONTSTYLE_NONE;

	int fontStyle = FONTSTYLE_UNDERLINE;
	HWND hWnd = _hCheckUnderline;

	if (whitchStyle == BOLD_STATUS)
	{
		fontStyle = FONTSTYLE_BOLD;
		hWnd = _hCheckBold;
	}
	if (whitchStyle == ITALIC_STATUS)
	{
		fontStyle = FONTSTYLE_ITALIC;
		hWnd = _hCheckItalic;
	}

	int isChecked = ::SendMessage(hWnd, BM_GETCHECK, 0, 0);
	if (isChecked != BST_INDETERMINATE)
	{
		if (isChecked == BST_CHECKED)
			style._fontStyle |= fontStyle;
		else
			style._fontStyle &= ~fontStyle;
	}
}

void WordStyleDlg::switchToTheme()
{
	int iSel = ::SendMessage(_hSwitch2ThemeCombo, CB_GETCURSEL, 0, 0);

	generic_string prevThemeName(_themeName);
	_themeName.clear();
	
	NppParameters *nppParamInst = NppParameters::getInstance();
    ThemeSwitcher & themeSwitcher = nppParamInst->getThemeSwitcher();
	pair<generic_string, generic_string> & themeInfo = themeSwitcher.getElementFromIndex(iSel);
    _themeName = themeInfo.second;

	if (_isThemeDirty)
	{
		TCHAR themeFileName[MAX_PATH];
		lstrcpy(themeFileName, prevThemeName.c_str());
		PathStripPath(themeFileName);
		PathRemoveExtension(themeFileName);
		int mb_response =
			::MessageBox( _hSelf,
				TEXT(" Unsaved changes are about to be discarded!\n") 
				TEXT(" Do you want to save your changes before switching themes?"),
				themeFileName,
				MB_ICONWARNING | MB_YESNO | MB_APPLMODAL | MB_SETFOREGROUND );
		if ( mb_response == IDYES )
			(NppParameters::getInstance())->writeStyles(_lsArray, _globalStyles);
	}
	nppParamInst->reloadStylers(&_themeName[0]);

	loadLangListFromNppParam();
	_restoreInvalid = true;

}

void WordStyleDlg::setStyleListFromLexer(int index)
{
    _currentLexerIndex = index;

    // Fill out Styles listbox
    // Before filling out, we clean it
	::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_RESETCONTENT, 0, 0);

	if (index)
	{
		const TCHAR *langName = _lsArray.getLexerNameFromIndex(index - 1);
		const TCHAR *ext = NppParameters::getInstance()->getLangExtFromName(langName);
		const TCHAR *userExt = (_lsArray.getLexerStylerByName(langName))->getLexerUserExt();
		::SendDlgItemMessage(_hSelf, IDC_DEF_EXT_EDIT, WM_SETTEXT, 0, (LPARAM)(ext));

        // WM_SETTEXT cause sending WM_COMMAND message with EN_CHANGE.
        // That makes status dirty, even it shouldn't in this case.
        // The walk around solution is get the current status before sending WM_SETTEXT,
        // then restore the status after sending this message.
        bool isDirty = _isDirty;
		bool isThemeDirty = _isThemeDirty;
		::SendDlgItemMessage(_hSelf, IDC_USER_EXT_EDIT, WM_SETTEXT, 0, (LPARAM)(userExt));
        _isDirty = isDirty;
        _isThemeDirty = isThemeDirty;
        ::EnableWindow(::GetDlgItem(_hSelf, IDC_SAVECLOSE_BUTTON), isDirty || isThemeDirty);
	}
	::ShowWindow(::GetDlgItem(_hSelf, IDC_DEF_EXT_EDIT), index?SW_SHOW:SW_HIDE);
    ::ShowWindow(::GetDlgItem(_hSelf, IDC_DEF_EXT_STATIC), index?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_USER_EXT_EDIT), index?SW_SHOW:SW_HIDE);
    ::ShowWindow(::GetDlgItem(_hSelf, IDC_USER_EXT_STATIC), index?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_PLUSSYMBOL2_STATIC), index?SW_SHOW:SW_HIDE);

	StyleArray & lexerStyler = index?_lsArray.getLexerFromIndex(index-1):_globalStyles;

    for (int i = 0, nb = lexerStyler.getNbStyler(); i < nb ; ++i)
    {
        Style & style = lexerStyler.getStyler(i);
		::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_ADDSTRING, 0, (LPARAM)style._styleDesc);
    }
	::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_SETCURSEL, 0, 0);
    setVisualFromStyleList();
}

void WordStyleDlg::setVisualFromStyleList() 
{
	showGlobalOverrideCtrls(false);

    Style & style = getCurrentStyler();

	// Global override style
	if (style._styleDesc && lstrcmp(style._styleDesc, TEXT("Global override")) == 0)
	{
		showGlobalOverrideCtrls(true);
	}

    //--Warning text
    //bool showWarning = ((_currentLexerIndex == 0) && (style._styleID == STYLE_DEFAULT));//?SW_SHOW:SW_HIDE;

	COLORREF c = RGB(0x00, 0x00, 0xFF);
	TCHAR str[256];

	str[0] = '\0';
	
	int i = ::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_LIST, LB_GETCURSEL, 0, 0);
	if (i == LB_ERR)
		return;
	::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_LIST, LB_GETTEXT, i, (LPARAM)str);

	i = ::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETCURSEL, 0, 0);
	if (i == LB_ERR)
		return;
	TCHAR styleName[64];
	::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETTEXT, i, (LPARAM)styleName);

	lstrcat(lstrcat(str, TEXT(" : ")), styleName);

	// PAD for fix a display glitch
	lstrcat(str, TEXT("          "));
	colourHooker.setColour(c);
	::SetWindowText(_hStyleInfoStaticText, str);

	//-- 2 couleurs : fg et bg
	bool isEnable = false;
	if (HIBYTE(HIWORD(style._fgColor)) != 0xFF)
	{
		_pFgColour->setColour(style._fgColor);
		_pFgColour->setEnabled((style._colorStyle & COLORSTYLE_FOREGROUND) != 0);
		isEnable = true;
	}
	enableFg(isEnable);

	isEnable = false;
	if (HIBYTE(HIWORD(style._bgColor)) != 0xFF)
	{
		_pBgColour->setColour(style._bgColor);
		_pBgColour->setEnabled((style._colorStyle & COLORSTYLE_BACKGROUND) != 0);
		isEnable = true;
	}
	enableBg(isEnable);

	//-- font name
	isEnable = false;
	int iFontName;
	if (style._fontName != NULL)
	{
		iFontName = ::SendMessage(_hFontNameCombo, CB_FINDSTRING, 1, (LPARAM)style._fontName);
		if (iFontName == CB_ERR)
			iFontName = 0;
		isEnable = true;
	}
	else
	{
		iFontName = 0;
	}
	::SendMessage(_hFontNameCombo, CB_SETCURSEL, iFontName, 0);
	enableFontName(isEnable);

	//-- font size
	isEnable = false;
	TCHAR intStr[5] = TEXT("");
	int iFontSize = 0;
	if (style._fontSize != STYLE_NOT_USED)
	{
		wsprintf(intStr, TEXT("%d"), style._fontSize);
		iFontSize = ::SendMessage(_hFontSizeCombo, CB_FINDSTRING, 1, (LPARAM)intStr);
		isEnable = true;
	}
	::SendMessage(_hFontSizeCombo, CB_SETCURSEL, iFontSize, 0);
	enableFontSize(isEnable);

	//-- font style : bold & italic
	isEnable = false;
	int isBold, isItalic, isUnderline;
	if (style._fontStyle != STYLE_NOT_USED)
	{
		isBold = (style._fontStyle & FONTSTYLE_BOLD)?BST_CHECKED:BST_UNCHECKED;
		isItalic = (style._fontStyle & FONTSTYLE_ITALIC)?BST_CHECKED:BST_UNCHECKED;
		isUnderline = (style._fontStyle & FONTSTYLE_UNDERLINE)?BST_CHECKED:BST_UNCHECKED;
		::SendMessage(_hCheckBold, BM_SETCHECK, isBold, 0);
		::SendMessage(_hCheckItalic, BM_SETCHECK, isItalic, 0);
		::SendMessage(_hCheckUnderline, BM_SETCHECK, isUnderline, 0);
		isEnable = true;
	}
	else // STYLE_NOT_USED : reset them all
	{
		::SendMessage(_hCheckBold, BM_SETCHECK, BST_UNCHECKED, 0);
		::SendMessage(_hCheckItalic, BM_SETCHECK, BST_UNCHECKED, 0);
		::SendMessage(_hCheckUnderline, BM_SETCHECK, BST_UNCHECKED, 0);
	}

    enableFontStyle(isEnable);


	//-- Default Keywords
	bool shouldBeDisplayed = style._keywordClass != STYLE_NOT_USED;
	if (shouldBeDisplayed)
	{
		LexerStyler & lexerStyler = _lsArray.getLexerFromIndex(_currentLexerIndex - 1);

		NppParameters *pNppParams = NppParameters::getInstance();
		LangType lType = pNppParams->getLangIDFromStr(lexerStyler.getLexerName());
		if (lType == L_TEXT)
		{
			generic_string str = lexerStyler.getLexerName();
			str += TEXT(" is not defined in NppParameters::getLangIDFromStr()");
				printStr(str.c_str());
		}
		const TCHAR *kws = pNppParams->getWordList(lType, style._keywordClass);
		if (!kws)
			kws = TEXT("");
		::SendDlgItemMessage(_hSelf, IDC_DEF_KEYWORDS_EDIT, WM_SETTEXT, 0, (LPARAM)(kws));

		const TCHAR *ckwStr = (style._keywords)?style._keywords->c_str():TEXT("");
		::SendDlgItemMessage(_hSelf, IDC_USER_KEYWORDS_EDIT, WM_SETTEXT, 0, (LPARAM)(ckwStr));
	}
	int showOption = shouldBeDisplayed?SW_SHOW:SW_HIDE;
	::ShowWindow(::GetDlgItem(_hSelf, IDC_DEF_KEYWORDS_EDIT), showOption);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_USER_KEYWORDS_EDIT),showOption);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_DEF_KEYWORDS_STATIC), showOption);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_USER_KEYWORDS_STATIC),showOption);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_PLUSSYMBOL_STATIC),showOption);

    redraw();
}

void WordStyleDlg::create(int dialogID, bool isRTL)
{
	StaticDialog::create(dialogID, isRTL);

	if ((NppParameters::getInstance())->isTransparentAvailable())
	{
		::ShowWindow(::GetDlgItem(_hSelf, IDC_SC_TRANSPARENT_CHECK), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_SC_PERCENTAGE_SLIDER), SW_SHOW);
		
		::SendDlgItemMessage(_hSelf, IDC_SC_PERCENTAGE_SLIDER, TBM_SETRANGE, FALSE, MAKELONG(20, 200));
		::SendDlgItemMessage(_hSelf, IDC_SC_PERCENTAGE_SLIDER, TBM_SETPOS, TRUE, 150);
		if (!(BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_SC_PERCENTAGE_SLIDER, BM_GETCHECK, 0, 0)))
			::EnableWindow(::GetDlgItem(_hSelf, IDC_SC_PERCENTAGE_SLIDER), FALSE);
	}
}

void WordStyleDlg::apply()
{
	LexerStylerArray & lsa = (NppParameters::getInstance())->getLStylerArray();
	StyleArray & globalStyles = (NppParameters::getInstance())->getGlobalStylers();

	lsa = _lsArray;
	globalStyles = _globalStyles;

	::EnableWindow(::GetDlgItem(_hSelf, IDOK), FALSE);
	::SendMessage(_hParent, WM_UPDATESCINTILLAS, 0, 0);
}
