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


#ifndef FUNCLISTPANEL_H
#define FUNCLISTPANEL_H

//#include <windows.h>
#ifndef DOCKINGDLGINTERFACE_H
#include "DockingDlgInterface.h"
#endif //DOCKINGDLGINTERFACE_H

#include "functionListPanel_rc.h"
#include "functionParser.h"
#include "TreeView.h"

#define FL_PANELTITLE     TEXT("Function List")
#define FL_SORTTIP        TEXT("sort")
#define FL_RELOADTIP      TEXT("Reload")

#define FL_FUCTIONLISTROOTNODE "FunctionList"
#define FL_SORTLOCALNODENAME   "SortTip"
#define FL_RELOADLOCALNODENAME "ReloadTip"

class ScintillaEditView;

struct FuncInfo {
	generic_string _displayText;
	size_t _offsetPos;
};
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
	FunctionListPanel(): DockingDlgInterface(IDD_FUNCLIST_PANEL), _ppEditView(NULL), _pTreeView(&_treeView),
	_reloadTipStr(TEXT("Reload")), _sortTipStr(TEXT("Sort")) {};

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
	void addEntry(const TCHAR *node, const TCHAR *displayText, size_t pos);
	void removeAllEntries();
	void removeEntry();
	void modifyEntry();
	void searchFuncAndSwitchView();

protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	HWND _hToolbarMenu;
	HWND _hSearchEdit;

	TreeView *_pTreeView;
	TreeView _treeView;
	TreeView _treeViewSearchResult;

	generic_string _sortTipStr;
	generic_string _reloadTipStr;

	ScintillaEditView **_ppEditView;
	FunctionParsersManager _funcParserMgr;
	std::vector<FuncInfo> _funcInfos;
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
};
#endif // FUNCLISTPANEL_H
