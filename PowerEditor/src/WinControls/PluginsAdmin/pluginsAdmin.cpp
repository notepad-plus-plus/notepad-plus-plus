     1|     1|     1|     1|// This file is part of npminmin project
     2|     2|     2|     2|// Copyright (C)2021 Don HO <don.h@free.fr>
     3|     3|     3|     3|
     4|     4|     4|     4|// This program is free software: you can redistribute it and/or modify
     5|     5|     5|     5|// it under the terms of the GNU General Public License as published by
     6|     6|     6|     6|// the Free Software Foundation, either version 3 of the License, or
     7|     7|     7|     7|// at your option any later version.
     8|     8|     8|     8|//
     9|     9|     9|     9|// This program is distributed in the hope that it will be useful,
    10|    10|    10|    10|// but WITHOUT ANY WARRANTY; without even the implied warranty of
    11|    11|    11|    11|// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    12|    12|    12|    12|// GNU General Public License for more details.
    13|    13|    13|    13|//
    14|    14|    14|    14|// You should have received a copy of the GNU General Public License
    15|    15|    15|    15|// along with this program.  If not, see <https://www.gnu.org/licenses/>.
    16|    16|    16|    16|
    17|    17|    17|    17|
    18|    18|    18|    18|#include "pluginsAdmin.h"
    19|    19|    19|    19|
    20|    20|    20|    20|#include <windows.h>
    21|    21|    21|    21|
    22|    22|    22|    22|#include <shlobj.h>
    23|    23|    23|    23|#include <shlwapi.h>
    24|    24|    24|    24|
    25|    25|    25|    25|#include <algorithm>
    26|    26|    26|    26|#include <cctype>
    27|    27|    27|    27|#include <cwchar>
    28|    28|    28|    28|#include <string>
    29|    29|    29|    29|#include <utility>
    30|    30|    30|    30|#include <vector>
    31|    31|    31|    31|
    32|    32|    32|    32|#ifdef NDEBUG
    33|    33|    33|    33|#include <cstring>
    34|    34|    34|    34|#else
    35|    35|    35|    35|#include <exception>
    36|    36|    36|    36|#include <fstream>
    37|    37|    37|    37|#endif
    38|    38|    38|    38|
    39|    39|    39|    39|#include <json.hpp>
    40|    40|    40|    40|
    41|    41|    41|    41|#include "Common.h"
    42|    42|    42|    42|#include "ListView.h"
    43|    43|    43|    43|#include "Notepad_plus_msgs.h"
    44|    44|    44|    44|#include "NppDarkMode.h"
    45|    45|    45|    45|#include "Parameters.h"
    46|    46|    46|    46|#include "PluginsManager.h"
    47|    47|    47|    47|#include "StaticDialog.h"
    48|    48|    48|    48|#include "localization.h"
    49|    49|    49|    49|#include "menuCmdID.h"
    50|    50|    50|    50|#include "pluginsAdminRes.h"
    51|    51|    51|    51|#include "resource.h"
    52|    52|    52|    52|
    53|    53|    53|    53|#ifdef NDEBUG
    54|    54|    54|    54|#include "verifySignedfile.h"
    55|    55|    55|    55|#endif
    56|    56|    56|    56|
    57|    57|    57|    57|#define TEXTFILE        256
    58|    58|    58|    58|#define IDR_PLUGINLISTJSONFILE  101
    59|    59|    59|    59|
    60|    60|    60|    60|using namespace std;
    61|    61|    61|    61|using nlohmann::json;
    62|    62|    62|    62|
    63|    63|    63|    63|
    64|    64|    64|    64|std::wstring PluginUpdateInfo::describe() const
    65|    65|    65|    65|{
    66|    66|    66|    66|	wstring desc;
    67|    67|    67|    67|	const wchar_t *EOL = L"\r\n";
    68|    68|    68|    68|	if (!_description.empty())
    69|    69|    69|    69|	{
    70|    70|    70|    70|		desc = _description;
    71|    71|    71|    71|		desc += EOL;
    72|    72|    72|    72|	}
    73|    73|    73|    73|
    74|    74|    74|    74|	if (!_author.empty())
    75|    75|    75|    75|	{
    76|    76|    76|    76|		desc += L"Author: ";
    77|    77|    77|    77|		desc += _author;
    78|    78|    78|    78|		desc += EOL;
    79|    79|    79|    79|	}
    80|    80|    80|    80|
    81|    81|    81|    81|	if (!_homepage.empty())
    82|    82|    82|    82|	{
    83|    83|    83|    83|		desc += L"Homepage: ";
    84|    84|    84|    84|		desc += _homepage;
    85|    85|    85|    85|		desc += EOL;
    86|    86|    86|    86|	}
    87|    87|    87|    87|
    88|    88|    88|    88|	return desc;
    89|    89|    89|    89|}
    90|    90|    90|    90|
    91|    91|    91|    91|/// Try to find in the Haystack the Needle - ignore case
    92|    92|    92|    92|static bool findStrNoCase(const std::wstring& strHaystack, const std::wstring& strNeedle)
    93|    93|    93|    93|{
    94|    94|    94|    94|	auto it = std::search(
    95|    95|    95|    95|		strHaystack.begin(), strHaystack.end(),
    96|    96|    96|    96|		strNeedle.begin(), strNeedle.end(),
    97|    97|    97|    97|		[](wchar_t ch1, wchar_t ch2){return towupper(ch1) == towupper(ch2); }
    98|    98|    98|    98|	);
    99|    99|    99|    99|	return (it != strHaystack.end());
   100|   100|   100|   100|}
   101|   101|   101|   101|
   102|   102|   102|   102|bool PluginsAdminDlg::isFoundInListFromIndex(const PluginViewList& inWhichList, int index, const wstring& str2search, bool inWhichPart) const
   103|   103|   103|   103|{
   104|   104|   104|   104|	const PluginUpdateInfo* pui = inWhichList.getPluginInfoFromUiIndex(index);
   105|   105|   105|   105|	wstring searchIn;
   106|   106|   106|   106|	if (inWhichPart == _inNames)
   107|   107|   107|   107|		searchIn = pui->_displayName;
   108|   108|   108|   108|	else //(inWhichPart == inDescs)
   109|   109|   109|   109|		searchIn = pui->_description;
   110|   110|   110|   110|
   111|   111|   111|   111|	return (findStrNoCase(searchIn, str2search));
   112|   112|   112|   112|}
   113|   113|   113|   113|
   114|   114|   114|   114|long PluginsAdminDlg::searchFromCurrentSel(const PluginViewList& inWhichList, const wstring& str2search, bool inWhichPart, bool isNextMode) const
   115|   115|   115|   115|{
   116|   116|   116|   116|	// search from current selected item or from the beginning
   117|   117|   117|   117|	long currentIndex = inWhichList.getSelectedIndex();
   118|   118|   118|   118|	int nbItem = static_cast<int>(inWhichList.nbItem());
   119|   119|   119|   119|	if (currentIndex == -1)
   120|   120|   120|   120|	{
   121|   121|   121|   121|		// no selection, let's search from 0 to the end
   122|   122|   122|   122|		for (int i = 0; i < nbItem; ++i)
   123|   123|   123|   123|		{
   124|   124|   124|   124|			if (isFoundInListFromIndex(inWhichList, i, str2search, inWhichPart))
   125|   125|   125|   125|				return i;
   126|   126|   126|   126|		}
   127|   127|   127|   127|	}
   128|   128|   128|   128|	else // with selection, let's search from currentIndex
   129|   129|   129|   129|	{
   130|   130|   130|   130|		// from current position to the end
   131|   131|   131|   131|		for (int i = currentIndex + (isNextMode ? 1 : 0); i < nbItem; ++i)
   132|   132|   132|   132|		{
   133|   133|   133|   133|			if (isFoundInListFromIndex(inWhichList, i, str2search, inWhichPart))
   134|   134|   134|   134|				return i;
   135|   135|   135|   135|		}
   136|   136|   136|   136|
   137|   137|   137|   137|		// from to beginning to current position
   138|   138|   138|   138|		for (int i = 0; i < currentIndex + (isNextMode ? 1 : 0); ++i)
   139|   139|   139|   139|		{
   140|   140|   140|   140|			if (isFoundInListFromIndex(inWhichList, i, str2search, inWhichPart))
   141|   141|   141|   141|				return i;
   142|   142|   142|   142|		}
   143|   143|   143|   143|	}
   144|   144|   144|   144|	return -1;
   145|   145|   145|   145|}
   146|   146|   146|   146|
   147|   147|   147|   147|void PluginsAdminDlg::create(int dialogID, bool isRTL, bool msgDestParent, WORD fontSize)
   148|   148|   148|   148|{
   149|   149|   149|   149|	// get plugin installation path and launch mode (Admin or normal)
   150|   150|   150|   150|	collectNppCurrentStatusInfos();
   151|   151|   151|   151|
   152|   152|   152|   152|	StaticDialog::create(dialogID, isRTL, msgDestParent, fontSize);
   153|   153|   153|   153|
   154|   154|   154|   154|	RECT rect{};
   155|   155|   155|   155|	getClientRect(rect);
   156|   156|   156|   156|	_tab.init(_hInst, _hSelf, false, true);
   157|   157|   157|   157|
   158|   158|   158|   158|	const wchar_t *available = L"Available";
   159|   159|   159|   159|	const wchar_t *updates = L"Updates";
   160|   160|   160|   160|	const wchar_t *installed = L"Installed";
   161|   161|   161|   161|	const wchar_t *incompatible = L"Incompatible";
   162|   162|   162|   162|
   163|   163|   163|   163|	_tab.insertAtEnd(available);
   164|   164|   164|   164|	_tab.insertAtEnd(updates);
   165|   165|   165|   165|	_tab.insertAtEnd(installed);
   166|   166|   166|   166|	_tab.insertAtEnd(incompatible);
   167|   167|   167|   167|
   168|   168|   168|   168|	RECT rcDesc{};
   169|   169|   169|   169|	getMappedChildRect(IDC_PLUGINADM_EDIT, rcDesc);
   170|   170|   170|   170|
   171|   171|   171|   171|	const long margeX = _dpiManager.getSystemMetricsForDpi(SM_CXEDGE);
   172|   172|   172|   172|	const long margeY = _dpiManager.scale(13);
   173|   173|   173|   173|
   174|   174|   174|   174|	rect.bottom = rcDesc.bottom + margeY;
   175|   175|   175|   175|	_tab.reSizeTo(rect);
   176|   176|   176|   176|	_tab.display();
   177|   177|   177|   177|
   178|   178|   178|   178|	RECT rcSearch{};
   179|   179|   179|   179|	getMappedChildRect(IDC_PLUGINADM_SEARCH_EDIT, rcSearch);
   180|   180|   180|   180|
   181|   181|   181|   181|	RECT listRect{
   182|   182|   182|   182|		rcDesc.left - margeX,
   183|   183|   183|   183|		rcSearch.bottom + margeY,
   184|   184|   184|   184|		rcDesc.right + _dpiManager.getSystemMetricsForDpi(SM_CXVSCROLL) + margeX,
   185|   185|   185|   185|		rcDesc.top - margeY
   186|   186|   186|   186|	};
   187|   187|   187|   187|
   188|   188|   188|   188|	NppParameters& nppParam = NppParameters::getInstance();
   189|   189|   189|   189|	NativeLangSpeaker *pNativeSpeaker = nppParam.getNativeLangSpeaker();
   190|   190|   190|   190|	wstring pluginStr = pNativeSpeaker->getAttrNameStr(L"Plugin", "PluginAdmin", "Plugin");
   191|   191|   191|   191|	wstring vesionStr = pNativeSpeaker->getAttrNameStr(L"Version", "PluginAdmin", "Version");
   192|   192|   192|   192|
   193|   193|   193|   193|	const COLORREF fgColor = nppParam.getCurrentDefaultFgColor();
   194|   194|   194|   194|	const COLORREF bgColor = nppParam.getCurrentDefaultBgColor();
   195|   195|   195|   195|
   196|   196|   196|   196|	const size_t szColVer = _dpiManager.scale(100);
   197|   197|   197|   197|	const size_t szColName = szColVer * 2;
   198|   198|   198|   198|
   199|   199|   199|   199|	auto initListView = [&](PluginViewList& list) -> void {
   200|   200|   200|   200|		list.addColumn(columnInfo(pluginStr, szColName));
   201|   201|   201|   201|		list.addColumn(columnInfo(vesionStr, szColVer));
   202|   202|   202|   202|		list.setViewStyleOption(LVS_EX_CHECKBOXES);
   203|   203|   203|   203|		list.initView(_hInst, _hSelf);
   204|   204|   204|   204|		const HWND hList = list.getViewHwnd();
   205|   205|   205|   205|		ListView_SetBkColor(hList, bgColor);
   206|   206|   206|   206|		ListView_SetTextBkColor(hList, bgColor);
   207|   207|   207|   207|		ListView_SetTextColor(hList, fgColor);
   208|   208|   208|   208|		const auto style = ::GetWindowLongPtr(hList, GWL_STYLE);
   209|   209|   209|   209|		::SetWindowLongPtr(hList, GWL_STYLE, style | WS_TABSTOP);
   210|   210|   210|   210|		::SetWindowPos(hList, ::GetDlgItem(_hSelf, IDC_PLUGINADM_RESEARCH_NEXT), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); // to allow tab switch
   211|   211|   211|   211|		list.reSizeView(listRect);
   212|   212|   212|   212|	};
   213|   213|   213|   213|
   214|   214|   214|   214|	initListView(_availableList);
   215|   215|   215|   215|	initListView(_updateList);
   216|   216|   216|   216|	initListView(_installedList);
   217|   217|   217|   217|	initListView(_incompatibleList);
   218|   218|   218|   218|
   219|   219|   219|   219|	switchDialog(0);
   220|   220|   220|   220|
   221|   221|   221|   221|	NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
   222|   222|   222|   222|	NppDarkMode::autoSubclassAndThemeWindowNotify(_hSelf);
   223|   223|   223|   223|
   224|   224|   224|   224|	HWND hPluginListVersionNumber = ::GetDlgItem(_hSelf, IDC_PLUGINLIST_VERSIONNUMBER_STATIC);
   225|   225|   225|   225|	::SetWindowText(hPluginListVersionNumber, _pluginListVersion.c_str());
   226|   226|   226|   226|
   227|   227|   227|   227|	_repoLink.init(_hInst, _hSelf);
   228|   228|   228|   228|	_repoLink.create(::GetDlgItem(_hSelf, IDC_PLUGINLIST_ADDR), L"https://github.com/notepad-plus-plus/nppPluginList");
   229|   229|   229|   229|
   230|   230|   230|   230|	goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);
   231|   231|   231|   231|}
   232|   232|   232|   232|
   233|   233|   233|   233|void PluginsAdminDlg::collectNppCurrentStatusInfos()
   234|   234|   234|   234|{
   235|   235|   235|   235|	NppParameters& nppParam = NppParameters::getInstance();
   236|   236|   236|   236|	_nppCurrentStatus._nppInstallPath = nppParam.getNppPath();
   237|   237|   237|   237|
   238|   238|   238|   238|	_nppCurrentStatus._isAppDataPluginsAllowed = ::SendMessage(_hParent, NPPM_GETAPPDATAPLUGINSALLOWED, 0, 0) == TRUE;
   239|   239|   239|   239|	_nppCurrentStatus._appdataPath = nppParam.getAppDataNppDir();
   240|   240|   240|   240|	wstring programFilesPath = NppParameters::getSpecialFolderLocation(CSIDL_PROGRAM_FILES);
   241|   241|   241|   241|	_nppCurrentStatus._isInProgramFiles = (_nppCurrentStatus._nppInstallPath.find(programFilesPath) == 0);
   242|   242|   242|   242|
   243|   243|   243|   243|}
   244|   244|   244|   244|
   245|   245|   245|   245|vector<PluginUpdateInfo*> PluginViewList::fromUiIndexesToPluginInfos(const std::vector<size_t>& uiIndexes) const
   246|   246|   246|   246|{
   247|   247|   247|   247|	std::vector<PluginUpdateInfo*> r;
   248|   248|   248|   248|	size_t nb = _ui.nbItem();
   249|   249|   249|   249|
   250|   250|   250|   250|	for (const auto &i : uiIndexes)
   251|   251|   251|   251|	{
   252|   252|   252|   252|		if (i < nb)
   253|   253|   253|   253|		{
   254|   254|   254|   254|			r.push_back(getPluginInfoFromUiIndex(i));
   255|   255|   255|   255|		}
   256|   256|   256|   256|	}
   257|   257|   257|   257|	return r;
   258|   258|   258|   258|}
   259|   259|   259|   259|
   260|   260|   260|   260|PluginsAdminDlg::PluginsAdminDlg()
   261|   261|   261|   261|{
   262|   262|   262|   262|	// npminmin: wingup/updater removed
   263|   263|   263|   267|	_updaterFullPath = L""; // npminmin: no gup.exe
   264|   264|   264|   268|
   265|   265|   265|   269|	// get plugin-list path
   266|   266|   266|   270|	_pluginListFullPath = nppParameters.getPluginConfDir();
   267|   267|   267|   271|
   268|   268|   268|   272|#if !defined(NDEBUG)  // if not debug, then it's release
   269|   269|   269|   273|	// load from nppPluginList.json instead of nppPluginList.dll
   270|   270|   270|   274|	pathAppend(_pluginListFullPath, L"nppPluginList.json");
   271|   271|   271|   275|#else //RELEASE
   272|   272|   272|   276|	pathAppend(_pluginListFullPath, L"nppPluginList.dll");
   273|   273|   273|   277|#endif
   274|   274|   274|   278|}
   275|   275|   275|   279|
   276|   276|   276|   280|wstring PluginsAdminDlg::getPluginListVerStr() const
   277|   277|   277|   281|{
   278|   278|   278|   282|	Version v;
   279|   279|   279|   283|	v.setVersionFrom(_pluginListFullPath);
   280|   280|   280|   284|	return v.toString();
   281|   281|   281|   285|}
   282|   282|   282|   286|
   283|   283|   283|   287|bool PluginsAdminDlg::exitToInstallRemovePlugins(Operation op, const vector<PluginUpdateInfo*>& puis)
   284|   284|   284|   288|{
   285|   285|   285|   289|	wstring opStr;
   286|   286|   286|   290|	if (op == pa_install)
   287|   287|   287|   291|		opStr = L"-unzipTo ";
   288|   288|   288|   292|	else if (op == pa_update)
   289|   289|   289|   293|		opStr = L"-unzipTo -clean ";
   290|   290|   290|   294|	else if (op == pa_remove)
   291|   291|   291|   295|		opStr = L"-clean ";
   292|   292|   292|   296|	else
   293|   293|   293|   297|		return false;
   294|   294|   294|   298|
   295|   295|   295|   299|	NppParameters& nppParameters = NppParameters::getInstance();
   296|   296|   296|   300|	wstring updaterDir = nppParameters.getNppPath();
   297|   297|   297|   301|	updaterDir += L"\\updater\\";
   298|   298|   298|   302|
   299|   299|   299|   303|	wstring updaterFullPath = L""; // npminmin: no updater
   300|   300|   300|   304|
   301|   301|   301|   305|	wstring updaterParams = opStr;
   302|   302|   302|   306|
   303|   303|   303|   307|	wchar_t nppFullPath[MAX_PATH]{};
   304|   304|   304|   308|	::GetModuleFileName(NULL, nppFullPath, MAX_PATH);
   305|   305|   305|   309|	updaterParams += L"\"";
   306|   306|   306|   310|	updaterParams += nppFullPath;
   307|   307|   307|   311|	updaterParams += L"\" ";
   308|   308|   308|   312|
   309|   309|   309|   313|	updaterParams += L"\"";
   310|   310|   310|   314|	updaterParams += nppParameters.getPluginRootDir();
   311|   311|   311|   315|	updaterParams += L"\"";
   312|   312|   312|   316|
   313|   313|   313|   317|	for (const auto &i : puis)
   314|   314|   314|   318|	{
   315|   315|   315|   319|		if (op == pa_install || op == pa_update)
   316|   316|   316|   320|		{
   317|   317|   317|   321|			// add folder to operate
   318|   318|   318|   322|			updaterParams += L" \"";
   319|   319|   319|   323|			updaterParams += i->_folderName;
   320|   320|   320|   324|			updaterParams += L" ";
   321|   321|   321|   325|			updaterParams += i->_repository;
   322|   322|   322|   326|			updaterParams += L" ";
   323|   323|   323|   327|			updaterParams += i->_id;
   324|   324|   324|   328|			updaterParams += L"\"";
   325|   325|   325|   329|		}
   326|   326|   326|   330|		else // op == pa_remove
   327|   327|   327|   331|		{
   328|   328|   328|   332|			// add folder to operate
   329|   329|   329|   333|			updaterParams += L" \"";
   330|   330|   330|   334|			wstring folderName = i->_folderName;
   331|   331|   331|   335|			if (folderName.empty())
   332|   332|   332|   336|			{
   333|   333|   333|   337|				auto lastindex = i->_displayName.find_last_of(L".");
   334|   334|   334|   338|				if (lastindex != wstring::npos)
   335|   335|   335|   339|					folderName = i->_displayName.substr(0, lastindex);
   336|   336|   336|   340|				else
   337|   337|   337|   341|					folderName = i->_displayName;	// This case will never occur, but in case if it occurs too
   338|   338|   338|   342|													// just putting the plugin name, so that whole plugin system is not screewed.
   339|   339|   339|   343|			}
   340|   340|   340|   344|			updaterParams += folderName;
   341|   341|   341|   345|			updaterParams += L"\"";
   342|   342|   342|   346|		}
   343|   343|   343|   347|	}
   344|   344|   344|   348|
   345|   345|   345|   349|	// Ask user's confirmation
   346|   346|   346|   350|	NativeLangSpeaker *pNativeSpeaker = nppParameters.getNativeLangSpeaker();
   347|   347|   347|   351|	auto res = pNativeSpeaker->messageBox("ExitToUpdatePlugins",
   348|   348|   348|   352|		_hSelf,
   349|   349|   349|   353|		L"If you click YES, you will quit npminmin to continue the operations.\nnpminmin will be restarted after all the operations are terminated.\nContinue?",
   350|   350|   350|   354|		L"npminmin is about to exit",
   351|   351|   351|   355|		MB_YESNO | MB_APPLMODAL);
   352|   352|   352|   356|
   353|   353|   353|   357|	if (res == IDYES)
   354|   354|   354|   358|	{
   355|   355|   355|   359|		NppParameters& nppParam = NppParameters::getInstance();
   356|   356|   356|   360|
   357|   357|   357|   361|		// gup path: makes trigger ready
   358|   358|   358|   362|		nppParam.setWingupFullPath(updaterFullPath);
   359|   359|   359|   363|
   360|   360|   360|   364|		// op: -clean or "-clean -unzip"
   361|   361|   361|   365|		// application path: npminmin path to be relaunched
   362|   362|   362|   366|		// plugin global path
   363|   363|   363|   367|		// plugin names or "plugin names + download url"
   364|   364|   364|   368|		nppParam.setWingupParams(updaterParams);
   365|   365|   365|   369|
   366|   366|   366|   370|		// gup folder path
   367|   367|   367|   371|		nppParam.setWingupDir(updaterDir);
   368|   368|   368|   372|
   369|   369|   369|   373|		// Quite npminmin so just before quitting npminmin launches gup with needed arguments  
   370|   370|   370|   374|		::PostMessage(_hParent, WM_COMMAND, IDM_FILE_EXIT, 0);
   371|   371|   371|   375|	}
   372|   372|   372|   376|
   373|   373|   373|   377|	return true;
   374|   374|   374|   378|}
   375|   375|   375|   379|
   376|   376|   376|   380|bool PluginsAdminDlg::installPlugins()
   377|   377|   377|   381|{
   378|   378|   378|   382|	// Need to exit npminmin
   379|   379|   379|   383|
   380|   380|   380|   384|	vector<size_t> indexes = _availableList.getCheckedIndexes();
   381|   381|   381|   385|	vector<PluginUpdateInfo*> puis = _availableList.fromUiIndexesToPluginInfos(indexes);
   382|   382|   382|   386|
   383|   383|   383|   387|	return exitToInstallRemovePlugins(pa_install, puis);
   384|   384|   384|   388|}
   385|   385|   385|   389|
   386|   386|   386|   390|bool PluginsAdminDlg::updatePlugins()
   387|   387|   387|   391|{
   388|   388|   388|   392|	// Need to exit npminmin
   389|   389|   389|   393|
   390|   390|   390|   394|	vector<size_t> indexes = _updateList.getCheckedIndexes();
   391|   391|   391|   395|	vector<PluginUpdateInfo*> puis = _updateList.fromUiIndexesToPluginInfos(indexes);
   392|   392|   392|   396|
   393|   393|   393|   397|	return exitToInstallRemovePlugins(pa_update, puis);
   394|   394|   394|   398|}
   395|   395|   395|   399|
   396|   396|   396|   400|bool PluginsAdminDlg::removePlugins()
   397|   397|   397|   401|{
   398|   398|   398|   402|	// Need to exit npminmin
   399|   399|   399|   403|
   400|   400|   400|   404|	vector<size_t> indexes = _installedList.getCheckedIndexes();
   401|   401|   401|   405|	vector<PluginUpdateInfo*> puis = _installedList.fromUiIndexesToPluginInfos(indexes);
   402|   402|   402|   406|
   403|   403|   403|   407|	return exitToInstallRemovePlugins(pa_remove, puis);
   404|   404|   404|   408|}
   405|   405|   405|   409|
   406|   406|   406|   410|void PluginsAdminDlg::changeTabName(LIST_TYPE index, wchar_t* name2change)
   407|   407|   407|   411|{
   408|   408|   408|   412|	TCITEM tie{};
   409|   409|   409|   413|	tie.mask = TCIF_TEXT;
   410|   410|   410|   414|	tie.pszText = name2change;
   411|   411|   411|   415|	TabCtrl_SetItem(_tab.getHSelf(), index, &tie);
   412|   412|   412|   416|
   413|   413|   413|   417|	wchar_t label[MAX_PATH]{};
   414|   414|   414|   418|	_tab.getCurrentTitle(label, MAX_PATH);
   415|   415|   415|   419|	::SetWindowText(_hSelf, label);
   416|   416|   416|   420|}
   417|   417|   417|   421|
   418|   418|   418|   422|void PluginsAdminDlg::changeColumnName(COLUMN_TYPE index, const wchar_t *name2change)
   419|   419|   419|   423|{
   420|   420|   420|   424|	_availableList.changeColumnName(index, name2change);
   421|   421|   421|   425|	_updateList.changeColumnName(index, name2change);
   422|   422|   422|   426|	_installedList.changeColumnName(index, name2change);
   423|   423|   423|   427|	_incompatibleList.changeColumnName(index, name2change);
   424|   424|   424|   428|}
   425|   425|   425|   429|
   426|   426|   426|   430|void PluginViewList::changeColumnName(COLUMN_TYPE index, const wchar_t *name2change)
   427|   427|   427|   431|{
   428|   428|   428|   432|	_ui.setColumnText(index, name2change);
   429|   429|   429|   433|}
   430|   430|   430|   434|
   431|   431|   431|   435|bool PluginViewList::removeFromFolderName(const wstring& folderName)
   432|   432|   432|   436|{
   433|   433|   433|   437|
   434|   434|   434|   438|	for (size_t i = 0; i < _ui.nbItem(); ++i)
   435|   435|   435|   439|	{
   436|   436|   436|   440|		const PluginUpdateInfo* pi = getPluginInfoFromUiIndex(i);
   437|   437|   437|   441|		if (pi->_folderName == folderName)
   438|   438|   438|   442|		{
   439|   439|   439|   443|			if (!_ui.removeFromIndex(i))
   440|   440|   440|   444|				return false;
   441|   441|   441|   445|
   442|   442|   442|   446|			for (size_t j = 0; j < _list.size(); ++j)
   443|   443|   443|   447|			{
   444|   444|   444|   448|				if (_list[j] == pi)
   445|   445|   445|   449|				{
   446|   446|   446|   450|					_list.erase(_list.begin() + j);
   447|   447|   447|   451|					return true;
   448|   448|   448|   452|				}
   449|   449|   449|   453|			}
   450|   450|   450|   454|		}
   451|   451|   451|   455|	}
   452|   452|   452|   456|	return false;
   453|   453|   453|   457|}
   454|   454|   454|   458|
   455|   455|   455|   459|void PluginViewList::pushBack(PluginUpdateInfo* pi)
   456|   456|   456|   460|{
   457|   457|   457|   461|	_list.push_back(pi);
   458|   458|   458|   462|
   459|   459|   459|   463|	vector<wstring> values2Add;
   460|   460|   460|   464|	values2Add.push_back(pi->_displayName);
   461|   461|   461|   465|	Version v = pi->_version;
   462|   462|   462|   466|	values2Add.push_back(v.toString());
   463|   463|   463|   467|
   464|   464|   464|   468|	// add in order
   465|   465|   465|   469|	size_t i = _ui.findAlphabeticalOrderPos(pi->_displayName, _sortType == DISPLAY_NAME_ALPHABET_ENCREASE ? _ui.sortEncrease : _ui.sortDecrease);
   466|   466|   466|   470|	_ui.addLine(values2Add, reinterpret_cast<LPARAM>(pi), static_cast<int>(i));
   467|   467|   467|   471|}
   468|   468|   468|   472|
   469|   469|   469|   473|// intervalVerStr format:
   470|   470|   470|   474|// 
   471|   471|   471|   475|// "6.9"          : exact version 6.9
   472|   472|   472|   476|// "[4.2,6.6.6]"  : from version 4.2 to 6.6.6 inclusive
   473|   473|   473|   477|// "[8.3,]"       : any version from 8.3 to the latest one
   474|   474|   474|   478|// "[,8.2.1]"     : 8.2.1 and any previous version
   475|   475|   475|   479|//
   476|   476|   476|   480|static std::pair<Version, Version> getIntervalVersions(std::wstring intervalVerStr)
   477|   477|   477|   481|{
   478|   478|   478|   482|	std::pair<Version, Version> result;
   479|   479|   479|   483|
   480|   480|   480|   484|	if (intervalVerStr.empty())
   481|   481|   481|   485|		return result;
   482|   482|   482|   486|
   483|   483|   483|   487|	const size_t indexEnd = intervalVerStr.length() - 1;
   484|   484|   484|   488|	if (intervalVerStr[0] == L'[' && intervalVerStr[indexEnd] == L']') // interval versions format
   485|   485|   485|   489|	{
   486|   486|   486|   490|		wstring cleanIntervalVerStr = intervalVerStr.substr(1, indexEnd - 1);
   487|   487|   487|   491|		vector<wstring> versionVect;
   488|   488|   488|   492|		cutStringBy(cleanIntervalVerStr.c_str(), versionVect, L',', true);
   489|   489|   489|   493|		if (versionVect.size() == 2)
   490|   490|   490|   494|		{
   491|   491|   491|   495|			if (!versionVect[0].empty() && !versionVect[1].empty()) // "[4.2,6.6.6]" : from version 4.2 to 6.6.6 inclusive
   492|   492|   492|   496|			{
   493|   493|   493|   497|				result.first = Version(versionVect[0]);
   494|   494|   494|   498|				result.second = Version(versionVect[1]);
   495|   495|   495|   499|			}
   496|   496|   496|   500|			else if (!versionVect[0].empty() && versionVect[1].empty()) // "[8.3,]" : any version from 8.3 to the latest one
   497|   497|   497|   501|