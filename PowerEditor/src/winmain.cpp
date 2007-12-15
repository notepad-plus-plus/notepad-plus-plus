//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "Notepad_plus.h"
#include "SysMsg.h"
#include "Process.h"

#include <exception>

//const char localConfFile[] = "doLocalConf.xml";

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

static string getParamVal(char c, char *list2Clean) {
	char word[1024];
	bool checkDash = true;
	bool checkCh = false;
	bool action = false;
	bool isFileNamePart = false;
	int pos2Erase = 0;

	for (int i = 0, j = 0 ;  i <= int(strlen(list2Clean)) ; i++)
	{
		if ((list2Clean[i] == ' ') || (list2Clean[i] == '\0'))
		{
			if (action)
			{
				word[j] = '\0';
				j = 0;
				action = false;

				for (i = i + 1 ;  i <= int(strlen(list2Clean)) ; i++, pos2Erase++)
					list2Clean[pos2Erase] = list2Clean[i];
						
				list2Clean[pos2Erase] = '\0';

				return word;
			}
			checkDash = true;
		}
		else if (list2Clean[i] == '"')
		{
			isFileNamePart = !isFileNamePart;
		}

		if (!isFileNamePart)
		{
			if (action)
			{
				word[j++] =  list2Clean[i];
			}
			else if (checkDash)
			{
				if (list2Clean[i] == '-')
					checkCh = true;
			            
				if (list2Clean[i] != ' ')
					checkDash = false;
			}
			else if (checkCh)
			{
				if (list2Clean[i] == c)
				{
					action = true;
					pos2Erase = i-1;
				}
				checkCh = false;
			}
		}
	}
	return "";
};

static LangType getLangTypeFromParam(char *list2Clean) {
	string langStr = getParamVal('l', list2Clean);
	return NppParameters::getLangIDFromStr(langStr.c_str());
};

static int getLn2GoFromParam(char *list2Clean) {
	string lineNumStr = getParamVal('n', list2Clean);
	return atoi(lineNumStr.c_str());
};

const char FLAG_MULTI_INSTANCE[] = "-multiInst";
const char FLAG_NO_PLUGIN[] = "-noPlugin";
const char FLAG_READONLY[] = "-ro";
const char FLAG_NOSESSION[] = "-nosession";
const char FLAG_NOTABBAR[] = "-notabbar";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpszCmdLine, int nCmdShow)
{
	bool TheFirstOne = true;

	::SetLastError(NO_ERROR);
	::CreateMutex(NULL, false, "nppInstance");
	if (::GetLastError() == ERROR_ALREADY_EXISTS)
		TheFirstOne = false;

	CmdLineParams cmdLineParams;
	bool isMultiInst = isInList(FLAG_MULTI_INSTANCE, lpszCmdLine);
	cmdLineParams._isNoTab = isInList(FLAG_NOTABBAR, lpszCmdLine);
	cmdLineParams._isNoPlugin = isInList(FLAG_NO_PLUGIN, lpszCmdLine);
	cmdLineParams._isReadOnly = isInList(FLAG_READONLY, lpszCmdLine);
	cmdLineParams._isNoSession = isInList(FLAG_NOSESSION, lpszCmdLine);
	cmdLineParams._langType = getLangTypeFromParam(lpszCmdLine);
	cmdLineParams._line2go = getLn2GoFromParam(lpszCmdLine);
	
	NppParameters *pNppParameters = NppParameters::getInstance();

	if ((!isMultiInst) && (!TheFirstOne))
	{
		HWND hNotepad_plus = ::FindWindow(Notepad_plus::getClassName(), NULL);
		for (;!(hNotepad_plus = ::FindWindow(Notepad_plus::getClassName(), NULL));)
			Sleep(100);

		// First of all, destroy static object NppParameters
		pNppParameters->destroyInstance();

		int sw;

		if (::IsZoomed(hNotepad_plus))
			sw = SW_MAXIMIZE;
		else if (::IsIconic(hNotepad_plus))
			sw = SW_RESTORE;
		else
			sw = SW_SHOW;

		// IMPORTANT !!!
		::ShowWindow(hNotepad_plus, sw);

		::SetForegroundWindow(hNotepad_plus);

		if (lpszCmdLine[0])
		{
			COPYDATASTRUCT paramData;
			paramData.dwData = COPYDATA_PARAMS;

			COPYDATASTRUCT fileNamesData;
			fileNamesData.dwData = COPYDATA_FILENAMES;

			string quotFileName = "\"";
            // tell the other running instance the FULL path to the new file to load
			if (lpszCmdLine[0] == '"')
			{
				fileNamesData.lpData = (void *)lpszCmdLine;
				fileNamesData.cbData = long(strlen(lpszCmdLine) + 1);
			}
			else
			{
				char longFileName[MAX_PATH];
				::GetFullPathName(lpszCmdLine, MAX_PATH, longFileName, NULL);
				quotFileName += longFileName;
				quotFileName += "\"";

				fileNamesData.lpData = (void *)quotFileName.c_str();
				fileNamesData.cbData = long(quotFileName.length() + 1);
			}
			paramData.lpData = &cmdLineParams;
			paramData.cbData = sizeof(cmdLineParams);

			::SendMessage(hNotepad_plus, WM_COPYDATA, (WPARAM)hInstance, (LPARAM)&paramData);
			::SendMessage(hNotepad_plus, WM_COPYDATA, (WPARAM)hInstance, (LPARAM)&fileNamesData);
		}
		return 0;
	}

	pNppParameters->load();
	Notepad_plus notepad_plus_plus;
	
	NppGUI & nppGui = (NppGUI &)pNppParameters->getNppGUI();

	string updaterDir = pNppParameters->getNppPath();
	updaterDir += "\\updater\\";

	string updaterFullPath = updaterDir + "gup.exe";
 
	string version = "-v";
	version += VERSION_VALUE;

	bool isUpExist = nppGui._doesExistUpdater = (::PathFileExists(updaterFullPath.c_str()) == TRUE);
	bool doUpdate = !nppGui._neverUpdate;
	bool winSupported = notepad_plus_plus.getWinVersion() >= WV_W2K;
	if (!winSupported)
		nppGui._doesExistUpdater = false;

	if (TheFirstOne && isUpExist && doUpdate && winSupported)
	{
		Process updater(updaterFullPath.c_str(), version.c_str(), updaterDir.c_str());
		updater.run();
	}

	MSG msg;
	msg.wParam = 0;
	try {
        char *pPathNames = NULL;
        if (lpszCmdLine[0])
        {
            pPathNames = lpszCmdLine;
        }
		notepad_plus_plus.init(hInstance, NULL, pPathNames, &cmdLineParams);

		while (::GetMessage(&msg, NULL, 0, 0))
		{
			// if the message doesn't belong to the notepad_plus_plus's dialog
			if (!notepad_plus_plus.isDlgsMsg(&msg))
			{
				if (::TranslateAccelerator(notepad_plus_plus.getHSelf(), notepad_plus_plus.getAccTable(), &msg) == 0)
				{
					try {
						::TranslateMessage(&msg);
						::DispatchMessage(&msg);
					} catch(std::exception ex) {
						::MessageBox(NULL, ex.what(), "Exception", MB_OK);
					} catch(...) {
						systemMessage("System Error");
					}
				}
			}
		}
	} catch(int i) {
		if (i == 106901)
			::MessageBox(NULL, "Scintilla.init is failed!", "106901", MB_OK);
		else {
			char str[50] = "God Damned Exception : ";
			char code[10];
			itoa(i, code, 10);
			::MessageBox(NULL, strcat(str, code), "int exception", MB_OK);
		}
	}
	return (UINT)msg.wParam;
}

