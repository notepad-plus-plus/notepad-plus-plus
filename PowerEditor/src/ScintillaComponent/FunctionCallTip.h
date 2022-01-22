// This file is part of Notepad++ project
// Copyright (C)2008 Harry Bruin <harrybharry@users.sourceforge.net>

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

#include "ScintillaEditView.h"

typedef std::vector<const TCHAR *> stringVec;

class FunctionCallTip {
	 friend class AutoCompletion;
public:
	explicit FunctionCallTip(ScintillaEditView * pEditView) : _pEditView(pEditView) {};
	~FunctionCallTip() {/* cleanup(); */};
	void setLanguageXML(TiXmlElement * pXmlKeyword);	//set calltip keyword node
	bool updateCalltip(int ch, bool needShown = false);	//Ch is character typed, or 0 if another event occured. NeedShown is true if calltip should be attempted to displayed. Return true if calltip was made visible
	void showNextOverload();							//show next overlaoded parameters
	void showPrevOverload();							//show prev overlaoded parameters
	bool isVisible() { return _pEditView?_pEditView->execute(SCI_CALLTIPACTIVE) == TRUE:false; };	//true if calltip visible
	void close();					//Close calltip if visible

private:
	ScintillaEditView * _pEditView = nullptr;	//Scintilla to display calltip in
	TiXmlElement * _pXmlKeyword = nullptr;	//current keyword node (first one)

	intptr_t _curPos = 0;					//cursor position
	intptr_t _startPos = 0;					//display start position

	TiXmlElement * _curFunction = nullptr;	//current function element
	//cache some XML values n stuff
	TCHAR * _funcName = nullptr;				//name of function
	stringVec _retVals;				//vector of overload return values/types
	std::vector<stringVec> _overloads;	//vector of overload params (=vector)
	stringVec _descriptions;		//vecotr of function descriptions
	size_t _currentNbOverloads = 0;		//current amount of overloads
	size_t _currentOverload = 0;			//current chosen overload
	size_t _currentParam = 0;				//current highlighted param

	TCHAR _start = '(';
	TCHAR _stop = ')';
	TCHAR _param = ',';
	TCHAR _terminal = ';';
    generic_string _additionalWordChar = TEXT("");
	bool _ignoreCase = true;
	bool _selfActivated = false;

	bool getCursorFunction();		//retrieve data about function at cursor. Returns true if a function was found. Calls loaddata if needed
	bool loadFunction();			//returns true if the function can be found
	void showCalltip();				//display calltip based on current variables
	void reset();					//reset all vars in case function is invalidated
	void cleanup();					//delete any leftovers
    bool isBasicWordChar(TCHAR ch) const {
        return ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_');
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
