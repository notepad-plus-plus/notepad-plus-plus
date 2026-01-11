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


#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Notepad_plus_msgs.h"
#include "NppXml.h"

class ScintillaEditView;

struct foundInfo final
{
	std::string _data;
	std::string _data2;
	intptr_t _pos = -1;
	intptr_t _pos2 = -1;
};

class FunctionParser
{
friend class FunctionParsersManager;
public:
	FunctionParser(const char* id, const char* displayName, const std::string& commentExpr, const std::string& functionExpr, const std::vector<std::string>& functionNameExprArray, const std::vector<std::string>& classNameExprArray) noexcept
		: _id(id), _displayName(displayName), _commentExpr(commentExpr), _functionExpr(functionExpr), _functionNameExprArray(functionNameExprArray), _classNameExprArray(classNameExprArray)
	{}

	virtual void parse(std::vector<foundInfo>& foundInfos, size_t begin, size_t end, ScintillaEditView** ppEditView, const std::string& classStructName = "") = 0;
	void funcParse(std::vector<foundInfo>& foundInfos, size_t begin, size_t end, ScintillaEditView** ppEditView, const std::string& classStructName = "", const std::vector<std::pair<size_t, size_t>>* commentZones = nullptr);
	static bool isInZones(size_t pos2Test, const std::vector<std::pair<size_t, size_t>>& zones);
	virtual ~FunctionParser() = default;

protected:
	std::string _id;
	std::string _displayName;
	std::string _commentExpr;
	std::string _functionExpr;
	std::vector<std::string> _functionNameExprArray;
	std::vector<std::string> _classNameExprArray;
	void getCommentZones(std::vector<std::pair<size_t, size_t>>& commentZone, size_t begin, size_t end, ScintillaEditView** ppEditView);
	void getInvertZones(std::vector<std::pair<size_t, size_t>>& destZones, const std::vector<std::pair<size_t, size_t>>& sourceZones, size_t begin, size_t end);
	static std::string parseSubLevel(size_t begin, size_t end, std::vector<std::string> dataToSearch, intptr_t& foundPos, ScintillaEditView** ppEditView);
};


class FunctionZoneParser : public FunctionParser
{
public:
	FunctionZoneParser() = delete;
	FunctionZoneParser(const char* id, const char* displayName, const std::string& commentExpr, const std::string& rangeExpr, const std::string& openSymbole, const std::string& closeSymbole,
		const std::vector<std::string>& classNameExprArray, const std::string& functionExpr, const std::vector<std::string>& functionNameExprArray) noexcept
		: FunctionParser(id, displayName, commentExpr, functionExpr, functionNameExprArray, classNameExprArray), _rangeExpr(rangeExpr), _openSymbole(openSymbole), _closeSymbole(closeSymbole)
	{}

	void parse(std::vector<foundInfo>& foundInfos, size_t begin, size_t end, ScintillaEditView** ppEditView, const std::string& classStructName = "") override;

protected:
	void classParse(std::vector<foundInfo>& foundInfos, std::vector<std::pair<size_t, size_t>>& scannedZones, const std::vector<std::pair<size_t, size_t>>& commentZones, size_t begin, size_t end, ScintillaEditView** ppEditView, const std::string& classStructName = "");

private:
	std::string _rangeExpr;
	std::string _openSymbole;
	std::string _closeSymbole;

	static size_t getBodyClosePos(size_t begin, const char* bodyOpenSymbol, const char* bodyCloseSymbol, const std::vector<std::pair<size_t, size_t>>& commentZones, ScintillaEditView** ppEditView);
};


class FunctionUnitParser : public FunctionParser
{
public:
	FunctionUnitParser(const char* id, const char* displayName, const std::string& commentExpr,
		const std::string& mainExpr, const std::vector<std::string>& functionNameExprArray,
		const std::vector<std::string>& classNameExprArray) noexcept
		: FunctionParser(id, displayName, commentExpr, mainExpr, functionNameExprArray, classNameExprArray)
	{}

	void parse(std::vector<foundInfo>& foundInfos, size_t begin, size_t end, ScintillaEditView** ppEditView, const std::string& classStructName = "") override;
};

class FunctionMixParser : public FunctionZoneParser
{
public:
	FunctionMixParser(const char* id, const char* displayName, const std::string& commentExpr, const std::string& rangeExpr, const std::string& openSymbole, const std::string& closeSymbole,
		const std::vector<std::string>& classNameExprArray, const std::string& functionExpr, const std::vector<std::string>& functionNameExprArray, std::unique_ptr<FunctionUnitParser> funcUnitPaser) noexcept
		: FunctionZoneParser(id, displayName, commentExpr, rangeExpr, openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray), _funcUnitPaser(std::move(funcUnitPaser))
	{}

	void parse(std::vector<foundInfo>& foundInfos, size_t begin, size_t end, ScintillaEditView** ppEditView, const std::string& classStructName = "") override;

private:
	std::unique_ptr<FunctionUnitParser> _funcUnitPaser = nullptr;
};


struct AssociationInfo final
{
	int _id;
	int _langID;
	std::wstring _ext;
	std::wstring _userDefinedLangName;

	AssociationInfo(int id, int langID, const wchar_t* ext, const wchar_t* userDefinedLangName) noexcept
		: _id(id), _langID(langID)
	{
		if (ext)
			_ext = ext;
		else
			_ext.clear();

		if (userDefinedLangName)
			_userDefinedLangName = userDefinedLangName;
		else
			_userDefinedLangName.clear();
	}
};

inline constexpr int nbMaxUserDefined = 25;

struct ParserInfo
{
	std::wstring _id; // xml parser rule file name - if empty, then we use default name. Mandatory if _userDefinedLangName is not empty
	std::unique_ptr<FunctionParser> _parser = nullptr;
	std::wstring _userDefinedLangName;

	ParserInfo() {}
	explicit ParserInfo(const std::wstring& id) noexcept : _id(id) {}
	explicit ParserInfo(const std::wstring& id, const std::wstring& userDefinedLangName) noexcept : _id(id), _userDefinedLangName(userDefinedLangName) {}
};

class FunctionParsersManager final
{
public:
	bool init(const std::wstring& xmlDirPath, const std::wstring& xmlInstalledPath, ScintillaEditView** ppEditView);
	bool parse(std::vector<foundInfo>& foundInfos, const AssociationInfo& assoInfo);
	

private:
	ScintillaEditView** _ppEditView = nullptr;
	std::wstring _xmlDirPath; // The 1st place to load function list files. Usually it's "%APPDATA%\Notepad++\functionList\"
	std::wstring _xmlDirInstalledPath; // Where Notepad++ is installed. The 2nd place to load function list files. Usually it's "%PROGRAMFILES%\Notepad++\functionList\" 

	std::unique_ptr<ParserInfo> _parsers[L_EXTERNAL + nbMaxUserDefined] = { nullptr };
	int _currentUDIndex = L_EXTERNAL;

	bool getOverrideMapFromXmlTree(const std::wstring& xmlDirPath);
	bool loadFuncListFromXmlTree(const std::wstring& xmlDirPath, LangType lType, const std::wstring& overrideId, int udlIndex = -1);
	static bool getZonePaserParameters(const NppXml::Element& classRangeParser, std::string& mainExprStr, std::string& openSymboleStr, std::string& closeSymboleStr, std::vector<std::string>& classNameExprArray, std::string& functionExprStr, std::vector<std::string>& functionNameExprArray);
	static bool getUnitPaserParameters(const NppXml::Element& functionParser, std::string& mainExprStr, std::vector<std::string>& functionNameExprArray, std::vector<std::string>& classNameExprArray);
	FunctionParser* getParser(const AssociationInfo& assoInfo);
};
