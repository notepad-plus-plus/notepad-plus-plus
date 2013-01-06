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
	if (!root) 
		return false;


	for (TiXmlNode *childNode = parserRoot->FirstChildElement(TEXT("parser"));
		childNode;
		childNode = childNode->NextSibling(TEXT("parser")) )
	{
		const TCHAR *id = (childNode->ToElement())->Attribute(TEXT("id"));
		if (!id || !id[0])
			continue;

		const TCHAR *displayName = (childNode->ToElement())->Attribute(TEXT("displayName"));
		if (!displayName || !displayName[0])
			displayName = id;

		TiXmlNode *classRangeParser = childNode->FirstChild(TEXT("classRange"));
		if (classRangeParser)
		{
			const TCHAR *mainExpr = NULL;
			const TCHAR *openSymbole = NULL;
			const TCHAR *closeSymbole = NULL;
			std::vector<generic_string> classNameExprArray;
			const TCHAR *functionExpr = NULL;
			std::vector<generic_string> functionNameExprArray;

			mainExpr = (classRangeParser->ToElement())->Attribute(TEXT("mainExpr"));
			if (!mainExpr)
				continue;

			openSymbole = (classRangeParser->ToElement())->Attribute(TEXT("openSymbole"));
			closeSymbole = (classRangeParser->ToElement())->Attribute(TEXT("closeSymbole"));
			TiXmlNode *classNameParser = childNode->FirstChild(TEXT("className"));
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

			TiXmlNode *functionParser = childNode->FirstChild(TEXT("function"));
			if (functionParser)
			{
				functionExpr = (classRangeParser->ToElement())->Attribute(TEXT("mainExpr"));
				if (!functionExpr)
					continue;
				TiXmlNode *functionNameParser = childNode->FirstChild(TEXT("functionName"));
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
			}

			_parsers.push_back(new FunctionZoneParser(id, displayName, mainExpr, openSymbole, closeSymbole, classNameExprArray, functionExpr, functionNameExprArray));
		}
		else
		{
			TiXmlNode *functionParser = childNode->FirstChild(TEXT("fuction"));
			if (!functionParser)
			{
				continue;
			}
		}

		//_parsers.push_back();
	}

	return (_parsers.size() != 0);
}

FunctionParser * FunctionParsersManager::getParser(generic_string ext)
{
	return NULL;
}

void FunctionZoneParser::parse(std::vector<foundInfo> & /*foundInfos*/)
{

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
	fp->parse(foundInfos);

	return true;
}