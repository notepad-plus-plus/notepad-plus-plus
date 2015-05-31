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

#include <algorithm>
#include <locale>
#include <shlwapi.h>
#include "AutoCompletion.h"
#include "Notepad_plus_msgs.h"

using namespace std;

static bool isInList(generic_string word, const vector<generic_string> & wordArray)
{
	for (size_t i = 0, len = wordArray.size(); i < len; ++i)
		if (wordArray[i] == word)
			return true;
	return false;
};


bool AutoCompletion::showApiComplete()
{
	if (!_funcCompletionActive)
		return false;

	// calculate entered word's length
	int curPos = int(_pEditView->execute(SCI_GETCURRENTPOS));
	int startPos = int(_pEditView->execute(SCI_WORDSTARTPOSITION, curPos, true));

	if (curPos == startPos)
		return false;

	size_t len = (curPos > startPos)?(curPos - startPos):(startPos - curPos);
	if (len >= _keyWordMaxLen)
		return false;

	_pEditView->execute(SCI_AUTOCSETSEPARATOR, WPARAM(' '));
	_pEditView->execute(SCI_AUTOCSETIGNORECASE, _ignoreCase);
	_pEditView->showAutoComletion(curPos - startPos, _keyWords.c_str());

	return true;
}

bool AutoCompletion::showApiAndWordComplete()
{
	int curPos = int(_pEditView->execute(SCI_GETCURRENTPOS));
	int startPos = int(_pEditView->execute(SCI_WORDSTARTPOSITION, curPos, true));

	if (curPos == startPos)
		return false;

	const size_t bufSize = 256;
	TCHAR beginChars[bufSize];
	
	size_t len = (curPos > startPos)?(curPos - startPos):(startPos - curPos);
	if (len >= bufSize)
		return false;

	// Get word array
	vector<generic_string> wordArray;
	_pEditView->getGenericText(beginChars, bufSize, startPos, curPos);

	getWordArray(wordArray, beginChars);


	for (size_t i = 0, len = _keyWordArray.size(); i < len; ++i)
	{
		if (_keyWordArray[i].find(beginChars) == 0)
		{
			if (!isInList(_keyWordArray[i], wordArray))
				wordArray.push_back(_keyWordArray[i]);
		}
	}

	sort(wordArray.begin(), wordArray.end());

	// Get word list
	generic_string words(TEXT(""));

	for (size_t i = 0, len = wordArray.size(); i < len; ++i)
	{
		words += wordArray[i];
		if (i != wordArray.size()-1)
			words += TEXT(" ");
	}

	_pEditView->execute(SCI_AUTOCSETSEPARATOR, WPARAM(' '));
	_pEditView->execute(SCI_AUTOCSETIGNORECASE, _ignoreCase);
	_pEditView->showAutoComletion(curPos - startPos, words.c_str());

	return true;
}

void AutoCompletion::getWordArray(vector<generic_string> & wordArray, TCHAR *beginChars)
{
	const size_t bufSize = 256;

	generic_string expr(TEXT("\\<"));
	expr += beginChars;
	expr += TEXT("[^ \\t\\n\\r.,;:\"()=<>'+!\\[\\]]*");

	int docLength = int(_pEditView->execute(SCI_GETLENGTH));

	int flags = SCFIND_WORDSTART | SCFIND_MATCHCASE | SCFIND_REGEXP | SCFIND_POSIX;

	_pEditView->execute(SCI_SETSEARCHFLAGS, flags);
	int posFind = _pEditView->searchInTarget(expr.c_str(), expr.length(), 0, docLength);

	while (posFind != -1 && posFind != -2)
	{
		int wordStart = int(_pEditView->execute(SCI_GETTARGETSTART));
		int wordEnd = int(_pEditView->execute(SCI_GETTARGETEND));

		size_t foundTextLen = wordEnd - wordStart;
		if (foundTextLen < bufSize)
		{
			TCHAR w[bufSize];
			_pEditView->getGenericText(w, bufSize, wordStart, wordEnd);

			if (lstrcmp(w, beginChars) != 0)
				if (!isInList(w, wordArray))
					wordArray.push_back(w);
		}
		posFind = _pEditView->searchInTarget(expr.c_str(), expr.length(), wordEnd, docLength);
	}
}

static generic_string addTrailingSlash(generic_string path)
{
	if(path.length() >=1 && path[path.length() - 1] == '\\')
		return path;
	else
		return path + L"\\";
}

static generic_string removeTrailingSlash(generic_string path)
{
	if(path.length() >= 1 && path[path.length() - 1] == '\\')
		return path.substr(0, path.length() - 1);
	else
		return path;
}

static bool isDirectory(generic_string path)
{
	DWORD type = ::GetFileAttributes(path.c_str());
	return type != INVALID_FILE_ATTRIBUTES && (type & FILE_ATTRIBUTE_DIRECTORY);
}

static bool isFile(generic_string path)
{
	DWORD type = ::GetFileAttributes(path.c_str());
	return type != INVALID_FILE_ATTRIBUTES && ! (type & FILE_ATTRIBUTE_DIRECTORY);
}

static bool isAllowedBeforeDriveLetter(TCHAR c)
{
	locale loc;
	return c == '\'' || c == '"' || c == '(' || std::isspace(c, loc);
}

static bool getRawPath(generic_string input, generic_string &rawPath_out)
{
	// Try to find a path in the given input.
	// Algorithm: look for a colon. The colon must be preceded by an alphabetic character.
	// The alphabetic character must, in turn, be preceded by nothing, or by whitespace, or by
	// a quotation mark.
	locale loc;
	size_t lastOccurrence = input.rfind(L":");
	if(lastOccurrence == std::string::npos) // No match.
		return false;
	else if(lastOccurrence == 0)
		return false;
	else if(!std::isalpha(input[lastOccurrence - 1], loc))
		return false;
	else if(lastOccurrence >= 2 && !isAllowedBeforeDriveLetter(input[lastOccurrence - 2]))
		return false;

	rawPath_out = input.substr(lastOccurrence - 1);
	return true;
}

static bool getPathsForPathCompletion(generic_string input, generic_string &rawPath_out, generic_string &pathToMatch_out)
{
	generic_string rawPath;
	if(! getRawPath(input, rawPath))
	{
		return false;
	}
	else if(isFile(rawPath) || isFile(removeTrailingSlash(rawPath)))
	{
		return false;
	}
	else if(isDirectory(rawPath))
	{
		rawPath_out = rawPath;
		pathToMatch_out = rawPath;
		return true;
	}
	else
	{
		size_t last_occurrence = rawPath.rfind(L"\\");
		if(last_occurrence == std::string::npos) // No match.
			return false;
		else
		{
			rawPath_out = rawPath;
			pathToMatch_out = rawPath.substr(0, last_occurrence);
			return true;
		}
	}
}

void AutoCompletion::showPathCompletion()
{
	// Get current line (at most MAX_PATH characters "backwards" from current caret).
	generic_string currentLine;
	{
		const long bufSize = MAX_PATH;
		TCHAR buf[bufSize + 1];
		const int currentPos = _pEditView->execute(SCI_GETCURRENTPOS);
		const int startPos = max(0, currentPos - bufSize);
		_pEditView->getGenericText(buf, bufSize + 1, startPos, currentPos);
		currentLine = buf;
	}

	/* Try to figure out which path the user wants us to complete.
	   We need to know the "raw path", which is what the user actually wrote.
	   But we also need to know which directory to look in (pathToMatch), which might
	   not be the same as what the user wrote. This happens when the user types an
	   incomplete name.
	   For instance: the user wants to autocomplete "C:\Wind", and assuming that no such directory
	   exists, this means we should list all files and directories in C:.
	*/
	generic_string rawPath, pathToMatch;
	if(! getPathsForPathCompletion(currentLine, rawPath, pathToMatch))
		return;

	// Get all files and directories in the path.
	generic_string autoCompleteEntries;
	{
		HANDLE hFind;
		WIN32_FIND_DATA data;
		generic_string pathToMatchPlusSlash = addTrailingSlash(pathToMatch);
		generic_string searchString = pathToMatchPlusSlash + TEXT("*.*");
		hFind = ::FindFirstFile(searchString.c_str(), &data);
		if(hFind != INVALID_HANDLE_VALUE)
		{
			// Maximum number of entries to show. Without this it appears to the user like N++ hangs when autocompleting
			// some really large directories (c:\windows\winxsys on my system for instance).
			const unsigned int maxEntries = 2000;
			unsigned int counter = 0;
			do
			{
				if(++counter > maxEntries)
					break;

				if(generic_string(data.cFileName) == TEXT(".") || generic_string(data.cFileName) == TEXT(".."))
					continue;

				if(! autoCompleteEntries.empty())
					autoCompleteEntries += TEXT("\n");

				autoCompleteEntries += pathToMatchPlusSlash;
				autoCompleteEntries += data.cFileName;
				if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) // If directory, add trailing slash.
					autoCompleteEntries += TEXT("\\");

			} while(::FindNextFile(hFind, &data));
			::FindClose(hFind);
		}
		else
			return;
	}

	// Show autocompletion box.
	_pEditView->execute(SCI_AUTOCSETSEPARATOR, WPARAM('\n'));
	_pEditView->execute(SCI_AUTOCSETIGNORECASE, true);
	_pEditView->showAutoComletion(rawPath.length(), autoCompleteEntries.c_str());
	return;
}

bool AutoCompletion::showWordComplete(bool autoInsert)
{
	int curPos = int(_pEditView->execute(SCI_GETCURRENTPOS));
	int startPos = int(_pEditView->execute(SCI_WORDSTARTPOSITION, curPos, true));

	if (curPos == startPos)
		return false;

	const size_t bufSize = 256;
	TCHAR beginChars[bufSize];
	
	size_t len = (curPos > startPos)?(curPos - startPos):(startPos - curPos);
	if (len >= bufSize)
		return false;

	// Get word array
	vector<generic_string> wordArray;
	_pEditView->getGenericText(beginChars, bufSize, startPos, curPos);

	getWordArray(wordArray, beginChars);

	if (wordArray.size() == 0) return false;

	if (wordArray.size() == 1 && autoInsert) 
	{
		_pEditView->replaceTargetRegExMode(wordArray[0].c_str(), startPos, curPos);
		_pEditView->execute(SCI_GOTOPOS, startPos + wordArray[0].length());
		return true;
	}

	sort(wordArray.begin(), wordArray.end());

	// Get word list
	generic_string words(TEXT(""));

	for (size_t i = 0, len = wordArray.size(); i < len; ++i)
	{
		words += wordArray[i];
		if (i != wordArray.size()-1)
			words += TEXT(" ");
	}

	_pEditView->execute(SCI_AUTOCSETSEPARATOR, WPARAM(' '));
	_pEditView->execute(SCI_AUTOCSETIGNORECASE, _ignoreCase);
	_pEditView->showAutoComletion(curPos - startPos, words.c_str());
	return true;
}

bool AutoCompletion::showFunctionComplete()
{
	if (!_funcCompletionActive)
		return false;

	if (_funcCalltip.updateCalltip(0, true))
	{
		return true;
	}
	return false;
}

void AutoCompletion::getCloseTag(char *closeTag, size_t closeTagSize, size_t caretPos)
{
	int flags = SCFIND_REGEXP | SCFIND_POSIX;
	_pEditView->execute(SCI_SETSEARCHFLAGS, flags);
	TCHAR tag2find[] = TEXT("<[^\\s>]*");
	int targetStart = _pEditView->searchInTarget(tag2find, lstrlen(tag2find), caretPos, 0);
	
	if (targetStart == -1 || targetStart == -2)
		return;

	int targetEnd = int(_pEditView->execute(SCI_GETTARGETEND));
	int foundTextLen = targetEnd - targetStart;
	if (foundTextLen < 2) // "<>" will be ignored
		return;

	if (size_t(foundTextLen) > closeTagSize - 2) // buffer size is not large enough. -2 for '/' & '\0'
		return;

	char tagHead[3];
	_pEditView->getText(tagHead, targetStart, targetStart+2);

	if (tagHead[1] == '/') // "</toto>" will be ignored
		return;

	char tagTail[2];
	_pEditView->getText(tagTail, caretPos-2, caretPos-1);

	if (tagTail[0] == '/') // "<toto/>" and "<toto arg="0" />" will be ignored
		return;

	closeTag[0] = '<';
	closeTag[1] = '/';
	_pEditView->getText(closeTag + 2, targetStart + 1, targetEnd);
	closeTag[foundTextLen+1] = '>';
	closeTag[foundTextLen+2] = '\0'; 
}

void InsertedMatchedChars::removeInvalidElements(MatchedCharInserted mci)
{
	if (mci._c == '\n' || mci._c == '\r') // create a new line, so all matched char are invalidated
	{
		_insertedMatchedChars.clear();
	}
	else
	{
		for (int i = _insertedMatchedChars.size() - 1; i >= 0; --i)
		{
			if (_insertedMatchedChars[i]._pos < mci._pos)
			{
				int posToDetectLine = _pEditView->execute(SCI_LINEFROMPOSITION, mci._pos);
				int startPosLine = _pEditView->execute(SCI_LINEFROMPOSITION, _insertedMatchedChars[i]._pos);

				if (posToDetectLine != startPosLine) //not in the same line
				{
					_insertedMatchedChars.erase(_insertedMatchedChars.begin() + i);
				}
			}
			else // current position is before matchedStartSybol Pos
			{
				_insertedMatchedChars.erase(_insertedMatchedChars.begin() + i);
			}
		}
	}
}

void InsertedMatchedChars::add(MatchedCharInserted mci)
{
	removeInvalidElements(mci);
	_insertedMatchedChars.push_back(mci);
}

// if current pos > matchedStartSybol Pos and current pos is on the same line of matchedStartSybolPos, it'll be checked then removed
// otherwise it is just removed
// return the pos of matchedEndSybol or -1 if matchedEndSybol not found
int InsertedMatchedChars::search(char startChar, char endChar, int posToDetect)
{
	if (isEmpty())
		return -1;
	int posToDetectLine = _pEditView->execute(SCI_LINEFROMPOSITION, posToDetect);
	
	for (int i = _insertedMatchedChars.size() - 1; i >= 0; --i)
	{
		if (_insertedMatchedChars[i]._c == startChar)
		{
			if (_insertedMatchedChars[i]._pos < posToDetect)
			{
				int startPosLine = _pEditView->execute(SCI_LINEFROMPOSITION, _insertedMatchedChars[i]._pos);
				if (posToDetectLine == startPosLine)
				{
					int endPos = _pEditView->execute(SCI_GETLINEENDPOSITION, startPosLine);

					for (int j = posToDetect; j <= endPos; ++j)
					{
						char aChar = (char)_pEditView->execute(SCI_GETCHARAT, j);

						if (aChar != ' ') // non space is not allowed
						{

							if (aChar == endChar) // found it!!!
							{
								_insertedMatchedChars.erase(_insertedMatchedChars.begin() + i);
								return j;
							}
							else // whichever character, stop searching
							{
								_insertedMatchedChars.erase(_insertedMatchedChars.begin() + i);
								return -1;
							}
						}
					}
				}
				else // not in the same line
				{
					_insertedMatchedChars.erase(_insertedMatchedChars.begin() + i);
				}
			}
			else // current position is before matchedStartSybol Pos
			{
				_insertedMatchedChars.erase(_insertedMatchedChars.begin() + i);
			}
		}
	}
	return -1;
}

void AutoCompletion::insertMatchedChars(int character, const MatchedPairConf & matchedPairConf)
{
	const vector< pair<char, char> > & matchedPairs = matchedPairConf._matchedPairs;
	int caretPos = _pEditView->execute(SCI_GETCURRENTPOS);
	char *matchedChars = NULL;

	// User defined matched pairs should be checked firstly
	for (size_t i = 0, len = matchedPairs.size(); i < len; ++i)
	{
		if (int(matchedPairs[i].first) == character)
		{
			char userMatchedChar[2] = {'\0', '\0'};
			userMatchedChar[0] = matchedPairs[i].second;
			_pEditView->execute(SCI_INSERTTEXT, caretPos, (LPARAM)userMatchedChar);
			return;
		}
	}

	// if there's no user defined matched pair found, continue to check notepad++'s one
	
	const size_t closeTagLen = 256;
	char closeTag[closeTagLen];
	closeTag[0] = '\0';
	switch (character)
	{
		case int('('):
			if (matchedPairConf._doParentheses)
			{
				matchedChars = ")";
				_insertedMatchedChars.add(MatchedCharInserted(char(character), caretPos - 1));
			}
		break;

		case int('['):
			if (matchedPairConf._doBrackets)
			{
				matchedChars = "]";
				_insertedMatchedChars.add(MatchedCharInserted(char(character), caretPos - 1));
			}
		break;

		case int('{'):
			if (matchedPairConf._doCurlyBrackets)
			{
				matchedChars = "}";
				_insertedMatchedChars.add(MatchedCharInserted(char(character), caretPos - 1));
			}
		break;

		case int('"'):
			if (matchedPairConf._doDoubleQuotes)
			{
				if (!_insertedMatchedChars.isEmpty())
				{
					int pos = _insertedMatchedChars.search('"', char(character), caretPos);
					if (pos != -1)
					{
						_pEditView->execute(SCI_DELETERANGE, pos, 1);
						_pEditView->execute(SCI_GOTOPOS, pos);
						return;
					}
				}

				matchedChars = "\"";
				_insertedMatchedChars.add(MatchedCharInserted(char(character), caretPos - 1));
			}
		break;
		case int('\''):
			if (matchedPairConf._doQuotes)
			{
				if (!_insertedMatchedChars.isEmpty())
				{
					int pos = _insertedMatchedChars.search('\'', char(character), caretPos);
					if (pos != -1)
					{
						_pEditView->execute(SCI_DELETERANGE, pos, 1);
						_pEditView->execute(SCI_GOTOPOS, pos);
						return;
					}
				}
				matchedChars = "'";
				_insertedMatchedChars.add(MatchedCharInserted(char(character), caretPos - 1));
			}
		break;

		case int('>'):
		{
			if (matchedPairConf._doHtmlXmlTag && (_curLang == L_HTML || _curLang == L_XML))
			{
				getCloseTag(closeTag, closeTagLen, caretPos);
				if (closeTag[0] != '\0')
					matchedChars = closeTag;
			}
		}
		break;

		case int(')') :
		case int(']') :
		case int('}') :
			if (!_insertedMatchedChars.isEmpty())
			{
				char startChar;
				if (character == int(')'))
				{
					if (!matchedPairConf._doParentheses)
						return;
					startChar = '(';
				}
				else if (character == int(']'))
				{
					if (!matchedPairConf._doBrackets)
						return;
					startChar = '[';
				}
				else // if (character == int('}'))
				{
					if (!matchedPairConf._doCurlyBrackets)
						return;
					startChar = '{';
				}

				int pos = _insertedMatchedChars.search(startChar, char(character), caretPos);
				if (pos != -1)
				{
					_pEditView->execute(SCI_DELETERANGE, pos, 1);
					_pEditView->execute(SCI_GOTOPOS, pos);
				}
				return;
			}
			break;

		default:
			if (!_insertedMatchedChars.isEmpty())
				_insertedMatchedChars.removeInvalidElements(MatchedCharInserted(char(character), caretPos - 1));
	}

	if (matchedChars)
		_pEditView->execute(SCI_INSERTTEXT, caretPos, (LPARAM)matchedChars);
}


void AutoCompletion::update(int character)
{
	const NppGUI & nppGUI = NppParameters::getInstance()->getNppGUI();
	if (!_funcCompletionActive && nppGUI._autocStatus == nppGUI.autoc_func)
		return;

	if (nppGUI._funcParams || _funcCalltip.isVisible()) {
		if (_funcCalltip.updateCalltip(character)) {	//calltip visible because triggered by autocomplete, set mode
			return;	//only return in case of success, else autocomplete
		}
	}

	if (!character)
		return;

	//If autocomplete already active, let Scintilla handle it
	if (_pEditView->execute(SCI_AUTOCACTIVE) != 0)
		return;

	const int wordSize = 64;
	TCHAR s[wordSize];
	_pEditView->getWordToCurrentPos(s, wordSize);
	
	if (lstrlen(s) >= int(nppGUI._autocFromLen))
	{
		if (nppGUI._autocStatus == nppGUI.autoc_word)
			showWordComplete(false);
		else if (nppGUI._autocStatus == nppGUI.autoc_func)
			showApiComplete();
		else if (nppGUI._autocStatus == nppGUI.autoc_both)
			showApiAndWordComplete();

	}
}

void AutoCompletion::callTipClick(int direction) {
	if (!_funcCompletionActive)
		return;

	if (direction == 1) {
		_funcCalltip.showPrevOverload();
	} else if (direction == 2) {
		_funcCalltip.showNextOverload();
	}
}

bool AutoCompletion::setLanguage(LangType language) {
	if (_curLang == language)
		return true;
	_curLang = language;

	TCHAR path[MAX_PATH];
	::GetModuleFileName(NULL, path, MAX_PATH);
	PathRemoveFileSpec(path);
	lstrcat(path, TEXT("\\plugins\\APIs\\"));
	lstrcat(path, getApiFileName());
	lstrcat(path, TEXT(".xml"));

	if (_pXmlFile)
		delete _pXmlFile;

	_pXmlFile = new TiXmlDocument(path);
	_funcCompletionActive = _pXmlFile->LoadFile();

	TiXmlNode * pAutoNode = NULL;
	if (_funcCompletionActive) {
		_funcCompletionActive = false;	//safety
		TiXmlNode * pNode = _pXmlFile->FirstChild(TEXT("NotepadPlus"));
		if (!pNode)
			return false;
		pAutoNode = pNode = pNode->FirstChildElement(TEXT("AutoComplete"));
		if (!pNode)
			return false;
		pNode = pNode->FirstChildElement(TEXT("KeyWord"));
		if (!pNode)
			return false;
		_pXmlKeyword = reinterpret_cast<TiXmlElement *>(pNode);
		if (!_pXmlKeyword)
			return false;
		_funcCompletionActive = true;
	}

	if(_funcCompletionActive) //try setting up environment
    {
		//setup defaults
		_ignoreCase = true;
		_funcCalltip._start = '(';
		_funcCalltip._stop = ')';
		_funcCalltip._param = ',';
		_funcCalltip._terminal = ';';
		_funcCalltip._ignoreCase = true;
        _funcCalltip._additionalWordChar = TEXT("");

		TiXmlElement * pElem = pAutoNode->FirstChildElement(TEXT("Environment"));
		if (pElem) 
        {	
			const TCHAR * val = 0;
			val = pElem->Attribute(TEXT("ignoreCase"));
			if (val && !lstrcmp(val, TEXT("no"))) {
				_ignoreCase = false;
				_funcCalltip._ignoreCase = false;
			}
			val = pElem->Attribute(TEXT("startFunc"));
			if (val && val[0])
				_funcCalltip._start = val[0];
			val = pElem->Attribute(TEXT("stopFunc"));
			if (val && val[0])
				_funcCalltip._stop = val[0];
			val = pElem->Attribute(TEXT("paramSeparator"));
			if (val && val[0])
				_funcCalltip._param = val[0];
			val = pElem->Attribute(TEXT("terminal"));
			if (val && val[0])
				_funcCalltip._terminal = val[0];
			val = pElem->Attribute(TEXT("additionalWordChar"));
			if (val && val[0])
                _funcCalltip._additionalWordChar = val;
		}
	}

	if (_funcCompletionActive) {
		_funcCalltip.setLanguageXML(_pXmlKeyword);
	} else {
		_funcCalltip.setLanguageXML(NULL);
	}

	_keyWords = TEXT("");
	_keyWordArray.clear();

	if (_funcCompletionActive)
	{
		//Cache the keywords
		//Iterate through all keywords
		TiXmlElement *funcNode = _pXmlKeyword;

		for (; funcNode; funcNode = funcNode->NextSiblingElement(TEXT("KeyWord")) )
		{
			const TCHAR *name = funcNode->Attribute(TEXT("name"));
			if (name)
			{
				size_t len = lstrlen(name);
				if (len)
				{
					_keyWordArray.push_back(name);
					if (len > _keyWordMaxLen)
						_keyWordMaxLen = len;
				}
			}
		}

		sort(_keyWordArray.begin(), _keyWordArray.end());

		for (size_t i = 0, len = _keyWordArray.size(); i < len; ++i)
		{
			_keyWords.append(_keyWordArray[i]);
			_keyWords.append(TEXT(" "));
		}
	}
	return _funcCompletionActive;
}

const TCHAR * AutoCompletion::getApiFileName()
{
	if (_curLang == L_USER)
	{
		Buffer * currentBuf = _pEditView->getCurrentBuffer();
		if (currentBuf->isUserDefineLangExt())
		{
			return currentBuf->getUserDefineLangName();
		}
	}

	if (_curLang >= L_EXTERNAL && _curLang < NppParameters::getInstance()->L_END)
		return NppParameters::getInstance()->getELCFromIndex(_curLang - L_EXTERNAL)._name;

	if (_curLang > L_EXTERNAL)
        _curLang = L_TEXT;

	return ScintillaEditView::langNames[_curLang].lexerName;

}
