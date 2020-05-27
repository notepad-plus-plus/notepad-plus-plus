// This file is part of Notepad++ project
// Copyright (C)2020 Don HO <don.h@free.fr>
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

#include <shlwapi.h>
#include <shlobj.h>
#include <uxtheme.h>
#include "preferenceDlg.h"
#include "lesDlgs.h"
#include "EncodingMapper.h"
#include "localization.h"

#define MyGetGValue(rgb)      (LOBYTE((rgb)>>8))

using namespace std;

const int BLINKRATE_FASTEST = 50;
const int BLINKRATE_SLOWEST = 2500;
const int BLINKRATE_INTERVAL = 50;

const int BORDERWIDTH_SMALLEST = 0;
const int BORDERWIDTH_LARGEST = 30;
const int BORDERWIDTH_INTERVAL = 1;

// This int encoding array is built from "EncodingUnit encodings[]" (see EncodingMapper.cpp)
// And DefaultNewDocDlg will use "int encoding array" to get more info from "EncodingUnit encodings[]"
static int encodings[] = {
	1250, 
	1251, 
	1252, 
	1253, 
	1254, 
	1255, 
	1256, 
	1257, 
	1258, 
	28591,
	28592,
	28593,
	28594,
	28595,
	28596,
	28597,
	28598,
	28599,
	28603,
	28604,
	28605,
	437,  
	720,  
	737,  
	775,  
	850,  
	852,  
	855,  
	857,  
	858,  
	860,  
	861,  
	862,  
	863,  
	865,  
	866,  
	869,  
	950,  
	936,  
	932,  
	949,  
	51949,
	874,
	10007,
	21866,
	20866
};

INT_PTR CALLBACK PreferenceDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			_barsDlg.init(_hInst, _hSelf);
			_barsDlg.create(IDD_PREFERENCE_BAR_BOX, false, false);
			_barsDlg.display();
			
			_marginsDlg.init(_hInst, _hSelf);
			_marginsDlg.create(IDD_PREFERENCE_MARGEIN_BOX, false, false);
			
			_settingsDlg.init(_hInst, _hSelf);
			_settingsDlg.create(IDD_PREFERENCE_SETTING_BOX, false, false);
			
			_defaultNewDocDlg.init(_hInst, _hSelf);
			_defaultNewDocDlg.create(IDD_PREFERENCE_NEWDOCSETTING_BOX, false, false);

			_defaultDirectoryDlg.init(_hInst, _hSelf);
			_defaultDirectoryDlg.create(IDD_PREFERENCE_DEFAULTDIRECTORY_BOX, false, false);

			_recentFilesHistoryDlg.init(_hInst, _hSelf);
			_recentFilesHistoryDlg.create(IDD_PREFERENCE_RECENTFILESHISTORY_BOX, false, false);

			_fileAssocDlg.init(_hInst, _hSelf);
			_fileAssocDlg.create(IDD_REGEXT_BOX, false, false);

			_printSettingsDlg.init(_hInst, _hSelf);
			_printSettingsDlg.create(IDD_PREFERENCE_PRINT_BOX, false, false);

			_langMenuDlg.init(_hInst, _hSelf);
			_langMenuDlg.create(IDD_PREFERENCE_LANG_BOX, false, false);

			_highlighting.init(_hInst, _hSelf);
			_highlighting.create(IDD_PREFERENCE_HILITE_BOX, false, false);

			_backupDlg.init(_hInst, _hSelf);
			_backupDlg.create(IDD_PREFERENCE_BACKUP_BOX, false, false);

			_autoCompletionDlg.init(_hInst, _hSelf);
			_autoCompletionDlg.create(IDD_PREFERENCE_AUTOCOMPLETION_BOX, false, false);

			_multiInstDlg.init(_hInst, _hSelf);
			_multiInstDlg.create(IDD_PREFERENCE_MULTIINSTANCE_BOX, false, false);

			_delimiterSettingsDlg.init(_hInst, _hSelf);
			_delimiterSettingsDlg.create(IDD_PREFERENCE_DELIMITERSETTINGS_BOX, false, false);

			_settingsOnCloudDlg.init(_hInst, _hSelf);
			_settingsOnCloudDlg.create(IDD_PREFERENCE_SETTINGSONCLOUD_BOX, false, false);

			_searchEngineDlg.init(_hInst, _hSelf);
			_searchEngineDlg.create(IDD_PREFERENCE_SEARCHENGINE_BOX, false, false);


			_wVector.push_back(DlgInfo(&_barsDlg, TEXT("General"), TEXT("Global")));
			_wVector.push_back(DlgInfo(&_marginsDlg, TEXT("Editing"), TEXT("Scintillas")));
			_wVector.push_back(DlgInfo(&_defaultNewDocDlg, TEXT("New Document"), TEXT("NewDoc")));
			_wVector.push_back(DlgInfo(&_defaultDirectoryDlg, TEXT("Default Directory"), TEXT("DefaultDir")));
			_wVector.push_back(DlgInfo(&_recentFilesHistoryDlg, TEXT("Recent Files History"), TEXT("RecentFilesHistory")));
			_wVector.push_back(DlgInfo(&_fileAssocDlg, TEXT("File Association"), TEXT("FileAssoc")));
			_wVector.push_back(DlgInfo(&_langMenuDlg, TEXT("Language"), TEXT("Language")));
			_wVector.push_back(DlgInfo(&_highlighting, TEXT("Highlighting"), TEXT("Highlighting")));
			_wVector.push_back(DlgInfo(&_printSettingsDlg, TEXT("Print"), TEXT("Print")));
			_wVector.push_back(DlgInfo(&_backupDlg, TEXT("Backup"), TEXT("Backup")));
			_wVector.push_back(DlgInfo(&_autoCompletionDlg, TEXT("Auto-Completion"), TEXT("AutoCompletion")));
			_wVector.push_back(DlgInfo(&_multiInstDlg, TEXT("Multi-Instance"), TEXT("MultiInstance")));
			_wVector.push_back(DlgInfo(&_delimiterSettingsDlg, TEXT("Delimiter"), TEXT("Delimiter")));
			_wVector.push_back(DlgInfo(&_settingsOnCloudDlg, TEXT("Cloud"), TEXT("Cloud")));
			_wVector.push_back(DlgInfo(&_searchEngineDlg, TEXT("Search Engine"), TEXT("SearchEngine")));
			_wVector.push_back(DlgInfo(&_settingsDlg, TEXT("MISC."), TEXT("MISC")));


			makeCategoryList();
			RECT rc;
			getClientRect(rc);

			rc.top += NppParameters::getInstance()._dpiManager.scaleY(10);
			rc.bottom -= NppParameters::getInstance()._dpiManager.scaleY(50);
			rc.left += NppParameters::getInstance()._dpiManager.scaleX(150);
			
			_barsDlg.reSizeTo(rc);
			_marginsDlg.reSizeTo(rc);
			_settingsDlg.reSizeTo(rc);
			_defaultNewDocDlg.reSizeTo(rc);
			_defaultDirectoryDlg.reSizeTo(rc);
			_recentFilesHistoryDlg.reSizeTo(rc);
			_fileAssocDlg.reSizeTo(rc);
			_langMenuDlg.reSizeTo(rc);
			_highlighting.reSizeTo(rc);
			_printSettingsDlg.reSizeTo(rc);
			_backupDlg.reSizeTo(rc);
			_autoCompletionDlg.reSizeTo(rc);
			_multiInstDlg.reSizeTo(rc);
			_delimiterSettingsDlg.reSizeTo(rc);
			_settingsOnCloudDlg.reSizeTo(rc);
			_searchEngineDlg.reSizeTo(rc);

			NppParameters& nppParam = NppParameters::getInstance();
			ETDTProc enableDlgTheme = (ETDTProc)nppParam.getEnableThemeDlgTexture();
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);

			return TRUE;
		}

		case WM_COMMAND :
		{
			if (LOWORD(wParam) == IDC_LIST_DLGTITLE)
			{
				if (HIWORD(wParam) == CBN_SELCHANGE)
				{
					auto i = ::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_GETCURSEL, 0, 0);
					if (i != LB_ERR)
					{
						showDialogByIndex(i);
					}
				}
			}
			else 
			{
				switch (wParam)
				{
					case IDC_BUTTON_CLOSE :
					case IDCANCEL :
						display(false);
						return TRUE;
						
					default :
						::SendMessage(_hParent, WM_COMMAND, wParam, lParam);
						return TRUE;
				}
			}
		}
	}
	return FALSE;
}

void PreferenceDlg::makeCategoryList()
{
	for (size_t i = 0, len = _wVector.size(); i < len; ++i)
	{
		::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(_wVector[i]._name.c_str()));
	}
	setListSelection(0);
}

int32_t PreferenceDlg::getIndexFromName(const TCHAR *name) const
{
	if (not name)
		return -1;

	int32_t i = 0;
	for (auto it = _wVector.begin() ; it != _wVector.end(); ++it, ++i)
	{
		if (it->_internalName == name)
			return i;
	}
	return -1;
}

bool PreferenceDlg::setListSelection(size_t currentSel) const
{
	// Stupid LB API doesn't allow LB_SETSEL to be used on single select listbox, so we do it in a hard way
	const size_t selStrLenMax = 255;
	TCHAR selStr[selStrLenMax + 1];
	auto lbTextLen = ::SendMessage(_hSelf, LB_GETTEXTLEN, currentSel, 0);

	if (lbTextLen > selStrLenMax)
		return false;

	::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_GETTEXT, currentSel, reinterpret_cast<LPARAM>(selStr));
	::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_SELECTSTRING, currentSel, reinterpret_cast<LPARAM>(selStr));
	return true;
}

bool PreferenceDlg::renameDialogTitle(const TCHAR *internalName, const TCHAR *newName)
{
	bool foundIt = false;
	size_t i = 0;
	for (size_t len = _wVector.size(); i < len; ++i)
	{
		if (_wVector[i]._internalName == internalName)
		{
			foundIt = true;
			break;
		}
	}
	if (!foundIt)
		return false;

	const size_t lenMax = 256;
	TCHAR oldName[lenMax] = {0};
	size_t txtLen = ::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_GETTEXTLEN, i, 0);
	if (txtLen >= lenMax)
		return false;

	::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_GETTEXT, i, reinterpret_cast<LPARAM>(oldName));

	// Same name, no need to change, but operation is considered success
	if (lstrcmp(newName, oldName) == 0)
		return true;

	::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_DELETESTRING, i, 0);
	::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_INSERTSTRING, i, reinterpret_cast<LPARAM>(newName));

	return true;
}

void PreferenceDlg::showDialogByName(const TCHAR *name) const
{
	int32_t i = getIndexFromName(name);
	if (i >= 0)
	{
		showDialogByIndex(i);
		setListSelection(i);
	}
}


void PreferenceDlg::showDialogByIndex(size_t index) const
{
	size_t len = _wVector.size();
	for (size_t i = 0; i < len; ++i)
	{
		_wVector[i]._dlg->display(false);
	}
	_wVector[index]._dlg->display(true);
}

void PreferenceDlg::destroy()
{
	_barsDlg.destroy();
	_marginsDlg.destroy();
	_settingsDlg.destroy();
	_fileAssocDlg.destroy();
	_langMenuDlg.destroy();
	_highlighting.destroy();
	_printSettingsDlg.destroy();
	_defaultNewDocDlg.destroy();
	_defaultDirectoryDlg.destroy();
	_recentFilesHistoryDlg.destroy();
	_backupDlg.destroy();
	_autoCompletionDlg.destroy();
	_multiInstDlg.destroy();
	_delimiterSettingsDlg.destroy();
}

INT_PTR CALLBACK BarsDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	NppParameters& nppParam = NppParameters::getInstance();
	
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			const NppGUI & nppGUI = nppParam.getNppGUI();
			toolBarStatusType tbStatus = nppGUI._toolBarStatus;
			int tabBarStatus = nppGUI._tabStatus;
			bool showTool = nppGUI._toolbarShow;
			bool showStatus = nppGUI._statusBarShow;
			bool showMenu = nppGUI._menuBarShow;

			::SendDlgItemMessage(_hSelf, IDC_CHECK_HIDE, BM_SETCHECK, showTool?BST_UNCHECKED:BST_CHECKED, 0);
			int ID2Check = 0;
			switch (tbStatus)
			{
				case TB_SMALL :
					ID2Check = IDC_RADIO_SMALLICON;
					break;
				case TB_LARGE :
					ID2Check = IDC_RADIO_BIGICON;
					break;
				case TB_STANDARD:
				default :
					ID2Check = IDC_RADIO_STANDARD;
			}
			::SendDlgItemMessage(_hSelf, ID2Check, BM_SETCHECK, BST_CHECKED, 0);
			
			::SendDlgItemMessage(_hSelf, IDC_CHECK_REDUCE, BM_SETCHECK, tabBarStatus & TAB_REDUCE, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_LOCK, BM_SETCHECK, !(tabBarStatus & TAB_DRAGNDROP), 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_ORANGE, BM_SETCHECK, tabBarStatus & TAB_DRAWTOPBAR, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_DRAWINACTIVE, BM_SETCHECK, tabBarStatus & TAB_DRAWINACTIVETAB, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_ENABLETABCLOSE, BM_SETCHECK, tabBarStatus & TAB_CLOSEBUTTON, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_DBCLICK2CLOSE, BM_SETCHECK, tabBarStatus & TAB_DBCLK2CLOSE, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_TAB_VERTICAL, BM_SETCHECK, tabBarStatus & TAB_VERTICAL, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_TAB_MULTILINE, BM_SETCHECK, tabBarStatus & TAB_MULTILINE, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_TAB_LAST_EXIT, BM_SETCHECK, tabBarStatus & TAB_QUITONEMPTY, 0);
			
			::SendDlgItemMessage(_hSelf, IDC_CHECK_TAB_HIDE, BM_SETCHECK, tabBarStatus & TAB_HIDE, 0);
			::SendMessage(_hSelf, WM_COMMAND, IDC_CHECK_TAB_HIDE, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_SHOWSTATUSBAR, BM_SETCHECK, showStatus, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_HIDEMENUBAR, BM_SETCHECK, !showMenu, 0);

			bool showDocSwitcher = ::SendMessage(::GetParent(_hParent), NPPM_ISDOCSWITCHERSHOWN, 0, 0) == TRUE;
			::SendDlgItemMessage(_hSelf, IDC_CHECK_DOCSWITCH, BM_SETCHECK, showDocSwitcher, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_DOCSWITCH_NOEXTCOLUMN, BM_SETCHECK, nppGUI._fileSwitcherWithoutExtColumn, 0);

			LocalizationSwitcher & localizationSwitcher = nppParam.getLocalizationSwitcher();

			for (size_t i = 0, len = localizationSwitcher.size(); i < len ; ++i)
			{
				pair<wstring, wstring> localizationInfo = localizationSwitcher.getElementFromIndex(i);
				::SendDlgItemMessage(_hSelf, IDC_COMBO_LOCALIZATION, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(localizationInfo.first.c_str()));
			}
			wstring lang = TEXT("English"); // Set default language as Englishs
			if (nppParam.getNativeLangA()) // if nativeLangA is not NULL, then we can be sure the default language (English) is not used
			{
				string fn = localizationSwitcher.getFileName();
				wstring fnW = s2ws(fn);
				lang = localizationSwitcher.getLangFromXmlFileName(fnW.c_str());
			}
			auto index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_LOCALIZATION, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(lang.c_str()));
			if (index != CB_ERR)
                ::SendDlgItemMessage(_hSelf, IDC_COMBO_LOCALIZATION, CB_SETCURSEL, index, 0);

			ETDTProc enableDlgTheme = reinterpret_cast<ETDTProc>(nppParam.getEnableThemeDlgTexture());
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);

			return TRUE;
		}
		
		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDC_CHECK_SHOWSTATUSBAR :
				{
					bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_SHOWSTATUSBAR, BM_GETCHECK, 0, 0));
					::SendMessage(::GetParent(_hParent), NPPM_HIDESTATUSBAR, 0, isChecked?FALSE:TRUE);
				}
				return TRUE;

				case IDC_CHECK_HIDEMENUBAR :
				{
					bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_HIDEMENUBAR, BM_GETCHECK, 0, 0));
					::SendMessage(::GetParent(_hParent), NPPM_HIDEMENU, 0, isChecked?TRUE:FALSE);
				}
				return TRUE;

				case IDC_CHECK_DOCSWITCH :
				{
					bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_DOCSWITCH, BM_GETCHECK, 0, 0));
					::SendMessage(::GetParent(_hParent), NPPM_SHOWDOCSWITCHER, 0, isChecked?TRUE:FALSE);
					getFocus();
				}
				return TRUE;
				case IDC_CHECK_DOCSWITCH_NOEXTCOLUMN :
				{
					bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_DOCSWITCH_NOEXTCOLUMN, BM_GETCHECK, 0, 0));
					::SendMessage(::GetParent(_hParent), NPPM_DOCSWITCHERDISABLECOLUMN, 0, isChecked?TRUE:FALSE);
				}
				return TRUE;

				case IDC_CHECK_TAB_HIDE :
				{
					bool toBeHidden = (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDC_CHECK_TAB_HIDE), BM_GETCHECK, 0, 0));
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_TAB_MULTILINE), !toBeHidden);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_TAB_VERTICAL), !toBeHidden);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_REDUCE), !toBeHidden);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_LOCK), !toBeHidden);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_ORANGE), !toBeHidden);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_DRAWINACTIVE), !toBeHidden);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_ENABLETABCLOSE), !toBeHidden);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_DBCLICK2CLOSE), !toBeHidden);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_TAB_LAST_EXIT), !toBeHidden);

					::SendMessage(::GetParent(_hParent), NPPM_HIDETABBAR, 0, toBeHidden);
					return TRUE;
				}
				
				case  IDC_CHECK_TAB_VERTICAL:
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_DRAWTABBAR_VERTICAL, 0);
					return TRUE;

				case IDC_CHECK_TAB_MULTILINE :
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_DRAWTABBAR_MULTILINE, 0);
					return TRUE;

				case IDC_CHECK_TAB_LAST_EXIT:
				{
					NppGUI & nppGUI = const_cast<NppGUI &>(nppParam.getNppGUI());
					nppGUI._tabStatus ^= TAB_QUITONEMPTY;
				}
				return TRUE;


				case IDC_CHECK_REDUCE :
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_REDUCETABBAR, 0);
					return TRUE;
					
				case IDC_CHECK_LOCK :
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_LOCKTABBAR, 0);
					return TRUE;
					
				case IDC_CHECK_ORANGE :
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_DRAWTABBAR_TOPBAR, 0);
					return TRUE;
					
				case IDC_CHECK_DRAWINACTIVE :
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_DRAWTABBAR_INACIVETAB, 0);
					return TRUE;
					
				case IDC_CHECK_ENABLETABCLOSE :
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_DRAWTABBAR_CLOSEBOTTUN, 0);
					return TRUE;

				case IDC_CHECK_DBCLICK2CLOSE :
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_DRAWTABBAR_DBCLK2CLOSE, 0);
					return TRUE;

				case IDC_CHECK_HIDE :
				{
					bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_HIDE, BM_GETCHECK, 0, 0));
					::SendMessage(::GetParent(_hParent), NPPM_HIDETOOLBAR, 0, isChecked?TRUE:FALSE);
				}
				return TRUE;
					
				case IDC_RADIO_SMALLICON :
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_TOOLBAR_REDUCE, 0);
					return TRUE;
					
				case IDC_RADIO_BIGICON :
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_TOOLBAR_ENLARGE, 0);
					return TRUE;
					
				case IDC_RADIO_STANDARD :
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_TOOLBAR_STANDARD, 0);
					return TRUE;

				default :
					switch (HIWORD(wParam))
					{
						case CBN_SELCHANGE : // == case LBN_SELCHANGE :
						{
							switch (LOWORD(wParam))
							{
								case IDC_COMBO_LOCALIZATION :
								{
									LocalizationSwitcher & localizationSwitcher = nppParam.getLocalizationSwitcher();
									auto index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_LOCALIZATION, CB_GETCURSEL, 0, 0);
									TCHAR langName[MAX_PATH];
									auto cbTextLen = ::SendDlgItemMessage(_hSelf, IDC_COMBO_LOCALIZATION, CB_GETLBTEXTLEN, index, 0);
									if (cbTextLen > MAX_PATH - 1)
										return TRUE;

									::SendDlgItemMessage(_hSelf, IDC_COMBO_LOCALIZATION, CB_GETLBTEXT, index, reinterpret_cast<LPARAM>(langName));
									if (langName[0])
									{
										// Make English as basic language
										if (localizationSwitcher.switchToLang(TEXT("English")))
										{
											::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_RELOADNATIVELANG, 0, 0);
										}
										// Change the language 
										if (localizationSwitcher.switchToLang(langName))
										{
											::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_RELOADNATIVELANG, 0, 0);
											::InvalidateRect(_hParent, NULL, TRUE);
										}
									}
								}
								return TRUE;
								default:
									break;
							}
						}
					}
			}
		}
	}
	return FALSE;
}

static WNDPROC oldFunclstToolbarProc = NULL;
static LRESULT CALLBACK editNumSpaceProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CHAR:
		{
			// All non decimal numbers and non white space and non backspace are ignored
			if ((wParam != 8 && wParam != 32 && wParam < 48) || wParam > 57)
			{
				return TRUE;
			}
		}
	}
	return oldFunclstToolbarProc(hwnd, message, wParam, lParam);
}

void MarginsDlg::initScintParam()
{
	NppParameters& nppParam = NppParameters::getInstance();
	ScintillaViewParams & svp = const_cast<ScintillaViewParams &>(nppParam.getSVP());
	
	::SendDlgItemMessage(_hSelf, IDC_RADIO_BOX, BM_SETCHECK, FALSE, 0);
	::SendDlgItemMessage(_hSelf, IDC_RADIO_CIRCLE, BM_SETCHECK, FALSE, 0);
	::SendDlgItemMessage(_hSelf, IDC_RADIO_ARROW, BM_SETCHECK, FALSE, 0);
	::SendDlgItemMessage(_hSelf, IDC_RADIO_SIMPLE, BM_SETCHECK, FALSE, 0);
	::SendDlgItemMessage(_hSelf, IDC_RADIO_FOLDMARGENONE, BM_SETCHECK, FALSE, 0);

	int id = 0;
	switch (svp._folderStyle)
	{
		case FOLDER_STYLE_NONE:
			id = IDC_RADIO_FOLDMARGENONE;
			break;
		case FOLDER_STYLE_BOX:
			id = IDC_RADIO_BOX;
			break;
		case FOLDER_STYLE_CIRCLE:
			id = IDC_RADIO_CIRCLE;
			break;
		case FOLDER_STYLE_ARROW:
			id = IDC_RADIO_ARROW;
			break;
		default : // FOLDER_STYLE_SIMPLE:
			id = IDC_RADIO_SIMPLE;
	}
	::SendDlgItemMessage(_hSelf, id, BM_SETCHECK, TRUE, 0);
	
	switch (svp._lineWrapMethod)
	{
		case LINEWRAP_ALIGNED:
			id = IDC_RADIO_LWALIGN;
			break;
		case LINEWRAP_INDENT:
			id = IDC_RADIO_LWINDENT;
			break;
		default : // LINEWRAP_DEFAULT
			id = IDC_RADIO_LWDEF;
	}
	::SendDlgItemMessage(_hSelf, id, BM_SETCHECK, TRUE, 0);

	::SendDlgItemMessage(_hSelf, IDC_CHECK_SMOOTHFONT, BM_SETCHECK, svp._doSmoothFont, 0);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_LINENUMBERMARGE, BM_SETCHECK, svp._lineNumberMarginShow, 0);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_BOOKMARKMARGE, BM_SETCHECK, svp._bookMarkMarginShow, 0);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_CURRENTLINEHILITE, BM_SETCHECK, svp._currentLineHilitingShow, 0);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_SCROLLBEYONDLASTLINE, BM_SETCHECK, svp._scrollBeyondLastLine, 0);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_DISABLEADVANCEDSCROLL, BM_SETCHECK, svp._disableAdvancedScrolling, 0);

	::SendDlgItemMessage(_hSelf, IDC_CHECK_NOEDGE, BM_SETCHECK, !svp._showBorderEdge, 0);

	bool canBeBg = svp._edgeMultiColumnPos.size() == 1;
	if (!canBeBg)
	{
		svp._isEdgeBgMode = false;
		::SendDlgItemMessage(_hSelf, IDC_CHECK_EDGEBGMODE, BM_SETCHECK, FALSE, 0);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_EDGEBGMODE), FALSE);
	}
	else
	{
		::SendDlgItemMessage(_hSelf, IDC_CHECK_EDGEBGMODE, BM_SETCHECK, svp._isEdgeBgMode, 0);
	}
	

	generic_string edgeColumnPosStr;
	for (auto i : svp._edgeMultiColumnPos)
	{
		std::string s = std::to_string(i);
		edgeColumnPosStr += generic_string(s.begin(), s.end());
		edgeColumnPosStr += TEXT(" ");
	}
	::SendDlgItemMessage(_hSelf, IDC_COLUMNPOS_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(edgeColumnPosStr.c_str()));

	oldFunclstToolbarProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(::GetDlgItem(_hSelf, IDC_COLUMNPOS_EDIT), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(editNumSpaceProc)));
}

INT_PTR CALLBACK MarginsDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI & nppGUI = const_cast<NppGUI &>(nppParam.getNppGUI());
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("0")));
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("1")));
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("2")));
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("3")));
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("Block")));


			::SendMessage(::GetDlgItem(_hSelf, IDC_WIDTH_COMBO), CB_SETCURSEL, nppGUI._caretWidth, 0);
            ::SendDlgItemMessage(_hSelf, IDC_CHECK_MULTISELECTION, BM_SETCHECK, nppGUI._enableMultiSelection, 0);
			
			::SendMessage(::GetDlgItem(_hSelf, IDC_CARETBLINKRATE_SLIDER),TBM_SETRANGEMIN, TRUE, BLINKRATE_FASTEST);
			::SendMessage(::GetDlgItem(_hSelf, IDC_CARETBLINKRATE_SLIDER),TBM_SETRANGEMAX, TRUE, BLINKRATE_SLOWEST);
			::SendMessage(::GetDlgItem(_hSelf, IDC_CARETBLINKRATE_SLIDER),TBM_SETPAGESIZE, 0, BLINKRATE_INTERVAL);
			int blinkRate = (nppGUI._caretBlinkRate==0)?BLINKRATE_SLOWEST:nppGUI._caretBlinkRate;
			::SendMessage(::GetDlgItem(_hSelf, IDC_CARETBLINKRATE_SLIDER),TBM_SETPOS, TRUE, blinkRate);

			::SendMessage(::GetDlgItem(_hSelf, IDC_BORDERWIDTH_SLIDER),TBM_SETRANGEMIN, TRUE, BORDERWIDTH_SMALLEST);
			::SendMessage(::GetDlgItem(_hSelf, IDC_BORDERWIDTH_SLIDER),TBM_SETRANGEMAX, TRUE, BORDERWIDTH_LARGEST);
			::SendMessage(::GetDlgItem(_hSelf, IDC_BORDERWIDTH_SLIDER),TBM_SETPAGESIZE, 0, BLINKRATE_INTERVAL);
			const ScintillaViewParams & svp = nppParam.getSVP();
			::SendMessage(::GetDlgItem(_hSelf, IDC_BORDERWIDTH_SLIDER),TBM_SETPOS, TRUE, svp._borderWidth);
			::SetDlgItemInt(_hSelf, IDC_BORDERWIDTHVAL_STATIC, svp._borderWidth, FALSE);

			initScintParam();
			
			ETDTProc enableDlgTheme = (ETDTProc)nppParam.getEnableThemeDlgTexture();
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);
			return TRUE;
		}

		case WM_HSCROLL:
		{
			HWND hCaretBlikRateSlider = ::GetDlgItem(_hSelf, IDC_CARETBLINKRATE_SLIDER);
			HWND hBorderWidthSlider = ::GetDlgItem(_hSelf, IDC_BORDERWIDTH_SLIDER);
			if (reinterpret_cast<HWND>(lParam) == hCaretBlikRateSlider)
			{
				auto blinkRate = ::SendMessage(hCaretBlikRateSlider, TBM_GETPOS, 0, 0);
				if (blinkRate == BLINKRATE_SLOWEST)
					blinkRate = 0;
				nppGUI._caretBlinkRate = static_cast<int>(blinkRate);

				::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SETCARETBLINKRATE, 0, 0);
			}
			else if (reinterpret_cast<HWND>(lParam) == hBorderWidthSlider)
			{
				auto borderWidth = ::SendMessage(hBorderWidthSlider, TBM_GETPOS, 0, 0);
				ScintillaViewParams & svp = (ScintillaViewParams &)nppParam.getSVP();
				svp._borderWidth = static_cast<int>(borderWidth);
				::SetDlgItemInt(_hSelf, IDC_BORDERWIDTHVAL_STATIC, static_cast<UINT>(borderWidth), FALSE);
				::SendMessage(::GetParent(_hParent), WM_SIZE, 0, 0);
			}
			return 0;	//return zero when handled
		}

		case WM_COMMAND : 
		{
			ScintillaViewParams & svp = const_cast<ScintillaViewParams &>(nppParam.getSVP());
			switch (wParam)
			{
				case IDC_CHECK_SMOOTHFONT:
					svp._doSmoothFont = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_SMOOTHFONT, BM_GETCHECK, 0, 0));
					::SendMessage(::GetParent(_hParent), NPPM_SETSMOOTHFONT, 0, svp._doSmoothFont);
					return TRUE;
				case IDC_CHECK_LINENUMBERMARGE:
					svp._lineNumberMarginShow = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_LINENUMBERMARGE, BM_GETCHECK, 0, 0));
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_LINENUMBER, 0);
					return TRUE;
				
				case IDC_CHECK_BOOKMARKMARGE:
					svp._bookMarkMarginShow = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_BOOKMARKMARGE, BM_GETCHECK, 0, 0));
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_SYMBOLMARGIN, 0);
					return TRUE;

				case IDC_CHECK_CURRENTLINEHILITE:
					svp._currentLineHilitingShow = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_CURRENTLINEHILITE, BM_GETCHECK, 0, 0));
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_CURLINE_HILITING, 0);
					return TRUE;

				case IDC_CHECK_SCROLLBEYONDLASTLINE:
					svp._scrollBeyondLastLine = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_SCROLLBEYONDLASTLINE, BM_GETCHECK, 0, 0));
					::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SCROLLBEYONDLASTLINE, 0, 0);
					return TRUE;

				case IDC_CHECK_DISABLEADVANCEDSCROLL:
					svp._disableAdvancedScrolling = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_DISABLEADVANCEDSCROLL, BM_GETCHECK, 0, 0));
					return TRUE;

                case IDC_CHECK_MULTISELECTION :
                    nppGUI._enableMultiSelection = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_MULTISELECTION, BM_GETCHECK, 0, 0));
                    ::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SETMULTISELCTION, 0, 0);
                    return TRUE;

				case IDC_CHECK_NOEDGE:
					svp._showBorderEdge = !(BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_NOEDGE, BM_GETCHECK, 0, 0));
					::SendMessage(::GetParent(_hParent), NPPM_SETEDITORBORDEREDGE, 0, svp._showBorderEdge ? TRUE : FALSE);
					return TRUE;

				case IDC_RADIO_SIMPLE:
					svp._folderStyle = FOLDER_STYLE_SIMPLE;
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_FOLDERMAGIN_SIMPLE, 0);
					return TRUE;
				case IDC_RADIO_ARROW:
					svp._folderStyle = FOLDER_STYLE_ARROW;
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_FOLDERMAGIN_ARROW, 0);
					return TRUE;
				case IDC_RADIO_CIRCLE:
					svp._folderStyle = FOLDER_STYLE_CIRCLE;
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_FOLDERMAGIN_CIRCLE, 0);
					return TRUE;
				case IDC_RADIO_BOX:
					svp._folderStyle = FOLDER_STYLE_BOX;
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_FOLDERMAGIN_BOX, 0);
					return TRUE;
					
				case IDC_RADIO_FOLDMARGENONE:
					svp._folderStyle = FOLDER_STYLE_NONE;
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_FOLDERMAGIN, 0);
					return TRUE;

				case IDC_CHECK_EDGEBGMODE:
					svp._isEdgeBgMode = isCheckedOrNot(IDC_CHECK_EDGEBGMODE);
					::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_EDGEMULTISETSIZE, 0, 0);
					return TRUE;

				case IDC_RADIO_LWDEF:
					svp._lineWrapMethod = LINEWRAP_DEFAULT;
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_LWDEF, 0);
					return TRUE;

				case IDC_RADIO_LWALIGN:
					svp._lineWrapMethod = LINEWRAP_ALIGNED;
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_LWALIGN, 0);
					return TRUE;

				case IDC_RADIO_LWINDENT:
					svp._lineWrapMethod = LINEWRAP_INDENT;
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_LWINDENT, 0);
					return TRUE;

				default :
					switch (HIWORD(wParam))
					{
						case CBN_SELCHANGE : // == case LBN_SELCHANGE :
						{
							if (LOWORD(wParam) == IDC_WIDTH_COMBO)
							{
								nppGUI._caretWidth = static_cast<int32_t>(::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_GETCURSEL, 0, 0));
								::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SETCARETWIDTH, 0, 0);
								return TRUE;
							}
						}
						break;

						case EN_CHANGE :
						{
							if (LOWORD(wParam) == IDC_COLUMNPOS_EDIT)
							{
								TCHAR text[MAX_PATH];
								::SendDlgItemMessage(_hSelf, IDC_COLUMNPOS_EDIT, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(text));

								if (str2numberVector(text, svp._edgeMultiColumnPos))
								{
									bool canBeBg = svp._edgeMultiColumnPos.size() == 1;
									if (!canBeBg)
									{
										svp._isEdgeBgMode = false;
										::SendDlgItemMessage(_hSelf, IDC_CHECK_EDGEBGMODE, BM_SETCHECK, FALSE, 0);
									}
									::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_EDGEBGMODE), canBeBg);
									::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_EDGEMULTISETSIZE, 0, 0);
									return TRUE;
								}
							}
						}
						break;
					}
			}
		}
	}
	return FALSE;
}

const size_t fileUpdateChoiceEnable = 0;
const size_t fileUpdateChoiceEnable4All = 1;
const size_t fileUpdateChoiceDisable = 2;
INT_PTR CALLBACK SettingsDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI & nppGUI = const_cast<NppGUI &>(nppParam.getNppGUI());
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			::SendDlgItemMessage(_hSelf, IDC_COMBO_FILEUPDATECHOICE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("Enable")));
			::SendDlgItemMessage(_hSelf, IDC_COMBO_FILEUPDATECHOICE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("Enable for all opened files")));
			::SendDlgItemMessage(_hSelf, IDC_COMBO_FILEUPDATECHOICE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("Disable")));

			int selIndex = -1;
			
			if (nppGUI._fileAutoDetection & cdEnabledOld)
			{
				selIndex = fileUpdateChoiceEnable4All;
			}
			else if (nppGUI._fileAutoDetection & cdEnabledNew)
			{				
				selIndex = fileUpdateChoiceEnable;
			}
			else //cdDisabled
			{
				::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_UPDATESILENTLY), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_UPDATEGOTOEOF), FALSE);
				
				selIndex = fileUpdateChoiceDisable;
			}
			
			::SendDlgItemMessage(_hSelf, IDC_COMBO_FILEUPDATECHOICE, CB_SETCURSEL, selIndex, 0);

			bool bCheck = (nppGUI._fileAutoDetection & cdAutoUpdate) ? true : false;
			::SendDlgItemMessage(_hSelf, IDC_CHECK_UPDATESILENTLY, BM_SETCHECK, bCheck? BST_CHECKED: BST_UNCHECKED, 0);

			bCheck = (nppGUI._fileAutoDetection & cdGo2end) ? true : false;
			::SendDlgItemMessage(_hSelf, IDC_CHECK_UPDATEGOTOEOF, BM_SETCHECK, bCheck ? BST_CHECKED : BST_UNCHECKED, 0);

			::SendDlgItemMessage(_hSelf, IDC_CHECK_MIN2SYSTRAY, BM_SETCHECK, nppGUI._isMinimizedToTray, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_DETECTENCODING, BM_SETCHECK, nppGUI._detectEncoding, 0);
            ::SendDlgItemMessage(_hSelf, IDC_CHECK_AUTOUPDATE, BM_SETCHECK, nppGUI._autoUpdateOpt._doAutoUpdate, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_MONOSPACEDFONT_FINDDLG, BM_SETCHECK, nppGUI._monospacedFontFindDlg, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_DIRECTWRITE_ENABLE, BM_SETCHECK, nppGUI._writeTechnologyEngine == directWriteTechnology, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_ENABLEDOCPEEKER, BM_SETCHECK, nppGUI._isDocPeekOnTab ? BST_CHECKED : BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_ENABLEDOCPEEKONMAP, BM_SETCHECK, nppGUI._isDocPeekOnMap ? BST_CHECKED : BST_UNCHECKED, 0);

			::ShowWindow(::GetDlgItem(_hSelf, IDC_CHECK_AUTOUPDATE), nppGUI._doesExistUpdater?SW_SHOW:SW_HIDE);
			
			BOOL linkEnable = FALSE;
			BOOL dontUnderline = FALSE;
			BOOL dontUnderlineState = FALSE;
			if (nppGUI._styleURL == 1)
			{
				linkEnable = TRUE;
				dontUnderline = TRUE;
				dontUnderlineState = TRUE;
				
			}
			else if (nppGUI._styleURL == 2)
			{
				linkEnable = TRUE;
				dontUnderline = FALSE;
				dontUnderlineState = TRUE;	
			}
			
			::SendDlgItemMessage(_hSelf, IDC_CHECK_CLICKABLELINK_ENABLE, BM_SETCHECK, linkEnable, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_CLICKABLELINK_NOUNDERLINE, BM_SETCHECK, dontUnderline, 0);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_CLICKABLELINK_NOUNDERLINE), dontUnderlineState);

			::SendDlgItemMessage(_hSelf, IDC_EDIT_SESSIONFILEEXT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(nppGUI._definedSessionExt.c_str()));
			::SendDlgItemMessage(_hSelf, IDC_EDIT_WORKSPACEFILEEXT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(nppGUI._definedWorkspaceExt.c_str()));
			
			::SendDlgItemMessage(_hSelf, IDC_CHECK_ENABLEDOCSWITCHER, BM_SETCHECK, nppGUI._doTaskList, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_STOPFILLINGFINDFIELD, BM_SETCHECK, nppGUI._stopFillingFindField, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_STYLEMRU, BM_SETCHECK, nppGUI._styleMRU, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_SHORTTITLE, BM_SETCHECK, nppGUI._shortTitlebar, 0);

			ETDTProc enableDlgTheme = (ETDTProc)nppParam.getEnableThemeDlgTexture();
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);

			return TRUE;
		}

		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				switch (LOWORD(wParam))
				{
					case  IDC_EDIT_SESSIONFILEEXT:
					{
						TCHAR sessionExt[MAX_PATH];
						::SendDlgItemMessage(_hSelf, IDC_EDIT_SESSIONFILEEXT, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(sessionExt));
						nppGUI._definedSessionExt = sessionExt;
						return TRUE;
					}
					case  IDC_EDIT_WORKSPACEFILEEXT:
					{
						TCHAR workspaceExt[MAX_PATH];
						::SendDlgItemMessage(_hSelf, IDC_EDIT_WORKSPACEFILEEXT, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(workspaceExt));
						nppGUI._definedWorkspaceExt = workspaceExt;
						return TRUE;
					}
				}
			}
			
			switch (wParam)
			{
				case IDC_CHECK_UPDATESILENTLY:
				case IDC_CHECK_UPDATEGOTOEOF:
				{
					bool isSilent = isCheckedOrNot(IDC_CHECK_UPDATESILENTLY);
					bool isGo2End = isCheckedOrNot(IDC_CHECK_UPDATEGOTOEOF);

					auto index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_FILEUPDATECHOICE, CB_GETCURSEL, 0, 0);

					int cd = cdDisabled;

					if (index == fileUpdateChoiceEnable || index == fileUpdateChoiceEnable4All)
					{
						if (index == fileUpdateChoiceEnable4All)
							cd |= cdEnabledOld;
						else
							cd |= cdEnabledNew;

						if (isSilent)
							cd |= cdAutoUpdate;
						if (isGo2End)
							cd |= cdGo2end;
					}

					nppGUI._fileAutoDetection = cd;
				}
				return TRUE;

				case IDC_CHECK_CLICKABLELINK_ENABLE:
				{
					bool isChecked = isCheckedOrNot(IDC_CHECK_CLICKABLELINK_ENABLE);
					if (!isChecked)
						::SendDlgItemMessage(_hSelf, IDC_CHECK_CLICKABLELINK_NOUNDERLINE, BM_SETCHECK, BST_UNCHECKED, 0);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_CLICKABLELINK_NOUNDERLINE), isChecked);
					
					nppGUI._styleURL = isChecked?2:0;
					HWND grandParent = ::GetParent(_hParent);
					::SendMessage(grandParent, NPPM_INTERNAL_UPDATECLICKABLELINKS, 0, 0);
				}
				return TRUE;

				case IDC_CHECK_CLICKABLELINK_NOUNDERLINE:
				{
					bool isChecked = isCheckedOrNot(IDC_CHECK_CLICKABLELINK_NOUNDERLINE);
					nppGUI._styleURL = isChecked?1:2;
					HWND grandParent = ::GetParent(_hParent);
					::SendMessage(grandParent, NPPM_INTERNAL_UPDATECLICKABLELINKS, 0, 0);
				}
				return TRUE;

				case IDC_CHECK_AUTOUPDATE:
					nppGUI._autoUpdateOpt._doAutoUpdate = isCheckedOrNot(static_cast<int32_t>(wParam));
					return TRUE;

				case IDC_CHECK_MIN2SYSTRAY:
					nppGUI._isMinimizedToTray = isCheckedOrNot(static_cast<int32_t>(wParam));
					return TRUE;

				case IDC_CHECK_DETECTENCODING:
					nppGUI._detectEncoding = isCheckedOrNot(static_cast<int32_t>(wParam));
					return TRUE;
				case IDC_CHECK_ENABLEDOCSWITCHER :
				{
					nppGUI._doTaskList = !nppGUI._doTaskList;
					if (nppGUI._doTaskList)
					{
						::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_STYLEMRU), TRUE);
					}
					else
					{
						nppGUI._styleMRU = false;
						::SendDlgItemMessage(_hSelf, IDC_CHECK_STYLEMRU, BM_SETCHECK, false, 0);
						::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_STYLEMRU), FALSE);
					}
					return TRUE;
				}

				case IDC_CHECK_STOPFILLINGFINDFIELD:
				{
					nppGUI._stopFillingFindField = isCheckedOrNot(IDC_CHECK_STOPFILLINGFINDFIELD);
					return TRUE;
				}

				case IDC_CHECK_STYLEMRU :
				{
					nppGUI._styleMRU = isCheckedOrNot(IDC_CHECK_STYLEMRU);
					return TRUE;
				}

				case IDC_CHECK_SHORTTITLE:
				{
					nppGUI._shortTitlebar = isCheckedOrNot(IDC_CHECK_SHORTTITLE);
					HWND grandParent = ::GetParent(_hParent);
					::SendMessage(grandParent, NPPM_INTERNAL_UPDATETITLEBAR, 0, 0);
					return TRUE;
				}

				case IDC_CHECK_MONOSPACEDFONT_FINDDLG:
				{
					nppGUI._monospacedFontFindDlg = isCheckedOrNot(IDC_CHECK_MONOSPACEDFONT_FINDDLG);
					return TRUE;
				}

				case IDC_CHECK_DIRECTWRITE_ENABLE:
				{
					nppGUI._writeTechnologyEngine = isCheckedOrNot(IDC_CHECK_DIRECTWRITE_ENABLE) ? directWriteTechnology : defaultTechnology;
					return TRUE;
				}

				case IDC_CHECK_ENABLEDOCPEEKER:
				{
					nppGUI._isDocPeekOnTab = isCheckedOrNot(IDC_CHECK_ENABLEDOCPEEKER);
					return TRUE;
				}

				case IDC_CHECK_ENABLEDOCPEEKONMAP:
				{
					nppGUI._isDocPeekOnMap = isCheckedOrNot(IDC_CHECK_ENABLEDOCPEEKONMAP);
					return TRUE;
				}
				
				default:
				{
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						if (LOWORD(wParam) == IDC_COMBO_FILEUPDATECHOICE)
						{
							auto index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_FILEUPDATECHOICE, CB_GETCURSEL, 0, 0);
							
							if (index == fileUpdateChoiceEnable || index == fileUpdateChoiceEnable4All)
							{
								bool isSilent = isCheckedOrNot(IDC_CHECK_UPDATESILENTLY);
								bool isGo2End = isCheckedOrNot(IDC_CHECK_UPDATEGOTOEOF);

								int cd = cdDisabled;

								if (index == fileUpdateChoiceEnable4All)
									cd |= cdEnabledOld;
								else
									cd |= cdEnabledNew;

								if (isSilent)
									cd |= cdAutoUpdate;
								if (isGo2End)
									cd |= cdGo2end;

								nppGUI._fileAutoDetection = cd;
								
								::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_UPDATESILENTLY), TRUE);
								::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_UPDATEGOTOEOF), TRUE);
							}
							else if (index == fileUpdateChoiceDisable)
							{
								::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_UPDATESILENTLY), FALSE);
								::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_UPDATEGOTOEOF), FALSE);
								
								nppGUI._fileAutoDetection = cdDisabled;
							}
							
							return TRUE;
						}
					}
				}
			}
		}
	}
	return FALSE;
}


void RecentFilesHistoryDlg::setCustomLen(int val)
{
	::EnableWindow(::GetDlgItem(_hSelf, IDC_CUSTOMIZELENGTHVAL_STATIC), val > 0);
	::SetDlgItemInt(_hSelf, IDC_CUSTOMIZELENGTHVAL_STATIC, val, FALSE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_CUSTOMIZELENGTHVAL_STATIC), val > 0?SW_SHOW:SW_HIDE);
}

INT_PTR CALLBACK DefaultNewDocDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )nppParam.getNppGUI();
	NewDocDefaultSettings & ndds = (NewDocDefaultSettings &)nppGUI.getNewDocDefaultSettings();

	switch (message)
	{
		case WM_INITDIALOG:
		{
			int ID2Check = IDC_RADIO_F_WIN;
			switch (ndds._format)
			{
				case EolType::windows:
					ID2Check = IDC_RADIO_F_WIN;
					break;
				case EolType::macos:
					ID2Check = IDC_RADIO_F_MAC;
					break;
				case EolType::unix:
					ID2Check = IDC_RADIO_F_UNIX;
					break;
				case EolType::unknown:
					assert(false);
					break;
			}
			::SendDlgItemMessage(_hSelf, ID2Check, BM_SETCHECK, BST_CHECKED, 0);

			switch (ndds._unicodeMode)
			{
				case uni16BE :
					ID2Check = IDC_RADIO_UCS2BIG;
					break;
				case uni16LE :
					ID2Check = IDC_RADIO_UCS2SMALL;
					break;
				case uniUTF8 :
					ID2Check = IDC_RADIO_UTF8;
					break;
				case uniCookie :
					ID2Check = IDC_RADIO_UTF8SANSBOM;
					break;

				default : //uni8Bit
					ID2Check = IDC_RADIO_ANSI;
			}

			int selIndex = -1;
			generic_string str;
			EncodingMapper& em = EncodingMapper::getInstance();
			for (size_t i = 0, encodingArraySize = sizeof(encodings)/sizeof(int) ; i < encodingArraySize ; ++i)
			{
				int cmdID = em.getIndexFromEncoding(encodings[i]);
				if (cmdID != -1)
				{
					cmdID += IDM_FORMAT_ENCODE;
					getNameStrFromCmd(cmdID, str);
					int index = static_cast<int32_t>(::SendDlgItemMessage(_hSelf, IDC_COMBO_OTHERCP, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(str.c_str())));
					if (ndds._codepage == encodings[i])
						selIndex = index;
					::SendDlgItemMessage(_hSelf, IDC_COMBO_OTHERCP, CB_SETITEMDATA, index, encodings[i]);
				}
			}

			if (ndds._codepage == -1 || selIndex == -1)
			{
				::EnableWindow(::GetDlgItem(_hSelf, IDC_COMBO_OTHERCP), false);
			}
			else
			{
				ID2Check = IDC_RADIO_OTHERCP;
				::SendDlgItemMessage(_hSelf, IDC_COMBO_OTHERCP, CB_SETCURSEL, selIndex, 0);
			}

			::SendDlgItemMessage(_hSelf, ID2Check, BM_SETCHECK, BST_CHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_OPENANSIASUTF8, BM_SETCHECK, (ID2Check == IDC_RADIO_UTF8SANSBOM && ndds._openAnsiAsUtf8)?BST_CHECKED:BST_UNCHECKED, 0);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_OPENANSIASUTF8), ID2Check == IDC_RADIO_UTF8SANSBOM);

			size_t index = 0;
			for (int i = L_TEXT ; i < nppParam.L_END ; ++i)
			{
				str.clear();
				if (static_cast<LangType>(i) != L_USER)
				{
					int cmdID = nppParam.langTypeToCommandID(static_cast<LangType>(i));
					if ((cmdID != -1))
					{
						getNameStrFromCmd(cmdID, str);
						if (str.length() > 0)
						{
							_langList.push_back(LangID_Name(static_cast<LangType>(i), str));
							::SendDlgItemMessage(_hSelf, IDC_COMBO_DEFAULTLANG, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(str.c_str()));
							if (ndds._lang == i)
								index = _langList.size() - 1;
						}
					}
				}
			}
			::SendDlgItemMessage(_hSelf, IDC_COMBO_DEFAULTLANG, CB_SETCURSEL, index, 0);

			//
			// To avoid the white control background to be displayed in dialog
			//
			ETDTProc enableDlgTheme = (ETDTProc)nppParam.getEnableThemeDlgTexture();
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);
		}

		case WM_COMMAND:
			switch (wParam)
			{
				case IDC_RADIO_UCS2BIG:
					ndds._unicodeMode = uni16BE;
					ndds._openAnsiAsUtf8 = false;
					makeOpenAnsiAsUtf8(false);
					ndds._codepage = -1;
					::EnableWindow(::GetDlgItem(_hSelf, IDC_COMBO_OTHERCP), false);
					return TRUE;
				case IDC_RADIO_UCS2SMALL:
					ndds._unicodeMode = uni16LE;
					ndds._openAnsiAsUtf8 = false;
					makeOpenAnsiAsUtf8(false);
					ndds._codepage = -1;
					::EnableWindow(::GetDlgItem(_hSelf, IDC_COMBO_OTHERCP), false);
					return TRUE;
				case IDC_RADIO_UTF8:
					ndds._unicodeMode = uniUTF8;
					ndds._openAnsiAsUtf8 = false;
					makeOpenAnsiAsUtf8(false);
					ndds._codepage = -1;
					::EnableWindow(::GetDlgItem(_hSelf, IDC_COMBO_OTHERCP), false);
					return TRUE;
				case IDC_RADIO_UTF8SANSBOM:
					ndds._unicodeMode = uniCookie;
					makeOpenAnsiAsUtf8(true);
					ndds._codepage = -1;
					::EnableWindow(::GetDlgItem(_hSelf, IDC_COMBO_OTHERCP), false);
					return TRUE;
				case IDC_RADIO_ANSI:
					ndds._unicodeMode = uni8Bit;
					ndds._openAnsiAsUtf8 = false;
					makeOpenAnsiAsUtf8(false);
					ndds._codepage = -1;
					::EnableWindow(::GetDlgItem(_hSelf, IDC_COMBO_OTHERCP), false);
					return TRUE;

				case IDC_CHECK_OPENANSIASUTF8 :
					ndds._openAnsiAsUtf8 = (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDC_CHECK_OPENANSIASUTF8), BM_GETCHECK, 0, 0));
					return TRUE;

				case IDC_RADIO_OTHERCP :
				{
					ndds._openAnsiAsUtf8 = false;
					makeOpenAnsiAsUtf8(false);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_COMBO_OTHERCP), true);
					auto index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_OTHERCP, CB_GETCURSEL, 0, 0);
					ndds._codepage = static_cast<int32_t>(::SendDlgItemMessage(_hSelf, IDC_COMBO_OTHERCP, CB_GETITEMDATA, index, 0));
					return TRUE;
				}

				case IDC_RADIO_F_MAC:
				{
					ndds._format = EolType::macos;
					return TRUE;
				}
				case IDC_RADIO_F_UNIX:
				{
					ndds._format = EolType::unix;
					return TRUE;
				}
				case IDC_RADIO_F_WIN:
				{
					ndds._format = EolType::windows;
					return TRUE;
				}

				default:
				{
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						if (LOWORD(wParam) == IDC_COMBO_DEFAULTLANG)
						{
							auto index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_DEFAULTLANG, CB_GETCURSEL, 0, 0);
							ndds._lang = _langList[index]._id;
							return TRUE;
						}
						else if (LOWORD(wParam) == IDC_COMBO_OTHERCP)
						{
							auto index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_OTHERCP, CB_GETCURSEL, 0, 0);
							ndds._codepage = static_cast<int32_t>(::SendDlgItemMessage(_hSelf, IDC_COMBO_OTHERCP, CB_GETITEMDATA, index, 0));
							return TRUE;
						}
					}
					return FALSE;
				}
			}
	}
 	return FALSE;
}

INT_PTR CALLBACK DefaultDirectoryDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )nppParam.getNppGUI();

	switch (message) 
	{
		case WM_INITDIALOG :
		{
			int ID2Check = 0;
			bool shouldActivated;
			switch (nppGUI._openSaveDir)
			{
				case dir_last :
					ID2Check = IDC_OPENSAVEDIR_REMEMBERLAST_RADIO;
					shouldActivated = false;
					break;
				case dir_userDef :
					ID2Check = IDC_OPENSAVEDIR_ALWAYSON_RADIO;
					shouldActivated = true;
					break;

				default : 
					ID2Check = IDC_OPENSAVEDIR_FOLLOWCURRENT_RADIO;
					shouldActivated = false;
			}
			::SendDlgItemMessage(_hSelf, ID2Check, BM_SETCHECK, BST_CHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_OPENSAVEDIR_ALWAYSON_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(nppGUI._defaultDir));
			::EnableWindow(::GetDlgItem(_hSelf, IDC_OPENSAVEDIR_ALWAYSON_EDIT), shouldActivated);
			::EnableWindow(::GetDlgItem(_hSelf, IDD_OPENSAVEDIR_ALWAYSON_BROWSE_BUTTON), shouldActivated);

			//
			// To avoid the white control background to be displayed in dialog
			//
			ETDTProc enableDlgTheme = (ETDTProc)nppParam.getEnableThemeDlgTexture();
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);

			::SendDlgItemMessage(_hSelf, IDC_OPENSAVEDIR_CHECK_USENEWSTYLESAVEDIALOG, BM_SETCHECK, nppGUI._useNewStyleSaveDlg ? BST_CHECKED : BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_OPENSAVEDIR_CHECK_DRROPFOLDEROPENFILES, BM_SETCHECK, nppGUI._isFolderDroppedOpenFiles ? BST_CHECKED : BST_UNCHECKED, 0);
		}

		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				switch (LOWORD(wParam))
				{
					case  IDC_OPENSAVEDIR_ALWAYSON_EDIT:
					{
						TCHAR inputDir[MAX_PATH];
						::SendDlgItemMessage(_hSelf, IDC_OPENSAVEDIR_ALWAYSON_EDIT, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(inputDir));
						wcscpy_s(nppGUI._defaultDir, inputDir);
						::ExpandEnvironmentStrings(nppGUI._defaultDir, nppGUI._defaultDirExp, _countof(nppGUI._defaultDirExp));
						nppParam.setWorkingDir(nppGUI._defaultDirExp);
						return TRUE;
					}
				}
			}

			switch (wParam)
			{
				case IDC_OPENSAVEDIR_FOLLOWCURRENT_RADIO:
					nppGUI._openSaveDir = dir_followCurrent;
					::EnableWindow(::GetDlgItem(_hSelf, IDC_OPENSAVEDIR_ALWAYSON_EDIT), false);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_OPENSAVEDIR_ALWAYSON_BROWSE_BUTTON), false);
					return TRUE;
				case IDC_OPENSAVEDIR_REMEMBERLAST_RADIO:
					nppGUI._openSaveDir = dir_last;
					::EnableWindow(::GetDlgItem(_hSelf, IDC_OPENSAVEDIR_ALWAYSON_EDIT), false);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_OPENSAVEDIR_ALWAYSON_BROWSE_BUTTON), false);
					return TRUE;
				case IDC_OPENSAVEDIR_ALWAYSON_RADIO:
					nppGUI._openSaveDir = dir_userDef;
					::EnableWindow(::GetDlgItem(_hSelf, IDC_OPENSAVEDIR_ALWAYSON_EDIT), true);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_OPENSAVEDIR_ALWAYSON_BROWSE_BUTTON), true);
					return TRUE;

				case IDD_OPENSAVEDIR_ALWAYSON_BROWSE_BUTTON :
					folderBrowser(_hSelf, TEXT("Select a folder as default directory"), IDC_OPENSAVEDIR_ALWAYSON_EDIT);
					return TRUE;

				case IDC_OPENSAVEDIR_CHECK_USENEWSTYLESAVEDIALOG:
					nppGUI._useNewStyleSaveDlg = isCheckedOrNot(IDC_OPENSAVEDIR_CHECK_USENEWSTYLESAVEDIALOG);
					return TRUE;

				case IDC_OPENSAVEDIR_CHECK_DRROPFOLDEROPENFILES:
					nppGUI._isFolderDroppedOpenFiles = isCheckedOrNot(IDC_OPENSAVEDIR_CHECK_DRROPFOLDEROPENFILES);
					return TRUE;

				default:
					return FALSE;
			}
		}
	}
	return FALSE;
}

INT_PTR CALLBACK RecentFilesHistoryDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )nppParam.getNppGUI();
	NativeLangSpeaker *pNativeSpeaker = nppParam.getNativeLangSpeaker();

	switch (message) 
	{
		case WM_INITDIALOG :
		{
			// Max number recent file setting
			::SetDlgItemInt(_hSelf, IDC_MAXNBFILEVAL_STATIC, nppParam.getNbMaxRecentFile(), FALSE);
			_nbHistoryVal.init(_hInst, _hSelf);
			_nbHistoryVal.create(::GetDlgItem(_hSelf, IDC_MAXNBFILEVAL_STATIC), IDC_MAXNBFILEVAL_STATIC);

			// Check on launch time settings
			::SendDlgItemMessage(_hSelf, IDC_CHECK_DONTCHECKHISTORY, BM_SETCHECK, !nppGUI._checkHistoryFiles, 0);

			// Display in submenu setting
			::SendDlgItemMessage(_hSelf, IDC_CHECK_INSUBMENU, BM_SETCHECK, nppParam.putRecentFileInSubMenu(), 0);

			// Recent File menu entry length setting
			int customLength = nppParam.getRecentFileCustomLength();
			int id = IDC_RADIO_CUSTOMIZELENTH;
			int length = customLength;

			if (customLength == RECENTFILES_SHOWONLYFILENAME)
			{
				id = IDC_RADIO_ONLYFILENAME;
				length = 0;
			}
			else if (customLength == RECENTFILES_SHOWFULLPATH || customLength < 0)
			{
				id = IDC_RADIO_FULLFILENAMEPATH;
				length = 0;
			}
			::SendDlgItemMessage(_hSelf, id, BM_SETCHECK, BST_CHECKED, 0);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_CUSTOMIZELENGTHVAL_STATIC), id == IDC_RADIO_CUSTOMIZELENTH);
			::ShowWindow(::GetDlgItem(_hSelf, IDC_CUSTOMIZELENGTHVAL_STATIC), id == IDC_RADIO_CUSTOMIZELENTH?SW_SHOW:SW_HIDE);
			
			::SetDlgItemInt(_hSelf, IDC_CUSTOMIZELENGTHVAL_STATIC, length, FALSE);
			_customLenVal.init(_hInst, _hSelf);
			_customLenVal.create(::GetDlgItem(_hSelf, IDC_CUSTOMIZELENGTHVAL_STATIC), nullptr);

			//
			// To avoid the white control background to be displayed in dialog
			//
			ETDTProc enableDlgTheme = (ETDTProc)nppParam.getEnableThemeDlgTexture();
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);
		}

		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDC_CHECK_DONTCHECKHISTORY:
					nppGUI._checkHistoryFiles = !isCheckedOrNot(IDC_CHECK_DONTCHECKHISTORY);
					return TRUE;

				case IDC_MAXNBFILEVAL_STATIC:
				{
					generic_string staticText = pNativeSpeaker->getLocalizedStrFromID("recent-file-history-maxfile", TEXT("Max File: "));
					ValueDlg nbFileMaxDlg;
					nbFileMaxDlg.init(NULL, _hSelf, nppParam.getNbMaxRecentFile(), staticText.c_str());
					
					POINT p;
					::GetCursorPos(&p);

					int nbMaxFile = nbFileMaxDlg.doDialog(p);
					if (nbMaxFile != -1)
					{
						if (nbMaxFile > NB_MAX_LRF_FILE)
							nbMaxFile = NB_MAX_LRF_FILE;
						
						nppParam.setNbMaxRecentFile(nbMaxFile);
						::SetDlgItemInt(_hSelf, IDC_MAXNBFILEVAL_STATIC, nbMaxFile, FALSE);

						// Validate modified value
						::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SETTING_HISTORY_SIZE, 0, 0);
					}
					return TRUE;
				}

				case IDC_CHECK_INSUBMENU:
					nppParam.setPutRecentFileInSubMenu(isCheckedOrNot(IDC_CHECK_INSUBMENU));
					::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_RECENTFILELIST_SWITCH, 0, 0);
					return TRUE;

				case IDC_RADIO_ONLYFILENAME:
					setCustomLen(0);
					nppParam.setRecentFileCustomLength(0);
					::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_RECENTFILELIST_UPDATE, 0, 0);
					return TRUE;
				case IDC_RADIO_FULLFILENAMEPATH:
					setCustomLen(0);
					nppParam.setRecentFileCustomLength(-1);
					::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_RECENTFILELIST_UPDATE, 0, 0);
					return TRUE;
				case IDC_RADIO_CUSTOMIZELENTH:
				{
					int len = nppParam.getRecentFileCustomLength();
					if (len <= 0)
					{
						setCustomLen(100);
						nppParam.setRecentFileCustomLength(100);
						::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_RECENTFILELIST_UPDATE, 0, 0);
					}
					return TRUE;
				}

				case IDC_CUSTOMIZELENGTHVAL_STATIC:
				{
					ValueDlg customLengthDlg;
					customLengthDlg.init(NULL, _hSelf, nppParam.getRecentFileCustomLength(), TEXT("Length: "));
					customLengthDlg.setNBNumber(3);

					POINT p;
					::GetCursorPos(&p);

					int size = customLengthDlg.doDialog(p);
					if (size != -1)
					{
						::SetDlgItemInt(_hSelf, IDC_CUSTOMIZELENGTHVAL_STATIC, size, FALSE);
						nppParam.setRecentFileCustomLength(size);
						::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_RECENTFILELIST_UPDATE, 0, 0);
					}
					return TRUE;
				}
				default:
					return FALSE;
			}
		}
	}
	return FALSE;
}

INT_PTR CALLBACK LangMenuDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI & nppGUI = const_cast<NppGUI &>(nppParam.getNppGUI());
	NativeLangSpeaker *pNativeSpeaker = nppParam.getNativeLangSpeaker();

	switch (message) 
	{
		case WM_INITDIALOG :
		{
			//
			// Lang Menu
			//
			for (int i = L_TEXT ; i < nppParam.L_END ; ++i)
			{
				generic_string str;
				if (static_cast<LangType>(i) != L_USER)
				{
					int cmdID = nppParam.langTypeToCommandID(static_cast<LangType>(i));
					if ((cmdID != -1))
					{
						getNameStrFromCmd(cmdID, str);
						if (str.length() > 0)
						{
							_langList.push_back(LangMenuItem(static_cast<LangType>(i), cmdID, str));
							::SendDlgItemMessage(_hSelf, IDC_LIST_ENABLEDLANG, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(str.c_str()));
						}
					}
				}
			}

			for (size_t i = 0, len = nppGUI._excludedLangList.size(); i < len ; ++i)
			{
				::SendDlgItemMessage(_hSelf, IDC_LIST_DISABLEDLANG, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(nppGUI._excludedLangList[i]._langName.c_str()));
			}

			::SendDlgItemMessage(_hSelf, IDC_CHECK_LANGMENUCOMPACT, BM_SETCHECK, nppGUI._isLangMenuCompact?BST_CHECKED:BST_UNCHECKED, 0);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_BUTTON_REMOVE), FALSE);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_BUTTON_RESTORE), FALSE);

			
			//
			// Tab settings
			//
			::SetDlgItemInt(_hSelf, IDC_TABSIZEVAL_STATIC, nppGUI._tabSize, FALSE);

			_tabSizeVal.init(_hInst, _hSelf);
			_tabSizeVal.create(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_STATIC), IDC_TABSIZEVAL_STATIC);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_REPLACEBYSPACE, BM_SETCHECK, nppGUI._tabReplacedBySpace, 0);

			::SendDlgItemMessage(_hSelf, IDC_LIST_TABSETTNG, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("[Default]")));
			const int nbLang = nppParam.getNbLang();
			for (int i = 0; i < nbLang; ++i)
			{
				::SendDlgItemMessage(_hSelf, IDC_LIST_TABSETTNG, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(nppParam.getLangFromIndex(i)->_langName.c_str()));
			}
			const int index2Begin = 0;
			::SendDlgItemMessage(_hSelf, IDC_LIST_TABSETTNG, LB_SETCURSEL, 0, index2Begin);
			::ShowWindow(::GetDlgItem(_hSelf, IDC_GR_TABVALUE_STATIC), SW_HIDE);
			::ShowWindow(::GetDlgItem(_hSelf, IDC_CHECK_DEFAULTTABVALUE), SW_HIDE);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_DISABLE_STATIC), FALSE);
			::ShowWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_DISABLE_STATIC), SW_HIDE);

			ETDTProc enableDlgTheme = reinterpret_cast<ETDTProc>(nppParam.getEnableThemeDlgTexture());
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);

			::SendDlgItemMessage(_hSelf, IDC_CHECK_BACKSLASHISESCAPECHARACTERFORSQL, BM_SETCHECK, nppGUI._backSlashIsEscapeCharacterForSql, 0);

			return TRUE;
		}

		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == LBN_SELCHANGE)
            {
				// Lang Menu
				if (LOWORD(wParam) == IDC_LIST_DISABLEDLANG || LOWORD(wParam) == IDC_LIST_ENABLEDLANG)
				{
					int idButton2Enable;
					int idButton2Disable;

					if (LOWORD(wParam) == IDC_LIST_ENABLEDLANG)
					{
						idButton2Enable = IDC_BUTTON_REMOVE;
						idButton2Disable = IDC_BUTTON_RESTORE;
					}
					else //IDC_LIST_DISABLEDLANG
					{
						idButton2Enable = IDC_BUTTON_RESTORE;
						idButton2Disable = IDC_BUTTON_REMOVE;
					}

					auto i = ::SendDlgItemMessage(_hSelf, LOWORD(wParam), LB_GETCURSEL, 0, 0);
					if (i != LB_ERR)
					{
						::EnableWindow(::GetDlgItem(_hSelf, idButton2Enable), TRUE);
						int idListbox2Disable = (LOWORD(wParam)== IDC_LIST_ENABLEDLANG)?IDC_LIST_DISABLEDLANG:IDC_LIST_ENABLEDLANG;
						::SendDlgItemMessage(_hSelf, idListbox2Disable, LB_SETCURSEL, static_cast<WPARAM>(-1), 0);
						::EnableWindow(::GetDlgItem(_hSelf, idButton2Disable), FALSE);
					}
					return TRUE;

				}
				// Tab setting
				else if (LOWORD(wParam) == IDC_LIST_TABSETTNG)
				{
					auto index = ::SendDlgItemMessage(_hSelf, IDC_LIST_TABSETTNG, LB_GETCURSEL, 0, 0);
					if (index == LB_ERR)
						return FALSE;
					::ShowWindow(::GetDlgItem(_hSelf, IDC_GR_TABVALUE_STATIC), index ? SW_SHOW : SW_HIDE);
					::ShowWindow(::GetDlgItem(_hSelf, IDC_CHECK_DEFAULTTABVALUE), index ? SW_SHOW : SW_HIDE);

					if (index)
					{
						Lang *lang = nppParam.getLangFromIndex(index - 1);
						if (!lang) return FALSE;
						bool useDefaultTab = (lang->_tabSize == -1 || lang->_tabSize == 0);

						::SendMessage(::GetDlgItem(_hSelf, IDC_CHECK_DEFAULTTABVALUE), BM_SETCHECK, useDefaultTab, 0);
						::EnableWindow(::GetDlgItem(_hSelf, IDC_TABSIZE_STATIC), !useDefaultTab);

						int size = useDefaultTab ? nppGUI._tabSize : lang->_tabSize;
						::SetDlgItemInt(_hSelf, IDC_TABSIZEVAL_STATIC, size, FALSE);
						::SetDlgItemInt(_hSelf, IDC_TABSIZEVAL_DISABLE_STATIC, size, FALSE);

						::EnableWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_STATIC), !useDefaultTab);
						::ShowWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_DISABLE_STATIC), useDefaultTab);
						::ShowWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_STATIC), !useDefaultTab);
						::SendMessage(::GetDlgItem(_hSelf, IDC_CHECK_REPLACEBYSPACE), BM_SETCHECK, useDefaultTab ? nppGUI._tabReplacedBySpace : lang->_isTabReplacedBySpace, 0);
						::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_REPLACEBYSPACE), !useDefaultTab);

						if (!useDefaultTab)
						{
							::SetDlgItemInt(_hSelf, IDC_TABSIZEVAL_STATIC, lang->_tabSize, FALSE);
							::SendMessage(::GetDlgItem(_hSelf, IDC_CHECK_REPLACEBYSPACE), BM_SETCHECK, lang->_isTabReplacedBySpace, 0);
						}
					}
					else
					{
						::EnableWindow(::GetDlgItem(_hSelf, IDC_TABSIZE_STATIC), TRUE);
						::EnableWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_STATIC), TRUE);
						::ShowWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_STATIC), SW_SHOW);
						::SetDlgItemInt(_hSelf, IDC_TABSIZEVAL_STATIC, nppGUI._tabSize, FALSE);
						::ShowWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_DISABLE_STATIC), SW_HIDE);
						::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_REPLACEBYSPACE), TRUE);
						::SendMessage(::GetDlgItem(_hSelf, IDC_CHECK_REPLACEBYSPACE), BM_SETCHECK, nppGUI._tabReplacedBySpace, 0);
					}

					return TRUE;
				}
            }

			// Check if it is double click
			else if (HIWORD(wParam) == LBN_DBLCLK)
			{
				// Lang Menu
				if (LOWORD(wParam) == IDC_LIST_DISABLEDLANG || LOWORD(wParam) == IDC_LIST_ENABLEDLANG)
				{
					// On double click an item, the item should be moved
					// from one list to other list

					HWND(lParam) == ::GetDlgItem(_hSelf, IDC_LIST_ENABLEDLANG) ?
						::SendMessage(_hSelf, WM_COMMAND, IDC_BUTTON_REMOVE, 0) :
						::SendMessage(_hSelf, WM_COMMAND, IDC_BUTTON_RESTORE, 0);
					return TRUE;
				}

				// Tab setting - Double click is not used at this moment
				/*else if (LOWORD(wParam) == IDC_LIST_TABSETTNG)
				{
				}*/
			}

			switch (wParam)
            {
				//
				// Lang Menu
				//
				case IDC_CHECK_LANGMENUCOMPACT : 
				{
					nppGUI._isLangMenuCompact = (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDC_CHECK_LANGMENUCOMPACT), BM_GETCHECK, 0, 0));
					::MessageBox(_hSelf, 
						nppGUI._isLangMenuCompact?TEXT("This option will be enabled on the next launch."):TEXT("This option will be disabled on the next launch."), 
						TEXT("Compact Language Menu"), MB_OK);
					return TRUE;
				}
				
				case IDC_CHECK_BACKSLASHISESCAPECHARACTERFORSQL:
				{
					nppGUI._backSlashIsEscapeCharacterForSql = isCheckedOrNot(IDC_CHECK_BACKSLASHISESCAPECHARACTERFORSQL);
					return TRUE;
				}

				case IDC_BUTTON_RESTORE :
				case IDC_BUTTON_REMOVE :
				{
					int list2Remove, list2Add, idButton2Enable, idButton2Disable;
					vector<LangMenuItem> *pSrcLst, *pDestLst;

					if (LOWORD(wParam)==IDC_BUTTON_REMOVE)
					{
						list2Remove = IDC_LIST_ENABLEDLANG;
						list2Add = IDC_LIST_DISABLEDLANG;
						idButton2Enable = IDC_BUTTON_RESTORE;
						idButton2Disable = IDC_BUTTON_REMOVE;
						pSrcLst = &_langList;
						pDestLst = &nppGUI._excludedLangList;
					}
					else
					{
						list2Remove = IDC_LIST_DISABLEDLANG;
						list2Add = IDC_LIST_ENABLEDLANG;
						idButton2Enable = IDC_BUTTON_REMOVE;
						idButton2Disable = IDC_BUTTON_RESTORE;
						pSrcLst = &nppGUI._excludedLangList;
						pDestLst = &_langList;
					}
					size_t iRemove = ::SendDlgItemMessage(_hSelf, list2Remove, LB_GETCURSEL, 0, 0);
					if (iRemove == -1)
						return TRUE;

					const size_t sL = 31;
					TCHAR s[sL + 1];
					auto lbTextLen = ::SendDlgItemMessage(_hSelf, list2Remove, LB_GETTEXTLEN, iRemove, 0);
					if (lbTextLen > sL)
						return TRUE;

					::SendDlgItemMessage(_hSelf, list2Remove, LB_GETTEXT, iRemove, reinterpret_cast<LPARAM>(s));

					LangMenuItem lmi = pSrcLst->at(iRemove);
					vector<LangMenuItem>::iterator lang2Remove = pSrcLst->begin() + iRemove;
					pSrcLst->erase(lang2Remove);

					auto iAdd = ::SendDlgItemMessage(_hSelf, list2Add, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s));
					::SendDlgItemMessage(_hSelf, list2Remove, LB_DELETESTRING, iRemove, 0);
					pDestLst->push_back(lmi);

					::SendDlgItemMessage(_hSelf, list2Add, LB_SETCURSEL, iAdd, 0);
					::SendDlgItemMessage(_hSelf, list2Remove, LB_SETCURSEL, static_cast<WPARAM>(-1), 0);
					::EnableWindow(::GetDlgItem(_hSelf, idButton2Enable), TRUE);
					::EnableWindow(::GetDlgItem(_hSelf, idButton2Disable), FALSE);

					if ((lmi._langType >= L_EXTERNAL) && (lmi._langType < nppParam.L_END))
					{
						bool found(false);
						for (size_t x = 0; x < nppParam.getExternalLexerDoc()->size() && !found; ++x)
						{
							TiXmlNode *lexersRoot = nppParam.getExternalLexerDoc()->at(x)->FirstChild(TEXT("NotepadPlus"))->FirstChildElement(TEXT("LexerStyles"));
							for (TiXmlNode *childNode = lexersRoot->FirstChildElement(TEXT("LexerType"));
								childNode ;
								childNode = childNode->NextSibling(TEXT("LexerType")))
							{
								TiXmlElement *element = childNode->ToElement();

								if (generic_string(element->Attribute(TEXT("name"))) == lmi._langName)
								{
									element->SetAttribute(TEXT("excluded"), (LOWORD(wParam)==IDC_BUTTON_REMOVE)?TEXT("yes"):TEXT("no"));
									nppParam.getExternalLexerDoc()->at(x)->SaveFile();
									found = true;
									break;
								}
							}
						}
					}

					HWND grandParent = ::GetParent(_hParent);

					if (LOWORD(wParam)==IDC_BUTTON_REMOVE)
					{
						HMENU menu2remove = reinterpret_cast<HMENU>(::SendMessage(grandParent, NPPM_INTERNAL_GETMENU, 0, 0));
						::DeleteMenu(menu2remove, lmi._cmdID, MF_BYCOMMAND);
					}
					else
					{
						HMENU menu = HMENU(::SendMessage(grandParent, NPPM_INTERNAL_GETMENU, 0, 0));
						HMENU subMenu = ::GetSubMenu(menu, MENUINDEX_LANGUAGE);

						// Add back a languge menu item always before the 3 last items:
						// 1. -----------------------
						// 2. Define your language...
						// 3. User-Defined
						int nbItem = ::GetMenuItemCount(subMenu);

						if (nbItem < 3)
							return FALSE;

						::InsertMenu(subMenu, nbItem - 3, MF_BYPOSITION, lmi._cmdID, lmi._langName.c_str());
					}
					::DrawMenuBar(grandParent);
					return TRUE;
				}

				//
				// Tab setting
				//
				case IDC_TABSIZEVAL_STATIC:
				{
					generic_string staticText = pNativeSpeaker->getLocalizedStrFromID("language-tabsize", TEXT("Tab Size: "));
					ValueDlg tabSizeDlg;
					tabSizeDlg.init(_hInst, _hParent, nppGUI._tabSize, staticText.c_str());
					POINT p;
					::GetCursorPos(&p);
					int size = tabSizeDlg.doDialog(p);

					//Tab size 0 removal
					if (size <= 0) return FALSE;

					::SetDlgItemInt(_hSelf, IDC_TABSIZEVAL_STATIC, size, FALSE);
					::SetDlgItemInt(_hSelf, IDC_TABSIZEVAL_DISABLE_STATIC, size, FALSE);

					auto index = ::SendDlgItemMessage(_hSelf, IDC_LIST_TABSETTNG, LB_GETCURSEL, 0, 0);
					if (index == LB_ERR) return FALSE;

					if (index != 0)
					{
						Lang *lang = nppParam.getLangFromIndex(index - 1);
						if (!lang) return FALSE;
						if (lang->_langID == L_JS)
						{
							Lang *ljs = nppParam.getLangFromID(L_JAVASCRIPT);
							ljs->_tabSize = size;
						}
						else if (lang->_langID == L_JAVASCRIPT)
						{
							Lang *ljavascript = nppParam.getLangFromID(L_JS);
							ljavascript->_tabSize = size;
						}

						lang->_tabSize = size;

						// write in langs.xml
						nppParam.insertTabInfo(lang->getLangName(), lang->getTabInfo());
					}
					else
					{
						nppGUI._tabSize = size;
					}

					::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SETTING_TAB_SIZE, 0, 0);
					return TRUE;
				}

				case IDC_CHECK_REPLACEBYSPACE:
				{
					bool isTabReplacedBySpace = BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDC_CHECK_REPLACEBYSPACE), BM_GETCHECK, 0, 0);
					auto index = ::SendDlgItemMessage(_hSelf, IDC_LIST_TABSETTNG, LB_GETCURSEL, 0, 0);
					if (index == LB_ERR) return FALSE;
					if (index != 0)
					{
						Lang *lang = nppParam.getLangFromIndex(index - 1);
						if (!lang) return FALSE;
						if (!lang->_tabSize || lang->_tabSize == -1)
							lang->_tabSize = nppGUI._tabSize;

						if (lang->_langID == L_JS)
						{
							Lang *ljs = nppParam.getLangFromID(L_JAVASCRIPT);
							ljs->_isTabReplacedBySpace = isTabReplacedBySpace;
						}
						else if (lang->_langID == L_JAVASCRIPT)
						{
							Lang *ljavascript = nppParam.getLangFromID(L_JS);
							ljavascript->_isTabReplacedBySpace = isTabReplacedBySpace;
						}

						lang->_isTabReplacedBySpace = isTabReplacedBySpace;

						// write in langs.xml
						nppParam.insertTabInfo(lang->getLangName(), lang->getTabInfo());
					}
					else
					{
						nppGUI._tabReplacedBySpace = isTabReplacedBySpace;
					}
					::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SETTING_TAB_REPLCESPACE, 0, 0);
					return TRUE;
				}

				case IDC_CHECK_DEFAULTTABVALUE:
				{
					bool useDefaultTab = BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDC_CHECK_DEFAULTTABVALUE), BM_GETCHECK, 0, 0);
					auto index = ::SendDlgItemMessage(_hSelf, IDC_LIST_TABSETTNG, LB_GETCURSEL, 0, 0);
					if (index == LB_ERR || index == 0) // index == 0 shouldn't happen
						return FALSE;

					Lang *lang = nppParam.getLangFromIndex(index - 1);
					if (!lang)
						return FALSE;

					//- Set tab setting in choosed language
					lang->_tabSize = useDefaultTab ? 0 : nppGUI._tabSize;
					lang->_isTabReplacedBySpace = useDefaultTab ? false : nppGUI._tabReplacedBySpace;

					//- set visual effect
					::EnableWindow(::GetDlgItem(_hSelf, IDC_TABSIZE_STATIC), !useDefaultTab);
					::SetDlgItemInt(_hSelf, IDC_TABSIZEVAL_STATIC, useDefaultTab ? nppGUI._tabSize : lang->_tabSize, FALSE);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_STATIC), !useDefaultTab);
					::ShowWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_DISABLE_STATIC), useDefaultTab);
					::ShowWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_STATIC), !useDefaultTab);
					::SendMessage(::GetDlgItem(_hSelf, IDC_CHECK_REPLACEBYSPACE), BM_SETCHECK, useDefaultTab ? nppGUI._tabReplacedBySpace : lang->_isTabReplacedBySpace, 0);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_REPLACEBYSPACE), !useDefaultTab);

					// write in langs.xml
					if (useDefaultTab)
						nppParam.insertTabInfo(lang->getLangName(), -1);

					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

INT_PTR CALLBACK Highlighting::run_dlgProc(UINT message, WPARAM wParam, LPARAM/* lParam*/)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )nppParam.getNppGUI();

	switch (message) 
	{
		case WM_INITDIALOG :
		{
			::SendDlgItemMessage(_hSelf, IDC_CHECK_ENABLSMARTHILITE, BM_SETCHECK, nppGUI._enableSmartHilite, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_SMARTHILITECASESENSITIVE, BM_SETCHECK, nppGUI._smartHiliteCaseSensitive, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_SMARTHILITEWHOLEWORDONLY, BM_SETCHECK, nppGUI._smartHiliteWordOnly, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_SMARTHILITEUSEFINDSETTINGS, BM_SETCHECK, nppGUI._smartHiliteUseFindSettings, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_SMARTHILITEANOTHERRVIEW, BM_SETCHECK, nppGUI._smartHiliteOnAnotherView, 0);

			::SendDlgItemMessage(_hSelf, IDC_CHECK_ENABLTAGSMATCHHILITE, BM_SETCHECK, nppGUI._enableTagsMatchHilite, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_ENABLTAGATTRHILITE, BM_SETCHECK, nppGUI._enableTagAttrsHilite, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_HIGHLITENONEHTMLZONE, BM_SETCHECK, nppGUI._enableHiliteNonHTMLZone, 0);

			::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_ENABLTAGATTRHILITE), nppGUI._enableTagsMatchHilite);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_HIGHLITENONEHTMLZONE), nppGUI._enableTagsMatchHilite);

			::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_SMARTHILITECASESENSITIVE), nppGUI._enableSmartHilite);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_SMARTHILITEWHOLEWORDONLY), nppGUI._enableSmartHilite);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_SMARTHILITEUSEFINDSETTINGS), nppGUI._enableSmartHilite);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_SMARTHILITEANOTHERRVIEW), nppGUI._enableSmartHilite);

			ETDTProc enableDlgTheme = reinterpret_cast<ETDTProc>(nppParam.getEnableThemeDlgTexture());
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);

			return TRUE;
		}

		case WM_COMMAND : 
		{
			switch (wParam)
            {
				case IDC_CHECK_ENABLSMARTHILITE:
				{
					nppGUI._enableSmartHilite = !nppGUI._enableSmartHilite;
					if (!nppGUI._enableSmartHilite)
					{
						//HWND grandParent = ::GetParent(_hParent);
						//::SendMessage(grandParent, NPPM_INTERNAL_CLEARINDICATOR, 0, 0);
					}
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_SMARTHILITECASESENSITIVE), nppGUI._enableSmartHilite);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_SMARTHILITEWHOLEWORDONLY), nppGUI._enableSmartHilite);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_SMARTHILITEUSEFINDSETTINGS), nppGUI._enableSmartHilite);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_SMARTHILITEANOTHERRVIEW), nppGUI._enableSmartHilite);
					HWND grandParent = ::GetParent(_hParent);
					::SendMessage(grandParent, NPPM_INTERNAL_CLEARINDICATOR, 0, 0);
					return TRUE;
				}

				case IDC_CHECK_SMARTHILITECASESENSITIVE:
				{
					nppGUI._smartHiliteCaseSensitive = isCheckedOrNot(IDC_CHECK_SMARTHILITECASESENSITIVE);
					if (nppGUI._smartHiliteCaseSensitive)
					{
						::SendDlgItemMessage(_hSelf, IDC_CHECK_SMARTHILITEUSEFINDSETTINGS, BM_SETCHECK, false, 0);
						nppGUI._smartHiliteUseFindSettings = false;
					}
					HWND grandParent = ::GetParent(_hParent);
					::SendMessage(grandParent, NPPM_INTERNAL_CLEARINDICATOR, 0, 0);
					return TRUE;
				}
				case IDC_CHECK_SMARTHILITEWHOLEWORDONLY:
				{
					nppGUI._smartHiliteWordOnly = isCheckedOrNot(IDC_CHECK_SMARTHILITEWHOLEWORDONLY);
					if (nppGUI._smartHiliteWordOnly)
					{
						::SendDlgItemMessage(_hSelf, IDC_CHECK_SMARTHILITEUSEFINDSETTINGS, BM_SETCHECK, false, 0);
						nppGUI._smartHiliteUseFindSettings = false;
					}
					HWND grandParent = ::GetParent(_hParent);
					::SendMessage(grandParent, NPPM_INTERNAL_CLEARINDICATOR, 0, 0);
					return TRUE;
				}
				case IDC_CHECK_SMARTHILITEUSEFINDSETTINGS:
				{
					nppGUI._smartHiliteUseFindSettings = isCheckedOrNot(IDC_CHECK_SMARTHILITEUSEFINDSETTINGS);
					if (nppGUI._smartHiliteUseFindSettings)
					{
						::SendDlgItemMessage(_hSelf, IDC_CHECK_SMARTHILITECASESENSITIVE, BM_SETCHECK, false, 0);
						::SendDlgItemMessage(_hSelf, IDC_CHECK_SMARTHILITEWHOLEWORDONLY, BM_SETCHECK, false, 0);
						nppGUI._smartHiliteCaseSensitive = false;
						nppGUI._smartHiliteWordOnly = false;
					}
					HWND grandParent = ::GetParent(_hParent);
					::SendMessage(grandParent, NPPM_INTERNAL_CLEARINDICATOR, 0, 0);
					return TRUE;
				}
				case IDC_CHECK_SMARTHILITEANOTHERRVIEW:
				{
					nppGUI._smartHiliteOnAnotherView = isCheckedOrNot(IDC_CHECK_SMARTHILITEANOTHERRVIEW);

					HWND grandParent = ::GetParent(_hParent);
					::SendMessage(grandParent, NPPM_INTERNAL_CLEARINDICATOR, 0, 0);
					return TRUE;
				}

				case IDC_CHECK_ENABLTAGSMATCHHILITE:
				{
					nppGUI._enableTagsMatchHilite = !nppGUI._enableTagsMatchHilite;
					if (!nppGUI._enableTagsMatchHilite)
					{
						HWND grandParent = ::GetParent(_hParent);
						::SendMessage(grandParent, NPPM_INTERNAL_CLEARINDICATORTAGMATCH, 0, 0);
					}
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_ENABLTAGATTRHILITE), nppGUI._enableTagsMatchHilite);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_HIGHLITENONEHTMLZONE), nppGUI._enableTagsMatchHilite);
					return TRUE;
				}

				case IDC_CHECK_ENABLTAGATTRHILITE:
				{
					nppGUI._enableTagAttrsHilite = !nppGUI._enableTagAttrsHilite;
					if (!nppGUI._enableTagAttrsHilite)
					{
						HWND grandParent = ::GetParent(_hParent);
						::SendMessage(grandParent, NPPM_INTERNAL_CLEARINDICATORTAGATTR, 0, 0);
					}
					return TRUE;
				}

				case IDC_CHECK_HIGHLITENONEHTMLZONE:
				{
					nppGUI._enableHiliteNonHTMLZone = !nppGUI._enableHiliteNonHTMLZone;
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

void trim(generic_string & str)
{
	generic_string::size_type pos = str.find_last_not_of(' ');

	if (pos != generic_string::npos)
	{
		str.erase(pos + 1);
		pos = str.find_first_not_of(' ');
		if (pos != generic_string::npos) str.erase(0, pos);
	}
	else str.erase(str.begin(), str.end());
};

INT_PTR CALLBACK PrintSettingsDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )nppParam.getNppGUI();

	switch (message) 
	{
		case WM_INITDIALOG :
		{
			bool printLN = nppGUI._printSettings._printLineNumber;
			::SendDlgItemMessage(_hSelf, IDC_CHECK_PRINTLINENUM, BM_SETCHECK, printLN, 0);

			int ID2Check = 0;
			switch (nppGUI._printSettings._printOption)
			{
				case SC_PRINT_NORMAL :
					ID2Check = IDC_RADIO_WYSIWYG;
					break;
				case SC_PRINT_INVERTLIGHT :
					ID2Check = IDC_RADIO_INVERT;
					break;
				case SC_PRINT_BLACKONWHITE :
					ID2Check = IDC_RADIO_BW;
					break;
				case SC_PRINT_COLOURONWHITE :
					ID2Check = IDC_RADIO_NOBG;
					break;
			}
			::SendDlgItemMessage(_hSelf, ID2Check, BM_SETCHECK, BST_CHECKED, 0);

			::SetDlgItemInt(_hSelf, IDC_EDIT_ML, nppGUI._printSettings._marge.left, FALSE);
			::SetDlgItemInt(_hSelf, IDC_EDIT_MR, nppGUI._printSettings._marge.right, FALSE);
			::SetDlgItemInt(_hSelf, IDC_EDIT_MT, nppGUI._printSettings._marge.top, FALSE);
			::SetDlgItemInt(_hSelf, IDC_EDIT_MB,  nppGUI._printSettings._marge.bottom, FALSE);
			::SetDlgItemText(_hSelf, IDC_EDIT_HLEFT, nppGUI._printSettings._headerLeft.c_str());
			::SetDlgItemText(_hSelf, IDC_EDIT_HMIDDLE, nppGUI._printSettings._headerMiddle.c_str());
			::SetDlgItemText(_hSelf, IDC_EDIT_HRIGHT, nppGUI._printSettings._headerRight.c_str());
			::SetDlgItemText(_hSelf, IDC_EDIT_FLEFT, nppGUI._printSettings._footerLeft.c_str());
			::SetDlgItemText(_hSelf, IDC_EDIT_FMIDDLE, nppGUI._printSettings._footerMiddle.c_str());
			::SetDlgItemText(_hSelf, IDC_EDIT_FRIGHT, nppGUI._printSettings._footerRight.c_str());

			TCHAR intStr[5];
			for (size_t i = 6 ; i < 15 ; ++i)
			{
				wsprintf(intStr, TEXT("%d"), i);
				::SendDlgItemMessage(_hSelf, IDC_COMBO_HFONTSIZE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(intStr));
				::SendDlgItemMessage(_hSelf, IDC_COMBO_FFONTSIZE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(intStr));
			}
			const std::vector<generic_string> & fontlist = nppParam.getFontList();
			for (size_t i = 0, len = fontlist.size() ; i < len ; ++i)
			{
				auto j = ::SendDlgItemMessage(_hSelf, IDC_COMBO_HFONTNAME, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(fontlist[i].c_str()));
				::SendDlgItemMessage(_hSelf, IDC_COMBO_FFONTNAME, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(fontlist[i].c_str()));

				::SendDlgItemMessage(_hSelf, IDC_COMBO_HFONTNAME, CB_SETITEMDATA, j, reinterpret_cast<LPARAM>(fontlist[i].c_str()));
				::SendDlgItemMessage(_hSelf, IDC_COMBO_FFONTNAME, CB_SETITEMDATA, j, reinterpret_cast<LPARAM>(fontlist[i].c_str()));
			}

			auto index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_HFONTNAME, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(nppGUI._printSettings._headerFontName.c_str()));
			if (index == CB_ERR)
				index = 0;
			::SendDlgItemMessage(_hSelf, IDC_COMBO_HFONTNAME, CB_SETCURSEL, index, 0);
			index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_FFONTNAME, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(nppGUI._printSettings._footerFontName.c_str()));
			if (index == CB_ERR)
				index = 0;
			::SendDlgItemMessage(_hSelf, IDC_COMBO_FFONTNAME, CB_SETCURSEL, index, 0);

			wsprintf(intStr, TEXT("%d"), nppGUI._printSettings._headerFontSize);
			::SendDlgItemMessage(_hSelf, IDC_COMBO_HFONTSIZE, CB_SELECTSTRING, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(intStr));
			wsprintf(intStr, TEXT("%d"), nppGUI._printSettings._footerFontSize);
			::SendDlgItemMessage(_hSelf, IDC_COMBO_FFONTSIZE, CB_SELECTSTRING, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(intStr));

			::SendDlgItemMessage(_hSelf, IDC_CHECK_HBOLD, BM_SETCHECK, nppGUI._printSettings._headerFontStyle & (FONTSTYLE_BOLD ? TRUE : FALSE), 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_HITALIC, BM_SETCHECK, nppGUI._printSettings._headerFontStyle & (FONTSTYLE_ITALIC ? TRUE : FALSE), 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_FBOLD, BM_SETCHECK, nppGUI._printSettings._footerFontStyle & (FONTSTYLE_BOLD ? TRUE : FALSE), 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_FITALIC, BM_SETCHECK, nppGUI._printSettings._footerFontStyle & (FONTSTYLE_ITALIC ? TRUE : FALSE), 0);

			varList.push_back(strCouple(TEXT("Full file name path"), TEXT("$(FULL_CURRENT_PATH)")));
			varList.push_back(strCouple(TEXT("File name"), TEXT("$(FILE_NAME)")));
			varList.push_back(strCouple(TEXT("File directory"), TEXT("$(CURRENT_DIRECTORY)")));
			varList.push_back(strCouple(TEXT("Page"), TEXT("$(CURRENT_PRINTING_PAGE)")));
			varList.push_back(strCouple(TEXT("Short date format"), TEXT("$(SHORT_DATE)")));
			varList.push_back(strCouple(TEXT("Long date format"), TEXT("$(LONG_DATE)")));
			varList.push_back(strCouple(TEXT("Time"), TEXT("$(TIME)")));

			for (size_t i = 0, len = varList.size() ; i < len ; ++i)
			{
				auto j = ::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(varList[i]._varDesc.c_str()));
				::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_SETITEMDATA, j, reinterpret_cast<LPARAM>(varList[i]._var.c_str()));
			}
			::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_SETCURSEL, 0, 0);



			ETDTProc enableDlgTheme = (ETDTProc)nppParam.getEnableThemeDlgTexture();
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);
			break;
		}
		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				switch (LOWORD(wParam))
				{
					case  IDC_EDIT_ML:
						nppGUI._printSettings._marge.left = ::GetDlgItemInt(_hSelf, IDC_EDIT_ML, NULL, FALSE);
						return TRUE;

					case  IDC_EDIT_MR:
						nppGUI._printSettings._marge.right = ::GetDlgItemInt(_hSelf, IDC_EDIT_MR, NULL, FALSE);
						return TRUE;

					case IDC_EDIT_MT :
						nppGUI._printSettings._marge.top = ::GetDlgItemInt(_hSelf, IDC_EDIT_MT, NULL, FALSE);
						return TRUE;

					case IDC_EDIT_MB :
						nppGUI._printSettings._marge.bottom = ::GetDlgItemInt(_hSelf, IDC_EDIT_MB, NULL, FALSE);
						return TRUE;

					default :
					{
						const int stringSize = 256;
						TCHAR str[stringSize];
						_focusedEditCtrl = LOWORD(wParam);
						::GetDlgItemText(_hSelf, _focusedEditCtrl, str, stringSize);
						::SendDlgItemMessage(_hSelf, IDC_VIEWPANEL_STATIC, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(str));
						switch (LOWORD(wParam))
						{
							case  IDC_EDIT_HLEFT:
								nppGUI._printSettings._headerLeft = str;
								trim(nppGUI._printSettings._headerLeft);
								return TRUE;

							case  IDC_EDIT_HMIDDLE:
								nppGUI._printSettings._headerMiddle = str;
								trim(nppGUI._printSettings._headerMiddle);
								return TRUE;

							case IDC_EDIT_HRIGHT :
								nppGUI._printSettings._headerRight = str;
								trim(nppGUI._printSettings._headerRight);
								return TRUE;

							case  IDC_EDIT_FLEFT:
								nppGUI._printSettings._footerLeft = str;
								trim(nppGUI._printSettings._footerLeft);
								return TRUE;

							case  IDC_EDIT_FMIDDLE:
								nppGUI._printSettings._footerMiddle = str;
								trim(nppGUI._printSettings._footerMiddle);
								return TRUE;

							case IDC_EDIT_FRIGHT :
								nppGUI._printSettings._footerRight = str;
								trim(nppGUI._printSettings._footerRight);
								return TRUE;

							default :
								return FALSE;
						}
					}
				}
			}
			else if (HIWORD(wParam) == EN_SETFOCUS)
			{
				const int stringSize = 256;
				TCHAR str[stringSize];
				_focusedEditCtrl = LOWORD(wParam);
				
				int focusedEditStatic = 0;
				int groupStatic = 0;
				switch (_focusedEditCtrl)
				{
					case IDC_EDIT_HLEFT : focusedEditStatic = IDC_HL_STATIC; groupStatic = IDC_HGB_STATIC; break;
					case IDC_EDIT_HMIDDLE : focusedEditStatic = IDC_HM_STATIC; groupStatic = IDC_HGB_STATIC; break;
					case IDC_EDIT_HRIGHT : focusedEditStatic = IDC_HR_STATIC; groupStatic = IDC_HGB_STATIC; break;
					case IDC_EDIT_FLEFT : focusedEditStatic = IDC_FL_STATIC; groupStatic = IDC_FGB_STATIC; break;
					case IDC_EDIT_FMIDDLE : focusedEditStatic = IDC_FM_STATIC; groupStatic = IDC_FGB_STATIC; break;
					case IDC_EDIT_FRIGHT : focusedEditStatic = IDC_FR_STATIC; groupStatic = IDC_FGB_STATIC; break;
					default : return TRUE;
				}
				::GetDlgItemText(_hSelf, _focusedEditCtrl, str, stringSize);
				::SendDlgItemMessage(_hSelf, IDC_VIEWPANEL_STATIC, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(str));
				::GetDlgItemText(_hSelf, groupStatic, str, stringSize);
				generic_string title = str;
				title += TEXT(" ");
				::GetDlgItemText(_hSelf, focusedEditStatic, str, stringSize);
				title += str;
				title += TEXT(" : ");
					
				::SendDlgItemMessage(_hSelf, IDC_WHICHPART_STATIC, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(title.c_str()));
				return TRUE;
			}
			else if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				auto iSel = ::SendDlgItemMessage(_hSelf, LOWORD(wParam), CB_GETCURSEL, 0, 0);

				switch (LOWORD(wParam))
				{
					case IDC_COMBO_HFONTNAME :
					case IDC_COMBO_FFONTNAME :
					{
						TCHAR *fnStr = (TCHAR *)::SendDlgItemMessage(_hSelf, LOWORD(wParam), CB_GETITEMDATA, iSel, 0);
						if (LOWORD(wParam) == IDC_COMBO_HFONTNAME)
							nppGUI._printSettings._headerFontName = fnStr;
						else
							nppGUI._printSettings._footerFontName = fnStr;
					}
					break;


					case IDC_COMBO_HFONTSIZE :
					case IDC_COMBO_FFONTSIZE :
					{
						const size_t intStrLen = 3;
						TCHAR intStr[intStrLen];

						auto lbTextLen = ::SendDlgItemMessage(_hSelf, LOWORD(wParam), CB_GETLBTEXTLEN, iSel, 0);
						if (lbTextLen >= intStrLen)
							return TRUE;

						::SendDlgItemMessage(_hSelf, LOWORD(wParam), CB_GETLBTEXT, iSel, reinterpret_cast<LPARAM>(intStr));

						int *pVal = (LOWORD(wParam) == IDC_COMBO_HFONTSIZE)?&(nppGUI._printSettings._headerFontSize):&(nppGUI._printSettings._footerFontSize);

						if (!intStr[0])
							*pVal = 0;
						else
							*pVal = generic_strtol(intStr, NULL, 10);
					}
					break;

					case IDC_COMBO_VARLIST :
					{
					}
					break;
				}
				return TRUE;
			
			}

			switch (wParam)
			{
				case IDC_CHECK_PRINTLINENUM:
					nppGUI._printSettings._printLineNumber = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_PRINTLINENUM, BM_GETCHECK, 0, 0));
					break;

				case  IDC_RADIO_WYSIWYG:
					nppGUI._printSettings._printOption = SC_PRINT_NORMAL;
					break;

				case  IDC_RADIO_INVERT:
					nppGUI._printSettings._printOption = SC_PRINT_INVERTLIGHT;
					break;

				case IDC_RADIO_BW :
					nppGUI._printSettings._printOption = SC_PRINT_BLACKONWHITE;
					break;

				case IDC_RADIO_NOBG :
					nppGUI._printSettings._printOption = SC_PRINT_COLOURONWHITE;
					break;
				case IDC_CHECK_HBOLD:
					nppGUI._printSettings._headerFontStyle ^= FONTSTYLE_BOLD;
					break;

				case  IDC_CHECK_HITALIC:
					nppGUI._printSettings._headerFontStyle ^= FONTSTYLE_ITALIC;
					break;

				case  IDC_CHECK_FBOLD:
					nppGUI._printSettings._footerFontStyle ^= FONTSTYLE_BOLD;
					break;

				case  IDC_CHECK_FITALIC:
					nppGUI._printSettings._footerFontStyle ^= FONTSTYLE_ITALIC;
					break;

				case  IDC_BUTTON_ADDVAR:
				{
					try {
						if (!_focusedEditCtrl)
							return TRUE;

						auto iSel = ::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_GETCURSEL, 0, 0);
						TCHAR *varStr = (TCHAR *)::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_GETITEMDATA, iSel, 0);
						size_t selStart = 0;
						size_t selEnd = 0;
						::SendDlgItemMessage(_hSelf, _focusedEditCtrl, EM_GETSEL, reinterpret_cast<WPARAM>(&selStart), reinterpret_cast<LPARAM>(&selEnd));

						const int stringSize = 256;
						TCHAR str[stringSize];
						::SendDlgItemMessage(_hSelf, _focusedEditCtrl, WM_GETTEXT, stringSize, reinterpret_cast<LPARAM>(str));

						generic_string str2Set(str);
						size_t strLen = str2Set.length();
						if (selStart > strLen || selEnd > strLen)
							selStart = selEnd = strLen;

						str2Set.replace(selStart, selEnd - selStart, varStr);

						::SetDlgItemText(_hSelf, _focusedEditCtrl, str2Set.c_str());
					}
					catch (...)
					{
						// Do nothing
					}
				}
				break;
			}
			return TRUE;
		}
	}
	return FALSE;
}


INT_PTR CALLBACK BackupDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI & nppGUI = const_cast<NppGUI &>(nppParam.getNppGUI());
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			::SendDlgItemMessage(_hSelf, IDC_CHECK_REMEMBERSESSION, BM_SETCHECK, nppGUI._rememberLastSession, 0);
			bool snapshotCheck = nppGUI._rememberLastSession && nppGUI.isSnapshotMode();
			::SendDlgItemMessage(_hSelf, IDC_BACKUPDIR_RESTORESESSION_CHECK, BM_SETCHECK, snapshotCheck?BST_CHECKED:BST_UNCHECKED, 0);
			auto periodicBackupInSec = static_cast<UINT>(nppGUI._snapshotBackupTiming / 1000);
			::SetDlgItemInt(_hSelf, IDC_BACKUPDIR_RESTORESESSION_EDIT,periodicBackupInSec, FALSE);
			generic_string backupFilePath = NppParameters::getInstance().getUserPath();
			backupFilePath += TEXT("\\backup\\");
			::SetDlgItemText(_hSelf, IDD_BACKUPDIR_RESTORESESSION_PATH_EDIT, backupFilePath.c_str());

			int ID2CheckBackupOnSave = 0;

			switch (nppGUI._backup)
			{
				case bak_simple :
					ID2CheckBackupOnSave = IDC_RADIO_BKSIMPLE;
					break;
				case bak_verbose :
					ID2CheckBackupOnSave = IDC_RADIO_BKVERBOSE;
					break;
				
				default : //bak_none
					ID2CheckBackupOnSave = IDC_RADIO_BKNONE;
			}
			::SendDlgItemMessage(_hSelf, ID2CheckBackupOnSave, BM_SETCHECK, BST_CHECKED, 0);

			if (nppGUI._useDir)
				::SendDlgItemMessage(_hSelf, IDC_BACKUPDIR_CHECK, BM_SETCHECK, BST_CHECKED, 0);

			::SendDlgItemMessage(_hSelf, IDC_BACKUPDIR_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>((nppGUI._backupDir.c_str())));

			updateBackupGUI();
			return TRUE;
		}
		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				switch (LOWORD(wParam))
				{
					case  IDC_BACKUPDIR_EDIT:
					{
						TCHAR inputDir[MAX_PATH];
						::SendDlgItemMessage(_hSelf, IDC_BACKUPDIR_EDIT, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(inputDir));
						nppGUI._backupDir = inputDir;
						return TRUE;
					}

					case IDC_BACKUPDIR_RESTORESESSION_EDIT:
					{
						
						const int stringSize = 16;
						TCHAR str[stringSize];

						::GetDlgItemText(_hSelf, IDC_BACKUPDIR_RESTORESESSION_EDIT, str, stringSize);

						if (lstrcmp(str, TEXT("")) == 0)
							return TRUE;

						nppGUI._snapshotBackupTiming = ::GetDlgItemInt(_hSelf, IDC_BACKUPDIR_RESTORESESSION_EDIT, NULL, FALSE) * 1000;
						if (!nppGUI._snapshotBackupTiming)
						{
							nppGUI._snapshotBackupTiming = 1000;
							::SetDlgItemInt(_hSelf, IDC_BACKUPDIR_RESTORESESSION_EDIT, 1, FALSE);
						}
						return TRUE;
					}
				}
			}
			else if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				switch (LOWORD(wParam))
				{
					case  IDC_BACKUPDIR_RESTORESESSION_EDIT:
					{
						//printStr(TEXT(""));
						const int stringSize = 16;
						TCHAR str[stringSize];

						::GetDlgItemText(_hSelf, IDC_BACKUPDIR_RESTORESESSION_EDIT, str, stringSize);

						if (lstrcmp(str, TEXT("")) == 0)
						{
							::SetDlgItemInt(_hSelf, IDC_BACKUPDIR_RESTORESESSION_EDIT, static_cast<int32_t>(nppGUI._snapshotBackupTiming / 1000), FALSE);
						}
					}
				}
			}

			switch (wParam)
			{
				case IDC_CHECK_REMEMBERSESSION:
				{
					nppGUI._rememberLastSession = isCheckedOrNot(IDC_CHECK_REMEMBERSESSION);
					if (!nppGUI._rememberLastSession)
					{
						::SendDlgItemMessage(_hSelf, IDC_BACKUPDIR_RESTORESESSION_CHECK, BM_SETCHECK, BST_UNCHECKED, 0);
						::SendMessage(_hSelf, WM_COMMAND, IDC_BACKUPDIR_RESTORESESSION_CHECK, 0);
					}
					updateBackupGUI();
					return TRUE;
				}
				case IDC_BACKUPDIR_RESTORESESSION_CHECK:
				{
					nppGUI._isSnapshotMode = BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_BACKUPDIR_RESTORESESSION_CHECK, BM_GETCHECK, 0, 0);
					updateBackupGUI();

					if (nppGUI._isSnapshotMode)
					{
						// Launch thread
						::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_ENABLESNAPSHOT, 0, 0);
					}
					return TRUE;
				}

				case IDC_RADIO_BKSIMPLE:
				{
					nppGUI._backup = bak_simple;
					updateBackupGUI();
					return TRUE;
				}

				case IDC_RADIO_BKVERBOSE:
				{
					nppGUI._backup = bak_verbose;
					updateBackupGUI();
					return TRUE;
				}

				case IDC_RADIO_BKNONE:
				{
					nppGUI._backup = bak_none;
					updateBackupGUI();
					return TRUE;
				}

				case IDC_BACKUPDIR_CHECK:
				{
					nppGUI._useDir = !nppGUI._useDir;
					updateBackupGUI();
					return TRUE;
				}
				case IDD_BACKUPDIR_BROWSE_BUTTON :
				{
					folderBrowser(_hSelf, TEXT("Select a folder as backup directory"), IDC_BACKUPDIR_EDIT);
					return TRUE;
				}


				default :
					return FALSE;
			}
			
		}
	}
	return FALSE;
}

void BackupDlg::updateBackupGUI()
{
	bool rememberSession = isCheckedOrNot(IDC_CHECK_REMEMBERSESSION);
	bool isSnapshot = isCheckedOrNot(IDC_BACKUPDIR_RESTORESESSION_CHECK);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_BACKUPDIR_RESTORESESSION_CHECK), rememberSession);
	::EnableWindow(::GetDlgItem(_hSelf, IDD_BACKUPDIR_RESTORESESSION_STATIC1), isSnapshot);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_BACKUPDIR_RESTORESESSION_EDIT), isSnapshot);
	::EnableWindow(::GetDlgItem(_hSelf, IDD_BACKUPDIR_RESTORESESSION_STATIC2), isSnapshot);
	::EnableWindow(::GetDlgItem(_hSelf, IDD_BACKUPDIR_RESTORESESSION_PATHLABEL_STATIC), isSnapshot);
	::EnableWindow(::GetDlgItem(_hSelf, IDD_BACKUPDIR_RESTORESESSION_PATH_EDIT), isSnapshot);

	bool noBackup = BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_RADIO_BKNONE, BM_GETCHECK, 0, 0);
	bool isEnableGlobableCheck = false;
	bool isEnableLocalCheck = false;

	if (!noBackup)
	{
		isEnableGlobableCheck = true;
		isEnableLocalCheck = BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_BACKUPDIR_CHECK, BM_GETCHECK, 0, 0);
	}
	::EnableWindow(::GetDlgItem(_hSelf, IDC_BACKUPDIR_USERCUSTOMDIR_GRPSTATIC), isEnableGlobableCheck);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_BACKUPDIR_CHECK), isEnableGlobableCheck);

	::EnableWindow(::GetDlgItem(_hSelf, IDD_BACKUPDIR_STATIC), isEnableLocalCheck);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_BACKUPDIR_EDIT), isEnableLocalCheck);
	::EnableWindow(::GetDlgItem(_hSelf, IDD_BACKUPDIR_BROWSE_BUTTON), isEnableLocalCheck);
}


INT_PTR CALLBACK AutoCompletionDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI & nppGUI = const_cast<NppGUI &>(nppParam.getNppGUI());
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			::SetDlgItemInt(_hSelf, IDD_AUTOC_STATIC_N,  static_cast<UINT>(nppGUI._autocFromLen), FALSE);
			_nbCharVal.init(_hInst, _hSelf);
			_nbCharVal.create(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_N), IDD_AUTOC_STATIC_N);

			bool isEnableAutoC = nppGUI._autocStatus != nppGUI.autoc_none;

			::SendDlgItemMessage(_hSelf, IDD_AUTOC_ENABLECHECK, BM_SETCHECK, isEnableAutoC?BST_CHECKED:BST_UNCHECKED, 0);
			
			int selectedID = IDD_AUTOC_BOTHRADIO;
			if (nppGUI._autocStatus == nppGUI.autoc_func)
				selectedID = IDD_AUTOC_FUNCRADIO;
			else if (nppGUI._autocStatus == nppGUI.autoc_word)
				selectedID = IDD_AUTOC_WORDRADIO;
			else if (nppGUI._autocStatus == nppGUI.autoc_both)
				selectedID = IDD_AUTOC_BOTHRADIO;
			
			::SendDlgItemMessage(_hSelf, selectedID, BM_SETCHECK, BST_CHECKED, 0);

			if (nppGUI._autocStatus == nppGUI.autoc_word || nppGUI._autocStatus == nppGUI.autoc_both)
				::SendDlgItemMessage(_hSelf, IDD_AUTOC_IGNORENUMBERS, BM_SETCHECK, nppGUI._autocIgnoreNumbers ? BST_CHECKED : BST_UNCHECKED, 0);

			if (!isEnableAutoC)
			{
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_FUNCRADIO), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_WORDRADIO), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_BOTHRADIO), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_IGNORENUMBERS), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_FROM), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_N), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_CHAR), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_NOTE), FALSE);
			}
			::SendDlgItemMessage(_hSelf, IDC_CHECK_MAINTAININDENT, BM_SETCHECK, nppGUI._maitainIndent, 0);

			::SendDlgItemMessage(_hSelf, IDD_FUNC_CHECK, BM_SETCHECK, nppGUI._funcParams ? BST_CHECKED : BST_UNCHECKED, 0);

			::SendDlgItemMessage(_hSelf, IDD_AUTOCPARENTHESES_CHECK, BM_SETCHECK, nppGUI._matchedPairConf._doParentheses?BST_CHECKED:BST_UNCHECKED, 0);
			if (nppGUI._matchedPairConf._doParentheses)
				::SendDlgItemMessage(_hSelf, IDD_AUTOCPARENTHESES_CHECK, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(TEXT(" (  )")));

			::SendDlgItemMessage(_hSelf, IDD_AUTOCBRACKET_CHECK, BM_SETCHECK, nppGUI._matchedPairConf._doBrackets?BST_CHECKED:BST_UNCHECKED, 0);
			if (nppGUI._matchedPairConf._doBrackets)
				::SendDlgItemMessage(_hSelf, IDD_AUTOCBRACKET_CHECK, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(TEXT(" [  ]")));

			::SendDlgItemMessage(_hSelf, IDD_AUTOCCURLYBRACKET_CHECK, BM_SETCHECK, nppGUI._matchedPairConf._doCurlyBrackets?BST_CHECKED:BST_UNCHECKED, 0);
			if (nppGUI._matchedPairConf._doCurlyBrackets)
				::SendDlgItemMessage(_hSelf, IDD_AUTOCCURLYBRACKET_CHECK, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(TEXT(" {  }")));
			
			::SendDlgItemMessage(_hSelf, IDD_AUTOC_QUOTESCHECK, BM_SETCHECK, nppGUI._matchedPairConf._doQuotes?BST_CHECKED:BST_UNCHECKED, 0);
			if (nppGUI._matchedPairConf._doQuotes)
				::SendDlgItemMessage(_hSelf, IDD_AUTOC_QUOTESCHECK, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(TEXT(" '  '")));
			
			::SendDlgItemMessage(_hSelf, IDD_AUTOC_DOUBLEQUOTESCHECK, BM_SETCHECK, nppGUI._matchedPairConf._doDoubleQuotes?BST_CHECKED:BST_UNCHECKED, 0);
			if (nppGUI._matchedPairConf._doDoubleQuotes)
				::SendDlgItemMessage(_hSelf, IDD_AUTOC_DOUBLEQUOTESCHECK, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(TEXT(" \"  \"")));
			
			::SendDlgItemMessage(_hSelf, IDD_AUTOCTAG_CHECK, BM_SETCHECK, nppGUI._matchedPairConf._doHtmlXmlTag?BST_CHECKED:BST_UNCHECKED, 0);

			::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIROPEN_EDIT1, EM_LIMITTEXT, 1, 0);
			::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIRCLOSE_EDIT1, EM_LIMITTEXT, 1, 0);
			::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIROPEN_EDIT2, EM_LIMITTEXT, 1, 0);
			::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIRCLOSE_EDIT2, EM_LIMITTEXT, 1, 0);
			::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIROPEN_EDIT3, EM_LIMITTEXT, 1, 0);
			::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIRCLOSE_EDIT3, EM_LIMITTEXT, 1, 0);

			size_t nbMatchedPair = nppGUI._matchedPairConf._matchedPairsInit.size();
			if (nbMatchedPair > 3)
				nbMatchedPair = 3;
			for (size_t i = 0; i < nbMatchedPair; ++i)
			{
				TCHAR openChar[2];
				openChar[0] = nppGUI._matchedPairConf._matchedPairsInit[i].first;
				openChar[1] = '\0';
				TCHAR closeChar[2];
				closeChar[0] = nppGUI._matchedPairConf._matchedPairsInit[i].second;
				closeChar[1] = '\0';

				if (i == 0)
				{
					::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIROPEN_EDIT1, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(openChar));
					::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIRCLOSE_EDIT1, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(closeChar));
				}
				else if (i == 1)
				{
					::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIROPEN_EDIT2, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(openChar));
					::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIRCLOSE_EDIT2, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(closeChar));
				}
				if (i == 2)
				{
					::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIROPEN_EDIT3, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(openChar));
					::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIRCLOSE_EDIT3, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(closeChar));
				}
			}

			return TRUE;
		}
		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				switch (LOWORD(wParam))
				{
					case IDC_MACHEDPAIROPEN_EDIT1 :
					case IDC_MACHEDPAIRCLOSE_EDIT1:
					case IDC_MACHEDPAIROPEN_EDIT2 :
					case IDC_MACHEDPAIRCLOSE_EDIT2:
					case IDC_MACHEDPAIROPEN_EDIT3 :
					case IDC_MACHEDPAIRCLOSE_EDIT3:
					{
						nppGUI._matchedPairConf._matchedPairs.clear();

						TCHAR opener[2] = {'\0', '\0'};
						TCHAR closer[2] = {'\0', '\0'};

						::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIROPEN_EDIT1, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(opener));
						::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIRCLOSE_EDIT1, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(closer));
						if (opener[0] < 0x80 && opener[0] != '\0' && closer[0] < 0x80 && closer[0] != '\0')
							nppGUI._matchedPairConf._matchedPairs.push_back(pair<char, char>(static_cast<char>(opener[0]), static_cast<char>(closer[0])));

						::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIROPEN_EDIT2, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(opener));
						::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIRCLOSE_EDIT2, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(closer));
						if (opener[0] < 0x80 && opener[0] != '\0' && closer[0] < 0x80 && closer[0] != '\0')
							nppGUI._matchedPairConf._matchedPairs.push_back(pair<char, char>(static_cast<char>(opener[0]), static_cast<char>(closer[0])));

						::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIROPEN_EDIT3, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(opener));
						::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIRCLOSE_EDIT3, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(closer));
						if (opener[0] < 0x80 && opener[0] != '\0' && closer[0] < 0x80 && closer[0] != '\0')
							nppGUI._matchedPairConf._matchedPairs.push_back(pair<char, char>(static_cast<char>(opener[0]), static_cast<char>(closer[0])));
						 
						return TRUE;
					}
				}
			}

			switch (wParam)
			{
				case IDC_CHECK_MAINTAININDENT:
				{
					nppGUI._maitainIndent = isCheckedOrNot(IDC_CHECK_MAINTAININDENT);
					return TRUE;
				}

				case IDD_AUTOC_ENABLECHECK :
				{
					bool isEnableAutoC = BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDD_AUTOC_ENABLECHECK, BM_GETCHECK, 0, 0);

					if (isEnableAutoC)
					{
						::SendDlgItemMessage(_hSelf, IDD_AUTOC_BOTHRADIO, BM_SETCHECK, BST_CHECKED, 0);
						nppGUI._autocStatus = nppGUI.autoc_both;

						::SendDlgItemMessage(_hSelf, IDD_AUTOC_IGNORENUMBERS, BM_SETCHECK, BST_UNCHECKED, 0);
						nppGUI._autocIgnoreNumbers = false;
					}
					else 
					{
						::SendDlgItemMessage(_hSelf, IDD_AUTOC_FUNCRADIO, BM_SETCHECK, BST_UNCHECKED, 0);
						::SendDlgItemMessage(_hSelf, IDD_AUTOC_WORDRADIO, BM_SETCHECK, BST_UNCHECKED, 0);
						::SendDlgItemMessage(_hSelf, IDD_AUTOC_BOTHRADIO, BM_SETCHECK, BST_UNCHECKED, 0);
						nppGUI._autocStatus = nppGUI.autoc_none;

						::SendDlgItemMessage(_hSelf, IDD_AUTOC_IGNORENUMBERS, BM_SETCHECK, BST_UNCHECKED, 0);
						nppGUI._autocIgnoreNumbers = false;
					}
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_FUNCRADIO), isEnableAutoC);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_WORDRADIO), isEnableAutoC);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_BOTHRADIO), isEnableAutoC);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_IGNORENUMBERS), isEnableAutoC);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_FROM), isEnableAutoC);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_N), isEnableAutoC);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_CHAR), isEnableAutoC);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_NOTE), isEnableAutoC);
					return TRUE;
				}

				case IDD_AUTOC_FUNCRADIO :
				{
					nppGUI._autocStatus = nppGUI.autoc_func;

					::SendDlgItemMessage(_hSelf, IDD_AUTOC_IGNORENUMBERS, BM_SETCHECK, BST_UNCHECKED, 0);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_IGNORENUMBERS), FALSE);
					nppGUI._autocIgnoreNumbers = false;

					return TRUE;
				}

				case IDD_AUTOC_WORDRADIO :
				{
					nppGUI._autocStatus = nppGUI.autoc_word;
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_IGNORENUMBERS), TRUE);
					return TRUE;
				}

				case IDD_AUTOC_BOTHRADIO :
				{
					nppGUI._autocStatus = nppGUI.autoc_both;
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_IGNORENUMBERS), TRUE);
					return TRUE;
				}

				case IDD_AUTOC_IGNORENUMBERS:
				{
					nppGUI._autocIgnoreNumbers = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDD_AUTOC_IGNORENUMBERS, BM_GETCHECK, 0, 0));
					return TRUE;
				}

				case IDD_FUNC_CHECK :
				{
					nppGUI._funcParams = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDD_FUNC_CHECK, BM_GETCHECK, 0, 0));
					return TRUE;
				}
				
				case IDD_AUTOC_STATIC_N :
				{
					const int NB_MIN_CHAR = 1;
					const int NB_MAX_CHAR = 9;

					NativeLangSpeaker *pNativeSpeaker = nppParam.getNativeLangSpeaker();
					generic_string strNbChar = pNativeSpeaker->getLocalizedStrFromID("autocomplete-nb-char", TEXT("Nb char : "));

					ValueDlg valDlg;
					valDlg.init(NULL, _hSelf, static_cast<int32_t>(nppGUI._autocFromLen), strNbChar.c_str());
					valDlg.setNBNumber(1);

					POINT p;
					::GetCursorPos(&p);

					int size = valDlg.doDialog(p);
					if (size != -1)
					{
						if (size > NB_MAX_CHAR)
							size = NB_MAX_CHAR;
						else if (size < NB_MIN_CHAR)
							size = NB_MIN_CHAR;
						
						nppGUI._autocFromLen = size;
						::SetDlgItemInt(_hSelf, IDD_AUTOC_STATIC_N, static_cast<int32_t>(nppGUI._autocFromLen), FALSE);
					}
					return TRUE;
				}

				case IDD_AUTOCPARENTHESES_CHECK :
				case IDD_AUTOCBRACKET_CHECK :
				case IDD_AUTOCCURLYBRACKET_CHECK :
				case IDD_AUTOC_DOUBLEQUOTESCHECK :
				case IDD_AUTOC_QUOTESCHECK :
				{
					bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, static_cast<int32_t>(wParam), BM_GETCHECK, 0, 0));
					const TCHAR *label;
					if (wParam == IDD_AUTOCPARENTHESES_CHECK)
					{
						nppGUI._matchedPairConf._doParentheses = isChecked;
						label = isChecked?TEXT(" (  )"):TEXT(" (");
					}
					else if (wParam == IDD_AUTOCBRACKET_CHECK)
					{
						nppGUI._matchedPairConf._doBrackets = isChecked;
						label = isChecked?TEXT(" [  ]"):TEXT(" [");
					}
					else if (wParam == IDD_AUTOCCURLYBRACKET_CHECK)
					{
						nppGUI._matchedPairConf._doCurlyBrackets = isChecked;
						label = isChecked?TEXT(" {  }"):TEXT(" {");
					}
					else if (wParam == IDD_AUTOC_DOUBLEQUOTESCHECK)
					{
						nppGUI._matchedPairConf._doDoubleQuotes = isChecked;
						label = isChecked?TEXT(" \"  \""):TEXT(" \"");
					}
					else // if (wParam == IDD_AUTOC_QUOTESCHECK)
					{
						nppGUI._matchedPairConf._doQuotes = isChecked;
						label = isChecked?TEXT(" '  '"):TEXT(" '");
					}
					::SendDlgItemMessage(_hSelf, static_cast<int32_t>(wParam), WM_SETTEXT, 0, reinterpret_cast<LPARAM>(label));
					return TRUE;
				}

				case IDD_AUTOCTAG_CHECK :
				{
					nppGUI._matchedPairConf._doHtmlXmlTag = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDD_AUTOCTAG_CHECK, BM_GETCHECK, 0, 0));
					return TRUE;
				}
				default :
					return FALSE;
			}
			
		}
	}
	return FALSE;
}


INT_PTR CALLBACK MultiInstDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	NppGUI & nppGUI = const_cast<NppGUI &>((NppParameters::getInstance()).getNppGUI());
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			MultiInstSetting multiInstSetting = nppGUI._multiInstSetting;

			::SendDlgItemMessage(_hSelf, IDC_SESSIONININST_RADIO, BM_SETCHECK, multiInstSetting == multiInstOnSession?BST_CHECKED:BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_MULTIINST_RADIO, BM_SETCHECK, multiInstSetting == multiInst?BST_CHECKED:BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_MONOINST_RADIO, BM_SETCHECK, multiInstSetting == monoInst?BST_CHECKED:BST_UNCHECKED, 0);
		}
		break;

		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDC_SESSIONININST_RADIO :
				{
					nppGUI._multiInstSetting = multiInstOnSession;
				}
				break;

				case IDC_MULTIINST_RADIO :
				{
					nppGUI._multiInstSetting = multiInst;
				}
				break;

				case IDC_MONOINST_RADIO :
				{
					nppGUI._multiInstSetting = monoInst;
				}
				break;
				default :
					return FALSE;
			}
		}
		break;
	}

	return FALSE;
}

void DelimiterSettingsDlg::detectSpace(const char *text2Check, int & nbSp, int & nbTab) const
{
	nbSp = nbTab = 0;
	for (size_t i = 0; i < strlen(text2Check); ++i)
	{
		if (text2Check[i] == ' ')
			++nbSp;
		else if (text2Check[i] == '\t')
			++nbTab;
	}
}


generic_string DelimiterSettingsDlg::getWarningText(size_t nbSp, size_t nbTab) const
{
	NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

	generic_string msg;
	if (nbSp && nbTab)
	{
		generic_string nbSpStr = std::to_wstring(nbSp);
		generic_string nbTabStr = std::to_wstring(nbTab);
		generic_string warnBegin = pNativeSpeaker->getLocalizedStrFromID("word-chars-list-warning-begin", TEXT(""));
		generic_string space = pNativeSpeaker->getLocalizedStrFromID("word-chars-list-space-warning", TEXT(""));
		generic_string tab = pNativeSpeaker->getLocalizedStrFromID("word-chars-list-tab-warning", TEXT(""));
		generic_string warnEnd = pNativeSpeaker->getLocalizedStrFromID("word-chars-list-warning-end", TEXT(""));

		// half translation is not allowed
		if (not warnBegin.empty() && not space.empty() && not tab.empty() && not warnEnd.empty())
		{
			space = stringReplace(space, TEXT("$INT_REPLACE$"), nbSpStr);
			tab = stringReplace(tab, TEXT("$INT_REPLACE$"), nbTabStr);
			msg = warnBegin;
			msg += space;
			msg += TEXT(" && ");
			msg += tab;
			msg += warnEnd;
		}
		else
		{
			msg = TEXT("Be aware: ");
			msg += nbSpStr;
			msg += TEXT(" space(s) && ");
			msg += std::to_wstring(nbTab);
			msg += TEXT(" TAB(s) in your character list.");
		}
	}
	else if (nbSp && not nbTab)
	{
		generic_string nbSpStr = std::to_wstring(nbSp);
		generic_string warnBegin = pNativeSpeaker->getLocalizedStrFromID("word-chars-list-warning-begin", TEXT(""));
		generic_string space = pNativeSpeaker->getLocalizedStrFromID("word-chars-list-space-warning", TEXT(""));
		generic_string warnEnd = pNativeSpeaker->getLocalizedStrFromID("word-chars-list-warning-end", TEXT(""));

		// half translation is not allowed
		if (not warnBegin.empty() && not space.empty() && not warnEnd.empty())
		{
			space = stringReplace(space, TEXT("$INT_REPLACE$"), nbSpStr);
			msg = warnBegin;
			msg += space;
			msg += warnEnd;
		}
		else
		{
			msg = TEXT("Be aware: ");
			msg += std::to_wstring(nbSp);
			msg += TEXT(" space(s) in your character list.");
		}
	}
	else if (not nbSp && nbTab)
	{
		generic_string nbTabStr = std::to_wstring(nbTab);
		generic_string warnBegin = pNativeSpeaker->getLocalizedStrFromID("word-chars-list-warning-begin", TEXT(""));
		generic_string tab = pNativeSpeaker->getLocalizedStrFromID("word-chars-list-tab-warning", TEXT(""));
		generic_string warnEnd = pNativeSpeaker->getLocalizedStrFromID("word-chars-list-warning-end", TEXT(""));

		// half translation is not allowed
		if (not warnBegin.empty() && not tab.empty() && not warnEnd.empty())
		{
			tab = stringReplace(tab, TEXT("$INT_REPLACE$"), nbTabStr);
			msg = warnBegin;
			msg += tab;
			msg += warnEnd;
		}
		else
		{
			msg = TEXT("Be aware: ");
			msg += std::to_wstring(nbTab);
			msg += TEXT(" TAB(s) in your character list.");
		}
	}
	else //(not nbSp && not nbTab)
	{
		// do nothing
	}

	return msg;
}

void DelimiterSettingsDlg::setWarningIfNeed() const
{
	generic_string msg;
	NppGUI & nppGUI = const_cast<NppGUI &>((NppParameters::getInstance()).getNppGUI());
	if (not nppGUI._isWordCharDefault)
	{
		int nbSp = 0;
		int nbTab = 0;
		detectSpace(nppGUI._customWordChars.c_str(), nbSp, nbTab);
		msg = getWarningText(nbSp, nbTab);
	}
	::SetDlgItemText(_hSelf, IDD_STATIC_WORDCHAR_WARNING, msg.c_str());
}

INT_PTR CALLBACK DelimiterSettingsDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	NppGUI & nppGUI = const_cast<NppGUI &>((NppParameters::getInstance()).getNppGUI());
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			//
			// Delimiter
			//
			TCHAR opener[2];
			opener[0] = nppGUI._leftmostDelimiter;
			opener[1] = '\0';
			TCHAR closer[2];
			closer[0] = nppGUI._rightmostDelimiter;
			closer[1] = '\0';
			bool onSeveralLines = nppGUI._delimiterSelectionOnEntireDocument;
			
			::SendDlgItemMessage(_hSelf, IDC_EDIT_OPENDELIMITER, EM_LIMITTEXT, 1, 0);
			::SendDlgItemMessage(_hSelf, IDC_EDIT_CLOSEDELIMITER, EM_LIMITTEXT, 1, 0);
			::SendDlgItemMessage(_hSelf, IDC_EDIT_OPENDELIMITER, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(opener));
			::SendDlgItemMessage(_hSelf, IDC_EDIT_CLOSEDELIMITER, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(closer));
			::SendDlgItemMessage(_hSelf, IDD_SEVERALLINEMODEON_CHECK, BM_SETCHECK, onSeveralLines?BST_CHECKED:BST_UNCHECKED, 0);

			POINT point = getTopPoint(::GetDlgItem(_hSelf, IDD_STATIC_BLABLA), false);
			_singleLineModePoint.x = point.x + 4;
			_singleLineModePoint.y = point.y - 4;

			point = getTopPoint(::GetDlgItem(_hSelf, IDD_STATIC_BLABLA2NDLINE), false);
			_multiLineModePoint.x = point.x + 4;
			_multiLineModePoint.y = point.y - 4;

			::GetClientRect(::GetDlgItem(_hSelf, IDC_EDIT_CLOSEDELIMITER), &_closerRect);
			_closerRect.right = _closerRect.right - _closerRect.left + 4;
			_closerRect.bottom = _closerRect.bottom - _closerRect.top + 4;

			::GetClientRect(::GetDlgItem(_hSelf, IDD_STATIC_CLOSEDELIMITER), &_closerLabelRect);
			_closerLabelRect.right = _closerLabelRect.right - _closerLabelRect.left + 4;
			_closerLabelRect.bottom = _closerLabelRect.bottom - _closerLabelRect.top + 4;


			::ShowWindow(::GetDlgItem(_hSelf, IDD_STATIC_BLABLA2NDLINE),onSeveralLines?SW_SHOW:SW_HIDE);

			POINT *p = onSeveralLines?&_multiLineModePoint:&_singleLineModePoint;
			::MoveWindow(::GetDlgItem(_hSelf, IDC_EDIT_CLOSEDELIMITER), p->x, p->y, _closerRect.right, _closerRect.bottom, TRUE);
			::MoveWindow(::GetDlgItem(_hSelf, IDD_STATIC_CLOSEDELIMITER), p->x + _closerRect.right + 4, p->y + 4, _closerLabelRect.right, _closerLabelRect.bottom, TRUE);

			//
			// Word Char List
			//
			
			::SetDlgItemTextA(_hSelf, IDC_WORDCHAR_CUSTOM_EDIT, nppGUI._customWordChars.c_str());
			::SendDlgItemMessage(_hSelf, IDC_RADIO_WORDCHAR_DEFAULT, BM_SETCHECK, nppGUI._isWordCharDefault ? BST_CHECKED : BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_RADIO_WORDCHAR_CUSTOM, BM_SETCHECK, not nppGUI._isWordCharDefault ? BST_CHECKED : BST_UNCHECKED, 0);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_WORDCHAR_CUSTOM_EDIT), not nppGUI._isWordCharDefault);

			setWarningIfNeed();

			NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
			generic_string tip2show = pNativeSpeaker->getLocalizedStrFromID("word-chars-list-tip", TEXT("This allows you to include additional character into current word characters while double clicking for selection or searching with \"Match whole word only\" option checked."));

			_tip = CreateToolTip(IDD_WORDCHAR_QUESTION_BUTTON, _hSelf, _hInst, const_cast<PTSTR>(tip2show.c_str()));
			if (_tip)
			{
				SendMessage(_tip, TTM_ACTIVATE, TRUE, 0);
				SendMessage(_tip, TTM_SETMAXTIPWIDTH, 0, 200);

				// Make tip stay 30 seconds
				SendMessage(_tip, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELPARAM((30000), (0)));
			}
			return TRUE;
		}

		case WM_CTLCOLORSTATIC:
		{
			HDC hdcStatic = (HDC) wParam;
			HWND hwnd = reinterpret_cast<HWND>(lParam);
			if (hwnd == ::GetDlgItem(_hSelf, IDD_STATIC_BLABLA) || hwnd == ::GetDlgItem(_hSelf, IDD_STATIC_BLABLA2NDLINE))
			{
				COLORREF bgColor = getCtrlBgColor(_hSelf);
				SetTextColor(hdcStatic, RGB(0, 0, 0));
				BYTE r = GetRValue(bgColor) - 30;
				BYTE g = MyGetGValue(bgColor) - 30;
				BYTE b = GetBValue(bgColor) - 30;
				SetBkColor(hdcStatic, RGB(r, g, b));
				return TRUE;
			}
			return FALSE;
		}

		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				switch (LOWORD(wParam))
				{
					case  IDC_EDIT_OPENDELIMITER:
					{
						TCHAR opener[2];
						::SendDlgItemMessage(_hSelf, IDC_EDIT_OPENDELIMITER, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(opener));
						nppGUI._leftmostDelimiter =  static_cast<char>(opener[0]);
						return TRUE;
					}
					case  IDC_EDIT_CLOSEDELIMITER:
					{
						TCHAR closer[2];
						::SendDlgItemMessage(_hSelf, IDC_EDIT_CLOSEDELIMITER, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(closer));
						nppGUI._rightmostDelimiter =  static_cast<char>(closer[0]);
						return TRUE;
					}

					case  IDC_WORDCHAR_CUSTOM_EDIT:
					{
						char customText[MAX_PATH];
						::GetDlgItemTextA(_hSelf, IDC_WORDCHAR_CUSTOM_EDIT, customText, MAX_PATH-1);
						nppGUI._customWordChars = customText;
						setWarningIfNeed();
						::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SETWORDCHARS, 0, 0);
						return TRUE;
					}

					default:
						return FALSE;
				}
			}

			switch (wParam)
			{

				case IDD_SEVERALLINEMODEON_CHECK :
				{
					bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDD_SEVERALLINEMODEON_CHECK, BM_GETCHECK, 0, 0));
					nppGUI._delimiterSelectionOnEntireDocument = isChecked;
					
					::ShowWindow(::GetDlgItem(_hSelf, IDD_STATIC_BLABLA2NDLINE),isChecked?SW_SHOW:SW_HIDE);

					POINT *p = isChecked?&_multiLineModePoint:&_singleLineModePoint;
					::MoveWindow(::GetDlgItem(_hSelf, IDC_EDIT_CLOSEDELIMITER), p->x, p->y, _closerRect.right, _closerRect.bottom, TRUE);
					::MoveWindow(::GetDlgItem(_hSelf, IDD_STATIC_CLOSEDELIMITER), p->x + _closerRect.right + 4, p->y + 4, _closerLabelRect.right, _closerLabelRect.bottom, TRUE);

					return TRUE;
				}

				case IDC_RADIO_WORDCHAR_DEFAULT:
				{
					::SendDlgItemMessage(_hSelf, IDC_RADIO_WORDCHAR_CUSTOM, BM_SETCHECK, BST_UNCHECKED, 0);
					nppGUI._isWordCharDefault = true;
					::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SETWORDCHARS, 0, 0);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_WORDCHAR_CUSTOM_EDIT), not nppGUI._isWordCharDefault);
					::SetDlgItemText(_hSelf, IDD_STATIC_WORDCHAR_WARNING, TEXT(""));
					return TRUE;
				}

				case IDC_RADIO_WORDCHAR_CUSTOM:
				{
					::SendDlgItemMessage(_hSelf, IDC_RADIO_WORDCHAR_DEFAULT, BM_SETCHECK, BST_UNCHECKED, 0);
					nppGUI._isWordCharDefault = false;
					::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SETWORDCHARS, 0, 0);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_WORDCHAR_CUSTOM_EDIT), not nppGUI._isWordCharDefault);
					setWarningIfNeed();
					return TRUE;
				}

				default :
					return FALSE;
			}
		}
		break;
	}

	return FALSE;
}

INT_PTR CALLBACK SettingsOnCloudDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	NppParameters& nppParams = NppParameters::getInstance();
	NppGUI & nppGUI = const_cast<NppGUI &>(nppParams.getNppGUI());

	if (HIWORD(wParam) == EN_CHANGE)
	{
		switch (LOWORD(wParam))
		{
			case  IDC_CLOUDPATH_EDIT:
			{
				TCHAR inputDir[MAX_PATH] = {'\0'};
				TCHAR inputDirExpanded[MAX_PATH] = {'\0'};
				::SendDlgItemMessage(_hSelf, IDC_CLOUDPATH_EDIT, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(inputDir));
				::ExpandEnvironmentStrings(inputDir, inputDirExpanded, MAX_PATH);
				NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
				if (::PathFileExists(inputDirExpanded))
				{
					nppGUI._cloudPath = inputDirExpanded;
					nppParams.setCloudChoice(inputDirExpanded);
					
					generic_string message;
					if (nppParams.isCloudPathChanged())
					{
						message = pNativeSpeaker->getLocalizedStrFromID("cloud-restart-warning", TEXT("Please restart Notepad++ to take effect."));
					}
					::SetDlgItemText(_hSelf, IDC_SETTINGSONCLOUD_WARNING_STATIC, message.c_str());
				}
				else
				{
					bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_WITHCLOUD_RADIO, BM_GETCHECK, 0, 0));
					if (isChecked)
					{
						generic_string message = pNativeSpeaker->getLocalizedStrFromID("cloud-invalid-warning", TEXT("Invalid path."));

						::SetDlgItemText(_hSelf, IDC_SETTINGSONCLOUD_WARNING_STATIC, message.c_str());
						nppParams.removeCloudChoice();
					}
				}
				return TRUE;
			}
		}
	}

	switch (message)
	{
		case WM_INITDIALOG:
		{
			generic_string message = TEXT("");

			bool withCloud = nppGUI._cloudPath != TEXT("");
			if (withCloud)
			{
				// detect validation of path
				if (!::PathFileExists(nppGUI._cloudPath.c_str()))
					message = TEXT("Invalid path");
			}
			
			::SetDlgItemText(_hSelf, IDC_SETTINGSONCLOUD_WARNING_STATIC, message.c_str());

			::SendDlgItemMessage(_hSelf, IDC_NOCLOUD_RADIO, BM_SETCHECK, !withCloud ? BST_CHECKED : BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_WITHCLOUD_RADIO, BM_SETCHECK, withCloud ? BST_CHECKED : BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_CLOUDPATH_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(nppGUI._cloudPath.c_str()));
			::EnableWindow(::GetDlgItem(_hSelf, IDC_CLOUDPATH_EDIT), withCloud);
			::EnableWindow(::GetDlgItem(_hSelf, IDD_CLOUDPATH_BROWSE_BUTTON), withCloud);
		}
		break;

		case WM_COMMAND:
		{
			NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
			switch (wParam)
			{
				case IDC_NOCLOUD_RADIO:
				{
					nppGUI._cloudPath = TEXT("");
					nppParams.removeCloudChoice();

					generic_string message;
					if (nppParams.isCloudPathChanged())
					{
						message = pNativeSpeaker->getLocalizedStrFromID("cloud-restart-warning", TEXT("Please restart Notepad++ to take effect."));
					}
					// else set empty string
					::SetDlgItemText(_hSelf, IDC_SETTINGSONCLOUD_WARNING_STATIC, message.c_str());

					::SendDlgItemMessage(_hSelf, IDC_CLOUDPATH_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(nppGUI._cloudPath.c_str()));
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CLOUDPATH_EDIT), false);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_CLOUDPATH_BROWSE_BUTTON), false);
				}
				break;

				case IDC_WITHCLOUD_RADIO:
				{
					generic_string message = pNativeSpeaker->getLocalizedStrFromID("cloud-invalid-warning", TEXT("Invalid path."));
					::SetDlgItemText(_hSelf, IDC_SETTINGSONCLOUD_WARNING_STATIC, message.c_str());

					::EnableWindow(::GetDlgItem(_hSelf, IDC_CLOUDPATH_EDIT), true);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_CLOUDPATH_BROWSE_BUTTON), true);
				}
				break;

				case IDD_CLOUDPATH_BROWSE_BUTTON:
				{
					generic_string message = pNativeSpeaker->getLocalizedStrFromID("cloud-select-folder", TEXT("Select a folder from/to where Notepad++ reads/writes its settings"));
					folderBrowser(_hSelf, message, IDC_CLOUDPATH_EDIT);
				}
				break;

				default:
					return FALSE;

			}
		}																						
	}
	return FALSE;
}

INT_PTR CALLBACK SearchEngineChoiceDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	NppParameters& nppParams = NppParameters::getInstance();
	NppGUI & nppGUI = const_cast<NppGUI &>(nppParams.getNppGUI());

	if (HIWORD(wParam) == EN_CHANGE)
	{
		switch (LOWORD(wParam))
		{
			case  IDC_SEARCHENGINE_EDIT:
			{
				TCHAR input[MAX_PATH] = { '\0' };
				::SendDlgItemMessage(_hSelf, IDC_SEARCHENGINE_EDIT, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(input));
				nppGUI._searchEngineCustom = input;
				return TRUE;
			}
		}
	}

	switch (message)
	{
		case WM_INITDIALOG:
		{
			if (nppGUI._searchEngineChoice == nppGUI.se_custom)
			{
				if (nppGUI._searchEngineCustom.empty())
				{
					nppGUI._searchEngineChoice = nppGUI.se_google;
				}
			}
			::SendDlgItemMessage(_hSelf, IDC_SEARCHENGINE_CUSTOM_RADIO, BM_SETCHECK, nppGUI._searchEngineChoice == nppGUI.se_custom ? BST_CHECKED : BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_SEARCHENGINE_DUCKDUCKGO_RADIO, BM_SETCHECK, nppGUI._searchEngineChoice == nppGUI.se_duckDuckGo ? BST_CHECKED : BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_SEARCHENGINE_GOOGLE_RADIO, BM_SETCHECK, nppGUI._searchEngineChoice == nppGUI.se_google ? BST_CHECKED : BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_SEARCHENGINE_BING_RADIO, BM_SETCHECK, nppGUI._searchEngineChoice == nppGUI.se_bing ? BST_CHECKED : BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_SEARCHENGINE_YAHOO_RADIO, BM_SETCHECK, nppGUI._searchEngineChoice == nppGUI.se_yahoo ? BST_CHECKED : BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_SEARCHENGINE_STACKOVERFLOW_RADIO, BM_SETCHECK, nppGUI._searchEngineChoice == nppGUI.se_stackoverflow ? BST_CHECKED : BST_UNCHECKED, 0);

			::SendDlgItemMessage(_hSelf, IDC_SEARCHENGINE_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(nppGUI._searchEngineCustom.c_str()));
			::EnableWindow(::GetDlgItem(_hSelf, IDC_SEARCHENGINE_EDIT), nppGUI._searchEngineChoice == nppGUI.se_custom);
		}
		break;

		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDC_SEARCHENGINE_DUCKDUCKGO_RADIO:
				{
					nppGUI._searchEngineChoice = nppGUI.se_duckDuckGo;
					::EnableWindow(::GetDlgItem(_hSelf, IDC_SEARCHENGINE_EDIT), false);
				}
				break;

				case IDC_SEARCHENGINE_GOOGLE_RADIO:
				{
					nppGUI._searchEngineChoice = nppGUI.se_google;
					::EnableWindow(::GetDlgItem(_hSelf, IDC_SEARCHENGINE_EDIT), false);
				}
				break;

				case IDC_SEARCHENGINE_BING_RADIO:
				{
					nppGUI._searchEngineChoice = nppGUI.se_bing;
					::EnableWindow(::GetDlgItem(_hSelf, IDC_SEARCHENGINE_EDIT), false);
				}
				break;

				case IDC_SEARCHENGINE_YAHOO_RADIO:
				{
					nppGUI._searchEngineChoice = nppGUI.se_yahoo;
					::EnableWindow(::GetDlgItem(_hSelf, IDC_SEARCHENGINE_EDIT), false);
				}
				break;

				case IDC_SEARCHENGINE_STACKOVERFLOW_RADIO:
				{
					nppGUI._searchEngineChoice = nppGUI.se_stackoverflow;
					::EnableWindow(::GetDlgItem(_hSelf, IDC_SEARCHENGINE_EDIT), false);
				}
				break;

				case IDC_SEARCHENGINE_CUSTOM_RADIO:
				{
					nppGUI._searchEngineChoice = nppGUI.se_custom;
					::EnableWindow(::GetDlgItem(_hSelf, IDC_SEARCHENGINE_EDIT), true);
				}
				break;

				default:
					return FALSE;
			}
		}
		break;
	}
	return FALSE;
}

