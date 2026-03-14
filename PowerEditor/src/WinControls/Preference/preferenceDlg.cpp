     1|// This file is part of npminmin project
     2|// Copyright (C)2021 Don HO <don.h@free.fr>
     3|
     4|// This program is free software: you can redistribute it and/or modify
     5|// it under the terms of the GNU General Public License as published by
     6|// the Free Software Foundation, either version 3 of the License, or
     7|// at your option any later version.
     8|//
     9|// This program is distributed in the hope that it will be useful,
    10|// but WITHOUT ANY WARRANTY; without even the implied warranty of
    11|// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    12|// GNU General Public License for more details.
    13|//
    14|// You should have received a copy of the GNU General Public License
    15|// along with this program.  If not, see <https://www.gnu.org/licenses/>.
    16|
    17|
    18|#include "preferenceDlg.h"
    19|
    20|#include <windows.h>
    21|
    22|#include <algorithm>
    23|#include <cassert>
    24|#include <cctype>
    25|#include <cstdint>
    26|#include <cstdlib>
    27|#include <cstring>
    28|#include <cwchar>
    29|#include <memory>
    30|#include <stdexcept>
    31|#include <string>
    32|#include <utility>
    33|#include <vector>
    34|
    35|#include <Scintilla.h>
    36|
    37|#include "ColourPicker.h"
    38|#include "Common.h"
    39|#include "ContextMenu.h"
    40|#include "ControlsTab.h"
    41|#include "EncodingMapper.h"
    42|#include "Notepad_plus_msgs.h"
    43|#include "NppConstants.h"
    44|#include "NppDarkMode.h"
    45|#include "NppXml.h"
    46|#include "Parameters.h"
    47|#include "ScintillaEditView.h"
    48|#include "ToolBar.h"
    49|#include "dpiManagerV2.h"
    50|#include "localization.h"
    51|#include "menuCmdID.h"
    52|#include "preference_rc.h"
    53|#include "regExtDlgRc.h"
    54|#include "resource.h"
    55|#include "shortcut.h"
    56|
    57|using namespace std;
    58|
    59|static constexpr int BLINKRATE_FASTEST = 50;
    60|static constexpr int BLINKRATE_SLOWEST = 2500;
    61|static constexpr int BLINKRATE_INTERVAL = 50;
    62|
    63|static constexpr int CARETLINEFRAME_SMALLEST = 1;
    64|static constexpr int CARETLINEFRAME_LARGEST = 6;
    65|static constexpr int CARETLINEFRAME_INTERVAL = 1;
    66|
    67|static constexpr int BORDERWIDTH_SMALLEST = 0;
    68|static constexpr int BORDERWIDTH_LARGEST = 30;
    69|static constexpr int BORDERWIDTH_INTERVAL = 1;
    70|
    71|static constexpr int PADDING_SMALLEST = 0;
    72|static constexpr int PADDING_LARGEST = 30;
    73|static constexpr int PADDING_INTERVAL = 1;
    74|
    75|static constexpr int DISTRACTIONFREE_SMALLEST = 3;
    76|static constexpr int DISTRACTIONFREE_LARGEST = 9;
    77|static constexpr int DISTRACTIONFREE_INTERVAL = 1;
    78|
    79|static constexpr int AUTOCOMPLETEFROMCHAR_SMALLEST = 1;
    80|static constexpr int AUTOCOMPLETEFROMCHAR_LARGEST = 9;
    81|static constexpr int AUTOCOMPLETEFROMCHAR_INTERVAL = 1;
    82|
    83|// This int encoding array is built from "EncodingUnit encodings[]" (see EncodingMapper.cpp)
    84|// And NewDocumentSubDlg will use "int encoding array" to get more info from "EncodingUnit encodings[]"
    85|static constexpr int encodings[]{
    86|	1250,
    87|	1251, 
    88|	1252, 
    89|	1253, 
    90|	1254, 
    91|	1255, 
    92|	1256, 
    93|	1257, 
    94|	1258, 
    95|	28591,
    96|	28592,
    97|	28593,
    98|	28594,
    99|	28595,
   100|	28596,
   101|	28597,
   102|	28598,
   103|	28599,
   104|	28603,
   105|	28604,
   106|	28605,
   107|	437,  
   108|	720,  
   109|	737,  
   110|	775,  
   111|	850,  
   112|	852,  
   113|	855,  
   114|	857,  
   115|	858,  
   116|	860,  
   117|	861,  
   118|	862,  
   119|	863,  
   120|	865,  
   121|	866,  
   122|	869,  
   123|	950,  
   124|	936,  
   125|	932,  
   126|	949,  
   127|	51949,
   128|	874,
   129|	10007,
   130|	21866,
   131|	20866
   132|};
   133|
   134|struct MsgData
   135|{
   136|	UINT uMsg = 0;
   137|	WPARAM wParam = 0;
   138|	LPARAM lParam = 0;
   139|};
   140|
   141|static LRESULT CALLBACK EditEnterProc(
   142|	HWND hwnd,
   143|	UINT Message,
   144|	WPARAM wParam,
   145|	LPARAM lParam,
   146|	UINT_PTR uIdSubclass,
   147|	DWORD_PTR dwRefData
   148|)
   149|{
   150|	auto* pMsgData = reinterpret_cast<MsgData*>(dwRefData);
   151|
   152|	switch (Message)
   153|	{
   154|		case WM_NCDESTROY:
   155|		{
   156|			::RemoveWindowSubclass(hwnd, EditEnterProc, uIdSubclass);
   157|			delete pMsgData;
   158|			break;
   159|		}
   160|
   161|		case WM_GETDLGCODE:
   162|		{
   163|			return (DLGC_WANTALLKEYS | ::DefSubclassProc(hwnd, Message, wParam, lParam));
   164|		}
   165|
   166|		case WM_CHAR:
   167|		{
   168|			if (wParam == VK_RETURN) // to avoid beep
   169|			{
   170|				return 0;
   171|			}
   172|			break;
   173|		}
   174|
   175|		case WM_KEYDOWN:
   176|		{
   177|			if (wParam == VK_RETURN)
   178|			{
   179|				::SendMessage(::GetParent(hwnd), pMsgData->uMsg, pMsgData->wParam, pMsgData->lParam);
   180|				return 0;
   181|			}
   182|			break;
   183|		}
   184|
   185|		default:
   186|			break;
   187|	}
   188|
   189|	return ::DefSubclassProc(hwnd, Message, wParam, lParam);
   190|}
   191|
   192|static void subclassEditToAcceptEnterKey(HWND hEdit, UINT uMsg, WPARAM wParam, LPARAM lParam)
   193|{
   194|	if (::GetWindowSubclass(hEdit, EditEnterProc, static_cast<UINT_PTR>(SubclassID::first), nullptr) == FALSE)
   195|	{
   196|		auto pMsgData = std::make_unique<MsgData>(uMsg, wParam, lParam);
   197|		if (::SetWindowSubclass(hEdit, EditEnterProc, static_cast<UINT_PTR>(SubclassID::first), reinterpret_cast<DWORD_PTR>(pMsgData.get())) == TRUE)
   198|		{
   199|			static_cast<void>(pMsgData.release());
   200|		}
   201|	}
   202|}
   203|
   204|
   205|bool PreferenceDlg::goToSection(size_t iPage, intptr_t ctrlID)
   206|{
   207|	::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_SETCURSEL, iPage, 0);
   208|	showDialogByIndex(iPage);
   209|	grabFocus();
   210|
   211|	if (ctrlID != -1)
   212|	{
   213|		::SetFocus(::GetDlgItem(_wVector[iPage]._dlg->getHSelf(), static_cast<int>(ctrlID)));
   214|		if (_gotoTip.isValid())
   215|		{
   216|			_gotoTip.hide();
   217|		}
   218|
   219|		NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
   220|		static wstring hereTip = pNativeSpeaker->getLocalizedStrFromID("goto-setting-tip", L"Find your setting here");
   221|		const bool isSuccessful = _gotoTip.init(_hInst, ::GetDlgItem(_wVector[iPage]._dlg->getHSelf(), static_cast<int>(ctrlID)), _hSelf, hereTip.c_str(), pNativeSpeaker->isRTL(), 2000);
   222|
   223|		if (!isSuccessful)
   224|			return false;
   225|
   226|		NppDarkMode::setDarkTooltips(_gotoTip.getTipHandle(), NppDarkMode::ToolTipsType::tooltip);
   227|		
   228|		_gotoTip.show();
   229|	}
   230|	return true;
   231|}
   232|
   233|intptr_t CALLBACK PreferenceDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
   234|{
   235|	switch (message) 
   236|	{
   237|		case WM_INITDIALOG :
   238|		{
   239|			_generalSubDlg.init(_hInst, _hSelf);
   240|			_generalSubDlg.create(IDD_PREFERENCE_SUB_GENERAL, false, false);
   241|			_generalSubDlg.display();
   242|			
   243|			_toolbarSubDlg.init(_hInst, _hSelf);
   244|			_toolbarSubDlg.create(IDD_PREFERENCE_SUB_TOOLBAR, false, false);
   245|
   246|			_tabbarSubDlg.init(_hInst, _hSelf);
   247|			_tabbarSubDlg.create(IDD_PREFERENCE_SUB_TABBAR, false, false);
   248|
   249|			_editingSubDlg.init(_hInst, _hSelf);
   250|			_editingSubDlg.create(IDD_PREFERENCE_SUB_EDITING, false, false);
   251|
   252|			_editing2SubDlg.init(_hInst, _hSelf);
   253|			_editing2SubDlg.create(IDD_PREFERENCE_SUB_EDITING2, false, false);
   254|
   255|			_darkModeSubDlg.init(_hInst, _hSelf);
   256|			_darkModeSubDlg.create(IDD_PREFERENCE_SUB_DARKMODE, false, false);
   257|
   258|			_marginsBorderEdgeSubDlg.init(_hInst, _hSelf);
   259|			_marginsBorderEdgeSubDlg.create(IDD_PREFERENCE_SUB_MARGING_BORDER_EDGE, false, false);
   260|			
   261|			_miscSubDlg.init(_hInst, _hSelf);
   262|			_miscSubDlg.create(IDD_PREFERENCE_SUB_MISC, false, false);
   263|			
   264|			_newDocumentSubDlg.init(_hInst, _hSelf);
   265|			_newDocumentSubDlg.create(IDD_PREFERENCE_SUB_NEWDOCUMENT, false, false);
   266|
   267|			_defaultDirectorySubDlg.init(_hInst, _hSelf);
   268|			_defaultDirectorySubDlg.create(IDD_PREFERENCE_SUB_DEFAULTDIRECTORY, false, false);
   269|
   270|			_recentFilesHistorySubDlg.init(_hInst, _hSelf);
   271|			_recentFilesHistorySubDlg.create(IDD_PREFERENCE_SUB_RECENTFILESHISTORY, false, false);
   272|
   273|			_fileAssocDlg.init(_hInst, _hSelf);
   274|			_fileAssocDlg.create(IDD_REGEXT_BOX, false, false);
   275|
   276|			_printSubDlg.init(_hInst, _hSelf);
   277|			_printSubDlg.create(IDD_PREFERENCE_SUB_PRINT, false, false);
   278|
   279|			_searchingSubDlg.init(_hInst, _hSelf);
   280|			_searchingSubDlg.create(IDD_PREFERENCE_SUB_SEARCHING, false, false);
   281|
   282|			_languageSubDlg.init(_hInst, _hSelf);
   283|			_languageSubDlg.create(IDD_PREFERENCE_SUB_LANGUAGE, false, false);
   284|
   285|			_indentationSubDlg.init(_hInst, _hSelf);
   286|			_indentationSubDlg.create(IDD_PREFERENCE_SUB_INDENTATION, false, false);
   287|
   288|			_highlightingSubDlg.init(_hInst, _hSelf);
   289|			_highlightingSubDlg.create(IDD_PREFERENCE_SUB_HIGHLIGHTING, false, false);
   290|
   291|			_backupSubDlg.init(_hInst, _hSelf);
   292|			_backupSubDlg.create(IDD_PREFERENCE_SUB_BACKUP, false, false);
   293|
   294|			_autoCompletionSubDlg.init(_hInst, _hSelf);
   295|			_autoCompletionSubDlg.create(IDD_PREFERENCE_SUB_AUTOCOMPLETION, false, false);
   296|
   297|			_multiInstanceSubDlg.init(_hInst, _hSelf);
   298|			_multiInstanceSubDlg.create(IDD_PREFERENCE_SUB_MULTIINSTANCE, false, false);
   299|
   300|			_delimiterSubDlg.init(_hInst, _hSelf);
   301|			_delimiterSubDlg.create(IDD_PREFERENCE_SUB_DELIMITER, false, false);
   302|			
   303|			_performanceSubDlg.init(_hInst, _hSelf);
   304|			_performanceSubDlg.create(IDD_PREFERENCE_SUB_PERFORMANCE, false, false);
   305|
   306|			_cloudAndLinkSubDlg.init(_hInst, _hSelf);
   307|			_cloudAndLinkSubDlg.create(IDD_PREFERENCE_SUB_CLOUD_LINK, false, false);
   308|
   309|			_searchEngineSubDlg.init(_hInst, _hSelf);
   310|			_searchEngineSubDlg.create(IDD_PREFERENCE_SUB_SEARCHENGINE, false, false);
   311|
   312|			_wVector.push_back(DlgInfo(&_generalSubDlg, L"General", L"Global"));
   313|			_wVector.push_back(DlgInfo(&_toolbarSubDlg, L"Toolbar", L"Toolbar"));
   314|			_wVector.push_back(DlgInfo(&_tabbarSubDlg, L"Tab Bar", L"Tabbar"));
   315|			_wVector.push_back(DlgInfo(&_editingSubDlg, L"Editing 1", L"Scintillas"));
   316|			_wVector.push_back(DlgInfo(&_editing2SubDlg, L"Editing 2", L"Scintillas2"));
   317|			_wVector.push_back(DlgInfo(&_darkModeSubDlg, L"Dark Mode", L"DarkMode"));
   318|			_wVector.push_back(DlgInfo(&_marginsBorderEdgeSubDlg, L"Margins/Border/Edge", L"MarginsBorderEdge"));
   319|			_wVector.push_back(DlgInfo(&_newDocumentSubDlg, L"New Document", L"NewDoc"));
   320|			_wVector.push_back(DlgInfo(&_defaultDirectorySubDlg, L"Default Directory", L"DefaultDir"));
   321|			_wVector.push_back(DlgInfo(&_recentFilesHistorySubDlg, L"Recent Files History", L"RecentFilesHistory"));
   322|			_wVector.push_back(DlgInfo(&_fileAssocDlg, L"File Association", L"FileAssoc"));
   323|			_wVector.push_back(DlgInfo(&_languageSubDlg, L"Language", L"Language"));
   324|			_wVector.push_back(DlgInfo(&_indentationSubDlg, L"Indentation", L"Indentation"));
   325|			_wVector.push_back(DlgInfo(&_highlightingSubDlg, L"Highlighting", L"Highlighting"));
   326|			_wVector.push_back(DlgInfo(&_printSubDlg, L"Print", L"Print"));
   327|			_wVector.push_back(DlgInfo(&_searchingSubDlg, L"Searching", L"Searching"));
   328|			_wVector.push_back(DlgInfo(&_backupSubDlg, L"Backup", L"Backup"));
   329|			_wVector.push_back(DlgInfo(&_autoCompletionSubDlg, L"Auto-Completion", L"AutoCompletion"));
   330|			_wVector.push_back(DlgInfo(&_multiInstanceSubDlg, L"Multi-Instance & Date", L"MultiInstance"));
   331|			_wVector.push_back(DlgInfo(&_delimiterSubDlg, L"Delimiter", L"Delimiter"));
   332|			_wVector.push_back(DlgInfo(&_performanceSubDlg, L"Performance", L"Performance"));
   333|			_wVector.push_back(DlgInfo(&_cloudAndLinkSubDlg, L"Cloud & Link", L"Cloud"));
   334|			_wVector.push_back(DlgInfo(&_searchEngineSubDlg, L"Search Engine", L"SearchEngine"));
   335|			_wVector.push_back(DlgInfo(&_miscSubDlg, L"MISC.", L"MISC"));
   336|
   337|
   338|			makeCategoryList();
   339|
   340|			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
   341|
   342|			return TRUE;
   343|		}
   344|
   345|		case WM_NCLBUTTONDOWN:
   346|		{
   347|			if (_gotoTip.isValid())
   348|			{
   349|				_gotoTip.hide();
   350|			}
   351|			return FALSE;
   352|		}
   353|
   354|		case WM_TIMER:
   355|		{
   356|			if (wParam == IDT_HIDE_TOOLTIP)
   357|			{
   358|				if (_gotoTip.isValid())
   359|				{
   360|					_gotoTip.hide();
   361|					KillTimer(_hSelf, IDT_HIDE_TOOLTIP);
   362|					return TRUE;
   363|				}
   364|			}
   365|			break;
   366|		}
   367|
   368|		case WM_CTLCOLORLISTBOX:
   369|		{
   370|			return NppDarkMode::onCtlColorListbox(wParam, lParam);
   371|		}
   372|
   373|		case WM_CTLCOLORDLG:
   374|		case WM_CTLCOLORSTATIC:
   375|		{
   376|			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
   377|		}
   378|
   379|		case WM_PRINTCLIENT:
   380|		{
   381|			if (NppDarkMode::isEnabled())
   382|			{
   383|				return TRUE;
   384|			}
   385|			break;
   386|		}
   387|
   388|		case NPPM_INTERNAL_REFRESHDARKMODE:
   389|		{
   390|			NppDarkMode::autoThemeChildControls(_hSelf);
   391|
   392|			if (_toolbarSubDlg._accentTip != nullptr)
   393|				NppDarkMode::setDarkTooltips(_toolbarSubDlg._accentTip, NppDarkMode::ToolTipsType::tooltip);
   394|
   395|			if (_tabbarSubDlg._tabCompactLabelLenTip != nullptr)
   396|				NppDarkMode::setDarkTooltips(_tabbarSubDlg._tabCompactLabelLenTip, NppDarkMode::ToolTipsType::tooltip);
   397|
   398|			if (_editing2SubDlg._tip != nullptr)
   399|				NppDarkMode::setDarkTooltips(_editing2SubDlg._tip, NppDarkMode::ToolTipsType::tooltip);
   400|
   401|			if (_marginsBorderEdgeSubDlg._verticalEdgeTip != nullptr)
   402|				NppDarkMode::setDarkTooltips(_marginsBorderEdgeSubDlg._verticalEdgeTip, NppDarkMode::ToolTipsType::tooltip);
   403|
   404|			for (auto& tip : _editing2SubDlg._tips)
   405|			{
   406|				if (tip != nullptr)
   407|				{
   408|					NppDarkMode::setDarkTooltips(tip, NppDarkMode::ToolTipsType::tooltip);
   409|				}
   410|			}
   411|			if (_delimiterSubDlg._tip != nullptr)
   412|				NppDarkMode::setDarkTooltips(_delimiterSubDlg._tip, NppDarkMode::ToolTipsType::tooltip);
   413|			if (_performanceSubDlg._largeFileRestrictionTip != nullptr)
   414|				NppDarkMode::setDarkTooltips(_performanceSubDlg._largeFileRestrictionTip, NppDarkMode::ToolTipsType::tooltip);
   415|			if (_searchingSubDlg._tipInSelThresh != nullptr)
   416|				NppDarkMode::setDarkTooltips(_searchingSubDlg._tipInSelThresh, NppDarkMode::ToolTipsType::tooltip);
   417|			if (_searchingSubDlg._tipFillFindWhatThresh != nullptr)
   418|				NppDarkMode::setDarkTooltips(_searchingSubDlg._tipFillFindWhatThresh, NppDarkMode::ToolTipsType::tooltip);
   419|
   420|			if (_indentationSubDlg._tipAutoIndentBasic)
   421|				NppDarkMode::setDarkTooltips(_indentationSubDlg._tipAutoIndentBasic, NppDarkMode::ToolTipsType::tooltip);
   422|			if (_indentationSubDlg._tipAutoIndentAdvanced)
   423|				NppDarkMode::setDarkTooltips(_indentationSubDlg._tipAutoIndentAdvanced, NppDarkMode::ToolTipsType::tooltip);
   424|
   425|			if (_miscSubDlg._tipScintillaRenderingTechnology)
   426|				NppDarkMode::setDarkTooltips(_miscSubDlg._tipScintillaRenderingTechnology, NppDarkMode::ToolTipsType::tooltip);
   427|
   428|			// groupbox label in dark mode support disabled text color
   429|			if (NppDarkMode::isEnabled())
   430|			{
   431|				const NppGUI& nppGUI = NppParameters::getInstance().getNppGUI();
   432|				::EnableWindow(::GetDlgItem(_highlightingSubDlg.getHSelf(), IDC_SMARTHILITEMATCHING_STATIC), nppGUI._enableSmartHilite);
   433|
   434|				const bool noBackup = _backupSubDlg.isCheckedOrNot(IDC_RADIO_BKNONE);
   435|				::EnableWindow(::GetDlgItem(_backupSubDlg.getHSelf(), IDC_BACKUPDIR_USERCUSTOMDIR_GRPSTATIC), !noBackup);
   436|
   437|				const bool isEnableAutoC = _autoCompletionSubDlg.isCheckedOrNot(IDD_AUTOC_ENABLECHECK);
   438|				::EnableWindow(::GetDlgItem(_autoCompletionSubDlg.getHSelf(), IDD_AUTOC_USEKEY_GRP_STATIC), isEnableAutoC);
   439|
   440|				const bool isFluentIcon = !_toolbarSubDlg.isCheckedOrNot(IDC_RADIO_STANDARD);
   441|				::EnableWindow(::GetDlgItem(_toolbarSubDlg.getHSelf(), IDC_TOOLBAR_GB_COLORCHOICE), isFluentIcon);
   442|				::EnableWindow(::GetDlgItem(_toolbarSubDlg.getHSelf(), IDC_TOOLBAR_GB_COLORIZATION), isFluentIcon);
   443|			}
   444|
   445|			return TRUE;
   446|		}
   447|
   448|		case NPPM_INTERNAL_SETTOOLICONSSET: // Set icons set only option (checkbox) on general sub-dialog, the remaining real operations will be done in NppDarkMode::refreshDarkMode
   449|		{
   450|			NppParameters& nppParams = NppParameters::getInstance();
   451|			NppGUI& nppGUI = nppParams.getNppGUI();
   452|			auto& nppGUITbInfo = nppGUI._tbIconInfo;
   453|			nppGUITbInfo = NppDarkMode::getToolbarIconInfo(static_cast<bool>(wParam));
   454|
   455|			const HWND hToolbarlSubDlg = _toolbarSubDlg.getHSelf();
   456|
   457|			auto checkOrUncheckBtn = [&hToolbarlSubDlg](int id, bool check = false) -> void
   458|				{
   459|					::SendDlgItemMessage(hToolbarlSubDlg, id, BM_SETCHECK, check ? BST_CHECKED : BST_UNCHECKED, 0);
   460|				};
   461|
   462|			checkOrUncheckBtn(IDC_RADIO_SMALLICON, nppGUITbInfo._tbIconSet == TB_SMALL);
   463|			checkOrUncheckBtn(IDC_RADIO_BIGICON, nppGUITbInfo._tbIconSet == TB_LARGE);
   464|			checkOrUncheckBtn(IDC_RADIO_SMALLICON2, nppGUITbInfo._tbIconSet == TB_SMALL2);
   465|			checkOrUncheckBtn(IDC_RADIO_BIGICON2, nppGUITbInfo._tbIconSet == TB_LARGE2);
   466|			checkOrUncheckBtn(IDC_RADIO_STANDARD, nppGUITbInfo._tbIconSet == TB_STANDARD);
   467|
   468|			checkOrUncheckBtn(IDC_RADIO_RED, nppGUITbInfo._tbColor == FluentColor::red);
   469|			checkOrUncheckBtn(IDC_RADIO_GREEN, nppGUITbInfo._tbColor == FluentColor::green);
   470|			checkOrUncheckBtn(IDC_RADIO_BLUE, nppGUITbInfo._tbColor == FluentColor::blue);
   471|			checkOrUncheckBtn(IDC_RADIO_PURPLE, nppGUITbInfo._tbColor == FluentColor::purple);
   472|			checkOrUncheckBtn(IDC_RADIO_CYAN, nppGUITbInfo._tbColor == FluentColor::cyan);
   473|			checkOrUncheckBtn(IDC_RADIO_OLIVE, nppGUITbInfo._tbColor == FluentColor::olive);
   474|			checkOrUncheckBtn(IDC_RADIO_YELLOW, nppGUITbInfo._tbColor == FluentColor::yellow);
   475|			checkOrUncheckBtn(IDC_RADIO_ACCENTCOLOR, nppGUITbInfo._tbColor == FluentColor::accent);
   476|			checkOrUncheckBtn(IDC_RADIO_CUSTOMCOLOR, nppGUITbInfo._tbColor == FluentColor::custom);
   477|			checkOrUncheckBtn(IDC_RADIO_DEFAULTCOLOR, nppGUITbInfo._tbColor == FluentColor::defaultColor);
   478|
   479|			checkOrUncheckBtn(IDC_RADIO_COMPLETE, nppGUITbInfo._tbUseMono);
   480|			checkOrUncheckBtn(IDC_RADIO_PARTIAL, !nppGUITbInfo._tbUseMono);
   481|
   482|			::SendMessage(hToolbarlSubDlg, NPPM_INTERNAL_CHANGETOOLBARCOLORABLESTATE, static_cast<WPARAM>(true), 0);
   483|
   484|			return TRUE;
   485|		}
   486|
   487|		case WM_DPICHANGED:
   488|		{
   489|			_dpiManager.setDpiWP(wParam);
   490|			_generalSubDlg.dpiManager().setDpiWP(wParam);
   491|			_toolbarSubDlg.dpiManager().setDpiWP(wParam);
   492|			_tabbarSubDlg.dpiManager().setDpiWP(wParam);
   493|			_editingSubDlg.dpiManager().setDpiWP(wParam);
   494|			_editing2SubDlg.dpiManager().setDpiWP(wParam);
   495|			_darkModeSubDlg.dpiManager().setDpiWP(wParam);
   496|			_marginsBorderEdgeSubDlg.dpiManager().setDpiWP(wParam);
   497|			_miscSubDlg.dpiManager().setDpiWP(wParam);
   498|			_fileAssocDlg.dpiManager().setDpiWP(wParam);
   499|			_languageSubDlg.dpiManager().setDpiWP(wParam);
   500|			_indentationSubDlg.dpiManager().setDpiWP(wParam);
   501|