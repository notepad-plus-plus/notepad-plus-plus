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

	if (_hFontSearchEdit != nullptr)
	{
		::DeleteObject(_hFontSearchEdit);
		_hFontSearchEdit = nullptr;
	}

	for (auto hImgList : _iconListVector)
	{
		if (hImgList != nullptr)
		{
			::ImageList_Destroy(hImgList);
		}
	}
	_iconListVector.clear();
}

void FunctionListPanel::addEntry(const wchar_t *nodeName, const wchar_t *displayText, size_t pos)
{
	HTREEITEM itemParent = NULL;
	std::wstring posStr = std::to_wstring(pos);

	HTREEITEM root = _treeView.getRoot();

	if (nodeName != NULL && *nodeName != '\0')
	{
		itemParent = _treeView.searchSubItemByName(nodeName, root);
		if (!itemParent)
		{
			wstring* invalidValueStr = new wstring(posStr);
			_posStrs.push_back(invalidValueStr);
			LPARAM lParamInvalidPosStr = reinterpret_cast<LPARAM>(invalidValueStr);

			itemParent = _treeView.addItem(nodeName, root, INDEX_NODE, lParamInvalidPosStr);
		}
	}
	else
		itemParent = root;

	wstring* posString = new wstring(posStr);
	_posStrs.push_back(posString);
	LPARAM lParamPosStr = reinterpret_cast<LPARAM>(posString);

	_treeView.addItem(displayText, itemParent, INDEX_LEAF, lParamPosStr);
}

void FunctionListPanel::removeAllEntries()
{
	_treeView.removeAllItems();
}

// bodyOpenSybe mbol & bodyCloseSymbol should be RE
size_t FunctionListPanel::getBodyClosePos(size_t begin, const wchar_t *bodyOpenSymbol, const wchar_t *bodyCloseSymbol)
{
	size_t cntOpen = 1;

	size_t docLen = (*_ppEditView)->getCurrentDocLen();

	if (begin >= docLen)
		return docLen;

	wstring exprToSearch = L"(";
	exprToSearch += bodyOpenSymbol;
	exprToSearch += L"|";
	exprToSearch += bodyCloseSymbol;
	exprToSearch += L")";


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

wstring FunctionListPanel::parseSubLevel(size_t begin, size_t end, std::vector< wstring > dataToSearch, intptr_t& foundPos)
{
	if (begin >= end)
	{
		foundPos = -1;
		return L"";
	}

	if (!dataToSearch.size())
		return L"";

	int flags = SCFIND_REGEXP | SCFIND_POSIX;

	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	const wchar_t *regExpr2search = dataToSearch[0].c_str();
	intptr_t targetStart = (*_ppEditView)->searchInTarget(regExpr2search, lstrlen(regExpr2search), begin, end);

	if (targetStart < 0)
	{
		foundPos = -1;
		return L"";
	}
	intptr_t targetEnd = (*_ppEditView)->execute(SCI_GETTARGETEND);

	if (dataToSearch.size() >= 2)
	{
		dataToSearch.erase(dataToSearch.begin());
		return parseSubLevel(targetStart, targetEnd, dataToSearch, foundPos);
	}
	else // only one processed element, so we conclude the result
	{
		wchar_t foundStr[1024]{};

		(*_ppEditView)->getGenericText(foundStr, 1024, targetStart, targetEnd);

		foundPos = targetStart;
		return foundStr;
	}
}

void FunctionListPanel::addInStateArray(TreeStateNode tree2Update, const wchar_t *searchText, bool isSorted)
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

TreeParams* FunctionListPanel::getFromStateArray(const wstring& fullFilePath)
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
		wchar_t text2search[MAX_PATH] = { '\0' };
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
			const wchar_t *fn = ((*_ppEditView)->getCurrentBuffer())->getFileName();

			wstring* invalidValueStr = new wstring(L"-1");
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
	wstring* posString1 = reinterpret_cast<wstring*>(lParam1);
	wstring* posString2 = reinterpret_cast<wstring*>(lParam2);
	
	size_t pos1 = _wtoi(posString1->c_str());
	size_t pos2 = _wtoi(posString2->c_str());
	if (pos1 > pos2)
		return 1;
	else 
		return -1;
}

bool FunctionListPanel::serialize(const wstring & outputFilename)
{
	Buffer* currentBuf = (*_ppEditView)->getCurrentBuffer();
	const wchar_t* fileNameLabel = currentBuf->getFileName();

	wstring fname2write;
	if (outputFilename.empty()) // if outputFilename is not given, get the current file path by adding the file extension
	{
		const wchar_t *fullFilePath = currentBuf->getFullPathName();

		// Export function list from an existing file
		bool exportFuncntionList = (NppParameters::getInstance()).doFunctionListExport();
		if (exportFuncntionList && doesFileExist(fullFilePath))
		{
			fname2write = fullFilePath;
			fname2write += L".result";
			fname2write += L".json";
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
				if (nodeName == std::string{ i[nameLabel] })
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
		wchar_t text2Search[MAX_PATH] = { '\0' };
		::SendMessage(_hSearchEdit, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(text2Search));
		bool isSorted =  shouldSort();
		addInStateArray(currentTree, text2Search, isSorted);
	}
	removeAllEntries();
	::SendMessage(_hSearchEdit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L""));
	setSort(false);

	_foundFuncInfos.clear();

	Buffer* currentBuf = (*_ppEditView)->getCurrentBuffer();
	const wchar_t *fn = currentBuf->getFileName();
	LangType langID = currentBuf->getLangType();
	if (langID == L_JS)
		langID = L_JAVASCRIPT;

	const wchar_t *udln = NULL;
	if (langID == L_USER)
	{
		udln = currentBuf->getUserDefineLangName();
	}

	const wchar_t* ext = ::PathFindExtension(fn);

	bool parsedOK = _funcParserMgr.parse(_foundFuncInfos, AssociationInfo(-1, langID, ext, udln));
	if (parsedOK)
	{
		wstring* invalidValueStr = new wstring(L"-1");
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
		const wchar_t *fullFilePath = currentBuf->getFullPathName();

		wstring* fullPathStr = new wstring(fullFilePath);
		_posStrs.push_back(fullPathStr);
		LPARAM lParamFullPathStr = reinterpret_cast<LPARAM>(fullPathStr);

		_treeView.setItemParam(root, lParamFullPathStr);
		TreeParams *previousParams = getFromStateArray(fullFilePath);
		if (!previousParams)
		{
			::SendMessage(_hSearchEdit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L""));
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
	const NppGUI& nppGUI = NppParameters::getInstance().getNppGUI();

	wstring shouldSortFunctionListStr = pNativeSpeaker->getAttrNameStr(L"Sort functions (A to Z) by default", FL_FUCTIONLISTROOTNODE, FL_PREFERENCE_INITIALSORT);

	_hPreferencesMenu = ::CreatePopupMenu();
	::InsertMenu(_hPreferencesMenu, 0, MF_BYCOMMAND, FL_PREFERENCES_INITIALSORT_ID, shouldSortFunctionListStr.c_str());
	::CheckMenuItem(_hPreferencesMenu, FL_PREFERENCES_INITIALSORT_ID, MF_BYCOMMAND | (nppGUI._shouldSortFunctionList ? MF_CHECKED : MF_UNCHECKED));
}

void FunctionListPanel::showPreferencesMenu()
{
	RECT rectToolbar{};
	RECT rectPreferencesButton{};
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
	HTREEITEM cItem{};
	TVITEM tvItem{};
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

			wstring *posStr = reinterpret_cast<wstring *>(tvItem.lParam);
			if (posStr)
			{
				int pos = _wtoi(posStr->c_str());
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

	wstring funcListXmlPath = nppParams.getUserPath();
	pathAppend(funcListXmlPath, L"functionList");

	wstring funcListDefaultXmlPath = nppParams.getNppPath();
	pathAppend(funcListDefaultXmlPath, L"functionList");

	bool doLocalConf = nppParams.isLocal();

	if (!doLocalConf)
	{
		if (!doesDirectoryExist(funcListXmlPath.c_str()))
		{
			if (doesDirectoryExist(funcListDefaultXmlPath.c_str()))
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
		if (doesDirectoryExist(funcListDefaultXmlPath.c_str()))
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
	TVITEM tvItem{};
	tvItem.mask = TVIF_IMAGE | TVIF_PARAM;
	tvItem.hItem = treeView.getSelection();
	::SendMessage(treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

	if (tvItem.iImage == INDEX_ROOT || tvItem.iImage == INDEX_NODE)
	{
		return false;
	}

	wstring *posStr = reinterpret_cast<wstring *>(tvItem.lParam);
	if (!posStr)
		return false;

	int pos = _wtoi(posStr->c_str());
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
					::SendMessage(_hSearchEdit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L""));
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
	wchar_t text2search[MAX_PATH] = { '\0' };
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
		const wchar_t *fn = ((*_ppEditView)->getCurrentBuffer())->getFileName();

		wstring* invalidValueStr = new wstring(L"-1");
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

static LRESULT CALLBACK funclstToolbarProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/)
{
	switch (message)
	{
		case WM_NCDESTROY:
		{
			::RemoveWindowSubclass(hwnd, funclstToolbarProc, uIdSubclass);
			break;
		}

		case WM_CTLCOLOREDIT :
		{
			return ::SendMessage(::GetParent(hwnd), WM_CTLCOLOREDIT, wParam, lParam);
		}
	}
	return ::DefSubclassProc(hwnd, message, wParam, lParam);
}

static LRESULT CALLBACK funclstSearchEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/)
{
	switch (message)
	{
		case WM_NCDESTROY:
		{
			::RemoveWindowSubclass(hwnd, funclstSearchEditProc, uIdSubclass);
			break;
		}

		case WM_CHAR:
		{
			if (wParam == VK_ESCAPE)
			{
				::SendMessage(hwnd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L""));
				return 0;
			}
			else if (wParam == VK_TAB)
			{
				::SendMessage(GetParent(hwnd), WM_COMMAND, VK_TAB, 1);
				return 0;
			}
		}
	}
	return ::DefSubclassProc(hwnd, message, wParam, lParam);
}

bool FunctionListPanel::shouldSort()
{
	TBBUTTONINFO tbbuttonInfo{};
	tbbuttonInfo.cbSize = sizeof(TBBUTTONINFO);
	tbbuttonInfo.dwMask = TBIF_STATE;

	::SendMessage(_hToolbarMenu, TB_GETBUTTONINFO, IDC_SORTBUTTON_FUNCLIST, reinterpret_cast<LPARAM>(&tbbuttonInfo));

	return (tbbuttonInfo.fsState & TBSTATE_CHECKED) != 0;
}

void FunctionListPanel::setSort(bool isEnabled)
{
	TBBUTTONINFO tbbuttonInfo{};
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
			wchar_t text2search[MAX_PATH] = { '\0' };
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
					return NppDarkMode::onCtlColorCtrl(hdc);
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

		case WM_INITDIALOG:
		{
			FunctionListPanel::initPreferencesMenu();

			NppParameters& nppParam = NppParameters::getInstance();

			setDpi();
			const int editWidth = _dpiManager.scale(100);
			const int editWidthSep = _dpiManager.scale(105); //editWidth + 5
			const int editHeight = _dpiManager.scale(20);

			// Create toolbar menu
			constexpr DWORD style = WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS;
			_hToolbarMenu = CreateWindowEx(0,TOOLBARCLASSNAME,NULL, style,
								0,0,0,0,_hSelf,nullptr, _hInst, NULL);

			const DWORD tbExStyle = static_cast<DWORD>(::SendMessage(_hToolbarMenu, TB_GETEXTENDEDSTYLE, 0, 0));
			::SendMessage(_hToolbarMenu, TB_SETEXTENDEDSTYLE, 0, tbExStyle | TBSTYLE_EX_DOUBLEBUFFER);

			constexpr UINT_PTR idSubclassFunclstToolbar = 2;
			::SetWindowSubclass(_hToolbarMenu, funclstToolbarProc, idSubclassFunclstToolbar, 0);

			const int iconSizeDyn = _dpiManager.scale(16);
			constexpr int nbIcons = 3;
			int iconIDs[nbIcons] = { IDI_FUNCLIST_SORTBUTTON, IDI_FUNCLIST_RELOADBUTTON, IDI_FUNCLIST_PREFERENCEBUTTON };
			int iconDarkModeIDs[nbIcons] = { IDI_FUNCLIST_SORTBUTTON_DM, IDI_FUNCLIST_RELOADBUTTON_DM, IDI_FUNCLIST_PREFERENCEBUTTON_DM };

			// Create an image lists for the toolbar icons
			HIMAGELIST hImageList = ImageList_Create(iconSizeDyn, iconSizeDyn, ILC_COLOR32 | ILC_MASK, nbIcons, 0);
			HIMAGELIST hImageListDm = ImageList_Create(iconSizeDyn, iconSizeDyn, ILC_COLOR32 | ILC_MASK, nbIcons, 0);
			_iconListVector.push_back(hImageList);
			_iconListVector.push_back(hImageListDm);

			for (size_t i = 0; i < nbIcons; ++i)
			{
				int icoID = iconIDs[i];
				HICON hIcon = nullptr;
				DPIManagerV2::loadIcon(_hInst, MAKEINTRESOURCE(icoID), iconSizeDyn, iconSizeDyn, &hIcon, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
				ImageList_AddIcon(_iconListVector.at(0), hIcon);
				::DestroyIcon(hIcon);
				hIcon = nullptr;

				icoID = iconDarkModeIDs[i];
				DPIManagerV2::loadIcon(_hInst, MAKEINTRESOURCE(icoID), iconSizeDyn, iconSizeDyn, &hIcon, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
				ImageList_AddIcon(_iconListVector.at(1), hIcon);
				::DestroyIcon(hIcon); // Clean up the loaded icon
			}

			// Attach the image list to the toolbar
			::SendMessage(_hToolbarMenu, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(_iconListVector.at(NppDarkMode::isEnabled() ? 1 : 0)));

			// Place holder of search text field
			TBBUTTON tbButtons[1 + nbIcons]{};

			tbButtons[0].idCommand = 0;
			tbButtons[0].iBitmap = editWidthSep;
			tbButtons[0].fsState = TBSTATE_ENABLED;
			tbButtons[0].fsStyle = BTNS_SEP; //This is just a separator (blank space)
			tbButtons[0].iString = 0;

			tbButtons[1].idCommand = IDC_SORTBUTTON_FUNCLIST;
			tbButtons[1].iBitmap = 0;
			tbButtons[1].fsState = TBSTATE_ENABLED;
			tbButtons[1].fsStyle = BTNS_CHECK;
			tbButtons[1].iString = 0;

			tbButtons[2].idCommand = IDC_RELOADBUTTON_FUNCLIST;
			tbButtons[2].iBitmap = 1;
			tbButtons[2].fsState = TBSTATE_ENABLED;
			tbButtons[2].fsStyle = BTNS_BUTTON;
			tbButtons[2].iString = 0;

			tbButtons[3].idCommand = IDC_PREFERENCEBUTTON_FUNCLIST;
			tbButtons[3].iBitmap = 2;
			tbButtons[3].fsState = TBSTATE_ENABLED;
			tbButtons[3].fsStyle = BTNS_BUTTON;
			tbButtons[3].iString = 0;

			::SendMessage(_hToolbarMenu, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
			::SendMessage(_hToolbarMenu, TB_SETBUTTONSIZE, 0, MAKELONG(iconSizeDyn, iconSizeDyn));
			::SendMessage(_hToolbarMenu, TB_ADDBUTTONS, sizeof(tbButtons) / sizeof(TBBUTTON), reinterpret_cast<LPARAM>(&tbButtons));
			::SendMessage(_hToolbarMenu, TB_AUTOSIZE, 0, 0);

			ShowWindow(_hToolbarMenu, SW_SHOW);

			// tips text for toolbar buttons
			NativeLangSpeaker *pNativeSpeaker = nppParam.getNativeLangSpeaker();
			_sortTipStr = pNativeSpeaker->getAttrNameStr(_sortTipStr.c_str(), FL_FUCTIONLISTROOTNODE, FL_SORTLOCALNODENAME);
			_reloadTipStr = pNativeSpeaker->getAttrNameStr(_reloadTipStr.c_str(), FL_FUCTIONLISTROOTNODE, FL_RELOADLOCALNODENAME);
			_preferenceTipStr = pNativeSpeaker->getAttrNameStr(_preferenceTipStr.c_str(), FL_FUCTIONLISTROOTNODE, FL_PREFERENCESLOCALNODENAME);

			_hSearchEdit = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, NULL,
								WS_CHILD | WS_VISIBLE | ES_AUTOVSCROLL,
								2, 2, editWidth, editHeight,
								_hToolbarMenu, reinterpret_cast<HMENU>(IDC_SEARCHFIELD_FUNCLIST), _hInst, 0 );

			constexpr UINT_PTR idSubclassFunclstSearchEdit = 3;
			::SetWindowSubclass(_hSearchEdit, funclstSearchEditProc, idSubclassFunclstSearchEdit, 0);

			if (_hFontSearchEdit == nullptr)
			{
				LOGFONT lf{ _dpiManager.getDefaultGUIFontForDpi() };
				_hFontSearchEdit = ::CreateFontIndirect(&lf);
			}

			if (_hFontSearchEdit != nullptr)
			{
				::SendMessage(_hSearchEdit, WM_SETFONT, reinterpret_cast<WPARAM>(_hFontSearchEdit), MAKELPARAM(TRUE, 0));
			}

			std::vector<int> imgIds = _treeView.getImageIds(
				{ IDI_FUNCLIST_ROOT, IDI_FUNCLIST_NODE, IDI_FUNCLIST_LEAF }
				, { IDI_FUNCLIST_ROOT_DM, IDI_FUNCLIST_NODE_DM, IDI_FUNCLIST_LEAF_DM }
				, { IDI_FUNCLIST_ROOT2, IDI_FUNCLIST_NODE2, IDI_FUNCLIST_LEAF2 }
			);

			_treeView.init(_hInst, _hSelf, IDC_LIST_FUNCLIST);
			_treeView.setImageList(imgIds);
			_treeViewSearchResult.init(_hInst, _hSelf, IDC_LIST_FUNCLIST_AUX);
			_treeViewSearchResult.setImageList(imgIds);
			
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
				::SendMessage(_hToolbarMenu, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(_iconListVector.at(NppDarkMode::isEnabled() ? 1 : 0)));
			}
			else
			{
				NppDarkMode::setTreeViewStyle(_treeView.getHSelf());
			}

			std::vector<int> imgIds = _treeView.getImageIds(
				{ IDI_FUNCLIST_ROOT, IDI_FUNCLIST_NODE, IDI_FUNCLIST_LEAF }
				, { IDI_FUNCLIST_ROOT_DM, IDI_FUNCLIST_NODE_DM, IDI_FUNCLIST_LEAF_DM }
				, { IDI_FUNCLIST_ROOT2, IDI_FUNCLIST_NODE2, IDI_FUNCLIST_LEAF2 }
			);

			_treeView.setImageList(imgIds);
			_treeViewSearchResult.setImageList(imgIds);

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
			int extraValue = _dpiManager.scale(4);

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
