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
	const NppXml::Element& classRangeParser,
	std::string& mainExprStr,
	std::string& openSymboleStr,
	std::string& closeSymboleStr,
	std::vector<std::string>& classNameExprArray,
	std::string& functionExprStr,
	std::vector<std::string>& functionNameExprArray
)
{
	const char* mainExpr = NppXml::attribute(classRangeParser, "mainExpr");
	if (!mainExpr || !mainExpr[0])
		return false;
	mainExprStr = mainExpr;

	const char* openSymbole = NppXml::attribute(classRangeParser, "openSymbole");
	if (openSymbole && openSymbole[0])
		openSymboleStr = openSymbole;

	const char* closeSymbole = NppXml::attribute(classRangeParser, "closeSymbole");
	if (closeSymbole && closeSymbole[0])
		closeSymboleStr = closeSymbole;

	NppXml::Element classNameParser = NppXml::firstChildElement(classRangeParser, "className");
	if (classNameParser)
	{
		for (NppXml::Element childNode2 = NppXml::firstChildElement(classNameParser, "nameExpr");
			childNode2;
			childNode2 = NppXml::nextSiblingElement(childNode2, "nameExpr"))
		{
			const char* expr = NppXml::attribute(childNode2, "expr");
			if (expr && expr[0])
				classNameExprArray.push_back(expr);
		}
	}

	NppXml::Element functionParser = NppXml::firstChildElement(classRangeParser, "function");
	if (!functionParser)
		return false;

	const char* functionExpr = NppXml::attribute(functionParser, "mainExpr");
	if (!functionExpr || !functionExpr[0])
		return false;
	functionExprStr = functionExpr;

	NppXml::Element functionNameParser = NppXml::firstChildElement(functionParser, "functionName");
	if (functionNameParser)
	{
		for (NppXml::Element childNode3 = NppXml::firstChildElement(functionNameParser, "funcNameExpr");
			childNode3;
			childNode3 = NppXml::nextSiblingElement(childNode3, "funcNameExpr"))
		{
			const char* expr = NppXml::attribute(childNode3, "expr");
			if (expr && expr[0])
				functionNameExprArray.push_back(expr);
		}
		
	}
	return true;
}

bool FunctionParsersManager::getUnitPaserParameters(
	const NppXml::Element& functionParser,
	std::string& mainExprStr,
	std::vector<std::string>& functionNameExprArray,
	std::vector<std::string>& classNameExprArray
)
{
	const char* mainExpr = NppXml::attribute(functionParser, "mainExpr");
	if (!mainExpr || !mainExpr[0])
		return false;
	mainExprStr = mainExpr;

	NppXml::Element functionNameParser = NppXml::firstChildElement(functionParser, "functionName");
	if (functionNameParser)
	{
		for (NppXml::Element childNode = NppXml::firstChildElement(functionNameParser, "nameExpr");
			childNode;
			childNode = NppXml::nextSiblingElement(childNode, "nameExpr"))
		{
			const char* expr = NppXml::attribute(childNode, "expr");
			if (expr && expr[0])
				functionNameExprArray.push_back(expr);
		}
	}

	NppXml::Element classNameParser = NppXml::firstChildElement(functionParser, "className");
	if (classNameParser)
	{
		for (NppXml::Element childNode = NppXml::firstChildElement(classNameParser, "nameExpr");
			childNode;
			childNode = NppXml::nextSiblingElement(childNode, "nameExpr"))
		{
			const char* expr = NppXml::attribute(childNode, "expr");
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

	NppXml::NewDocument xmlFuncListDoc{};
	const bool loadOK = NppXml::loadFileFunctionParser(&xmlFuncListDoc, funcListRulePath.c_str());

	if (!loadOK)
		return false;

	NppXml::Element root = NppXml::firstChildElement(&xmlFuncListDoc, "NotepadPlus");
	if (!root)
		return false;

	root = NppXml::firstChildElement(root, "functionList");
	if (!root)
		return false;

	NppXml::Element parserRoot = NppXml::firstChildElement(root, "parser");
	if (!parserRoot)
		return false;

	const char* id = NppXml::attribute(parserRoot, "id");
	if (!id || !id[0])
		return false;

	std::string commentExpr;
	const char* pCommentExpr = NppXml::attribute(parserRoot, "commentExpr");
	if (pCommentExpr && pCommentExpr[0])
		commentExpr = pCommentExpr;

	std::vector<std::string> classNameExprArray;
	std::vector<std::string> functionNameExprArray;

	const char* displayName = NppXml::attribute(parserRoot, "displayName");
	if (!displayName || !displayName[0])
		displayName = id;

	NppXml::Element classRangeParser = NppXml::firstChildElement(parserRoot, "classRange");
	NppXml::Element functionParser = NppXml::firstChildElement(parserRoot, "function");
	if (classRangeParser && functionParser)
	{
		std::string mainExpr, openSymbole, closeSymbole, functionExpr;
		getZonePaserParameters(classRangeParser, mainExpr, openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray);

		std::string mainExpr2;
		std::vector<std::string> classNameExprArray2;
		std::vector<std::string> functionNameExprArray2;
		getUnitPaserParameters(functionParser, mainExpr2, functionNameExprArray2, classNameExprArray2);
		auto funcUnitPaser = std::make_unique<FunctionUnitParser>(id, displayName, commentExpr, mainExpr2, functionNameExprArray2, classNameExprArray2);

		_parsers[index]->_parser = std::make_unique<FunctionMixParser>(id, displayName, commentExpr, mainExpr, openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray, std::move(funcUnitPaser));
	}
	else if (classRangeParser)
	{
		std::string mainExpr, openSymbole, closeSymbole, functionExpr;
		getZonePaserParameters(classRangeParser, mainExpr, openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray);
		_parsers[index]->_parser = std::make_unique<FunctionZoneParser>(id, displayName, commentExpr, mainExpr, openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray);
	}
	else if (functionParser)
	{
		std::string mainExpr;
		getUnitPaserParameters(functionParser, mainExpr, functionNameExprArray, classNameExprArray);
		_parsers[index]->_parser = std::make_unique<FunctionUnitParser>(id, displayName, commentExpr, mainExpr, functionNameExprArray, classNameExprArray);
	}

	return true;
}

bool FunctionParsersManager::getOverrideMapFromXmlTree(const std::wstring& xmlDirPath)
{
	std::wstring funcListRulePath = xmlDirPath;
	funcListRulePath += L"\\overrideMap.xml";
	
	NppXml::NewDocument xmlFuncListDoc{};
	const bool loadOK = NppXml::loadFile(&xmlFuncListDoc, funcListRulePath.c_str());

	if (!loadOK)
		return false;
	
	NppXml::Element root = NppXml::firstChildElement(&xmlFuncListDoc, "NotepadPlus");
	if (!root) 
		return false;

	root = NppXml::firstChildElement(root, "functionList");
	if (!root) 
		return false;

	NppXml::Element associationMapRoot = NppXml::firstChildElement(root, "associationMap");
	if (associationMapRoot) 
	{
		for (NppXml::Element childNode = NppXml::firstChildElement(associationMapRoot,"association");
			childNode;
			childNode = NppXml::nextSiblingElement(childNode, "association"))
		{
			const char* id = NppXml::attribute(childNode, "id");
			if (!id || !id[0])
				continue;

			const int langID = NppXml::intAttribute(childNode, "langID", -1);
			const char* userDefinedLangName = NppXml::attribute(childNode, "userDefinedLangName");
			if (langID >= 0)
			{
				_parsers[langID] = std::make_unique<ParserInfo>(string2wstring(id));
			}
			else if (userDefinedLangName && userDefinedLangName[0])
			{
				++_currentUDIndex;

				if (_currentUDIndex < L_EXTERNAL + nbMaxUserDefined)
				{
					_parsers[_currentUDIndex] = std::make_unique<ParserInfo>(string2wstring(id), string2wstring(userDefinedLangName));
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
	intptr_t targetStart = (*ppEditView)->searchInTarget(_functionExpr, begin, end);
	
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
			auto foundData = std::string(1024, '\0');
			(*ppEditView)->getGenericText(foundData.data(), foundData.length(), targetStart, targetEnd);

			fi._data = foundData; // whole found data
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
		targetStart = (*ppEditView)->searchInTarget(_functionExpr, begin, end);
	}
}


std::string FunctionParser::parseSubLevel(size_t begin, size_t end, std::vector<std::string> dataToSearch, intptr_t& foundPos, ScintillaEditView** ppEditView)
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
	intptr_t targetStart = (*ppEditView)->searchInTarget(dataToSearch[0], begin, end);

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
	auto foundStr = std::string(1024, '\0');
	(*ppEditView)->getGenericText(foundStr.data(), foundStr.length(), targetStart, targetEnd);

	foundPos = targetStart;
	return foundStr;
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
	const char* bodyOpenSymbol,
	const char* bodyCloseSymbol,
	const std::vector<std::pair<size_t, size_t>>& commentZones,
	ScintillaEditView** ppEditView
)
{
	size_t cntOpen = 1;

	size_t docLen = (*ppEditView)->getCurrentDocLen();

	if (begin >= docLen)
		return docLen;

	std::string exprToSearch = "(";
	exprToSearch += bodyOpenSymbol;
	exprToSearch += "|";
	exprToSearch += bodyCloseSymbol;
	exprToSearch += ")";


	static constexpr int flags = SCFIND_REGEXP | SCFIND_POSIX | SCFIND_REGEXP_DOTMATCHESNL;

	(*ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	intptr_t targetStart = (*ppEditView)->searchInTarget(exprToSearch, begin, docLen);
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
				const intptr_t tmpStart = (*ppEditView)->searchInTarget(bodyOpenSymbol, targetStart, targetEnd);
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

		targetStart = (*ppEditView)->searchInTarget(exprToSearch, targetEnd, docLen);

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
	intptr_t targetStart = (*ppEditView)->searchInTarget(_rangeExpr, begin, end);
	
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
		targetStart = (*ppEditView)->searchInTarget(_rangeExpr, begin, end);
	}
}


void FunctionParser::getCommentZones(std::vector<std::pair<size_t, size_t>>& commentZone, size_t begin, size_t end, ScintillaEditView** ppEditView)
{
	if ((begin >= end) || (_commentExpr.empty()))
		return;

	static constexpr int flags = SCFIND_REGEXP | SCFIND_POSIX | SCFIND_REGEXP_DOTMATCHESNL;

	(*ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	intptr_t targetStart = (*ppEditView)->searchInTarget(_commentExpr, begin, end);
	
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
		targetStart = (*ppEditView)->searchInTarget(_commentExpr, begin, end);
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
