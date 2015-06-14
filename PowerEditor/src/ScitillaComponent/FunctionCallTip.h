// this file is part of Notepad++
// Copyright (C)2008 Harry Bruin <harrybharry@users.sourceforge.net>
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

#ifndef FUNCTIONCALLTIP_H
#define FUNCTIONCALLTIP_H

#ifndef SCINTILLA_EDIT_VIEW_H
#include "ScintillaEditView.h"
#endif //SCINTILLA_EDIT_VIEW_H

typedef std::vector<const TCHAR *> stringVec;

class FunctionCallTip {
	 friend class AutoCompletion;
public:
	FunctionCallTip(ScintillaEditView * pEditView) : _pEditView(pEditView), _pXmlKeyword(NULL), _curPos(0), _startPos(0),
													_curFunction(NULL), _currentNrOverloads(0), _currentOverload(0),
													_currentParam(0), _funcName(NULL),
													_start('('), _stop(')'), _param(','), _terminal(';'), _ignoreCase(true),
													_additionalWordChar(TEXT("")), _selfActivated(false)
													{};
	~FunctionCallTip() {/* cleanup(); */};
	void setLanguageXML(TiXmlElement * pXmlKeyword);	//set calltip keyword node
	bool updateCalltip(int ch, bool needShown = false);	//Ch is character typed, or 0 if another event occured. NeedShown is true if calltip should be attempted to displayed. Return true if calltip was made visible
	void showNextOverload();							//show next overlaoded parameters
	void showPrevOverload();							//show prev overlaoded parameters
	bool isVisible() { return _pEditView?_pEditView->execute(SCI_CALLTIPACTIVE) == TRUE:false; };	//true if calltip visible
	void close();					//Close calltip if visible

private:
	ScintillaEditView * _pEditView;	//Scintilla to display calltip in
	TiXmlElement * _pXmlKeyword;	//current keyword node (first one)

	int _curPos;					//cursor position
	int _startPos;					//display start position

	TiXmlElement * _curFunction;	//current function element
	//cache some XML values n stuff
	TCHAR * _funcName;				//name of function
	stringVec _retVals;				//vector of overload return values/types
	std::vector<stringVec> _overloads;	//vector of overload params (=vector)
	stringVec _descriptions;		//vecotr of function descriptions
	int _currentNrOverloads;		//current amount of overloads
	int _currentOverload;			//current chosen overload
	int _currentParam;				//current highlighted param

	TCHAR _start;
	TCHAR _stop;
	TCHAR _param;
	TCHAR _terminal;
    generic_string _additionalWordChar;
	bool _ignoreCase;
	bool _selfActivated;

	bool getCursorFunction();		//retrieve data about function at cursor. Returns true if a function was found. Calls loaddata if needed
	bool loadFunction();			//returns true if the function can be found
	void showCalltip();				//display calltip based on current variables
	void reset();					//reset all vars in case function is invalidated
	void cleanup();					//delete any leftovers
    bool isBasicWordChar(TCHAR ch) const {
        return (ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z' || ch >= '0' && ch <= '9' || ch == '_');
    };
    bool isAdditionalWordChar(TCHAR ch) const {
        const TCHAR *addChars = _additionalWordChar.c_str();
        size_t len = _additionalWordChar.length();
        for (size_t i = 0 ; i < len ; ++i)
            if (ch == addChars[i])
                return true;
        return false;
    };
};

#endif// FUNCTIONCALLTIP_H
