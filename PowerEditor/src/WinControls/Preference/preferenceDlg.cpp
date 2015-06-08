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

#include <shlwapi.h>
#include <Shlobj.h>
#include <uxtheme.h>
#include "preferenceDlg.h"
#include "lesDlgs.h"
#include "EncodingMapper.h"

using namespace std;

const int BLINKRATE_FASTEST = 50;
const int BLINKRATE_SLOWEST = 2500;
const int BLINKRATE_INTERVAL = 50;

const int BORDERWIDTH_SMALLEST = 0;
const int BORDERWIDTH_LARGEST = 30;
const int BORDERWIDTH_INTERVAL = 1;

// This int encoding array is built from "EncodingUnit encodings[]" (see EncodingMapper.cpp)
// And DefaultNewDocDlg will use "int encoding array" to get more info from "EncodingUnit encodings[]"
int encodings[] = {
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

INT_PTR CALLBACK PreferenceDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
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

			_tabSettings.init(_hInst, _hSelf);
			_tabSettings.create(IDD_PREFERENCE_TABSETTINGS_BOX, false, false);

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


			_wVector.push_back(DlgInfo(&_barsDlg, TEXT("General"), TEXT("Global")));
			_wVector.push_back(DlgInfo(&_marginsDlg, TEXT("Editing"), TEXT("Scintillas")));
			_wVector.push_back(DlgInfo(&_defaultNewDocDlg, TEXT("New Document"), TEXT("NewDoc")));
			_wVector.push_back(DlgInfo(&_defaultDirectoryDlg, TEXT("Default Directory"), TEXT("DefaultDir")));
			_wVector.push_back(DlgInfo(&_recentFilesHistoryDlg, TEXT("Recent Files History"), TEXT("RecentFilesHistory")));
			_wVector.push_back(DlgInfo(&_fileAssocDlg, TEXT("File Association"), TEXT("FileAssoc")));
			_wVector.push_back(DlgInfo(&_langMenuDlg, TEXT("Language Menu"), TEXT("LangMenu")));
			_wVector.push_back(DlgInfo(&_tabSettings, TEXT("Tab Settings"), TEXT("TabSettings")));
			_wVector.push_back(DlgInfo(&_printSettingsDlg, TEXT("Print"), TEXT("Print")));
			_wVector.push_back(DlgInfo(&_backupDlg, TEXT("Backup"), TEXT("Backup")));
			_wVector.push_back(DlgInfo(&_autoCompletionDlg, TEXT("Auto-Completion"), TEXT("AutoCompletion")));
			_wVector.push_back(DlgInfo(&_multiInstDlg, TEXT("Multi-Instance"), TEXT("MultiInstance")));
			_wVector.push_back(DlgInfo(&_delimiterSettingsDlg, TEXT("Delimiter"), TEXT("Delimiter")));
			_wVector.push_back(DlgInfo(&_settingsOnCloudDlg, TEXT("Cloud"), TEXT("Cloud")));
			_wVector.push_back(DlgInfo(&_settingsDlg, TEXT("MISC."), TEXT("MISC")));


			makeCategoryList();
			RECT rc;
			getClientRect(rc);

			rc.top += NppParameters::getInstance()->_dpiManager.scaleY(10);
			rc.bottom -= NppParameters::getInstance()->_dpiManager.scaleY(50);
			rc.left += NppParameters::getInstance()->_dpiManager.scaleX(150);
			
			_barsDlg.reSizeTo(rc);
			_marginsDlg.reSizeTo(rc);
			_settingsDlg.reSizeTo(rc);
			_defaultNewDocDlg.reSizeTo(rc);
			_defaultDirectoryDlg.reSizeTo(rc);
			_recentFilesHistoryDlg.reSizeTo(rc);
			_fileAssocDlg.reSizeTo(rc);
			_langMenuDlg.reSizeTo(rc);
			_tabSettings.reSizeTo(rc);
			_printSettingsDlg.reSizeTo(rc);
			_backupDlg.reSizeTo(rc);
			_autoCompletionDlg.reSizeTo(rc);
			_multiInstDlg.reSizeTo(rc);
			_delimiterSettingsDlg.reSizeTo(rc);
			_settingsOnCloudDlg.reSizeTo(rc);

			NppParameters *pNppParam = NppParameters::getInstance();
			ETDTProc enableDlgTheme = (ETDTProc)pNppParam->getEnableThemeDlgTexture();
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
					int i = ::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_GETCURSEL, 0, 0);
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
		::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_ADDSTRING, 0, (LPARAM)_wVector[i]._name.c_str());
	}
	setListSelection(0);
}

void PreferenceDlg::setListSelection(size_t currentSel) const
{
	// Stupid LB API doesn't allow LB_SETSEL to be used on single select listbox, so we do it in a hard way
	TCHAR selStr[256];
	::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_GETTEXT, currentSel, (LPARAM)selStr);
	::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_SELECTSTRING, currentSel, (LPARAM)selStr);
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

	::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_GETTEXT, i, (LPARAM)oldName);

	// Same name, no need to change, but operation is considered success
	if (lstrcmp(newName, oldName) == 0)
		return true;

	::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_DELETESTRING, i, 0);
	::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_INSERTSTRING, i, (LPARAM)newName);

	return true;
}

void PreferenceDlg::showDialogByIndex(int index)
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
	_tabSettings.destroy();
	_printSettingsDlg.destroy();
	_defaultNewDocDlg.destroy();
	_defaultDirectoryDlg.destroy();
	_recentFilesHistoryDlg.destroy();
	_backupDlg.destroy();
	_autoCompletionDlg.destroy();
	_multiInstDlg.destroy();
	_delimiterSettingsDlg.destroy();
}

INT_PTR CALLBACK BarsDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	
	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			const NppGUI & nppGUI = pNppParam->getNppGUI();
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
			
			::SendDlgItemMessage(_hSelf, IDC_CHECK_TAB_HIDE, BM_SETCHECK, tabBarStatus & TAB_HIDE, 0);
			::SendMessage(_hSelf, WM_COMMAND, IDC_CHECK_TAB_HIDE, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_SHOWSTATUSBAR, BM_SETCHECK, showStatus, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_HIDEMENUBAR, BM_SETCHECK, !showMenu, 0);

			bool showDocSwitcher = ::SendMessage(::GetParent(_hParent), NPPM_ISDOCSWITCHERSHOWN, 0, 0) == TRUE;
			::SendDlgItemMessage(_hSelf, IDC_CHECK_DOCSWITCH, BM_SETCHECK, showDocSwitcher, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_DOCSWITCH_NOEXTCOLUMN, BM_SETCHECK, nppGUI._fileSwitcherWithoutExtColumn, 0);

			LocalizationSwitcher & localizationSwitcher = pNppParam->getLocalizationSwitcher();

			for (size_t i = 0, len = localizationSwitcher.size(); i < len ; ++i)
			{
				pair<wstring, wstring> localizationInfo = localizationSwitcher.getElementFromIndex(i);
				::SendDlgItemMessage(_hSelf, IDC_COMBO_LOCALIZATION, CB_ADDSTRING, 0, (LPARAM)localizationInfo.first.c_str());
			}
			wstring lang = TEXT("English"); // Set default language as Englishs
			if (pNppParam->getNativeLangA()) // if nativeLangA is not NULL, then we can be sure the default language (English) is not used
			{
				string fn = localizationSwitcher.getFileName();
				wstring fnW(fn.begin(), fn.end());
				fnW.assign(fn.begin(), fn.end());
				lang = localizationSwitcher.getLangFromXmlFileName(fnW.c_str());
			}
			int index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_LOCALIZATION, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)lang.c_str());
			if (index != CB_ERR)
                ::SendDlgItemMessage(_hSelf, IDC_COMBO_LOCALIZATION, CB_SETCURSEL, index, 0);

			ETDTProc enableDlgTheme = (ETDTProc)pNppParam->getEnableThemeDlgTexture();
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

					::SendMessage(::GetParent(_hParent), NPPM_HIDETABBAR, 0, toBeHidden);
					return TRUE;
				}
				
				case  IDC_CHECK_TAB_VERTICAL:
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_DRAWTABBAR_VERTICAL, 0);
					return TRUE;

				case IDC_CHECK_TAB_MULTILINE :
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_DRAWTABBAR_MULTILINE, 0);
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
									LocalizationSwitcher & localizationSwitcher = pNppParam->getLocalizationSwitcher();
									int index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_LOCALIZATION, CB_GETCURSEL, 0, 0);
									wchar_t langName[MAX_PATH];
									::SendDlgItemMessage(_hSelf, IDC_COMBO_LOCALIZATION, CB_GETLBTEXT, index, (LPARAM)langName);
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


void MarginsDlg::initScintParam()
{
	NppParameters *pNppParam = NppParameters::getInstance();
	const ScintillaViewParams & svp = pNppParam->getSVP();
	
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

	::SendDlgItemMessage(_hSelf, IDC_CHECK_LINENUMBERMARGE, BM_SETCHECK, svp._lineNumberMarginShow, 0);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_BOOKMARKMARGE, BM_SETCHECK, svp._bookMarkMarginShow, 0);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_CURRENTLINEHILITE, BM_SETCHECK, svp._currentLineHilitingShow, 0);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_DISABLEADVANCEDSCROLL, BM_SETCHECK, svp._disableAdvancedScrolling, 0);
	
	bool isEnable = !(svp._edgeMode == EDGE_NONE);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_SHOWVERTICALEDGE, BM_SETCHECK, isEnable, 0);
	::SendDlgItemMessage(_hSelf, IDC_RADIO_LNMODE, BM_SETCHECK, (svp._edgeMode == EDGE_LINE), 0);
	::SendDlgItemMessage(_hSelf, IDC_RADIO_BGMODE, BM_SETCHECK, (svp._edgeMode == EDGE_BACKGROUND), 0);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_LNMODE), isEnable);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_BGMODE), isEnable);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_NBCOLONE_STATIC), isEnable);
	
	::SetDlgItemInt(_hSelf, IDC_COLONENUMBER_STATIC, svp._edgeNbColumn, FALSE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_COLONENUMBER_STATIC), isEnable);

}


INT_PTR CALLBACK MarginsDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			_verticalEdgeLineNbColVal.init(_hInst, _hSelf);
			_verticalEdgeLineNbColVal.create(::GetDlgItem(_hSelf, IDC_COLONENUMBER_STATIC), IDC_COLONENUMBER_STATIC);

			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, (LPARAM)TEXT("0"));
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, (LPARAM)TEXT("1"));
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, (LPARAM)TEXT("2"));
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, (LPARAM)TEXT("3"));
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, (LPARAM)TEXT("Block"));


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
			const ScintillaViewParams & svp = pNppParam->getSVP();
			::SendMessage(::GetDlgItem(_hSelf, IDC_BORDERWIDTH_SLIDER),TBM_SETPOS, TRUE, svp._borderWidth);
			::SetDlgItemInt(_hSelf, IDC_BORDERWIDTHVAL_STATIC, svp._borderWidth, FALSE);
			
			initScintParam();
			
			ETDTProc enableDlgTheme = (ETDTProc)pNppParam->getEnableThemeDlgTexture();
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);
			return TRUE;
		}

		case WM_HSCROLL:
		{
			HWND hCaretBlikRateSlider = ::GetDlgItem(_hSelf, IDC_CARETBLINKRATE_SLIDER);
			HWND hBorderWidthSlider = ::GetDlgItem(_hSelf, IDC_BORDERWIDTH_SLIDER);
			if ((HWND)lParam == hCaretBlikRateSlider)
			{
				int blinkRate = (int)::SendMessage(hCaretBlikRateSlider, TBM_GETPOS, 0, 0);
				if (blinkRate == BLINKRATE_SLOWEST)
					blinkRate = 0;
				nppGUI._caretBlinkRate = blinkRate;

				::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SETCARETBLINKRATE, 0, 0);
			}
			else if ((HWND)lParam == hBorderWidthSlider)
			{
				int borderWidth = (int)::SendMessage(hBorderWidthSlider, TBM_GETPOS, 0, 0);
				ScintillaViewParams & svp = (ScintillaViewParams &)pNppParam->getSVP();
				svp._borderWidth = borderWidth;
				::SetDlgItemInt(_hSelf, IDC_BORDERWIDTHVAL_STATIC, borderWidth, FALSE);
				::SendMessage(::GetParent(_hParent), WM_SIZE, 0, 0);
			}
			return 0;	//return zero when handled
				
		}

		case WM_COMMAND : 
		{			
			ScintillaViewParams & svp = (ScintillaViewParams &)pNppParam->getSVP();
			int iView = 1;
			switch (wParam)
			{
				case IDC_CHECK_LINENUMBERMARGE:
					svp._lineNumberMarginShow = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_LINENUMBERMARGE, BM_GETCHECK, 0, 0));
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_LINENUMBER, iView);
					return TRUE;
				
				case IDC_CHECK_BOOKMARKMARGE:
					svp._bookMarkMarginShow = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_BOOKMARKMARGE, BM_GETCHECK, 0, 0));
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_SYMBOLMARGIN, iView);
					return TRUE;

				case IDC_CHECK_CURRENTLINEHILITE:
					svp._currentLineHilitingShow = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_CURRENTLINEHILITE, BM_GETCHECK, 0, 0));
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_CURLINE_HILITING, iView);
					return TRUE;

				case IDC_CHECK_DISABLEADVANCEDSCROLL:
					svp._disableAdvancedScrolling = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_DISABLEADVANCEDSCROLL, BM_GETCHECK, 0, 0));
					//::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_CURLINE_HILITING, iView);
					return TRUE;

                case IDC_CHECK_MULTISELECTION :
                    nppGUI._enableMultiSelection = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_MULTISELECTION, BM_GETCHECK, 0, 0));
                    ::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SETMULTISELCTION, 0, 0);
                    return TRUE;

				case IDC_RADIO_SIMPLE:
					svp._folderStyle = FOLDER_STYLE_SIMPLE;
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_FOLDERMAGIN_SIMPLE, iView);
					return TRUE;
				case IDC_RADIO_ARROW:
					svp._folderStyle = FOLDER_STYLE_ARROW;
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_FOLDERMAGIN_ARROW, iView);
					return TRUE;
				case IDC_RADIO_CIRCLE:
					svp._folderStyle = FOLDER_STYLE_CIRCLE;
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_FOLDERMAGIN_CIRCLE, iView);
					return TRUE;
				case IDC_RADIO_BOX:
					svp._folderStyle = FOLDER_STYLE_BOX;
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_FOLDERMAGIN_BOX, iView);
					return TRUE;
					
				case IDC_RADIO_FOLDMARGENONE:
					svp._folderStyle = FOLDER_STYLE_NONE;
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_FOLDERMAGIN, iView);
					return TRUE;
					
				case IDC_CHECK_SHOWVERTICALEDGE:
				{
					int modeID = 0;
					bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_SHOWVERTICALEDGE, BM_GETCHECK, 0, 0));
					if (isChecked)
					{
						::SendDlgItemMessage(_hSelf, IDC_RADIO_LNMODE, BM_SETCHECK, TRUE, 0);
						svp._edgeMode = EDGE_LINE;
						modeID = IDM_VIEW_EDGELINE;
					}
					else
					{
						::SendDlgItemMessage(_hSelf, IDC_RADIO_LNMODE, BM_SETCHECK, FALSE, 0);
						::SendDlgItemMessage(_hSelf, IDC_RADIO_BGMODE, BM_SETCHECK, FALSE, 0);
						svp._edgeMode = EDGE_NONE;
						modeID = IDM_VIEW_EDGENONE;
					}
					::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_LNMODE), isChecked);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_BGMODE), isChecked);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_NBCOLONE_STATIC), isChecked);
					::ShowWindow(::GetDlgItem(_hSelf, IDC_COLONENUMBER_STATIC), isChecked);
	
					::SendMessage(_hParent, WM_COMMAND, modeID, iView);
					return TRUE;
				}
				case IDC_RADIO_LNMODE:
					svp._edgeMode = EDGE_LINE;
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_EDGELINE, iView);
					return TRUE;
					
				case IDC_RADIO_BGMODE:
					svp._edgeMode = EDGE_BACKGROUND;
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_EDGEBACKGROUND, iView);
					return TRUE;
				
				case IDC_COLONENUMBER_STATIC:
				{
					ScintillaViewParams & svp = (ScintillaViewParams &)pNppParam->getSVP();

					ValueDlg nbColumnEdgeDlg;
					nbColumnEdgeDlg.init(NULL, _hSelf, svp._edgeNbColumn, TEXT("Nb of column:"));
					nbColumnEdgeDlg.setNBNumber(3);

					POINT p;
					::GetCursorPos(&p);

					int val = nbColumnEdgeDlg.doDialog(p);
					if (val != -1)
					{
						svp._edgeNbColumn = val;
						::SetDlgItemInt(_hSelf, IDC_COLONENUMBER_STATIC, svp._edgeNbColumn, FALSE);

						// Execute modified value
						::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SETTING_EDGE_SIZE, 0, 0);
					}
					return TRUE;
				}

				case IDC_RADIO_LWDEF:
					svp._lineWrapMethod = LINEWRAP_DEFAULT;
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_LWDEF, iView);
					return TRUE;

				case IDC_RADIO_LWALIGN:
					svp._lineWrapMethod = LINEWRAP_ALIGNED;
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_LWALIGN, iView);
					return TRUE;

				case IDC_RADIO_LWINDENT:
					svp._lineWrapMethod = LINEWRAP_INDENT;
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_LWINDENT, iView);
					return TRUE;

				default :
					switch (HIWORD(wParam))
					{
						case CBN_SELCHANGE : // == case LBN_SELCHANGE :
						{
							switch (LOWORD(wParam))
							{
								case IDC_WIDTH_COMBO:
								{
									nppGUI._caretWidth = ::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_GETCURSEL, 0, 0);
									::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SETCARETWIDTH, 0, 0);
									return TRUE;			
								}
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

INT_PTR CALLBACK SettingsDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			if (nppGUI._fileAutoDetection == cdEnabled)
			{
				::SendDlgItemMessage(_hSelf, IDC_CHECK_FILEAUTODETECTION, BM_SETCHECK, BST_CHECKED, 0);
			}
			else if (nppGUI._fileAutoDetection == cdAutoUpdate)
			{
				::SendDlgItemMessage(_hSelf, IDC_CHECK_FILEAUTODETECTION, BM_SETCHECK, BST_CHECKED, 0);
				::SendDlgItemMessage(_hSelf, IDC_CHECK_UPDATESILENTLY, BM_SETCHECK, BST_CHECKED, 0);
			}
			else if (nppGUI._fileAutoDetection == cdGo2end)
			{
				::SendDlgItemMessage(_hSelf, IDC_CHECK_FILEAUTODETECTION, BM_SETCHECK, BST_CHECKED, 0);
				::SendDlgItemMessage(_hSelf, IDC_CHECK_UPDATEGOTOEOF, BM_SETCHECK, BST_CHECKED, 0);
			}
			else if (nppGUI._fileAutoDetection == cdAutoUpdateGo2end)
			{
				::SendDlgItemMessage(_hSelf, IDC_CHECK_FILEAUTODETECTION, BM_SETCHECK, BST_CHECKED, 0);
				::SendDlgItemMessage(_hSelf, IDC_CHECK_UPDATESILENTLY, BM_SETCHECK, BST_CHECKED, 0);
				::SendDlgItemMessage(_hSelf, IDC_CHECK_UPDATEGOTOEOF, BM_SETCHECK, BST_CHECKED, 0);
			}
			else //cdDisabled
			{
				::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_UPDATESILENTLY), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_UPDATEGOTOEOF), FALSE);
			}

			::SendDlgItemMessage(_hSelf, IDC_CHECK_MIN2SYSTRAY, BM_SETCHECK, nppGUI._isMinimizedToTray, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_DETECTENCODING, BM_SETCHECK, nppGUI._detectEncoding, 0);
            ::SendDlgItemMessage(_hSelf, IDC_CHECK_AUTOUPDATE, BM_SETCHECK, nppGUI._autoUpdateOpt._doAutoUpdate, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_BACKSLASHISESCAPECHARACTERFORSQL, BM_SETCHECK, nppGUI._backSlashIsEscapeCharacterForSql, 0);

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

			::SendDlgItemMessage(_hSelf, IDC_EDIT_SESSIONFILEEXT, WM_SETTEXT, 0, (LPARAM)nppGUI._definedSessionExt.c_str());
			
			::SendDlgItemMessage(_hSelf, IDC_CHECK_ENABLEDOCSWITCHER, BM_SETCHECK, nppGUI._doTaskList, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_MAINTAININDENT, BM_SETCHECK, nppGUI._maitainIndent, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_STYLEMRU, BM_SETCHECK, nppGUI._styleMRU, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_ENABLSMARTHILITE, BM_SETCHECK, nppGUI._enableSmartHilite, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_SMARTHILITECASESENSITIVE, BM_SETCHECK, nppGUI._smartHiliteCaseSensitive, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_ENABLTAGSMATCHHILITE, BM_SETCHECK, nppGUI._enableTagsMatchHilite, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_ENABLTAGATTRHILITE, BM_SETCHECK, nppGUI._enableTagAttrsHilite, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_HIGHLITENONEHTMLZONE, BM_SETCHECK, nppGUI._enableHiliteNonHTMLZone, 0);

			::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_ENABLTAGATTRHILITE), nppGUI._enableTagsMatchHilite);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_HIGHLITENONEHTMLZONE), nppGUI._enableTagsMatchHilite);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_SMARTHILITECASESENSITIVE), nppGUI._enableSmartHilite);

			::SendDlgItemMessage(_hSelf, IDC_CHECK_SHORTTITLE, BM_SETCHECK, nppGUI._shortTitlebar, 0);

			ETDTProc enableDlgTheme = (ETDTProc)pNppParam->getEnableThemeDlgTexture();
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
						::SendDlgItemMessage(_hSelf, IDC_EDIT_SESSIONFILEEXT, WM_GETTEXT, MAX_PATH, (LPARAM)sessionExt);
						nppGUI._definedSessionExt = sessionExt;
						return TRUE;
					}
				}
			}
			
			switch (wParam)
			{
				case IDC_CHECK_FILEAUTODETECTION:
				{
					bool isChecked = isCheckedOrNot(IDC_CHECK_FILEAUTODETECTION);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_UPDATESILENTLY), isChecked);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_UPDATEGOTOEOF), isChecked);

					bool isSilent = isCheckedOrNot(IDC_CHECK_UPDATESILENTLY);
					bool isGo2End = isCheckedOrNot(IDC_CHECK_UPDATEGOTOEOF);

					ChangeDetect cd;

					if (!isChecked)
						cd = cdDisabled;
					else if (!isSilent && !isGo2End)
						cd = cdEnabled;
					else if (!isSilent && isGo2End)
						cd = cdGo2end;
					else if (isSilent && !isGo2End)
						cd = cdAutoUpdate;
					else //(isSilent && isGo2End)
						cd = cdAutoUpdateGo2end;

					nppGUI._fileAutoDetection = cd;
				}
				return TRUE;

				case IDC_CHECK_UPDATESILENTLY:
				case IDC_CHECK_UPDATEGOTOEOF:
				{
					bool isSilent = isCheckedOrNot(IDC_CHECK_UPDATESILENTLY);
					bool isGo2End = isCheckedOrNot(IDC_CHECK_UPDATEGOTOEOF);

					ChangeDetect cd;

					if (!isSilent && !isGo2End)
						cd = cdEnabled;
					else if (!isSilent && isGo2End)
						cd = cdGo2end;
					else if (isSilent && !isGo2End)
						cd = cdAutoUpdate;
					else //(isSilent && isGo2End)
						cd = cdAutoUpdateGo2end;

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
				}
				return TRUE;

				case IDC_CHECK_CLICKABLELINK_NOUNDERLINE:
				{
					bool isChecked = isCheckedOrNot(IDC_CHECK_CLICKABLELINK_NOUNDERLINE);
					nppGUI._styleURL = isChecked?1:2;
				}
				return TRUE;

				case IDC_CHECK_AUTOUPDATE:
                    nppGUI._autoUpdateOpt._doAutoUpdate = isCheckedOrNot(wParam);
					return TRUE;

				case IDC_CHECK_MIN2SYSTRAY:
					nppGUI._isMinimizedToTray = isCheckedOrNot(wParam);
					return TRUE;

				case IDC_CHECK_DETECTENCODING:
					nppGUI._detectEncoding = isCheckedOrNot(wParam);
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

				case IDC_CHECK_MAINTAININDENT :
				{
					nppGUI._maitainIndent = !nppGUI._maitainIndent;
					return TRUE;
				}

				case IDC_CHECK_ENABLSMARTHILITE :
				{
					nppGUI._enableSmartHilite = !nppGUI._enableSmartHilite;
					if (!nppGUI._enableSmartHilite)
					{
						HWND grandParent = ::GetParent(_hParent);
						::SendMessage(grandParent, NPPM_INTERNAL_CLEARINDICATOR, 0, 0);
					}
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_SMARTHILITECASESENSITIVE), nppGUI._enableSmartHilite);
					return TRUE;
				}

				case IDC_CHECK_SMARTHILITECASESENSITIVE:
				{
					nppGUI._smartHiliteCaseSensitive = !nppGUI._smartHiliteCaseSensitive;
					if (!nppGUI._smartHiliteCaseSensitive)
					{
						HWND grandParent = ::GetParent(_hParent);
						::SendMessage(grandParent, NPPM_INTERNAL_CLEARINDICATOR, 0, 0);
					}
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

				case IDC_CHECK_STYLEMRU :
				{
					nppGUI._styleMRU = !nppGUI._styleMRU;
					return TRUE;
				}

				case IDC_CHECK_SHORTTITLE:
				{
					nppGUI._shortTitlebar = !nppGUI._shortTitlebar;
					HWND grandParent = ::GetParent(_hParent);
					::SendMessage(grandParent, NPPM_INTERNAL_UPDATETITLEBAR, 0, 0);
					return TRUE;
				}

				case IDC_CHECK_BACKSLASHISESCAPECHARACTERFORSQL :
				{
					nppGUI._backSlashIsEscapeCharacterForSql = isCheckedOrNot(IDC_CHECK_BACKSLASHISESCAPECHARACTERFORSQL);
					return TRUE;
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

INT_PTR CALLBACK DefaultNewDocDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )pNppParam->getNppGUI();
	NewDocDefaultSettings & ndds = (NewDocDefaultSettings &)nppGUI.getNewDocDefaultSettings();

	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			int ID2Check = 0;

			switch (ndds._format)
			{
				case  MAC_FORMAT :
					ID2Check = IDC_RADIO_F_MAC;
					break;
				case UNIX_FORMAT :
					ID2Check = IDC_RADIO_F_UNIX;
					break;
				
				default : //WIN_FORMAT
					ID2Check = IDC_RADIO_F_WIN;
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
			EncodingMapper *em = EncodingMapper::getInstance();
			for (size_t i = 0, encodingArraySize = sizeof(encodings)/sizeof(int) ; i < encodingArraySize ; ++i)
			{
				int cmdID = em->getIndexFromEncoding(encodings[i]);
				if (cmdID != -1)
				{
					cmdID += IDM_FORMAT_ENCODE;
					getNameStrFromCmd(cmdID, str);
					int index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_OTHERCP, CB_ADDSTRING, 0, (LPARAM)str.c_str());
					if (ndds._codepage == encodings[i])
						selIndex = index;
					::SendDlgItemMessage(_hSelf, IDC_COMBO_OTHERCP, CB_SETITEMDATA, index, (LPARAM)encodings[i]);
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
			
			int index = 0;
			for (int i = L_TEXT ; i < pNppParam->L_END ; ++i)
			{
				generic_string str;
				if ((LangType)i != L_USER)
				{
					int cmdID = pNppParam->langTypeToCommandID((LangType)i);
					if ((cmdID != -1))
					{
						getNameStrFromCmd(cmdID, str);
						if (str.length() > 0)
						{
							_langList.push_back(LangID_Name((LangType)i, str));
							::SendDlgItemMessage(_hSelf, IDC_COMBO_DEFAULTLANG, CB_ADDSTRING, 0, (LPARAM)str.c_str());
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
			ETDTProc enableDlgTheme = (ETDTProc)pNppParam->getEnableThemeDlgTexture();
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);
		}

		case WM_COMMAND : 
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
					::EnableWindow(::GetDlgItem(_hSelf, IDC_COMBO_OTHERCP), true);
					int index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_OTHERCP, CB_GETCURSEL, 0, 0);
					int cp = ::SendDlgItemMessage(_hSelf, IDC_COMBO_OTHERCP, CB_GETITEMDATA, index, 0);
					ndds._codepage = cp;
					return TRUE;
				}

				case IDC_RADIO_F_MAC:
					ndds._format = MAC_FORMAT;
					return TRUE;
				case IDC_RADIO_F_UNIX:
					ndds._format = UNIX_FORMAT;
					return TRUE;
				case IDC_RADIO_F_WIN:
					ndds._format = WIN_FORMAT;
					return TRUE;

				default:
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						if (LOWORD(wParam) == IDC_COMBO_DEFAULTLANG)
						{
							int index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_DEFAULTLANG, CB_GETCURSEL, 0, 0);
							ndds._lang = _langList[index]._id;
							return TRUE;
						}
						else if (LOWORD(wParam) == IDC_COMBO_OTHERCP)
						{
							int index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_OTHERCP, CB_GETCURSEL, 0, 0);
							int cp = ::SendDlgItemMessage(_hSelf, IDC_COMBO_OTHERCP, CB_GETITEMDATA, index, 0);
							ndds._codepage = cp;
							return TRUE;
						}
					}
					return FALSE;
			}
	}
 	return FALSE;
}

INT_PTR CALLBACK DefaultDirectoryDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )pNppParam->getNppGUI();

	switch (Message) 
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
			::SendDlgItemMessage(_hSelf, IDC_OPENSAVEDIR_ALWAYSON_EDIT, WM_SETTEXT, 0, (LPARAM)nppGUI._defaultDir);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_OPENSAVEDIR_ALWAYSON_EDIT), shouldActivated);
			::EnableWindow(::GetDlgItem(_hSelf, IDD_OPENSAVEDIR_ALWAYSON_BROWSE_BUTTON), shouldActivated);

			//
			// To avoid the white control background to be displayed in dialog
			//
			ETDTProc enableDlgTheme = (ETDTProc)pNppParam->getEnableThemeDlgTexture();
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);
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
						::SendDlgItemMessage(_hSelf, IDC_OPENSAVEDIR_ALWAYSON_EDIT, WM_GETTEXT, MAX_PATH, (LPARAM)inputDir);
						lstrcpy(nppGUI._defaultDir, inputDir);
						::ExpandEnvironmentStrings(nppGUI._defaultDir, nppGUI._defaultDirExp, 500);
						pNppParam->setWorkingDir(nppGUI._defaultDirExp);
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
					folderBrowser(_hSelf, IDC_OPENSAVEDIR_ALWAYSON_EDIT);
					return TRUE;

				default:
					return FALSE;
			}
		}
	}
	return FALSE;
}

INT_PTR CALLBACK RecentFilesHistoryDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )pNppParam->getNppGUI();

	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			// Max number recent file setting
			::SetDlgItemInt(_hSelf, IDC_MAXNBFILEVAL_STATIC, pNppParam->getNbMaxRecentFile(), FALSE);
			_nbHistoryVal.init(_hInst, _hSelf);
			_nbHistoryVal.create(::GetDlgItem(_hSelf, IDC_MAXNBFILEVAL_STATIC), IDC_MAXNBFILEVAL_STATIC);

			// Check on launch time settings
			::SendDlgItemMessage(_hSelf, IDC_CHECK_DONTCHECKHISTORY, BM_SETCHECK, !nppGUI._checkHistoryFiles, 0);

			// Disply in submenu setting
			::SendDlgItemMessage(_hSelf, IDC_CHECK_INSUBMENU, BM_SETCHECK, pNppParam->putRecentFileInSubMenu(), 0);

			// Recent File menu entry length setting
			int customLength = pNppParam->getRecentFileCustomLength();
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
			_customLenVal.create(::GetDlgItem(_hSelf, IDC_CUSTOMIZELENGTHVAL_STATIC), NULL);

			//
			// To avoid the white control background to be displayed in dialog
			//
			ETDTProc enableDlgTheme = (ETDTProc)pNppParam->getEnableThemeDlgTexture();
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
					ValueDlg nbFileMaxDlg;
					nbFileMaxDlg.init(NULL, _hSelf, pNppParam->getNbMaxRecentFile(), TEXT("Max File: "));
					
					POINT p;
					::GetCursorPos(&p);

					int nbMaxFile = nbFileMaxDlg.doDialog(p);
					if (nbMaxFile != -1)
					{
						if (nbMaxFile > NB_MAX_LRF_FILE)
							nbMaxFile = NB_MAX_LRF_FILE;
						
						pNppParam->setNbMaxRecentFile(nbMaxFile);
						::SetDlgItemInt(_hSelf, IDC_MAXNBFILEVAL_STATIC, nbMaxFile, FALSE);

						// Validate modified value
						::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SETTING_HISTORY_SIZE, 0, 0);
					}
					return TRUE;
				}

				case IDC_CHECK_INSUBMENU:
					pNppParam->setPutRecentFileInSubMenu(isCheckedOrNot(IDC_CHECK_INSUBMENU));
					::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_RECENTFILELIST_SWITCH, 0, 0);
					return TRUE;

				case IDC_RADIO_ONLYFILENAME:
					setCustomLen(0);
					pNppParam->setRecentFileCustomLength(0);
					::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_RECENTFILELIST_UPDATE, 0, 0);
					return TRUE;
				case IDC_RADIO_FULLFILENAMEPATH:
					setCustomLen(0);
					pNppParam->setRecentFileCustomLength(-1);
					::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_RECENTFILELIST_UPDATE, 0, 0);
					return TRUE;
				case IDC_RADIO_CUSTOMIZELENTH:
				{
					int len = pNppParam->getRecentFileCustomLength();
					if (len <= 0)
					{
						setCustomLen(100);
						pNppParam->setRecentFileCustomLength(100);
						::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_RECENTFILELIST_UPDATE, 0, 0);
					}
					return TRUE;
				}

				case IDC_CUSTOMIZELENGTHVAL_STATIC:
				{
					ValueDlg customLengthDlg;
					customLengthDlg.init(NULL, _hSelf, pNppParam->getRecentFileCustomLength(), TEXT("Length: "));
					customLengthDlg.setNBNumber(3);

					POINT p;
					::GetCursorPos(&p);

					int size = customLengthDlg.doDialog(p);
					if (size != -1)
					{
						::SetDlgItemInt(_hSelf, IDC_CUSTOMIZELENGTHVAL_STATIC, size, FALSE);
						pNppParam->setRecentFileCustomLength(size);
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

INT_PTR CALLBACK LangMenuDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )pNppParam->getNppGUI();

	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			int nbLang = pNppParam->getNbLang();
	        for (int i = 0 ; i < nbLang ; ++i)
            {
				::SendDlgItemMessage(_hSelf, IDC_LIST_TABSETTNG, LB_ADDSTRING, 0, (LPARAM)pNppParam->getLangFromIndex(i)->_langName.c_str());
            }

			for (int i = L_TEXT ; i < pNppParam->L_END ; ++i)
			{
				generic_string str;
				if ((LangType)i != L_USER)
				{
					int cmdID = pNppParam->langTypeToCommandID((LangType)i);
					if ((cmdID != -1))
					{
						getNameStrFromCmd(cmdID, str);
						if (str.length() > 0)
						{
							_langList.push_back(LangMenuItem((LangType)i, cmdID, str));
							::SendDlgItemMessage(_hSelf, IDC_LIST_ENABLEDLANG, LB_ADDSTRING, 0, (LPARAM)str.c_str());
						}
					}
				}
			}

			for (size_t i = 0, len = nppGUI._excludedLangList.size(); i < len ; ++i)
			{
				::SendDlgItemMessage(_hSelf, IDC_LIST_DISABLEDLANG, LB_ADDSTRING, 0, (LPARAM)nppGUI._excludedLangList[i]._langName.c_str());
			}

			::SendDlgItemMessage(_hSelf, IDC_CHECK_LANGMENUCOMPACT, BM_SETCHECK, nppGUI._isLangMenuCompact?BST_CHECKED:BST_UNCHECKED, 0);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_BUTTON_REMOVE), FALSE);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_BUTTON_RESTORE), FALSE);

			ETDTProc enableDlgTheme = (ETDTProc)pNppParam->getEnableThemeDlgTexture();
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);

			return TRUE;
		}
		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == LBN_SELCHANGE)
            {
				if (LOWORD(wParam) == IDC_LIST_DISABLEDLANG || LOWORD(wParam) == IDC_LIST_ENABLEDLANG)
				{
					HWND hEnableList = ::GetDlgItem(_hSelf, IDC_LIST_ENABLEDLANG);
					HWND hDisableList = ::GetDlgItem(_hSelf, IDC_LIST_DISABLEDLANG);
					if (HIWORD(wParam) == LBN_DBLCLK)
					{
						if (HWND(lParam) == hEnableList)
							::SendMessage(_hSelf, WM_COMMAND, IDC_BUTTON_REMOVE, 0);
						else if (HWND(lParam) == hDisableList)
							::SendMessage(_hSelf, WM_COMMAND, IDC_BUTTON_RESTORE, 0);
						return TRUE;
					}
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

					int i = ::SendDlgItemMessage(_hSelf, LOWORD(wParam), LB_GETCURSEL, 0, 0);
					if (i != LB_ERR)
					{
						::EnableWindow(::GetDlgItem(_hSelf, idButton2Enable), TRUE);
						int idListbox2Disable = (LOWORD(wParam)== IDC_LIST_ENABLEDLANG)?IDC_LIST_DISABLEDLANG:IDC_LIST_ENABLEDLANG;
						::SendDlgItemMessage(_hSelf, idListbox2Disable, LB_SETCURSEL, (WPARAM)-1, 0);
						::EnableWindow(::GetDlgItem(_hSelf, idButton2Disable), FALSE);
					}
					return TRUE;

				}
            }

			switch (wParam)
            {
				case IDC_CHECK_LANGMENUCOMPACT : 
				{
					nppGUI._isLangMenuCompact = (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDC_CHECK_LANGMENUCOMPACT), BM_GETCHECK, 0, 0));
					::MessageBox(_hSelf, 
						nppGUI._isLangMenuCompact?TEXT("This option will be enabled on the next launch."):TEXT("This option will be disabled on the next launch."), 
						TEXT("Compact Language Menu"), MB_OK);
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

					TCHAR s[32];
					::SendDlgItemMessage(_hSelf, list2Remove, LB_GETTEXT, iRemove, (LPARAM)s);

					LangMenuItem lmi = pSrcLst->at(iRemove);
					vector<LangMenuItem>::iterator lang2Remove = pSrcLst->begin() + iRemove;
					pSrcLst->erase(lang2Remove);

					int iAdd = ::SendDlgItemMessage(_hSelf, list2Add, LB_ADDSTRING, 0, (LPARAM)s);
					::SendDlgItemMessage(_hSelf, list2Remove, LB_DELETESTRING, iRemove, 0);
					pDestLst->push_back(lmi);

					::SendDlgItemMessage(_hSelf, list2Add, LB_SETCURSEL, iAdd, 0);
					::SendDlgItemMessage(_hSelf, list2Remove, LB_SETCURSEL, (WPARAM)-1, 0);
					::EnableWindow(::GetDlgItem(_hSelf, idButton2Enable), TRUE);
					::EnableWindow(::GetDlgItem(_hSelf, idButton2Disable), FALSE);

					if ((lmi._langType >= L_EXTERNAL) && (lmi._langType < pNppParam->L_END))
					{
						bool found(false);
						for(size_t x = 0; x < pNppParam->getExternalLexerDoc()->size() && !found; ++x)
						{
							TiXmlNode *lexersRoot = pNppParam->getExternalLexerDoc()->at(x)->FirstChild(TEXT("NotepadPlus"))->FirstChildElement(TEXT("LexerStyles"));
							for (TiXmlNode *childNode = lexersRoot->FirstChildElement(TEXT("LexerType"));
								childNode ;
								childNode = childNode->NextSibling(TEXT("LexerType")))
							{
								TiXmlElement *element = childNode->ToElement();

								if (generic_string(element->Attribute(TEXT("name"))) == lmi._langName)
								{
									element->SetAttribute(TEXT("excluded"), (LOWORD(wParam)==IDC_BUTTON_REMOVE)?TEXT("yes"):TEXT("no"));
									pNppParam->getExternalLexerDoc()->at(x)->SaveFile();
									found = true;
									break;
								}
							}
						}
					}

					HWND grandParent = ::GetParent(_hParent);

					if (LOWORD(wParam)==IDC_BUTTON_REMOVE)
					{
						::DeleteMenu((HMENU)::SendMessage(grandParent, NPPM_INTERNAL_GETMENU, 0, 0), lmi._cmdID, MF_BYCOMMAND);
					}
					else
					{
						::InsertMenu(::GetSubMenu((HMENU)::SendMessage(grandParent, NPPM_INTERNAL_GETMENU, 0, 0), MENUINDEX_LANGUAGE), iAdd-1, MF_BYPOSITION, lmi._cmdID, lmi._langName.c_str());
					}
					::DrawMenuBar(grandParent);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

INT_PTR CALLBACK TabSettings::run_dlgProc(UINT Message, WPARAM wParam, LPARAM/* lParam*/)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )pNppParam->getNppGUI();

	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			::SetDlgItemInt(_hSelf, IDC_TABSIZEVAL_STATIC, nppGUI._tabSize, FALSE);

            _tabSizeVal.init(_hInst, _hSelf);
			_tabSizeVal.create(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_STATIC), IDC_TABSIZEVAL_STATIC);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_REPLACEBYSPACE, BM_SETCHECK, nppGUI._tabReplacedBySpace, 0);

			int nbLang = pNppParam->getNbLang();
            ::SendDlgItemMessage(_hSelf, IDC_LIST_TABSETTNG, LB_ADDSTRING, 0, (LPARAM)TEXT("[Default]"));
	        for (int i = 0 ; i < nbLang ; ++i)
            {
				::SendDlgItemMessage(_hSelf, IDC_LIST_TABSETTNG, LB_ADDSTRING, 0, (LPARAM)pNppParam->getLangFromIndex(i)->_langName.c_str());
            }
            const int index2Begin = 0;
	        ::SendDlgItemMessage(_hSelf, IDC_LIST_TABSETTNG, LB_SETCURSEL, 0, index2Begin);
	        ::ShowWindow(::GetDlgItem(_hSelf, IDC_GR_TABVALUE_STATIC), SW_HIDE);
            ::ShowWindow(::GetDlgItem(_hSelf, IDC_CHECK_DEFAULTTABVALUE), SW_HIDE);
            ::EnableWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_DISABLE_STATIC), FALSE);            
            ::ShowWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_DISABLE_STATIC), SW_HIDE);

			ETDTProc enableDlgTheme = (ETDTProc)pNppParam->getEnableThemeDlgTexture();
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);

			return TRUE;
		}
		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == LBN_SELCHANGE)
            {
				if (LOWORD(wParam) == IDC_LIST_TABSETTNG)
				{
					int index = ::SendDlgItemMessage(_hSelf, IDC_LIST_TABSETTNG, LB_GETCURSEL, 0, 0);
					if (index == LB_ERR)
						return FALSE;
					::ShowWindow(::GetDlgItem(_hSelf, IDC_GR_TABVALUE_STATIC), index?SW_SHOW:SW_HIDE);
					::ShowWindow(::GetDlgItem(_hSelf, IDC_CHECK_DEFAULTTABVALUE), index?SW_SHOW:SW_HIDE);

					if (index)
					{
						Lang *lang = pNppParam->getLangFromIndex(index - 1);
						if (!lang) return FALSE;
							bool useDefaultTab = (lang->_tabSize == -1 || lang->_tabSize == 0);

						::SendMessage(::GetDlgItem(_hSelf, IDC_CHECK_DEFAULTTABVALUE), BM_SETCHECK, useDefaultTab, 0);
						::EnableWindow(::GetDlgItem(_hSelf, IDC_TABSIZE_STATIC), !useDefaultTab);

						int size = useDefaultTab?nppGUI._tabSize:lang->_tabSize;
						::SetDlgItemInt(_hSelf, IDC_TABSIZEVAL_STATIC, size, FALSE);
						::SetDlgItemInt(_hSelf, IDC_TABSIZEVAL_DISABLE_STATIC, size, FALSE);

						::EnableWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_STATIC), !useDefaultTab);
						::ShowWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_DISABLE_STATIC), useDefaultTab);
						::ShowWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_STATIC), !useDefaultTab);
						::SendMessage(::GetDlgItem(_hSelf, IDC_CHECK_REPLACEBYSPACE), BM_SETCHECK, useDefaultTab?nppGUI._tabReplacedBySpace:lang->_isTabReplacedBySpace, 0);
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

			switch (wParam)
            {
                case IDC_TABSIZEVAL_STATIC:
				{
            		ValueDlg tabSizeDlg;
			        tabSizeDlg.init(_hInst, _hParent, nppGUI._tabSize, TEXT("Tab Size : "));
			        POINT p;
			        ::GetCursorPos(&p);
			        int size = tabSizeDlg.doDialog(p);
			        if (size == -1) return FALSE;

					::SetDlgItemInt(_hSelf, IDC_TABSIZEVAL_STATIC, size, FALSE);
					::SetDlgItemInt(_hSelf, IDC_TABSIZEVAL_DISABLE_STATIC, size, FALSE);

                    int index = ::SendDlgItemMessage(_hSelf, IDC_LIST_TABSETTNG, LB_GETCURSEL, 0, 0);
                    if (index == LB_ERR) return FALSE;

                    if (index != 0)
                    {
                        Lang *lang = pNppParam->getLangFromIndex(index - 1);
                        if (!lang) return FALSE;
                        lang->_tabSize = size;

                        // write in langs.xml
                        pNppParam->insertTabInfo(lang->getLangName(), lang->getTabInfo());
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
                    int index = ::SendDlgItemMessage(_hSelf, IDC_LIST_TABSETTNG, LB_GETCURSEL, 0, 0);
                    if (index == LB_ERR) return FALSE;
                    if (index != 0)
                    {
                        Lang *lang = pNppParam->getLangFromIndex(index - 1);
                        if (!lang) return FALSE;
                        if (!lang->_tabSize || lang->_tabSize == -1)
                            lang->_tabSize = nppGUI._tabSize;
                        lang->_isTabReplacedBySpace = isTabReplacedBySpace;

                        // write in langs.xml
                        pNppParam->insertTabInfo(lang->getLangName(), lang->getTabInfo());
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
                    int index = ::SendDlgItemMessage(_hSelf, IDC_LIST_TABSETTNG, LB_GETCURSEL, 0, 0);
	                if (index == LB_ERR || index == 0) // index == 0 shouldn't happen
		                return FALSE;

                    Lang *lang = pNppParam->getLangFromIndex(index - 1);
                    if (!lang)
                        return FALSE;

                    //- Set tab setting in choosed language
                    lang->_tabSize = useDefaultTab?0:nppGUI._tabSize;
                    lang->_isTabReplacedBySpace = useDefaultTab?false:nppGUI._tabReplacedBySpace;

                    //- set visual effect
                    ::EnableWindow(::GetDlgItem(_hSelf, IDC_TABSIZE_STATIC), !useDefaultTab);
					::SetDlgItemInt(_hSelf, IDC_TABSIZEVAL_STATIC, useDefaultTab?nppGUI._tabSize:lang->_tabSize, FALSE);
                    ::EnableWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_STATIC), !useDefaultTab);
                    ::ShowWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_DISABLE_STATIC), useDefaultTab);
                    ::ShowWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_STATIC), !useDefaultTab);
                    ::SendMessage(::GetDlgItem(_hSelf, IDC_CHECK_REPLACEBYSPACE), BM_SETCHECK, useDefaultTab?nppGUI._tabReplacedBySpace:lang->_isTabReplacedBySpace, 0);
                    ::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_REPLACEBYSPACE), !useDefaultTab);

                    // write in langs.xml
                    if (useDefaultTab)
                        pNppParam->insertTabInfo(lang->getLangName(), -1);

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
		if(pos != generic_string::npos) str.erase(0, pos);
	}
	else str.erase(str.begin(), str.end());
};

INT_PTR CALLBACK PrintSettingsDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )pNppParam->getNppGUI();

	switch (Message) 
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
			for(size_t i = 6 ; i < 15 ; ++i)
			{
				wsprintf(intStr, TEXT("%d"), i);
				::SendDlgItemMessage(_hSelf, IDC_COMBO_HFONTSIZE, CB_ADDSTRING, 0, (LPARAM)intStr);
				::SendDlgItemMessage(_hSelf, IDC_COMBO_FFONTSIZE, CB_ADDSTRING, 0, (LPARAM)intStr);
			}
			const std::vector<generic_string> & fontlist = pNppParam->getFontList();
			for (size_t i = 0, len = fontlist.size() ; i < len ; ++i)
			{
				int j = ::SendDlgItemMessage(_hSelf, IDC_COMBO_HFONTNAME, CB_ADDSTRING, 0, (LPARAM)fontlist[i].c_str());
				::SendDlgItemMessage(_hSelf, IDC_COMBO_FFONTNAME, CB_ADDSTRING, 0, (LPARAM)fontlist[i].c_str());

				::SendDlgItemMessage(_hSelf, IDC_COMBO_HFONTNAME, CB_SETITEMDATA, j, (LPARAM)fontlist[i].c_str());
				::SendDlgItemMessage(_hSelf, IDC_COMBO_FFONTNAME, CB_SETITEMDATA, j, (LPARAM)fontlist[i].c_str());
			}

			int index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_HFONTNAME, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)nppGUI._printSettings._headerFontName.c_str());
			if (index == CB_ERR)
				index = 0;
			::SendDlgItemMessage(_hSelf, IDC_COMBO_HFONTNAME, CB_SETCURSEL, index, 0);
			index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_FFONTNAME, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)nppGUI._printSettings._footerFontName.c_str());
			if (index == CB_ERR)
				index = 0;
			::SendDlgItemMessage(_hSelf, IDC_COMBO_FFONTNAME, CB_SETCURSEL, index, 0);

			wsprintf(intStr, TEXT("%d"), nppGUI._printSettings._headerFontSize);
			::SendDlgItemMessage(_hSelf, IDC_COMBO_HFONTSIZE, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)intStr);
			wsprintf(intStr, TEXT("%d"), nppGUI._printSettings._footerFontSize);
			::SendDlgItemMessage(_hSelf, IDC_COMBO_FFONTSIZE, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)intStr);

			::SendDlgItemMessage(_hSelf, IDC_CHECK_HBOLD, BM_SETCHECK, nppGUI._printSettings._headerFontStyle & FONTSTYLE_BOLD?TRUE:FALSE, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_HITALIC, BM_SETCHECK, nppGUI._printSettings._headerFontStyle & FONTSTYLE_ITALIC?TRUE:FALSE, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_FBOLD, BM_SETCHECK, nppGUI._printSettings._footerFontStyle & FONTSTYLE_BOLD?TRUE:FALSE, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_FITALIC, BM_SETCHECK, nppGUI._printSettings._footerFontStyle & FONTSTYLE_ITALIC?TRUE:FALSE, 0);

			varList.push_back(strCouple(TEXT("Full file name path"), TEXT("$(FULL_CURRENT_PATH)")));
			varList.push_back(strCouple(TEXT("File name"), TEXT("$(FILE_NAME)")));
			varList.push_back(strCouple(TEXT("File directory"), TEXT("$(CURRENT_DIRECTORY)")));
			varList.push_back(strCouple(TEXT("Page"), TEXT("$(CURRENT_PRINTING_PAGE)")));
			varList.push_back(strCouple(TEXT("Short date format"), TEXT("$(SHORT_DATE)")));
			varList.push_back(strCouple(TEXT("Long date format"), TEXT("$(LONG_DATE)")));
			varList.push_back(strCouple(TEXT("Time"), TEXT("$(TIME)")));

			for (size_t i = 0, len = varList.size() ; i < len ; ++i)
			{
				int j = ::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_ADDSTRING, 0, (LPARAM)varList[i]._varDesc.c_str());
				::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_SETITEMDATA, j, (LPARAM)varList[i]._var.c_str());
			}
			::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_SETCURSEL, 0, 0);



			ETDTProc enableDlgTheme = (ETDTProc)pNppParam->getEnableThemeDlgTexture();
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
						::SendDlgItemMessage(_hSelf, IDC_VIEWPANEL_STATIC, WM_SETTEXT, 0, (LPARAM)str);
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
				::SendDlgItemMessage(_hSelf, IDC_VIEWPANEL_STATIC, WM_SETTEXT, 0, (LPARAM)str);
				::GetDlgItemText(_hSelf, groupStatic, str, stringSize);
				generic_string title = str;
				title += TEXT(" ");
				::GetDlgItemText(_hSelf, focusedEditStatic, str, stringSize);
				title += str;
				title += TEXT(" : ");
					
				::SendDlgItemMessage(_hSelf, IDC_WHICHPART_STATIC, WM_SETTEXT, 0, (LPARAM)title.c_str());
				return TRUE;
			}
			else if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				int iSel = ::SendDlgItemMessage(_hSelf, LOWORD(wParam), CB_GETCURSEL, 0, 0);

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
						TCHAR intStr[5];
						::SendDlgItemMessage(_hSelf, LOWORD(wParam), CB_GETLBTEXT, iSel, (LPARAM)intStr);

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
					if (!_focusedEditCtrl)
						return TRUE;

					int iSel = ::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_GETCURSEL, 0, 0);
					TCHAR *varStr = (TCHAR *)::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_GETITEMDATA, iSel, 0);

					::SendDlgItemMessage(_hSelf, _focusedEditCtrl, EM_GETSEL, (WPARAM)&_selStart, (LPARAM)&_selEnd);

					const int stringSize = 256;
					TCHAR str[stringSize];
					::SendDlgItemMessage(_hSelf, _focusedEditCtrl, WM_GETTEXT, stringSize, (LPARAM)str);

					generic_string str2Set(str);
					str2Set.replace(_selStart, _selEnd - _selStart, varStr);
					
					::SetDlgItemText(_hSelf, _focusedEditCtrl, str2Set.c_str());
				}
				break;
			}
			return TRUE;
		}
	}
	return FALSE;
}


INT_PTR CALLBACK BackupDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			::SendDlgItemMessage(_hSelf, IDC_CHECK_REMEMBERSESSION, BM_SETCHECK, nppGUI._rememberLastSession, 0);
			bool snapshotCheck = nppGUI._rememberLastSession && nppGUI.isSnapshotMode();
			::SendDlgItemMessage(_hSelf, IDC_BACKUPDIR_RESTORESESSION_CHECK, BM_SETCHECK, snapshotCheck?BST_CHECKED:BST_UNCHECKED, 0);
			int periodicBackupInSec = nppGUI._snapshotBackupTiming/1000;
			::SetDlgItemInt(_hSelf, IDC_BACKUPDIR_RESTORESESSION_EDIT, periodicBackupInSec, FALSE);
			generic_string backupFilePath = NppParameters::getInstance()->getUserPath();
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

			::SendDlgItemMessage(_hSelf, IDC_BACKUPDIR_EDIT, WM_SETTEXT, 0, (LPARAM)nppGUI._backupDir.c_str());

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
						::SendDlgItemMessage(_hSelf, IDC_BACKUPDIR_EDIT, WM_GETTEXT, MAX_PATH, (LPARAM)inputDir);
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
							::SetDlgItemInt(_hSelf, IDC_BACKUPDIR_RESTORESESSION_EDIT, nppGUI._snapshotBackupTiming/1000, FALSE);
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
					folderBrowser(_hSelf, IDC_BACKUPDIR_EDIT);
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


INT_PTR CALLBACK AutoCompletionDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			::SetDlgItemInt(_hSelf, IDD_AUTOC_STATIC_N,  nppGUI._autocFromLen, FALSE);
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

			if (!isEnableAutoC)
			{
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_FUNCRADIO), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_WORDRADIO), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_BOTHRADIO), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_FROM), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_N), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_CHAR), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_NOTE), FALSE);
			}
			::SendDlgItemMessage(_hSelf, IDD_FUNC_CHECK, BM_SETCHECK, nppGUI._funcParams ? BST_CHECKED : BST_UNCHECKED, 0);

			::SendDlgItemMessage(_hSelf, IDD_AUTOCPARENTHESES_CHECK, BM_SETCHECK, nppGUI._matchedPairConf._doParentheses?BST_CHECKED:BST_UNCHECKED, 0);
			if (nppGUI._matchedPairConf._doParentheses)
				::SendDlgItemMessage(_hSelf, IDD_AUTOCPARENTHESES_CHECK, WM_SETTEXT, 0, (LPARAM)TEXT(" (  )"));

			::SendDlgItemMessage(_hSelf, IDD_AUTOCBRACKET_CHECK, BM_SETCHECK, nppGUI._matchedPairConf._doBrackets?BST_CHECKED:BST_UNCHECKED, 0);
			if (nppGUI._matchedPairConf._doBrackets)
				::SendDlgItemMessage(_hSelf, IDD_AUTOCBRACKET_CHECK, WM_SETTEXT, 0, (LPARAM)TEXT(" [  ]"));

			::SendDlgItemMessage(_hSelf, IDD_AUTOCCURLYBRACKET_CHECK, BM_SETCHECK, nppGUI._matchedPairConf._doCurlyBrackets?BST_CHECKED:BST_UNCHECKED, 0);
			if (nppGUI._matchedPairConf._doCurlyBrackets)
				::SendDlgItemMessage(_hSelf, IDD_AUTOCCURLYBRACKET_CHECK, WM_SETTEXT, 0, (LPARAM)TEXT(" {  }"));
			
			::SendDlgItemMessage(_hSelf, IDD_AUTOC_QUOTESCHECK, BM_SETCHECK, nppGUI._matchedPairConf._doQuotes?BST_CHECKED:BST_UNCHECKED, 0);
			if (nppGUI._matchedPairConf._doQuotes)
				::SendDlgItemMessage(_hSelf, IDD_AUTOC_QUOTESCHECK, WM_SETTEXT, 0, (LPARAM)TEXT(" '  '"));
			
			::SendDlgItemMessage(_hSelf, IDD_AUTOC_DOUBLEQUOTESCHECK, BM_SETCHECK, nppGUI._matchedPairConf._doDoubleQuotes?BST_CHECKED:BST_UNCHECKED, 0);
			if (nppGUI._matchedPairConf._doDoubleQuotes)
				::SendDlgItemMessage(_hSelf, IDD_AUTOC_DOUBLEQUOTESCHECK, WM_SETTEXT, 0, (LPARAM)TEXT(" \"  \""));
			
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
					::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIROPEN_EDIT1, WM_SETTEXT, 0, (LPARAM)openChar);
					::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIRCLOSE_EDIT1, WM_SETTEXT, 0, (LPARAM)closeChar);
				}
				else if (i == 1)
				{
					::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIROPEN_EDIT2, WM_SETTEXT, 0, (LPARAM)openChar);
					::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIRCLOSE_EDIT2, WM_SETTEXT, 0, (LPARAM)closeChar);
				}
				if (i == 2)
				{
					::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIROPEN_EDIT3, WM_SETTEXT, 0, (LPARAM)openChar);
					::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIRCLOSE_EDIT3, WM_SETTEXT, 0, (LPARAM)closeChar);
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

						::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIROPEN_EDIT1, WM_GETTEXT, MAX_PATH, (LPARAM)opener);
						::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIRCLOSE_EDIT1, WM_GETTEXT, MAX_PATH, (LPARAM)closer);
						if (opener[0] < 0x80 && opener[0] != '\0' && closer[0] < 0x80 && closer[0] != '\0')
							nppGUI._matchedPairConf._matchedPairs.push_back(pair<char, char>(static_cast<char>(opener[0]), static_cast<char>(closer[0])));

						::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIROPEN_EDIT2, WM_GETTEXT, MAX_PATH, (LPARAM)opener);
						::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIRCLOSE_EDIT2, WM_GETTEXT, MAX_PATH, (LPARAM)closer);
						if (opener[0] < 0x80 && opener[0] != '\0' && closer[0] < 0x80 && closer[0] != '\0')
							nppGUI._matchedPairConf._matchedPairs.push_back(pair<char, char>(static_cast<char>(opener[0]), static_cast<char>(closer[0])));

						::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIROPEN_EDIT3, WM_GETTEXT, MAX_PATH, (LPARAM)opener);
						::SendDlgItemMessage(_hSelf, IDC_MACHEDPAIRCLOSE_EDIT3, WM_GETTEXT, MAX_PATH, (LPARAM)closer);
						if (opener[0] < 0x80 && opener[0] != '\0' && closer[0] < 0x80 && closer[0] != '\0')
							nppGUI._matchedPairConf._matchedPairs.push_back(pair<char, char>(static_cast<char>(opener[0]), static_cast<char>(closer[0])));
						 
						return TRUE;
					}
				}
			}

			switch (wParam)
			{

				case IDD_AUTOC_ENABLECHECK :
				{
					bool isEnableAutoC = BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDD_AUTOC_ENABLECHECK, BM_GETCHECK, 0, 0);

					if (isEnableAutoC)
					{
						::SendDlgItemMessage(_hSelf, IDD_AUTOC_BOTHRADIO, BM_SETCHECK, BST_CHECKED, 0);
						nppGUI._autocStatus = nppGUI.autoc_both;
					}
					else 
					{
						::SendDlgItemMessage(_hSelf, IDD_AUTOC_FUNCRADIO, BM_SETCHECK, BST_UNCHECKED, 0);
						::SendDlgItemMessage(_hSelf, IDD_AUTOC_WORDRADIO, BM_SETCHECK, BST_UNCHECKED, 0);
						::SendDlgItemMessage(_hSelf, IDD_AUTOC_BOTHRADIO, BM_SETCHECK, BST_UNCHECKED, 0);
						nppGUI._autocStatus = nppGUI.autoc_none;
					}
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_FUNCRADIO), isEnableAutoC);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_WORDRADIO), isEnableAutoC);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_BOTHRADIO), isEnableAutoC);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_FROM), isEnableAutoC);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_N), isEnableAutoC);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_CHAR), isEnableAutoC);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_NOTE), isEnableAutoC);
					return TRUE;
				}

				case IDD_AUTOC_FUNCRADIO :
				{
					nppGUI._autocStatus = nppGUI.autoc_func;
					return TRUE;
				}

				case IDD_AUTOC_WORDRADIO :
				{
					nppGUI._autocStatus = nppGUI.autoc_word;
					return TRUE;
				}

				case IDD_AUTOC_BOTHRADIO :
				{
					nppGUI._autocStatus = nppGUI.autoc_both;
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

					ValueDlg valDlg;
					//NppGUI & nppGUI = (NppGUI &)((NppParameters::getInstance())->getNppGUI());
					valDlg.init(NULL, _hSelf, nppGUI._autocFromLen, TEXT("Nb char : "));
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
						::SetDlgItemInt(_hSelf, IDD_AUTOC_STATIC_N,  nppGUI._autocFromLen, FALSE);
					}
					return TRUE;
				}

				case IDD_AUTOCPARENTHESES_CHECK :
				case IDD_AUTOCBRACKET_CHECK :
				case IDD_AUTOCCURLYBRACKET_CHECK :
				case IDD_AUTOC_DOUBLEQUOTESCHECK :
				case IDD_AUTOC_QUOTESCHECK :
				{
					bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0));
					TCHAR *label;
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
					::SendDlgItemMessage(_hSelf, wParam, WM_SETTEXT, 0, (LPARAM)label);
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


INT_PTR CALLBACK MultiInstDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM)
{
	NppGUI & nppGUI = (NppGUI &)((NppParameters::getInstance())->getNppGUI());
	switch (Message) 
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

INT_PTR CALLBACK DelimiterSettingsDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	NppGUI & nppGUI = (NppGUI &)((NppParameters::getInstance())->getNppGUI());
	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			TCHAR opener[2];
			opener[0] = nppGUI._leftmostDelimiter;
			opener[1] = '\0';
			TCHAR closer[2];
			closer[0] = nppGUI._rightmostDelimiter;
			closer[1] = '\0';
			bool onSeveralLines = nppGUI._delimiterSelectionOnEntireDocument;

			::SendDlgItemMessage(_hSelf, IDC_EDIT_OPENDELIMITER, EM_LIMITTEXT, 1, 0);
			::SendDlgItemMessage(_hSelf, IDC_EDIT_CLOSEDELIMITER, EM_LIMITTEXT, 1, 0);
			::SendDlgItemMessage(_hSelf, IDC_EDIT_OPENDELIMITER, WM_SETTEXT, 0, (LPARAM)opener);
			::SendDlgItemMessage(_hSelf, IDC_EDIT_CLOSEDELIMITER, WM_SETTEXT, 0, (LPARAM)closer);
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

			return TRUE;
		}

		case WM_CTLCOLORSTATIC:
		{
			HDC hdcStatic = (HDC) wParam;
			HWND hwnd = (HWND) lParam;
			if (hwnd == ::GetDlgItem(_hSelf, IDD_STATIC_BLABLA) || hwnd == ::GetDlgItem(_hSelf, IDD_STATIC_BLABLA2NDLINE))
			{
				COLORREF bgColor = getCtrlBgColor(_hSelf);
				SetTextColor(hdcStatic, RGB(0, 0, 0));
				SetBkColor(hdcStatic, RGB(GetRValue(bgColor) - 30, GetGValue(bgColor) - 30, GetBValue(bgColor) - 30));
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
						::SendDlgItemMessage(_hSelf, IDC_EDIT_OPENDELIMITER, WM_GETTEXT, MAX_PATH, (LPARAM)opener);
						nppGUI._leftmostDelimiter =  static_cast<char>(opener[0]);
						return TRUE;
					}
					case  IDC_EDIT_CLOSEDELIMITER:
					{
						TCHAR closer[2];
						::SendDlgItemMessage(_hSelf, IDC_EDIT_CLOSEDELIMITER, WM_GETTEXT, MAX_PATH, (LPARAM)closer);
						nppGUI._rightmostDelimiter =  static_cast<char>(closer[0]);
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

				default :
					return FALSE;
			}
		}
		break;
	}

	return FALSE;
}

INT_PTR CALLBACK SettingsOnCloudDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM)
{
	NppGUI & nppGUI = (NppGUI &)((NppParameters::getInstance())->getNppGUI());
	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			CloudChoice cloudChoice = nppGUI._cloudChoice;
			_initialCloudChoice = nppGUI._cloudChoice;
/*
			COLORREF bgColor = getCtrlBgColor(_hSelf);
			SetTextColor(hdcStatic, RGB(255, 0, 0));
			SetBkColor(hdcStatic, RGB(GetRValue(bgColor) - 30, GetGValue(bgColor) - 30, GetBValue(bgColor) - 30));
*/
			::SendDlgItemMessage(_hSelf, IDC_NOCLOUD_RADIO, BM_SETCHECK, cloudChoice == noCloud?BST_CHECKED:BST_UNCHECKED, 0);

			::SendDlgItemMessage(_hSelf, IDC_DROPBOX_RADIO, BM_SETCHECK, cloudChoice == dropbox?BST_CHECKED:BST_UNCHECKED, 0);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_DROPBOX_RADIO), (nppGUI._availableClouds & DROPBOX_AVAILABLE) != 0);

			::SendDlgItemMessage(_hSelf, IDC_ONEDRIVE_RADIO, BM_SETCHECK, cloudChoice == oneDrive?BST_CHECKED:BST_UNCHECKED, 0);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_ONEDRIVE_RADIO), (nppGUI._availableClouds & ONEDRIVE_AVAILABLE) != 0);

			::SendDlgItemMessage(_hSelf, IDC_GOOGLEDRIVE_RADIO, BM_SETCHECK, cloudChoice == googleDrive?BST_CHECKED:BST_UNCHECKED, 0);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_GOOGLEDRIVE_RADIO), (nppGUI._availableClouds & GOOGLEDRIVE_AVAILABLE) != 0);

		}
		break;

		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDC_NOCLOUD_RADIO :
				{
					nppGUI._cloudChoice = noCloud;
					removeCloudChoice();
					
					generic_string message = _initialCloudChoice != nppGUI._cloudChoice?TEXT("Please restart Notepad++ to take effect."):TEXT("");
					::SetDlgItemText(_hSelf, IDC_SETTINGSONCLOUD_WARNING_STATIC, message.c_str());
				}
				break;

				case IDC_DROPBOX_RADIO :
				{
					nppGUI._cloudChoice = dropbox;
					setCloudChoice("dropbox");

					generic_string message = _initialCloudChoice != nppGUI._cloudChoice?TEXT("Please restart Notepad++ to take effect."):TEXT("");
					::SetDlgItemText(_hSelf, IDC_SETTINGSONCLOUD_WARNING_STATIC, message.c_str());
				}
				break;

				case IDC_ONEDRIVE_RADIO :
				{
					nppGUI._cloudChoice = oneDrive;
					setCloudChoice("oneDrive");

					generic_string message = _initialCloudChoice != nppGUI._cloudChoice?TEXT("Please restart Notepad++ to take effect."):TEXT("");
					::SetDlgItemText(_hSelf, IDC_SETTINGSONCLOUD_WARNING_STATIC, message.c_str());
				}
				break;

				case IDC_GOOGLEDRIVE_RADIO :
				{
					nppGUI._cloudChoice = googleDrive;
					setCloudChoice("googleDrive");

					generic_string message = _initialCloudChoice != nppGUI._cloudChoice?TEXT("Please restart Notepad++ to take effect."):TEXT("");
					::SetDlgItemText(_hSelf, IDC_SETTINGSONCLOUD_WARNING_STATIC, message.c_str());
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

void SettingsOnCloudDlg::setCloudChoice(const char *choice)
{
	generic_string cloudChoicePath = (NppParameters::getInstance())->getSettingsFolder();
	cloudChoicePath += TEXT("\\cloud\\");

	if (!PathFileExists(cloudChoicePath.c_str()))
	{
		::CreateDirectory(cloudChoicePath.c_str(), NULL);
	}
	cloudChoicePath += TEXT("choice");
	writeFileContent(cloudChoicePath.c_str(), choice);

}

void SettingsOnCloudDlg::removeCloudChoice()
{
	generic_string cloudChoicePath = (NppParameters::getInstance())->getSettingsFolder();
	//NppParameters *nppParams = ;

	cloudChoicePath += TEXT("\\cloud\\choice");
	if (PathFileExists(cloudChoicePath.c_str()))
	{
		::DeleteFile(cloudChoicePath.c_str());
	}
}

