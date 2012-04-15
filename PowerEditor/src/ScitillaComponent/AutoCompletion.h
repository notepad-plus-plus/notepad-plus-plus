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


#ifndef AUTOCOMPLETION_H
#define AUTOCOMPLETION_H

#ifndef FUNCTIONCALLTIP_H
#include "FunctionCallTip.h"
#endif// FUNCTIONCALLTIP_H

#ifndef TINYXML_INCLUDED
#include "tinyxml.h"
#endif// TINYXML_INCLUDED

class ScintillaEditView;

class AutoCompletion {
public:
	enum ActiveCompletion {CompletionNone = 0, CompletionAuto, CompletionWord, CompletionFunc};

	AutoCompletion(ScintillaEditView * pEditView) : _funcCompletionActive(false), _pEditView(pEditView), _funcCalltip(pEditView), 
																_curLang(L_TEXT), _pXmlFile(NULL), _activeCompletion(CompletionNone),
																_pXmlKeyword(NULL), _ignoreCase(true), _keyWords(TEXT("")) {
		//Do not load any language yet
	};

	~AutoCompletion(){
		if (_pXmlFile)
			delete _pXmlFile;
	};

	bool setLanguage(LangType language);

	//AutoComplete from the list
	bool showAutoComplete();
	//WordCompletion from the current file
	bool showWordComplete(bool autoInsert);	//autoInsert true if completion should fill in the word on a single match
	//Parameter display from the list
	bool showFunctionComplete();

	void update(int character);
	void callTipClick(int direction);

private:
	bool _funcCompletionActive;
	ScintillaEditView * _pEditView;
	LangType _curLang;
	TiXmlDocument *_pXmlFile;
	TiXmlElement *_pXmlKeyword;
	ActiveCompletion _activeCompletion;

	bool _ignoreCase;

	generic_string _keyWords;

	FunctionCallTip _funcCalltip;
	const TCHAR * getApiFileName();
};

#endif //AUTOCOMPLETION_H
