#include "Buffer.h"

long Buffer::_recentTagCtr = 0;

void Buffer::setFileName(const char *fn, LangType defaultLang) 
{
	bool isExtSet = false;

	NppParameters *pNppParamInst = NppParameters::getInstance();
	strcpy(_fullPathName, fn);
    if (PathFileExists(_fullPathName))
	{
		// for _lang
		char *ext = PathFindExtension(_fullPathName);
		if (*ext == '.') ext += 1;

		// Define User Lang firstly
		const char *langName = NULL;
		if ((langName = pNppParamInst->getLangNameFromExt(ext)))
		{
			_lang = L_USER;
			strcpy(_userLangExt, langName);
			isExtSet = true;
		}
		else // if it's not user lang, then check if it's supported lang
		{
			_lang = getLangFromExt(ext);
			if (_lang == L_TXT)
			{
				char *fileName = PathFindFileName(_fullPathName);

				if ((!_stricmp(fileName, "makefile")) || (!_stricmp(fileName, "GNUmakefile")))
					_lang = L_MAKEFILE;
				else if (!_stricmp(fileName, "CmakeLists.txt"))
					_lang = L_CMAKE;
			}
		}

		if (!isExtSet)
			_userLangExt[0] = '\0';
		// for _timeStamp
		updatTimeStamp();
	}
	else // new doc
	{
		_lang = defaultLang;
		_timeStamp = 0;
	}
}

LangType Buffer::getLangFromExt(const char *ext)
{
	int i = 0;
	Lang *l = NppParameters::getInstance()->getLangFromIndex(i++);
	while (l)
	{
		const char *defList = l->getDefaultExtList();
		const char *userList = NULL;

		LexerStylerArray &lsa = (NppParameters::getInstance())->getLStylerArray();
		const char *lName = l->getLangName();
		LexerStyler *pLS = lsa.getLexerStylerByName(lName);
		
		if (pLS)
			userList = pLS->getLexerUserExt();

		std::string list("");
		if (defList)
			list += defList;
		if (userList)
		{
			list += " ";
			list += userList;
		}
		if (isInList(ext, list.c_str()))
			return l->getLangID();
		l = (NppParameters::getInstance())->getLangFromIndex(i++);
	}
	return L_TXT;
}
