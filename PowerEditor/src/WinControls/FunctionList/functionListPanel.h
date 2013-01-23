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


class FunctionListPanel : public DockingDlgInterface {
public:
	FunctionListPanel(): DockingDlgInterface(IDD_FUNCLIST_PANEL), _ppEditView(NULL) {};

	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView);

    virtual void display(bool toShow = true) const {
        DockingDlgInterface::display(toShow);
    };

    void setParent(HWND parent2set){
        _hParent = parent2set;
    };
	
	// functionalities
	void reload();
	void addEntry(const TCHAR *node, const TCHAR *displayText, size_t pos);
	void removeAllEntries();
	void removeEntry();
	void modifyEntry();
	void update();
	
	void parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, const TCHAR *regExpr2search, std::vector< generic_string > dataToSearch, std::vector< generic_string > data2ToSearch, generic_string classStructName = TEXT(""));
	/*
	void parse(size_t begin, size_t end, const TCHAR *wordToExclude, const TCHAR *regExpr2search, ...);
	bool parseSubLevel(size_t begin, size_t end, const TCHAR *wordToExclude, const TCHAR *regExpr2search, ...);
	*/
	void parse2(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, const TCHAR *block, std::vector< generic_string > blockNameToSearch,  const TCHAR *bodyOpenSymbol, const TCHAR *bodyCloseSymbol, const TCHAR *function, std::vector< generic_string > functionToSearch);

protected:
	virtual BOOL CALLBACK FunctionListPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	TreeView _treeView;
	ScintillaEditView **_ppEditView;
	FunctionParsersManager _funcParserMgr;
	std::vector<FuncInfo> _funcInfos;
	std::vector< std::pair<int, int> > _skipZones;
	generic_string parseSubLevel(size_t begin, size_t end, std::vector< generic_string > dataToSearch, int & foundPos);
	size_t getBodyClosePos(size_t begin, const TCHAR *bodyOpenSymbol, const TCHAR *bodyCloseSymbol);
	void notified(LPNMHDR notification);
};
#endif // FUNCLISTPANEL_H
