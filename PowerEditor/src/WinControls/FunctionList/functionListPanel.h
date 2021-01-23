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

#define FL_PANELTITLE     TEXT("Function List")
#define FL_FUCTIONLISTROOTNODE "FunctionList"

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
	generic_string _text2Find;
	bool _doSort;

	SearchParameters(): _text2Find(TEXT("")), _doSort(false){
	};

	bool hasParams()const{
		return (_text2Find != TEXT("") || _doSort);
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

    virtual void display(bool toShow = true) const {
        DockingDlgInterface::display(toShow);
    };

	virtual void setBackgroundColor(COLORREF bgColour) {
		TreeView_SetBkColor(_treeView.getHSelf(), bgColour);
		TreeView_SetBkColor(_treeViewSearchResult.getHSelf(), bgColour);
    };
	virtual void setForegroundColor(COLORREF fgColour) {
		TreeView_SetTextColor(_treeView.getHSelf(), fgColour);
		TreeView_SetTextColor(_treeViewSearchResult.getHSelf(), fgColour);
    };

    void setParent(HWND parent2set){
        _hParent = parent2set;
    };
	
	// functionalities
	void sortOrUnsort();
	void reload();
	void markEntry();
	bool serialize(const generic_string & outputFilename = TEXT(""));
	void addEntry(const TCHAR *node, const TCHAR *displayText, size_t pos);
	void removeAllEntries();
	void searchFuncAndSwitchView();

protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	HWND _hToolbarMenu = nullptr;
	HWND _hSearchEdit = nullptr;

	TreeView *_pTreeView = nullptr;
	TreeView _treeView;
	TreeView _treeViewSearchResult;

	long _findLine = -1;
	long _findEndLine = -1;
	HTREEITEM _findItem;

	generic_string _sortTipStr = TEXT("Reload");
	generic_string _reloadTipStr = TEXT("Sort");

	std::vector<foundInfo> _foundFuncInfos;

	std::vector<generic_string*> posStrs;

	ScintillaEditView **_ppEditView = nullptr;
	FunctionParsersManager _funcParserMgr;
	std::vector< std::pair<int, int> > _skipZones;
	std::vector<TreeParams> _treeParams;
	HIMAGELIST _hTreeViewImaLst;

	generic_string parseSubLevel(size_t begin, size_t end, std::vector< generic_string > dataToSearch, int & foundPos);
	size_t getBodyClosePos(size_t begin, const TCHAR *bodyOpenSymbol, const TCHAR *bodyCloseSymbol);
	void notified(LPNMHDR notification);
	void addInStateArray(TreeStateNode tree2Update, const TCHAR *searchText, bool isSorted);
	TreeParams* getFromStateArray(generic_string fullFilePath);
	BOOL setTreeViewImageList(int root_id, int node_id, int leaf_id);
	bool openSelection(const TreeView &treeView);
	bool shouldSort();
	void setSort(bool isEnabled);
	void findMarkEntry(HTREEITEM htItem, LONG line);
};

