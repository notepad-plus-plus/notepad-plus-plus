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
const auto FUNC_IMG_ID = 1000;

class ScintillaEditView;

struct MatchedCharInserted {
	MatchedCharInserted() = delete;
	char _c;
	size_t _pos;
	MatchedCharInserted(char c, size_t pos) : _c(c), _pos(pos) {};
};

class InsertedMatchedChars {
public:
	void init(ScintillaEditView * pEditView) { _pEditView = pEditView; };
	void removeInvalidElements(MatchedCharInserted mci);
	void add(MatchedCharInserted mci);
	bool isEmpty() const { return _insertedMatchedChars.size() == 0; };
	intptr_t search(char startChar, char endChar, size_t posToDetect);

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

    const char* xpmfn[53] = {
        /* columns rows colors chars-per-pixel */
        "16 16 36 1 ",
        "u c None",
        "  c #131313",
        ". c #252525",
        "X c #161616",
        "o c #202020",
        "O c #393939",
        "+ c #242424",
        "@ c #282828",
        "# c #4E4E4E",
        "$ c #343434",
        "% c #5B5B5B",
        "& c #5F5F5F",
        "* c #626262",
        "= c #404040",
        "- c #686868",
        "; c #434343",
        ": c #464646",
        "> c #484848",
        ", c #494949",
        "< c #515151",
        "1 c #929292",
        "2 c #9B9B9B",
        "3 c #636363",
        "4 c #656565",
        "5 c #AFAFAF",
        "6 c #B7B7B7",
        "7 c #757575",
        "8 c #CDCDCD",
        "9 c #858585",
        "0 c #868686",
        "q c #DDDDDD",
        "w c #E1E1E1",
        "e c #E9E9E9",
        "r c #EEEEEE",
        "t c #959595",
        "y c #F6F6F6",
        /* pixels */
        "uuuuuuuuuuuuuuuu",
        "uuuuu5o.:yuuuuuu",
        "uuuu8 $:.0uuuuuu",
        "uuuu2 yuuuuuuuuu",
        "uuu6$ 46uuuuuuuu",
        "uuuO   Ouuuuuuuu",
        "uuuu;#uuuuuuuuuu",
        "uuuu##y& 3uu<+uu",
        "uuuu#;0.@X0, >uu",
        "uuuu+>uuroo >uuu",
        "uuuu >uuu* =uuuu",
        "uuuu 2uu, Xotuuu",
        "uuue 4u< >9 %owu",
        "u:,#X0uO>uu1 $yu",
        "u- +7uuuuuuuuuuu",
        "uuuuuuuuuuuuuuuu"
    };

 	FunctionCallTip _funcCalltip;

	const TCHAR * getApiFileName();
	void getWordArray(std::vector<generic_string> & wordArray, TCHAR *beginChars, TCHAR *excludeChars);
};
