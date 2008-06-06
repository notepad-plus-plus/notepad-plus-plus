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

#include "AutoCompletion.h"
#include "Notepad_plus_msgs.h"

static bool isInList(string word, const vector<string> & wordArray)
{
	for (size_t i = 0 ; i < wordArray.size() ; i++)
		if (wordArray[i] == word)
			return true;
	return false;
};

AutoCompletion::AutoCompletion(ScintillaEditView * pEditView) : _active(false), _pEditView(pEditView), _funcCalltip(pEditView), 
																_curLang(L_TXT), _XmlFile(NULL), _activeCompletion(CompletionNone),
																_pXmlKeyword(NULL), _ignoreCase(true), _keyWords("")
{
	//Do not load any language yet
}

bool AutoCompletion::showAutoComplete() {
	if (!_active)
		return false;

	int curPos = int(_pEditView->execute(SCI_GETCURRENTPOS));
	int line = _pEditView->getCurrentLineNumber();
	int startLinePos = int(_pEditView->execute(SCI_POSITIONFROMLINE, line ));
	int startWordPos = startLinePos;

	int len = curPos-startLinePos;
	char * lineBuffer = new char[len+1];
	_pEditView->getText(lineBuffer, startLinePos, curPos);
	//_pEditView->execute(SCI_GETTEXT, (WPARAM)len, (LPARAM)lineBuffer);

	int offset = len-1;
	int nrChars = 0;
	char c;
	while (offset>=0)
	{
		c = lineBuffer[offset];
		if (isalnum(c) || c == '_') {
			nrChars++;
		} else {
			break;
		}
		offset--;
		
	}
	startWordPos = curPos-nrChars;

	_pEditView->execute(SCI_AUTOCSETSEPARATOR, WPARAM('\n'));
	_pEditView->execute(SCI_AUTOCSETIGNORECASE, _ignoreCase);
	_pEditView->execute(SCI_AUTOCSHOW, curPos-startWordPos, WPARAM(_keyWords.c_str()));

	_activeCompletion = CompletionAuto;
	return true;
}

bool AutoCompletion::showWordComplete(bool autoInsert) {
	if (!_active)
		return false;

	int curPos = int(_pEditView->execute(SCI_GETCURRENTPOS));
	int startPos = int(_pEditView->execute(SCI_WORDSTARTPOSITION, curPos, true));

	if (curPos == startPos)
		return false;

	const size_t bufSize = 256;
	size_t len = (curPos > startPos)?(curPos - startPos):(startPos - curPos);
	if (len >= bufSize)
		return false;

	char beginChars[bufSize];

	_pEditView->getText(beginChars, startPos, curPos);

	string expr("\\<");
	expr += beginChars;
	expr += "[^ \\t.,;:\"()=<>'+!\\[\\]]*";

	int docLength = int(_pEditView->execute(SCI_GETLENGTH));

	int flags = SCFIND_WORDSTART | SCFIND_MATCHCASE | SCFIND_REGEXP | SCFIND_POSIX;

	_pEditView->execute(SCI_SETTARGETSTART, 0);
	_pEditView->execute(SCI_SETTARGETEND, docLength);
	_pEditView->execute(SCI_SETSEARCHFLAGS, flags);
	
	vector<string> wordArray;

	int posFind = int(_pEditView->execute(SCI_SEARCHINTARGET, expr.length(), (LPARAM)expr.c_str()));

	while (posFind != -1)
	{
		int wordStart = int(_pEditView->execute(SCI_GETTARGETSTART));
		int wordEnd = int(_pEditView->execute(SCI_GETTARGETEND));
		
		size_t foundTextLen = wordEnd - wordStart;

		if (foundTextLen < bufSize)
		{
			char w[bufSize];
			_pEditView->getText(w, wordStart, wordEnd);

			if (strcmp(w, beginChars))
				if (!isInList(w, wordArray))
					wordArray.push_back(w);
		}
		_pEditView->execute(SCI_SETTARGETSTART, wordEnd/*posFind + foundTextLen*/);
		_pEditView->execute(SCI_SETTARGETEND, docLength);
		posFind = int(_pEditView->execute(SCI_SEARCHINTARGET, expr.length(), (LPARAM)expr.c_str()));
	}
	if (wordArray.size() == 0) return false;

	if (wordArray.size() == 1 && autoInsert) 
	{
		_pEditView->execute(SCI_SETTARGETSTART, startPos);
		_pEditView->execute(SCI_SETTARGETEND, curPos);
		_pEditView->execute(SCI_REPLACETARGETRE, wordArray[0].length(), (LPARAM)wordArray[0].c_str());
		
		_pEditView->execute(SCI_GOTOPOS, startPos + wordArray[0].length());
		return true;
	}

	string words("");

	for (size_t i = 0 ; i < wordArray.size() ; i++)
	{
		words += wordArray[i];
		if (i != wordArray.size()-1)
			words += " ";
	}

	_pEditView->execute(SCI_AUTOCSETSEPARATOR, WPARAM(' '));
	_pEditView->execute(SCI_AUTOCSETIGNORECASE, _ignoreCase);
	_pEditView->execute(SCI_AUTOCSHOW, curPos - startPos, WPARAM(words.c_str()));

	_activeCompletion = CompletionWord;
	return true;
}

bool AutoCompletion::showFunctionComplete() {
	if (!_active)
		return false;

	if (_funcCalltip.updateCalltip(0, true)) {
		_activeCompletion = CompletionFunc;
		return true;
	}
	return false;
}

void AutoCompletion::update(int character) {
	if (!_active)
		return;

	const NppGUI & nppGUI = NppParameters::getInstance()->getNppGUI();

	if (nppGUI._funcParams || _funcCalltip.isVisible()) {
		if (_funcCalltip.updateCalltip(character)) {	//calltip visible because triggered by autocomplete, set mode
			_activeCompletion = CompletionFunc;
			return;	//only return in case of success, else autocomplete
		}
	}

	if (!character)
		return;

	//If autocomplete already active, let Scintilla handle it
	if (_pEditView->execute(SCI_AUTOCACTIVE) != 0)
		return;

	char s[64];
	_pEditView->getWordToCurrentPos(s, sizeof(s));
	
	if (strlen(s) >= nppGUI._autocFromLen)
	{
		if (nppGUI._autocStatus == nppGUI.autoc_word)
			showWordComplete(false);
		else if (nppGUI._autocStatus == nppGUI.autoc_func)
			showAutoComplete();
	}
}

void AutoCompletion::callTipClick(int direction) {
	if (!_active)
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

	char path[MAX_PATH];
	::GetModuleFileName(NULL, path, MAX_PATH);
	PathRemoveFileSpec(path);
	strcat(path, "\\plugins\\APIs\\");
	strcat(path, getApiFileName());
	strcat(path, ".xml");

	_XmlFile = TiXmlDocument(path);
	_active = _XmlFile.LoadFile();

	TiXmlNode * pAutoNode = NULL;
	if (_active) {
		_active = false;	//safety
		TiXmlNode * pNode = _XmlFile.FirstChild("NotepadPlus");
		if (!pNode)
			return false;
		pAutoNode = pNode = pNode->FirstChildElement("AutoComplete");
		if (!pNode)
			return false;
		pNode = pNode->FirstChildElement("KeyWord");
		if (!pNode)
			return false;
		_pXmlKeyword = reinterpret_cast<TiXmlElement *>(pNode);
		if (!_pXmlKeyword)
			return false;
		_active = true;
	}

	if(_active) {	//try setting up environment
		//setup defaults
		_ignoreCase = true;
		_funcCalltip._start = '(';
		_funcCalltip._stop = ')';
		_funcCalltip._param = ',';
		_funcCalltip._terminal = ';';

		TiXmlElement * pElem = pAutoNode->FirstChildElement("Environment");
		if (pElem) {	
			const char * val = 0;
			val = pElem->Attribute("ignoreCase");
			if (val && !strcmp(val, "no"))
				_ignoreCase = false;
			val = pElem->Attribute("startFunc");
			if (val && val[0])
				_funcCalltip._start = val[0];
			val = pElem->Attribute("stopFunc");
			if (val && val[0])
				_funcCalltip._stop = val[0];
			val = pElem->Attribute("paramSeparator");
			if (val && val[0])
				_funcCalltip._param = val[0];
			val = pElem->Attribute("terminal");
			if (val && val[0])
				_funcCalltip._terminal = val[0];
		}
	}

	if (_active) {
		_funcCalltip.setLanguageXML(_pXmlKeyword);
	} else {
		_funcCalltip.setLanguageXML(NULL);
	}

	_keyWords = "";
	if (_active) {	//Cache the keywords
		//Iterate through all keywords
		TiXmlElement *funcNode = _pXmlKeyword;
		const char * name = NULL;
		for (; funcNode; funcNode = funcNode->NextSiblingElement("KeyWord") ) {
			name = funcNode->Attribute("name");
			if (!name)		//malformed node
				continue;
			_keyWords.append(name);
			_keyWords.append("\n");
		}
	}
	return _active;
}

const char * AutoCompletion::getApiFileName() {
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

	return ScintillaEditView::langNames[_curLang].lexerName;

}
