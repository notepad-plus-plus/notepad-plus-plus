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

#include "FunctionCallTip.h"
#include "tinyxml.h"

const size_t tagMaxLen = 256;

class ScintillaEditView;

struct MatchedCharInserted {
	MatchedCharInserted() = delete;
	char _c;
	int _pos;
	MatchedCharInserted(char c, int pos) : _c(c), _pos(pos) {};
};

class InsertedMatchedChars {
public:
	void init(ScintillaEditView * pEditView) { _pEditView = pEditView; };
	void removeInvalidElements(MatchedCharInserted mci);
	void add(MatchedCharInserted mci);
	bool isEmpty() const { return _insertedMatchedChars.size() == 0; };
	int search(char startChar, char endChar, int posToDetect);

private:
	std::vector<MatchedCharInserted> _insertedMatchedChars;
	ScintillaEditView * _pEditView = nullptr;
};

class AutoCompletion {
public:
	explicit AutoCompletion(ScintillaEditView * pEditView): _pEditView(pEditView), _funcCalltip(pEditView) {
		//Do not load any language yet
		_insertedMatchedChars.init(_pEditView);
	};

	~AutoCompletion(){
		delete _pXmlFile;
	};

	bool setLanguage(LangType language);

	//AutoComplete from the list
	bool showApiComplete();
	//WordCompletion from the current file
	bool showWordComplete(bool autoInsert);	//autoInsert true if completion should fill in the word on a single match
	// AutoComplete from both the list and the current file
	bool showApiAndWordComplete();
	//Parameter display from the list
	bool showFunctionComplete();
	// Autocomplete from path.
	void showPathCompletion();

	void insertMatchedChars(int character, const MatchedPairConf & matchedPairConf);
	void update(int character);
	void callTipClick(size_t direction);
	void getCloseTag(char *closeTag, size_t closeTagLen, size_t caretPos, bool isHTML);

private:
	bool _funcCompletionActive = false;
	ScintillaEditView * _pEditView = nullptr;
	LangType _curLang = L_TEXT;
	TiXmlDocument *_pXmlFile = nullptr;
	TiXmlElement *_pXmlKeyword = nullptr;

	InsertedMatchedChars _insertedMatchedChars;

	bool _ignoreCase = true;

	std::vector<generic_string> _keyWordArray;
	generic_string _keyWords;
	size_t _keyWordMaxLen = 0;

	FunctionCallTip _funcCalltip;

	const TCHAR * getApiFileName();
	void getWordArray(std::vector<generic_string> & wordArray, TCHAR *beginChars, TCHAR *excludeChars);
};
