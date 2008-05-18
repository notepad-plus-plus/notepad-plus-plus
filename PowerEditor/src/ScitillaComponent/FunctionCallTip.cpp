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

#include "FunctionCallTip.h"


struct Token {
	char * token;
	int length;
	bool isIdentifier;
	Token(char * tok, int len, bool isID) : token(tok), length(len), isIdentifier(isID) {};
};

struct FunctionValues {
	int lastIdentifier;
	int lastFunctionIdentifier;
	int param;
	int scopeLevel;
	FunctionValues() : lastIdentifier(-1), lastFunctionIdentifier(-1), param(0), scopeLevel(-1) {};
};

bool stringEqualN(const char * first, const char * second, int n) {
	int i = 0;
	while(i < n) {	//no checks for 0 as n has to take that into account (if one string is shorter, no overflow as 0 doesnt equal anything
		if (first[i] != second[i])
			return false;
		i++;
	}
	return true;
};


void FunctionCallTip::initCalltip(ScintillaEditView * editView, const char * language) {
	_pEditView = editView;
	if (_langName)
		delete [] _langName;
	_langName = new char[strlen(language)+1];
	strcpy(_langName, language);
	_initialized = true;
}

void FunctionCallTip::updateCalltip(int ch, bool needShown) {
	if (!_initialized)
		return;

	if (!needShown && ch != _start && !isVisible())		//must be already visible
		return;

	_curPos = _pEditView->execute(SCI_GETCURRENTPOS);

	//recalculate everything
	if (!getCursorFunction()) {	//cannot display calltip (anymore)
		close();
		return;
	}
	showCalltip();
	return;

	/*
	switch (ch) {
		case '(':
		case 0: {	//recalculate everything
			if (!getCursorFunction()) {	//cannot display calltip (anymore)
				close();
				return;
			}
			showCalltip();
			return; }
		case ')': {	//close calltip
			close();
			return; }
		case ',': {	//show next param
			_currentParam++;
			showCalltip();
			return; }
		default: {	//any other character: nothing to do
			break; }
	}
	*/
}

void FunctionCallTip::showNextOverload() {
	if (!_initialized)
		return;
	if (!isVisible())
		return;
	_currentOverload = (_currentOverload+1) % _currentNrOverloads;
	showCalltip();
}

void FunctionCallTip::showPrevOverload() {
	if (!_initialized)
		return;
	if (!isVisible())
		return;
	_currentOverload = _currentOverload > 0?(_currentOverload-1) : _currentNrOverloads-1;
	showCalltip();
}

void FunctionCallTip::close() {
	if (!_initialized)
		return;
	if (!isVisible()) {	
		return;
	}

	_pEditView->execute(SCI_CALLTIPCANCEL);
	_currentOverload = 0;
}

bool FunctionCallTip::getCursorFunction() {
	int line = _pEditView->execute(SCI_LINEFROMPOSITION, _curPos);
	int startpos = _pEditView->execute(SCI_POSITIONFROMLINE, line);
	int endpos = _pEditView->execute(SCI_GETLINEENDPOSITION, line);
	int len = endpos - startpos + 3;	//also take CRLF in account, even if not there
	int offset = _curPos - startpos;	//offset is cursor location, only stuff before cursor has influence
	if (offset < 2) {
		reset();
		if (_funcName) {
			delete [] _funcName;
			_funcName = 0;
		}
		return false;	//cannot be a func, need name and separator
	}
	char * lineData = new char[len];
	_pEditView->execute(SCI_GETLINE, line, (LPARAM)lineData);

	//line aquired, find the functionname
	//first split line into tokens to parse
	//token is identifier or some expression, whitespace is ignored
	std::vector< Token > tokenVector;
	int tokenLen = 0;
	char ch;
	for (int i = 0; i < offset; i++) {	//we dont care about stuff after the offset
		//tokenVector.push_back(pair(lineData+i, len));
		ch = lineData[i];
		if (ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z' || ch >= '0' && ch <= '9' || ch == '_') {	//part of identifier
			tokenLen = 0;
			char * begin = lineData+i;
			while ( (ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z' || ch >= '0' && ch <= '9' || ch == '_') && i < offset) {
				tokenLen++;
				i++;
				ch = lineData[i];
			}
			tokenVector.push_back(Token(begin, tokenLen, true));
			i--;	//correct overshooting of while loop
		} else {
			if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {	//whitespace
				//do nothing
			} else {
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
	for (size_t i = 0; i < vsize; i++) {
		Token & curToken = tokenVector.at(i);
		if (curToken.isIdentifier) {
			curValue.lastIdentifier = i;
		} else {
			if (curToken.token[0] == _start) {
				scopeLevel++;
				newValue = curValue;
				valueVec.push_back(newValue);	//store the current settings, so when this new function doesnt happen to be the 'real' one, we can restore everything
				
				curValue.scopeLevel = scopeLevel;
				if (i > 0 && curValue.lastIdentifier == i-1) {	//identifier must be right before (, else we have some expression like "( x + y() )"
					curValue.lastFunctionIdentifier = curValue.lastIdentifier;
					curValue.param = 0;
				} else {	//some expression
					curValue.lastFunctionIdentifier = -1;
				}
			} else if (curToken.token[0] == _param && curValue.lastFunctionIdentifier > -1) {
				curValue.param++;
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

		if (!_funcName || !stringEqualN(_funcName, funcToken.token, strlen(_funcName))) {	//check if we need to reload data
			if (_funcName)
				delete [] _funcName;
			_funcName = new char[strlen(funcToken.token) + 1];	//add the return type aswell
			strcpy(_funcName, funcToken.token);
			res = loadData();
		} else {
			res = true;
		}
	}
	if (!res) {
		reset();
		if (_funcName) {
			delete [] _funcName;
			_funcName = 0;
		}
	}
	delete [] lineData;
	return res;
}

/*
CTF (CalltipFormat) file structure (ignore spaces).
There MUST be a return type, however, may be empty string (or empty line even)
_no_ CR. ESC = 027 = 0x1B
(
NULL FunctionName LF
return value (ESC possibleparam)* LF
)*
NULL
*/

bool FunctionCallTip::loadData() {
	char filePath[MAX_PATH];
	::GetModuleFileName(NULL, filePath, MAX_PATH);
	PathRemoveFileSpec(filePath);
	strcat(filePath, "\\plugins\\APIs\\");
	strcat(filePath, _langName);
	strcat(filePath, ".ctf");

	FILE * ctfFile = fopen(filePath, "rb");	//CalltipFormat File
	if (!ctfFile)
		return false;

	//Copy all data, close with \n to be sure
	fseek(ctfFile, 0, SEEK_END );
	size_t fileSize = ftell(ctfFile);
	fseek(ctfFile, 0, SEEK_SET );
	char * fileData = new char[fileSize+1];
	size_t nbChar = fread(fileData, 1, fileSize, ctfFile);
	fileData[nbChar] = 0;
	fclose(ctfFile);

	//The functions should be ordered, but linear search because we cant access like array
	size_t i = 0, location = 0;
	bool found = false;
	int funcLen = strlen(_funcName);
	fileSize -= funcLen;	//if not enough is left, no need to keep searching
	while(i < fileSize) {
		if (fileData[i] == 0) {	//reached new function
			//case sensitive search
			if (stringEqualN(_funcName, fileData+i+1, funcLen)) {	//found it
				found = true;
				location = i + 2 + funcLen;	//skip zero + LF + name of func (all known at this point)
				break;
			}
		}
		i++;
	}
	if (!found) {
		delete [] fileData;
		return false;
	}

	int argStart = location;
	std::vector< pair<int, int> > overloadLocations;	//start, length
	while(fileData[location] != 0) {	//keep reading each line of overloads untill next function is found
		argStart = location;
		while(fileData[location] != '\n') {
			location++;
		}
		overloadLocations.push_back(pair<int, int>(argStart, location-argStart));
		location++;	//skip newline
	}
	
	_currentNrOverloads = overloadLocations.size();
	_currentOverloads = new char*[_currentNrOverloads];
	_params = new vector<int>[_currentNrOverloads];

	int j = 0;
	for(int i = 0; i < _currentNrOverloads; i++) {
			pair<int, int> & cur = overloadLocations.at(i);
			_currentOverloads[i] = new char[cur.second+1];
			memcpy(_currentOverloads[i], fileData+cur.first, cur.second);
			_currentOverloads[i][cur.second] = 0;
			j = 0;
			while(_currentOverloads[i][j] != 0) {
				if (_currentOverloads[i][j] == 0x1B) {
					_currentOverloads[i][j] = 0;
					_params[i].push_back(j+1);
				}
				j++;
			}
	}


	delete [] fileData;
	return true;
}

void FunctionCallTip::showCalltip() {
	if (_currentNrOverloads == 0) {
		//ASSERT
		return;
	}
	char * curOverloadText = _currentOverloads[_currentOverload];
	int bytesNeeded = strlen(curOverloadText) + strlen(_funcName) + 5;//'retval funcName (params)\0'
	size_t nrParams = _params[_currentOverload].size();
	if (nrParams) {
		for(size_t i = 0; i < nrParams; i++) {
			bytesNeeded += strlen(curOverloadText+_params[_currentOverload][i]) + 2;	//'param, '
		}
	}

	if (_currentNrOverloads > 1) {
		bytesNeeded += 24;	//  /\00001 of 00003\/
	}
	char * textBuffer = new char[bytesNeeded];
	textBuffer[0] = 0;

	if (_currentNrOverloads > 1) {
		sprintf(textBuffer, "\001%u of %u\002", _currentOverload+1, _currentNrOverloads);
	}

	strcat(textBuffer, curOverloadText);
	strcat(textBuffer, " ");
	strcat(textBuffer, _funcName);
	strcat(textBuffer, " (");

	int highlightstart = 0;
	int highlightend = 0;
	for(size_t i = 0; i < nrParams; i++) {
		if (i == _currentParam) {
			highlightstart = strlen(textBuffer);
			highlightend = highlightstart + strlen(curOverloadText+_params[_currentOverload][i]);
		}
		strcat(textBuffer, curOverloadText+_params[_currentOverload][i]);
		if (i < nrParams-1)
			strcat(textBuffer, ", ");
	}

	strcat(textBuffer, ")");

	if (isVisible())
		_pEditView->execute(SCI_CALLTIPCANCEL);
	else
		_startPos = _curPos;
	_pEditView->execute(SCI_CALLTIPSHOW, _startPos, (LPARAM)textBuffer);

	if (highlightstart != highlightend) {
		_pEditView->execute(SCI_CALLTIPSETHLT, highlightstart, highlightend);
	}

	delete [] textBuffer;
}

void FunctionCallTip::reset() {
	_currentOverload = 0;
	//_currentNrOverloads = 0;
	_currentParam = 0;
	_curPos = 0;
	_startPos = 0;
}

void FunctionCallTip::cleanup() {
	if (_currentOverloads) {
		for(int i = 0; i < _currentNrOverloads; i++)
			delete [] _currentOverloads[i];
		_currentNrOverloads = 0;
		delete [] _currentOverloads;
		_currentOverloads = 0;
	}
	if(_funcName) {
		delete [] _funcName;
		_funcName = 0;
	}
	if (_params) {
		delete [] _params;
		_params = 0;
	}
	if (_langName) {
		delete [] _langName;
		_langName = 0;
	}
	reset();
	_pEditView = NULL;
	_currentOverload = 0;
	_currentParam = 0;
	_initialized = false;
}

