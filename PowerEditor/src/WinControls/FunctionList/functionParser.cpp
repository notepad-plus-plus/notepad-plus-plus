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

#include "precompiledHeaders.h"
#include "ScintillaEditView.h"
#include "functionParser.h"
#include "boostregexsearch.h"

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

		
		std::vector<generic_string> classNameExprArray;
		const TCHAR *functionExpr = NULL;
		std::vector<generic_string> functionNameExprArray;

		const TCHAR *displayName = (childNode->ToElement())->Attribute(TEXT("displayName"));
		if (!displayName || !displayName[0])
			displayName = id;

		const TCHAR *commentExpr = NULL;

		TiXmlNode *classRangeParser = childNode->FirstChild(TEXT("classRange"));
		if (classRangeParser)
		{
			const TCHAR *mainExpr = NULL;
			const TCHAR *openSymbole = NULL;
			const TCHAR *closeSymbole = NULL;

			mainExpr = (classRangeParser->ToElement())->Attribute(TEXT("mainExpr"));
			if (!mainExpr)
				continue;

			openSymbole = (classRangeParser->ToElement())->Attribute(TEXT("openSymbole"));
			closeSymbole = (classRangeParser->ToElement())->Attribute(TEXT("closeSymbole"));
			TiXmlNode *commentSymbols = classRangeParser->FirstChild(TEXT("comment"));
			if (commentSymbols)
			{
				commentExpr = (commentSymbols->ToElement())->Attribute(TEXT("expr"));
			}

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
				continue;

			functionExpr = (functionParser->ToElement())->Attribute(TEXT("mainExpr"));
			if (!functionExpr)
				continue;

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

			_parsers.push_back(new FunctionZoneParser(id, displayName, commentExpr, mainExpr, openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray));
		}
		else
		{
			TiXmlNode *functionParser = childNode->FirstChild(TEXT("function"));
			if (!functionParser)
			{
				continue;
			}

			const TCHAR *mainExpr = (functionParser->ToElement())->Attribute(TEXT("mainExpr"));
			if (!mainExpr)
				continue;

			TiXmlNode *commentSymbols = functionParser->FirstChild(TEXT("comment"));
			if (commentSymbols)
			{
				commentExpr = (commentSymbols->ToElement())->Attribute(TEXT("expr"));
			}

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
			if (functionNameParser)
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
			_parsers.push_back(new FunctionUnitParser(id, displayName, commentExpr, mainExpr, functionNameExprArray, classNameExprArray));
		}
	}

	TiXmlNode *associationMapRoot = root->FirstChild(TEXT("associationMap"));
	if (associationMapRoot) 
	{
		for (TiXmlNode *childNode = associationMapRoot->FirstChildElement(TEXT("association"));
			childNode;
			childNode = childNode->NextSibling(TEXT("association")) )
		{
			const TCHAR *ext = (childNode->ToElement())->Attribute(TEXT("ext"));
			const TCHAR *id = (childNode->ToElement())->Attribute(TEXT("id"));
			if (ext && ext[0] && id && id[0])
			{
				for (size_t i = 0; i < _parsers.size(); i++)
				{
					if (_parsers[i]->_id == id)
					{
						_associationMap.push_back(std::pair<generic_string, size_t>(ext, i));
						break;
					}
				}
			}
		}
	}

	return (_parsers.size() != 0);
}

FunctionParser * FunctionParsersManager::getParser(generic_string ext)
{
	for (size_t i = 0; i < _associationMap.size(); i++)
	{
		if (ext == _associationMap[i].first)
			return _parsers[_associationMap[i].second];
	}
	return NULL;
}

void FunctionParser::funcParse(std::vector<foundInfo> & foundInfos,  size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName)
{
	if (begin >= end)
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

			if (_classNameExprArray.size())
			{
				fi._data2 = parseSubLevel(targetStart, targetEnd, _classNameExprArray, foundPos, ppEditView);
				fi._pos2 = foundPos;
			}
			else if (classStructName != TEXT(""))
			{
				fi._data2 = classStructName;
				fi._pos2 = 0; // change -1 valeur for validated data2
			}
		}

		if (fi._pos != -1 || fi._pos2 != -1) // at least one should be found
		{
			foundInfos.push_back(fi);
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

bool FunctionParsersManager::parse(std::vector<foundInfo> & foundInfos, generic_string ext)
{
	if (!_pXmlFuncListDoc)
		return false;

	// Serch the right parser from the given ext in the map
	FunctionParser *fp = getParser(ext);
	if (!fp)
		return false;

	// parse
	int docLen = (*_ppEditView)->getCurrentDocLen();
	fp->parse(foundInfos, 0, docLen, _ppEditView);

	return true;
}


size_t FunctionZoneParser::getBodyClosePos(size_t begin, const TCHAR *bodyOpenSymbol, const TCHAR *bodyCloseSymbol, ScintillaEditView **ppEditView)
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

			// Now we determinate the symbol (open or close)
			int tmpStart = (*ppEditView)->searchInTarget(bodyOpenSymbol, lstrlen(bodyOpenSymbol), targetStart, targetEnd);
			if (tmpStart != -1 && tmpStart != -2) // open symbol found 
			{
				cntOpen++;
			}
			else // if it's not open symbol, then it must be the close one
			{
				cntOpen--;
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

void FunctionZoneParser::classParse(vector<foundInfo> & foundInfos, vector< pair<int, int> > &scannedZone, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName)
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
		scannedZone.push_back(pair<int, int>(targetStart, targetEnd));

		// Get class name
		int foundPos = 0;
		generic_string classStructName = parseSubLevel(targetStart, targetEnd, _classNameExprArray, foundPos, ppEditView);
		

		if (_openSymbole != TEXT("") && _closeSymbole != TEXT(""))
		{
			targetEnd = getBodyClosePos(targetEnd, _openSymbole.c_str(), _closeSymbole.c_str(), ppEditView);
		}

		if (targetEnd > int(end)) //we found a result but outside our range, therefore do not process it
		{
			break;
		}
		int foundTextLen = targetEnd - targetStart;
		if (targetStart + foundTextLen == int(end))
            break;

		// Begin to search all method inside
		vector< generic_string > emptyArray;
		funcParse(foundInfos, targetStart, targetEnd, ppEditView, classStructName);

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

void FunctionParser::getInvertZones(vector< pair<int, int> > &  destZones, vector< pair<int, int> > &  sourceZones, size_t begin, size_t end)
{
	if (sourceZones.size() == 0)
	{
		destZones.push_back(pair<int, int>(begin, end));
	}
	else
	{
		// todo : check the begin
		size_t i = 0;
		for (; i < sourceZones.size() - 1; i++)
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
	vector< pair<int, int> > commentZones, nonCommentZones;
	getCommentZones(commentZones, begin, end, ppEditView);
	getInvertZones(nonCommentZones, commentZones, begin, end);
	for (size_t i = 0; i < nonCommentZones.size(); i++)
	{
		classParse(foundInfos, commentZones, nonCommentZones[i].first, nonCommentZones[i].second, ppEditView, classStructName);
	}
}

void FunctionUnitParser::parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName)
{
	vector< pair<int, int> > commentZones, nonCommentZones;
	getCommentZones(commentZones, begin, end, ppEditView);
	getInvertZones(nonCommentZones, commentZones, begin, end);
	for (size_t i = 0; i < nonCommentZones.size(); i++)
	{
		funcParse(foundInfos, nonCommentZones[i].first, nonCommentZones[i].second, ppEditView, classStructName);
	}
}