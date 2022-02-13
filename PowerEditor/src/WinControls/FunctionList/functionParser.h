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
	generic_string _data;
	generic_string _data2;
	intptr_t _pos = -1;
	intptr_t _pos2 = -1;
};

class FunctionParser
{
friend class FunctionParsersManager;
public:
	FunctionParser(const TCHAR *id, const TCHAR *displayName, const TCHAR *commentExpr, const generic_string& functionExpr, const std::vector<generic_string>& functionNameExprArray, const std::vector<generic_string>& classNameExprArray):
	  _id(id), _displayName(displayName), _commentExpr(commentExpr?commentExpr:TEXT("")), _functionExpr(functionExpr), _functionNameExprArray(functionNameExprArray), _classNameExprArray(classNameExprArray){};

	virtual void parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName = TEXT("")) = 0;
	void funcParse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName = TEXT(""), const std::vector< std::pair<size_t, size_t> > * commentZones = NULL);
	bool isInZones(size_t pos2Test, const std::vector< std::pair<size_t, size_t> > & zones);
	virtual ~FunctionParser() = default;

protected:
	generic_string _id;
	generic_string _displayName;
	generic_string _commentExpr;
	generic_string _functionExpr;
	std::vector<generic_string> _functionNameExprArray;
	std::vector<generic_string> _classNameExprArray;
	void getCommentZones(std::vector< std::pair<size_t, size_t> > & commentZone, size_t begin, size_t end, ScintillaEditView **ppEditView);
	void getInvertZones(std::vector< std::pair<size_t, size_t> > & destZones, std::vector< std::pair<size_t, size_t> > & sourceZones, size_t begin, size_t end);
	generic_string parseSubLevel(size_t begin, size_t end, std::vector< generic_string > dataToSearch, intptr_t & foundPos, ScintillaEditView **ppEditView);
};


class FunctionZoneParser : public FunctionParser
{
public:
	FunctionZoneParser() = delete;
	FunctionZoneParser(const TCHAR *id, const TCHAR *displayName, const TCHAR *commentExpr, const generic_string& rangeExpr, const generic_string& openSymbole, const generic_string& closeSymbole,
		const std::vector<generic_string>& classNameExprArray, const generic_string& functionExpr, const std::vector<generic_string>& functionNameExprArray):
		FunctionParser(id, displayName, commentExpr, functionExpr, functionNameExprArray, classNameExprArray), _rangeExpr(rangeExpr), _openSymbole(openSymbole), _closeSymbole(closeSymbole) {};

	void parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName = TEXT(""));
	
protected:
	void classParse(std::vector<foundInfo> & foundInfos, std::vector< std::pair<size_t, size_t> > & scannedZones, const std::vector< std::pair<size_t, size_t> > & commentZones, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName = TEXT(""));

private:
	generic_string _rangeExpr;
	generic_string _openSymbole;
	generic_string _closeSymbole;

	size_t getBodyClosePos(size_t begin, const TCHAR *bodyOpenSymbol, const TCHAR *bodyCloseSymbol, const std::vector< std::pair<size_t, size_t> > & commentZones, ScintillaEditView **ppEditView);
};



class FunctionUnitParser : public FunctionParser
{
public:
	FunctionUnitParser(const TCHAR *id, const TCHAR *displayName, const TCHAR *commentExpr,
		const generic_string& mainExpr, const std::vector<generic_string>& functionNameExprArray,
		const std::vector<generic_string>& classNameExprArray): FunctionParser(id, displayName, commentExpr, mainExpr, functionNameExprArray, classNameExprArray)
	{}

	void parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName = TEXT(""));
};

class FunctionMixParser : public FunctionZoneParser
{
public:
	FunctionMixParser(const TCHAR *id, const TCHAR *displayName, const TCHAR *commentExpr, const generic_string& rangeExpr, const generic_string& openSymbole, const generic_string& closeSymbole,
		const std::vector<generic_string>& classNameExprArray, const generic_string& functionExpr, const std::vector<generic_string>& functionNameExprArray, FunctionUnitParser *funcUnitPaser):
		FunctionZoneParser(id, displayName, commentExpr, rangeExpr,	openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray), _funcUnitPaser(funcUnitPaser){};
		
	~FunctionMixParser()
	{
		delete _funcUnitPaser;
	}

	void parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName = TEXT(""));

private:
	FunctionUnitParser* _funcUnitPaser = nullptr;
};


struct AssociationInfo final
{
	int _id;
	int _langID;
	generic_string _ext;
	generic_string _userDefinedLangName;

	AssociationInfo(int id, int langID, const TCHAR *ext, const TCHAR *userDefinedLangName)
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
	generic_string _id; // xml parser rule file name - if empty, then we use default name. Mandatory if _userDefinedLangName is not empty
	FunctionParser* _parser = nullptr;
	generic_string _userDefinedLangName;

	ParserInfo() {};
	ParserInfo(const generic_string& id): _id(id) {};
	ParserInfo(const generic_string& id, const generic_string& userDefinedLangName): _id(id), _userDefinedLangName(userDefinedLangName) {};
	~ParserInfo() { if (_parser) delete _parser; }
};

class FunctionParsersManager final
{
public:
	~FunctionParsersManager();

	bool init(const generic_string& xmlPath, const generic_string& xmlInstalledPath, ScintillaEditView ** ppEditView);
	bool parse(std::vector<foundInfo> & foundInfos, const AssociationInfo & assoInfo);
	

private:
	ScintillaEditView **_ppEditView = nullptr;
	generic_string _xmlDirPath; // The 1st place to load function list files. Usually it's "%APPDATA%\Notepad++\functionList\"
	generic_string _xmlDirInstalledPath; // Where Notepad++ is installed. The 2nd place to load function list files. Usually it's "%PROGRAMFILES%\Notepad++\functionList\" 

	ParserInfo* _parsers[L_EXTERNAL + nbMaxUserDefined] = {nullptr};
	int _currentUDIndex = L_EXTERNAL;

	bool getOverrideMapFromXmlTree(generic_string & xmlDirPath);
	bool loadFuncListFromXmlTree(generic_string & xmlDirPath, LangType lType, const generic_string& overrideId, int udlIndex = -1);
	bool getZonePaserParameters(TiXmlNode *classRangeParser, generic_string &mainExprStr, generic_string &openSymboleStr, generic_string &closeSymboleStr, std::vector<generic_string> &classNameExprArray, generic_string &functionExprStr, std::vector<generic_string> &functionNameExprArray);
	bool getUnitPaserParameters(TiXmlNode *functionParser, generic_string &mainExprStr, std::vector<generic_string> &functionNameExprArray, std::vector<generic_string> &classNameExprArray);
	FunctionParser * getParser(const AssociationInfo & assoInfo);
};
