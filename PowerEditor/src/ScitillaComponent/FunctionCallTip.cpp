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


#include "FunctionCallTip.h"


struct Token {
	TCHAR * token;
	int length;
	bool isIdentifier;
	Token(TCHAR * tok, int len, bool isID) : token(tok), length(len), isIdentifier(isID) {};
};

struct FunctionValues {
	int lastIdentifier;
	int lastFunctionIdentifier;
	int param;
	int scopeLevel;
	FunctionValues() : lastIdentifier(-1), lastFunctionIdentifier(-1), param(0), scopeLevel(-1) {};
};

inline bool lower(TCHAR c) {
	return (c >= 'a' && c <= 'z');	
}

inline bool match(TCHAR c1, TCHAR c2) {
	if (c1 == c2)	return true;
	if (lower(c1))
		return ((c1-32) == c2);
	if (lower(c2))
		return ((c2-32) == c1);
	return false;	
}

//test string case insensitive ala Scintilla
//0 if equal, <0 of before, >0 if after (name1 that is)
int testNameNoCase(const TCHAR * name1, const TCHAR * name2, int len = -1) {
	if (len == -1) {
		len = 1024;	//magic value, but it probably fails way before it reaches this
	}
	int i = 0;
	while(match(name1[i], name2[i])) {
		if (name1[i] == 0 || i == len) {
			return 0;	//equal	
		}
		++i;	
	}
	
	int subs1 = lower(name1[i])?32:0;
	int subs2 = lower(name2[i])?32:0;
	
	return ( (name1[i]-subs1) - (name2[i]-subs2) );
}

void FunctionCallTip::setLanguageXML(TiXmlElement * pXmlKeyword) {
	if (isVisible())
		close();
	_pXmlKeyword = pXmlKeyword;

	// Clear all buffered values, because they may point to freed memory area.
	reset();

	// Also clear _funcName so that next getCursorFunction will call loadFunction to parse XML structure
	if (_funcName)
		delete [] _funcName;
	_funcName = 0;
}

bool FunctionCallTip::updateCalltip(int ch, bool needShown) {
	if (!needShown && ch != _start && !isVisible())		//must be already visible
		return false;

	_curPos = _pEditView->execute(SCI_GETCURRENTPOS);

	//recalculate everything
	if (!getCursorFunction()) {	//cannot display calltip (anymore)
		close();
		return false;
	}
	showCalltip();
	return true;
}

void FunctionCallTip::showNextOverload() {
	if (!isVisible())
		return;
	_currentOverload = (_currentOverload+1) % _currentNrOverloads;
	showCalltip();
}

void FunctionCallTip::showPrevOverload() {
	if (!isVisible())
		return;
	_currentOverload = _currentOverload > 0?(_currentOverload-1) : _currentNrOverloads-1;
	showCalltip();
}

void FunctionCallTip::close()
{
	if (!isVisible() || !_selfActivated)
		return;

	_pEditView->execute(SCI_CALLTIPCANCEL);
	_selfActivated = false;
	_currentOverload = 0;
}

bool FunctionCallTip::getCursorFunction()
{
	int line = _pEditView->execute(SCI_LINEFROMPOSITION, _curPos);
	int startpos = _pEditView->execute(SCI_POSITIONFROMLINE, line);
	int endpos = _pEditView->execute(SCI_GETLINEENDPOSITION, line);
	int len = endpos - startpos + 3;	//also take CRLF in account, even if not there
	int offset = _curPos - startpos;	//offset is cursor location, only stuff before cursor has influence
	const int maxLen = 256;

	if ((offset < 2) || (len >= maxLen))
	{
		reset();
		return false;	//cannot be a func, need name and separator
	}
	
	TCHAR lineData[maxLen] = TEXT("");

	_pEditView->getLine(line, lineData, len);

	//line aquired, find the functionname
	//first split line into tokens to parse
	//token is identifier or some expression, whitespace is ignored
	std::vector< Token > tokenVector;
	int tokenLen = 0;
	TCHAR ch;
	for (int i = 0; i < offset; ++i) 	//we dont care about stuff after the offset
    {
		//tokenVector.push_back(pair(lineData+i, len));
		ch = lineData[i];
		if (isBasicWordChar(ch) || isAdditionalWordChar(ch))	//part of identifier
        {
			tokenLen = 0;
			TCHAR * begin = lineData+i;
            while ((isBasicWordChar(ch) || isAdditionalWordChar(ch)) && i < offset) {
				++tokenLen;
				++i;
				ch = lineData[i];
			}
			tokenVector.push_back(Token(begin, tokenLen, true));
			i--;	//correct overshooting of while loop
		}
        else
        {
			if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') 	//whitespace
            {
				//do nothing
			}
            else
            {
				tokenLen = 1;
				tokenVector.push_back(Token(lineData+i, tokenLen, false));
			}
		}
	}

	size_t vsize = tokenVector.size();
	//mind nested funcs, like |blblb a (x, b(), c);|
	//therefore, use stack
	std::vector<FunctionValues> valueVec;

	FunctionValues curValue, newValue;
	int scopeLevel = 0;
	for (size_t i = 0; i < vsize; ++i)
	{
		Token & curToken = tokenVector.at(i);
		if (curToken.isIdentifier) {
			curValue.lastIdentifier = i;
		} else {
			if (curToken.token[0] == _start)
			{
				++scopeLevel;
				newValue = curValue;
				valueVec.push_back(newValue);	//store the current settings, so when this new function doesnt happen to be the 'real' one, we can restore everything
				
				curValue.scopeLevel = scopeLevel;
				if (i > 0 && curValue.lastIdentifier == int(i)-1) {	//identifier must be right before (, else we have some expression like "( x + y() )"
					curValue.lastFunctionIdentifier = curValue.lastIdentifier;
					curValue.param = 0;
				} else {	//some expression
					curValue.lastFunctionIdentifier = -1;
				}
			} else if (curToken.token[0] == _param && curValue.lastFunctionIdentifier > -1) {
				++curValue.param;
			} else if (curToken.token[0] == _stop) {
				if (scopeLevel)	//scope cannot go below -1
					scopeLevel--;
				if (valueVec.size() > 0) {	//only pop level if scope was of actual function
					curValue = valueVec.back();
					valueVec.pop_back();
				} else {
					//invalidate curValue
					curValue = FunctionValues();
				}
			} else if (curToken.token[0] == _terminal) {
				//invalidate everything
				valueVec.clear();
				curValue = FunctionValues();
			}
		}
	}
	
	bool res = false;

	if (curValue.lastFunctionIdentifier == -1) {	//not in direct function. Start popping the stack untill we empty it, or a func IS found
		while(curValue.lastFunctionIdentifier == -1 && valueVec.size() > 0) {
			curValue = valueVec.back();
			valueVec.pop_back();
		}
	}
	if (curValue.lastFunctionIdentifier > -1) {
		Token funcToken = tokenVector.at(curValue.lastFunctionIdentifier);
		funcToken.token[funcToken.length] = 0;
		_currentParam = curValue.param;

		bool same = false;
		if (_funcName) {
			if(_ignoreCase)
				same = testNameNoCase(_funcName, funcToken.token, lstrlen(_funcName)) == 0;
			else
				same = generic_strncmp(_funcName, funcToken.token, lstrlen(_funcName)) == 0;
		}
		if (!same) {	//check if we need to reload data
			if (_funcName) {
				delete [] _funcName;
			}
			_funcName = new TCHAR[funcToken.length+1];
			lstrcpy(_funcName, funcToken.token);
			res = loadFunction();
		} else {
			res = true;
		}
	}
	return res;
}

/*
Find function in XML structure and parse it
*/
bool FunctionCallTip::loadFunction()
{
	reset();	//set everything back to 0
	//The functions should be ordered, but linear search because we cant access like array
	_curFunction = NULL;
	//Iterate through all keywords and find the correct function keyword
	TiXmlElement *funcNode = _pXmlKeyword;
	
	for (; funcNode; funcNode = funcNode->NextSiblingElement(TEXT("KeyWord")) )
	{
		const TCHAR * name = NULL;
		name = funcNode->Attribute(TEXT("name"));
		if (!name)		//malformed node
			continue;
		int compVal = 0;
		if (_ignoreCase)
			compVal = testNameNoCase(name, _funcName);	//lstrcmpi doesnt work in this case
		else
			compVal = lstrcmp(name, _funcName);
		if (!compVal) 	//found it!
        {
			const TCHAR * val = funcNode->Attribute(TEXT("func"));
			if (val)
			{
				if (!lstrcmp(val, TEXT("yes"))) 
                {
					//what we've been looking for
					_curFunction = funcNode;
					break;
				}
				else 
                {
					//name matches, but not a function, abort the entire procedure
					return false;
				}
			}
		}
	}

	//Nothing found
	if (!_curFunction)
		return false;

	stringVec paramVec;

	TiXmlElement *overloadNode = _curFunction->FirstChildElement(TEXT("Overload"));
	TiXmlElement *paramNode = NULL;
	for (; overloadNode ; overloadNode = overloadNode->NextSiblingElement(TEXT("Overload")) ) {
		const TCHAR * retVal = overloadNode->Attribute(TEXT("retVal"));
		if (!retVal)
			continue;	//malformed node
		_retVals.push_back(retVal);

		const TCHAR * description = overloadNode->Attribute(TEXT("descr"));
		if (description)
			_descriptions.push_back(description);
		else
			_descriptions.push_back(TEXT(""));	//"no description available"

		paramNode = overloadNode->FirstChildElement(TEXT("Param"));
		for (; paramNode ; paramNode = paramNode->NextSiblingElement(TEXT("Param")) ) {
			const TCHAR * param = paramNode->Attribute(TEXT("name"));
			if (!param)
				continue;	//malformed node
			paramVec.push_back(param);
		}
		_overloads.push_back(paramVec);
		paramVec.clear();

		++_currentNrOverloads;
	}

	_currentNrOverloads = (int)_overloads.size();

	if (_currentNrOverloads == 0)	//malformed node
		return false;

	return true;
}

void FunctionCallTip::showCalltip() 
{
	if (_currentNrOverloads == 0)
    {
		//ASSERT
		return;
	}

	//Check if the current overload still holds. If the current param exceeds amounti n overload, see if another one fits better (enough params)
	stringVec & params = _overloads.at(_currentOverload);
	size_t psize = params.size()+1;
	if ((size_t)_currentParam >= psize) {
		size_t osize = _overloads.size();
		for(size_t i = 0; i < osize; ++i) {
			psize = _overloads.at(i).size()+1;
			if ((size_t)_currentParam < psize) {
				_currentOverload = i;
				break;
			}
		}
	}
	const TCHAR * curRetValText = _retVals.at(_currentOverload);
	const TCHAR * curDescriptionText = _descriptions.at(_currentOverload);
	bool hasDescr = true;
	if (!curDescriptionText[0])
		hasDescr = false;

	int bytesNeeded = lstrlen(curRetValText) + lstrlen(_funcName) + 5;//'retval funcName (params)\0'
	if (hasDescr)
		bytesNeeded += lstrlen(curDescriptionText);

	size_t nrParams = params.size();
	for(size_t i = 0; i < nrParams; ++i) {
		bytesNeeded += lstrlen(params.at(i)) + 2;	//'param, '
	}

	if (_currentNrOverloads > 1) {
		bytesNeeded += 24;	//  /\00001 of 00003\/
	}

	const int maxLen = 2048;
	if (bytesNeeded >= maxLen)
		return;

	TCHAR textBuffer[maxLen];
	textBuffer[0] = 0;

	if (_currentNrOverloads > 1) {
		wsprintf(textBuffer, TEXT("\001%u of %u\002"), _currentOverload+1, _currentNrOverloads);
	}

	lstrcat(textBuffer, curRetValText);
	lstrcat(textBuffer, TEXT(" "));
	lstrcat(textBuffer, _funcName);
	lstrcat(textBuffer, TEXT(" ("));

	int highlightstart = 0;
	int highlightend = 0;
	for(size_t i = 0; i < nrParams; ++i) 
	{
		if (int(i) == _currentParam) 
		{
			highlightstart = lstrlen(textBuffer);
			highlightend = highlightstart + lstrlen(params.at(i));
		}
		lstrcat(textBuffer, params.at(i));
		if (i < nrParams-1)
			lstrcat(textBuffer, TEXT(", "));
	}

	lstrcat(textBuffer, TEXT(")"));
	if (hasDescr) {
		lstrcat(textBuffer, TEXT("\n"));
		lstrcat(textBuffer, curDescriptionText);
	}

	if (isVisible())
		_pEditView->execute(SCI_CALLTIPCANCEL);
	else
		_startPos = _curPos;
	_pEditView->showCallTip(_startPos, textBuffer);

	_selfActivated = true;
	if (highlightstart != highlightend) {
		_pEditView->execute(SCI_CALLTIPSETHLT, highlightstart, highlightend);
	}
}

void FunctionCallTip::reset() {
	_currentOverload = 0;
	_currentParam = 0;
	//_curPos = 0;
	_startPos = 0;
	_overloads.clear();
	_currentNrOverloads = 0;
	_retVals.clear();
	_descriptions.clear();
}

void FunctionCallTip::cleanup() {
	reset();
	if (_funcName)
		delete [] _funcName;
	_funcName = 0;
	_pEditView = NULL;
}
