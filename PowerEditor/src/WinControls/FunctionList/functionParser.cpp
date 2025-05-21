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

#include <shlwapi.h>
#include "ScintillaEditView.h"
#include "functionParser.h"
#include "BoostRegexSearch.h"

using namespace std;

FunctionParsersManager::~FunctionParsersManager()
{
	for (size_t i = 0, len = L_EXTERNAL + nbMaxUserDefined; i < len; ++i)
	{
		if (_parsers[i] != nullptr)
			delete _parsers[i];
	}
}

bool FunctionParsersManager::init(const wstring& xmlDirPath, const wstring& xmlInstalledPath, ScintillaEditView ** ppEditView)
{
	_ppEditView = ppEditView;
	_xmlDirPath = xmlDirPath;
	_xmlDirInstalledPath = xmlInstalledPath;

	bool isOK = getOverrideMapFromXmlTree(_xmlDirPath);
	if (isOK)
		return true;
	else if (_xmlDirPath != _xmlDirInstalledPath && !_xmlDirInstalledPath.empty())
		return getOverrideMapFromXmlTree(_xmlDirInstalledPath);
	else
		return false;
}

bool FunctionParsersManager::getZonePaserParameters(TiXmlNode *classRangeParser, wstring &mainExprStr, wstring &openSymboleStr, wstring &closeSymboleStr, std::vector<wstring> &classNameExprArray, wstring &functionExprStr, std::vector<wstring> &functionNameExprArray)
{
	const wchar_t *mainExpr = NULL;
	const wchar_t *openSymbole = NULL;
	const wchar_t *closeSymbole = NULL;
	const wchar_t *functionExpr = NULL;

	mainExpr = (classRangeParser->ToElement())->Attribute(L"mainExpr");
	if (!mainExpr || !mainExpr[0])
		return false;
	mainExprStr = mainExpr;

	openSymbole = (classRangeParser->ToElement())->Attribute(L"openSymbole");
	if (openSymbole && openSymbole[0])
		openSymboleStr = openSymbole;

	closeSymbole = (classRangeParser->ToElement())->Attribute(L"closeSymbole");
	if (closeSymbole && closeSymbole[0])
		closeSymboleStr = closeSymbole;

	TiXmlNode *classNameParser = classRangeParser->FirstChild(L"className");
	if (classNameParser)
	{
		for (TiXmlNode *childNode2 = classNameParser->FirstChildElement(L"nameExpr");
			childNode2;
			childNode2 = childNode2->NextSibling(L"nameExpr") )
		{
			const wchar_t *expr = (childNode2->ToElement())->Attribute(L"expr");
			if (expr && expr[0])
				classNameExprArray.push_back(expr);
		}
	}

	TiXmlNode *functionParser = classRangeParser->FirstChild(L"function");
	if (!functionParser)
		return false;

	functionExpr = (functionParser->ToElement())->Attribute(L"mainExpr");
	if (!functionExpr || !functionExpr[0])
		return false;
	functionExprStr = functionExpr;

	TiXmlNode *functionNameParser = functionParser->FirstChild(L"functionName");
	if (functionNameParser)
	{
		for (TiXmlNode *childNode3 = functionNameParser->FirstChildElement(L"funcNameExpr");
			childNode3;
			childNode3 = childNode3->NextSibling(L"funcNameExpr") )
		{
			const wchar_t *expr = (childNode3->ToElement())->Attribute(L"expr");
			if (expr && expr[0])
				functionNameExprArray.push_back(expr);
		}
		
	}
	return true;
}

bool FunctionParsersManager::getUnitPaserParameters(TiXmlNode *functionParser, wstring &mainExprStr, std::vector<wstring> &functionNameExprArray, std::vector<wstring> &classNameExprArray)
{
	const wchar_t *mainExpr = (functionParser->ToElement())->Attribute(L"mainExpr");
	if (!mainExpr || !mainExpr[0])
		return false;
	mainExprStr = mainExpr;

	TiXmlNode *functionNameParser = functionParser->FirstChild(L"functionName");
	if (functionNameParser)
	{
		for (TiXmlNode *childNode = functionNameParser->FirstChildElement(L"nameExpr");
			childNode;
			childNode = childNode->NextSibling(L"nameExpr") )
		{
			const wchar_t *expr = (childNode->ToElement())->Attribute(L"expr");
			if (expr && expr[0])
				functionNameExprArray.push_back(expr);
		}
	}

	TiXmlNode *classNameParser = functionParser->FirstChild(L"className");
	if (classNameParser)
	{
		for (TiXmlNode *childNode = classNameParser->FirstChildElement(L"nameExpr");
			childNode;
			childNode = childNode->NextSibling(L"nameExpr") )
		{
			const wchar_t *expr = (childNode->ToElement())->Attribute(L"expr");
			if (expr && expr[0])
				classNameExprArray.push_back(expr);
		}
	}
	return true;
}


bool FunctionParsersManager::loadFuncListFromXmlTree(const wstring & xmlDirPath, LangType lType, const wstring& overrideId, int udlIndex)
{
	wstring funcListRulePath = xmlDirPath;
	funcListRulePath += L"\\";
	int index = -1;
	if (lType == L_USER) // UDL
	{
		if (overrideId.empty())
			return false;
		
		if (udlIndex == -1)
			return false;

		index = udlIndex;
		funcListRulePath += overrideId;
	}
	else // Supported Language
	{
		index = lType;
		if (overrideId.empty())
		{
			wstring lexerName = ScintillaEditView::_langNameInfoArray[lType]._langName;
			funcListRulePath += lexerName;
			funcListRulePath += L".xml";
		}
		else
		{
			funcListRulePath += overrideId;
		}
	}

	if (index > _currentUDIndex)
		return false;

	if (_parsers[index] == nullptr)
		return false;

	TiXmlDocument xmlFuncListDoc(funcListRulePath);
	bool loadOK = xmlFuncListDoc.LoadFile();

	if (!loadOK)
		return false;

	TiXmlNode *root = xmlFuncListDoc.FirstChild(L"NotepadPlus");
	if (!root)
		return false;

	root = root->FirstChild(L"functionList");
	if (!root)
		return false;

	TiXmlNode *parserRoot = root->FirstChild(L"parser");
	if (!parserRoot)
		return false;

	const wchar_t *id = (parserRoot->ToElement())->Attribute(L"id");
	if (!id || !id[0])
		return false;

	wstring commentExpr(L"");
	const wchar_t *pCommentExpr = (parserRoot->ToElement())->Attribute(L"commentExpr");
	if (pCommentExpr && pCommentExpr[0])
		commentExpr = pCommentExpr;

	std::vector<wstring> classNameExprArray;
	std::vector<wstring> functionNameExprArray;

	const wchar_t *displayName = (parserRoot->ToElement())->Attribute(L"displayName");
	if (!displayName || !displayName[0])
		displayName = id;

	TiXmlNode *classRangeParser = parserRoot->FirstChild(L"classRange");
	TiXmlNode *functionParser = parserRoot->FirstChild(L"function");
	if (classRangeParser && functionParser)
	{
		wstring mainExpr, openSymbole, closeSymbole, functionExpr;
		getZonePaserParameters(classRangeParser, mainExpr, openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray);

		wstring mainExpr2;
		std::vector<wstring> classNameExprArray2;
		std::vector<wstring> functionNameExprArray2;
		getUnitPaserParameters(functionParser, mainExpr2, functionNameExprArray2, classNameExprArray2);
		FunctionUnitParser *funcUnitPaser = new FunctionUnitParser(id, displayName, commentExpr.c_str(), mainExpr2, functionNameExprArray2, classNameExprArray2);

		_parsers[index]->_parser = new FunctionMixParser(id, displayName, commentExpr.c_str(), mainExpr, openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray, funcUnitPaser);
	}
	else if (classRangeParser)
	{
		wstring mainExpr, openSymbole, closeSymbole, functionExpr;
		getZonePaserParameters(classRangeParser, mainExpr, openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray);
		_parsers[index]->_parser = new FunctionZoneParser(id, displayName, commentExpr.c_str(), mainExpr.c_str(), openSymbole.c_str(), closeSymbole.c_str(), classNameExprArray, functionExpr.c_str(), functionNameExprArray);
	}
	else if (functionParser)
	{
		wstring  mainExpr;
		getUnitPaserParameters(functionParser, mainExpr, functionNameExprArray, classNameExprArray);
		_parsers[index]->_parser = new FunctionUnitParser(id, displayName, commentExpr.c_str(), mainExpr.c_str(), functionNameExprArray, classNameExprArray);
	}

	return true;
}

bool FunctionParsersManager::getOverrideMapFromXmlTree(const wstring & xmlDirPath)
{
	wstring funcListRulePath = xmlDirPath;
	funcListRulePath += L"\\overrideMap.xml";
	
	TiXmlDocument xmlFuncListDoc(funcListRulePath);
	bool loadOK = xmlFuncListDoc.LoadFile();

	if (!loadOK)
		return false;
	
	TiXmlNode *root = xmlFuncListDoc.FirstChild(L"NotepadPlus");
	if (!root) 
		return false;

	root = root->FirstChild(L"functionList");
	if (!root) 
		return false;

	TiXmlNode *associationMapRoot = root->FirstChild(L"associationMap");
	if (associationMapRoot) 
	{
		for (TiXmlNode *childNode = associationMapRoot->FirstChildElement(L"association");
			childNode;
			childNode = childNode->NextSibling(L"association") )
		{
			int langID;
			const wchar_t *langIDStr = (childNode->ToElement())->Attribute(L"langID", &langID);
			const wchar_t *id = (childNode->ToElement())->Attribute(L"id");
			const wchar_t *userDefinedLangName = (childNode->ToElement())->Attribute(L"userDefinedLangName");

			if (!(id && id[0]))
				continue;

			if (langIDStr && langIDStr[0])
			{
				_parsers[langID] = new ParserInfo(id);
			}
			else if (userDefinedLangName && userDefinedLangName[0])
			{
				++_currentUDIndex;

				if (_currentUDIndex < L_EXTERNAL + nbMaxUserDefined)
				{
					_parsers[_currentUDIndex] = new ParserInfo(id, userDefinedLangName);	
				}
			}
		}
	}

	return true;
}

FunctionParser * FunctionParsersManager::getParser(const AssociationInfo & assoInfo)
{
	const unsigned char doNothing = 0;
	const unsigned char checkLangID = 1;
	const unsigned char checkUserDefined = 2;

	unsigned char choice = doNothing;
	// langID != -1 && langID != L_USER
	if (assoInfo._langID != -1 && assoInfo._langID != L_USER)
		choice = checkLangID;
	// langID == L_USER, we chack the userDefinedLangName
	else if (assoInfo._langID == L_USER && assoInfo._userDefinedLangName != L"")
		choice = checkUserDefined;
	else
		return NULL;

	switch (choice)
	{
		case checkLangID:
		{
			if (_parsers[assoInfo._langID] != nullptr)
			{
				if (_parsers[assoInfo._langID]->_parser != nullptr)
					return _parsers[assoInfo._langID]->_parser;
				else
				{
					// load it
					if (loadFuncListFromXmlTree(_xmlDirPath, static_cast<LangType>(assoInfo._langID), _parsers[assoInfo._langID]->_id))
						return _parsers[assoInfo._langID]->_parser;
					else if (_xmlDirPath != _xmlDirInstalledPath && !_xmlDirInstalledPath.empty() && loadFuncListFromXmlTree(_xmlDirInstalledPath, static_cast<LangType>(assoInfo._langID), _parsers[assoInfo._langID]->_id))
						return _parsers[assoInfo._langID]->_parser;
				}
			}
			else
			{
				_parsers[assoInfo._langID] = new ParserInfo;
				// load it
				if (loadFuncListFromXmlTree(_xmlDirPath, static_cast<LangType>(assoInfo._langID), _parsers[assoInfo._langID]->_id))
					return _parsers[assoInfo._langID]->_parser;
				else if (_xmlDirPath != _xmlDirInstalledPath && !_xmlDirInstalledPath.empty() && loadFuncListFromXmlTree(_xmlDirInstalledPath, static_cast<LangType>(assoInfo._langID), _parsers[assoInfo._langID]->_id))
					return _parsers[assoInfo._langID]->_parser;

				return nullptr;
			}
		}
		break;

		case checkUserDefined:
		{
			if (_currentUDIndex == L_EXTERNAL) // no User Defined Language parser
				return nullptr;

			for (int i = L_EXTERNAL + 1; i <= _currentUDIndex; ++i)
			{
				if (_parsers[i]->_userDefinedLangName == assoInfo._userDefinedLangName)
				{
					if (_parsers[i]->_parser)
					{
						return _parsers[i]->_parser;
					}
					else
					{
						// load it
						if (loadFuncListFromXmlTree(_xmlDirPath, static_cast<LangType>(assoInfo._langID), _parsers[i]->_id, i))
							return _parsers[i]->_parser;
						else if (_xmlDirPath != _xmlDirInstalledPath && !_xmlDirInstalledPath.empty() && loadFuncListFromXmlTree(_xmlDirInstalledPath, static_cast<LangType>(assoInfo._langID), _parsers[i]->_id, i))
							return _parsers[i]->_parser;
					}

					break;
				}
			}

			return nullptr;
		}
		break;

	}

	return nullptr;
}


void FunctionParser::funcParse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, wstring classStructName, const std::vector< std::pair<size_t, size_t> > * commentZones)
{
	if (begin >= end)
		return;

	if (_functionExpr.length() == 0)
		return;

	int flags = SCFIND_REGEXP | SCFIND_POSIX | SCFIND_REGEXP_DOTMATCHESNL;

	(*ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	intptr_t targetStart = (*ppEditView)->searchInTarget(_functionExpr.c_str(), _functionExpr.length(), begin, end);
	intptr_t targetEnd = 0;
	
	//foundInfos.clear();
	while (targetStart >= 0)
	{
		targetStart = (*ppEditView)->execute(SCI_GETTARGETSTART);
		targetEnd = (*ppEditView)->execute(SCI_GETTARGETEND);
		if (targetEnd > static_cast<intptr_t>(end)) //we found a result but outside our range, therefore do not process it
		{
			break;
		}
		size_t foundTextLen = targetEnd - targetStart;
		if (targetStart + foundTextLen == end)
            break;

		foundInfo fi;

		// dataToSearch & data2ToSearch are optional
		if (!_functionNameExprArray.size() && !_classNameExprArray.size())
		{
			wchar_t foundData[1024];
			(*ppEditView)->getGenericText(foundData, 1024, targetStart, targetEnd);

			fi._data = foundData; // whole found data
			fi._pos = targetStart;

		}
		else
		{
			intptr_t foundPos;
			if (_functionNameExprArray.size())
			{
				fi._data = parseSubLevel(targetStart, targetEnd, _functionNameExprArray, foundPos, ppEditView);
				fi._pos = foundPos;
			}

			if (!classStructName.empty())
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
				foundInfos.push_back(fi);
		}
		
		begin = targetStart + foundTextLen;
		targetStart = (*ppEditView)->searchInTarget(_functionExpr.c_str(), _functionExpr.length(), begin, end);
	}
}


wstring FunctionParser::parseSubLevel(size_t begin, size_t end, std::vector< wstring > dataToSearch, intptr_t & foundPos, ScintillaEditView **ppEditView)
{
	if (begin >= end)
	{
		foundPos = -1;
		return wstring();
	}

	if (!dataToSearch.size())
		return wstring();

	int flags = SCFIND_REGEXP | SCFIND_POSIX  | SCFIND_REGEXP_DOTMATCHESNL;

	(*ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	const wchar_t *regExpr2search = dataToSearch[0].c_str();
	intptr_t targetStart = (*ppEditView)->searchInTarget(regExpr2search, lstrlen(regExpr2search), begin, end);

	if (targetStart < 0)
	{
		foundPos = -1;
		return wstring();
	}
	intptr_t targetEnd = (*ppEditView)->execute(SCI_GETTARGETEND);

	if (dataToSearch.size() >= 2)
	{
		dataToSearch.erase(dataToSearch.begin());
		return parseSubLevel(targetStart, targetEnd, dataToSearch, foundPos, ppEditView);
	}

	// only one processed element, so we conclude the result
	wchar_t foundStr[1024];
	(*ppEditView)->getGenericText(foundStr, 1024, targetStart, targetEnd);

	foundPos = targetStart;
	return foundStr;
}


bool FunctionParsersManager::parse(std::vector<foundInfo> & foundInfos, const AssociationInfo & assoInfo)
{
	// Serch the right parser from the given ext in the map
	FunctionParser *fp = getParser(assoInfo);
	if (!fp)
		return false;

	// parse
	size_t docLen = (*_ppEditView)->getCurrentDocLen();
	fp->parse(foundInfos, 0, docLen, _ppEditView);

	return true;
}


size_t FunctionZoneParser::getBodyClosePos(size_t begin, const wchar_t *bodyOpenSymbol, const wchar_t *bodyCloseSymbol, const std::vector< std::pair<size_t, size_t> > & commentZones, ScintillaEditView **ppEditView)
{
	size_t cntOpen = 1;

	size_t docLen = (*ppEditView)->getCurrentDocLen();

	if (begin >= docLen)
		return docLen;

	wstring exprToSearch = L"(";
	exprToSearch += bodyOpenSymbol;
	exprToSearch += L"|";
	exprToSearch += bodyCloseSymbol;
	exprToSearch += L")";


	int flags = SCFIND_REGEXP | SCFIND_POSIX | SCFIND_REGEXP_DOTMATCHESNL;

	(*ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	intptr_t targetStart = (*ppEditView)->searchInTarget(exprToSearch.c_str(), exprToSearch.length(), begin, docLen);
	LRESULT targetEnd = 0;

	do
	{
		if (targetStart >= 0) // found open or close symbol
		{
			targetEnd = (*ppEditView)->execute(SCI_GETTARGETEND);

			// Treat it only if it's NOT in the comment zone
			if (!isInZones(targetStart, commentZones))
			{
				// Now we determine the symbol (open or close)
				intptr_t tmpStart = (*ppEditView)->searchInTarget(bodyOpenSymbol, lstrlen(bodyOpenSymbol), targetStart, targetEnd);
				if (tmpStart >= 0) // open symbol found 
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

void FunctionZoneParser::classParse(vector<foundInfo> & foundInfos, vector< pair<size_t, size_t> > &scannedZones, const std::vector< std::pair<size_t, size_t> > & commentZones, size_t begin, size_t end, ScintillaEditView **ppEditView, wstring /*classStructName*/)
{
	if (begin >= end)
		return;

	int flags = SCFIND_REGEXP | SCFIND_POSIX | SCFIND_REGEXP_DOTMATCHESNL;

	(*ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	intptr_t targetStart = (*ppEditView)->searchInTarget(_rangeExpr.c_str(), _rangeExpr.length(), begin, end);

	intptr_t targetEnd = 0;
	
	while (targetStart >= 0)
	{
		targetEnd = (*ppEditView)->execute(SCI_GETTARGETEND);

		// Get class name
		intptr_t foundPos = 0;
		wstring subLevelClassStructName = parseSubLevel(targetStart, targetEnd, _classNameExprArray, foundPos, ppEditView);
		

		if (!_openSymbole.empty() && !_closeSymbole.empty())
		{
			targetEnd = getBodyClosePos(targetEnd, _openSymbole.c_str(), _closeSymbole.c_str(), commentZones, ppEditView);
		}

		if (targetEnd > static_cast<intptr_t>(end)) //we found a result but outside our range, therefore do not process it
			break;
		
		scannedZones.push_back(pair<size_t, size_t>(targetStart, targetEnd));

		size_t foundTextLen = targetEnd - targetStart;
		if (targetStart + foundTextLen == end)
            break;

		// Begin to search all method inside
		//vector< wstring > emptyArray;
		if (!isInZones(targetStart, commentZones))
		{
			funcParse(foundInfos, targetStart, targetEnd, ppEditView, subLevelClassStructName, &commentZones);
		}
		begin = targetStart + (targetEnd - targetStart);
		targetStart = (*ppEditView)->searchInTarget(_rangeExpr.c_str(), _rangeExpr.length(), begin, end);
	}
}


void FunctionParser::getCommentZones(vector< pair<size_t, size_t> > & commentZone, size_t begin, size_t end, ScintillaEditView **ppEditView)
{
	if ((begin >= end) || (_commentExpr.empty()))
		return;

	int flags = SCFIND_REGEXP | SCFIND_POSIX | SCFIND_REGEXP_DOTMATCHESNL;

	(*ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	intptr_t targetStart = (*ppEditView)->searchInTarget(_commentExpr.c_str(), _commentExpr.length(), begin, end);
	intptr_t targetEnd = 0;
	
	while (targetStart >= 0)
	{
		targetStart = (*ppEditView)->execute(SCI_GETTARGETSTART);
		targetEnd = (*ppEditView)->execute(SCI_GETTARGETEND);
		if (targetEnd > static_cast<intptr_t>(end)) //we found a result but outside our range, therefore do not process it
			break;

		commentZone.push_back(pair<size_t, size_t>(targetStart, targetEnd));

		intptr_t foundTextLen = targetEnd - targetStart;
		if (targetStart + foundTextLen == static_cast<intptr_t>(end))
            break;
		
		begin = targetStart + foundTextLen;
		targetStart = (*ppEditView)->searchInTarget(_commentExpr.c_str(), _commentExpr.length(), begin, end);
	}
}


bool FunctionParser::isInZones(size_t pos2Test, const std::vector< std::pair<size_t, size_t>> & zones)
{
	for (size_t i = 0, len = zones.size(); i < len; ++i)
	{
		if (pos2Test >= zones[i].first && pos2Test < zones[i].second)
			return true;
	}
	return false;
}


void FunctionParser::getInvertZones(vector< pair<size_t, size_t> > &  destZones, const vector< pair<size_t, size_t> > &  sourceZones, size_t begin, size_t end)
{
	if (sourceZones.size() == 0)
	{
		destZones.push_back(pair<size_t, size_t>(begin, end));
	}
	else
	{
		// check the begin
		if (begin < sourceZones[0].first)
		{
			destZones.push_back(pair<size_t, size_t>(begin, sourceZones[0].first - 1));
		}

		size_t i = 0;
		for (size_t len = sourceZones.size() - 1; i < len; ++i)
		{
			size_t newBegin = sourceZones[i].second + 1;
			size_t newEnd = sourceZones[i+1].first - 1;
			if (newBegin < newEnd)
				destZones.push_back(pair<size_t, size_t>(newBegin, newEnd));
		}
		size_t lastBegin = sourceZones[i].second + 1;
		if (lastBegin < end)
			destZones.push_back(pair<size_t, size_t>(lastBegin, end));
	}
}


void FunctionZoneParser::parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, wstring classStructName)
{
	vector< pair<size_t, size_t> > classZones, commentZones, nonCommentZones;
	getCommentZones(commentZones, begin, end, ppEditView);
	getInvertZones(nonCommentZones, commentZones, begin, end);
	for (size_t i = 0, len = nonCommentZones.size(); i < len; ++i)
	{
		classParse(foundInfos, classZones, commentZones, nonCommentZones[i].first, nonCommentZones[i].second, ppEditView, classStructName);
	}
}

void FunctionUnitParser::parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, wstring classStructName)
{
	vector< pair<size_t, size_t> > commentZones, nonCommentZones;
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
struct SortZones final
{
	bool operator() (const pair<int, int> & l, const pair<int, int> & r)
	{
		return (l.first < r.first);
	}
};

void FunctionMixParser::parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, wstring classStructName)
{
	vector< pair<size_t, size_t> > commentZones, scannedZones, nonScannedZones;
	getCommentZones(commentZones, begin, end, ppEditView);

	classParse(foundInfos, scannedZones, commentZones, begin, end, ppEditView, classStructName);

	// the second level
	for (size_t i = 0, len = scannedZones.size(); i < len; ++i)
	{
		vector< pair<size_t, size_t> > temp;
		classParse(foundInfos, temp, commentZones, scannedZones[i].first, scannedZones[i].second, ppEditView, classStructName);
	}
	// invert scannedZones
	getInvertZones(nonScannedZones, scannedZones, begin, end);

	// for each nonScannedZones, search functions
	if (_funcUnitPaser)
	{
		for (size_t i = 0, len = nonScannedZones.size(); i < len; ++i)
		{
			_funcUnitPaser->funcParse(foundInfos, nonScannedZones[i].first, nonScannedZones[i].second, ppEditView, classStructName, &commentZones);
		}
	}
}
