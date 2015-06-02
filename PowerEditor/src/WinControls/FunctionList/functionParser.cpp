// This file is part of Notepad++ project
// Copyright (C)2012 Don HO <don.h@free.fr>
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

#include <shlwapi.h>
#include "ScintillaEditView.h"
#include "functionParser.h"
#include "boostregexsearch.h"

using namespace std;

FunctionParsersManager::~FunctionParsersManager()
{
	for (size_t i = 0, len = _parsers.size(); i < len; ++i)
	{
		delete _parsers[i];
	}

	if (_pXmlFuncListDoc)
		delete _pXmlFuncListDoc;
}

bool FunctionParsersManager::init(generic_string xmlPath, ScintillaEditView ** ppEditView)
{
	_ppEditView = ppEditView;
	bool loadOkay = false;

	if (PathFileExists(xmlPath.c_str()))
	{
        _pXmlFuncListDoc = new TiXmlDocument(xmlPath);
        loadOkay = _pXmlFuncListDoc->LoadFile();
        if (loadOkay)
        {
            loadOkay = getFuncListFromXmlTree();
        }
    }

	return loadOkay;
}

bool FunctionParsersManager::getZonePaserParameters(TiXmlNode *classRangeParser, generic_string &mainExprStr, generic_string &openSymboleStr, generic_string &closeSymboleStr, std::vector<generic_string> &classNameExprArray, generic_string &functionExprStr, std::vector<generic_string> &functionNameExprArray)
{
	const TCHAR *mainExpr = NULL;
	const TCHAR *openSymbole = NULL;
	const TCHAR *closeSymbole = NULL;
	const TCHAR *functionExpr = NULL;

	mainExpr = (classRangeParser->ToElement())->Attribute(TEXT("mainExpr"));
	if (!mainExpr || !mainExpr[0])
		return false;
	mainExprStr = mainExpr;

	openSymbole = (classRangeParser->ToElement())->Attribute(TEXT("openSymbole"));
	if (openSymbole && openSymbole[0])
		openSymboleStr = openSymbole;

	closeSymbole = (classRangeParser->ToElement())->Attribute(TEXT("closeSymbole"));
	if (closeSymbole && closeSymbole[0])
		closeSymboleStr = closeSymbole;

	TiXmlNode *classNameParser = classRangeParser->FirstChild(TEXT("className"));
	if (classNameParser)
	{
		for (TiXmlNode *childNode2 = classNameParser->FirstChildElement(TEXT("nameExpr"));
			childNode2;
			childNode2 = childNode2->NextSibling(TEXT("nameExpr")) )
		{
			const TCHAR *expr = (childNode2->ToElement())->Attribute(TEXT("expr"));
			if (expr && expr[0])
				classNameExprArray.push_back(expr);
		}
	}

	TiXmlNode *functionParser = classRangeParser->FirstChild(TEXT("function"));
	if (!functionParser)
		return false;

	functionExpr = (functionParser->ToElement())->Attribute(TEXT("mainExpr"));
	if (!functionExpr || !functionExpr[0])
		return false;
	functionExprStr = functionExpr;

	TiXmlNode *functionNameParser = functionParser->FirstChild(TEXT("functionName"));
	if (functionNameParser)
	{
		for (TiXmlNode *childNode3 = functionNameParser->FirstChildElement(TEXT("funcNameExpr"));
			childNode3;
			childNode3 = childNode3->NextSibling(TEXT("funcNameExpr")) )
		{
			const TCHAR *expr = (childNode3->ToElement())->Attribute(TEXT("expr"));
			if (expr && expr[0])
				functionNameExprArray.push_back(expr);
		}
		
	}
	return true;
}

bool FunctionParsersManager::getUnitPaserParameters(TiXmlNode *functionParser, generic_string &mainExprStr, std::vector<generic_string> &functionNameExprArray, std::vector<generic_string> &classNameExprArray)
{
	const TCHAR *mainExpr = (functionParser->ToElement())->Attribute(TEXT("mainExpr"));
	if (!mainExpr || !mainExpr[0])
		return false;
	mainExprStr = mainExpr;

	TiXmlNode *functionNameParser = functionParser->FirstChild(TEXT("functionName"));
	if (functionNameParser)
	{
		for (TiXmlNode *childNode = functionNameParser->FirstChildElement(TEXT("nameExpr"));
			childNode;
			childNode = childNode->NextSibling(TEXT("nameExpr")) )
		{
			const TCHAR *expr = (childNode->ToElement())->Attribute(TEXT("expr"));
			if (expr && expr[0])
				functionNameExprArray.push_back(expr);
		}
	}

	TiXmlNode *classNameParser = functionParser->FirstChild(TEXT("className"));
	if (classNameParser)
	{
		for (TiXmlNode *childNode = classNameParser->FirstChildElement(TEXT("nameExpr"));
			childNode;
			childNode = childNode->NextSibling(TEXT("nameExpr")) )
		{
			const TCHAR *expr = (childNode->ToElement())->Attribute(TEXT("expr"));
			if (expr && expr[0])
				classNameExprArray.push_back(expr);
		}
	}
	return true;
}

void FunctionParsersManager::writeFunctionListXml(const TCHAR *destFoder) const 
{
	generic_string dest = destFoder;
	PathAppend(dest, TEXT("functionList.xml"));
	if (_pXmlFuncListDoc)
		_pXmlFuncListDoc->SaveFile(dest.c_str());
}

bool FunctionParsersManager::getFuncListFromXmlTree()
{
	if (!_pXmlFuncListDoc)
		return false;
	
	TiXmlNode *root = _pXmlFuncListDoc->FirstChild(TEXT("NotepadPlus"));
	if (!root) 
		return false;

	root = root->FirstChild(TEXT("functionList"));
	if (!root) 
		return false;

	TiXmlNode *parserRoot = root->FirstChild(TEXT("parsers"));
	if (!parserRoot) 
		return false;


	for (TiXmlNode *childNode = parserRoot->FirstChildElement(TEXT("parser"));
		childNode;
		childNode = childNode->NextSibling(TEXT("parser")) )
	{
		const TCHAR *id = (childNode->ToElement())->Attribute(TEXT("id"));
		if (!id || !id[0])
			continue;

		generic_string commentExpr(TEXT(""));
		const TCHAR *pCommentExpr = (childNode->ToElement())->Attribute(TEXT("commentExpr"));
		if (pCommentExpr && pCommentExpr[0])
			commentExpr = pCommentExpr;
		

		std::vector<generic_string> classNameExprArray;
		std::vector<generic_string> functionNameExprArray;

		const TCHAR *displayName = (childNode->ToElement())->Attribute(TEXT("displayName"));
		if (!displayName || !displayName[0])
			displayName = id;

		TiXmlNode *classRangeParser = childNode->FirstChild(TEXT("classRange"));
		TiXmlNode *functionParser = childNode->FirstChild(TEXT("function"));
		if (classRangeParser && functionParser)
		{
			generic_string mainExpr, openSymbole, closeSymbole, functionExpr;
			getZonePaserParameters(classRangeParser, mainExpr, openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray);
			
			generic_string mainExpr2;
			std::vector<generic_string> classNameExprArray2;
			std::vector<generic_string> functionNameExprArray2;
			getUnitPaserParameters(functionParser, mainExpr2, functionNameExprArray2, classNameExprArray2);
			FunctionUnitParser *funcUnitPaser = new FunctionUnitParser(id, displayName, commentExpr.c_str(), mainExpr2.c_str(), functionNameExprArray2, classNameExprArray2);

			_parsers.push_back(new FunctionMixParser(id, displayName, commentExpr.c_str(), mainExpr.c_str(), openSymbole.c_str(), closeSymbole.c_str(), classNameExprArray, functionExpr.c_str(), functionNameExprArray, funcUnitPaser));
		}
		else if (classRangeParser)
		{
			generic_string mainExpr, openSymbole, closeSymbole, functionExpr;
			getZonePaserParameters(classRangeParser, mainExpr, openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray);
			_parsers.push_back(new FunctionZoneParser(id, displayName, commentExpr.c_str(), mainExpr.c_str(), openSymbole.c_str(), closeSymbole.c_str(), classNameExprArray, functionExpr.c_str(), functionNameExprArray));
		}
		else if (functionParser)
		{
			generic_string  mainExpr;
			getUnitPaserParameters(functionParser, mainExpr, functionNameExprArray, classNameExprArray);
			_parsers.push_back(new FunctionUnitParser(id, displayName, commentExpr.c_str(), mainExpr.c_str(), functionNameExprArray, classNameExprArray));
		}
	}

	TiXmlNode *associationMapRoot = root->FirstChild(TEXT("associationMap"));
	if (associationMapRoot) 
	{
		for (TiXmlNode *childNode = associationMapRoot->FirstChildElement(TEXT("association"));
			childNode;
			childNode = childNode->NextSibling(TEXT("association")) )
		{
			int langID;
			const TCHAR *langIDStr = (childNode->ToElement())->Attribute(TEXT("langID"), &langID);
			const TCHAR *exts = (childNode->ToElement())->Attribute(TEXT("ext"));
			const TCHAR *id = (childNode->ToElement())->Attribute(TEXT("id"));
			const TCHAR *userDefinedLangName = (childNode->ToElement())->Attribute(TEXT("userDefinedLangName"));
			if (((langIDStr && langIDStr[0]) || (exts && exts[0]) || (userDefinedLangName && userDefinedLangName[0])) && (id && id[0]))
			{
				for (size_t i = 0, len = _parsers.size(); i < len; ++i)
				{
					if (_parsers[i]->_id == id)
					{
						_associationMap.push_back(AssociationInfo(i, langIDStr?langID:-1, exts?exts:TEXT(""), userDefinedLangName?userDefinedLangName:TEXT("")));
						break;
					}
				}
			}
		}
	}

	return (_parsers.size() != 0);
}

FunctionParser * FunctionParsersManager::getParser(const AssociationInfo & assoInfo)
{
	const unsigned char doNothing = 0;
	const unsigned char checkLangID = 1;
	const unsigned char checkUserDefined = 2;
	const unsigned char checkExt = 3;

	unsigned char choice = doNothing;
	// langID != -1 && langID != L_USER
	if (assoInfo._langID != -1 && assoInfo._langID != L_USER)
		choice = checkLangID;
	// langID == L_USER, we chack the userDefinedLangName
	else if (assoInfo._langID == L_USER && assoInfo._userDefinedLangName != TEXT(""))
		choice = checkUserDefined;
	// langID == -1, we chack the ext
	else if (assoInfo._langID == -1 && assoInfo._ext != TEXT(""))
		choice = checkExt;
	else
		return NULL;

	for (size_t i = 0, len = _associationMap.size(); i < len; ++i)
	{
		switch (choice)
		{
			case checkLangID:
			{
				if (assoInfo._langID == _associationMap[i]._langID)
					return _parsers[_associationMap[i]._id];			
			}
			break;

			case checkUserDefined:
			{
				if (assoInfo._userDefinedLangName == _associationMap[i]._userDefinedLangName)
					return _parsers[_associationMap[i]._id];			
			}
			break;

			case checkExt:
			{
				if (assoInfo._ext == _associationMap[i]._ext)
					return _parsers[_associationMap[i]._id];		
			}
			break;

		}
	}
	return NULL;
}


void FunctionParser::funcParse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName, const std::vector< std::pair<int, int> > * commentZones)
{
	if (begin >= end)
		return;

	if (_functionExpr.length() == 0)
		return;

	int flags = SCFIND_REGEXP | SCFIND_POSIX | SCFIND_REGEXP_DOTMATCHESNL;

	(*ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	int targetStart = (*ppEditView)->searchInTarget(_functionExpr.c_str(), _functionExpr.length(), begin, end);
	int targetEnd = 0;
	
	//foundInfos.clear();
	while (targetStart != -1 && targetStart != -2)
	{
		targetStart = int((*ppEditView)->execute(SCI_GETTARGETSTART));
		targetEnd = int((*ppEditView)->execute(SCI_GETTARGETEND));
		if (targetEnd > int(end)) //we found a result but outside our range, therefore do not process it
		{
			break;
		}
		int foundTextLen = targetEnd - targetStart;
		if (targetStart + foundTextLen == int(end))
            break;

		foundInfo fi;

		// dataToSearch & data2ToSearch are optional
		if (!_functionNameExprArray.size() && !_classNameExprArray.size())
		{
			TCHAR foundData[1024];
			(*ppEditView)->getGenericText(foundData, 1024, targetStart, targetEnd);

			fi._data = foundData; // whole found data
			fi._pos = targetStart;

		}
		else
		{
			int foundPos;
			if (_functionNameExprArray.size())
			{
				fi._data = parseSubLevel(targetStart, targetEnd, _functionNameExprArray, foundPos, ppEditView);
				fi._pos = foundPos;
			}

			if (classStructName != TEXT(""))
			{
				fi._data2 = classStructName;
				fi._pos2 = -1; // change -1 valeur for validated data2
			}
			else if (_classNameExprArray.size())
			{
				fi._data2 = parseSubLevel(targetStart, targetEnd, _classNameExprArray, foundPos, ppEditView);
				fi._pos2 = foundPos;
			}
		}

		if (fi._pos != -1 || fi._pos2 != -1) // at least one should be found
		{
			if (commentZones != NULL)
			{
				if (!isInZones(fi._pos, *commentZones) && !isInZones(fi._pos2, *commentZones))
					foundInfos.push_back(fi);
			}
			else
			{
				foundInfos.push_back(fi);
			}
		}
		
		begin = targetStart + foundTextLen;
		targetStart = (*ppEditView)->searchInTarget(_functionExpr.c_str(), _functionExpr.length(), begin, end);
	}
}

generic_string FunctionParser::parseSubLevel(size_t begin, size_t end, std::vector< generic_string > dataToSearch, int & foundPos, ScintillaEditView **ppEditView)
{
	if (begin >= end)
	{
		foundPos = -1;
		return TEXT("");
	}

	if (!dataToSearch.size())
		return TEXT("");

	int flags = SCFIND_REGEXP | SCFIND_POSIX  | SCFIND_REGEXP_DOTMATCHESNL;

	(*ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	const TCHAR *regExpr2search = dataToSearch[0].c_str();
	int targetStart = (*ppEditView)->searchInTarget(regExpr2search, lstrlen(regExpr2search), begin, end);

	if (targetStart == -1 || targetStart == -2)
	{
		foundPos = -1;
		return TEXT("");
	}
	int targetEnd = int((*ppEditView)->execute(SCI_GETTARGETEND));

	if (dataToSearch.size() >= 2)
	{
		dataToSearch.erase(dataToSearch.begin());
		return parseSubLevel(targetStart, targetEnd, dataToSearch, foundPos, ppEditView);
	}
	else // only one processed element, so we conclude the result
	{
		TCHAR foundStr[1024];

		(*ppEditView)->getGenericText(foundStr, 1024, targetStart, targetEnd);

		foundPos = targetStart;
		return foundStr;
	}
}

bool FunctionParsersManager::parse(std::vector<foundInfo> & foundInfos, const AssociationInfo & assoInfo)
{
	if (!_pXmlFuncListDoc)
		return false;

	// Serch the right parser from the given ext in the map
	FunctionParser *fp = getParser(assoInfo);
	if (!fp)
		return false;

	// parse
	int docLen = (*_ppEditView)->getCurrentDocLen();
	fp->parse(foundInfos, 0, docLen, _ppEditView);

	return true;
}

size_t FunctionZoneParser::getBodyClosePos(size_t begin, const TCHAR *bodyOpenSymbol, const TCHAR *bodyCloseSymbol, const std::vector< std::pair<int, int> > & commentZones, ScintillaEditView **ppEditView)
{
	size_t cntOpen = 1;

	int docLen = (*ppEditView)->getCurrentDocLen();

	if (begin >= (size_t)docLen)
		return docLen;

	generic_string exprToSearch = TEXT("(");
	exprToSearch += bodyOpenSymbol;
	exprToSearch += TEXT("|");
	exprToSearch += bodyCloseSymbol;
	exprToSearch += TEXT(")");


	int flags = SCFIND_REGEXP | SCFIND_POSIX | SCFIND_REGEXP_DOTMATCHESNL;

	(*ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	int targetStart = (*ppEditView)->searchInTarget(exprToSearch.c_str(), exprToSearch.length(), begin, docLen);
	int targetEnd = 0;

	do
	{
		if (targetStart != -1 && targetStart != -2) // found open or close symbol
		{
			targetEnd = int((*ppEditView)->execute(SCI_GETTARGETEND));

			// Treat it only if it's NOT in the comment zone
			if (!isInZones(targetStart, commentZones))
			{
				// Now we determinate the symbol (open or close)
				int tmpStart = (*ppEditView)->searchInTarget(bodyOpenSymbol, lstrlen(bodyOpenSymbol), targetStart, targetEnd);
				if (tmpStart != -1 && tmpStart != -2) // open symbol found 
				{
					++cntOpen;
				}
				else // if it's not open symbol, then it must be the close one
				{
					--cntOpen;
				}
			}
		}
		else // nothing found
		{
			cntOpen = 0; // get me out of here
			targetEnd = begin;
		}

		targetStart = (*ppEditView)->searchInTarget(exprToSearch.c_str(), exprToSearch.length(), targetEnd, docLen);

	} while (cntOpen);

	return targetEnd;
}

void FunctionZoneParser::classParse(vector<foundInfo> & foundInfos, vector< pair<int, int> > &scannedZones, const std::vector< std::pair<int, int> > & commentZones, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName)
{
	if (begin >= end)
		return;

	int flags = SCFIND_REGEXP | SCFIND_POSIX | SCFIND_REGEXP_DOTMATCHESNL;

	(*ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	int targetStart = (*ppEditView)->searchInTarget(_rangeExpr.c_str(), _rangeExpr.length(), begin, end);

	int targetEnd = 0;
	
	while (targetStart != -1 && targetStart != -2)
	{
		targetEnd = int((*ppEditView)->execute(SCI_GETTARGETEND));

		// Get class name
		int foundPos = 0;
		generic_string classStructName = parseSubLevel(targetStart, targetEnd, _classNameExprArray, foundPos, ppEditView);
		

		if (_openSymbole != TEXT("") && _closeSymbole != TEXT(""))
		{
			targetEnd = getBodyClosePos(targetEnd, _openSymbole.c_str(), _closeSymbole.c_str(), commentZones, ppEditView);
		}

		if (targetEnd > int(end)) //we found a result but outside our range, therefore do not process it
		{
			break;
		}
		
		scannedZones.push_back(pair<int, int>(targetStart, targetEnd));

		int foundTextLen = targetEnd - targetStart;
		if (targetStart + foundTextLen == int(end))
            break;

		// Begin to search all method inside
		//vector< generic_string > emptyArray;
		if (!isInZones(targetStart, commentZones))
		{
			funcParse(foundInfos, targetStart, targetEnd, ppEditView, classStructName, &commentZones);
		}
		begin = targetStart + (targetEnd - targetStart);
		targetStart = (*ppEditView)->searchInTarget(_rangeExpr.c_str(), _rangeExpr.length(), begin, end);
	}
}

void FunctionParser::getCommentZones(vector< pair<int, int> > & commentZone, size_t begin, size_t end, ScintillaEditView **ppEditView)
{
	if ((begin >= end) || (_commentExpr == TEXT("")))
	{
		return;
	}

	int flags = SCFIND_REGEXP | SCFIND_POSIX | SCFIND_REGEXP_DOTMATCHESNL;

	(*ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	int targetStart = (*ppEditView)->searchInTarget(_commentExpr.c_str(), _commentExpr.length(), begin, end);
	int targetEnd = 0;
	
	while (targetStart != -1 && targetStart != -2)
	{
		targetStart = int((*ppEditView)->execute(SCI_GETTARGETSTART));
		targetEnd = int((*ppEditView)->execute(SCI_GETTARGETEND));
		if (targetEnd > int(end)) //we found a result but outside our range, therefore do not process it
		{
			break;
		}

		commentZone.push_back(pair<int, int>(targetStart, targetEnd));

		int foundTextLen = targetEnd - targetStart;
		if (targetStart + foundTextLen == int(end))
            break;
		
		begin = targetStart + foundTextLen;
		targetStart = (*ppEditView)->searchInTarget(_commentExpr.c_str(), _commentExpr.length(), begin, end);
	}
}

bool FunctionParser::isInZones(int pos2Test, const std::vector< std::pair<int, int> > & zones)
{
	for (size_t i = 0, len = zones.size(); i < len; ++i)
	{
		if (pos2Test >= zones[i].first && pos2Test < zones[i].second)
		{
			return true;
		}
	}
	return false;
}

void FunctionParser::getInvertZones(vector< pair<int, int> > &  destZones, vector< pair<int, int> > &  sourceZones, size_t begin, size_t end)
{
	if (sourceZones.size() == 0)
	{
		destZones.push_back(pair<int, int>(begin, end));
	}
	else
	{
		// check the begin
		if (int(begin) < sourceZones[0].first)
		{
			destZones.push_back(pair<int, int>(begin, sourceZones[0].first - 1));
		}

		size_t i = 0;
		for (size_t len = sourceZones.size() - 1; i < len; ++i)
		{
			int newBegin = sourceZones[i].second + 1;
			int newEnd = sourceZones[i+1].first - 1;
			if (newBegin < newEnd)
				destZones.push_back(pair<int, int>(newBegin, newEnd));
		}
		int lastBegin = sourceZones[i].second + 1;
		if (lastBegin < int(end))
			destZones.push_back(pair<int, int>(lastBegin, end));		
	}
}

void FunctionZoneParser::parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName)
{
	vector< pair<int, int> > classZones, commentZones, nonCommentZones;
	getCommentZones(commentZones, begin, end, ppEditView);
	getInvertZones(nonCommentZones, commentZones, begin, end);
	for (size_t i = 0, len = nonCommentZones.size(); i < len; ++i)
	{
		classParse(foundInfos, classZones, commentZones, nonCommentZones[i].first, nonCommentZones[i].second, ppEditView, classStructName);
	}
}

void FunctionUnitParser::parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName)
{
	vector< pair<int, int> > commentZones, nonCommentZones;
	getCommentZones(commentZones, begin, end, ppEditView);
	getInvertZones(nonCommentZones, commentZones, begin, end);
	for (size_t i = 0, len = nonCommentZones.size(); i < len; ++i)
	{
		funcParse(foundInfos, nonCommentZones[i].first, nonCommentZones[i].second, ppEditView, classStructName);
	}
}

//
// SortClass for vector<pair<int, int>>
// sort in _selLpos : increased order
struct SortZones {
	bool operator() (pair<int, int> & l, pair<int, int> & r) {
		return (l.first < r.first);
	}
};

void FunctionMixParser::parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName)
{
	vector< pair<int, int> > commentZones, scannedZones, nonCommentZones, nonScannedZones;
	getCommentZones(commentZones, begin, end, ppEditView);

	classParse(foundInfos, scannedZones, commentZones, begin, end, ppEditView, classStructName);

	// the second level
	for (size_t i = 0, len = scannedZones.size(); i < len; ++i)
	{
		vector< pair<int, int> > temp;
		classParse(foundInfos, temp, commentZones, scannedZones[i].first, scannedZones[i].second, ppEditView, classStructName);
	}
	// invert scannedZones
	getInvertZones(nonScannedZones, scannedZones, begin, end);

	// for each nonScannedZones, search functions
	if (_funcUnitPaser)
	{
		for (size_t i = 0, len = nonScannedZones.size(); i < len; ++i)
		{
			_funcUnitPaser->funcParse(foundInfos, nonScannedZones[i].first, nonScannedZones[i].second, ppEditView, classStructName);
		}
	}
}