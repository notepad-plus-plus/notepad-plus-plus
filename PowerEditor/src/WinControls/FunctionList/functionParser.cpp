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


#include "functionParser.h"

#include <windows.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <BoostRegexSearch.h>
#include <Scintilla.h>

#include "Common.h"
#include "Notepad_plus_msgs.h"
#include "NppXml.h"
#include "ScintillaEditView.h"

bool FunctionParsersManager::init(const std::wstring& xmlDirPath, const std::wstring& xmlInstalledPath, ScintillaEditView** ppEditView)
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

bool FunctionParsersManager::getZonePaserParameters(
	TiXmlElement* classRangeParser,
	std::wstring& mainExprStr,
	std::wstring& openSymboleStr,
	std::wstring& closeSymboleStr,
	std::vector<std::wstring>& classNameExprArray,
	std::wstring& functionExprStr,
	std::vector<std::wstring>& functionNameExprArray
)
{
	const wchar_t* mainExpr = classRangeParser->Attribute(L"mainExpr");
	if (!mainExpr || !mainExpr[0])
		return false;
	mainExprStr = mainExpr;

	const wchar_t* openSymbole = classRangeParser->Attribute(L"openSymbole");
	if (openSymbole && openSymbole[0])
		openSymboleStr = openSymbole;

	const wchar_t* closeSymbole = classRangeParser->Attribute(L"closeSymbole");
	if (closeSymbole && closeSymbole[0])
		closeSymboleStr = closeSymbole;

	TiXmlElement* classNameParser = classRangeParser->FirstChildElement(L"className");
	if (classNameParser)
	{
		for (TiXmlElement* childNode2 = classNameParser->FirstChildElement(L"nameExpr");
			childNode2;
			childNode2 = childNode2->NextSiblingElement(L"nameExpr"))
		{
			const wchar_t* expr = childNode2->Attribute(L"expr");
			if (expr && expr[0])
				classNameExprArray.push_back(expr);
		}
	}

	TiXmlElement* functionParser = classRangeParser->FirstChildElement(L"function");
	if (!functionParser)
		return false;

	const wchar_t* functionExpr = functionParser->Attribute(L"mainExpr");
	if (!functionExpr || !functionExpr[0])
		return false;
	functionExprStr = functionExpr;

	TiXmlElement* functionNameParser = functionParser->FirstChildElement(L"functionName");
	if (functionNameParser)
	{
		for (TiXmlElement* childNode3 = functionNameParser->FirstChildElement(L"funcNameExpr");
			childNode3;
			childNode3 = childNode3->NextSiblingElement(L"funcNameExpr"))
		{
			const wchar_t* expr = childNode3->Attribute(L"expr");
			if (expr && expr[0])
				functionNameExprArray.push_back(expr);
		}
		
	}
	return true;
}

bool FunctionParsersManager::getUnitPaserParameters(
	TiXmlElement* functionParser,
	std::wstring& mainExprStr,
	std::vector<std::wstring>& functionNameExprArray,
	std::vector<std::wstring>& classNameExprArray
)
{
	const wchar_t* mainExpr = functionParser->Attribute(L"mainExpr");
	if (!mainExpr || !mainExpr[0])
		return false;
	mainExprStr = mainExpr;

	TiXmlElement* functionNameParser = functionParser->FirstChildElement(L"functionName");
	if (functionNameParser)
	{
		for (TiXmlElement* childNode = functionNameParser->FirstChildElement(L"nameExpr");
			childNode;
			childNode = childNode->NextSiblingElement(L"nameExpr"))
		{
			const wchar_t* expr = childNode->Attribute(L"expr");
			if (expr && expr[0])
				functionNameExprArray.push_back(expr);
		}
	}

	TiXmlElement* classNameParser = functionParser->FirstChildElement(L"className");
	if (classNameParser)
	{
		for (TiXmlElement* childNode = classNameParser->FirstChildElement(L"nameExpr");
			childNode;
			childNode = childNode->NextSiblingElement(L"nameExpr"))
		{
			const wchar_t* expr = childNode->Attribute(L"expr");
			if (expr && expr[0])
				classNameExprArray.push_back(expr);
		}
	}
	return true;
}

bool FunctionParsersManager::loadFuncListFromXmlTree(const std::wstring& xmlDirPath, LangType lType, const std::wstring& overrideId, int udlIndex)
{
	std::wstring funcListRulePath = xmlDirPath;
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
			std::wstring lexerName = ScintillaEditView::_langNameInfoArray[lType]._langName;
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

	TiXmlDocument xmlFuncListDoc;
	bool loadOK = xmlFuncListDoc.LoadFile(funcListRulePath);

	if (!loadOK)
		return false;

	TiXmlElement* root = xmlFuncListDoc.FirstChildElement(L"NotepadPlus");
	if (!root)
		return false;

	root = root->FirstChildElement(L"functionList");
	if (!root)
		return false;

	TiXmlElement* parserRoot = root->FirstChildElement(L"parser");
	if (!parserRoot)
		return false;

	const wchar_t* id = parserRoot->Attribute(L"id");
	if (!id || !id[0])
		return false;

	std::wstring commentExpr;
	const wchar_t* pCommentExpr = parserRoot->Attribute(L"commentExpr");
	if (pCommentExpr && pCommentExpr[0])
		commentExpr = pCommentExpr;

	std::vector<std::wstring> classNameExprArray;
	std::vector<std::wstring> functionNameExprArray;

	const wchar_t* displayName = parserRoot->Attribute(L"displayName");
	if (!displayName || !displayName[0])
		displayName = id;

	TiXmlElement* classRangeParser = parserRoot->FirstChildElement(L"classRange");
	TiXmlElement* functionParser = parserRoot->FirstChildElement(L"function");
	if (classRangeParser && functionParser)
	{
		std::wstring mainExpr, openSymbole, closeSymbole, functionExpr;
		getZonePaserParameters(classRangeParser, mainExpr, openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray);

		std::wstring mainExpr2;
		std::vector<std::wstring> classNameExprArray2;
		std::vector<std::wstring> functionNameExprArray2;
		getUnitPaserParameters(functionParser, mainExpr2, functionNameExprArray2, classNameExprArray2);
		auto funcUnitPaser = std::make_unique<FunctionUnitParser>(id, displayName, commentExpr, mainExpr2, functionNameExprArray2, classNameExprArray2);

		_parsers[index]->_parser = std::make_unique<FunctionMixParser>(id, displayName, commentExpr, mainExpr, openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray, std::move(funcUnitPaser));
	}
	else if (classRangeParser)
	{
		std::wstring mainExpr, openSymbole, closeSymbole, functionExpr;
		getZonePaserParameters(classRangeParser, mainExpr, openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray);
		_parsers[index]->_parser = std::make_unique<FunctionZoneParser>(id, displayName, commentExpr, mainExpr, openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray);
	}
	else if (functionParser)
	{
		std::wstring mainExpr;
		getUnitPaserParameters(functionParser, mainExpr, functionNameExprArray, classNameExprArray);
		_parsers[index]->_parser = std::make_unique<FunctionUnitParser>(id, displayName, commentExpr, mainExpr, functionNameExprArray, classNameExprArray);
	}

	return true;
}

bool FunctionParsersManager::getOverrideMapFromXmlTree(const std::wstring& xmlDirPath)
{
	std::wstring funcListRulePath = xmlDirPath;
	funcListRulePath += L"\\overrideMap.xml";
	
	TiXmlDocument xmlFuncListDoc;
	bool loadOK = xmlFuncListDoc.LoadFile(funcListRulePath);

	if (!loadOK)
		return false;
	
	TiXmlElement* root = xmlFuncListDoc.FirstChildElement(L"NotepadPlus");
	if (!root) 
		return false;

	root = root->FirstChildElement(L"functionList");
	if (!root) 
		return false;

	TiXmlElement* associationMapRoot = root->FirstChildElement(L"associationMap");
	if (associationMapRoot) 
	{
		for (TiXmlElement* childNode = associationMapRoot->FirstChildElement(L"association");
			childNode;
			childNode = childNode->NextSiblingElement(L"association"))
		{
			int langID = -1;
			const wchar_t* langIDStr = childNode->Attribute(L"langID", &langID);
			const wchar_t* id = childNode->Attribute(L"id");
			const wchar_t* userDefinedLangName = childNode->Attribute(L"userDefinedLangName");

			if (!(id && id[0]))
				continue;

			if (langIDStr && langIDStr[0])
			{
				_parsers[langID] = std::make_unique<ParserInfo>(id);
			}
			else if (userDefinedLangName && userDefinedLangName[0])
			{
				++_currentUDIndex;

				if (_currentUDIndex < L_EXTERNAL + nbMaxUserDefined)
				{
					_parsers[_currentUDIndex] = std::make_unique<ParserInfo>(id, userDefinedLangName);
				}
			}
		}
	}

	return true;
}

FunctionParser* FunctionParsersManager::getParser(const AssociationInfo& assoInfo)
{
	enum ParserType : unsigned char
	{
		doNothing,
		checkLangID,
		checkUserDefined
	};

	unsigned char choice = doNothing;
	// langID != -1 && langID != L_USER
	if (assoInfo._langID != -1 && assoInfo._langID != L_USER)
		choice = checkLangID;
	// langID == L_USER, we chack the userDefinedLangName
	else if (assoInfo._langID == L_USER && assoInfo._userDefinedLangName != L"")
		choice = checkUserDefined;
	else
		return nullptr;

	switch (choice)
	{
		case checkLangID:
		{
			if (_parsers[assoInfo._langID] != nullptr)
			{
				if (_parsers[assoInfo._langID]->_parser != nullptr)
					return _parsers[assoInfo._langID]->_parser.get();

				// load it
				if (loadFuncListFromXmlTree(_xmlDirPath, static_cast<LangType>(assoInfo._langID), _parsers[assoInfo._langID]->_id))
					return _parsers[assoInfo._langID]->_parser.get();
				else if (_xmlDirPath != _xmlDirInstalledPath && !_xmlDirInstalledPath.empty() && loadFuncListFromXmlTree(_xmlDirInstalledPath, static_cast<LangType>(assoInfo._langID), _parsers[assoInfo._langID]->_id))
					return _parsers[assoInfo._langID]->_parser.get();
			}
			else
			{
				_parsers[assoInfo._langID] = std::make_unique<ParserInfo>();
				// load it
				if (loadFuncListFromXmlTree(_xmlDirPath, static_cast<LangType>(assoInfo._langID), _parsers[assoInfo._langID]->_id))
					return _parsers[assoInfo._langID]->_parser.get();
				else if (_xmlDirPath != _xmlDirInstalledPath && !_xmlDirInstalledPath.empty() && loadFuncListFromXmlTree(_xmlDirInstalledPath, static_cast<LangType>(assoInfo._langID), _parsers[assoInfo._langID]->_id))
					return _parsers[assoInfo._langID]->_parser.get();

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
						return _parsers[i]->_parser.get();
					}

					// load it
					if (loadFuncListFromXmlTree(_xmlDirPath, static_cast<LangType>(assoInfo._langID), _parsers[i]->_id, i))
						return _parsers[i]->_parser.get();
					if (_xmlDirPath != _xmlDirInstalledPath && !_xmlDirInstalledPath.empty() && loadFuncListFromXmlTree(_xmlDirInstalledPath, static_cast<LangType>(assoInfo._langID), _parsers[i]->_id, i))
						return _parsers[i]->_parser.get();
					break;
				}
			}
			return nullptr;
		}
	}
	return nullptr;
}

void FunctionParser::funcParse(
	std::vector<foundInfo>& foundInfos,
	size_t begin,
	size_t end,
	ScintillaEditView** ppEditView,
	const std::string& classStructName,
	const std::vector<std::pair<size_t, size_t>>* commentZones
)
{
	if (begin >= end)
		return;

	if (_functionExpr.length() == 0)
		return;

	static constexpr int flags = SCFIND_REGEXP | SCFIND_POSIX | SCFIND_REGEXP_DOTMATCHESNL;

	(*ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	intptr_t targetStart = (*ppEditView)->searchInTarget(_functionExpr.c_str(), _functionExpr.length(), begin, end);
	
	//foundInfos.clear();
	while (targetStart >= 0)
	{
		targetStart = (*ppEditView)->execute(SCI_GETTARGETSTART);
		intptr_t targetEnd = (*ppEditView)->execute(SCI_GETTARGETEND);
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
			wchar_t foundData[1024]{};
			(*ppEditView)->getGenericText(foundData, 1024, targetStart, targetEnd);

			fi._data = wstring2string(foundData); // whole found data
			fi._pos = targetStart;

		}
		else
		{
			intptr_t foundPos = -1;
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
			if (commentZones != nullptr)
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


std::string FunctionParser::parseSubLevel(size_t begin, size_t end, std::vector<std::wstring> dataToSearch, intptr_t& foundPos, ScintillaEditView** ppEditView)
{
	if (begin >= end)
	{
		foundPos = -1;
		return "";
	}

	if (!dataToSearch.size())
		return "";

	static constexpr int flags = SCFIND_REGEXP | SCFIND_POSIX  | SCFIND_REGEXP_DOTMATCHESNL;

	(*ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	intptr_t targetStart = (*ppEditView)->searchInTarget(dataToSearch[0].c_str(), dataToSearch[0].length(), begin, end);

	if (targetStart < 0)
	{
		foundPos = -1;
		return "";
	}
	intptr_t targetEnd = (*ppEditView)->execute(SCI_GETTARGETEND);

	if (dataToSearch.size() >= 2)
	{
		dataToSearch.erase(dataToSearch.begin());
		return parseSubLevel(targetStart, targetEnd, dataToSearch, foundPos, ppEditView);
	}

	// only one processed element, so we conclude the result
	wchar_t foundStr[1024]{};
	(*ppEditView)->getGenericText(foundStr, 1024, targetStart, targetEnd);

	foundPos = targetStart;
	return wstring2string(foundStr);
}


bool FunctionParsersManager::parse(std::vector<foundInfo>& foundInfos, const AssociationInfo& assoInfo)
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


size_t FunctionZoneParser::getBodyClosePos(
	size_t begin,
	const wchar_t* bodyOpenSymbol,
	const wchar_t* bodyCloseSymbol,
	const std::vector<std::pair<size_t, size_t>>& commentZones,
	ScintillaEditView** ppEditView
)
{
	size_t cntOpen = 1;

	size_t docLen = (*ppEditView)->getCurrentDocLen();

	if (begin >= docLen)
		return docLen;

	std::wstring exprToSearch = L"(";
	exprToSearch += bodyOpenSymbol;
	exprToSearch += L"|";
	exprToSearch += bodyCloseSymbol;
	exprToSearch += L")";


	static constexpr int flags = SCFIND_REGEXP | SCFIND_POSIX | SCFIND_REGEXP_DOTMATCHESNL;

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
				const intptr_t tmpStart = (*ppEditView)->searchInTarget(bodyOpenSymbol, lstrlen(bodyOpenSymbol), targetStart, targetEnd);
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

void FunctionZoneParser::classParse(
	std::vector<foundInfo>& foundInfos,
	std::vector<std::pair<size_t, size_t>>& scannedZones,
	const std::vector<std::pair<size_t, size_t>>& commentZones,
	size_t begin, size_t end, ScintillaEditView** ppEditView,
	const std::string& /*classStructName*/
)
{
	if (begin >= end)
		return;

	static constexpr int flags = SCFIND_REGEXP | SCFIND_POSIX | SCFIND_REGEXP_DOTMATCHESNL;

	(*ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	intptr_t targetStart = (*ppEditView)->searchInTarget(_rangeExpr.c_str(), _rangeExpr.length(), begin, end);
	
	while (targetStart >= 0)
	{
		intptr_t targetEnd = (*ppEditView)->execute(SCI_GETTARGETEND);

		// Get class name
		intptr_t foundPos = 0;
		std::string subLevelClassStructName = parseSubLevel(targetStart, targetEnd, _classNameExprArray, foundPos, ppEditView);
		

		if (!_openSymbole.empty() && !_closeSymbole.empty())
		{
			targetEnd = getBodyClosePos(targetEnd, _openSymbole.c_str(), _closeSymbole.c_str(), commentZones, ppEditView);
		}

		if (targetEnd > static_cast<intptr_t>(end)) //we found a result but outside our range, therefore do not process it
			break;
		
		scannedZones.push_back(std::pair<size_t, size_t>(targetStart, targetEnd));

		size_t foundTextLen = targetEnd - targetStart;
		if (targetStart + foundTextLen == end)
			break;

		// Begin to search all method inside
		// std::vector<std::wstring> emptyArray;
		if (!isInZones(targetStart, commentZones))
		{
			funcParse(foundInfos, targetStart, targetEnd, ppEditView, subLevelClassStructName, &commentZones);
		}
		begin = targetStart + (targetEnd - targetStart);
		targetStart = (*ppEditView)->searchInTarget(_rangeExpr.c_str(), _rangeExpr.length(), begin, end);
	}
}


void FunctionParser::getCommentZones(std::vector<std::pair<size_t, size_t>>& commentZone, size_t begin, size_t end, ScintillaEditView** ppEditView)
{
	if ((begin >= end) || (_commentExpr.empty()))
		return;

	static constexpr int flags = SCFIND_REGEXP | SCFIND_POSIX | SCFIND_REGEXP_DOTMATCHESNL;

	(*ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	intptr_t targetStart = (*ppEditView)->searchInTarget(_commentExpr.c_str(), _commentExpr.length(), begin, end);
	
	while (targetStart >= 0)
	{
		targetStart = (*ppEditView)->execute(SCI_GETTARGETSTART);
		intptr_t targetEnd = (*ppEditView)->execute(SCI_GETTARGETEND);
		if (targetEnd > static_cast<intptr_t>(end)) //we found a result but outside our range, therefore do not process it
			break;

		commentZone.push_back(std::pair<size_t, size_t>(targetStart, targetEnd));

		intptr_t foundTextLen = targetEnd - targetStart;
		if (targetStart + foundTextLen == static_cast<intptr_t>(end))
			break;
		
		begin = targetStart + foundTextLen;
		targetStart = (*ppEditView)->searchInTarget(_commentExpr.c_str(), _commentExpr.length(), begin, end);
	}
}


bool FunctionParser::isInZones(size_t pos2Test, const std::vector<std::pair<size_t, size_t>>& zones)
{
	for (size_t i = 0, len = zones.size(); i < len; ++i)
	{
		if (pos2Test >= zones[i].first && pos2Test < zones[i].second)
			return true;
	}
	return false;
}


void FunctionParser::getInvertZones(std::vector<std::pair<size_t, size_t>>& destZones, const std::vector<std::pair<size_t, size_t>>& sourceZones, size_t begin, size_t end)
{
	if (sourceZones.size() == 0)
	{
		destZones.push_back(std::pair<size_t, size_t>(begin, end));
	}
	else
	{
		// check the begin
		if (begin < sourceZones[0].first)
		{
			destZones.push_back(std::pair<size_t, size_t>(begin, sourceZones[0].first - 1));
		}

		size_t i = 0;
		for (size_t len = sourceZones.size() - 1; i < len; ++i)
		{
			size_t newBegin = sourceZones[i].second + 1;
			size_t newEnd = sourceZones[i + 1].first - 1;
			if (newBegin < newEnd)
				destZones.push_back(std::pair<size_t, size_t>(newBegin, newEnd));
		}
		size_t lastBegin = sourceZones[i].second + 1;
		if (lastBegin < end)
			destZones.push_back(std::pair<size_t, size_t>(lastBegin, end));
	}
}


void FunctionZoneParser::parse(std::vector<foundInfo>& foundInfos, size_t begin, size_t end, ScintillaEditView** ppEditView, const std::string& classStructName)
{
	std::vector<std::pair<size_t, size_t>> classZones, commentZones, nonCommentZones;
	getCommentZones(commentZones, begin, end, ppEditView);
	getInvertZones(nonCommentZones, commentZones, begin, end);
	for (size_t i = 0, len = nonCommentZones.size(); i < len; ++i)
	{
		classParse(foundInfos, classZones, commentZones, nonCommentZones[i].first, nonCommentZones[i].second, ppEditView, classStructName);
	}
}

void FunctionUnitParser::parse(std::vector<foundInfo>& foundInfos, size_t begin, size_t end, ScintillaEditView** ppEditView, const std::string& classStructName)
{
	std::vector<std::pair<size_t, size_t>> commentZones, nonCommentZones;
	getCommentZones(commentZones, begin, end, ppEditView);
	getInvertZones(nonCommentZones, commentZones, begin, end);
	for (size_t i = 0, len = nonCommentZones.size(); i < len; ++i)
	{
		funcParse(foundInfos, nonCommentZones[i].first, nonCommentZones[i].second, ppEditView, classStructName);
	}
}

void FunctionMixParser::parse(std::vector<foundInfo>& foundInfos, size_t begin, size_t end, ScintillaEditView** ppEditView, const std::string& classStructName)
{
	std::vector<std::pair<size_t, size_t>> commentZones, scannedZones, nonScannedZones;
	getCommentZones(commentZones, begin, end, ppEditView);

	classParse(foundInfos, scannedZones, commentZones, begin, end, ppEditView, classStructName);

	// the second level
	for (size_t i = 0, len = scannedZones.size(); i < len; ++i)
	{
		std::vector<std::pair<size_t, size_t>> temp;
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
