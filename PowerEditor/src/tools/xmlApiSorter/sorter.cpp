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


#include <cstdlib>
#include <iostream>
#include "tinyxml.h"

#include <vector>
#include <algorithm>

using namespace std;

struct xmlname{
	TiXmlElement * node;
	const char * name;
	xmlname(TiXmlElement * n, const char * na) { node = n; name = na;}
};

//true if x1 smaller
bool sortXMLCase(const xmlname & x1, const xmlname & x2) {
	return (strcmp(x1.name, x2.name) < 0);
}

inline bool lower(char c) {
	return (c >= 'a' && c <= 'z');	
}

inline bool match(char c1, char c2) {
	if (c1 == c2)	return true;
	if (lower(c1))
		return ((c1-32) == c2);
	if (lower(c2))
		return ((c2-32) == c1);
	return false;	
}

//true if x1 smaller
bool sortXML(const xmlname & x1, const xmlname & x2) {
	
	const char * n1 = x1.name, * n2 = x2.name;
	int i = 0;
	while(match(n2[i], n1[i])) {
		if (n1[i] == 0) {
			return true;	//equal	
		}
		i++;	
	}
	
	int subs1 = lower(n1[i])?32:0;
	int subs2 = lower(n2[i])?32:0;
	
	return ( (n1[i]-subs1) < (n2[i]-subs2) );
	
}

void merge(TiXmlElement * n1, TiXmlElement * n2);

int main(int argc, char *argv[])
{
	const char * file = NULL;

	if (argc < 2) {
		cout << "Usage: sorter.exe xmlfile.xml" << endl;
		return 1;
	} 
	file = argv[1];

	TiXmlDocument *pXmlApi = NULL;
	pXmlApi = new TiXmlDocument(file);
	bool loadOkay = pXmlApi->LoadFile();
	if (!loadOkay) return 1;

	TiXmlNode *root = pXmlApi->FirstChild("NotepadPlus");
	if (!root) {
		cout << "NotepadPlus node not found\n";
		return 1;
	}
	TiXmlElement *autoc = root->FirstChildElement("AutoComplete");
	if (!autoc) {
		cout << "AutoComplete node not found\n";
		return 1;
	}
	const char * langName = autoc->Attribute("language");

	TiXmlElement *envNode = autoc->FirstChildElement("Environment");
	bool ignoreCase = false;
	if (envNode) {
		cout << "Found environment settings\n";
		const char * ignoreCaseText = envNode->Attribute("ignoreCase");
		if (ignoreCaseText) {
			ignoreCase = (strcmp(ignoreCaseText, "yes") == 0);
			if (ignoreCase) {
				cout << "Sorting case insensitive\n";
			} else {
				cout << "Sorting case sensitive\n";
			}
		} else {
			cout <<"Cannot find attribute \"ignoreCase\", defaulting to case sensitive sort\nConsider adding the node\n";
		}
	} else {
		cout << "No environment settings found, defaulting to case sensitive sort\nConsider adding the node\n";
	}

	vector<xmlname> words;
	for (TiXmlElement *childNode = autoc->FirstChildElement("KeyWord");
		childNode ;
		childNode = childNode->NextSiblingElement("KeyWord") )
	{
		const char * name = childNode->Attribute("name");
		if (!name) {
			cout << "Warning: KeyWord without name!, skipping...\n";
			continue;
		} else {
			int i = 0;
			while(name[i] != 0) {
				if (!isalnum(name[i]) && name[i] != '_') {
					cout << "Warning, keyword " << name << " contains unsupported characters!\n";
					break;
				}
				i++;
			}
			words.push_back(xmlname(childNode, name));
		}
	}

	if (ignoreCase)
		sort(words.begin(), words.end(), sortXML);
	else
		sort(words.begin(), words.end(), sortXMLCase);

	for(size_t i = 1; i < words.size(); i++) {
		//merge duplicates
		if (!strcmp(words[i].name, words[i-1].name)) {
			merge(words[i-1].node, words[i].node);
			words.erase(words.begin() + i);
		}
	}

	TiXmlDocument doc;
	TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "Windows-1252", "" );
	doc.LinkEndChild( decl );
	TiXmlElement * element = new TiXmlElement( "NotepadPlus" );
	doc.LinkEndChild( element );
	TiXmlElement * element2 = new TiXmlElement( "AutoComplete" );
	element->LinkEndChild( element2 );

	if (langName)
		element2->SetAttribute("language", langName);

	if (envNode)
		element2->LinkEndChild(envNode);
	
	for(size_t i = 0; i < words.size(); i++) {
		element2->LinkEndChild(words[i].node);
	}

	doc.SaveFile( file );

	return 0;
}

void merge(TiXmlElement * n1, TiXmlElement * n2) {
	const char * funcAttr = NULL;
	funcAttr = n2->Attribute("func");
	if (!funcAttr || !strcmp(funcAttr, "yes")) {
		return;	
	}

	n1->SetAttribute("func", "yes");
	
	for (TiXmlElement *childNode = n2->FirstChildElement("Overload");
		childNode ;
		childNode = childNode->NextSiblingElement("Overload") )
	{
		n1->LinkEndChild(childNode);
	}
	
	return;
}
