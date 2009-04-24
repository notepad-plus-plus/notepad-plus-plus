//this file is part of Notepad++
//Copyright (C)2008 Harry Bruin <harrybharry@users.sourceforge.net>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef FUNCTIONCALLTIP_H
#define FUNCTIONCALLTIP_H

#include "ScintillaEditView.h"

typedef std::vector<const TCHAR *> stringVec;

class FunctionCallTip {
	 friend class AutoCompletion;
public:
	FunctionCallTip(ScintillaEditView * pEditView) : _pEditView(pEditView), _pXmlKeyword(NULL), _curPos(0), _startPos(0),
													_curFunction(NULL), _currentNrOverloads(0), _currentOverload(0),
													_currentParam(0), _funcName(NULL),
													_start('('), _stop(')'), _param(','), _terminal(';'), _ignoreCase(true)
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
	vector<stringVec> _overloads;	//vector of overload params (=vector)
	stringVec _descriptions;		//vecotr of function descriptions
	int _currentNrOverloads;		//current amount of overloads
	int _currentOverload;			//current chosen overload
	int _currentParam;				//current highlighted param

	TCHAR _start;
	TCHAR _stop;
	TCHAR _param;
	TCHAR _terminal;
	bool _ignoreCase;

	bool getCursorFunction();		//retrieve data about function at cursor. Returns true if a function was found. Calls loaddata if needed
	bool loadFunction();			//returns true if the function can be found
	void showCalltip();				//display calltip based on current variables
	void reset();					//reset all vars in case function is invalidated
	void cleanup();					//delete any leftovers
};

#endif// FUNCTIONCALLTIP_H
