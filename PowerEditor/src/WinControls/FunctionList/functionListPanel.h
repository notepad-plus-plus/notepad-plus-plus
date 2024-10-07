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

#pragma once

#include "DockingDlgInterface.h"
#include "functionListPanel_rc.h"
#include "functionParser.h"
#include "TreeView.h"

#define FL_PANELTITLE     L"Function List"
#define FL_FUCTIONLISTROOTNODE "FunctionList"

#define FL_SORTLOCALNODENAME        "SortTip"
#define FL_RELOADLOCALNODENAME      "ReloadTip"
#define FL_PREFERENCESLOCALNODENAME "PreferencesTip"

#define FL_PREFERENCE_INITIALSORT "PreferencesInitialSort"

class ScintillaEditView;

/*
1. global function + object + methods: Tree view of 2 levels - only the leaf contains the position info
root
|
|---leaf
|
|---node
|    |
|    |---leaf
|    |
|    |---leaf
|
|---node
     |
     |---leaf

2. each rule associates with file kind. For example, c_def (for *.c), cpp_def (for *.cpp) cpp_header (for *h) java_def (for *.java)...etc.



*/

struct SearchParameters {
	std::wstring _text2Find;
	bool _doSort = false;

	bool hasParams()const{
		return (_text2Find != L"" || _doSort);
	};
};

struct TreeParams {
	TreeStateNode _treeState;
	SearchParameters _searchParameters;
};

class FunctionListPanel : public DockingDlgInterface {
public:
	FunctionListPanel(): DockingDlgInterface(IDD_FUNCLIST_PANEL), _pTreeView(&_treeView) {};
	~FunctionListPanel();

	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView);

	void display(bool toShow = true) const override {
		DockingDlgInterface::display(toShow);
	};

	void setBackgroundColor(COLORREF bgColour) override {
		TreeView_SetBkColor(_treeView.getHSelf(), bgColour);
		TreeView_SetBkColor(_treeViewSearchResult.getHSelf(), bgColour);
	};

	void setForegroundColor(COLORREF fgColour) override {
		TreeView_SetTextColor(_treeView.getHSelf(), fgColour);
		TreeView_SetTextColor(_treeViewSearchResult.getHSelf(), fgColour);
	};

    void setParent(HWND parent2set){
        _hParent = parent2set;
    };
	
	// functionalities
	static int CALLBACK categorySortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/);
	void sortOrUnsort();
	void reload();
	void markEntry();
	bool serialize(const std::wstring & outputFilename = L"");
	void addEntry(const wchar_t *node, const wchar_t *displayText, size_t pos);
	void removeAllEntries();
	void searchFuncAndSwitchView();

protected:
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	HMENU _hPreferencesMenu = NULL;

private:
	HWND _hToolbarMenu = nullptr;
	HWND _hSearchEdit = nullptr;
	HFONT _hFontSearchEdit = nullptr;
	std::vector<HIMAGELIST> _iconListVector;

	TreeView *_pTreeView = nullptr;
	TreeView _treeView;
	TreeView _treeViewSearchResult;

	SCROLLINFO si{};
	long _findLine = -1;
	long _findEndLine = -1;
	HTREEITEM _findItem = nullptr;

	std::wstring _sortTipStr = L"Sort";
	std::wstring _reloadTipStr = L"Reload";
	std::wstring _preferenceTipStr = L"Preferences";

	std::vector<foundInfo> _foundFuncInfos;

	std::vector<std::wstring*> _posStrs;

	ScintillaEditView **_ppEditView = nullptr;
	FunctionParsersManager _funcParserMgr;
	std::vector< std::pair<int, int> > _skipZones;
	std::vector<TreeParams> _treeParams;

	std::wstring parseSubLevel(size_t begin, size_t end, std::vector< std::wstring > dataToSearch, intptr_t& foundPos);
	size_t getBodyClosePos(size_t begin, const wchar_t *bodyOpenSymbol, const wchar_t *bodyCloseSymbol);
	void notified(LPNMHDR notification);
	void addInStateArray(TreeStateNode tree2Update, const wchar_t *searchText, bool isSorted);
	TreeParams* getFromStateArray(const std::wstring& fullFilePath);
	bool openSelection(const TreeView &treeView);
	bool shouldSort();
	void setSort(bool isEnabled);
	void findMarkEntry(HTREEITEM htItem, LONG line);
	void initPreferencesMenu();
	void showPreferencesMenu();
};
