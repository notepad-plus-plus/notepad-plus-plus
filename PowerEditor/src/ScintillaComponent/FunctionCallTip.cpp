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


#include "FunctionCallTip.h"

#include <cstring>
#include <locale>
#include <memory>
#include <sstream>
#include <vector>

#include <Scintilla.h>

#include "NppXml.h"

struct Token {
	Token() = delete;
	explicit Token(char* tok, int len, bool isID) noexcept : token(tok), length(len), isIdentifier(isID) {}

	char* token;
	int length;
	bool isIdentifier;
};

struct FunctionValues {
	int lastIdentifier = -1;
	int lastFunctionIdentifier = -1;
	int param = 0;
	int scopeLevel = -1;
};

//test string case insensitive ala Scintilla
//0 if equal, <0 of before, >0 if after (name1 that is)
static int testNameNoCase(const char* name1, const char* name2, int len = -1)
{
	static const auto loc = std::locale("");
	if (len == -1)
	{
		len = 1024; //magic value, but it probably fails way before it reaches this
	}

	for (int i = 0; i < len; ++i)
	{
		const unsigned char char1 = name1[i];
		const unsigned char char2 = name2[i];

		if (char1 == '\0' || char2 == '\0') // null terminator check
			return char1 - char2;

		if (char1 == char2) // equal
			continue;

		// compare ignoring case
		if (std::tolower(char1, loc) != std::tolower(char2, loc))
		{
			return char1 - char2;
		}
	}

	return 0; // equal up to specified length.
}

void FunctionCallTip::setLanguageXML(NppXml::Element xmlKeyword)
{
	if (isVisible())
		close();
	_xmlKeyword = xmlKeyword;

	// Clear all buffered values, because they may point to freed memory area.
	reset();

	// Also clear _funcName so that next getCursorFunction will call loadFunction to parse XML structure
	_funcName.reset(nullptr);
}

bool FunctionCallTip::updateCalltip(int ch, bool needShown)
{
	if (!needShown && ch != _start && ch != _param && !isVisible()) //must be already visible
		return false;

	_curPos = _pEditView->execute(SCI_GETCURRENTPOS);

	//recalculate everything
	if (!getCursorFunction())
	{
		//cannot display calltip (anymore)
		close();
		return false;
	}
	showCalltip();
	return true;
}

void FunctionCallTip::showNextOverload()
{
	if (!isVisible())
		return;

	if (_currentNbOverloads > 0)
		_currentOverload = (_currentOverload + 1) % _currentNbOverloads;

	showCalltip();
}

void FunctionCallTip::showPrevOverload()
{
	if (!isVisible())
		return;
	_currentOverload = _currentOverload > 0 ? (_currentOverload - 1) : (_currentNbOverloads - 1);
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
	auto line = _pEditView->execute(SCI_LINEFROMPOSITION, _curPos);
	intptr_t startpos = _pEditView->execute(SCI_POSITIONFROMLINE, line);
	intptr_t endpos = _pEditView->execute(SCI_GETLINEENDPOSITION, line);
	intptr_t len = endpos - startpos + 3;	//also take CRLF in account, even if not there
	intptr_t offset = _curPos - startpos;	//offset is cursor location, only stuff before cursor has influence
	static constexpr intptr_t maxLen = 256;

	if ((offset < 2) || (len >= maxLen))
	{
		reset();
		return false;	//cannot be a func, need name and separator
	}
	
	char lineData[maxLen]{};

	_pEditView->getLine(line, lineData, len);

	//line aquired, find the functionname
	//first split line into tokens to parse
	//token is identifier or some expression, whitespace is ignored
	std::vector<Token> tokenVector;

	for (int i = 0; i < offset; ++i) //we don't care about stuff after the offset
	{
		//tokenVector.push_back(pair(lineData+i, len));
		char ch = lineData[i];
		if (isBasicWordChar(ch) || isAdditionalWordChar(ch)) //part of identifier
		{
			int tokenLen = 0;
			char* begin = lineData + i;
			while ((isBasicWordChar(ch) || isAdditionalWordChar(ch)) && i < offset)
			{
				++tokenLen;
				++i;
				ch = lineData[i];
			}
			tokenVector.push_back(Token(begin, tokenLen, true));
			--i; //correct overshooting of while loop
		}
		else
		{
			if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') //whitespace
			{
				//do nothing
			}
			else
			{
				tokenVector.push_back(Token(lineData + i, 1, false));
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
		const Token& curToken = tokenVector.at(i);
		if (curToken.isIdentifier)
		{
			curValue.lastIdentifier = static_cast<int>(i);
		}
		else
		{
			if (curToken.token[0] == _start)
			{
				++scopeLevel;
				newValue = curValue;
				valueVec.push_back(newValue); //store the current settings, so when this new function doesn't happen to be the 'real' one, we can restore everything
				
				curValue.scopeLevel = scopeLevel;
				if (i > 0 && curValue.lastIdentifier == static_cast<int>(i) - 1)
				{
					//identifier must be right before (, else we have some expression like "( x + y() )"
					curValue.lastFunctionIdentifier = curValue.lastIdentifier;
					curValue.param = 0;
				}
				else
				{
					//some expression
					curValue.lastFunctionIdentifier = -1;
				}
			}
			else if (curToken.token[0] == _param && curValue.lastFunctionIdentifier > -1)
			{
				++curValue.param;
			}
			else if (curToken.token[0] == _stop)
			{
				if (scopeLevel)	//scope cannot go below -1
					--scopeLevel;
				if (valueVec.size() > 0)
				{
					//only pop level if scope was of actual function
					curValue = valueVec.back();
					valueVec.pop_back();
				}
				else
				{
					//invalidate curValue
					curValue = FunctionValues();
				}
			}
			else if (curToken.token[0] == _terminal)
			{
				//invalidate everything
				valueVec.clear();
				curValue = FunctionValues();
			}
		}
	}
	
	bool res = false;

	if (curValue.lastFunctionIdentifier == -1)
	{
		//not in direct function. Start popping the stack until we empty it, or a func IS found
		while (curValue.lastFunctionIdentifier == -1 && valueVec.size() > 0)
		{
			curValue = valueVec.back();
			valueVec.pop_back();
		}
	}

	if (curValue.lastFunctionIdentifier > -1)
	{
		Token funcToken = tokenVector.at(curValue.lastFunctionIdentifier);
		funcToken.token[funcToken.length] = 0;
		_currentParam = curValue.param;

		bool same = false;
		if (_funcName.get())
		{
			if (_ignoreCase)
				same = testNameNoCase(_funcName.get(), funcToken.token, static_cast<int>(std::strlen(_funcName.get()))) == 0;
			else
				same = std::strncmp(_funcName.get(), funcToken.token, std::strlen(_funcName.get())) == 0;
		}

		if (!same)
		{
			//check if we need to reload data
			_funcName.reset(new char[funcToken.length + 1]);
			::strcpy_s(_funcName.get(), static_cast<size_t>(funcToken.length) + 1, funcToken.token);
			res = loadFunction();
		}
		else
		{
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
	reset(); //set everything back to 0
	//The functions should be ordered, but linear search because we can't access like array
	_curFunction = {};
	//Iterate through all keywords and find the correct function keyword
	for (NppXml::Element funcNode = _xmlKeyword;
		funcNode;
		funcNode = NppXml::nextSiblingElement(funcNode, "KeyWord"))
	{
		const char* name = NppXml::attribute(funcNode, "name");
		if (!name) //malformed node
			continue;
		int compVal = 0;
		if (_ignoreCase)
			compVal = testNameNoCase(name, _funcName.get()); //lstrcmpi doesn't work in this case
		else
			compVal = std::strcmp(name, _funcName.get());
		if (compVal == 0) //found it!
		{
			const char* val = NppXml::attribute(funcNode, "func");
			if (val)
			{
				if (std::strcmp(val, "yes") == 0)
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

	for (NppXml::Element overloadNode = NppXml::firstChildElement(_curFunction, "Overload");
		overloadNode;
		overloadNode = NppXml::nextSiblingElement(overloadNode, "Overload"))
	{
		const char* retVal = NppXml::attribute(overloadNode, "retVal");
		if (!retVal)
			continue; //malformed node
		_retVals.push_back(retVal);

		const char* description = NppXml::attribute(overloadNode, "descr");
		if (description)
			_descriptions.push_back(description);
		else
			_descriptions.push_back(""); //"no description available"

		for (NppXml::Element paramNode = NppXml::firstChildElement(overloadNode, "Param");
			paramNode;
			paramNode = NppXml::nextSiblingElement(paramNode, "Param"))
		{
			const char* param = NppXml::attribute(paramNode, "name");
			if (!param)
				continue; //malformed node
			paramVec.push_back(param);
		}
		_overloads.push_back(paramVec);
		paramVec.clear();
	}

	_currentNbOverloads = _overloads.size();

	if (_currentNbOverloads == 0)	//malformed node
		return false;

	return true;
}

void FunctionCallTip::showCalltip() 
{
	if (_currentNbOverloads == 0)
	{
		//ASSERT
		return;
	}

	//Check if the current overload still holds. If the current param exceeds amounti n overload, see if another one fits better (enough params)
	const stringVec& params = _overloads.at(_currentOverload);
	size_t psize = params.size() + 1;
	if (_currentParam >= psize)
	{
		size_t osize = _overloads.size();
		for (size_t i = 0; i < osize; ++i)
		{
			psize = _overloads.at(i).size()+1;
			if (_currentParam < psize)
			{
				_currentOverload = i;
				break;
			}
		}
	}

	std::stringstream callTipText;

	if (_currentNbOverloads > 1)
	{
		callTipText << "\001" << _currentOverload + 1 << " of " << _currentNbOverloads << "\002";
	}

	callTipText << _retVals.at(_currentOverload) << ' ' << _funcName << ' ' << _start;

	int highlightstart = 0;
	int highlightend = 0;
	size_t nbParams = params.size();
	for (size_t i = 0; i < nbParams; ++i)
	{
		if (i == _currentParam) 
		{
			highlightstart = static_cast<int>(callTipText.str().length());
			highlightend = highlightstart + static_cast<int>(std::strlen(params.at(i)));
		}
		callTipText << params.at(i);
		if (i < nbParams - 1)
			callTipText << _param << ' ';
	}

	callTipText << _stop;
	if (_descriptions.at(_currentOverload)[0])
	{
		callTipText << "\n" << _descriptions.at(_currentOverload);
	}

	if (isVisible())
		_pEditView->execute(SCI_CALLTIPCANCEL);
	else
		_startPos = _curPos;
	_pEditView->showCallTip(_startPos, callTipText.str());

	_selfActivated = true;
	if (highlightstart != highlightend)
	{
		_pEditView->execute(SCI_CALLTIPSETHLT, highlightstart, highlightend);
	}
}

void FunctionCallTip::reset()
{
	_currentOverload = 0;
	_currentParam = 0;
	//_curPos = 0;
	_startPos = 0;
	_overloads.clear();
	_currentNbOverloads = 0;
	_retVals.clear();
	_descriptions.clear();
}

void FunctionCallTip::cleanup()
{
	reset();
	_funcName.reset(nullptr);
	_pEditView = nullptr;
}
