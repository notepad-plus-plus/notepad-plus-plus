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
    18|#include <algorithm>
    19|#include <shlwapi.h>
    20|#include <uxtheme.h> // for EnableThemeDialogTexture
    21|#include <format>
    22|#include <windowsx.h> // for GET_X_LPARAM, GET_Y_LPARAM
    23|#include <atomic>
    24|#include "Notepad_plus_Window.h"
    25|#include "TaskListDlg.h"
    26|#include "ShortcutMapper.h"
    27|#include "ansiCharPanel.h"
    28|#include "clipboardHistoryPanel.h"
    29|#include "VerticalFileSwitcher.h"
    30|#include "ProjectPanel.h"
    31|#include "documentMap.h"
    32|#include "functionListPanel.h"
    33|#include "fileBrowser.h"
    34|#include "NppDarkMode.h"
    35|#include "NppConstants.h"
    36|
    37|using namespace std;
    38|
    39|#ifndef WM_DPICHANGED
    40|#define WM_DPICHANGED 0x02E0
    41|#endif
    42|
    43|std::atomic<bool> g_bNppExitFlag{ false };
    44|const UINT WM_TASKBARCREATED = ::RegisterWindowMessage(L"TaskbarCreated");
    45|
    46|
    47|struct SortTaskListPred final
    48|{
    49|	DocTabView *_views[2];
    50|
    51|	SortTaskListPred(DocTabView &p, DocTabView &s)
    52|	{
    53|		_views[MAIN_VIEW] = &p;
    54|		_views[SUB_VIEW] = &s;
    55|	}
    56|
    57|	bool operator()(const TaskLstFnStatus &l, const TaskLstFnStatus &r) const {
    58|		BufferID lID = _views[l._iView]->getBufferByIndex(l._docIndex);
    59|		BufferID rID = _views[r._iView]->getBufferByIndex(r._docIndex);
    60|		Buffer * bufL = MainFileManager.getBufferByID(lID);
    61|		Buffer * bufR = MainFileManager.getBufferByID(rID);
    62|		return bufL->getRecentTag() > bufR->getRecentTag();
    63|	}
    64|};
    65|
    66|// app-restart feature needs Win10 20H1+ (builds 18963+), but it was quietly introduced earlier in the Fall Creators Update (b1709+)
    67|bool SetOSAppRestart()
    68|{
    69|	NppParameters& nppParam = NppParameters::getInstance();
    70|	if (nppParam.getWinVersion() < WV_WIN10)
    71|		return false; // unsupported
    72|
    73|	bool bRet = false;
    74|	bool bUnregister = nppParam.isRegForOSAppRestartDisabled();
    75|
    76|	wstring nppIssueLog;
    77|
    78|	wchar_t wszCmdLine[RESTART_MAX_CMD_LINE] = { 0 };
    79|	DWORD cchCmdLine = _countof(wszCmdLine);
    80|	DWORD dwPreviousFlags = 0;
    81|	HRESULT hr = ::GetApplicationRestartSettings(::GetCurrentProcess(), wszCmdLine, &cchCmdLine, &dwPreviousFlags);
    82|	if (bUnregister)
    83|	{
    84|		// unregistering (disabling) request
    85|
    86|		if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
    87|		{
    88|			// has not been registered before, nothing to do here
    89|			bRet = true;
    90|		}
    91|		else
    92|		{
    93|			if (hr == S_OK)
    94|			{
    95|				// has been already registered before, try to unregister
    96|				if (::UnregisterApplicationRestart() == S_OK)
    97|				{
    98|					bRet = true;
    99|				}
   100|			}
   101|		}
   102|	}
   103|	else
   104|	{
   105|		// registering request
   106|
   107|		if (hr == S_OK)
   108|			::UnregisterApplicationRestart(); // remove a previous registration 1st
   109|
   110|		if (nppParam.getCmdLineString().length() < RESTART_MAX_CMD_LINE)
   111|		{
   112|			// do not restart the process:
   113|			// RESTART_NO_CRASH  (1) ... for termination due to application crashes
   114|			// RESTART_NO_HANG   (2) ... for termination due to application hangs
   115|			// RESTART_NO_PATCH  (4) ... for termination due to patch installations
   116|			// RESTART_NO_REBOOT (8) ... when the system is rebooted
   117|			hr = ::RegisterApplicationRestart(nppParam.getCmdLineString().c_str(), RESTART_NO_CRASH | RESTART_NO_HANG | RESTART_NO_PATCH);
   118|			if (hr == S_OK)
   119|			{
   120|				bRet = true;
   121|			}
   122|		}
   123|	}
   124|
   125|	return bRet;
   126|}
   127|
   128|LRESULT CALLBACK Notepad_plus_Window::Notepad_plus_Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
   129|{
   130|	if (hwnd == NULL)
   131|		return FALSE;
   132|
   133|	switch(message)
   134|	{
   135|		case WM_NCCREATE:
   136|		{
   137|			// First message we get the pointer of instantiated object
   138|			// then store it into GWLP_USERDATA index in order to retrieve afterward
   139|			Notepad_plus_Window *pM30ide = static_cast<Notepad_plus_Window *>((reinterpret_cast<LPCREATESTRUCT>(lParam))->lpCreateParams);
   140|			pM30ide->_hSelf = hwnd;
   141|			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pM30ide));
   142|
   143|			if (NppDarkMode::isExperimentalSupported())
   144|			{
   145|				NppDarkMode::enableDarkScrollBarForWindowAndChildren(hwnd);
   146|			}
   147|
   148|			return TRUE;
   149|		}
   150|
   151|		default:
   152|		{
   153|			return (reinterpret_cast<Notepad_plus_Window *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA))->runProc(hwnd, message, wParam, lParam));
   154|		}
   155|	}
   156|}
   157|
   158|
   159|LRESULT Notepad_plus_Window::runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
   160|{
   161|	switch (message)
   162|	{
   163|		case WM_CREATE:
   164|		{
   165|			try
   166|			{
   167|				NppDarkMode::setDarkTitleBar(hwnd);
   168|				NppDarkMode::autoSubclassWindowMenuBar(hwnd);
   169|				NppDarkMode::autoSubclassCtlColor(hwnd);
   170|
   171|				_notepad_plus_plus_core._pPublicInterface = this;
   172|				LRESULT lRet = _notepad_plus_plus_core.init(hwnd);
   173|
   174|				if (NppDarkMode::isEnabled() && NppDarkMode::isExperimentalSupported())
   175|				{
   176|					// Inform application of the frame change.
   177|					::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
   178|				}
   179|
   180|				SetOSAppRestart();
   181|
   182|				return lRet;
   183|			}
   184|			catch (std::exception& ex)
   185|			{
   186|				::MessageBoxA(hwnd, ex.what(), "Exception On WM_CREATE", MB_OK);
   187|				exit(-1);
   188|			}
   189|			break;
   190|		}
   191|		default:
   192|		{
   193|			return _notepad_plus_plus_core.process(hwnd, message, wParam, lParam);
   194|		}
   195|	}
   196|}
   197|
   198|// Used by NPPM_GETFILENAMEATCURSOR
   199|int CharacterIs(wchar_t c, const wchar_t *any)
   200|{
   201|	int i;
   202|	for (i = 0; any[i] != 0; i++)
   203|	{
   204|		if (any[i] == c) return TRUE;
   205|	}
   206|	return FALSE;
   207|}
   208|
   209|LRESULT Notepad_plus::process(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
   210|{
   211|	LRESULT result = FALSE;
   212|	NppParameters& nppParam = NppParameters::getInstance();
   213|
   214|	switch (message)
   215|	{
   216|		case WM_NCACTIVATE:
   217|		{
   218|			// Note: lParam is -1 to prevent endless loops of calls
   219|			::SendMessage(_dockingManager.getHSelf(), WM_NCACTIVATE, wParam, -1);
   220|			NppDarkMode::calculateTreeViewStyle();
   221|			return ::DefWindowProc(hwnd, message, wParam, lParam);
   222|		}
   223|
   224|		case WM_SETTINGCHANGE:
   225|		{
   226|			NppDarkMode::handleSettingChange(hwnd, lParam);
   227|
   228|			const bool enableDarkMode = NppDarkMode::isExperimentalActive();
   229|			NppGUI& nppGUI = nppParam.getNppGUI();
   230|
   231|			// Windows mode is enabled
   232|			// and don't change if npminmin is already in same mode as OS after changing OS mode
   233|			if (NppDarkMode::isWindowsModeEnabled() && (enableDarkMode != NppDarkMode::isEnabled()))
   234|			{
   235|				nppGUI._darkmode._isEnabled = enableDarkMode;
   236|				if (!_preference.isCreated())
   237|				{
   238|					auto& nppGUITbInfo = nppGUI._tbIconInfo;
   239|					nppGUITbInfo = NppDarkMode::getToolbarIconInfo();
   240|
   241|					switch (nppGUITbInfo._tbIconSet)
   242|					{
   243|						case TB_SMALL:
   244|							_toolBar.reduce();
   245|							break;
   246|
   247|						case TB_LARGE:
   248|							_toolBar.enlarge();
   249|							break;
   250|
   251|						case TB_SMALL2:
   252|							_toolBar.reduceToSet2();
   253|							break;
   254|
   255|						case TB_LARGE2:
   256|							_toolBar.enlargeToSet2();
   257|							break;
   258|
   259|						case TB_STANDARD:
   260|							_toolBar.setToBmpIcons();
   261|							break;
   262|					}
   263|					NppDarkMode::refreshDarkMode(hwnd, false);
   264|				}
   265|				else
   266|				{
   267|					HWND hSubDlg = _preference._darkModeSubDlg.getHSelf();
   268|
   269|					// don't use IDC_RADIO_DARKMODE_FOLLOWWINDOWS, it is only for button,
   270|					// it calls NppDarkMode::handleSettingChange, which is not needed here
   271|					::SendMessage(hSubDlg, WM_COMMAND, IDC_RADIO_DARKMODE_DARKMODE, 0);
   272|				}
   273|			}
   274|
   275|			// let the Scintilla to update according to the possible changed OS settings
   276|			// (mouse wheel vertical & horizontal scroll amount, DirectWrite rendering params, base elements, style etc.)
   277|			::SendMessage(_mainEditView.getHSelf(), WM_SETTINGCHANGE, wParam, lParam);
   278|			::SendMessage(_subEditView.getHSelf(), WM_SETTINGCHANGE, wParam, lParam);
   279|
   280|			return ::DefWindowProc(hwnd, message, wParam, lParam);
   281|		}
   282|
   283|		case NPPM_INTERNAL_REFRESHDARKMODE:
   284|		{
   285|			refreshDarkMode(static_cast<bool>(wParam));
   286|			// Notify plugins that Dark Mode changed
   287|			SCNotification scnN{};
   288|			scnN.nmhdr.code = NPPN_DARKMODECHANGED;
   289|			scnN.nmhdr.hwndFrom = hwnd;
   290|			scnN.nmhdr.idFrom = 0;
   291|			_pluginsManager.notify(&scnN);
   292|			return TRUE;
   293|		}
   294|
   295|		case NPPM_INTERNAL_TOOLBARICONSCHANGED:
   296|		{
   297|			refreshInternalPanelIcons();
   298|			// Notify plugins that toolbar icons have changed
   299|			SCNotification scnN{};
   300|			scnN.nmhdr.code = NPPN_TOOLBARICONSETCHANGED;
   301|			scnN.nmhdr.hwndFrom = hwnd;
   302|			scnN.nmhdr.idFrom = _toolBar.getState();
   303|			_pluginsManager.notify(&scnN);
   304|			return TRUE;
   305|		}
   306|
   307|		case WM_DRAWITEM:
   308|		{
   309|			DRAWITEMSTRUCT *dis = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
   310|			if (dis->CtlType == ODT_TAB)
   311|				return ::SendMessage(dis->hwndItem, WM_DRAWITEM, wParam, lParam);
   312|			break;
   313|		}
   314|
   315|		case WM_DOCK_USERDEFINE_DLG:
   316|		{
   317|			dockUserDlg();
   318|			return TRUE;
   319|		}
   320|
   321|		case WM_UNDOCK_USERDEFINE_DLG:
   322|		{
   323|			undockUserDlg();
   324|			return TRUE;
   325|		}
   326|
   327|		case WM_REMOVE_USERLANG:
   328|		{
   329|			wchar_t *userLangName = reinterpret_cast<wchar_t *>(lParam);
   330|			if (!userLangName || !userLangName[0])
   331|				return FALSE;
   332|
   333|			wstring name{userLangName};
   334|
   335|			//loop through buffers and reset the language (L_USER, L"")) if (L_USER, name)
   336|			for (size_t i = 0; i < MainFileManager.getNbBuffers(); ++i)
   337|			{
   338|				Buffer* buf = MainFileManager.getBufferByIndex(i);
   339|				if (buf->getLangType() == L_USER && name == buf->getUserDefineLangName())
   340|					buf->setLangType(L_USER, L"");
   341|			}
   342|			return TRUE;
   343|		}
   344|
   345|		case WM_RENAME_USERLANG:
   346|		{
   347|			if (!lParam || !((reinterpret_cast<wchar_t *>(lParam))[0]) || !wParam || !((reinterpret_cast<wchar_t *>(wParam))[0]))
   348|				return FALSE;
   349|
   350|			wstring oldName{ reinterpret_cast<wchar_t *>(lParam) };
   351|			wstring newName{ reinterpret_cast<wchar_t *>(wParam) };
   352|
   353|			//loop through buffers and reset the language (L_USER, newName) if (L_USER, oldName)
   354|			for (size_t i = 0; i < MainFileManager.getNbBuffers(); ++i)
   355|			{
   356|				Buffer* buf = MainFileManager.getBufferByIndex(i);
   357|				if (buf->getLangType() == L_USER && oldName == buf->getUserDefineLangName())
   358|					buf->setLangType(L_USER, newName.c_str());
   359|			}
   360|			return TRUE;
   361|		}
   362|
   363|		case WM_CLOSE_USERDEFINE_DLG:
   364|		{
   365|			checkMenuItem(IDM_LANG_USER_DLG, false);
   366|			_toolBar.setCheck(IDM_LANG_USER_DLG, false);
   367|			return TRUE;
   368|		}
   369|
   370|		case WM_REPLACEALL_INOPENEDDOC:
   371|		{
   372|			replaceInOpenedFiles();
   373|			return TRUE;
   374|		}
   375|
   376|		case WM_FINDALL_INOPENEDDOC:
   377|		{
   378|			findInOpenedFiles();
   379|			return TRUE;
   380|		}
   381|
   382|		case WM_FINDALL_INCURRENTDOC:
   383|		{
   384|			const bool isEntireDoc = wParam == 0;
   385|			return findInCurrentFile(isEntireDoc);
   386|		}
   387|
   388|		case WM_FINDINFILES:
   389|		{
   390|			return findInFiles();
   391|		}
   392|
   393|		case WM_FINDINPROJECTS:
   394|		{
   395|			return findInProjects();
   396|		}
   397|
   398|		case WM_FINDALL_INCURRENTFINDER:
   399|		{
   400|			FindersInfo *findInFolderInfo = reinterpret_cast<FindersInfo *>(wParam);
   401|			Finder * newFinder = _findReplaceDlg.createFinder();
   402|			
   403|			findInFolderInfo->_pDestFinder = newFinder;
   404|			bool isOK = findInFinderFiles(findInFolderInfo);
   405|			return isOK;
   406|		}
   407|
   408|		case WM_REPLACEINFILES:
   409|		{
   410|			replaceInFiles();
   411|			return TRUE;
   412|		}
   413|
   414|		case WM_REPLACEINPROJECTS:
   415|		{
   416|			replaceInProjects();
   417|			return TRUE;
   418|		}
   419|
   420|		case NPPM_LAUNCHFINDINFILESDLG:
   421|		{
   422|			// Find in files function code should be here due to the number of parameters (2) cannot be passed via WM_COMMAND
   423|
   424|			bool isFirstTime = !_findReplaceDlg.isCreated();
   425|			_findReplaceDlg.doDialog(FIND_DLG, _nativeLangSpeaker.isRTL());
   426|
   427|			_findReplaceDlg.setSearchTextWithSettings();
   428|
   429|			if (isFirstTime)
   430|				_nativeLangSpeaker.changeFindReplaceDlgLang(_findReplaceDlg);
   431|			_findReplaceDlg.launchFindInFilesDlg();
   432|			setFindReplaceFolderFilter(reinterpret_cast<const wchar_t*>(wParam), reinterpret_cast<const wchar_t*>(lParam));
   433|
   434|			return TRUE;
   435|		}
   436|
   437|		case NPPM_INTERNAL_FINDINPROJECTS:
   438|		{
   439|			bool isFirstTime = not _findReplaceDlg.isCreated();
   440|			_findReplaceDlg.doDialog(FIND_DLG, _nativeLangSpeaker.isRTL());
   441|
   442|			_findReplaceDlg.setSearchTextWithSettings();
   443|
   444|			if (isFirstTime)
   445|				_nativeLangSpeaker.changeFindReplaceDlgLang(_findReplaceDlg);
   446|
   447|			_findReplaceDlg.launchFindInProjectsDlg();
   448|			_findReplaceDlg.setProjectCheckmarks(NULL, (int) wParam);
   449|			return TRUE;
   450|		}
   451|
   452|		case NPPM_INTERNAL_FINDINFINDERDLG:
   453|		{
   454|			Finder *launcher = reinterpret_cast<Finder *>(wParam);
   455|
   456|			bool isFirstTime = !_findInFinderDlg.isCreated();
   457|
   458|			_findInFinderDlg.doDialog(launcher, _nativeLangSpeaker.isRTL());
   459|
   460|			_findReplaceDlg.setSearchTextWithSettings();
   461|			setFindReplaceFolderFilter(NULL, NULL);
   462|
   463|			if (isFirstTime)
   464|				_nativeLangSpeaker.changeFindReplaceDlgLang(_findReplaceDlg);
   465|
   466|			return TRUE;
   467|		}
   468|
   469|		case NPPM_DOOPEN:
   470|		case WM_DOOPEN:
   471|		{
   472|			BufferID id = doOpen(reinterpret_cast<const wchar_t *>(lParam));
   473|			if (id != BUFFER_INVALID)
   474|				return switchToFile(id);
   475|			break;
   476|		}
   477|
   478|		case NPPM_GETBUFFERLANGTYPE:
   479|		{
   480|			if (!wParam)
   481|				return -1;
   482|			BufferID id = (BufferID)wParam;
   483|			Buffer * b = MainFileManager.getBufferByID(id);
   484|			return b->getLangType();
   485|		}
   486|
   487|		case NPPM_SETBUFFERLANGTYPE:
   488|		{
   489|			if (!wParam)
   490|				return FALSE;
   491|			if (lParam < L_TEXT || lParam >= L_EXTERNAL || lParam == L_USER)
   492|				return FALSE;
   493|
   494|			BufferID id = (BufferID)wParam;
   495|			Buffer * b = MainFileManager.getBufferByID(id);
   496|			b->setLangType((LangType)lParam);
   497|			return TRUE;
   498|		}
   499|
   500|		case NPPM_GETBUFFERENCODING:
   501|