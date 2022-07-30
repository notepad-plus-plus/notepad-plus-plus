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

#include <shlwapi.h>
#include "preferenceDlg.h"
#include "lesDlgs.h"
#include "EncodingMapper.h"
#include "localization.h"

#define MyGetGValue(rgb)      (LOBYTE((rgb)>>8))

using namespace std;

const int BLINKRATE_FASTEST = 50;
const int BLINKRATE_SLOWEST = 2500;
const int BLINKRATE_INTERVAL = 50;

const int CARETLINEFRAME_SMALLEST = 1;
const int CARETLINEFRAME_LARGEST = 6;
const int CARETLINEFRAME_INTERVAL = 1;

const int BORDERWIDTH_SMALLEST = 0;
const int BORDERWIDTH_LARGEST = 30;
const int BORDERWIDTH_INTERVAL = 1;

const int PADDING_SMALLEST = 0;
const int PADDING_LARGEST = 30;
const int PADDING_INTERVAL = 1;

const int DISTRACTIONFREE_SMALLEST = 3;
const int DISTRACTIONFREE_LARGEST = 9;
const int DISTRACTIONFREE_INTERVAL = 1;

// This int encoding array is built from "EncodingUnit encodings[]" (see EncodingMapper.cpp)
// And NewDocumentSubDlg will use "int encoding array" to get more info from "EncodingUnit encodings[]"
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

bool PreferenceDlg::goToSection(size_t iPage, intptr_t ctrlID)
{
	::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_SETCURSEL, iPage, 0);
	showDialogByIndex(iPage);
	getFocus();

	if (ctrlID != -1)
	{
		::SetFocus(::GetDlgItem(_wVector[iPage]._dlg->getHSelf(), int(ctrlID)));
	}

	return true;
}

intptr_t CALLBACK PreferenceDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			_generalSubDlg.init(_hInst, _hSelf);
			_generalSubDlg.create(IDD_PREFERENCE_SUB_GENRAL, false, false);
			_generalSubDlg.display();
			
			_editingSubDlg.init(_hInst, _hSelf);
			_editingSubDlg.create(IDD_PREFERENCE_SUB_EDITING, false, false);

			_darkModeSubDlg.init(_hInst, _hSelf);
			_darkModeSubDlg.create(IDD_PREFERENCE_SUB_DARKMODE, false, false);

			_marginsBorderEdgeSubDlg.init(_hInst, _hSelf);
			_marginsBorderEdgeSubDlg.create(IDD_PREFERENCE_SUB_MARGING_BORDER_EDGE, false, false);
			
			_miscSubDlg.init(_hInst, _hSelf);
			_miscSubDlg.create(IDD_PREFERENCE_SUB_MISC, false, false);
			
			_newDocumentSubDlg.init(_hInst, _hSelf);
			_newDocumentSubDlg.create(IDD_PREFERENCE_SUB_NEWDOCUMENT, false, false);

			_defaultDirectorySubDlg.init(_hInst, _hSelf);
			_defaultDirectorySubDlg.create(IDD_PREFERENCE_SUB_DEFAULTDIRECTORY, false, false);

			_recentFilesHistorySubDlg.init(_hInst, _hSelf);
			_recentFilesHistorySubDlg.create(IDD_PREFERENCE_SUB_RECENTFILESHISTORY, false, false);

			_fileAssocDlg.init(_hInst, _hSelf);
			_fileAssocDlg.create(IDD_REGEXT_BOX, false, false);

			_printSubDlg.init(_hInst, _hSelf);
			_printSubDlg.create(IDD_PREFERENCE_SUB_PRINT, false, false);

			_searchingSubDlg.init(_hInst, _hSelf);
			_searchingSubDlg.create(IDD_PREFERENCE_SUB_SEARCHING, false, false);

			_languageSubDlg.init(_hInst, _hSelf);
			_languageSubDlg.create(IDD_PREFERENCE_SUB_LANGUAGE, false, false);

			_highlightingSubDlg.init(_hInst, _hSelf);
			_highlightingSubDlg.create(IDD_PREFERENCE_SUB_HIGHLIGHTING, false, false);

			_backupSubDlg.init(_hInst, _hSelf);
			_backupSubDlg.create(IDD_PREFERENCE_SUB_BACKUP, false, false);

			_autoCompletionSubDlg.init(_hInst, _hSelf);
			_autoCompletionSubDlg.create(IDD_PREFERENCE_SUB_AUTOCOMPLETION, false, false);

			_multiInstanceSubDlg.init(_hInst, _hSelf);
			_multiInstanceSubDlg.create(IDD_PREFERENCE_SUB_MULTIINSTANCE, false, false);

			_delimiterSubDlg.init(_hInst, _hSelf);
			_delimiterSubDlg.create(IDD_PREFERENCE_SUB_DELIMITER, false, false);

			_cloudAndLinkSubDlg.init(_hInst, _hSelf);
			_cloudAndLinkSubDlg.create(IDD_PREFERENCE_SUB_CLOUD_LINK, false, false);

			_searchEngineSubDlg.init(_hInst, _hSelf);
			_searchEngineSubDlg.create(IDD_PREFERENCE_SUB_SEARCHENGINE, false, false);			

			_wVector.push_back(DlgInfo(&_generalSubDlg, TEXT("General"), TEXT("Global")));
			_wVector.push_back(DlgInfo(&_editingSubDlg, TEXT("Editing"), TEXT("Scintillas")));
			_wVector.push_back(DlgInfo(&_darkModeSubDlg, TEXT("Dark Mode"), TEXT("DarkMode")));
			_wVector.push_back(DlgInfo(&_marginsBorderEdgeSubDlg, TEXT("Margins/Border/Edge"), TEXT("MarginsBorderEdge")));
			_wVector.push_back(DlgInfo(&_newDocumentSubDlg, TEXT("New Document"), TEXT("NewDoc")));
			_wVector.push_back(DlgInfo(&_defaultDirectorySubDlg, TEXT("Default Directory"), TEXT("DefaultDir")));
			_wVector.push_back(DlgInfo(&_recentFilesHistorySubDlg, TEXT("Recent Files History"), TEXT("RecentFilesHistory")));
			_wVector.push_back(DlgInfo(&_fileAssocDlg, TEXT("File Association"), TEXT("FileAssoc")));
			_wVector.push_back(DlgInfo(&_languageSubDlg, TEXT("Language"), TEXT("Language")));
			_wVector.push_back(DlgInfo(&_highlightingSubDlg, TEXT("Highlighting"), TEXT("Highlighting")));
			_wVector.push_back(DlgInfo(&_printSubDlg, TEXT("Print"), TEXT("Print")));
			_wVector.push_back(DlgInfo(&_searchingSubDlg, TEXT("Searching"), TEXT("Searching")));
			_wVector.push_back(DlgInfo(&_backupSubDlg, TEXT("Backup"), TEXT("Backup")));
			_wVector.push_back(DlgInfo(&_autoCompletionSubDlg, TEXT("Auto-Completion"), TEXT("AutoCompletion")));
			_wVector.push_back(DlgInfo(&_multiInstanceSubDlg, TEXT("Multi-Instance & Date"), TEXT("MultiInstance")));
			_wVector.push_back(DlgInfo(&_delimiterSubDlg, TEXT("Delimiter"), TEXT("Delimiter")));
			_wVector.push_back(DlgInfo(&_cloudAndLinkSubDlg, TEXT("Cloud & Link"), TEXT("Cloud")));
			_wVector.push_back(DlgInfo(&_searchEngineSubDlg, TEXT("Search Engine"), TEXT("SearchEngine")));
			_wVector.push_back(DlgInfo(&_miscSubDlg, TEXT("MISC."), TEXT("MISC")));


			makeCategoryList();
			RECT rc;
			getClientRect(rc);

			rc.top += NppParameters::getInstance()._dpiManager.scaleY(10);
			rc.bottom -= NppParameters::getInstance()._dpiManager.scaleY(50);
			rc.left += NppParameters::getInstance()._dpiManager.scaleX(150);
			
			_generalSubDlg.reSizeTo(rc);
			_editingSubDlg.reSizeTo(rc);
			_darkModeSubDlg.reSizeTo(rc);
			_marginsBorderEdgeSubDlg.reSizeTo(rc);
			_miscSubDlg.reSizeTo(rc);
			_newDocumentSubDlg.reSizeTo(rc);
			_defaultDirectorySubDlg.reSizeTo(rc);
			_recentFilesHistorySubDlg.reSizeTo(rc);
			_fileAssocDlg.reSizeTo(rc);
			_languageSubDlg.reSizeTo(rc);
			_highlightingSubDlg.reSizeTo(rc);
			_printSubDlg.reSizeTo(rc);
			_searchingSubDlg.reSizeTo(rc);
			_backupSubDlg.reSizeTo(rc);
			_autoCompletionSubDlg.reSizeTo(rc);
			_multiInstanceSubDlg.reSizeTo(rc);
			_delimiterSubDlg.reSizeTo(rc);
			_cloudAndLinkSubDlg.reSizeTo(rc);
			_searchEngineSubDlg.reSizeTo(rc);

			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			return TRUE;
		}

		case WM_CTLCOLORLISTBOX:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorListbox(wParam, lParam);
			}
			break;
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
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
			return TRUE;
		}

		case PREF_MSG_ISCHECKED_GENERALPAGE:
		{
			if (!lParam)
				return FALSE;

			bool isChecked = _generalSubDlg.isCheckedOrNot(static_cast<int>(wParam));
			*((bool*)lParam) = isChecked;
			return TRUE;
		}

		case PREF_MSG_SETTOOLICONSFROMSTDTOSMALL:
			_generalSubDlg.setToolIconsFromStdToSmall();
			return TRUE;

		case PREF_MSG_DISABLETABBARALTERNATEICONS:
			_generalSubDlg.disableTabbarAlternateIcons();
			return TRUE;

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
	if (!name)
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

	if (static_cast<size_t>(lbTextLen) > selStrLenMax)
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
	TCHAR oldName[lenMax] = { '\0' };
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
	_generalSubDlg.destroy();
	_editingSubDlg.destroy();
	_darkModeSubDlg.destroy();
	_marginsBorderEdgeSubDlg.destroy();
	_miscSubDlg.destroy();
	_fileAssocDlg.destroy();
	_languageSubDlg.destroy();
	_highlightingSubDlg.destroy();
	_printSubDlg.destroy();
	_searchingSubDlg.destroy();
	_newDocumentSubDlg.destroy();
	_defaultDirectorySubDlg.destroy();
	_recentFilesHistorySubDlg.destroy();
	_backupSubDlg.destroy();
	_autoCompletionSubDlg.destroy();
	_multiInstanceSubDlg.destroy();
	_delimiterSubDlg.destroy();
}

void GeneralSubDlg::setToolIconsFromStdToSmall()
{
	::SendDlgItemMessage(_hSelf, IDC_RADIO_STANDARD, BM_SETCHECK, BST_UNCHECKED, 0);
	::SendDlgItemMessage(_hSelf, IDC_RADIO_SMALLICON, BM_SETCHECK, BST_CHECKED, 0);
	::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_TOOLBAR_REDUCE, 0);
}

void GeneralSubDlg::disableTabbarAlternateIcons()
{
	NppGUI& nppGUI = NppParameters::getInstance().getNppGUI();
	int altIconsBit = TAB_ALTICONS;
	nppGUI._tabStatus &= ~altIconsBit;
	::SendDlgItemMessage(_hSelf, IDC_CHECK_TAB_ALTICONS, BM_SETCHECK, BST_UNCHECKED, 0);
}


intptr_t CALLBACK GeneralSubDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
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
			bool hideRightShortcutsFromMenu = nppGUI._hideMenuRightShortcuts;

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
				case TB_SMALL2 :
					ID2Check = IDC_RADIO_SMALLICON2;
					break;
				case TB_LARGE2 :
					ID2Check = IDC_RADIO_BIGICON2;
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
			::SendDlgItemMessage(_hSelf, IDC_CHECK_TAB_ALTICONS, BM_SETCHECK, tabBarStatus & TAB_ALTICONS, 0);
			
			::SendDlgItemMessage(_hSelf, IDC_CHECK_TAB_HIDE, BM_SETCHECK, tabBarStatus & TAB_HIDE, 0);
			::SendMessage(_hSelf, WM_COMMAND, IDC_CHECK_TAB_HIDE, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_SHOWSTATUSBAR, BM_SETCHECK, showStatus, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_HIDEMENUBAR, BM_SETCHECK, !showMenu, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_HIDERIGHTSHORTCUTSOFMENUBAR, BM_SETCHECK, hideRightShortcutsFromMenu, 0);

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

			return TRUE;
		}

		case WM_CTLCOLORLISTBOX:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
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

				case IDC_CHECK_HIDERIGHTSHORTCUTSOFMENUBAR:
				{
					bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_HIDERIGHTSHORTCUTSOFMENUBAR, BM_GETCHECK, 0, 0));
					NppGUI& nppGUI = nppParam.getNppGUI();
					nppGUI._hideMenuRightShortcuts = isChecked;
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
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_TAB_ALTICONS), !toBeHidden);

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
					NppGUI & nppGUI = nppParam.getNppGUI();
					nppGUI._tabStatus ^= TAB_QUITONEMPTY;
				}
				return TRUE;

				case IDC_CHECK_TAB_ALTICONS:
				{
					NppGUI& nppGUI = nppParam.getNppGUI();
					nppGUI._tabStatus ^= TAB_ALTICONS;
					bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_TAB_ALTICONS, BM_GETCHECK, 0, 0));
					::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_CHANGETABBAEICONS, 0, isChecked ? 1 : (nppGUI._darkmode._isEnabled ? 2 : 0));
					return TRUE;
				}

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

				case IDC_RADIO_SMALLICON2:
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_TOOLBAR_REDUCE_SET2, 0);
					return TRUE;

				case IDC_RADIO_BIGICON2:
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_TOOLBAR_ENLARGE_SET2, 0);
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

void EditingSubDlg::initScintParam()
{
	NppParameters& nppParam = NppParameters::getInstance();
	ScintillaViewParams & svp = const_cast<ScintillaViewParams &>(nppParam.getSVP());

	int id = 0;
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

	int lineHilite = 0;
	switch (svp._currentLineHiliteMode)
	{
		case LINEHILITE_NONE:
			lineHilite = IDC_RADIO_CLM_NONE;
			break;
		case LINEHILITE_FRAME:
			lineHilite = IDC_RADIO_CLM_FRAME;
			break;
		default : // LINEHILITE_HILITE
			lineHilite = IDC_RADIO_CLM_HILITE;
	}
	::SendDlgItemMessage(_hSelf, lineHilite, BM_SETCHECK, TRUE, 0);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_CARETLINEFRAME_WIDTH_SLIDER), (svp._currentLineHiliteMode == LINEHILITE_FRAME));

	::SendDlgItemMessage(_hSelf, IDC_CHECK_VIRTUALSPACE, BM_SETCHECK, svp._virtualSpace, 0);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_SCROLLBEYONDLASTLINE, BM_SETCHECK, svp._scrollBeyondLastLine, 0);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_RIGHTCLICKKEEPSSELECTION, BM_SETCHECK, svp._rightClickKeepsSelection, 0);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_DISABLEADVANCEDSCROLL, BM_SETCHECK, svp._disableAdvancedScrolling, 0);
}

void EditingSubDlg::changeLineHiliteMode(bool enableSlider)
{
	::EnableWindow(::GetDlgItem(_hSelf, IDC_CARETLINEFRAME_WIDTH_SLIDER), enableSlider);
	redraw();
	::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_CURLINE_HILITING, 0);
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


intptr_t CALLBACK EditingSubDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI & nppGUI = nppParam.getNppGUI();
	ScintillaViewParams& svp = (ScintillaViewParams&)nppParam.getSVP();
	
	switch (message)
	{
		case WM_INITDIALOG :
		{
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("0")));
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("1")));
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("2")));
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("3")));
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("Block")));
			::SendDlgItemMessage(_hSelf, IDC_WIDTH_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("Block After")));

			::SendMessage(::GetDlgItem(_hSelf, IDC_WIDTH_COMBO), CB_SETCURSEL, nppGUI._caretWidth, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_FOLDINGTOGGLE, BM_SETCHECK, nppGUI._enableFoldCmdToggable, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_MULTISELECTION, BM_SETCHECK, nppGUI._enableMultiSelection, 0);
			
			::SendMessage(::GetDlgItem(_hSelf, IDC_CARETBLINKRATE_SLIDER),TBM_SETRANGEMIN, TRUE, BLINKRATE_FASTEST);
			::SendMessage(::GetDlgItem(_hSelf, IDC_CARETBLINKRATE_SLIDER),TBM_SETRANGEMAX, TRUE, BLINKRATE_SLOWEST);
			::SendMessage(::GetDlgItem(_hSelf, IDC_CARETBLINKRATE_SLIDER),TBM_SETPAGESIZE, 0, BLINKRATE_INTERVAL);
			int blinkRate = (nppGUI._caretBlinkRate == 0) ? BLINKRATE_SLOWEST : nppGUI._caretBlinkRate;
			::SendMessage(::GetDlgItem(_hSelf, IDC_CARETBLINKRATE_SLIDER),TBM_SETPOS, TRUE, blinkRate);

			::SendMessage(::GetDlgItem(_hSelf, IDC_CARETLINEFRAME_WIDTH_SLIDER), TBM_SETRANGEMIN, TRUE, CARETLINEFRAME_SMALLEST);
			::SendMessage(::GetDlgItem(_hSelf, IDC_CARETLINEFRAME_WIDTH_SLIDER), TBM_SETRANGEMAX, TRUE, CARETLINEFRAME_LARGEST);
			::SendMessage(::GetDlgItem(_hSelf, IDC_CARETLINEFRAME_WIDTH_SLIDER), TBM_SETPAGESIZE, 0, CARETLINEFRAME_INTERVAL);
			::SendMessage(::GetDlgItem(_hSelf, IDC_CARETLINEFRAME_WIDTH_SLIDER), TBM_SETPOS, TRUE, svp._currentLineFrameWidth);
			::SetDlgItemInt(_hSelf, IDC_CARETLINEFRAME_WIDTH_DISPLAY, svp._currentLineFrameWidth, FALSE);


			// defaul =>  (svp._eolMode == svp.roundedRectangleText)
			bool checkDefaultCRLF = true;
			bool checkPlainTextCRLF = false;
			bool checkWithColorCRLF = false;

			if (svp._eolMode == svp.plainText)
			{
				checkDefaultCRLF = false;
				checkPlainTextCRLF = true;
				checkWithColorCRLF = false;
			}
			else if (svp._eolMode == svp.plainTextCustomColor)
			{
				checkDefaultCRLF = false;
				checkPlainTextCRLF = true;
				checkWithColorCRLF = true;
			}
			else if (svp._eolMode == svp.roundedRectangleTextCustomColor)
			{
				checkDefaultCRLF = true;
				checkPlainTextCRLF = false;
				checkWithColorCRLF = true;
			}
			::SendDlgItemMessage(_hSelf, IDC_RADIO_ROUNDCORNER_CRLF, BM_SETCHECK, checkDefaultCRLF, 0);
			::SendDlgItemMessage(_hSelf, IDC_RADIO_PLEINTEXT_CRLF, BM_SETCHECK, checkPlainTextCRLF, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_WITHCUSTOMCOLOR_CRLF, BM_SETCHECK, checkWithColorCRLF, 0);


			NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
			generic_string tip2show = pNativeSpeaker->getLocalizedStrFromID("eol-custom-color-tip", TEXT("Go to Style Configurator to change the default EOL custom color (\"EOL custom color\")."));

			_tip = CreateToolTip(IDC_BUTTON_LAUNCHSTYLECONF_CRLF, _hSelf, _hInst, const_cast<PTSTR>(tip2show.c_str()), pNativeSpeaker->isRTL());
			if (_tip)
			{
				SendMessage(_tip, TTM_ACTIVATE, TRUE, 0);
			}

			initScintParam();

			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			if (_tip)
			{
				NppDarkMode::setDarkTooltips(_tip, NppDarkMode::ToolTipsType::tooltip);
			}

			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORLISTBOX:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORDLG:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORSTATIC:
		{
			int dlgCtrlID = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam));

			// handle blurry text with disabled states for the affected static controls
			if (dlgCtrlID == IDC_CARETLINEFRAME_WIDTH_STATIC || dlgCtrlID == IDC_CARETLINEFRAME_WIDTH_DISPLAY)
			{
				return NppDarkMode::onCtlColorDarkerBGStaticText(reinterpret_cast<HDC>(wParam), (svp._currentLineHiliteMode == LINEHILITE_FRAME));
			}

			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_HSCROLL:
		{
			HWND hCaretBlinkRateSlider = ::GetDlgItem(_hSelf, IDC_CARETBLINKRATE_SLIDER);
			HWND hCaretLineFrameSlider = ::GetDlgItem(_hSelf, IDC_CARETLINEFRAME_WIDTH_SLIDER);

			if (reinterpret_cast<HWND>(lParam) == hCaretBlinkRateSlider)
			{
				auto blinkRate = ::SendMessage(hCaretBlinkRateSlider, TBM_GETPOS, 0, 0);
				if (blinkRate == BLINKRATE_SLOWEST)
					blinkRate = 0;
				nppGUI._caretBlinkRate = static_cast<int>(blinkRate);

				::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SETCARETBLINKRATE, 0, 0);
			}
			else if (reinterpret_cast<HWND>(lParam) == hCaretLineFrameSlider)
			{
				svp._currentLineFrameWidth = static_cast<unsigned char>(::SendMessage(hCaretLineFrameSlider, TBM_GETPOS, 0, 0));
				::SetDlgItemInt(_hSelf, IDC_CARETLINEFRAME_WIDTH_DISPLAY, svp._currentLineFrameWidth, FALSE);
				::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_CARETLINEFRAME, NULL, svp._currentLineFrameWidth);
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

				case IDC_RADIO_CLM_NONE:
					svp._currentLineHiliteMode = LINEHILITE_NONE;
					changeLineHiliteMode(false);
					return TRUE;

				case IDC_RADIO_CLM_HILITE:
					svp._currentLineHiliteMode = LINEHILITE_HILITE;
					changeLineHiliteMode(false);
					return TRUE;

				case IDC_RADIO_CLM_FRAME:
					svp._currentLineHiliteMode = LINEHILITE_FRAME;
					changeLineHiliteMode(true);
					return TRUE;

				case IDC_RADIO_ROUNDCORNER_CRLF:
				case IDC_RADIO_PLEINTEXT_CRLF:
				case IDC_CHECK_WITHCUSTOMCOLOR_CRLF:
				{
					bool doCustomColor = isCheckedOrNot(IDC_CHECK_WITHCUSTOMCOLOR_CRLF);

					if (wParam == IDC_RADIO_ROUNDCORNER_CRLF)
					{
						svp._eolMode = doCustomColor ? svp.roundedRectangleTextCustomColor : svp.roundedRectangleText;
					}
					else if (wParam == IDC_RADIO_PLEINTEXT_CRLF)
					{
						svp._eolMode = doCustomColor ? svp.plainTextCustomColor : svp.plainText;
					}
					else // IDC_CHECK_WITHCUSTOMCOLOR_CRLF
					{
						if (isCheckedOrNot(IDC_RADIO_ROUNDCORNER_CRLF))
						{
							svp._eolMode = doCustomColor ? svp.roundedRectangleTextCustomColor : svp.roundedRectangleText;
						}
						else // IDC_RADIO_PLEINTEXT_CRLF
						{
							svp._eolMode = doCustomColor ? svp.plainTextCustomColor : svp.plainText;
						}
					}

					HWND grandParent = ::GetParent(_hParent);
					::SendMessage(grandParent, NPPM_INTERNAL_CRLFFORMCHANGED, 0, 0);
					return TRUE;
				}

				case IDC_BUTTON_LAUNCHSTYLECONF_CRLF:
				{
					HWND grandParent = ::GetParent(_hParent);
					::SendMessage(grandParent, NPPM_INTERNAL_CRLFLAUNCHSTYLECONF, 0, 0);
					return TRUE;
				}

				case IDC_CHECK_VIRTUALSPACE:
					svp._virtualSpace = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_VIRTUALSPACE, BM_GETCHECK, 0, 0));
					::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_VIRTUALSPACE, 0, 0);
					return TRUE;

				case IDC_CHECK_SCROLLBEYONDLASTLINE:
					svp._scrollBeyondLastLine = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_SCROLLBEYONDLASTLINE, BM_GETCHECK, 0, 0));
					::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SCROLLBEYONDLASTLINE, 0, 0);
					return TRUE;

				case IDC_CHECK_RIGHTCLICKKEEPSSELECTION:
					svp._rightClickKeepsSelection = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_RIGHTCLICKKEEPSSELECTION, BM_GETCHECK, 0, 0));
					return TRUE;

				case IDC_CHECK_DISABLEADVANCEDSCROLL:
					svp._disableAdvancedScrolling = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_DISABLEADVANCEDSCROLL, BM_GETCHECK, 0, 0));
					return TRUE;

                case IDC_CHECK_MULTISELECTION :
                    nppGUI._enableMultiSelection = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_MULTISELECTION, BM_GETCHECK, 0, 0));
                    ::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SETMULTISELCTION, 0, 0);
                    return TRUE;

				case IDC_CHECK_FOLDINGTOGGLE:
					nppGUI._enableFoldCmdToggable = isCheckedOrNot(IDC_CHECK_FOLDINGTOGGLE);
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
					}
			}
		}
	}
	return FALSE;
}


void DarkModeSubDlg::enableCustomizedColorCtrls(bool doEnable)
{
	::EnableWindow(_pBackgroundColorPicker->getHSelf(), doEnable);
	::EnableWindow(_pSofterBackgroundColorPicker->getHSelf(), doEnable);
	::EnableWindow(_pHotBackgroundColorPicker->getHSelf(), doEnable);
	::EnableWindow(_pPureBackgroundColorPicker->getHSelf(), doEnable);
	::EnableWindow(_pErrorBackgroundColorPicker->getHSelf(), doEnable);
	::EnableWindow(_pTextColorPicker->getHSelf(), doEnable);
	::EnableWindow(_pDarkerTextColorPicker->getHSelf(), doEnable);
	::EnableWindow(_pDisabledTextColorPicker->getHSelf(), doEnable);
	::EnableWindow(_pEdgeColorPicker->getHSelf(), doEnable);
	::EnableWindow(_pLinkColorPicker->getHSelf(), doEnable);
	::EnableWindow(_pHotEdgeColorPicker->getHSelf(), doEnable);
	::EnableWindow(_pDisabledEdgeColorPicker->getHSelf(), doEnable);

	::EnableWindow(::GetDlgItem(_hSelf, IDD_CUSTOMIZED_RESET_BUTTON), doEnable);

	if (doEnable)
	{
		_pBackgroundColorPicker->setColour(NppDarkMode::getBackgroundColor());
		_pSofterBackgroundColorPicker->setColour(NppDarkMode::getSofterBackgroundColor());
		_pHotBackgroundColorPicker->setColour(NppDarkMode::getHotBackgroundColor());
		_pPureBackgroundColorPicker->setColour(NppDarkMode::getDarkerBackgroundColor());
		_pErrorBackgroundColorPicker->setColour(NppDarkMode::getErrorBackgroundColor());
		_pTextColorPicker->setColour(NppDarkMode::getTextColor());
		_pDarkerTextColorPicker->setColour(NppDarkMode::getDarkerTextColor());
		_pDisabledTextColorPicker->setColour(NppDarkMode::getDisabledTextColor());
		_pEdgeColorPicker->setColour(NppDarkMode::getEdgeColor());
		_pLinkColorPicker->setColour(NppDarkMode::getLinkTextColor());
		_pHotEdgeColorPicker->setColour(NppDarkMode::getHotEdgeColor());
		_pDisabledEdgeColorPicker->setColour(NppDarkMode::getDisabledEdgeColor());

		redraw();
	}
}

void DarkModeSubDlg::move2CtrlLeft(int ctrlID, HWND handle2Move, int handle2MoveWidth, int handle2MoveHeight)
{
	POINT p;
	RECT rc;
	::GetWindowRect(::GetDlgItem(_hSelf, ctrlID), &rc);

	NppParameters& nppParam = NppParameters::getInstance();

	if(nppParam.getNativeLangSpeaker()->isRTL())
		p.x = rc.right + nppParam._dpiManager.scaleX(5) + handle2MoveWidth;
	else
		p.x = rc.left - nppParam._dpiManager.scaleX(5) - handle2MoveWidth;

	p.y = rc.top + ((rc.bottom - rc.top) / 2) - handle2MoveHeight / 2;

	::ScreenToClient(_hSelf, &p);
	::MoveWindow(handle2Move, p.x, p.y, handle2MoveWidth, handle2MoveHeight, TRUE);
}

intptr_t CALLBACK DarkModeSubDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI& nppGUI = nppParam.getNppGUI();
	switch (message)
	{
		case WM_INITDIALOG:
		{
			::SendDlgItemMessage(_hSelf, IDC_CHECK_DARKMODE_ENABLE, BM_SETCHECK, nppGUI._darkmode._isEnabled, 0);

			int id = IDC_RADIO_DARKMODE_BLACK;
			switch (nppGUI._darkmode._colorTone)
			{
				case NppDarkMode::redTone:
					id = IDC_RADIO_DARKMODE_RED;
					break;
				case NppDarkMode::greenTone:
					id = IDC_RADIO_DARKMODE_GREEN;
					break;
				case NppDarkMode::blueTone:
					id = IDC_RADIO_DARKMODE_BLUE;
					break;
				case NppDarkMode::purpleTone:
					id = IDC_RADIO_DARKMODE_PURPLE;
					break;
				case NppDarkMode::cyanTone:
					id = IDC_RADIO_DARKMODE_CYAN;
					break;
				case NppDarkMode::oliveTone:
					id = IDC_RADIO_DARKMODE_OLIVE;
					break;
				case NppDarkMode::customizedTone:
					id = IDC_RADIO_DARKMODE_CUSTOMIZED;
					break;
			}
			::SendDlgItemMessage(_hSelf, id, BM_SETCHECK, TRUE, 0);

			_pBackgroundColorPicker = new ColourPicker;
			_pSofterBackgroundColorPicker = new ColourPicker;
			_pHotBackgroundColorPicker = new ColourPicker;
			_pPureBackgroundColorPicker = new ColourPicker;
			_pErrorBackgroundColorPicker = new ColourPicker;
			_pTextColorPicker = new ColourPicker;
			_pDarkerTextColorPicker = new ColourPicker;
			_pDisabledTextColorPicker = new ColourPicker;
			_pEdgeColorPicker = new ColourPicker;
			_pLinkColorPicker = new ColourPicker;
			_pHotEdgeColorPicker = new ColourPicker;
			_pDisabledEdgeColorPicker = new ColourPicker;

			_pBackgroundColorPicker->init(_hInst, _hSelf);
			_pSofterBackgroundColorPicker->init(_hInst, _hSelf);
			_pHotBackgroundColorPicker->init(_hInst, _hSelf);
			_pPureBackgroundColorPicker->init(_hInst, _hSelf);

			_pErrorBackgroundColorPicker->init(_hInst, _hSelf);
			_pTextColorPicker->init(_hInst, _hSelf);
			_pDarkerTextColorPicker->init(_hInst, _hSelf);
			_pDisabledTextColorPicker->init(_hInst, _hSelf);
			_pEdgeColorPicker->init(_hInst, _hSelf);
			_pLinkColorPicker->init(_hInst, _hSelf);
			_pHotEdgeColorPicker->init(_hInst, _hSelf);
			_pDisabledEdgeColorPicker->init(_hInst, _hSelf);

			int cpDynamicalWidth = NppParameters::getInstance()._dpiManager.scaleX(25);
			int cpDynamicalHeight = NppParameters::getInstance()._dpiManager.scaleY(25);

			move2CtrlLeft(IDD_CUSTOMIZED_COLOR1_STATIC, _pPureBackgroundColorPicker->getHSelf(), cpDynamicalWidth, cpDynamicalHeight);
			move2CtrlLeft(IDD_CUSTOMIZED_COLOR2_STATIC, _pHotBackgroundColorPicker->getHSelf(), cpDynamicalWidth, cpDynamicalHeight);
			move2CtrlLeft(IDD_CUSTOMIZED_COLOR3_STATIC, _pSofterBackgroundColorPicker->getHSelf(), cpDynamicalWidth, cpDynamicalHeight);
			move2CtrlLeft(IDD_CUSTOMIZED_COLOR4_STATIC, _pBackgroundColorPicker->getHSelf(), cpDynamicalWidth, cpDynamicalHeight);
			move2CtrlLeft(IDD_CUSTOMIZED_COLOR5_STATIC, _pErrorBackgroundColorPicker->getHSelf(), cpDynamicalWidth, cpDynamicalHeight);
			move2CtrlLeft(IDD_CUSTOMIZED_COLOR6_STATIC, _pTextColorPicker->getHSelf(), cpDynamicalWidth, cpDynamicalHeight);
			move2CtrlLeft(IDD_CUSTOMIZED_COLOR7_STATIC, _pDarkerTextColorPicker->getHSelf(), cpDynamicalWidth, cpDynamicalHeight);
			move2CtrlLeft(IDD_CUSTOMIZED_COLOR8_STATIC, _pDisabledTextColorPicker->getHSelf(), cpDynamicalWidth, cpDynamicalHeight);
			move2CtrlLeft(IDD_CUSTOMIZED_COLOR9_STATIC, _pEdgeColorPicker->getHSelf(), cpDynamicalWidth, cpDynamicalHeight);
			move2CtrlLeft(IDD_CUSTOMIZED_COLOR10_STATIC, _pLinkColorPicker->getHSelf(), cpDynamicalWidth, cpDynamicalHeight);
			move2CtrlLeft(IDD_CUSTOMIZED_COLOR11_STATIC, _pHotEdgeColorPicker->getHSelf(), cpDynamicalWidth, cpDynamicalHeight);
			move2CtrlLeft(IDD_CUSTOMIZED_COLOR12_STATIC, _pDisabledEdgeColorPicker->getHSelf(), cpDynamicalWidth, cpDynamicalHeight);

			_pBackgroundColorPicker->display();
			_pSofterBackgroundColorPicker->display();
			_pHotBackgroundColorPicker->display();
			_pPureBackgroundColorPicker->display();
			_pErrorBackgroundColorPicker->display();
			_pTextColorPicker->display();
			_pDarkerTextColorPicker->display();
			_pDisabledTextColorPicker->display();
			_pEdgeColorPicker->display();
			_pLinkColorPicker->display();
			_pHotEdgeColorPicker->display();
			_pDisabledEdgeColorPicker->display();

			::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_DARKMODE_BLACK), nppGUI._darkmode._isEnabled);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_DARKMODE_RED), nppGUI._darkmode._isEnabled);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_DARKMODE_GREEN), nppGUI._darkmode._isEnabled);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_DARKMODE_BLUE), nppGUI._darkmode._isEnabled);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_DARKMODE_PURPLE), nppGUI._darkmode._isEnabled);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_DARKMODE_CYAN), nppGUI._darkmode._isEnabled);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_DARKMODE_OLIVE), nppGUI._darkmode._isEnabled);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_DARKMODE_CUSTOMIZED), nppGUI._darkmode._isEnabled);

			enableCustomizedColorCtrls(nppGUI._darkmode._isEnabled && id == IDC_RADIO_DARKMODE_CUSTOMIZED);

			return TRUE;
		}

		case WM_CTLCOLORDLG:
		{
			return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORSTATIC:
		{
			auto hdcStatic = reinterpret_cast<HDC>(wParam);
			auto dlgCtrlID = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam));

			bool isStaticText = (dlgCtrlID == IDD_CUSTOMIZED_COLOR1_STATIC ||
				dlgCtrlID == IDD_CUSTOMIZED_COLOR2_STATIC ||
				dlgCtrlID == IDD_CUSTOMIZED_COLOR3_STATIC ||
				dlgCtrlID == IDD_CUSTOMIZED_COLOR4_STATIC ||
				dlgCtrlID == IDD_CUSTOMIZED_COLOR5_STATIC ||
				dlgCtrlID == IDD_CUSTOMIZED_COLOR6_STATIC ||
				dlgCtrlID == IDD_CUSTOMIZED_COLOR7_STATIC ||
				dlgCtrlID == IDD_CUSTOMIZED_COLOR8_STATIC ||
				dlgCtrlID == IDD_CUSTOMIZED_COLOR9_STATIC ||
				dlgCtrlID == IDD_CUSTOMIZED_COLOR10_STATIC ||
				dlgCtrlID == IDD_CUSTOMIZED_COLOR11_STATIC ||
				dlgCtrlID == IDD_CUSTOMIZED_COLOR12_STATIC);
			//set the static text colors to show enable/disable instead of ::EnableWindow which causes blurry text
			if (isStaticText)
			{
				bool isTextEnabled = nppGUI._darkmode._isEnabled && nppGUI._darkmode._colorTone == NppDarkMode::customizedTone;
				return NppDarkMode::onCtlColorDarkerBGStaticText(hdcStatic, isTextEnabled);
			}

			return NppDarkMode::onCtlColorDarker(hdcStatic);
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_DESTROY:
		{
			_pBackgroundColorPicker->destroy();
			_pSofterBackgroundColorPicker->destroy();
			_pHotBackgroundColorPicker->destroy();
			_pPureBackgroundColorPicker->destroy();
			_pErrorBackgroundColorPicker->destroy();
			_pTextColorPicker->destroy();
			_pDarkerTextColorPicker->destroy();
			_pDisabledTextColorPicker->destroy();
			_pEdgeColorPicker->destroy();
			_pLinkColorPicker->destroy();
			_pHotEdgeColorPicker->destroy();
			_pDisabledEdgeColorPicker->destroy();

			delete _pBackgroundColorPicker;
			delete _pSofterBackgroundColorPicker;
			delete _pHotBackgroundColorPicker;
			delete _pPureBackgroundColorPicker;
			delete _pErrorBackgroundColorPicker;
			delete _pTextColorPicker;
			delete _pDarkerTextColorPicker;
			delete _pDisabledTextColorPicker;
			delete _pEdgeColorPicker;
			delete _pLinkColorPicker;
			delete _pHotEdgeColorPicker;
			delete _pDisabledEdgeColorPicker;
		}

		case WM_COMMAND:
		{
			bool changed = false;
			bool forceRefresh = false;
			bool doEnableCustomizedColorCtrls = false;
			switch (wParam)
			{
				case IDC_CHECK_DARKMODE_ENABLE:
				{
					bool enableDarkMode = isCheckedOrNot(static_cast<int>(wParam));
					nppGUI._darkmode._isEnabled = enableDarkMode;

					::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_DARKMODE_BLACK), enableDarkMode);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_DARKMODE_RED), enableDarkMode);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_DARKMODE_GREEN), enableDarkMode);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_DARKMODE_BLUE), enableDarkMode);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_DARKMODE_PURPLE), enableDarkMode);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_DARKMODE_CYAN), enableDarkMode);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_DARKMODE_OLIVE), enableDarkMode);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_DARKMODE_CUSTOMIZED), enableDarkMode);

					doEnableCustomizedColorCtrls = enableDarkMode && nppGUI._darkmode._colorTone == NppDarkMode::customizedTone;
					enableCustomizedColorCtrls(doEnableCustomizedColorCtrls);

					// Maintain the coherence in preferences
					if (nppGUI._darkmode._isEnabled)
					{
						// For toolbar: if dark mode enabled & TB_STANDARD is selected, switch to TB_SMALL
						bool isStandardChecked = false;
						::SendMessage(_hParent, PREF_MSG_ISCHECKED_GENERALPAGE, IDC_RADIO_STANDARD, LPARAM(&isStandardChecked));
						if (isStandardChecked)
							::SendMessage(_hParent, PREF_MSG_SETTOOLICONSFROMSTDTOSMALL, 0, 0);

						// For tabbar: uncheck Alternate icons checkbox
						::SendMessage(_hParent, PREF_MSG_DISABLETABBARALTERNATEICONS, 0, 0);
					}

					changed = true;
				}
				break;

				case IDC_RADIO_DARKMODE_BLACK:
				case IDC_RADIO_DARKMODE_RED:
				case IDC_RADIO_DARKMODE_GREEN:
				case IDC_RADIO_DARKMODE_BLUE:
				case IDC_RADIO_DARKMODE_PURPLE:
				case IDC_RADIO_DARKMODE_CYAN:
				case IDC_RADIO_DARKMODE_OLIVE:
				case IDC_RADIO_DARKMODE_CUSTOMIZED:
				case IDD_CUSTOMIZED_RESET_BUTTON:
					if (wParam == IDC_RADIO_DARKMODE_BLACK)
					{
						if (nppGUI._darkmode._colorTone == NppDarkMode::blackTone)
							return TRUE;
						nppGUI._darkmode._colorTone = NppDarkMode::blackTone;
					}
					else if (wParam == IDC_RADIO_DARKMODE_RED)
					{
						if (nppGUI._darkmode._colorTone == NppDarkMode::redTone)
							return TRUE;
						nppGUI._darkmode._colorTone = NppDarkMode::redTone;
					}
					else if (wParam == IDC_RADIO_DARKMODE_GREEN)
					{
						if (nppGUI._darkmode._colorTone == NppDarkMode::greenTone)
							return TRUE;
						nppGUI._darkmode._colorTone = NppDarkMode::greenTone;
					}
					else if (wParam == IDC_RADIO_DARKMODE_BLUE)
					{
						if (nppGUI._darkmode._colorTone == NppDarkMode::blueTone)
							return TRUE;
						nppGUI._darkmode._colorTone = NppDarkMode::blueTone;
					}
					else if (wParam == IDC_RADIO_DARKMODE_PURPLE)
					{
						if (nppGUI._darkmode._colorTone == NppDarkMode::purpleTone)
							return TRUE;
						nppGUI._darkmode._colorTone = NppDarkMode::purpleTone;
					}
					else if (wParam == IDC_RADIO_DARKMODE_CYAN)
					{
						if (nppGUI._darkmode._colorTone == NppDarkMode::cyanTone)
							return TRUE;
						nppGUI._darkmode._colorTone = NppDarkMode::cyanTone;
					}
					else if (wParam == IDC_RADIO_DARKMODE_OLIVE)
					{
						if (nppGUI._darkmode._colorTone == NppDarkMode::oliveTone)
							return TRUE;
						nppGUI._darkmode._colorTone = NppDarkMode::oliveTone;
					}
					else if (wParam == IDC_RADIO_DARKMODE_CUSTOMIZED)
					{
						if (nppGUI._darkmode._colorTone == NppDarkMode::customizedTone)
							return TRUE;
						nppGUI._darkmode._colorTone = NppDarkMode::customizedTone;
						doEnableCustomizedColorCtrls = true;
					}
					
					else if (wParam == IDD_CUSTOMIZED_RESET_BUTTON)
					{
						nppGUI._darkmode._customColors = NppDarkMode::getDarkModeDefaultColors();
						NppDarkMode::changeCustomTheme(nppGUI._darkmode._customColors);
						doEnableCustomizedColorCtrls = true;
					}


					// switch to chosen dark mode
					nppGUI._darkmode._isEnabled = true;
					NppDarkMode::setDarkTone(nppGUI._darkmode._colorTone);
					changed = true;
					forceRefresh = true;

					enableCustomizedColorCtrls(doEnableCustomizedColorCtrls);
					break;

				default:
					switch (HIWORD(wParam))
					{
						case CPN_COLOURPICKED:
						{
							COLORREF c;
							if (reinterpret_cast<HWND>(lParam) == _pBackgroundColorPicker->getHSelf())
							{
								c = _pBackgroundColorPicker->getColour();
								NppDarkMode::setBackgroundColor(c);
								nppGUI._darkmode._customColors.background = c;
							}
							else if (reinterpret_cast<HWND>(lParam) == _pSofterBackgroundColorPicker->getHSelf())
							{
								c = _pSofterBackgroundColorPicker->getColour();
								NppDarkMode::setSofterBackgroundColor(c);
								nppGUI._darkmode._customColors.softerBackground = c;
							}
							else if (reinterpret_cast<HWND>(lParam) == _pHotBackgroundColorPicker->getHSelf())
							{
								c = _pHotBackgroundColorPicker->getColour();
								NppDarkMode::setHotBackgroundColor(c);
								nppGUI._darkmode._customColors.hotBackground = c;
							}
							else if (reinterpret_cast<HWND>(lParam) == _pPureBackgroundColorPicker->getHSelf())
							{
								c = _pPureBackgroundColorPicker->getColour();
								NppDarkMode::setDarkerBackgroundColor(c);
								nppGUI._darkmode._customColors.pureBackground = c;
							}
							else if (reinterpret_cast<HWND>(lParam) == _pErrorBackgroundColorPicker->getHSelf())
							{
								c = _pErrorBackgroundColorPicker->getColour();
								NppDarkMode::setErrorBackgroundColor(c);
								nppGUI._darkmode._customColors.errorBackground = c;
							}
							else if (reinterpret_cast<HWND>(lParam) == _pTextColorPicker->getHSelf())
							{
								c = _pTextColorPicker->getColour();
								NppDarkMode::setTextColor(c);
								nppGUI._darkmode._customColors.text = c;
							}
							else if (reinterpret_cast<HWND>(lParam) == _pDarkerTextColorPicker->getHSelf())
							{
								c = _pDarkerTextColorPicker->getColour();
								NppDarkMode::setDarkerTextColor(c);
								nppGUI._darkmode._customColors.darkerText = c;
							}
							else if (reinterpret_cast<HWND>(lParam) == _pDisabledTextColorPicker->getHSelf())
							{
								c = _pDisabledTextColorPicker->getColour();
								NppDarkMode::setDisabledTextColor(c);
								nppGUI._darkmode._customColors.disabledText = c;
							}
							else if (reinterpret_cast<HWND>(lParam) == _pEdgeColorPicker->getHSelf())
							{
								c = _pEdgeColorPicker->getColour();
								NppDarkMode::setEdgeColor(c);
								nppGUI._darkmode._customColors.edge = c;
							}
							else if (reinterpret_cast<HWND>(lParam) == _pLinkColorPicker->getHSelf())
							{
								c = _pLinkColorPicker->getColour();
								NppDarkMode::setLinkTextColor(c);
								nppGUI._darkmode._customColors.linkText = c;
							}
							else if (reinterpret_cast<HWND>(lParam) == _pHotEdgeColorPicker->getHSelf())
							{
								c = _pHotEdgeColorPicker->getColour();
								NppDarkMode::setHotEdgeColor(c);
								nppGUI._darkmode._customColors.hotEdge = c;
							}
							else if (reinterpret_cast<HWND>(lParam) == _pDisabledEdgeColorPicker->getHSelf())
							{
								c = _pDisabledEdgeColorPicker->getColour();
								NppDarkMode::setDisabledEdgeColor(c);
								nppGUI._darkmode._customColors.disabledEdge = c;
							}
							else
							{
								return FALSE;
							}

							nppGUI._darkmode._isEnabled = true;
							NppDarkMode::setDarkTone(nppGUI._darkmode._colorTone);
							changed = true;
							forceRefresh = true;
							doEnableCustomizedColorCtrls = true;
						}
						break;

						default:
						{
							return FALSE;
						}
					}
			}

			if (changed)
			{
				if (!doEnableCustomizedColorCtrls)
				{
					COLORREF disabledColor = nppGUI._darkmode._isEnabled ? NppDarkMode::getDarkerBackgroundColor() : ::GetSysColor(COLOR_3DFACE);

					_pBackgroundColorPicker->setColour(disabledColor);
					_pSofterBackgroundColorPicker->setColour(disabledColor);
					_pHotBackgroundColorPicker->setColour(disabledColor);
					_pPureBackgroundColorPicker->setColour(disabledColor);
					_pErrorBackgroundColorPicker->setColour(disabledColor);
					_pTextColorPicker->setColour(disabledColor);
					_pDarkerTextColorPicker->setColour(disabledColor);
					_pDisabledTextColorPicker->setColour(disabledColor);
					_pEdgeColorPicker->setColour(disabledColor);
					_pLinkColorPicker->setColour(disabledColor);
					_pHotEdgeColorPicker->setColour(disabledColor);
					_pDisabledEdgeColorPicker->setColour(disabledColor);

					redraw();
				}

				NppDarkMode::refreshDarkMode(_hSelf, forceRefresh);
				getFocus(); // to make black mode title bar appear
				return TRUE;
			}

			return FALSE;
		}
	}
	return FALSE;
}

void MarginsBorderEdgeSubDlg::initScintParam()
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
	
	::SendDlgItemMessage(_hSelf, IDC_CHECK_LINENUMBERMARGE, BM_SETCHECK, svp._lineNumberMarginShow, 0);
	::SendDlgItemMessage(_hSelf, IDC_RADIO_DYNAMIC, BM_SETCHECK, svp._lineNumberMarginDynamicWidth, 0);
	::SendDlgItemMessage(_hSelf, IDC_RADIO_CONSTANT, BM_SETCHECK, !svp._lineNumberMarginDynamicWidth, 0);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_DYNAMIC), svp._lineNumberMarginShow);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_CONSTANT), svp._lineNumberMarginShow);

	::SendDlgItemMessage(_hSelf, IDC_CHECK_BOOKMARKMARGE, BM_SETCHECK, svp._bookMarkMarginShow, 0);
	
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

intptr_t CALLBACK MarginsBorderEdgeSubDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	NppParameters& nppParam = NppParameters::getInstance();
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			const ScintillaViewParams & svp = nppParam.getSVP();
			::SendMessage(::GetDlgItem(_hSelf, IDC_BORDERWIDTH_SLIDER),TBM_SETRANGEMIN, TRUE, BORDERWIDTH_SMALLEST);
			::SendMessage(::GetDlgItem(_hSelf, IDC_BORDERWIDTH_SLIDER),TBM_SETRANGEMAX, TRUE, BORDERWIDTH_LARGEST);
			::SendMessage(::GetDlgItem(_hSelf, IDC_BORDERWIDTH_SLIDER),TBM_SETPAGESIZE, 0, BORDERWIDTH_INTERVAL);
			::SendMessage(::GetDlgItem(_hSelf, IDC_BORDERWIDTH_SLIDER),TBM_SETPOS, TRUE, svp._borderWidth);
			::SetDlgItemInt(_hSelf, IDC_BORDERWIDTHVAL_STATIC, svp._borderWidth, FALSE);

			::SendMessage(::GetDlgItem(_hSelf, IDC_PADDINGLEFT_SLIDER), TBM_SETRANGEMIN, TRUE, PADDING_SMALLEST);
			::SendMessage(::GetDlgItem(_hSelf, IDC_PADDINGLEFT_SLIDER), TBM_SETRANGEMAX, TRUE, PADDING_LARGEST);
			::SendMessage(::GetDlgItem(_hSelf, IDC_PADDINGLEFT_SLIDER), TBM_SETPAGESIZE, 0, PADDING_INTERVAL);
			::SendMessage(::GetDlgItem(_hSelf, IDC_PADDINGLEFT_SLIDER), TBM_SETPOS, TRUE, svp._paddingLeft);
			::SetDlgItemInt(_hSelf, IDC_PADDINGLEFTVAL_STATIC, svp._paddingLeft, FALSE);

			::SendMessage(::GetDlgItem(_hSelf, IDC_PADDINGRIGHT_SLIDER), TBM_SETRANGEMIN, TRUE, PADDING_SMALLEST);
			::SendMessage(::GetDlgItem(_hSelf, IDC_PADDINGRIGHT_SLIDER), TBM_SETRANGEMAX, TRUE, PADDING_LARGEST);
			::SendMessage(::GetDlgItem(_hSelf, IDC_PADDINGRIGHT_SLIDER), TBM_SETPAGESIZE, 0, PADDING_INTERVAL);
			::SendMessage(::GetDlgItem(_hSelf, IDC_PADDINGRIGHT_SLIDER), TBM_SETPOS, TRUE, svp._paddingRight);
			::SetDlgItemInt(_hSelf, IDC_PADDINGRIGHTVAL_STATIC, svp._paddingRight, FALSE);

			::SendMessage(::GetDlgItem(_hSelf, IDC_DISTRACTIONFREE_SLIDER), TBM_SETRANGEMIN, TRUE, DISTRACTIONFREE_SMALLEST);
			::SendMessage(::GetDlgItem(_hSelf, IDC_DISTRACTIONFREE_SLIDER), TBM_SETRANGEMAX, TRUE, DISTRACTIONFREE_LARGEST);
			::SendMessage(::GetDlgItem(_hSelf, IDC_DISTRACTIONFREE_SLIDER), TBM_SETPAGESIZE, 0, DISTRACTIONFREE_INTERVAL);
			::SendMessage(::GetDlgItem(_hSelf, IDC_DISTRACTIONFREE_SLIDER), TBM_SETPOS, TRUE, svp._distractionFreeDivPart);
			::SetDlgItemInt(_hSelf, IDC_DISTRACTIONFREEVAL_STATIC, svp._distractionFreeDivPart, FALSE);

			initScintParam();

			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_HSCROLL:
		{
			ScintillaViewParams & svp = (ScintillaViewParams &)nppParam.getSVP();
			HWND hBorderWidthSlider = ::GetDlgItem(_hSelf, IDC_BORDERWIDTH_SLIDER);
			HWND hPaddingLeftSlider = ::GetDlgItem(_hSelf, IDC_PADDINGLEFT_SLIDER);
			HWND hPaddingRightSlider = ::GetDlgItem(_hSelf, IDC_PADDINGRIGHT_SLIDER);
			HWND hDistractionFreeSlider = ::GetDlgItem(_hSelf, IDC_DISTRACTIONFREE_SLIDER);
			if (reinterpret_cast<HWND>(lParam) == hBorderWidthSlider)
			{
				auto borderWidth = ::SendMessage(hBorderWidthSlider, TBM_GETPOS, 0, 0);
				svp._borderWidth = static_cast<int>(borderWidth);
				::SetDlgItemInt(_hSelf, IDC_BORDERWIDTHVAL_STATIC, static_cast<UINT>(borderWidth), FALSE);
				::SendMessage(::GetParent(_hParent), WM_SIZE, 0, 0);
			}
			else if (reinterpret_cast<HWND>(lParam) == hPaddingLeftSlider)
			{
				svp._paddingLeft = static_cast<unsigned char>(::SendMessage(hPaddingLeftSlider, TBM_GETPOS, 0, 0));
				::SetDlgItemInt(_hSelf, IDC_PADDINGLEFTVAL_STATIC, static_cast<UINT>(svp._paddingLeft), FALSE);
				::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_UPDATETEXTZONEPADDING, 0, 0);

			}
			else if (reinterpret_cast<HWND>(lParam) == hPaddingRightSlider)
			{
				svp._paddingRight = static_cast<unsigned char>(::SendMessage(hPaddingRightSlider, TBM_GETPOS, 0, 0));
				::SetDlgItemInt(_hSelf, IDC_PADDINGRIGHTVAL_STATIC, static_cast<UINT>(svp._paddingRight), FALSE);
				::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_UPDATETEXTZONEPADDING, 0, 0);
			}
			else if (reinterpret_cast<HWND>(lParam) == hDistractionFreeSlider)
			{
				svp._distractionFreeDivPart = static_cast<unsigned char>(::SendMessage(hDistractionFreeSlider, TBM_GETPOS, 0, 0));
				::SetDlgItemInt(_hSelf, IDC_DISTRACTIONFREEVAL_STATIC, static_cast<UINT>(svp._distractionFreeDivPart), FALSE);
				::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_UPDATETEXTZONEPADDING, 0, 0);
			}
			return 0;	//return zero when handled
		}

		case WM_COMMAND : 
		{
			ScintillaViewParams & svp = const_cast<ScintillaViewParams &>(nppParam.getSVP());
			switch (wParam)
			{
				case IDC_CHECK_LINENUMBERMARGE:
					svp._lineNumberMarginShow = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_LINENUMBERMARGE, BM_GETCHECK, 0, 0));
					::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_DYNAMIC), svp._lineNumberMarginShow);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_RADIO_CONSTANT), svp._lineNumberMarginShow);
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_LINENUMBER, 0);
					return TRUE;
				case IDC_RADIO_DYNAMIC:
					svp._lineNumberMarginDynamicWidth = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_RADIO_DYNAMIC, BM_GETCHECK, 0, 0));
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_LINENUMBER, 0);
					return TRUE;
				case IDC_RADIO_CONSTANT:
					svp._lineNumberMarginDynamicWidth = !(BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_RADIO_CONSTANT, BM_GETCHECK, 0, 0));
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_LINENUMBER, 0);
					return TRUE;
				
				case IDC_CHECK_BOOKMARKMARGE:
					svp._bookMarkMarginShow = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_BOOKMARKMARGE, BM_GETCHECK, 0, 0));
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_SYMBOLMARGIN, 0);
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

				default :
					switch (HIWORD(wParam))
					{
						case EN_CHANGE :
						{
							if (LOWORD(wParam) == IDC_COLUMNPOS_EDIT)
							{
								TCHAR text[MAX_PATH] = {'\0'};
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
intptr_t CALLBACK MiscSubDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI & nppGUI = nppParam.getNppGUI();

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
			::SendDlgItemMessage(_hSelf, IDC_CHECK_SAVEALLCONFIRM, BM_SETCHECK, nppGUI._saveAllConfirm, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_AUTOUPDATE, BM_SETCHECK, nppGUI._autoUpdateOpt._doAutoUpdate, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_DIRECTWRITE_ENABLE, BM_SETCHECK, nppGUI._writeTechnologyEngine == directWriteTechnology, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_ENABLEDOCPEEKER, BM_SETCHECK, nppGUI._isDocPeekOnTab ? BST_CHECKED : BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_ENABLEDOCPEEKONMAP, BM_SETCHECK, nppGUI._isDocPeekOnMap ? BST_CHECKED : BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_MUTE_SOUNDS, BM_SETCHECK, nppGUI._muteSounds ? BST_CHECKED : BST_UNCHECKED, 0);

			::ShowWindow(::GetDlgItem(_hSelf, IDC_CHECK_AUTOUPDATE), nppGUI._doesExistUpdater?SW_SHOW:SW_HIDE);

			::SendDlgItemMessage(_hSelf, IDC_EDIT_SESSIONFILEEXT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(nppGUI._definedSessionExt.c_str()));
			::SendDlgItemMessage(_hSelf, IDC_EDIT_WORKSPACEFILEEXT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(nppGUI._definedWorkspaceExt.c_str()));
			
			::SendDlgItemMessage(_hSelf, IDC_CHECK_ENABLEDOCSWITCHER, BM_SETCHECK, nppGUI._doTaskList, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_STYLEMRU, BM_SETCHECK, nppGUI._styleMRU, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_SHORTTITLE, BM_SETCHECK, nppGUI._shortTitlebar, 0);

			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORLISTBOX:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
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

				case IDC_CHECK_MUTE_SOUNDS:
				{
					nppGUI._muteSounds = isCheckedOrNot(IDC_CHECK_MUTE_SOUNDS);
					return TRUE;
				}
				
				case IDC_CHECK_SAVEALLCONFIRM:
				{
					nppGUI._saveAllConfirm = isCheckedOrNot(IDC_CHECK_SAVEALLCONFIRM);
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


void RecentFilesHistorySubDlg::setCustomLen(int val)
{
	::EnableWindow(::GetDlgItem(_hSelf, IDC_CUSTOMIZELENGTHVAL_STATIC), val > 0);
	::SetDlgItemInt(_hSelf, IDC_CUSTOMIZELENGTHVAL_STATIC, val, FALSE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_CUSTOMIZELENGTHVAL_STATIC), val > 0?SW_SHOW:SW_HIDE);
}

intptr_t CALLBACK NewDocumentSubDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
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
					ID2Check = IDC_RADIO_UTF16BIG;
					break;
				case uni16LE :
					ID2Check = IDC_RADIO_UTF16SMALL;
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

			for (int i = L_TEXT + 1 ; i < nppParam.L_END ; ++i) // Skip L_TEXT
			{
				LangType lt = static_cast<LangType>(i);
				str.clear();
				if (lt != L_USER && lt != L_JS)
				{
					int cmdID = nppParam.langTypeToCommandID(lt);
					if ((cmdID != -1))
					{
						getNameStrFromCmd(cmdID, str);
						if (str.length() > 0)
						{
							size_t index = ::SendDlgItemMessage(_hSelf, IDC_COMBO_DEFAULTLANG, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(str.c_str()));
							::SendDlgItemMessage(_hSelf, IDC_COMBO_DEFAULTLANG, CB_SETITEMDATA, index, lt);
						}
					}
				}
			}

			// Insert L_TEXT to the 1st position
			int normalTextCmdID = nppParam.langTypeToCommandID(L_TEXT);
			getNameStrFromCmd(normalTextCmdID, str);
			::SendDlgItemMessage(_hSelf, IDC_COMBO_DEFAULTLANG, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(str.c_str()));

			// Set chosen language
			LangType l = L_TEXT;
			size_t cbCount = ::SendDlgItemMessage(_hSelf, IDC_COMBO_DEFAULTLANG, CB_GETCOUNT, 0, 0);
			size_t j = 0;
			for (; j < cbCount; ++j)
			{
				l = static_cast<LangType>(::SendDlgItemMessage(_hSelf, IDC_COMBO_DEFAULTLANG, CB_GETITEMDATA, j, 0));
				if (ndds._lang == l)
					break;
			}
			::SendDlgItemMessage(_hSelf, IDC_COMBO_DEFAULTLANG, CB_SETCURSEL, j, 0);
		}

		case WM_CTLCOLORLISTBOX:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_COMMAND:
			switch (wParam)
			{
				case IDC_RADIO_UTF16BIG:
					ndds._unicodeMode = uni16BE;
					ndds._openAnsiAsUtf8 = false;
					makeOpenAnsiAsUtf8(false);
					ndds._codepage = -1;
					::EnableWindow(::GetDlgItem(_hSelf, IDC_COMBO_OTHERCP), false);
					return TRUE;
				case IDC_RADIO_UTF16SMALL:
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
							ndds._lang = static_cast<LangType>(::SendDlgItemMessage(_hSelf, IDC_COMBO_DEFAULTLANG, CB_GETITEMDATA, index, 0));
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

intptr_t CALLBACK DefaultDirectorySubDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
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

			::SendDlgItemMessage(_hSelf, IDC_OPENSAVEDIR_CHECK_DRROPFOLDEROPENFILES, BM_SETCHECK, nppGUI._isFolderDroppedOpenFiles ? BST_CHECKED : BST_UNCHECKED, 0);
		}

		case WM_CTLCOLOREDIT:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				switch (LOWORD(wParam))
				{
					case  IDC_OPENSAVEDIR_ALWAYSON_EDIT:
					{
						TCHAR inputDir[MAX_PATH] = { '\0' };
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
					::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_REFRESHWORKDIR, 0, 0);
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
					::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_REFRESHWORKDIR, 0, 0);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_OPENSAVEDIR_ALWAYSON_EDIT), true);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_OPENSAVEDIR_ALWAYSON_BROWSE_BUTTON), true);
					return TRUE;

				case IDD_OPENSAVEDIR_ALWAYSON_BROWSE_BUTTON :
					{
						generic_string title = nppParam.getNativeLangSpeaker()->getLocalizedStrFromID("default-open-save-select-folder",
							TEXT("Select a folder as default directory"));
						folderBrowser(_hSelf, title, IDC_OPENSAVEDIR_ALWAYSON_EDIT);
					}
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

intptr_t CALLBACK RecentFilesHistorySubDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
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
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
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

intptr_t CALLBACK LanguageSubDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI & nppGUI = nppParam.getNppGUI();
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
			::SendDlgItemMessage(_hSelf, IDC_LIST_TABSETTNG, LB_SETCURSEL, index2Begin, 0);
			::ShowWindow(::GetDlgItem(_hSelf, IDC_GR_TABVALUE_STATIC), SW_HIDE);
			::ShowWindow(::GetDlgItem(_hSelf, IDC_CHECK_DEFAULTTABVALUE), SW_HIDE);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_DISABLE_STATIC), FALSE);
			::ShowWindow(::GetDlgItem(_hSelf, IDC_TABSIZEVAL_DISABLE_STATIC), SW_HIDE);

			::SendDlgItemMessage(_hSelf, IDC_CHECK_BACKSLASHISESCAPECHARACTERFORSQL, BM_SETCHECK, nppGUI._backSlashIsEscapeCharacterForSql, 0);

			return TRUE;
		}

		case WM_CTLCOLORLISTBOX:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorListbox(wParam, lParam);
			}
			break;
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
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
					if (static_cast<intptr_t>(iRemove) == -1)
						return TRUE;

					const size_t sL = 31;
					TCHAR s[sL + 1];
					auto lbTextLen = ::SendDlgItemMessage(_hSelf, list2Remove, LB_GETTEXTLEN, iRemove, 0);
					if (static_cast<size_t>(lbTextLen) > sL)
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

intptr_t CALLBACK HighlightingSubDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM/* lParam*/)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )nppParam.getNppGUI();

	switch (message) 
	{
		case WM_INITDIALOG :
		{
			::SendDlgItemMessage(_hSelf, IDC_CHECK_MARKALLCASESENSITIVE, BM_SETCHECK, nppGUI._markAllCaseSensitive, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_MARKALLWHOLEWORDONLY, BM_SETCHECK, nppGUI._markAllWordOnly, 0);

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

			return TRUE;
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDC_CHECK_MARKALLCASESENSITIVE:
				{
					nppGUI._markAllCaseSensitive = isCheckedOrNot(IDC_CHECK_MARKALLCASESENSITIVE);
					HWND grandParent = ::GetParent(_hParent);
					::SendMessage(grandParent, NPPM_INTERNAL_CLEARINDICATOR, 0, 0);
					return TRUE;
				}

				case IDC_CHECK_MARKALLWHOLEWORDONLY:
				{
					nppGUI._markAllWordOnly = isCheckedOrNot(IDC_CHECK_MARKALLWHOLEWORDONLY);
					HWND grandParent = ::GetParent(_hParent);
					::SendMessage(grandParent, NPPM_INTERNAL_CLEARINDICATOR, 0, 0);
					return TRUE;
				}

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

intptr_t CALLBACK PrintSubDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
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
			for (int i = 6 ; i < 15 ; ++i)
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

			::SendDlgItemMessage(_hSelf, IDC_CHECK_HBOLD, BM_SETCHECK, nppGUI._printSettings._headerFontStyle & FONTSTYLE_BOLD, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_HITALIC, BM_SETCHECK, nppGUI._printSettings._headerFontStyle & FONTSTYLE_ITALIC, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_FBOLD, BM_SETCHECK, nppGUI._printSettings._footerFontStyle & FONTSTYLE_BOLD, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_FITALIC, BM_SETCHECK, nppGUI._printSettings._footerFontStyle & FONTSTYLE_ITALIC, 0);

			::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("Full file name path")));
			::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("File name")));
			::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("File directory")));
			::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("Page")));
			::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("Short date format")));
			::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("Long date format")));
			::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("Time")));

			varList.push_back(TEXT("$(FULL_CURRENT_PATH)"));
			varList.push_back(TEXT("$(FILE_NAME)"));
			varList.push_back(TEXT("$(CURRENT_DIRECTORY)"));
			varList.push_back(TEXT("$(CURRENT_PRINTING_PAGE)"));
			varList.push_back(TEXT("$(SHORT_DATE)"));
			varList.push_back(TEXT("$(LONG_DATE)"));
			varList.push_back(TEXT("$(TIME)"));

			::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_SETCURSEL, 0, 0);

			break;
		}

		case WM_CTLCOLOREDIT:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORLISTBOX:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
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
						if (static_cast<size_t>(lbTextLen) >= intStrLen)
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

						size_t iSel = ::SendDlgItemMessage(_hSelf, IDC_COMBO_VARLIST, CB_GETCURSEL, 0, 0);
						if (iSel >= varList.size())
							return TRUE;

						TCHAR *varStr = (TCHAR*)varList[iSel].c_str();
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


intptr_t CALLBACK BackupSubDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI & nppGUI = nppParam.getNppGUI();
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

		case WM_CTLCOLOREDIT:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORDLG:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORSTATIC:
		{
			auto hdcStatic = reinterpret_cast<HDC>(wParam);
			auto dlgCtrlID = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam));

			bool isStaticText = (dlgCtrlID == IDD_BACKUPDIR_RESTORESESSION_STATIC1 ||
				dlgCtrlID == IDD_BACKUPDIR_RESTORESESSION_STATIC2 ||
				dlgCtrlID == IDD_BACKUPDIR_RESTORESESSION_PATHLABEL_STATIC);
			//set the static text colors to show enable/disable instead of ::EnableWindow which causes blurry text
			if (isStaticText)
			{
				bool isTextEnabled = isCheckedOrNot(IDC_BACKUPDIR_RESTORESESSION_CHECK);
				return NppDarkMode::onCtlColorDarkerBGStaticText(hdcStatic, isTextEnabled);
			}

			if (dlgCtrlID == IDD_BACKUPDIR_STATIC)
			{
				bool isTextEnabled = !isCheckedOrNot(IDC_RADIO_BKNONE) && isCheckedOrNot(IDC_BACKUPDIR_CHECK);
				return NppDarkMode::onCtlColorDarkerBGStaticText(hdcStatic, isTextEnabled);
			}

			if (NppDarkMode::isEnabled())
			{
				if (dlgCtrlID == IDD_BACKUPDIR_RESTORESESSION_PATH_EDIT)
				{
					return NppDarkMode::onCtlColor(hdcStatic);
				}
				return NppDarkMode::onCtlColorDarker(hdcStatic);
			}
			return FALSE;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				switch (LOWORD(wParam))
				{
					case  IDC_BACKUPDIR_EDIT:
					{
						TCHAR inputDir[MAX_PATH] = {'\0'};
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
					generic_string title = nppParam.getNativeLangSpeaker()->getLocalizedStrFromID("backup-select-folder",
						TEXT("Select a folder as backup directory"));
					folderBrowser(_hSelf, title, IDC_BACKUPDIR_EDIT);
					return TRUE;
				}


				default :
					return FALSE;
			}
			
		}
	}
	return FALSE;
}

void BackupSubDlg::updateBackupGUI()
{
	bool rememberSession = isCheckedOrNot(IDC_CHECK_REMEMBERSESSION);
	bool isSnapshot = isCheckedOrNot(IDC_BACKUPDIR_RESTORESESSION_CHECK);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_BACKUPDIR_RESTORESESSION_CHECK), rememberSession);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_BACKUPDIR_RESTORESESSION_EDIT), isSnapshot);
	::EnableWindow(::GetDlgItem(_hSelf, IDD_BACKUPDIR_RESTORESESSION_PATH_EDIT), isSnapshot);

	bool noBackup = BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_RADIO_BKNONE, BM_GETCHECK, 0, 0);
	bool isEnableGlobableCheck = false;
	bool isEnableLocalCheck = false;

	if (!noBackup)
	{
		isEnableGlobableCheck = true;
		isEnableLocalCheck = BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_BACKUPDIR_CHECK, BM_GETCHECK, 0, 0);
	}
	//::EnableWindow(::GetDlgItem(_hSelf, IDC_BACKUPDIR_USERCUSTOMDIR_GRPSTATIC), isEnableGlobableCheck);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_BACKUPDIR_CHECK), isEnableGlobableCheck);

	::EnableWindow(::GetDlgItem(_hSelf, IDC_BACKUPDIR_EDIT), isEnableLocalCheck);
	::EnableWindow(::GetDlgItem(_hSelf, IDD_BACKUPDIR_BROWSE_BUTTON), isEnableLocalCheck);

	redraw();
}


intptr_t CALLBACK AutoCompletionSubDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI & nppGUI = nppParam.getNppGUI();
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
				
			::SendDlgItemMessage(_hSelf, IDD_AUTOC_USEENTER, BM_SETCHECK, nppGUI._autocInsertSelectedUseENTER ? BST_CHECKED : BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDD_AUTOC_USETAB, BM_SETCHECK, nppGUI._autocInsertSelectedUseTAB ? BST_CHECKED : BST_UNCHECKED, 0);


			if (!isEnableAutoC)
			{
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_FUNCRADIO), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_WORDRADIO), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_BOTHRADIO), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_USEENTER), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_USETAB), FALSE);
				::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_IGNORENUMBERS), FALSE);
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

		case WM_CTLCOLOREDIT:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORDLG:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORSTATIC:
		{
			auto hdcStatic = reinterpret_cast<HDC>(wParam);
			auto dlgCtrlID = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam));

			bool isStaticText = (dlgCtrlID == IDD_AUTOC_STATIC_FROM ||
				dlgCtrlID == IDD_AUTOC_STATIC_CHAR ||
				dlgCtrlID == IDD_AUTOC_STATIC_NOTE);
			//set the static text colors to show enable/disable instead of ::EnableWindow which causes blurry text
			if (isStaticText)
			{
				bool isTextEnabled = isCheckedOrNot(IDD_AUTOC_ENABLECHECK);
				auto result = NppDarkMode::onCtlColorDarkerBGStaticText(hdcStatic, isTextEnabled);
				_nbCharVal.display(isTextEnabled);
				return result;
			}

			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(hdcStatic);
			}
			return FALSE;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
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
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_USEKEY_GRP_STATIC), isEnableAutoC);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_USEENTER), isEnableAutoC);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_USETAB), isEnableAutoC);
					::EnableWindow(::GetDlgItem(_hSelf, IDD_AUTOC_IGNORENUMBERS), isEnableAutoC);

					redraw();
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

				case IDD_AUTOC_USEENTER:
				{
					nppGUI._autocInsertSelectedUseENTER = isCheckedOrNot(static_cast<int32_t>(wParam));
					return TRUE;
				}

				case IDD_AUTOC_USETAB:
				{
					nppGUI._autocInsertSelectedUseTAB = isCheckedOrNot(static_cast<int32_t>(wParam));
					return TRUE;
				}

				case IDD_AUTOC_IGNORENUMBERS:
				{
					nppGUI._autocIgnoreNumbers = isCheckedOrNot(static_cast<int32_t>(wParam));
					return TRUE;
				}

				case IDD_FUNC_CHECK :
				{
					nppGUI._funcParams = isCheckedOrNot(static_cast<int32_t>(wParam));
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
					bool isChecked = isCheckedOrNot(static_cast<int32_t>(wParam));
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


intptr_t CALLBACK MultiInstanceSubDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	NppGUI & nppGUI = (NppParameters::getInstance()).getNppGUI();
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			MultiInstSetting multiInstSetting = nppGUI._multiInstSetting;

			::SendDlgItemMessage(_hSelf, IDC_SESSIONININST_RADIO, BM_SETCHECK, multiInstSetting == multiInstOnSession?BST_CHECKED:BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_MULTIINST_RADIO, BM_SETCHECK, multiInstSetting == multiInst?BST_CHECKED:BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_MONOINST_RADIO, BM_SETCHECK, multiInstSetting == monoInst?BST_CHECKED:BST_UNCHECKED, 0);

			::SendDlgItemMessage(_hSelf, IDD_DATETIMEFORMAT_REVERSEORDER_CHECK, BM_SETCHECK, nppGUI._dateTimeReverseDefaultOrder ? BST_CHECKED : BST_UNCHECKED, 0);

			::SetDlgItemText(_hSelf, IDC_DATETIMEFORMAT_EDIT, nppGUI._dateTimeFormat.c_str());
			generic_string datetimeStr = getDateTimeStrFrom(nppGUI._dateTimeFormat, _BTTF_time);
			::SetDlgItemText(_hSelf, IDD_DATETIMEFORMAT_RESULT_STATIC, datetimeStr.c_str());
		}
		break;

		case WM_CTLCOLOREDIT:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_DATETIMEFORMAT_EDIT)
			{
				const size_t inputLen = 256;
				TCHAR input[inputLen];
				::GetDlgItemText(_hSelf, IDC_DATETIMEFORMAT_EDIT, input, inputLen);

				nppGUI._dateTimeFormat = input;
				generic_string datetimeStr = getDateTimeStrFrom(nppGUI._dateTimeFormat, _BTTF_time);
				::SetDlgItemText(_hSelf, IDD_DATETIMEFORMAT_RESULT_STATIC, datetimeStr.c_str());
				return TRUE;
			}

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

				case IDD_DATETIMEFORMAT_REVERSEORDER_CHECK:
				{
					nppGUI._dateTimeReverseDefaultOrder = isCheckedOrNot(IDD_DATETIMEFORMAT_REVERSEORDER_CHECK);
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

void DelimiterSubDlg::detectSpace(const char *text2Check, int & nbSp, int & nbTab) const
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


generic_string DelimiterSubDlg::getWarningText(size_t nbSp, size_t nbTab) const
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
		if (!warnBegin.empty() && !space.empty() && !tab.empty() && !warnEnd.empty())
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
	else if (nbSp && !nbTab)
	{
		generic_string nbSpStr = std::to_wstring(nbSp);
		generic_string warnBegin = pNativeSpeaker->getLocalizedStrFromID("word-chars-list-warning-begin", TEXT(""));
		generic_string space = pNativeSpeaker->getLocalizedStrFromID("word-chars-list-space-warning", TEXT(""));
		generic_string warnEnd = pNativeSpeaker->getLocalizedStrFromID("word-chars-list-warning-end", TEXT(""));

		// half translation is not allowed
		if (!warnBegin.empty() && !space.empty() && !warnEnd.empty())
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
	else if (!nbSp && nbTab)
	{
		generic_string nbTabStr = std::to_wstring(nbTab);
		generic_string warnBegin = pNativeSpeaker->getLocalizedStrFromID("word-chars-list-warning-begin", TEXT(""));
		generic_string tab = pNativeSpeaker->getLocalizedStrFromID("word-chars-list-tab-warning", TEXT(""));
		generic_string warnEnd = pNativeSpeaker->getLocalizedStrFromID("word-chars-list-warning-end", TEXT(""));

		// half translation is not allowed
		if (!warnBegin.empty() && !tab.empty() && !warnEnd.empty())
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
	else // (!nbSp && !nbTab)
	{
		// do nothing
	}

	return msg;
}

void DelimiterSubDlg::setWarningIfNeed() const
{
	generic_string msg;
	NppGUI & nppGUI = (NppParameters::getInstance()).getNppGUI();
	if (!nppGUI._isWordCharDefault)
	{
		int nbSp = 0;
		int nbTab = 0;
		detectSpace(nppGUI._customWordChars.c_str(), nbSp, nbTab);
		msg = getWarningText(nbSp, nbTab);
	}
	::SetDlgItemText(_hSelf, IDD_STATIC_WORDCHAR_WARNING, msg.c_str());
}

intptr_t CALLBACK DelimiterSubDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	NppGUI & nppGUI = (NppParameters::getInstance()).getNppGUI();
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
			::SendDlgItemMessage(_hSelf, IDC_RADIO_WORDCHAR_CUSTOM, BM_SETCHECK, !nppGUI._isWordCharDefault ? BST_CHECKED : BST_UNCHECKED, 0);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_WORDCHAR_CUSTOM_EDIT), !nppGUI._isWordCharDefault);

			setWarningIfNeed();

			NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
			generic_string tip2show = pNativeSpeaker->getLocalizedStrFromID("word-chars-list-tip", TEXT("This allows you to include additional character into current word characters while double clicking for selection or searching with \"Match whole word only\" option checked."));

			_tip = CreateToolTip(IDD_WORDCHAR_QUESTION_BUTTON, _hSelf, _hInst, const_cast<PTSTR>(tip2show.c_str()), pNativeSpeaker->isRTL());
			if (_tip)
			{
				SendMessage(_tip, TTM_ACTIVATE, TRUE, 0);
				SendMessage(_tip, TTM_SETMAXTIPWIDTH, 0, 200);

				// Make tip stay 30 seconds
				SendMessage(_tip, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELPARAM((30000), (0)));
			}
			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			if (_tip)
			{
				NppDarkMode::setDarkTooltips(_tip, NppDarkMode::ToolTipsType::tooltip);
			}

			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORDLG:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORSTATIC:
		{
			auto hdcStatic = reinterpret_cast<HDC>(wParam);
			auto dlgCtrlID = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam));
			bool isBlabla = (dlgCtrlID == IDD_STATIC_BLABLA) || (dlgCtrlID == IDD_STATIC_BLABLA2NDLINE);
			if (NppDarkMode::isEnabled())
			{
				if (isBlabla)
				{
					return NppDarkMode::onCtlColor(hdcStatic);
				}
				return NppDarkMode::onCtlColorDarker(hdcStatic);
			}
			else if (isBlabla)
			{
				COLORREF bgColor = getCtrlBgColor(_hSelf);
				SetTextColor(hdcStatic, RGB(0, 0, 0));
				BYTE r = GetRValue(bgColor) - 30;
				BYTE g = MyGetGValue(bgColor) - 30;
				BYTE b = GetBValue(bgColor) - 30;
				SetBkColor(hdcStatic, RGB(r, g, b));
				return TRUE;
			}
			break;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				switch (LOWORD(wParam))
				{
					case  IDC_EDIT_OPENDELIMITER:
					{
						TCHAR opener[2] = { '\0' };
						::SendDlgItemMessage(_hSelf, IDC_EDIT_OPENDELIMITER, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(opener));
						nppGUI._leftmostDelimiter =  static_cast<char>(opener[0]);
						return TRUE;
					}
					case  IDC_EDIT_CLOSEDELIMITER:
					{
						TCHAR closer[2] = { '\0' };
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
					::EnableWindow(::GetDlgItem(_hSelf, IDC_WORDCHAR_CUSTOM_EDIT), !nppGUI._isWordCharDefault);
					::SetDlgItemText(_hSelf, IDD_STATIC_WORDCHAR_WARNING, TEXT(""));
					return TRUE;
				}

				case IDC_RADIO_WORDCHAR_CUSTOM:
				{
					::SendDlgItemMessage(_hSelf, IDC_RADIO_WORDCHAR_DEFAULT, BM_SETCHECK, BST_UNCHECKED, 0);
					nppGUI._isWordCharDefault = false;
					::SendMessage(::GetParent(_hParent), NPPM_INTERNAL_SETWORDCHARS, 0, 0);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_WORDCHAR_CUSTOM_EDIT), !nppGUI._isWordCharDefault);
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

intptr_t CALLBACK CloudAndLinkSubDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	NppParameters& nppParams = NppParameters::getInstance();
	NppGUI & nppGUI = nppParams.getNppGUI();
	const size_t uriSchemesMaxLength = 2048;

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

			case IDC_URISCHEMES_EDIT:
			{
				TCHAR uriScheme[uriSchemesMaxLength] = { '\0' };
				::SendDlgItemMessage(_hSelf, IDC_URISCHEMES_EDIT, WM_GETTEXT, uriSchemesMaxLength, reinterpret_cast<LPARAM>(uriScheme));
				nppGUI._uriSchemes = uriScheme;
				HWND grandParent = ::GetParent(_hParent);
				::SendMessage(grandParent, NPPM_INTERNAL_UPDATECLICKABLELINKS, 0, 0);
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

			BOOL linkEnable = nppGUI._styleURL != urlDisable;
			BOOL dontUnderline = (nppGUI._styleURL == urlNoUnderLineFg) || (nppGUI._styleURL == urlNoUnderLineBg);
			BOOL roundBoxMode = (nppGUI._styleURL == urlNoUnderLineBg) || (nppGUI._styleURL == urlUnderLineBg);
			::SendDlgItemMessage(_hSelf, IDC_URISCHEMES_EDIT, EM_SETLIMITTEXT, uriSchemesMaxLength, 0);
			::SetWindowText(::GetDlgItem(_hSelf, IDC_URISCHEMES_EDIT), nppGUI._uriSchemes.c_str());

			::SendDlgItemMessage(_hSelf, IDC_CHECK_CLICKABLELINK_ENABLE, BM_SETCHECK, linkEnable, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_CLICKABLELINK_NOUNDERLINE, BM_SETCHECK, dontUnderline, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_CLICKABLELINK_FULLBOXMODE, BM_SETCHECK, roundBoxMode, 0);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_CLICKABLELINK_NOUNDERLINE), linkEnable);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_CLICKABLELINK_FULLBOXMODE), linkEnable);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_URISCHEMES_EDIT), linkEnable);
		}
		break;

		case WM_CTLCOLOREDIT:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORDLG:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORSTATIC:
		{
			auto hdcStatic = reinterpret_cast<HDC>(wParam);
			auto dlgCtrlID = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam));

			bool isStaticText = dlgCtrlID == IDC_URISCHEMES_STATIC;
			//set the static text colors to show enable/disable instead of ::EnableWindow which causes blurry text
			if (isStaticText)
			{
				bool isTextEnabled = isCheckedOrNot(IDC_CHECK_CLICKABLELINK_ENABLE);
				return NppDarkMode::onCtlColorDarkerBGStaticText(hdcStatic, isTextEnabled);
			}

			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(hdcStatic);
			}
			return FALSE;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

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

				case IDC_CHECK_CLICKABLELINK_ENABLE:
				{
					bool isChecked = isCheckedOrNot(IDC_CHECK_CLICKABLELINK_ENABLE);
					if (!isChecked)
					{
						::SendDlgItemMessage(_hSelf, IDC_CHECK_CLICKABLELINK_NOUNDERLINE, BM_SETCHECK, BST_UNCHECKED, 0);
						::SendDlgItemMessage(_hSelf, IDC_CHECK_CLICKABLELINK_FULLBOXMODE, BM_SETCHECK, BST_UNCHECKED, 0);
					}
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_CLICKABLELINK_NOUNDERLINE), isChecked);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_CLICKABLELINK_FULLBOXMODE), isChecked);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_URISCHEMES_EDIT), isChecked);

					redraw();

					nppGUI._styleURL = isChecked ? urlUnderLineFg : urlDisable;
					HWND grandParent = ::GetParent(_hParent);
					::SendMessage(grandParent, NPPM_INTERNAL_UPDATECLICKABLELINKS, 0, 0);
				}
				return TRUE;

				case IDC_CHECK_CLICKABLELINK_NOUNDERLINE:
				case IDC_CHECK_CLICKABLELINK_FULLBOXMODE:
				{
					bool isNoUnderline = isCheckedOrNot(IDC_CHECK_CLICKABLELINK_NOUNDERLINE);
					bool isRoundBoxMode = isCheckedOrNot(IDC_CHECK_CLICKABLELINK_FULLBOXMODE);
					if (isRoundBoxMode)
						nppGUI._styleURL = isNoUnderline ? urlNoUnderLineBg : urlUnderLineBg;
					else
						nppGUI._styleURL = isNoUnderline ? urlNoUnderLineFg : urlUnderLineFg;
					HWND grandParent = ::GetParent(_hParent);
					::SendMessage(grandParent, NPPM_INTERNAL_UPDATECLICKABLELINKS, 0, 0);
				}
				return TRUE;

				default:
					return FALSE;

			}
		}																						
	}
	return FALSE;
}

intptr_t CALLBACK SearchEngineSubDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	NppParameters& nppParams = NppParameters::getInstance();
	NppGUI & nppGUI = nppParams.getNppGUI();

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
			if (nppGUI._searchEngineChoice == nppGUI.se_bing)
				nppGUI._searchEngineChoice = nppGUI.se_duckDuckGo;

			::SendDlgItemMessage(_hSelf, IDC_SEARCHENGINE_CUSTOM_RADIO, BM_SETCHECK, nppGUI._searchEngineChoice == nppGUI.se_custom ? BST_CHECKED : BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_SEARCHENGINE_DUCKDUCKGO_RADIO, BM_SETCHECK, nppGUI._searchEngineChoice == nppGUI.se_duckDuckGo ? BST_CHECKED : BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_SEARCHENGINE_GOOGLE_RADIO, BM_SETCHECK, nppGUI._searchEngineChoice == nppGUI.se_google ? BST_CHECKED : BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_SEARCHENGINE_YAHOO_RADIO, BM_SETCHECK, nppGUI._searchEngineChoice == nppGUI.se_yahoo ? BST_CHECKED : BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_SEARCHENGINE_STACKOVERFLOW_RADIO, BM_SETCHECK, nppGUI._searchEngineChoice == nppGUI.se_stackoverflow ? BST_CHECKED : BST_UNCHECKED, 0);

			::SendDlgItemMessage(_hSelf, IDC_SEARCHENGINE_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(nppGUI._searchEngineCustom.c_str()));
			::EnableWindow(::GetDlgItem(_hSelf, IDC_SEARCHENGINE_EDIT), nppGUI._searchEngineChoice == nppGUI.se_custom);
		}
		break;

		case WM_CTLCOLOREDIT:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

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

intptr_t CALLBACK SearchingSubDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	NppParameters& nppParams = NppParameters::getInstance();
	NppGUI& nppGUI = nppParams.getNppGUI();

	switch (message)
	{
		case WM_INITDIALOG:
		{
			::SendDlgItemMessage(_hSelf, IDC_CHECK_STOPFILLINGFINDFIELD, BM_SETCHECK, nppGUI._stopFillingFindField, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_MONOSPACEDFONT_FINDDLG, BM_SETCHECK, nppGUI._monospacedFontFindDlg, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_FINDDLG_ALWAYS_VISIBLE, BM_SETCHECK, nppGUI._findDlgAlwaysVisible, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_CONFIRMREPLOPENDOCS, BM_SETCHECK, nppGUI._confirmReplaceInAllOpenDocs, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_REPLACEANDSTOP, BM_SETCHECK, nppGUI._replaceStopsWithoutFindingNext, 0);
			::SendDlgItemMessage(_hSelf, IDC_CHECK_SHOWONCEPERFOUNDLINE, BM_SETCHECK, nppGUI._finderShowOnlyOneEntryPerFoundLine, 0);
		}
		break;

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDC_CHECK_STOPFILLINGFINDFIELD:
				{
					nppGUI._stopFillingFindField = isCheckedOrNot(IDC_CHECK_STOPFILLINGFINDFIELD);
					return TRUE;
				}
				break;

				case IDC_CHECK_MONOSPACEDFONT_FINDDLG:
				{
					nppGUI._monospacedFontFindDlg = isCheckedOrNot(IDC_CHECK_MONOSPACEDFONT_FINDDLG);
					return TRUE;
				}
				break;

				case IDC_CHECK_FINDDLG_ALWAYS_VISIBLE:
				{
					nppGUI._findDlgAlwaysVisible = isCheckedOrNot(IDC_CHECK_FINDDLG_ALWAYS_VISIBLE);
					return TRUE;
				}
				break;

				case IDC_CHECK_CONFIRMREPLOPENDOCS:
				{
					nppGUI._confirmReplaceInAllOpenDocs = isCheckedOrNot(IDC_CHECK_CONFIRMREPLOPENDOCS);
					return TRUE;
				}
				break;

				case IDC_CHECK_REPLACEANDSTOP:
				{
					nppGUI._replaceStopsWithoutFindingNext = isCheckedOrNot(IDC_CHECK_REPLACEANDSTOP);
					return TRUE;
				}
				break;

				case IDC_CHECK_SHOWONCEPERFOUNDLINE:
				{
					nppGUI._finderShowOnlyOneEntryPerFoundLine = isCheckedOrNot(IDC_CHECK_SHOWONCEPERFOUNDLINE);
					return TRUE;
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
