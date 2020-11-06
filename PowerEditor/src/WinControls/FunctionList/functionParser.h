// This file is part of Notepad++ project
// Copyright (C)2020 Don HO <don.h@free.fr>
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

#pragma once

class ScintillaEditView;
class TiXmlDocument;
class TiXmlNode;

struct foundInfo final
{
	generic_string _data;
	generic_string _data2;
	int _pos = -1;
	int _pos2 = -1;
};

class FunctionParser
{
friend class FunctionParsersManager;
public:
	FunctionParser(const TCHAR *id, const TCHAR *displayName, const TCHAR *commentExpr, const generic_string& functionExpr, const std::vector<generic_string>& functionNameExprArray, const std::vector<generic_string>& classNameExprArray):
	  _id(id), _displayName(displayName), _commentExpr(commentExpr?commentExpr:TEXT("")), _functionExpr(functionExpr), _functionNameExprArray(functionNameExprArray), _classNameExprArray(classNameExprArray){};

	virtual void parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName = TEXT("")) = 0;
	void funcParse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName = TEXT(""), const std::vector< std::pair<int, int> > * commentZones = NULL);
	bool isInZones(int pos2Test, const std::vector< std::pair<int, int> > & zones);
	virtual ~FunctionParser() = default;

protected:
	generic_string _id;
	generic_string _displayName;
	generic_string _commentExpr;
	generic_string _functionExpr;
	std::vector<generic_string> _functionNameExprArray;
	std::vector<generic_string> _classNameExprArray;
	void getCommentZones(std::vector< std::pair<int, int> > & commentZone, size_t begin, size_t end, ScintillaEditView **ppEditView);
	void getInvertZones(std::vector< std::pair<int, int> > & destZones, std::vector< std::pair<int, int> > & sourceZones, size_t begin, size_t end);
	generic_string parseSubLevel(size_t begin, size_t end, std::vector< generic_string > dataToSearch, int & foundPos, ScintillaEditView **ppEditView);
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
	void classParse(std::vector<foundInfo> & foundInfos, std::vector< std::pair<int, int> > & scannedZones, const std::vector< std::pair<int, int> > & commentZones, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName = TEXT(""));

private:
	generic_string _rangeExpr;
	generic_string _openSymbole;
	generic_string _closeSymbole;

	size_t getBodyClosePos(size_t begin, const TCHAR *bodyOpenSymbol, const TCHAR *bodyCloseSymbol, const std::vector< std::pair<int, int> > & commentZones, ScintillaEditView **ppEditView);
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

	bool init(const generic_string& xmlPath, ScintillaEditView ** ppEditView);
	bool parse(std::vector<foundInfo> & foundInfos, const AssociationInfo & assoInfo);
	

private:
	ScintillaEditView **_ppEditView = nullptr;
	generic_string _xmlDirPath;
	ParserInfo* _parsers[L_EXTERNAL + nbMaxUserDefined] = {nullptr};
	int _currentUDIndex = L_EXTERNAL;

	bool getOverrideMapFromXmlTree();
	bool loadFuncListFromXmlTree(LangType lType, const generic_string& overrideId, int udlIndex = -1);
	bool getZonePaserParameters(TiXmlNode *classRangeParser, generic_string &mainExprStr, generic_string &openSymboleStr, generic_string &closeSymboleStr, std::vector<generic_string> &classNameExprArray, generic_string &functionExprStr, std::vector<generic_string> &functionNameExprArray);
	bool getUnitPaserParameters(TiXmlNode *functionParser, generic_string &mainExprStr, std::vector<generic_string> &functionNameExprArray, std::vector<generic_string> &classNameExprArray);
	FunctionParser * getParser(const AssociationInfo & assoInfo);
};
