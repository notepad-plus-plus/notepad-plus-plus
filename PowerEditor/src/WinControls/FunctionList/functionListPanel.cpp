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

#include "json.hpp"
#include "functionListPanel.h"
#include "ScintillaEditView.h"
#include "localization.h"
#include <fstream>

using nlohmann::json;
using namespace std;

#define INDEX_ROOT        0
#define INDEX_NODE        1
#define INDEX_LEAF        2

#define FL_PREFERENCES_INITIALSORT_ID   1

FunctionListPanel::~FunctionListPanel()
{
	for (const auto s : _posStrs)
	{
		delete s;
	}
}

void FunctionListPanel::addEntry(const TCHAR *nodeName, const TCHAR *displayText, size_t pos)
{
	HTREEITEM itemParent = NULL;
	std::wstring posStr = std::to_wstring(pos);

	HTREEITEM root = _treeView.getRoot();

	if (nodeName != NULL && *nodeName != '\0')
	{
		itemParent = _treeView.searchSubItemByName(nodeName, root);
		if (!itemParent)
		{
			generic_string* invalidValueStr = new generic_string(posStr);
			_posStrs.push_back(invalidValueStr);
			LPARAM lParamInvalidPosStr = reinterpret_cast<LPARAM>(invalidValueStr);

			itemParent = _treeView.addItem(nodeName, root, INDEX_NODE, lParamInvalidPosStr);
		}
	}
	else
		itemParent = root;

	generic_string* posString = new generic_string(posStr);
	_posStrs.push_back(posString);
	LPARAM lParamPosStr = reinterpret_cast<LPARAM>(posString);

	_treeView.addItem(displayText, itemParent, INDEX_LEAF, lParamPosStr);
}

void FunctionListPanel::removeAllEntries()
{
	_treeView.removeAllItems();
}

// bodyOpenSybe mbol & bodyCloseSymbol should be RE
size_t FunctionListPanel::getBodyClosePos(size_t begin, const TCHAR *bodyOpenSymbol, const TCHAR *bodyCloseSymbol)
{
	size_t cntOpen = 1;

	size_t docLen = (*_ppEditView)->getCurrentDocLen();

	if (begin >= docLen)
		return docLen;

	generic_string exprToSearch = TEXT("(");
	exprToSearch += bodyOpenSymbol;
	exprToSearch += TEXT("|");
	exprToSearch += bodyCloseSymbol;
	exprToSearch += TEXT(")");


	int flags = SCFIND_REGEXP | SCFIND_POSIX;

	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	intptr_t targetStart = (*_ppEditView)->searchInTarget(exprToSearch.c_str(), exprToSearch.length(), begin, docLen);
	intptr_t targetEnd = 0;

	do
	{
		if (targetStart >= 0) // found open or close symbol
		{
			targetEnd = (*_ppEditView)->execute(SCI_GETTARGETEND);

			// Now we determinate the symbol (open or close)
			intptr_t tmpStart = (*_ppEditView)->searchInTarget(bodyOpenSymbol, lstrlen(bodyOpenSymbol), targetStart, targetEnd);
			if (tmpStart >= 0) // open symbol found
			{
				++cntOpen;
			}
			else // if it's not open symbol, then it must be the close one
			{
				--cntOpen;
			}
		}
		else // nothing found
		{
			cntOpen = 0; // get me out of here
			targetEnd = begin;
		}

		targetStart = (*_ppEditView)->searchInTarget(exprToSearch.c_str(), exprToSearch.length(), targetEnd, docLen);

	} while (cntOpen);

	return targetEnd;
}

generic_string FunctionListPanel::parseSubLevel(size_t begin, size_t end, std::vector< generic_string > dataToSearch, intptr_t& foundPos)
{
	if (begin >= end)
	{
		foundPos = -1;
		return TEXT("");
	}

	if (!dataToSearch.size())
		return TEXT("");

	int flags = SCFIND_REGEXP | SCFIND_POSIX;

	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	const TCHAR *regExpr2search = dataToSearch[0].c_str();
	intptr_t targetStart = (*_ppEditView)->searchInTarget(regExpr2search, lstrlen(regExpr2search), begin, end);

	if (targetStart < 0)
	{
		foundPos = -1;
		return TEXT("");
	}
	intptr_t targetEnd = (*_ppEditView)->execute(SCI_GETTARGETEND);

	if (dataToSearch.size() >= 2)
	{
		dataToSearch.erase(dataToSearch.begin());
		return parseSubLevel(targetStart, targetEnd, dataToSearch, foundPos);
	}
	else // only one processed element, so we conclude the result
	{
		TCHAR foundStr[1024];

		(*_ppEditView)->getGenericText(foundStr, 1024, targetStart, targetEnd);

		foundPos = targetStart;
		return foundStr;
	}
}

void FunctionListPanel::addInStateArray(TreeStateNode tree2Update, const TCHAR *searchText, bool isSorted)
{
	bool found = false;
	for (size_t i = 0, len = _treeParams.size(); i < len; ++i)
	{
		if (_treeParams[i]._treeState._extraData == tree2Update._extraData)
		{
			_treeParams[i]._searchParameters._text2Find = searchText;
			_treeParams[i]._searchParameters._doSort = isSorted;
			_treeParams[i]._treeState = tree2Update;
			found = true;
		}
	}
	if (!found)
	{
		TreeParams params;
		params._treeState = tree2Update;
		params._searchParameters._text2Find = searchText;
		params._searchParameters._doSort = isSorted;
		_treeParams.push_back(params);
	}
}

TreeParams* FunctionListPanel::getFromStateArray(generic_string fullFilePath)
{
	for (size_t i = 0, len = _treeParams.size(); i < len; ++i)
	{
		if (_treeParams[i]._treeState._extraData == fullFilePath)
			return &_treeParams[i];
	}
	return NULL;
}

void FunctionListPanel::sortOrUnsort()
{
	bool doSort = shouldSort();
	if (doSort)
		_pTreeView->sort(_pTreeView->getRoot(), true);
	else
	{
		TCHAR text2search[MAX_PATH] ;
		::SendMessage(_hSearchEdit, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(text2search));

		if (text2search[0] == '\0') // main view
		{
			_pTreeView->customSorting(_pTreeView->getRoot(), categorySortFunc, 0, true);
		}
		else // aux view
		{
			reload();

			if (_treeView.getRoot() == NULL)
				return;

			_treeViewSearchResult.removeAllItems();
			const TCHAR *fn = ((*_ppEditView)->getCurrentBuffer())->getFileName();

			generic_string* invalidValueStr = new generic_string(TEXT("-1"));
			_posStrs.push_back(invalidValueStr);
			LPARAM lParamInvalidPosStr = reinterpret_cast<LPARAM>(invalidValueStr);
			_treeViewSearchResult.addItem(fn, NULL, INDEX_ROOT, lParamInvalidPosStr);

			_treeView.searchLeafAndBuildTree(_treeViewSearchResult, text2search, INDEX_LEAF);
			_treeViewSearchResult.display(true);
			_treeViewSearchResult.expand(_treeViewSearchResult.getRoot());
			_treeView.display(false);
			_pTreeView = &_treeViewSearchResult;
		}
	}
}

int CALLBACK FunctionListPanel::categorySortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/)
{
	generic_string* posString1 = reinterpret_cast<generic_string*>(lParam1);
	generic_string* posString2 = reinterpret_cast<generic_string*>(lParam2);
	
	size_t pos1 = generic_atoi(posString1->c_str());
	size_t pos2 = generic_atoi(posString2->c_str());
	if (pos1 > pos2)
		return 1;
	else 
		return -1;
}

bool FunctionListPanel::serialize(const generic_string & outputFilename)
{
	Buffer* currentBuf = (*_ppEditView)->getCurrentBuffer();
	const TCHAR* fileNameLabel = currentBuf->getFileName();

	generic_string fname2write;
	if (outputFilename.empty()) // if outputFilename is not given, get the current file path by adding the file extension
	{
		const TCHAR *fullFilePath = currentBuf->getFullPathName();

		// Export function list from an existing file
		bool exportFuncntionList = (NppParameters::getInstance()).doFunctionListExport();
		if (exportFuncntionList && ::PathFileExists(fullFilePath))
		{
			fname2write = fullFilePath;
			fname2write += TEXT(".result");
			fname2write += TEXT(".json");
		}
		else
			return false;
	}
	else
	{
		fname2write = outputFilename;
	}

	const char* rootLabel = "root";
	const char* nodesLabel = "nodes";
	const char* leavesLabel = "leaves";
	const char* nameLabel = "name";

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	json j;
	j[rootLabel] = wmc.wchar2char(fileNameLabel, CP_ACP);

	for (const auto & info : _foundFuncInfos)
	{
		std::string leafName = wmc.wchar2char(info._data.c_str(), CP_ACP);

		if (!info._data2.empty()) // node
		{
			bool isFound = false;
			std::string nodeName = wmc.wchar2char(info._data2.c_str(), CP_ACP);

			for (auto & i : j[nodesLabel])
			{
				if (nodeName == i[nameLabel])
				{
					i[leavesLabel].push_back(leafName.c_str());
					isFound = true;
					break;
				}
			}

			if (!isFound)
			{
				json aNode = { { leavesLabel, json::array() },{ nameLabel, nodeName.c_str() } };
				aNode[leavesLabel].push_back(leafName.c_str());
				j[nodesLabel].push_back(aNode);
			}
		}
		else // leaf
		{
			j[leavesLabel].push_back(leafName.c_str());
		}
	}

	std::ofstream file(wmc.wchar2char(fname2write.c_str(), CP_ACP));
	file << j;

	return true;
}

void FunctionListPanel::reload()
{
	bool isScrollBarOn = GetWindowLongPtr(_treeView.getHSelf(), GWL_STYLE) & WS_VSCROLL;
	//get scroll position
	if (isScrollBarOn)
	{
		GetScrollInfo(_treeView.getHSelf(), SB_VERT, &si);
	}

	// clean up
	_findLine = -1;
	_findEndLine = -1;
	TreeStateNode currentTree;
	bool isOK = _treeView.retrieveFoldingStateTo(currentTree, _treeView.getRoot());
	if (isOK)
	{
		TCHAR text2Search[MAX_PATH];
		::SendMessage(_hSearchEdit, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(text2Search));
		bool isSorted =  shouldSort();
		addInStateArray(currentTree, text2Search, isSorted);
	}
	removeAllEntries();
	::SendMessage(_hSearchEdit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(TEXT("")));
	setSort(false);

	_foundFuncInfos.clear();

	Buffer* currentBuf = (*_ppEditView)->getCurrentBuffer();
	const TCHAR *fn = currentBuf->getFileName();
	LangType langID = currentBuf->getLangType();
	if (langID == L_JS)
		langID = L_JAVASCRIPT;

	const TCHAR *udln = NULL;
	if (langID == L_USER)
	{
		udln = currentBuf->getUserDefineLangName();
	}

	TCHAR *ext = ::PathFindExtension(fn);

	bool parsedOK = _funcParserMgr.parse(_foundFuncInfos, AssociationInfo(-1, langID, ext, udln));
	if (parsedOK)
	{
		generic_string* invalidValueStr = new generic_string(TEXT("-1"));
		_posStrs.push_back(invalidValueStr);
		LPARAM lParamInvalidPosStr = reinterpret_cast<LPARAM>(invalidValueStr);

		_treeView.addItem(fn, NULL, INDEX_ROOT, lParamInvalidPosStr);
	}

	for (size_t i = 0, len = _foundFuncInfos.size(); i < len; ++i)
	{
		addEntry(_foundFuncInfos[i]._data2.c_str(), _foundFuncInfos[i]._data.c_str(), _foundFuncInfos[i]._pos);
	}

	HTREEITEM root = _treeView.getRoot();

	if (root)
	{
		currentBuf = (*_ppEditView)->getCurrentBuffer();
		const TCHAR *fullFilePath = currentBuf->getFullPathName();

		generic_string* fullPathStr = new generic_string(fullFilePath);
		_posStrs.push_back(fullPathStr);
		LPARAM lParamFullPathStr = reinterpret_cast<LPARAM>(fullPathStr);

		_treeView.setItemParam(root, lParamFullPathStr);
		TreeParams *previousParams = getFromStateArray(fullFilePath);
		if (!previousParams)
		{
			::SendMessage(_hSearchEdit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(TEXT("")));
			setSort(NppParameters::getInstance().getNppGUI()._shouldSortFunctionList);
			sortOrUnsort();
			_treeView.expand(root);
		}
		else
		{
			::SendMessage(_hSearchEdit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>((previousParams->_searchParameters)._text2Find.c_str()));

			bool isSort = (previousParams->_searchParameters)._doSort;
			setSort(isSort);
			if (isSort)
				_pTreeView->sort(_pTreeView->getRoot(), true);

			_treeView.restoreFoldingStateFrom(previousParams->_treeState, root);
		}
	}

	// invalidate the editor rect
	::InvalidateRect(_hSearchEdit, NULL, TRUE);

	//set scroll position
	if (isScrollBarOn)
	{
		SetScrollInfo(_treeView.getHSelf(), SB_VERT, &si, TRUE);
	}
}

void FunctionListPanel::initPreferencesMenu()
{
	NativeLangSpeaker* pNativeSpeaker = NppParameters::getInstance().getNativeLangSpeaker();
	NppGUI& nppGUI = NppParameters::getInstance().getNppGUI();

	generic_string shouldSortFunctionListStr = pNativeSpeaker->getAttrNameStr(TEXT("Sort functions (A to Z) by default"), FL_FUCTIONLISTROOTNODE, FL_PREFERENCE_INITIALSORT);

	_hPreferencesMenu = ::CreatePopupMenu();
	::InsertMenu(_hPreferencesMenu, 0, MF_BYCOMMAND, FL_PREFERENCES_INITIALSORT_ID, shouldSortFunctionListStr.c_str());
	::CheckMenuItem(_hPreferencesMenu, FL_PREFERENCES_INITIALSORT_ID, MF_BYCOMMAND | (nppGUI._shouldSortFunctionList ? MF_CHECKED : MF_UNCHECKED));
}

void FunctionListPanel::showPreferencesMenu()
{
	RECT rectToolbar;
	RECT rectPreferencesButton;
	::GetWindowRect(_hToolbarMenu, &rectToolbar);
	::SendMessage(_hToolbarMenu, TB_GETRECT, IDC_PREFERENCEBUTTON_FUNCLIST, (LPARAM)&rectPreferencesButton);

	::TrackPopupMenu(_hPreferencesMenu,
		NppParameters::getInstance().getNativeLangSpeaker()->isRTL() ? TPM_RIGHTALIGN | TPM_LAYOUTRTL : TPM_LEFTALIGN,
		rectToolbar.left + rectPreferencesButton.left,
		rectToolbar.top + rectPreferencesButton.bottom,
		0, _hSelf, NULL);
}

void FunctionListPanel::markEntry()
{
	LONG lineNr = static_cast<LONG>((*_ppEditView)->getCurrentLineNumber());
	HTREEITEM root = _treeView.getRoot();
	if (_findLine != -1 && _findEndLine != -1 && lineNr >= _findLine && lineNr < _findEndLine)
		return;
	_findLine = -1;
	_findEndLine = -1;
	findMarkEntry(root, lineNr);
	if (_findLine != -1)
	{
		_treeView.selectItem(_findItem);
	}
	else
	{
		_treeView.selectItem(root);
	}

}

void FunctionListPanel::findMarkEntry(HTREEITEM htItem, LONG line)
{
	HTREEITEM cItem;
	TVITEM tvItem;
	for (; htItem != NULL; htItem = _treeView.getNextSibling(htItem))
	{
		cItem = _treeView.getChildFrom(htItem);
		if (cItem != NULL)
		{
			findMarkEntry(cItem, line);
		}
		else
		{
			tvItem.hItem = htItem;
			tvItem.mask = TVIF_IMAGE | TVIF_PARAM;
			::SendMessage(_treeViewSearchResult.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

			generic_string *posStr = reinterpret_cast<generic_string *>(tvItem.lParam);
			if (posStr)
			{
				int pos = generic_atoi(posStr->c_str());
				if (pos != -1)
				{
					LONG sci_line = static_cast<LONG>((*_ppEditView)->execute(SCI_LINEFROMPOSITION, pos));
					if (line >= sci_line)
					{
						if (sci_line > _findLine || _findLine == -1)
						{
							_findLine = sci_line;
							_findItem = htItem;
						}
					}
					else
					{
						if (sci_line < _findEndLine)
							_findEndLine = sci_line;
					}
				}
			}
		}
	}
}

void FunctionListPanel::init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView)
{
	DockingDlgInterface::init(hInst, hPere);
	_ppEditView = ppEditView;
	NppParameters& nppParams = NppParameters::getInstance();

	generic_string funcListXmlPath = nppParams.getUserPath();
	pathAppend(funcListXmlPath, TEXT("functionList"));

	generic_string funcListDefaultXmlPath = nppParams.getNppPath();
	pathAppend(funcListDefaultXmlPath, TEXT("functionList"));

	bool doLocalConf = nppParams.isLocal();

	if (!doLocalConf)
	{
		if (!PathFileExists(funcListXmlPath.c_str()))
		{
			if (PathFileExists(funcListDefaultXmlPath.c_str()))
			{
				::CopyFile(funcListDefaultXmlPath.c_str(), funcListXmlPath.c_str(), TRUE);
				_funcParserMgr.init(funcListXmlPath, funcListDefaultXmlPath, ppEditView);
			}
		}
		else
		{
			_funcParserMgr.init(funcListXmlPath, funcListDefaultXmlPath, ppEditView);
		}
	}
	else
	{
		generic_string funcListDefaultXmlPath = nppParams.getNppPath();
		pathAppend(funcListDefaultXmlPath, TEXT("functionList"));
		if (PathFileExists(funcListDefaultXmlPath.c_str()))
		{
			_funcParserMgr.init(funcListDefaultXmlPath, funcListDefaultXmlPath, ppEditView);
		}
	}

	//init scrollinfo structure
	ZeroMemory(&si, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS;
}

bool FunctionListPanel::openSelection(const TreeView & treeView)
{
	TVITEM tvItem;
	tvItem.mask = TVIF_IMAGE | TVIF_PARAM;
	tvItem.hItem = treeView.getSelection();
	::SendMessage(treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

	if (tvItem.iImage == INDEX_ROOT || tvItem.iImage == INDEX_NODE)
	{
		return false;
	}

	generic_string *posStr = reinterpret_cast<generic_string *>(tvItem.lParam);
	if (!posStr)
		return false;

	int pos = generic_atoi(posStr->c_str());
	if (pos == -1)
		return false;

	auto sci_line = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, pos);
	(*_ppEditView)->execute(SCI_ENSUREVISIBLE, sci_line);
	(*_ppEditView)->scrollPosToCenter(pos);

	return true;
}

void FunctionListPanel::notified(LPNMHDR notification)
{
	if (notification->code == TTN_GETDISPINFO)
	{
		LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)notification;
		lpttt->hinst = NULL;

		if (notification->idFrom == IDC_SORTBUTTON_FUNCLIST)
		{
			wcscpy_s(lpttt->szText, _sortTipStr.c_str());
		}
		else if (notification->idFrom == IDC_RELOADBUTTON_FUNCLIST)
		{
			wcscpy_s(lpttt->szText, _reloadTipStr.c_str());
		}
		else if (notification->idFrom == IDC_PREFERENCEBUTTON_FUNCLIST)
		{
			wcscpy_s(lpttt->szText, _preferenceTipStr.c_str());
		}
	}
	else if (notification->hwndFrom == _treeView.getHSelf() || notification->hwndFrom == _treeViewSearchResult.getHSelf())
	{
		const TreeView & treeView = notification->hwndFrom == _treeView.getHSelf()?_treeView:_treeViewSearchResult;
		switch (notification->code)
		{
			case NM_DBLCLK:
			{
				openSelection(treeView);
				PostMessage(_hParent, WM_COMMAND, SCEN_SETFOCUS << 16, reinterpret_cast<LPARAM>((*_ppEditView)->getHSelf()));
			}
			break;

			case NM_RETURN:
				SetWindowLongPtr(_hSelf, DWLP_MSGRESULT, 1); // remove beep
			break;

			case TVN_KEYDOWN:
			{
				LPNMTVKEYDOWN ptvkd = (LPNMTVKEYDOWN)notification;

				if (ptvkd->wVKey == VK_RETURN)
				{
					if (!openSelection(treeView))
					{
						HTREEITEM hItem = treeView.getSelection();
						treeView.toggleExpandCollapse(hItem);
						break;
					}
					PostMessage(_hParent, WM_COMMAND, SCEN_SETFOCUS << 16, reinterpret_cast<LPARAM>((*_ppEditView)->getHSelf()));
				}
				else if (ptvkd->wVKey == VK_TAB)
				{
					::SetFocus(_hSearchEdit);
					SetWindowLongPtr(_hSelf, DWLP_MSGRESULT, 1); // remove beep
				}
				else if (ptvkd->wVKey == VK_ESCAPE)
				{
					::SendMessage(_hSearchEdit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(TEXT("")));
					SetWindowLongPtr(_hSelf, DWLP_MSGRESULT, 1); // remove beep
					PostMessage(_hParent, WM_COMMAND, SCEN_SETFOCUS << 16, reinterpret_cast<LPARAM>((*_ppEditView)->getHSelf()));
				}
			}
			break;
		}
	}
	else if (notification->code == DMN_SWITCHIN)
	{
		reload();
	}
	else if (notification->code == DMN_CLOSE)
	{
		::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_FUNC_LIST, 0);
	}
}

void FunctionListPanel::searchFuncAndSwitchView()
{
	TCHAR text2search[MAX_PATH] ;
	::SendMessage(_hSearchEdit, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(text2search));

	if (text2search[0] == '\0')
	{
		_treeViewSearchResult.display(false);
		_treeView.display(true);
		_pTreeView = &_treeView;
	}
	else
	{
		if (_treeView.getRoot() == NULL)
			return;

		_treeViewSearchResult.removeAllItems();
		const TCHAR *fn = ((*_ppEditView)->getCurrentBuffer())->getFileName();

		generic_string* invalidValueStr = new generic_string(TEXT("-1"));
		_posStrs.push_back(invalidValueStr);
		LPARAM lParamInvalidPosStr = reinterpret_cast<LPARAM>(invalidValueStr);
		_treeViewSearchResult.addItem(fn, NULL, INDEX_ROOT, lParamInvalidPosStr);

		_treeView.searchLeafAndBuildTree(_treeViewSearchResult, text2search, INDEX_LEAF);
		_treeViewSearchResult.display(true);
		_treeViewSearchResult.expand(_treeViewSearchResult.getRoot());
		_treeView.display(false);
		_pTreeView = &_treeViewSearchResult;

		// invalidate the editor rect
		::InvalidateRect(_hSearchEdit, NULL, TRUE);
	}

	// restore selected sorting
	if (shouldSort())
		_pTreeView->sort(_pTreeView->getRoot(), true);
	else
		_pTreeView->customSorting(_pTreeView->getRoot(), categorySortFunc, 0, true);
}

static WNDPROC oldFunclstToolbarProc = NULL;
static LRESULT CALLBACK funclstToolbarProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CTLCOLOREDIT :
		{
			return ::SendMessage(::GetParent(hwnd), WM_CTLCOLOREDIT, wParam, lParam);
		}
	}
	return oldFunclstToolbarProc(hwnd, message, wParam, lParam);
}

static WNDPROC oldFunclstSearchEditProc = NULL;
static LRESULT CALLBACK funclstSearchEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CHAR:
		{
			if (wParam == VK_ESCAPE)
			{
				::SendMessage(hwnd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(TEXT("")));
				return FALSE;
			}
			else if (wParam == VK_TAB)
			{
				::SendMessage(GetParent(hwnd), WM_COMMAND, VK_TAB, 1);
				return FALSE;
			}
		}
	}
	return oldFunclstSearchEditProc(hwnd, message, wParam, lParam);
}

bool FunctionListPanel::shouldSort()
{
	TBBUTTONINFO tbbuttonInfo;
	tbbuttonInfo.cbSize = sizeof(TBBUTTONINFO);
	tbbuttonInfo.dwMask = TBIF_STATE;

	::SendMessage(_hToolbarMenu, TB_GETBUTTONINFO, IDC_SORTBUTTON_FUNCLIST, reinterpret_cast<LPARAM>(&tbbuttonInfo));

	return (tbbuttonInfo.fsState & TBSTATE_CHECKED) != 0;
}

void FunctionListPanel::setSort(bool isEnabled)
{
	TBBUTTONINFO tbbuttonInfo;
	tbbuttonInfo.cbSize = sizeof(TBBUTTONINFO);
	tbbuttonInfo.dwMask = TBIF_STATE;
	tbbuttonInfo.fsState = isEnabled ? TBSTATE_ENABLED | TBSTATE_CHECKED : TBSTATE_ENABLED;
	::SendMessage(_hToolbarMenu, TB_SETBUTTONINFO, IDC_SORTBUTTON_FUNCLIST, reinterpret_cast<LPARAM>(&tbbuttonInfo));
}

intptr_t CALLBACK FunctionListPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		// Make edit field red if not found
		case WM_CTLCOLOREDIT :
		{
			TCHAR text2search[MAX_PATH] ;
			::SendMessage(_hSearchEdit, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(text2search));
			bool textFound = false;
			if (text2search[0] == '\0')
			{
				textFound = true; // no text, use the default color
			}

			if (!textFound)
			{
				HTREEITEM searchViewRoot = _treeViewSearchResult.getRoot();
				if (searchViewRoot)
				{
					if (_treeViewSearchResult.getChildFrom(searchViewRoot))
					{
						textFound = true; // children on root found, use the default color
					}
				}
				else
				{
					textFound = true; // no root (no parser), use the default color
				}
			}

			auto hdc = reinterpret_cast<HDC>(wParam);

			if (NppDarkMode::isEnabled())
			{
				if (textFound)
				{
					return NppDarkMode::onCtlColorSofter(hdc);
				}
				else // text not found
				{
					return NppDarkMode::onCtlColorError(hdc);
				}
			}

			if (textFound)
			{
				return FALSE;
			}

			// text not found
			// if the text not found modify the background color of the editor
			static HBRUSH hBrushBackground = CreateSolidBrush(BCKGRD_COLOR);
			SetTextColor(hdc, TXT_COLOR);
			SetBkColor(hdc, BCKGRD_COLOR);
			return reinterpret_cast<LRESULT>(hBrushBackground);
		}

		case WM_INITDIALOG :
		{
			FunctionListPanel::initPreferencesMenu();

			NppParameters& nppParams = NppParameters::getInstance();

			int editWidth = nppParams._dpiManager.scaleX(100);
			int editWidthSep = nppParams._dpiManager.scaleX(105); //editWidth + 5
			int editHeight = nppParams._dpiManager.scaleY(20);

			// Create toolbar menu
			int style = WS_CHILD | WS_VISIBLE | CCS_ADJUSTABLE | TBSTYLE_AUTOSIZE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | BTNS_AUTOSIZE | BTNS_SEP | TBSTYLE_TOOLTIPS;
			_hToolbarMenu = CreateWindowEx(0,TOOLBARCLASSNAME,NULL, style,
								0,0,0,0,_hSelf,nullptr, _hInst, NULL);

			oldFunclstToolbarProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hToolbarMenu, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(funclstToolbarProc)));

			// Add the bmap image into toolbar's imagelist
			int iconSizeDyn = nppParams._dpiManager.scaleX(16);
			::SendMessage(_hToolbarMenu, TB_SETBITMAPSIZE, 0, MAKELPARAM(iconSizeDyn, iconSizeDyn));

			TBADDBITMAP addbmp = { 0, 0 };
			const int nbIcons = 3;
			int iconIDs[nbIcons] = { IDI_FUNCLIST_SORTBUTTON, IDI_FUNCLIST_RELOADBUTTON, IDI_FUNCLIST_PREFERENCEBUTTON };
			int iconDarkModeIDs[nbIcons] = { IDI_FUNCLIST_SORTBUTTON_DM, IDI_FUNCLIST_RELOADBUTTON_DM, IDI_FUNCLIST_PREFERENCEBUTTON_DM };
			for (size_t i = 0; i < nbIcons; ++i)
			{
				int icoID = NppDarkMode::isEnabled() ? iconDarkModeIDs[i] : iconIDs[i];
				HBITMAP hBmp = static_cast<HBITMAP>(::LoadImage(_hInst, MAKEINTRESOURCE(icoID), IMAGE_BITMAP, iconSizeDyn, iconSizeDyn, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT));
				addbmp.nID = reinterpret_cast<UINT_PTR>(hBmp);
				::SendMessage(_hToolbarMenu, TB_ADDBITMAP, 1, reinterpret_cast<LPARAM>(&addbmp));
			}

			// Place holder of search text field
			TBBUTTON tbButtons[1 + nbIcons];

			tbButtons[0].idCommand = 0;
			tbButtons[0].iBitmap = editWidthSep;
			tbButtons[0].fsState = TBSTATE_ENABLED;
			tbButtons[0].fsStyle = BTNS_SEP; //This is just a separator (blank space)
			tbButtons[0].iString = 0;

			tbButtons[1].idCommand = IDC_SORTBUTTON_FUNCLIST;
			tbButtons[1].iBitmap = 0;
			tbButtons[1].fsState = TBSTATE_ENABLED;
			tbButtons[1].fsStyle = BTNS_CHECK | BTNS_AUTOSIZE;
			tbButtons[1].iString = reinterpret_cast<intptr_t>(TEXT(""));

			tbButtons[2].idCommand = IDC_RELOADBUTTON_FUNCLIST;
			tbButtons[2].iBitmap = 1;
			tbButtons[2].fsState = TBSTATE_ENABLED;
			tbButtons[2].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
			tbButtons[2].iString = reinterpret_cast<intptr_t>(TEXT(""));

			tbButtons[3].idCommand = IDC_PREFERENCEBUTTON_FUNCLIST;
			tbButtons[3].iBitmap = 2;
			tbButtons[3].fsState = TBSTATE_ENABLED;
			tbButtons[3].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
			tbButtons[3].iString = reinterpret_cast<intptr_t>(TEXT(""));

			::SendMessage(_hToolbarMenu, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
			::SendMessage(_hToolbarMenu, TB_SETBUTTONSIZE, 0, MAKELONG(nppParams._dpiManager.scaleX(16), nppParams._dpiManager.scaleY(16)));
			::SendMessage(_hToolbarMenu, TB_ADDBUTTONS, sizeof(tbButtons) / sizeof(TBBUTTON), reinterpret_cast<LPARAM>(&tbButtons));
			::SendMessage(_hToolbarMenu, TB_AUTOSIZE, 0, 0);

			ShowWindow(_hToolbarMenu, SW_SHOW);

			// tips text for toolbar buttons
			NativeLangSpeaker *pNativeSpeaker = nppParams.getNativeLangSpeaker();
			_sortTipStr = pNativeSpeaker->getAttrNameStr(_sortTipStr.c_str(), FL_FUCTIONLISTROOTNODE, FL_SORTLOCALNODENAME);
			_reloadTipStr = pNativeSpeaker->getAttrNameStr(_reloadTipStr.c_str(), FL_FUCTIONLISTROOTNODE, FL_RELOADLOCALNODENAME);
			_preferenceTipStr = pNativeSpeaker->getAttrNameStr(_preferenceTipStr.c_str(), FL_FUCTIONLISTROOTNODE, FL_PREFERENCESLOCALNODENAME);

			_hSearchEdit = CreateWindowEx(0, L"Edit", NULL,
								WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOVSCROLL,
								2, 2, editWidth, editHeight,
								_hToolbarMenu, reinterpret_cast<HMENU>(IDC_SEARCHFIELD_FUNCLIST), _hInst, 0 );

			oldFunclstSearchEditProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSearchEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(funclstSearchEditProc)));

			HFONT hf = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
			if (hf)
				::SendMessage(_hSearchEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hf), MAKELPARAM(TRUE, 0));

			_treeView.init(_hInst, _hSelf, IDC_LIST_FUNCLIST);
			_treeView.setImageList(CX_BITMAP, CY_BITMAP, 3, IDI_FUNCLIST_ROOT, IDI_FUNCLIST_NODE, IDI_FUNCLIST_LEAF);
			_treeViewSearchResult.init(_hInst, _hSelf, IDC_LIST_FUNCLIST_AUX);
			_treeViewSearchResult.setImageList(CX_BITMAP, 3, CY_BITMAP, IDI_FUNCLIST_ROOT, IDI_FUNCLIST_NODE, IDI_FUNCLIST_LEAF);
			
			_treeView.makeLabelEditable(false);

			_treeView.display();

			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
			NppDarkMode::autoSubclassAndThemeWindowNotify(_hSelf);

			return TRUE;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			if (static_cast<BOOL>(lParam) != TRUE)
			{
				NppDarkMode::autoThemeChildControls(_hSelf);
			}
			NppDarkMode::setTreeViewStyle(_treeView.getHSelf());
			return TRUE;
		}

		case WM_DESTROY:
			_treeView.destroy();
			_treeViewSearchResult.destroy();
			::DestroyMenu(_hPreferencesMenu);
			::DestroyWindow(_hToolbarMenu);
			break;

		case WM_COMMAND :
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				switch (LOWORD(wParam))
				{
					case  IDC_SEARCHFIELD_FUNCLIST:
					{
						searchFuncAndSwitchView();
						return TRUE;
					}
				}
			}
			else if (wParam == VK_TAB)
			{
				if (_treeViewSearchResult.isVisible())
					::SetFocus(_treeViewSearchResult.getHSelf());
				else
					::SetFocus(_treeView.getHSelf());
				return TRUE;
			}

			switch (LOWORD(wParam))
			{
				case IDC_SORTBUTTON_FUNCLIST:
				{
					sortOrUnsort();
				}
				return TRUE;

				case IDC_RELOADBUTTON_FUNCLIST:
				{
					reload();
				}
				return TRUE;

				case IDC_PREFERENCEBUTTON_FUNCLIST:
				{
					showPreferencesMenu();
				}
				return TRUE;

				case FL_PREFERENCES_INITIALSORT_ID:
				{
					bool& shouldSortFunctionList = NppParameters::getInstance().getNppGUI()._shouldSortFunctionList;
					shouldSortFunctionList = !shouldSortFunctionList;
					::CheckMenuItem(_hPreferencesMenu, FL_PREFERENCES_INITIALSORT_ID, MF_BYCOMMAND | (shouldSortFunctionList ? MF_CHECKED : MF_UNCHECKED));
				}
				return TRUE;
			}
		}
		break;

		case WM_NOTIFY:
		{
			notified((LPNMHDR)lParam);
		}
		return TRUE;

		case WM_SIZE:
		{
			int width = LOWORD(lParam);
			int height = HIWORD(lParam);
			int extraValue = NppParameters::getInstance()._dpiManager.scaleX(4);

			RECT toolbarMenuRect;
			::GetClientRect(_hToolbarMenu, &toolbarMenuRect);

			::MoveWindow(_hToolbarMenu, 0, 0, width, toolbarMenuRect.bottom, TRUE);

			HWND hwnd = _treeView.getHSelf();
			if (hwnd)
				::MoveWindow(hwnd, 0, toolbarMenuRect.bottom + extraValue, width, height - toolbarMenuRect.bottom - extraValue, TRUE);

			HWND hwnd_aux = _treeViewSearchResult.getHSelf();
			if (hwnd_aux)
				::MoveWindow(hwnd_aux, 0, toolbarMenuRect.bottom + extraValue, width, height - toolbarMenuRect.bottom - extraValue, TRUE);

			break;
		}

		default :
			return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
	}
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}
