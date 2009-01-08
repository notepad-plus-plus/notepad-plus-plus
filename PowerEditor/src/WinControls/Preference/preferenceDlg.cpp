//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <windows.h>
#include "preferenceDlg.h"

const int BLINKRATE_FASTEST = 50;
const int BLINKRATE_SLOWEST = 2500;
const int BLINKRATE_INTERVAL = 50;

BOOL CALLBACK PreferenceDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			_ctrlTab.init(_hInst, _hSelf, false, true, true);
			_ctrlTab.setFont(TEXT("Tahoma"), 13);
			
			_barsDlg.init(_hInst, _hSelf);
			_barsDlg.create(IDD_PREFERENCE_BAR_BOX);
			_barsDlg.display();
			
			_marginsDlg.init(_hInst, _hSelf);
			_marginsDlg.create(IDD_PREFERENCE_MARGEIN_BOX);
			
			_settingsDlg.init(_hInst, _hSelf);
			_settingsDlg.create(IDD_PREFERENCE_SETTING_BOX);
			
			_defaultNewDocDlg.init(_hInst, _hSelf);
			_defaultNewDocDlg.create(IDD_PREFERENCE_NEWDOCSETTING_BOX);

			_fileAssocDlg.init(_hInst, _hSelf);
			_fileAssocDlg.create(IDD_REGEXT_BOX);

			_printSettingsDlg.init(_hInst, _hSelf);
			_printSettingsDlg.create(IDD_PREFERENCE_PRINT_BOX);

		
			_printSettings2Dlg.init(_hInst, _hSelf);
			_printSettings2Dlg.create(IDD_PREFERENCE_PRINT2_BOX);

			_langMenuDlg.init(_hInst, _hSelf);
			_langMenuDlg.create(IDD_PREFERENCE_LANG_BOX);

			_backupDlg.init(_hInst, _hSelf);
			_backupDlg.create(IDD_PREFERENCE_BACKUP_BOX);

			_wVector.push_back(DlgInfo(&_barsDlg, TEXT("Global"), TEXT("Global")));
			_wVector.push_back(DlgInfo(&_marginsDlg, TEXT("Edit Components"), TEXT("Scintillas")));
			_wVector.push_back(DlgInfo(&_defaultNewDocDlg, TEXT("New Document/Open Save Directory"), TEXT("NewDoc")));
			_wVector.push_back(DlgInfo(&_fileAssocDlg, TEXT("File Association"), TEXT("FileAssoc")));
			_wVector.push_back(DlgInfo(&_langMenuDlg, TEXT("Language Menu"), TEXT("LangMenu")));
			_wVector.push_back(DlgInfo(&_printSettingsDlg, TEXT("Print - Colour and Margin"), TEXT("Print1")));
			_wVector.push_back(DlgInfo(&_printSettings2Dlg, TEXT("Print - Header and Footer"), TEXT("Print2")));
			_wVector.push_back(DlgInfo(&_backupDlg, TEXT("Backup/Auto-completion"), TEXT("Backup")));
			_wVector.push_back(DlgInfo(&_settingsDlg, TEXT("MISC"), TEXT("MISC")));
			_ctrlTab.createTabs(_wVector);
			_ctrlTab.display();
			RECT rc;
			getClientRect(rc);
			_ctrlTab.reSizeTo(rc);
			rc.bottom -= 30;
			
			_barsDlg.reSizeTo(rc);
			_marginsDlg.reSizeTo(rc);
			_settingsDlg.reSizeTo(rc);
			_defaultNewDocDlg.reSizeTo(rc);
			_fileAssocDlg.reSizeTo(rc);
			_langMenuDlg.reSizeTo(rc);
			_printSettingsDlg.reSizeTo(rc);
			_printSettings2Dlg.reSizeTo(rc);
			_backupDlg.reSizeTo(rc);

			NppParameters *pNppParam = NppParameters::getInstance();
			ETDTProc enableDlgTheme = (ETDTProc)pNppParam->getEnableThemeDlgTexture();
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);
			return TRUE;
		}

		case WM_NOTIFY :		  
		{
			NMHDR *nmhdr = (NMHDR *)lParam;
			if (nmhdr->code == TCN_SELCHANGE)
			{
				if (nmhdr->hwndFrom == _ctrlTab.getHSelf())
				{
					_ctrlTab.clickedUpdate();
					return TRUE;
				}
			}
			break;
		}

		case WM_COMMAND : 
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
	return FALSE;
}

BOOL CALLBACK BarsDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
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

#ifndef UNICODE
			::EnableWindow(::GetDlgItem(_hSelf, IDC_LOCALIZATION_GB_STATIC), FALSE);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_COMBO_LOCALIZATION), FALSE);
#else
			LocalizationSwicher & localizationSwitcher = pNppParam->getLocalizationSwitcher();

			for (size_t i = 0 ; i < localizationSwitcher.size() ; i++)
			{
				pair<wstring, wstring> localizationInfo = localizationSwitcher.getElementFromIndex(i);
				::SendDlgItemMessage(_hSelf, IDC_COMBO_LOCALIZATION, CB_ADDSTRING, 0, (LPARAM)localizationInfo.first.c_str());
			}
#endif

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
#ifdef UNICODE
				default :
					switch (HIWORD(wParam))
					{
						case CBN_SELCHANGE : // == case LBN_SELCHANGE :
						{
							switch (LOWORD(wParam))
							{
								case IDC_COMBO_LOCALIZATION :
								{
									LocalizationSwicher & localizationSwitcher = pNppParam->getLocalizationSwitcher();
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
#endif
			}
		}
	}
	return FALSE;
}

void MarginsDlg::changePanelTo(int index)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	const ScintillaViewParams & svp = pNppParam->getSVP(index?SCIV_SECOND:SCIV_PRIMARY);
	
	::SendDlgItemMessage(_hSelf, IDC_RADIO_BOX, BM_SETCHECK, FALSE, 0);
	::SendDlgItemMessage(_hSelf, IDC_RADIO_CIRCLE, BM_SETCHECK, FALSE, 0);
	::SendDlgItemMessage(_hSelf, IDC_RADIO_ARROW, BM_SETCHECK, FALSE, 0);
	::SendDlgItemMessage(_hSelf, IDC_RADIO_SIMPLE, BM_SETCHECK, FALSE, 0);

	int id = 0;
	switch (svp._folderStyle)
	{
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
	
	::SendDlgItemMessage(_hSelf, IDC_CHECK_LINENUMBERMARGE, BM_SETCHECK, svp._lineNumberMarginShow, 0);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_BOOKMARKMARGE, BM_SETCHECK, svp._bookMarkMarginShow, 0);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_CURRENTLINEHILITE, BM_SETCHECK, svp._currentLineHilitingShow, 0);
	
	bool isEnable = !(svp._edgeMode == EDGE_NONE);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_SHOWVERTICALEDGE, BM_SETCHECK, isEnable, 0);
	::SendDlgItemMessage(_hSelf, IDC_RADIO_LNMODE, BM_SETCHECK, (svp._edgeMode == EDGE_LINE), 0);
	::SendDlgItemMessage(_hSelf, IDC_RADIO_BGMODE, BM_SETCHECK, (svp._edgeMode == EDGE_BACKGROUND), 0);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_LNMODE), isEnable);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_BGMODE), isEnable);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_NBCOLONE_STATIC), isEnable);
	
	TCHAR nbColStr[10];
	wsprintf(nbColStr, TEXT("%d"), svp._edgeNbColumn);
	::SetWindowText(::GetDlgItem(_hSelf, IDC_COLONENUMBER_STATIC), nbColStr);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_COLONENUMBER_STATIC), isEnable);

}

BOOL CALLBACK MarginsDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			TCHAR nbStr[10];
			wsprintf(nbStr, TEXT("%d"), nppGUI._tabSize);
			HWND hTabSize_val = ::GetDlgItem(_hSelf, IDC_TABSIZEVAL_STATIC);
			::SetWindowText(hTabSize_val, nbStr);

			_tabSizeVal.init(_hInst, _hSelf);
			_tabSizeVal.create(hTabSize_val, IDM_SETTING_TAB_SIZE);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_REPLACEBYSPACE, BM_SETCHECK, nppGUI._tabReplacedBySpace, 0);

			_verticalEdgeLineNbColVal.init(_hInst, _hSelf);
			_verticalEdgeLineNbColVal.create(::GetDlgItem(_hSelf, IDC_COLONENUMBER_STATIC), IDM_SETTING_EDGE_SIZE);

			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, (LPARAM)TEXT("0"));
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, (LPARAM)TEXT("1"));
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, (LPARAM)TEXT("2"));
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, (LPARAM)TEXT("3"));
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, (LPARAM)TEXT("Block"));


			::SendMessage(::GetDlgItem(_hSelf, IDC_WIDTH_COMBO), CB_SETCURSEL, nppGUI._caretWidth, 0);
			
			::SendMessage(::GetDlgItem(_hSelf, IDC_CARETBLINKRATE_SLIDER),TBM_SETRANGEMIN, TRUE, BLINKRATE_FASTEST);
			::SendMessage(::GetDlgItem(_hSelf, IDC_CARETBLINKRATE_SLIDER),TBM_SETRANGEMAX, TRUE, BLINKRATE_SLOWEST);
			::SendMessage(::GetDlgItem(_hSelf, IDC_CARETBLINKRATE_SLIDER),TBM_SETPAGESIZE, 0, BLINKRATE_INTERVAL);
			int blinkRate = (nppGUI._caretBlinkRate==0)?BLINKRATE_SLOWEST:nppGUI._caretBlinkRate;
			::SendMessage(::GetDlgItem(_hSelf, IDC_CARETBLINKRATE_SLIDER),TBM_SETPOS, TRUE, blinkRate);

			::SendDlgItemMessage(_hSelf, IDC_COMBO_SCINTILLAVIEWCHOIX, CB_ADDSTRING, 0, (LPARAM)TEXT("Primary View"));
			::SendDlgItemMessage(_hSelf, IDC_COMBO_SCINTILLAVIEWCHOIX, CB_ADDSTRING, 0, (LPARAM)TEXT("Second View"));
			::SendDlgItemMessage(_hSelf, IDC_COMBO_SCINTILLAVIEWCHOIX, CB_SETCURSEL, 0, 0);
			
			changePanelTo(SCIV_PRIMARY);
			
			ETDTProc enableDlgTheme = (ETDTProc)pNppParam->getEnableThemeDlgTexture();
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);
			return TRUE;
		}

		case WM_HSCROLL:
		{
			//case IDC_CARETBLINKRATE_SLIDER:
			NppGUI & nppGUI = (NppGUI &)NppParameters::getInstance()->getNppGUI();
			//nppGUI._caretBlinkRate = ::SendMessage(::GetDlgItem(_hSelf, IDC_CARETBLINKRATE_SLIDER),TBM_GETPOS, 0, 0);
			int blinkRate = (int)::SendMessage(::GetDlgItem(_hSelf, IDC_CARETBLINKRATE_SLIDER),TBM_GETPOS, 0, 0);
			if (blinkRate == BLINKRATE_SLOWEST)
				blinkRate = 0;
			nppGUI._caretBlinkRate = blinkRate;

			::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SETCARETBLINKRATE, 0, 0);
			return 0;	//return zero when handled
				
		}

		case WM_COMMAND : 
		{			
			int i = ::SendDlgItemMessage(_hSelf, IDC_COMBO_SCINTILLAVIEWCHOIX, CB_GETCURSEL, 0, 0);
			ScintillaViewParams & svp = (ScintillaViewParams &)pNppParam->getSVP(i?SCIV_SECOND:SCIV_PRIMARY);
			int iView = i + 1;
			switch (wParam)
			{
				case IDM_SETTING_TAB_SIZE:
				{
					::SendMessage(_hParent, WM_COMMAND, IDM_SETTING_TAB_SIZE, 0);
					TCHAR nbStr[10];
					wsprintf(nbStr, TEXT("%d"), nppGUI._tabSize);
					::SetWindowText(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_STATIC), nbStr);
					return TRUE;
				}

				case IDC_CHECK_REPLACEBYSPACE:
					::SendMessage(_hParent, WM_COMMAND, IDM_SETTING_TAB_REPLCESPACE, 0);
					return TRUE;

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
				
				case IDM_SETTING_EDGE_SIZE:
				{
					::SendMessage(_hParent, WM_COMMAND, IDM_SETTING_EDGE_SIZE, iView);
					TCHAR nbColStr[10];
					wsprintf(nbColStr, TEXT("%d"), svp._edgeNbColumn);
					::SetWindowText(::GetDlgItem(_hSelf, IDC_COLONENUMBER_STATIC), nbColStr);
					return TRUE;
				}

				default :
					switch (HIWORD(wParam))
					{
						case CBN_SELCHANGE : // == case LBN_SELCHANGE :
						{
							switch (LOWORD(wParam))
							{
								case IDC_COMBO_SCINTILLAVIEWCHOIX :
								{
									changePanelTo(i);
									return TRUE;
								}
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

BOOL CALLBACK SettingsDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			TCHAR nbStr[10];
			wsprintf(nbStr, TEXT("%d"), pNppParam->getNbMaxFile());
			::SetWindowText(::GetDlgItem(_hSelf, IDC_MAXNBFILEVAL_STATIC), nbStr);

			_nbHistoryVal.init(_hInst, _hSelf);
			_nbHistoryVal.create(::GetDlgItem(_hSelf, IDC_MAXNBFILEVAL_STATIC), IDM_SETTING_HISTORY_SIZE);

			::SendDlgItemMessage(_hSelf, IDC_CHECK_DONTCHECKHISTORY, BM_SETCHECK, !nppGUI._checkHistoryFiles, 0);
			
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
			::SendDlgItemMessage(_hSelf, IDC_CHECK_REMEMBERSESSION, BM_SETCHECK, nppGUI._rememberLastSession, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_AUTOUPDATE, BM_SETCHECK, !nppGUI._neverUpdate, 0);

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
			::SendDlgItemMessage(_hSelf, IDC_CHECK_ENABLTAGSMATCHHILITE, BM_SETCHECK, nppGUI._enableTagsMatchHilite, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_ENABLTAGATTRHILITE, BM_SETCHECK, nppGUI._enableTagAttrsHilite, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_HIGHLITENONEHTMLZONE, BM_SETCHECK, nppGUI._enableHiliteNonHTMLZone, 0);

			::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_ENABLTAGATTRHILITE), nppGUI._enableTagsMatchHilite);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_HIGHLITENONEHTMLZONE), nppGUI._enableTagsMatchHilite);

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
				case IDC_CHECK_DONTCHECKHISTORY:
					nppGUI._checkHistoryFiles = !isCheckedOrNot(IDC_CHECK_DONTCHECKHISTORY);
					return TRUE;
				
				case IDC_CHECK_FILEAUTODETECTION:
				{
					bool isChecked = isCheckedOrNot(IDC_CHECK_FILEAUTODETECTION);
					if (!isChecked)
					{
						::SendDlgItemMessage(_hSelf, IDC_CHECK_UPDATESILENTLY, BM_SETCHECK, BST_UNCHECKED, 0);
						::SendDlgItemMessage(_hSelf, IDC_CHECK_UPDATEGOTOEOF, BM_SETCHECK, BST_UNCHECKED, 0);
					}
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_UPDATESILENTLY), isChecked);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_UPDATEGOTOEOF), isChecked);

					nppGUI._fileAutoDetection = isChecked?cdAutoUpdate:cdDisabled;
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
					nppGUI._neverUpdate = !isCheckedOrNot(wParam);
					return TRUE;

				case IDC_CHECK_MIN2SYSTRAY:
					nppGUI._isMinimizedToTray = isCheckedOrNot(wParam);
					return TRUE;

				case IDC_CHECK_REMEMBERSESSION:
					//::SendMessage(_hParent, WM_COMMAND, IDM_SETTING_REMEMBER_LAST_SESSION, 0);
					nppGUI._rememberLastSession = isCheckedOrNot(wParam);
					return TRUE;

				case IDM_SETTING_HISTORY_SIZE:
				{
					::SendMessage(_hParent, WM_COMMAND, IDM_SETTING_HISTORY_SIZE, 0);
					TCHAR nbStr[10];
					wsprintf(nbStr, TEXT("%d"), pNppParam->getNbMaxFile());
					::SetWindowText(::GetDlgItem(_hSelf, IDC_MAXNBFILEVAL_STATIC), nbStr);
					return TRUE;
				}

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
			}
		}
	}
	return FALSE;
}

BOOL CALLBACK DefaultNewDocDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
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

			switch (ndds._encoding)
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
			::SendDlgItemMessage(_hSelf, ID2Check, BM_SETCHECK, BST_CHECKED, 0);

			int index = 0;
			for (int i = L_TXT ; i < pNppParam->L_END ; i++)
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
			//::ExpandEnvironmentStrings(nppGUI._defaultDir, nppGUI._defaultDirExp, 500);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_OPENSAVEDIR_ALWAYSON_EDIT), shouldActivated);
			::EnableWindow(::GetDlgItem(_hSelf, IDD_OPENSAVEDIR_ALWAYSON_BROWSE_BUTTON), shouldActivated);
			
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
				case IDC_RADIO_UCS2BIG:
					ndds._encoding = uni16BE;
					return TRUE;
				case IDC_RADIO_UCS2SMALL:
					ndds._encoding = uni16LE;
					return TRUE;
				case IDC_RADIO_UTF8:
					ndds._encoding = uniUTF8;
					return TRUE;
				case IDC_RADIO_UTF8SANSBOM:
					ndds._encoding = uniCookie;
					return TRUE;
				case IDC_RADIO_ANSI:
					ndds._encoding = uni8Bit;
					return TRUE;


				case IDC_RADIO_F_MAC:
					ndds._format = MAC_FORMAT;
					return TRUE;
				case IDC_RADIO_F_UNIX:
					ndds._format = UNIX_FORMAT;
					return TRUE;
				case IDC_RADIO_F_WIN:
					ndds._format = WIN_FORMAT;
					return TRUE;

			
			
			
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
					if ((HIWORD(wParam) == CBN_SELCHANGE) && (LOWORD(wParam) == IDC_COMBO_DEFAULTLANG))
					{
						int index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_DEFAULTLANG, CB_GETCURSEL, 0, 0);
						ndds._lang = _langList[index]._id;
						return TRUE;
					}
					return FALSE;
			}
		}
	}
	return FALSE;
}

BOOL CALLBACK LangMenuDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )pNppParam->getNppGUI();

	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			for (int i = L_TXT ; i < pNppParam->L_END ; i++)
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

			for (size_t i = 0 ; i < nppGUI._excludedLangList.size() ; i++)
			{
				::SendDlgItemMessage(_hSelf, IDC_LIST_DISABLEDLANG, LB_ADDSTRING, 0, (LPARAM)nppGUI._excludedLangList[i]._langName.c_str());
			}


			::EnableWindow(::GetDlgItem(_hSelf, IDC_BUTTON_REMOVE), FALSE);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_BUTTON_RESTORE), FALSE);

			ETDTProc enableDlgTheme = (ETDTProc)pNppParam->getEnableThemeDlgTexture();
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);

			return TRUE;
		}
		case WM_COMMAND : 
		{
			switch (LOWORD(wParam))
            {
				case IDC_LIST_DISABLEDLANG :
                case IDC_LIST_ENABLEDLANG :
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
					::SendDlgItemMessage(_hSelf, list2Remove, LB_SETCURSEL, -1, 0);
					::EnableWindow(::GetDlgItem(_hSelf, idButton2Enable), TRUE);
					::EnableWindow(::GetDlgItem(_hSelf, idButton2Disable), FALSE);

					if ((lmi._langType >= L_EXTERNAL) && (lmi._langType < pNppParam->L_END))
					{
						bool found(false);
						for(size_t x = 0; x < pNppParam->getExternalLexerDoc()->size() && !found; x++)
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

BOOL CALLBACK PrintSettingsDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
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

			TCHAR valStrL[10];
			wsprintf(valStrL, TEXT("%d"), nppGUI._printSettings._marge.left);

			TCHAR valStrR[10];
			wsprintf(valStrR, TEXT("%d"), nppGUI._printSettings._marge.right);

			TCHAR valStrT[10];
			wsprintf(valStrT, TEXT("%d"), nppGUI._printSettings._marge.top);

			TCHAR valStrB[10];
			wsprintf(valStrB, TEXT("%d"), nppGUI._printSettings._marge.bottom);

			::SendDlgItemMessage(_hSelf, IDC_EDIT_ML, WM_SETTEXT, 0, (LPARAM)valStrL);
			::SendDlgItemMessage(_hSelf, IDC_EDIT_MR, WM_SETTEXT, 0, (LPARAM)valStrR);
			::SendDlgItemMessage(_hSelf, IDC_EDIT_MT, WM_SETTEXT, 0, (LPARAM)valStrT);
			::SendDlgItemMessage(_hSelf, IDC_EDIT_MB, WM_SETTEXT, 0, (LPARAM)valStrB);

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
						return FALSE;
				}
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
			}
			return TRUE;
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

BOOL CALLBACK PrintSettings2Dlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )pNppParam->getNppGUI();

	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			::SendDlgItemMessage(_hSelf, IDC_EDIT_HLEFT, WM_SETTEXT, 0, (LPARAM)nppGUI._printSettings._headerLeft.c_str());
			::SendDlgItemMessage(_hSelf, IDC_EDIT_HMIDDLE, WM_SETTEXT, 0, (LPARAM)nppGUI._printSettings._headerMiddle.c_str());
			::SendDlgItemMessage(_hSelf, IDC_EDIT_HRIGHT, WM_SETTEXT, 0, (LPARAM)nppGUI._printSettings._headerRight.c_str());
			::SendDlgItemMessage(_hSelf, IDC_EDIT_FLEFT, WM_SETTEXT, 0, (LPARAM)nppGUI._printSettings._footerLeft.c_str());
			::SendDlgItemMessage(_hSelf, IDC_EDIT_FMIDDLE, WM_SETTEXT, 0, (LPARAM)nppGUI._printSettings._footerMiddle.c_str());
			::SendDlgItemMessage(_hSelf, IDC_EDIT_FRIGHT, WM_SETTEXT, 0, (LPARAM)nppGUI._printSettings._footerRight.c_str());

			TCHAR intStr[5];
			for(size_t i = 6 ; i < 15 ; i++)
			{
				wsprintf(intStr, TEXT("%d"), i);
				::SendDlgItemMessage(_hSelf, IDC_COMBO_HFONTSIZE, CB_ADDSTRING, 0, (LPARAM)intStr);
				::SendDlgItemMessage(_hSelf, IDC_COMBO_FFONTSIZE, CB_ADDSTRING, 0, (LPARAM)intStr);
			}
			const std::vector<std::generic_string> & fontlist = pNppParam->getFontList();
			for (size_t i = 0 ; i < fontlist.size() ; i++)
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
			::SendDlgItemMessage(_hSelf, IDC_COMBO_HFONTSIZE, CB_SELECTSTRING, -1, (LPARAM)intStr);
			wsprintf(intStr, TEXT("%d"), nppGUI._printSettings._footerFontSize);
			::SendDlgItemMessage(_hSelf, IDC_COMBO_FFONTSIZE, CB_SELECTSTRING, -1, (LPARAM)intStr);

			::SendDlgItemMessage(_hSelf, IDC_CHECK_HBOLD, BM_SETCHECK, nppGUI._printSettings._headerFontStyle & FONTSTYLE_BOLD?TRUE:FALSE, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_HITALIC, BM_SETCHECK, nppGUI._printSettings._headerFontStyle & FONTSTYLE_ITALIC?TRUE:FALSE, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_FBOLD, BM_SETCHECK, nppGUI._printSettings._footerFontStyle & FONTSTYLE_BOLD?TRUE:FALSE, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_FITALIC, BM_SETCHECK, nppGUI._printSettings._footerFontStyle & FONTSTYLE_ITALIC?TRUE:FALSE, 0);

			varList.push_back(strCouple(TEXT("Full file name path"), TEXT("$(FULL_CURRENT_PATH)")));
			varList.push_back(strCouple(TEXT("File name"), TEXT("$(FILE_NAME)")));
			varList.push_back(strCouple(TEXT("File directory"), TEXT("$(FULL_CURRENT_PATH)")));
			varList.push_back(strCouple(TEXT("Page"), TEXT("$(CURRENT_PRINTING_PAGE)")));
			varList.push_back(strCouple(TEXT("Short date format"), TEXT("$(SHORT_DATE)")));
			varList.push_back(strCouple(TEXT("Long date format"), TEXT("$(LONG_DATE)")));
			varList.push_back(strCouple(TEXT("Time"), TEXT("$(TIME)")));

			for (size_t i = 0 ; i < varList.size() ; i++)
			{
				int j = ::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_ADDSTRING, 0, (LPARAM)varList[i]._varDesc.c_str());
				::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_SETITEMDATA, j, (LPARAM)varList[i]._var.c_str());
			}
			::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_SETCURSEL, 0, 0);

			//_colourHooker.setColour(RGB(0, 0, 0xFF));
			//_colourHooker.hookOn(::GetDlgItem(_hSelf, IDC_VIEWPANEL_STATIC));
			ETDTProc enableDlgTheme = (ETDTProc)pNppParam->getEnableThemeDlgTexture();
			if (enableDlgTheme)
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);

			return TRUE;
		}
		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == EN_CHANGE)
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
			else if (HIWORD(wParam) == EN_SETFOCUS)
			{
				const int stringSize = 256;
				TCHAR str[stringSize];
				_focusedEditCtrl = LOWORD(wParam);
				::GetDlgItemText(_hSelf, _focusedEditCtrl, str, stringSize);
				//_colourHooker.setColour(RGB(0, 0, 0xFF));
				::SendDlgItemMessage(_hSelf, IDC_VIEWPANEL_STATIC, WM_SETTEXT, 0, (LPARAM)str);
				
				int focusedEditStatic;
				int groupStatic;
				switch (_focusedEditCtrl)
				{
					case IDC_EDIT_HLEFT : focusedEditStatic = IDC_HL_STATIC; groupStatic = IDC_HGB_STATIC; break;
					case IDC_EDIT_HMIDDLE : focusedEditStatic = IDC_HM_STATIC; groupStatic = IDC_HGB_STATIC; break;
					case IDC_EDIT_HRIGHT : focusedEditStatic = IDC_HR_STATIC; groupStatic = IDC_HGB_STATIC; break;
					case IDC_EDIT_FLEFT : focusedEditStatic = IDC_FL_STATIC; groupStatic = IDC_FGB_STATIC; break;
					case IDC_EDIT_FMIDDLE : focusedEditStatic = IDC_FM_STATIC; groupStatic = IDC_FGB_STATIC; break;
					case IDC_EDIT_FRIGHT : focusedEditStatic = IDC_FR_STATIC; groupStatic = IDC_FGB_STATIC; break;
				}

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

						if ((!intStr) || (!intStr[0]))
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
/*
					TCHAR toto[32];
					wsprintf(toto, TEXT("_selStart = %d\r_selEnd = %d"), _selStart, _selEnd);
					::MessageBox(NULL, toto, TEXT(""), MB_OK);
*/
					const int stringSize = 256;
					TCHAR str[stringSize];
					::SendDlgItemMessage(_hSelf, _focusedEditCtrl, WM_GETTEXT, stringSize, (LPARAM)str);
					//::MessageBox(NULL, str, TEXT(""), MB_OK);

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

BOOL CALLBACK BackupDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			TCHAR nbStr[10];
			wsprintf(nbStr, TEXT("%d"), nppGUI._autocFromLen);
			HWND hNbChar_val = ::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_N);
			::SetWindowText(hNbChar_val, nbStr);

			_nbCharVal.init(_hInst, _hSelf);
			_nbCharVal.create(hNbChar_val, IDM_SETTING_AUTOCNBCHAR);

			int ID2Check = 0;

			switch (nppGUI._backup)
			{
				case bak_simple :
					ID2Check = IDC_RADIO_BKSIMPLE;
					break;
				case bak_verbose :
					ID2Check = IDC_RADIO_BKVERBOSE;
					break;
				
				default : //bak_none
					ID2Check = IDC_RADIO_BKNONE;
			}
			::SendDlgItemMessage(_hSelf, ID2Check, BM_SETCHECK, BST_CHECKED, 0);

			if (nppGUI._useDir)
				::SendDlgItemMessage(_hSelf, IDC_BACKUPDIR_CHECK, BM_SETCHECK, BST_CHECKED, 0);

			::SendDlgItemMessage(_hSelf, IDC_BACKUPDIR_EDIT, WM_SETTEXT, 0, (LPARAM)nppGUI._backupDir);
			
			bool isEnableAutoC = nppGUI._autocStatus != nppGUI.autoc_none;

			::SendDlgItemMessage(_hSelf, IDD_AUTOC_ENABLECHECK, BM_SETCHECK, isEnableAutoC?BST_CHECKED:BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDD_AUTOC_FUNCRADIO, BM_SETCHECK, nppGUI._autocStatus == nppGUI.autoc_func?BST_CHECKED:BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDD_AUTOC_WORDRADIO, BM_SETCHECK, nppGUI._autocStatus == nppGUI.autoc_word?BST_CHECKED:BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDD_FUNC_CHECK, BM_SETCHECK, nppGUI._funcParams?BST_CHECKED:BST_UNCHECKED, 0);
			if (!isEnableAutoC)
			{
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_FUNCRADIO), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_WORDRADIO), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_FROM), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_N), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_CHAR), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_NOTE), FALSE);
			}
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
						lstrcpy(nppGUI._backupDir, inputDir);
						return TRUE;
					}
				}
			}

			switch (wParam)
			{
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

				case IDD_AUTOC_ENABLECHECK :
				{
					bool isEnableAutoC = BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDD_AUTOC_ENABLECHECK, BM_GETCHECK, 0, 0);

					if (isEnableAutoC)
					{
						::SendDlgItemMessage(_hSelf, IDD_AUTOC_FUNCRADIO, BM_SETCHECK, BST_CHECKED, 0);
						nppGUI._autocStatus = nppGUI.autoc_func;
					}
					else 
					{
						::SendDlgItemMessage(_hSelf, IDD_AUTOC_FUNCRADIO, BM_SETCHECK, BST_UNCHECKED, 0);
						::SendDlgItemMessage(_hSelf, IDD_AUTOC_WORDRADIO, BM_SETCHECK, BST_UNCHECKED, 0);
						nppGUI._autocStatus = nppGUI.autoc_none;
					}
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_FUNCRADIO), isEnableAutoC);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_WORDRADIO), isEnableAutoC);
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
				case IDD_FUNC_CHECK :
				{
					nppGUI._funcParams = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDD_FUNC_CHECK, BM_GETCHECK, 0, 0));
					return TRUE;
				}
				
				case IDM_SETTING_AUTOCNBCHAR :
				{
					::SendMessage(_hParent, WM_COMMAND, IDM_SETTING_AUTOCNBCHAR, 0);
					TCHAR nbStr[10];
					wsprintf(nbStr, TEXT("%d"), pNppParam->getNppGUI()._autocFromLen);
					::SetWindowText(::GetDlgItem(_hSelf, IDD_AUTOC_STATIC_N), nbStr);
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
