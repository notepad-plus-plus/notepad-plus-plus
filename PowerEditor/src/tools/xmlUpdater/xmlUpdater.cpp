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


#include <windows.h>
#include "tinyxml.h"
#define MODEL_INVALID 1
#define SRC_INVALID 2
#define DEST_INVALID 3

static bool isInList(const char *token2Find, char *list2Clean) {
	char word[1024];
	bool isFileNamePart = false;

	for (int i = 0, j = 0 ;  i <= int(strlen(list2Clean)) ; i++)
	{
		if ((list2Clean[i] == ' ') || (list2Clean[i] == '\0'))
		{
			if ((j) && (!isFileNamePart))
			{
				word[j] = '\0';
				j = 0;
				bool bingo = !strcmp(token2Find, word);

				if (bingo)
				{
					int wordLen = int(strlen(word));
					int prevPos = i - wordLen;

					for (i = i + 1 ;  i <= int(strlen(list2Clean)) ; i++, prevPos++)
						list2Clean[prevPos] = list2Clean[i];

					list2Clean[prevPos] = '\0';
					
					return true;
				}
			}
		}
		else if (list2Clean[i] == '"')
		{
			isFileNamePart = !isFileNamePart;
		}
		else
		{
			word[j++] = list2Clean[i];
		}
	}
	return false;
};


void update(TiXmlNode *modelNode, TiXmlNode *srcNode, TiXmlNode *destNode) {
	TiXmlNode *srcChildNode = NULL;
	TiXmlNode *destChildNode = NULL;
	TiXmlNode *modelChildNode = modelNode->FirstChild("Node");
	
	if (!srcNode) return;

	for (modelChildNode = modelNode->FirstChild("Node"); 
		 modelChildNode;
		 modelChildNode = modelChildNode->NextSibling("Node"))
	{
		const char *nodeName = (modelChildNode->ToElement())->Attribute("nodeName");
		const char *name = (modelChildNode->ToElement())->Attribute("name");
		if (nodeName)
		{
			srcChildNode = srcNode->FirstChild(nodeName);
			if (!srcChildNode) continue;

			destChildNode = destNode->FirstChild(nodeName);
			if (!destChildNode)
			{
				//Insertion
				destNode->InsertEndChild(*srcChildNode);
				continue;
			}
			if (name && name[0])
			{
				srcChildNode = srcNode->FirstChild(nodeName);
				while (srcChildNode)
				{
					const char *attrib = (srcChildNode->ToElement())->Attribute(name);
					if (attrib)
					{
						const char *action = (srcChildNode->ToElement())->Attribute("action");
						bool remove = false;
						bool found = false;

						if (action && !strcmp(action, "remove"))
							remove = true;

						destChildNode = destNode->FirstChild(nodeName);
						while (destChildNode)
						{
							const char *attribDest = (destChildNode->ToElement())->Attribute(name);
							if ((attribDest) && (!strcmp(attrib, attribDest)))
							{
								found = true;
								break;
							}
							destChildNode = destChildNode->NextSibling(nodeName);
						}
						if (remove)
						{
							if (found) destNode->RemoveChild(destChildNode);
						}
						else
						{
							if (found)
								update(modelChildNode, srcChildNode, destChildNode);
							else
								destNode->InsertEndChild(*srcChildNode);
						}
					}
					srcChildNode = srcChildNode->NextSibling(nodeName);
				} // while srcChildNode
			}
		}
		update(modelChildNode, srcChildNode, destChildNode);
	}
};


int main(int argc, char *argv[])
{
	if (argc != 4) 
	{
		printf("Syntax : xmlUpdater model.xml src.xml dest.xml");
		return -1;
	}

	char *xmlModelPath = argv[1];
	char *xmlSrcPath = argv[2];
	char *xmlDestPath = argv[3];

	//printf("%s\n", xmlModelPath);
	//printf("%s\n", xmlSrcPath);
	//printf("%s\n", xmlDestPath);

	TiXmlDocument *pXmlModel = NULL;
	TiXmlDocument *pXmlSrc = NULL;
	TiXmlDocument *pXmlDest = NULL;

	try {
		pXmlModel = new TiXmlDocument(xmlModelPath);
		bool loadOkay = pXmlModel->LoadFile();
		if (!loadOkay) throw int(MODEL_INVALID);

		pXmlSrc = new TiXmlDocument(xmlSrcPath);
		loadOkay = pXmlSrc->LoadFile();
		if (!loadOkay) throw int(SRC_INVALID);
		
		pXmlDest = new TiXmlDocument(xmlDestPath);
		loadOkay = pXmlDest->LoadFile();
		if (!loadOkay) throw int(DEST_INVALID);

		TiXmlNode *root = pXmlModel->FirstChild("Node");
		const char *nodeRootName = (root->ToElement())->Attribute("nodeName");
		if (nodeRootName)
		{
			TiXmlNode *srcRoot = pXmlSrc->FirstChild(nodeRootName);
			if (!srcRoot) throw int(4);
			TiXmlNode *destRoot = pXmlDest->FirstChild(nodeRootName);
			if (!destRoot)
			{
				throw int(DEST_INVALID);
			}
			else
			{
				update(root, srcRoot, destRoot);
			}
		}
	} catch (int errMsg) {
		char *msg;
		if (errMsg == MODEL_INVALID)
			msg = "Model file is invalidated";
		if (errMsg == SRC_INVALID)
			msg = "Source file is invalidated";
		if (errMsg == DEST_INVALID)
			msg = "File to update is invalidated";

		if (pXmlModel) delete pXmlModel;
		if (pXmlSrc) delete pXmlSrc;
		if (pXmlDest) delete pXmlDest;

		printf(msg);
		return -1;
	}

	pXmlDest->SaveFile();
	
	delete pXmlModel;
	delete pXmlSrc;
	delete pXmlDest;
	printf("Update successful");

	return 0;
}

/*
const char FLAG_SILENT[] = "-silent";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpszCmdLine, int nCmdShow)
//int main(int argc, char *argv[])
{
	bool isSilentMode = isInList(FLAG_SILENT, lpszCmdLine);

 	int	argc=0;
 	LPSTR	argv[10];
 	LPSTR	p, q;

 	argv[argc] = "xmlUpdater.exe";
 	// Parse command line handling quotes.
 	p = lpszCmdLine;
 	while (*p) 
	{
 	    // for each argument
 	    while ((*p) && (*p == ' '))
 			p++;	// skip over leading spaces
 	    if (*p == '\042') 
		{
 	       p++;		// skip "
 	       q = p;
 	       // scan to end of argument
 	       // doesn't handle embedded quotes
 	       while ((*p) && (*p != '\042'))
 		    p++;
 	       argv[++argc] = q;
 	       if (*p)
 			*p++ = '\0';
 	    }
 	    else if (*p) 
		{
 	       // delimited by spaces
 	       q = p;
 	       while ((*p) && (*p != ' '))
 		    p++;
 	       argv[++argc] = q;
 	       if (*p)
 		    *p++ = '\0';
 	    }
 	}
 	argv[++argc] = (LPSTR)NULL;

	if (argc < 4) 
	{
		//printf();
		if (!isSilentMode)
			MessageBox(NULL,  "xmlUpdater model.xml src.xml dest.xml", "Syntax", MB_OK);
		return -1;
	}

	char *xmlModelPath = argv[1];
	char *xmlSrcPath = argv[2];
	char *xmlDestPath = argv[3];

	//printf("%s\n", xmlModelPath);
	//printf("%s\n", xmlSrcPath);
	//printf("%s\n", xmlDestPath);

	TiXmlDocument *pXmlModel = NULL;
	TiXmlDocument *pXmlSrc = NULL;
	TiXmlDocument *pXmlDest = NULL;

	try {
		pXmlModel = new TiXmlDocument(xmlModelPath);
		bool loadOkay = pXmlModel->LoadFile();
		if (!loadOkay) throw int(MODEL_INVALID);

		pXmlSrc = new TiXmlDocument(xmlSrcPath);
		loadOkay = pXmlSrc->LoadFile();
		if (!loadOkay) throw int(SRC_INVALID);
		
		pXmlDest = new TiXmlDocument(xmlDestPath);
		loadOkay = pXmlDest->LoadFile();
		if (!loadOkay) throw int(DEST_INVALID);

		TiXmlNode *root = pXmlModel->FirstChild("Node");
		const char *nodeRootName = (root->ToElement())->Attribute("nodeName");
		if (nodeRootName)
		{
			TiXmlNode *srcRoot = pXmlSrc->FirstChild(nodeRootName);
			if (!srcRoot) throw int(4);
			TiXmlNode *destRoot = pXmlDest->FirstChild(nodeRootName);
			if (!destRoot)
			{
				throw int(DEST_INVALID);
			}
			else
			{
				update(root, srcRoot, destRoot);
			}
		}
	} catch (int errMsg) {
		char *msg;
		if (errMsg == MODEL_INVALID)
			msg = "Model file is invalidated";
		if (errMsg == SRC_INVALID)
			msg = "Source file is invalidated";
		if (errMsg == DEST_INVALID)
			msg = "File to update is invalidated";

		if (pXmlModel) delete pXmlModel;
		if (pXmlSrc) delete pXmlSrc;
		if (pXmlDest) delete pXmlDest;

		if (!isSilentMode)
			MessageBox(NULL, msg, "Update Failure", MB_OK);
		return -1;
	}

	pXmlDest->SaveFile();
	
	delete pXmlModel;
	delete pXmlSrc;
	delete pXmlDest;
	if (!isSilentMode)
		MessageBox(NULL, "Update successful", "Update status", MB_OK);

	return 0;
}
*/
