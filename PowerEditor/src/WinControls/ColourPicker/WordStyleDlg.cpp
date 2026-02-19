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


#include "WordStyleDlg.h"

#include <windows.h>

#include <shlwapi.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ColourPicker.h"
#include "Common.h"
#include "Notepad_plus_msgs.h"
#include "NppConstants.h"
#include "NppDarkMode.h"
#include "Parameters.h"
#include "StaticDialog.h"
#include "TabBar.h"
#include "WordStyleDlgRes.h"
#include "documentMap.h"
#include "dpiManagerV2.h"
#include "localization.h"
#include "preference_rc.h"
#include "resource.h"

// The following parameters are for apply() method which will re-initialize the followings GUI with modified styler:
// 2 Scintilla edit zones, Search result (displayed by Sintilla), Notepad++ GUI & components concerning theme
enum ApplyChange
{
	NO_VISUAL_CHANGE    = 0x00, // No need to apply visual effect - User ext.
	GENERAL_CHANGE      = 0x01, // For Sintilla zones & Notepad++ GUI (Tabbar, Find dialog, etc...)
	THEME_CHANGE        = 0x02, // For the components concerning theme, for example the background color of dockable panels 
	COLOR_CHANGE_4_MENU = 0x04, // For the color items displayed on the menu
};

static constexpr bool C_FOREGROUND = false;
static constexpr bool C_BACKGROUND = true;

using namespace std;

void WordStyleDlg::updateGlobalOverrideCtrls()
{
	const NppGUI & nppGUI = (NppParameters::getInstance()).getNppGUI();
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_FG_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableFg, 0);
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_BG_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableBg, 0);
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_FONT_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableFont, 0);
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_FONTSIZE_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableFontSize, 0);
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_BOLD_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableBold, 0);
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_ITALIC_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableItalic, 0);
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_UNDERLINE_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableUnderLine, 0);
}

intptr_t CALLBACK WordStyleDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG :
		{
			NppParameters& nppParamInst = NppParameters::getInstance();

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

			_currentThemeIndex = -1;
			int defaultThemeIndex = 0;
			ThemeSwitcher & themeSwitcher = nppParamInst.getThemeSwitcher();
			for (size_t i = 0 ; i < themeSwitcher.size() ; ++i)
			{
				pair<wstring, wstring> & themeInfo = themeSwitcher.getElementFromIndex(i);
				const auto j = static_cast<int>(::SendMessage(_hSwitch2ThemeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(themeInfo.first.c_str())));
				if (! themeInfo.second.compare( nppParamInst.getNppGUI()._themeName ) )
				{
					_currentThemeIndex = j;
					_themeName.assign(themeInfo.second);
				}
				if (! themeInfo.first.compare(L"Default") )
				{
					defaultThemeIndex = j;
				}
			}
			if (_currentThemeIndex == -1)
			{
				_currentThemeIndex = defaultThemeIndex;
			}
			::SendMessage(_hSwitch2ThemeCombo, CB_SETCURSEL, _currentThemeIndex, 0);

			for (size_t i = 0 ; i < sizeof(fontSizeStrs)/(3*sizeof(wchar_t)) ; ++i)
				::SendMessage(_hFontSizeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(fontSizeStrs[i]));

			const std::vector<wstring> & fontlist = nppParamInst.getFontList();
			for (size_t i = 0, len = fontlist.size() ; i < len ; ++i)
			{
				auto j = ::SendMessage(_hFontNameCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(fontlist[i].c_str()));
				::SendMessage(_hFontNameCombo, CB_SETITEMDATA, j, reinterpret_cast<LPARAM>(fontlist[i].c_str()));
			}

			_pFgColour = std::make_unique<ColourPicker>();
			_pBgColour = std::make_unique<ColourPicker>();
			_pFgColour->init(_hInst, _hSelf);
			_pBgColour->init(_hInst, _hSelf);

			setDpi();
			const int cpDynamicalSize = _dpiManager.scale(25);

			move2CtrlRight(IDC_FG_STATIC, _pFgColour->getHSelf(), cpDynamicalSize, cpDynamicalSize);
			move2CtrlRight(IDC_BG_STATIC, _pBgColour->getHSelf(), cpDynamicalSize, cpDynamicalSize);

			_pFgColour->display();
			_pBgColour->display();

			::EnableWindow(::GetDlgItem(_hSelf, IDOK), _isDirty);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_SAVECLOSE_BUTTON), FALSE/*!_isSync*/);

			loadLangListFromNppParam();
			updateGlobalOverrideCtrls();
			setVisualFromStyleList();

			_goToSettings.init(_hInst, _hSelf);
			_goToSettings.create(::GetDlgItem(_hSelf, IDC_GLOBAL_GOTOSETTINGS_LINK), L"");
			std::pair<intptr_t, intptr_t> pageAndCtrlID = goToPreferencesSettings();
			_goToSettings.display(pageAndCtrlID.first != -1);

			HWND hWhatIsGlobalOverride = ::GetDlgItem(_hSelf, IDC_GLOBAL_WHATISGLOBALOVERRIDE_LINK);
			_globalOverrideLinkTip.init(_hInst, _hSelf);
			_globalOverrideLinkTip.create(hWhatIsGlobalOverride, L"");

			const Style& style = getCurrentStyler();
			bool showWhatIsGlobalOverride = (style._styleDesc == L"Global override");
			_globalOverrideLinkTip.display(showWhatIsGlobalOverride);

			NativeLangSpeaker* pNativeSpeaker = nppParamInst.getNativeLangSpeaker();
			wstring globalOverrideTipStr = pNativeSpeaker->getLocalizedStrFromID("global-override-tip", L"Enabling \"Global override\" here will override that parameter in all language styles. What you probably really want is to use the \"Default Style\" settings instead");
			_globalOverrideTip = CreateToolTip(IDC_GLOBAL_WHATISGLOBALOVERRIDE_LINK, _hSelf, _hInst, globalOverrideTipStr.data(), false);

			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
			NppDarkMode::autoSubclassAndThemeWindowNotify(_hSelf);

			goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);

			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			return NppDarkMode::onCtlColorCtrl(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORLISTBOX:
		{
			return NppDarkMode::onCtlColorListbox(wParam, lParam);
		}

		case WM_CTLCOLORDLG:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORSTATIC:
		{
			auto hdcStatic = reinterpret_cast<HDC>(wParam);
			auto dlgCtrlID = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam));

			if (dlgCtrlID == IDC_STYLEDESCRIPTION_STATIC)
			{
				return NppDarkMode::onCtlColorDlgLinkText(hdcStatic, true);
			}

			bool isStaticText = (dlgCtrlID == IDC_FG_STATIC ||
				dlgCtrlID == IDC_BG_STATIC ||
				dlgCtrlID == IDC_FONTNAME_STATIC ||
				dlgCtrlID == IDC_FONTSIZE_STATIC);
			//set the static text colors to show enable/disable instead of ::EnableWindow which causes blurry text
			if (isStaticText)
			{
				Style& style = getCurrentStyler();
				bool isTextEnabled = false;

				if (dlgCtrlID == IDC_FG_STATIC)
				{
					isTextEnabled = HIBYTE(HIWORD(style._fgColor)) != 0xFF;

					// Selected text colour style
					if (style._styleDesc == L"Selected text colour")
					{
						isTextEnabled = NppParameters::getInstance().getSVP()._selectedTextForegroundSingleColor;
					}
				}
				else if (dlgCtrlID == IDC_BG_STATIC)
				{
					isTextEnabled = HIBYTE(HIWORD(style._bgColor)) != 0xFF;
				}
				else if (dlgCtrlID == IDC_FONTNAME_STATIC)
				{
					isTextEnabled = !style._fontName.empty();
				}
				else if (dlgCtrlID == IDC_FONTSIZE_STATIC)
				{
					isTextEnabled = style._fontSize != STYLE_NOT_USED && style._fontSize < 100; // style._fontSize has only 2 digits
				}

				return NppDarkMode::onCtlColorDlgStaticText(hdcStatic, isTextEnabled);
			}

			if (dlgCtrlID == IDC_DEF_EXT_EDIT || dlgCtrlID == IDC_DEF_KEYWORDS_EDIT)
			{
				return NppDarkMode::onCtlColor(hdcStatic);
			}
			return NppDarkMode::onCtlColorDlg(hdcStatic);
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::autoThemeChildControls(_hSelf);
			::SendMessage(_pFgColour->getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
			::SendMessage(_pBgColour->getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
			return TRUE;
		}

		case WM_DESTROY:
		{
			destroy();
			return TRUE;
		}

		case WM_HSCROLL:
		{
			if (reinterpret_cast<HWND>(lParam) == ::GetDlgItem(_hSelf, IDC_SC_PERCENTAGE_SLIDER))
			{
				const auto percent = static_cast<int>(::SendDlgItemMessage(_hSelf, IDC_SC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0));
				(NppParameters::getInstance()).SetTransparent(_hSelf, percent);
			}
			return TRUE;
		}

		case WM_DPICHANGED:
		{
			_dpiManager.setDpiWP(wParam);

			const int cpDynamicalSize = _dpiManager.scale(25);
			move2CtrlRight(IDC_FG_STATIC, _pFgColour->getHSelf(), cpDynamicalSize, cpDynamicalSize);
			move2CtrlRight(IDC_BG_STATIC, _pBgColour->getHSelf(), cpDynamicalSize, cpDynamicalSize);

			setPositionDpi(lParam);

			return TRUE;
		}

		case WM_COMMAND:
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				int editID = LOWORD(wParam);
				if (editID == IDC_USER_KEYWORDS_EDIT)
				{
					updateUserKeywords();
					notifyDataModified();
					apply(GENERAL_CHANGE);
					return TRUE;
				}
				else if (editID == IDC_USER_EXT_EDIT)
				{
					updateExtension();
					notifyDataModified();
					apply(NO_VISUAL_CHANGE);
					return TRUE;
				}
				return FALSE;
			}
			else
			{
				switch (wParam)
				{
					case IDC_BOLD_CHECK :
						updateFontStyleStatus(BOLD_STATUS);
						notifyDataModified();
						apply(GENERAL_CHANGE);
						return TRUE;

					case IDC_ITALIC_CHECK :
						updateFontStyleStatus(ITALIC_STATUS);
						notifyDataModified();
						apply(GENERAL_CHANGE);
						return TRUE;

					case IDC_UNDERLINE_CHECK :
						updateFontStyleStatus(UNDERLINE_STATUS);
						notifyDataModified();
						apply(GENERAL_CHANGE);
						return TRUE;
					
					case IDC_GLOBAL_GOTOSETTINGS_LINK :
					{
						std::pair<intptr_t, intptr_t> pageAndCtrlID = goToPreferencesSettings();

						if (pageAndCtrlID.first != -1)
							::SendMessage(_hParent, NPPM_INTERNAL_LAUNCHPREFERENCES, pageAndCtrlID.first, pageAndCtrlID.second);
					}
					return TRUE;

					case IDCANCEL :
						if (_isDirty)
						{
							NppParameters& nppParamInst = NppParameters::getInstance();
							if (_restoreInvalid)
							{
								wstring str(nppParamInst.getNppGUI()._themeName);
								nppParamInst.reloadStylers(str.c_str());
								loadLangListFromNppParam();
							}

							LexerStylerArray & lsArray = nppParamInst.getLStylerArray();
							StyleArray & globalStyles = nppParamInst.getGlobalStylers();

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
							nppParamInst.initTabCustomColors();
							nppParamInst.initFindDlgStatusMsgCustomColors();

							_restoreInvalid = false;
							_isDirty = false;
							_isThemeDirty = false;
							setVisualFromStyleList();

							::SendMessage(_hSwitch2ThemeCombo, CB_SETCURSEL, _currentThemeIndex, 0);
							::SendMessage(_hParent, WM_UPDATESCINTILLAS, _isThemeChanged, 0);
							::SendMessage(_hParent, WM_UPDATEMAINMENUBITMAPS, 0, 0);

							_isThemeChanged = false;
						}
						::EnableWindow(::GetDlgItem(_hSelf, IDC_SAVECLOSE_BUTTON), FALSE);
						display(false);
						return TRUE;

					case IDC_SAVECLOSE_BUTTON :
					{
						if (_isDirty)
						{
							const LexerStylerArray & lsa = (NppParameters::getInstance()).getLStylerArray();
							const StyleArray & globalStyles = (NppParameters::getInstance()).getGlobalStylers();

							_lsArray = lsa;
							_globalStyles = globalStyles;
							updateThemeName(_themeName);
							_restoreInvalid = false;

							_currentThemeIndex = static_cast<int>(::SendMessage(_hSwitch2ThemeCombo, CB_GETCURSEL, 0, 0));
							::EnableWindow(::GetDlgItem(_hSelf, IDOK), FALSE);
							_isDirty = false;
							_isThemeChanged = false;
						}
						_isThemeDirty = false;
						auto newSavedFilePath = (NppParameters::getInstance()).writeStyles(_lsArray, _globalStyles);
						if (!newSavedFilePath.empty())
							updateThemeName(newSavedFilePath);

						::EnableWindow(::GetDlgItem(_hSelf, IDC_SAVECLOSE_BUTTON), FALSE);
						display(false);

						// With the application of each modification, the following 2 actions (applications) are not necessary 
						//::SendMessage(_hParent, WM_UPDATESCINTILLAS, TRUE, 0);
						//::SendMessage(_hParent, WM_UPDATEMAINMENUBITMAPS, 0, 0);

						const wchar_t* fn = ::PathFindFileName(_themeName.c_str());
						NppDarkMode::setThemeName((!NppDarkMode::isEnabled() && lstrcmp(fn, L"stylers.xml") == 0) ? L"" : fn);

						return TRUE;
					}

					case IDC_SC_TRANSPARENT_CHECK :
					{
						bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_SC_TRANSPARENT_CHECK, BM_GETCHECK, 0, 0));
						if (isChecked)
						{
							const auto percent = static_cast<int>(::SendDlgItemMessage(_hSelf, IDC_SC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0));
							(NppParameters::getInstance()).SetTransparent(_hSelf, percent);
						}
						else
							(NppParameters::getInstance()).removeTransparent(_hSelf);

						::EnableWindow(::GetDlgItem(_hSelf, IDC_SC_PERCENTAGE_SLIDER), isChecked);
						return TRUE;
					}

					case IDC_GLOBAL_FG_CHECK :
					{
						GlobalOverride & glo = (NppParameters::getInstance()).getGlobalOverrideStyle();
						glo.enableFg = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, static_cast<int>(wParam), BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply(GENERAL_CHANGE);
						return TRUE;
					}

					case  IDC_GLOBAL_BG_CHECK:
					{
						GlobalOverride & glo = (NppParameters::getInstance()).getGlobalOverrideStyle();
						glo.enableBg = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, static_cast<int>(wParam), BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply(GENERAL_CHANGE);
						return TRUE;
					}

					case IDC_GLOBAL_FONT_CHECK :
					{
						GlobalOverride & glo = (NppParameters::getInstance()).getGlobalOverrideStyle();
						glo.enableFont = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, static_cast<int>(wParam), BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply(GENERAL_CHANGE);
						return TRUE;
					}
					case IDC_GLOBAL_FONTSIZE_CHECK :
					{
						GlobalOverride & glo = (NppParameters::getInstance()).getGlobalOverrideStyle();
						glo.enableFontSize = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, static_cast<int>(wParam), BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply(GENERAL_CHANGE);
						return TRUE;
					}
					case IDC_GLOBAL_BOLD_CHECK :
					{
						GlobalOverride & glo = (NppParameters::getInstance()).getGlobalOverrideStyle();
						glo.enableBold = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, static_cast<int>(wParam), BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply(GENERAL_CHANGE);
						return TRUE;
					}

					case IDC_GLOBAL_ITALIC_CHECK :
					{
						GlobalOverride & glo = (NppParameters::getInstance()).getGlobalOverrideStyle();
						glo.enableItalic = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, static_cast<int>(wParam), BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply(GENERAL_CHANGE);
						return TRUE;
					}
					case IDC_GLOBAL_UNDERLINE_CHECK :
					{
						GlobalOverride & glo = (NppParameters::getInstance()).getGlobalOverrideStyle();
						glo.enableUnderLine = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, static_cast<int>(wParam), BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply(GENERAL_CHANGE);
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
										apply(GENERAL_CHANGE);
										break;
									case IDC_FONTSIZE_COMBO :
										updateFontSize();
										notifyDataModified();
										apply(GENERAL_CHANGE);
										break;
									case IDC_LANGUAGES_COMBO :
									{
										const auto i = static_cast<int>(::SendDlgItemMessage(_hSelf, LOWORD(wParam), CB_GETCURSEL, 0, 0));
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
										applyCurrentSelectedThemeAndUpdateUI();
										break;
								}
								return TRUE;
							}

							case CPN_COLOURPICKED:
							{
								int applicationInfo = getApplicationInfo();

								int tabColourIndex = whichTabColourIndex();

								if (reinterpret_cast<HWND>(lParam) == _pFgColour->getHSelf())
								{
									updateColour(C_FOREGROUND);
									notifyDataModified();

									if (tabColourIndex != -1)
									{
										TabBarPlus::setColour(_pFgColour->getColour(), static_cast<TabBarPlus::tabColourIndex>(tabColourIndex), nullptr);
									}
									else
									{
										int findDlgStatusMsgIndex = whichFindDlgStatusMsgColourIndex();
										if (findDlgStatusMsgIndex != -1)
										{
											NppParameters& nppParamInst = NppParameters::getInstance();
											nppParamInst.setFindDlgStatusMsgIndexColor(_pFgColour->getColour(), findDlgStatusMsgIndex);
										}
										else if (isDocumentMapStyle())
										{
											ViewZoneDlg::setColour(_pFgColour->getColour(), ViewZoneDlg::ViewZoneColorIndex::focus);
										}
									}
									apply(applicationInfo);
									return TRUE;
								}
								else if (reinterpret_cast<HWND>(lParam) == _pBgColour->getHSelf())
								{
									updateColour(C_BACKGROUND);
									notifyDataModified();
									

									if (tabColourIndex != -1)
									{
										tabColourIndex = (tabColourIndex == TabBarPlus::inactiveText ? TabBarPlus::inactiveBg : tabColourIndex);
										TabBarPlus::setColour(_pBgColour->getColour(), static_cast<TabBarPlus::tabColourIndex>(tabColourIndex), nullptr);
									}
									else if (isDocumentMapStyle())
									{
										ViewZoneDlg::setColour(_pBgColour->getColour(), ViewZoneDlg::ViewZoneColorIndex::frost);
									}
									else
									{
										int colourIndex = whichIndividualTabColourId();

										if (colourIndex != -1)
										{
											if (colourIndex >= TabBarPlus::individualTabColourId::id5)
												colourIndex -= TabBarPlus::individualTabColourId::id5;

											NppParameters& nppParamInst = NppParameters::getInstance();
											nppParamInst.setIndividualTabColor(_pBgColour->getColour(), colourIndex, NppDarkMode::isEnabled());
										}
									}

									apply(applicationInfo);
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
			break;
		}
		default :
			return FALSE;
	}
	return FALSE;
}

void WordStyleDlg::move2CtrlRight(int ctrlID, HWND handle2Move, int handle2MoveWidth, int handle2MoveHeight)
{
	POINT p{};
	RECT rc{};
	::GetWindowRect(::GetDlgItem(_hSelf, ctrlID), &rc);

	p.x = rc.right + _dpiManager.scale(5);
	p.y = rc.top + ((rc.bottom - rc.top) / 2) - handle2MoveHeight / 2;

	::ScreenToClient(_hSelf, &p);
	::MoveWindow(handle2Move, p.x, p.y, handle2MoveWidth, handle2MoveHeight, TRUE);
}

void WordStyleDlg::loadLangListFromNppParam()
{
	NppParameters& nppParamInst = NppParameters::getInstance();
	_lsArray = nppParamInst.getLStylerArray();
	_globalStyles = nppParamInst.getGlobalStylers();

	// Clean up Language List
	::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_COMBO, CB_RESETCONTENT, 0, 0);

	::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Global Styles"));
	// All the lexers
	for (size_t i = 0, nb = _lsArray.getNbLexer() ; i < nb ; ++i)
	{
		const std::wstring langName = (_lsArray.getLexerDescFromIndex(i));
		::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(langName.c_str()));
	}

	static constexpr int index2Begin = 0;
	::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_COMBO, CB_SETCURSEL, index2Begin, 0);
	::RedrawWindow(::GetDlgItem(_hSelf, IDC_LANGUAGES_COMBO), nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
	setStyleListFromLexer(index2Begin);
}

void WordStyleDlg::updateThemeName(const wstring& themeName)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI& nppGUI = nppParam.getNppGUI();
	nppGUI._themeName.assign( themeName );
}

bool WordStyleDlg::getStyleName(std::wstring& styleName, const size_t styleNameLenLimit) const
{
	auto i = ::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETCURSEL, 0, 0);
	if (i == LB_ERR)
		return false;
	
	const auto lbTextLen = ::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETTEXTLEN, i, 0);
	if (lbTextLen == LB_ERR || static_cast<size_t>(lbTextLen) > styleNameLenLimit)
		return false;

	auto buffer = std::wstring(static_cast<size_t>(lbTextLen), L'\0');
	::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETTEXT, i, reinterpret_cast<LPARAM>(buffer.data()));

	styleName = buffer;
	return true;
}

int WordStyleDlg::getApplicationInfo() const
{
	std::wstring styleName;
	if (!WordStyleDlg::getStyleName(styleName))
	{
		return NO_VISUAL_CHANGE;
	}

	if (styleName == L"Default Style")
	{
		return (GENERAL_CHANGE | THEME_CHANGE);
	}

	if ((styleName == L"Mark Style 1")
		|| (styleName == L"Mark Style 2")
		|| (styleName == L"Mark Style 3")
		|| (styleName == L"Mark Style 4")
		|| (styleName == L"Mark Style 5")
		|| (styleName == TABBAR_INDIVIDUALCOLOR_1)
		|| (styleName == TABBAR_INDIVIDUALCOLOR_2)
		|| (styleName == TABBAR_INDIVIDUALCOLOR_3)
		|| (styleName == TABBAR_INDIVIDUALCOLOR_4)
		|| (styleName == TABBAR_INDIVIDUALCOLOR_5)
		|| (styleName == TABBAR_INDIVIDUALCOLOR_DM_1)
		|| (styleName == TABBAR_INDIVIDUALCOLOR_DM_2)
		|| (styleName == TABBAR_INDIVIDUALCOLOR_DM_3)
		|| (styleName == TABBAR_INDIVIDUALCOLOR_DM_4)
		|| (styleName == TABBAR_INDIVIDUALCOLOR_DM_5))
	{
		return (GENERAL_CHANGE | COLOR_CHANGE_4_MENU);
	}
	return GENERAL_CHANGE;
}

int WordStyleDlg::whichTabColourIndex() const 
{
	std::wstring styleName;
	if (!WordStyleDlg::getStyleName(styleName))
	{
		return -1;
	}

	using enum TabBarPlus::tabColourIndex;

	if (styleName == TABBAR_ACTIVEFOCUSEDINDCATOR)
		return activeFocusedTop;

	if (styleName == TABBAR_ACTIVEUNFOCUSEDINDCATOR)
		return activeUnfocusedTop;

	if (styleName == TABBAR_ACTIVETEXT)
		return activeText;

	if (styleName == TABBAR_INACTIVETEXT)
		return inactiveText;

	return -1;
}

int WordStyleDlg::whichIndividualTabColourId()
{
	std::wstring styleName;
	if (!WordStyleDlg::getStyleName(styleName))
	{
		return -1;
	}

	using enum TabBarPlus::individualTabColourId;

	if (styleName == TABBAR_INDIVIDUALCOLOR_1)
		return id0;

	if (styleName == TABBAR_INDIVIDUALCOLOR_2)
		return id1;

	if (styleName == TABBAR_INDIVIDUALCOLOR_3)
		return id2;

	if (styleName == TABBAR_INDIVIDUALCOLOR_4)
		return id3;

	if (styleName == TABBAR_INDIVIDUALCOLOR_5)
		return id4;


	if (styleName == TABBAR_INDIVIDUALCOLOR_DM_1)
		return id5;

	if (styleName == TABBAR_INDIVIDUALCOLOR_DM_2)
		return id6;

	if (styleName == TABBAR_INDIVIDUALCOLOR_DM_3)
		return id7;

	if (styleName == TABBAR_INDIVIDUALCOLOR_DM_4)
		return id8;

	if (styleName == TABBAR_INDIVIDUALCOLOR_DM_5)
		return id9;


	return -1;
}

int WordStyleDlg::whichFindDlgStatusMsgColourIndex()
{
	std::wstring styleName;
	if (!WordStyleDlg::getStyleName(styleName))
	{
		return -1;
	}

	using enum TabBarPlus::individualTabColourId;

	if (styleName == FINDDLG_STAUSNOTFOUND_COLOR)
		return id0;

	if (styleName == FINDDLG_STAUSMESSAGE_COLOR)
		return id1;

	if (styleName == FINDDLG_STAUSREACHED_COLOR)
		return id2;

	return -1;
}

bool WordStyleDlg::isDocumentMapStyle()
{
	std::wstring styleName;
	return WordStyleDlg::getStyleName(styleName) && (styleName == VIEWZONE_DOCUMENTMAP);
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
	auto iFontSizeSel = ::SendMessage(_hFontSizeCombo, CB_GETCURSEL, 0, 0);

	if (iFontSizeSel != 0)
	{
		static constexpr size_t intStrLen = 3;
		wchar_t intStr[intStrLen]{};

		auto lbTextLen = ::SendMessage(_hFontSizeCombo, CB_GETLBTEXTLEN, iFontSizeSel, 0);
		if (static_cast<size_t>(lbTextLen) >= intStrLen)
			return;

		::SendMessage(_hFontSizeCombo, CB_GETLBTEXT, iFontSizeSel, reinterpret_cast<LPARAM>(intStr));

		if (!intStr[0])
			style._fontSize = STYLE_NOT_USED;
		else
		{
			wchar_t *finStr = nullptr;
			style._fontSize = wcstol(intStr, &finStr, 10);
			if (*finStr != '\0')
				style._fontSize = STYLE_NOT_USED;
		}
	}
	else
		style._fontSize = 0;
}

void WordStyleDlg::updateExtension()
{
	static constexpr int NB_MAX = 256;
	wchar_t ext[NB_MAX]{};
	::SendDlgItemMessage(_hSelf, IDC_USER_EXT_EDIT, WM_GETTEXT, NB_MAX, reinterpret_cast<LPARAM>(ext));
	_lsArray.getLexerFromIndex(static_cast<size_t>(_currentLexerIndex) - 1).setLexerUserExt(ext);
}

void WordStyleDlg::updateUserKeywords()
{
	Style & style = getCurrentStyler();
	//const int NB_MAX = 2048;
	//wchar_t kw[NB_MAX];
	auto len = ::SendDlgItemMessage(_hSelf, IDC_USER_KEYWORDS_EDIT, WM_GETTEXTLENGTH, 0, 0);
	len += 1;
	auto kw = std::make_unique<wchar_t[]>(len);
	std::fill_n(kw.get(), len, L'\0');
	::SendDlgItemMessage(_hSelf, IDC_USER_KEYWORDS_EDIT, WM_GETTEXT, len, reinterpret_cast<LPARAM>(kw.get()));
	style._keywords = kw.get();
}

void WordStyleDlg::updateFontName()
{
	Style & style = getCurrentStyler();
	auto iFontSel = ::SendMessage(_hFontNameCombo, CB_GETCURSEL, 0, 0);
	auto* fnStr = reinterpret_cast<wchar_t*>(::SendMessage(_hFontNameCombo, CB_GETITEMDATA, iFontSel, 0));
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

	auto isChecked = ::SendMessage(hWnd, BM_GETCHECK, 0, 0);
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
	auto iSel = ::SendMessage(_hSwitch2ThemeCombo, CB_GETCURSEL, 0, 0);

	wstring prevThemeName(_themeName);
	_themeName.clear();

	NppParameters& nppParamInst = NppParameters::getInstance();
	ThemeSwitcher & themeSwitcher = nppParamInst.getThemeSwitcher();
	pair<wstring, wstring> & themeInfo = themeSwitcher.getElementFromIndex(iSel);
	_themeName = themeInfo.second;

	if (_isThemeDirty)
	{
		wchar_t themeFileName[MAX_PATH]{};
		wcscpy_s(themeFileName, prevThemeName.c_str());
		PathStripPath(themeFileName);
		PathRemoveExtension(themeFileName);
		NativeLangSpeaker *pNativeSpeaker = nppParamInst.getNativeLangSpeaker();
		int mb_response = pNativeSpeaker->messageBox("SwitchUnsavedThemeWarning",
			_hSelf,
			L"Unsaved changes are about to be discarded!\nDo you want to save your changes before switching themes?",
			L"$STR_REPLACE$",
			MB_ICONWARNING | MB_YESNO | MB_APPLMODAL | MB_SETFOREGROUND,
			0,
			themeFileName);
		if ( mb_response == IDYES )
			(NppParameters::getInstance()).writeStyles(_lsArray, _globalStyles);
	}
	nppParamInst.reloadStylers(_themeName.c_str());

	loadLangListFromNppParam();
	_restoreInvalid = true;

}

void WordStyleDlg::applyCurrentSelectedThemeAndUpdateUI()
{
	switchToTheme();
	setVisualFromStyleList();
	notifyDataModified();
	_isThemeDirty = false;
	apply(GENERAL_CHANGE | THEME_CHANGE);
}

bool WordStyleDlg::selectThemeByName(const wchar_t* themeName)
{
	LRESULT iTheme = ::SendMessage(_hSwitch2ThemeCombo, CB_FINDSTRING, 1, reinterpret_cast<LPARAM>(themeName));
	if (iTheme == CB_ERR)
		return false;

	::SendMessage(_hSwitch2ThemeCombo, CB_SETCURSEL, iTheme, 0);

	applyCurrentSelectedThemeAndUpdateUI();

	return true;
}

bool WordStyleDlg::goToSection(const wchar_t* sectionNames)
{
	if (!sectionNames || !sectionNames[0])
		return false;

	std::vector<std::wstring> sections = tokenizeString(sectionNames, ':');

	if (sections.size() == 0 || sections.size() >= 3)
		return false;

	auto i = ::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_COMBO, CB_FINDSTRING, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(sections[0].c_str()));
	if (i == LB_ERR)
		return false;
	::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_COMBO, CB_SETCURSEL, i, 0);
	setStyleListFromLexer(static_cast<int>(i));

	if (sections.size() == 1)
		return true;

	i = ::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_FINDSTRING, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(sections[1].c_str()));
	if (i == LB_ERR)
		return false;
	::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_SETCURSEL, i, 0);
	setVisualFromStyleList();

	grabFocus();

	return true;
}

void WordStyleDlg::setStyleListFromLexer(int index)
{
	_currentLexerIndex = index;

	// Fill out Styles listbox
	// Before filling out, we clean it
	::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_RESETCONTENT, 0, 0);

	if (index)
	{
		const wchar_t* langName = _lsArray.getLexerNameFromIndex(static_cast<size_t>(index) - 1);
		const wchar_t *ext = NppParameters::getInstance().getLangExtFromName(langName);
		const wchar_t *userExt = (_lsArray.getLexerStylerByName(langName))->getLexerUserExt();

		if (ext)
			::SendDlgItemMessage(_hSelf, IDC_DEF_EXT_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(ext));

		// WM_SETTEXT cause sending WM_COMMAND message with EN_CHANGE.
		// That makes status dirty, even it shouldn't in this case.
		// The walk around solution is get the current status before sending WM_SETTEXT,
		// then restore the status after sending this message.
		bool isDirty = _isDirty;
		bool isThemeDirty = _isThemeDirty;

		static constexpr int NB_MAX = 256;
		wchar_t currentExt[NB_MAX]{};
		::SendDlgItemMessage(_hSelf, IDC_USER_EXT_EDIT, WM_GETTEXT, NB_MAX, reinterpret_cast<LPARAM>(currentExt));

		if (userExt && lstrcmp(currentExt, userExt) != 0)
		{
			::SendDlgItemMessage(_hSelf, IDC_USER_EXT_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(userExt));
		}
		_isDirty = isDirty;
		_isThemeDirty = isThemeDirty;
		::EnableWindow(::GetDlgItem(_hSelf, IDC_SAVECLOSE_BUTTON), isDirty || isThemeDirty);
	}
	::ShowWindow(::GetDlgItem(_hSelf, IDC_DEF_EXT_EDIT), index?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_DEF_EXT_STATIC), index?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_USER_EXT_EDIT), index?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_USER_EXT_STATIC), index?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_PLUSSYMBOL2_STATIC), index?SW_SHOW:SW_HIDE);

	StyleArray& lexerStyler = index ? _lsArray.getLexerFromIndex(static_cast<size_t>(index) - 1) : _globalStyles;

	for (const auto& style : lexerStyler)
	{
		const std::wstring styleDesc = (style._styleDesc);
		::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(styleDesc.c_str()));
	}
	::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_SETCURSEL, 0, 0);
	setVisualFromStyleList();
}


std::pair<intptr_t, intptr_t> WordStyleDlg::goToPreferencesSettings()
{
	std::pair<intptr_t, intptr_t> result;
	result.first = -1;  // Page
	result.second = -1; // Control

	enum preferencesSectionPage {
		general = 0,
		toolbar,
		tabbar,
		edit1,
		edit2,
		darkMode,
		margins,
		newDocument,
		defaultDirectory,
		recentFileHistory,
		fileAssociation,
		language,
		indentation,
		highlighting,
		print,
		searching,
		backup,
		autoCompletion,
		multiInstance,
		delimiter,
		performance,
		cloudAndLink,
		searchEngine,
		misc
	};

	const Style& style = getCurrentStyler();

	// Check if it's one of following Global Styles:
	if (style._styleDesc == L"Current line background colour")
	{
		result.first = edit1;
		result.second = IDC_RADIO_CLM_HILITE;
	}
	else if (style._styleDesc == L"Multi-edit carets color" || style._styleDesc == L"Multi-selected text color")
	{
		result.first = edit2;
		result.second = IDC_CHECK_MULTISELECTION;
	}
	else if (style._styleDesc == L"Caret colour")
	{
		result.first = edit1;
		result.second = IDC_CARETSETTING_STATIC;
	}
	else if (style._styleDesc == L"Edge colour")
	{
		result.first = margins;
		result.second = IDC_COLUMNPOS_EDIT;
	}
	else if (style._styleDesc == L"Line number margin")
	{
		result.first = margins;
		result.second = IDC_CHECK_LINENUMBERMARGE;
	}
	else if (style._styleDesc == L"Bookmark margin")
	{
		result.first = margins;
		result.second = IDC_CHECK_BOOKMARKMARGE;
	}
	else if (style._styleDesc == L"Change History margin" || style._styleDesc == L"Change History modified"
		|| style._styleDesc == L"Change History revert modified" || style._styleDesc == L"Change History revert origin"
		|| style._styleDesc == L"Change History saved")
	{
		result.first = margins;
		result.second = IDC_GB_CHANGHISTORY;
	}
	else if (style._styleDesc == L"Fold" || style._styleDesc == L"Fold active" || style._styleDesc == L"Fold margin")
	{
		result.first = margins;
		result.second = IDC_FMS_GB_STATIC;
	}
	else if (style._styleDesc == L"Smart Highlighting")
	{
		result.first = highlighting;
		result.second = IDC_CHECK_ENABLSMARTHILITE;
	}
	else if (style._styleDesc == L"Tags match highlighting")
	{
		result.first = highlighting;
		result.second = IDC_CHECK_ENABLTAGSMATCHHILITE;
	}
	else if (style._styleDesc == L"Tags attribute")
	{
		result.first = highlighting;
		result.second = IDC_CHECK_ENABLTAGATTRHILITE;
	}
	else if (style._styleDesc == L"Mark Style 1" || style._styleDesc == L"Mark Style 2" || style._styleDesc == L"Mark Style 3"
		|| style._styleDesc == L"Mark Style 4" || style._styleDesc == L"Mark Style 5")
	{
		result.first = highlighting;
		result.second = IDC_MARKALL_STATIC;
	}
	else if (style._styleDesc == L"URL hovered")
	{
		result.first = cloudAndLink;
		result.second = IDC_CHECK_CLICKABLELINK_ENABLE;
	}
	else if (style._styleDesc == L"EOL custom color")
	{
		result.first = edit2;
		result.second = IDC_CHECK_WITHCUSTOMCOLOR_CRLF;
	}
	else if (style._styleDesc == L"Active tab focused indicator" || style._styleDesc == L"Active tab unfocused indicator")
	{
		result.first = tabbar;
		result.second = IDC_CHECK_ORANGE;
	}
	else if (style._styleDesc == L"Inactive tabs")
	{
		result.first = tabbar;
		result.second = IDC_CHECK_DRAWINACTIVE;
	}
	else if (style._styleDesc == g_npcStyleName)
	{
		result.first = edit2;
		result.second = IDC_CHECK_NPC_COLOR;
	}

	return result;
}

void WordStyleDlg::syncWithSelFgSingleColorCtrl()
{
	const Style& style = getCurrentStyler();

	// Selected text colour style
	if (style._styleDesc == L"Selected text colour")
	{
		// Only in case that dialog is on "Selected text colour":
		// Switch to a section then switch back for refresh current state of "Selected text colour"
		goToSection(L"Global Styles:Default Style");
		goToSection(L"Global Styles:Selected text colour");
	}
}

void WordStyleDlg::setVisualFromStyleList()
{
	showGlobalOverrideCtrls(false);

	Style & style = getCurrentStyler();

	// Global override style
	if (style._styleDesc == L"Global override")
	{
		showGlobalOverrideCtrls(true);
	}

	std::pair<intptr_t, intptr_t> pageAndCtrlID = goToPreferencesSettings();
	_goToSettings.display(pageAndCtrlID.first != -1);

	//--Warning text
	//bool showWarning = ((_currentLexerIndex == 0) && (style._styleID == STYLE_DEFAULT));//?SW_SHOW:SW_HIDE;

	static constexpr size_t strLen = 256;
	wchar_t str[strLen + 1] = { '\0' };

	str[0] = '\0';

	auto i = ::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_COMBO, CB_GETCURSEL, 0, 0);
	if (i == LB_ERR)
		return;
	auto lbTextLen = ::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_COMBO, CB_GETLBTEXTLEN, i, 0);
	if (static_cast<size_t>(lbTextLen) > strLen)
		return;

	::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_COMBO, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(str));

	i = ::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETCURSEL, 0, 0);
	if (i == LB_ERR)
		return;
	static constexpr size_t styleNameLen = 64;
	wchar_t styleName[styleNameLen + 1] = { '\0' };
	lbTextLen = ::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETTEXTLEN, i, 0);
	if (static_cast<size_t>(lbTextLen) > styleNameLen)
		return;
	::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETTEXT, i, reinterpret_cast<LPARAM>(styleName));
	wcscat_s(str, L": ");
	wcscat_s(str, styleName);

	::SetWindowText(_hStyleInfoStaticText, str);

	//-- 2 couleurs : fg et bg
	bool isEnable = false;
	if (HIBYTE(HIWORD(style._fgColor)) != 0xFF)
	{
		_pFgColour->setColour(style._fgColor);
		_pFgColour->setEnabled((style._colorStyle & COLORSTYLE_FOREGROUND) != 0);
		isEnable = true;
	}

	// Selected text colour style
	if (style._styleDesc == L"Selected text colour")
	{
		isEnable = false; // disable by default for "Selected text colour" style

		if (NppParameters::getInstance().getSVP()._selectedTextForegroundSingleColor)
			isEnable = true;
	}
	::EnableWindow(_pFgColour->getHSelf(), isEnable);
	InvalidateRect(_hFgColourStaticText, NULL, FALSE);

	isEnable = false;
	if (HIBYTE(HIWORD(style._bgColor)) != 0xFF)
	{
		_pBgColour->setColour(style._bgColor);
		_pBgColour->setEnabled((style._colorStyle & COLORSTYLE_BACKGROUND) != 0);
		isEnable = true;
	}
	::EnableWindow(_pBgColour->getHSelf(), isEnable);
	InvalidateRect(_hBgColourStaticText, NULL, FALSE);

	//-- font name
	LRESULT iFontName = 0;
	if (!style._fontName.empty())
	{
		iFontName = ::SendMessage(_hFontNameCombo, CB_FINDSTRING, 1, reinterpret_cast<LPARAM>(style._fontName.c_str()));
		if (iFontName == CB_ERR)
			iFontName = 0;
	}

	::SendMessage(_hFontNameCombo, CB_SETCURSEL, iFontName, 0);
	::EnableWindow(_hFontNameCombo, style._isFontEnabled);
	::RedrawWindow(_hFontNameCombo, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
	InvalidateRect(_hFontNameStaticText, NULL, FALSE);

	//-- font size
	static constexpr size_t intStrLen = 3;
	LRESULT iFontSize = 0;
	if (style._fontSize != STYLE_NOT_USED && style._fontSize < 100) // style._fontSize has only 2 digits
	{
		wchar_t intStr[intStrLen]{};
		wsprintf(intStr, L"%d", style._fontSize);
		iFontSize = ::SendMessage(_hFontSizeCombo, CB_FINDSTRING, 1, reinterpret_cast<LPARAM>(intStr));
	}
	::SendMessage(_hFontSizeCombo, CB_SETCURSEL, iFontSize, 0);
	::EnableWindow(_hFontSizeCombo, style._isFontEnabled);
	::RedrawWindow(_hFontSizeCombo, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
	InvalidateRect(_hFontSizeStaticText, NULL, FALSE);
	
	//-- font style : bold & italic
	if (style._fontStyle != STYLE_NOT_USED)
	{
		int isBold = (style._fontStyle & FONTSTYLE_BOLD)?BST_CHECKED:BST_UNCHECKED;
		int isItalic = (style._fontStyle & FONTSTYLE_ITALIC)?BST_CHECKED:BST_UNCHECKED;
		int isUnderline = (style._fontStyle & FONTSTYLE_UNDERLINE)?BST_CHECKED:BST_UNCHECKED;
		::SendMessage(_hCheckBold, BM_SETCHECK, isBold, 0);
		::SendMessage(_hCheckItalic, BM_SETCHECK, isItalic, 0);
		::SendMessage(_hCheckUnderline, BM_SETCHECK, isUnderline, 0);
	}
	else // STYLE_NOT_USED : reset them all
	{
		::SendMessage(_hCheckBold, BM_SETCHECK, BST_UNCHECKED, 0);
		::SendMessage(_hCheckItalic, BM_SETCHECK, BST_UNCHECKED, 0);
		::SendMessage(_hCheckUnderline, BM_SETCHECK, BST_UNCHECKED, 0);
	}

	enableFontStyle(style._isFontEnabled);


	//-- Default Keywords
	bool shouldBeDisplayed = style._keywordClass != STYLE_NOT_USED;
	if (shouldBeDisplayed)
	{
		LexerStyler & lexerStyler = _lsArray.getLexerFromIndex(static_cast<size_t>(_currentLexerIndex) - 1);

		NppParameters& nppParams = NppParameters::getInstance();
		LangType lType = nppParams.getLangIDFromStr(lexerStyler.getLexerName());
		if (lType == L_TEXT)
		{
			wstring lexerNameStr = lexerStyler.getLexerName();
			lexerNameStr += L" is not defined in NppParameters::getLangIDFromStr()";
				printStr(lexerNameStr.c_str());
		}
		const wchar_t *kws = nppParams.getWordList(lType, style._keywordClass);
		if (!kws)
			kws = L"";
		::SendDlgItemMessage(_hSelf, IDC_DEF_KEYWORDS_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(kws));

		const wchar_t *ckwStr = style._keywords.c_str();
		::SendDlgItemMessage(_hSelf, IDC_USER_KEYWORDS_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(ckwStr));
	}

	int showOption = shouldBeDisplayed?SW_SHOW:SW_HIDE;
	::ShowWindow(::GetDlgItem(_hSelf, IDC_DEF_KEYWORDS_EDIT), showOption);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_USER_KEYWORDS_EDIT),showOption);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_DEF_KEYWORDS_STATIC), showOption);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_USER_KEYWORDS_STATIC),showOption);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_PLUSSYMBOL_STATIC),showOption);

	redraw();
}


void WordStyleDlg::create(int dialogID, bool isRTL, bool msgDestParent, WORD fontSize)
{
	StaticDialog::create(dialogID, isRTL, msgDestParent, fontSize);

	if ((NppParameters::getInstance()).isTransparentAvailable())
	{
		::ShowWindow(::GetDlgItem(_hSelf, IDC_SC_TRANSPARENT_CHECK), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_SC_PERCENTAGE_SLIDER), SW_SHOW);

		::SendDlgItemMessage(_hSelf, IDC_SC_PERCENTAGE_SLIDER, TBM_SETRANGE, FALSE, MAKELONG(20, 200));
		::SendDlgItemMessage(_hSelf, IDC_SC_PERCENTAGE_SLIDER, TBM_SETPOS, TRUE, 150);
		if (!(BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_SC_PERCENTAGE_SLIDER, BM_GETCHECK, 0, 0)))
			::EnableWindow(::GetDlgItem(_hSelf, IDC_SC_PERCENTAGE_SLIDER), FALSE);
	}
}

void WordStyleDlg::doDialog(bool isRTL)
{
	if (!isCreated())
	{
		const auto dpiContext = DPIManagerV2::setThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		WordStyleDlg::create(IDD_STYLER_DLG, isRTL);
		DPIManagerV2::setThreadDpiAwarenessContext(dpiContext);
		prepare2Cancel();
	}

	if (!::IsWindowVisible(_hSelf))
	{
		prepare2Cancel();
	}
	display();
}

void WordStyleDlg::destroy()
{
	if (_pFgColour.get() != nullptr)
	{
		_pFgColour->destroy();
		_pBgColour.reset(nullptr);
	}

	if (_pBgColour.get() != nullptr)
	{
		_pBgColour->destroy();
		_pBgColour.reset(nullptr);
	}
}

void WordStyleDlg::prepare2Cancel()
{
	_styles2restored = (NppParameters::getInstance()).getLStylerArray();
	_gstyles2restored = (NppParameters::getInstance()).getGlobalStylers();
	_gOverride2restored = (NppParameters::getInstance()).getGlobalOverrideStyle();
}

void WordStyleDlg::redraw(bool forceUpdate) const
{
	_pFgColour->redraw(forceUpdate);
	_pBgColour->redraw(forceUpdate);
	::InvalidateRect(_hStyleInfoStaticText, NULL, TRUE);
	::UpdateWindow(_hStyleInfoStaticText);
}

void WordStyleDlg::restoreGlobalOverrideValues() const
{
	GlobalOverride& gOverride = (NppParameters::getInstance()).getGlobalOverrideStyle();
	gOverride = _gOverride2restored;
}

void WordStyleDlg::apply(int applicationInfo)
{
	LexerStylerArray & lsa = (NppParameters::getInstance()).getLStylerArray();
	lsa = _lsArray;

	StyleArray & globalStyles = (NppParameters::getInstance()).getGlobalStylers();
	globalStyles = _globalStyles;

	if ((applicationInfo & THEME_CHANGE) != 0)
		_isThemeChanged = true;

	if (applicationInfo & GENERAL_CHANGE || applicationInfo & THEME_CHANGE)
		::SendMessage(_hParent, WM_UPDATESCINTILLAS, (applicationInfo & THEME_CHANGE) != 0, 0);
	if (applicationInfo & COLOR_CHANGE_4_MENU)
		::SendMessage(_hParent, WM_UPDATEMAINMENUBITMAPS, 0, 0);

	::EnableWindow(::GetDlgItem(_hSelf, IDOK), FALSE);
}

void WordStyleDlg::addLastThemeEntry() const
{
	NppParameters& nppParamInst = NppParameters::getInstance();
	ThemeSwitcher& themeSwitcher = nppParamInst.getThemeSwitcher();
	std::pair<wstring, wstring>& themeInfo = themeSwitcher.getElementFromIndex(themeSwitcher.size() - 1);
	::SendMessage(_hSwitch2ThemeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(themeInfo.first.c_str()));
}

Style& WordStyleDlg::getCurrentStyler()
{
	auto styleIndex = static_cast<int>(::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETCURSEL, 0, 0));
	if (styleIndex == LB_ERR)
		styleIndex = 0;

	try {
		if (_currentLexerIndex == 0)
		{
			return _globalStyles.getStyler(styleIndex);
		}
		else
		{
			LexerStyler& lexerStyler = _lsArray.getLexerFromIndex(static_cast<size_t>(_currentLexerIndex) - 1);
			return lexerStyler.getStyler(styleIndex);
		}
	}
	catch (...)
	{
		return _globalStyles.getStyler(0);
	}
}

void WordStyleDlg::enableFontStyle(bool isEnable) const
{
	::EnableWindow(_hCheckBold, isEnable);
	::EnableWindow(_hCheckItalic, isEnable);
	::EnableWindow(_hCheckUnderline, isEnable);
}

long WordStyleDlg::notifyDataModified()
{
	_isDirty = true;
	_isThemeDirty = true;
	::EnableWindow(::GetDlgItem(_hSelf, IDC_SAVECLOSE_BUTTON), TRUE);
	return TRUE;
}

void WordStyleDlg::showGlobalOverrideCtrls(bool show)
{
	if (show)
	{
		updateGlobalOverrideCtrls();
	}
	::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_FG_CHECK), show ? SW_SHOW : SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_BG_CHECK), show ? SW_SHOW : SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_FONT_CHECK), show ? SW_SHOW : SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_FONTSIZE_CHECK), show ? SW_SHOW : SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_BOLD_CHECK), show ? SW_SHOW : SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_ITALIC_CHECK), show ? SW_SHOW : SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_UNDERLINE_CHECK), show ? SW_SHOW : SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_WHATISGLOBALOVERRIDE_LINK), show ? SW_SHOW : SW_HIDE);
	_isShownGOCtrls = show;
}
