// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
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

#ifndef FUNCTIONPARSER_H
#define FUNCTIONPARSER_H

class ScintillaEditView;
class TiXmlDocument;
class TiXmlNode;

struct foundInfo {
	generic_string _data;
	generic_string _data2;
	int _pos;
	int _pos2;
	foundInfo(): _data(TEXT("")), _data2(TEXT("")), _pos(-1), _pos2(-1) {};
};

class FunctionParser {
friend class FunctionParsersManager;
public:
	FunctionParser(const TCHAR *id, const TCHAR *displayName, const TCHAR *commentExpr, generic_string functionExpr, std::vector<generic_string> functionNameExprArray, std::vector<generic_string> classNameExprArray): 
	  _id(id), _displayName(displayName), _commentExpr(commentExpr?commentExpr:TEXT("")), _functionExpr(functionExpr), _functionNameExprArray(functionNameExprArray), _classNameExprArray(classNameExprArray){};

	virtual void parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName = TEXT("")) = 0;
	void funcParse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName = TEXT(""), const std::vector< std::pair<int, int> > * commentZones = NULL);
	bool isInZones(int pos2Test, const std::vector< std::pair<int, int> > & zones);
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


class FunctionZoneParser : public FunctionParser {
public:
	FunctionZoneParser(const TCHAR *id, const TCHAR *displayName, const TCHAR *commentExpr, generic_string rangeExpr,	generic_string openSymbole,	generic_string closeSymbole,
		std::vector<generic_string> classNameExprArray, generic_string functionExpr, std::vector<generic_string> functionNameExprArray):
		FunctionParser(id, displayName, commentExpr, functionExpr, functionNameExprArray, classNameExprArray), _rangeExpr(rangeExpr), _openSymbole(openSymbole), _closeSymbole(closeSymbole) {};

	void parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName = TEXT(""));
	
protected:
	void classParse(std::vector<foundInfo> & foundInfos, std::vector< std::pair<int, int> > & scannedZones, const std::vector< std::pair<int, int> > & commentZones, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName = TEXT(""));

private:
	generic_string _rangeExpr;
	generic_string _openSymbole;
	generic_string _closeSymbole;
	generic_string _functionExpr;

	size_t getBodyClosePos(size_t begin, const TCHAR *bodyOpenSymbol, const TCHAR *bodyCloseSymbol, const std::vector< std::pair<int, int> > & commentZones, ScintillaEditView **ppEditView);
};



class FunctionUnitParser : public FunctionParser {
public:
	FunctionUnitParser(const TCHAR *id, const TCHAR *displayName, const TCHAR *commentExpr,
		generic_string mainExpr, std::vector<generic_string> functionNameExprArray, 
		std::vector<generic_string> classNameExprArray): FunctionParser(id, displayName, commentExpr, mainExpr, functionNameExprArray, classNameExprArray){};

	void parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName = TEXT(""));

};

class FunctionMixParser : public FunctionZoneParser {
public:
	FunctionMixParser(const TCHAR *id, const TCHAR *displayName, const TCHAR *commentExpr, generic_string rangeExpr,	generic_string openSymbole,	generic_string closeSymbole,
		std::vector<generic_string> classNameExprArray, generic_string functionExpr, std::vector<generic_string> functionNameExprArray, FunctionUnitParser *funcUnitPaser):
		FunctionZoneParser(id, displayName, commentExpr, rangeExpr,	openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray), _funcUnitPaser(funcUnitPaser){};
		
	~FunctionMixParser() {
		if (_funcUnitPaser)
			delete _funcUnitPaser;
	}
	void parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView **ppEditView, generic_string classStructName = TEXT(""));

private:
	FunctionUnitParser *_funcUnitPaser;
};

struct AssociationInfo {
	int _id;
	int _langID;
	generic_string _ext;
	generic_string _userDefinedLangName;

	AssociationInfo(int id, int langID, const TCHAR *ext, const TCHAR *userDefinedLangName): _id(id), _langID(langID) {
		if (ext)
			_ext = ext;
		else
			_ext = TEXT("");

		if (userDefinedLangName)
			_userDefinedLangName = userDefinedLangName;
		else
			_userDefinedLangName = TEXT("");
	};
};

class FunctionParsersManager {
public:
	FunctionParsersManager() : _ppEditView(NULL), _pXmlFuncListDoc(NULL){};
	~FunctionParsersManager();
	bool init(generic_string xmlPath, ScintillaEditView ** ppEditView);
	bool parse(std::vector<foundInfo> & foundInfos, const AssociationInfo & assoInfo);
	void writeFunctionListXml(const TCHAR *destFoder) const;
	
private:
	ScintillaEditView **_ppEditView;
	std::vector<FunctionParser *> _parsers;
	std::vector<AssociationInfo> _associationMap;
	TiXmlDocument *_pXmlFuncListDoc;

	bool getFuncListFromXmlTree();
	bool getZonePaserParameters(TiXmlNode *classRangeParser, generic_string &mainExprStr, generic_string &openSymboleStr, generic_string &closeSymboleStr, std::vector<generic_string> &classNameExprArray, generic_string &functionExprStr, std::vector<generic_string> &functionNameExprArray);
	bool getUnitPaserParameters(TiXmlNode *functionParser, generic_string &mainExprStr, std::vector<generic_string> &functionNameExprArray, std::vector<generic_string> &classNameExprArray);
	FunctionParser * getParser(const AssociationInfo & assoInfo);
};

#endif //FUNCTIONPARSER_H

