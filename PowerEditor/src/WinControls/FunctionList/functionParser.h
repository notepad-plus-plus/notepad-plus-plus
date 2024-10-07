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

class ScintillaEditView;
class TiXmlDocument;
class TiXmlNode;

struct foundInfo final
{
	std::wstring _data;
	std::wstring _data2;
	intptr_t _pos = -1;
	intptr_t _pos2 = -1;
};

class FunctionParser
{
friend class FunctionParsersManager;
public:
	FunctionParser(const wchar_t *id, const wchar_t *displayName, const wchar_t *commentExpr, const std::wstring& functionExpr, const std::vector<std::wstring>& functionNameExprArray, const std::vector<std::wstring>& classNameExprArray):
	  _id(id), _displayName(displayName), _commentExpr(commentExpr?commentExpr:L""), _functionExpr(functionExpr), _functionNameExprArray(functionNameExprArray), _classNameExprArray(classNameExprArray){};

	virtual void parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, std::wstring classStructName = L"") = 0;
	void funcParse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, std::wstring classStructName = L"", const std::vector< std::pair<size_t, size_t> > * commentZones = NULL);
	bool isInZones(size_t pos2Test, const std::vector< std::pair<size_t, size_t> > & zones);
	virtual ~FunctionParser() = default;

protected:
	std::wstring _id;
	std::wstring _displayName;
	std::wstring _commentExpr;
	std::wstring _functionExpr;
	std::vector<std::wstring> _functionNameExprArray;
	std::vector<std::wstring> _classNameExprArray;
	void getCommentZones(std::vector< std::pair<size_t, size_t> > & commentZone, size_t begin, size_t end, ScintillaEditView **ppEditView);
	void getInvertZones(std::vector< std::pair<size_t, size_t> > & destZones, const std::vector< std::pair<size_t, size_t> > & sourceZones, size_t begin, size_t end);
	std::wstring parseSubLevel(size_t begin, size_t end, std::vector< std::wstring > dataToSearch, intptr_t & foundPos, ScintillaEditView **ppEditView);
};


class FunctionZoneParser : public FunctionParser
{
public:
	FunctionZoneParser() = delete;
	FunctionZoneParser(const wchar_t *id, const wchar_t *displayName, const wchar_t *commentExpr, const std::wstring& rangeExpr, const std::wstring& openSymbole, const std::wstring& closeSymbole,
		const std::vector<std::wstring>& classNameExprArray, const std::wstring& functionExpr, const std::vector<std::wstring>& functionNameExprArray):
		FunctionParser(id, displayName, commentExpr, functionExpr, functionNameExprArray, classNameExprArray), _rangeExpr(rangeExpr), _openSymbole(openSymbole), _closeSymbole(closeSymbole) {};

	void parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, std::wstring classStructName = L"") override;
	
protected:
	void classParse(std::vector<foundInfo> & foundInfos, std::vector< std::pair<size_t, size_t> > & scannedZones, const std::vector< std::pair<size_t, size_t> > & commentZones, size_t begin, size_t end, ScintillaEditView **ppEditView, std::wstring classStructName = L"");

private:
	std::wstring _rangeExpr;
	std::wstring _openSymbole;
	std::wstring _closeSymbole;

	size_t getBodyClosePos(size_t begin, const wchar_t *bodyOpenSymbol, const wchar_t *bodyCloseSymbol, const std::vector< std::pair<size_t, size_t> > & commentZones, ScintillaEditView **ppEditView);
};



class FunctionUnitParser : public FunctionParser
{
public:
	FunctionUnitParser(const wchar_t *id, const wchar_t *displayName, const wchar_t *commentExpr,
		const std::wstring& mainExpr, const std::vector<std::wstring>& functionNameExprArray,
		const std::vector<std::wstring>& classNameExprArray): FunctionParser(id, displayName, commentExpr, mainExpr, functionNameExprArray, classNameExprArray)
	{}

	void parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, std::wstring classStructName = L"") override;
};

class FunctionMixParser : public FunctionZoneParser
{
public:
	FunctionMixParser(const wchar_t *id, const wchar_t *displayName, const wchar_t *commentExpr, const std::wstring& rangeExpr, const std::wstring& openSymbole, const std::wstring& closeSymbole,
		const std::vector<std::wstring>& classNameExprArray, const std::wstring& functionExpr, const std::vector<std::wstring>& functionNameExprArray, FunctionUnitParser *funcUnitPaser):
		FunctionZoneParser(id, displayName, commentExpr, rangeExpr,	openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray), _funcUnitPaser(funcUnitPaser){};
		
	~FunctionMixParser()
	{
		delete _funcUnitPaser;
	}

	void parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, std::wstring classStructName = L"") override;

private:
	FunctionUnitParser* _funcUnitPaser = nullptr;
};


struct AssociationInfo final
{
	int _id;
	int _langID;
	std::wstring _ext;
	std::wstring _userDefinedLangName;

	AssociationInfo(int id, int langID, const wchar_t *ext, const wchar_t *userDefinedLangName)
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
	};
};

const int nbMaxUserDefined = 25;

struct ParserInfo
{
	std::wstring _id; // xml parser rule file name - if empty, then we use default name. Mandatory if _userDefinedLangName is not empty
	FunctionParser* _parser = nullptr;
	std::wstring _userDefinedLangName;

	ParserInfo() {};
	ParserInfo(const std::wstring& id): _id(id) {};
	ParserInfo(const std::wstring& id, const std::wstring& userDefinedLangName): _id(id), _userDefinedLangName(userDefinedLangName) {};
	~ParserInfo() { if (_parser) delete _parser; }
};

class FunctionParsersManager final
{
public:
	~FunctionParsersManager();

	bool init(const std::wstring& xmlPath, const std::wstring& xmlInstalledPath, ScintillaEditView ** ppEditView);
	bool parse(std::vector<foundInfo> & foundInfos, const AssociationInfo & assoInfo);
	

private:
	ScintillaEditView **_ppEditView = nullptr;
	std::wstring _xmlDirPath; // The 1st place to load function list files. Usually it's "%APPDATA%\Notepad++\functionList\"
	std::wstring _xmlDirInstalledPath; // Where Notepad++ is installed. The 2nd place to load function list files. Usually it's "%PROGRAMFILES%\Notepad++\functionList\" 

	ParserInfo* _parsers[L_EXTERNAL + nbMaxUserDefined] = {nullptr};
	int _currentUDIndex = L_EXTERNAL;

	bool getOverrideMapFromXmlTree(const std::wstring & xmlDirPath);
	bool loadFuncListFromXmlTree(const std::wstring & xmlDirPath, LangType lType, const std::wstring& overrideId, int udlIndex = -1);
	bool getZonePaserParameters(TiXmlNode *classRangeParser, std::wstring &mainExprStr, std::wstring &openSymboleStr, std::wstring &closeSymboleStr, std::vector<std::wstring> &classNameExprArray, std::wstring &functionExprStr, std::vector<std::wstring> &functionNameExprArray);
	bool getUnitPaserParameters(TiXmlNode *functionParser, std::wstring &mainExprStr, std::vector<std::wstring> &functionNameExprArray, std::vector<std::wstring> &classNameExprArray);
	FunctionParser * getParser(const AssociationInfo & assoInfo);
};
