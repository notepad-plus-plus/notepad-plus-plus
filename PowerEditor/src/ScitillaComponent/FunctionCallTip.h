//this file is part of notepad++
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


class FunctionCallTip {
public:
	FunctionCallTip() : _pEditView(NULL), _langName(NULL), _currentOverloads(NULL), _funcName(NULL), _params(NULL), _initialized(false),
						_start('('), _stop(')'), _param(','), _terminal(';') {};
	~FunctionCallTip() {/* cleanup(); */};
	void initCalltip(ScintillaEditView * editView, const char * language);	//show the calltip
	void updateCalltip(int ch, bool needShown = false);		//Ch is character typed, or 0 if another event occured. NeedShown is true if calltip should be attempted to displayed
	void showNextOverload();
	void showPrevOverload();
	bool isVisible() { return _pEditView?_pEditView->execute(SCI_CALLTIPACTIVE) == TRUE:false; };
	void close();					//Close calltip if visible

private:
	char * _langName;				//current language
	ScintillaEditView * _pEditView;	//Scintilla to display calltip in
	bool _initialized;

	int _curPos;					//cursor position
	int _startPos;					//display start position

	char * _funcName;				//current name of function
	char ** _currentOverloads;		//pointer to list of strings each containing possible overload
	vector<int> * _params;			//array of vectors with offsets found for params
	int _currentNrOverloads;		//current amount of overloads
	int _currentOverload;			//current chosen overload
	int _currentParam;				//current highlighted param

	char _start;
	char _stop;
	char _param;
	char _terminal;

	bool getCursorFunction();		//retrieve data about function at cursor. Returns true if a function was found. Calls loaddata if needed
	bool loadData();				//returns true if the function can be found
	void showCalltip();				//display calltip based on current variables
	void reset();					//reset all vars in case function is invalidated
	void cleanup();					//delete any leftovers
};
#endif// FUNCTIONCALLTIP_H
