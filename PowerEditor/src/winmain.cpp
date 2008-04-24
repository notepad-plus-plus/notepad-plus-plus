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

typedef std::vector<const char*> ParamVector;
void parseCommandLine(char * commandLine, ParamVector & paramVector) {
	bool isInFile = false;
	bool isInWhiteSpace = true;
	paramVector.clear();
	size_t commandLength = strlen(commandLine);
	for(size_t i = 0; i < commandLength; i++) {
		switch(commandLine[i]) {
			case '\"': {										//quoted filename, ignore any following whitespace
				if (!isInFile) {	//" will always be treated as start or end of param, in case the user forgot to add an space
					paramVector.push_back(commandLine+i+1);	//add next param(since zero terminated string original, no overflow of +1)
				}
				isInFile = !isInFile;
				isInWhiteSpace = false;
				//because we dont want to leave in any quotes in the filename, remove them now (with zero terminator)
				commandLine[i] = 0;
				break; }
			case '\t':	//also treat tab as whitespace
			case ' ': {
				isInWhiteSpace = true;
				if (!isInFile)
					commandLine[i] = 0;		//zap spaces into zero terminators, unless its part of a filename
				break; }
			default: {											//default char, if beginning of word, add it
				if (!isInFile && isInWhiteSpace) {
					paramVector.push_back(commandLine+i);	//add next param 
					isInWhiteSpace = false;
				}
				break; }
		}
	}
	//the commandline string is now a list of zero terminated strings concatenated, and the vector contains all the substrings
}

bool isInList(const char *token2Find, ParamVector & params) {
	int nrItems = params.size();

	for (int i = 0; i < nrItems; i++)
	{
		if (!strcmp(token2Find, params.at(i))) {
			params.erase(params.begin() + i);
			return true;
		}
	}
	return false;
};

string getParamVal(char c, ParamVector & params) {
	int nrItems = params.size();

	for (int i = 0; i < nrItems; i++)
	{
		const char * token = params.at(i);
		if (token[0] == '-' && strlen(token) >= 2 && token[1] == c) {	//dash, and enough chars
			string retval(token+2);
			params.erase(params.begin() + i);
			return retval;
		}
	}
	return string("");
}

LangType getLangTypeFromParam(ParamVector & params) {
	string langStr = getParamVal('l', params);
	return NppParameters::getLangIDFromStr(langStr.c_str());
};

int getLn2GoFromParam(ParamVector & params) {
	string lineNumStr = getParamVal('n', params);
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

	ParamVector params;
	parseCommandLine(lpszCmdLine, params);

	CmdLineParams cmdLineParams;
	bool isMultiInst = isInList(FLAG_MULTI_INSTANCE, params);
	cmdLineParams._isNoTab = isInList(FLAG_NOTABBAR, params);
	cmdLineParams._isNoPlugin = isInList(FLAG_NO_PLUGIN, params);
	cmdLineParams._isReadOnly = isInList(FLAG_READONLY, params);
	cmdLineParams._isNoSession = isInList(FLAG_NOSESSION, params);
	cmdLineParams._langType = getLangTypeFromParam(params);
	cmdLineParams._line2go = getLn2GoFromParam(params);
	
	NppParameters *pNppParameters = NppParameters::getInstance();

	// override the settings if notepad style is present
	if (pNppParameters->asNotepadStyle())
	{
		isMultiInst = true;
		cmdLineParams._isNoTab = true;
		cmdLineParams._isNoSession = true;
	}

	string quotFileName = "";
    // tell the running instance the FULL path to the new files to load
	size_t nrFilesToOpen = params.size();
	const char * currentFile;
	char fullFileName[MAX_PATH];
	for(size_t i = 0; i < nrFilesToOpen; i++) {
		currentFile = params.at(i);
		//check if relative or full path. Relative paths dont have a colon for driveletter
		BOOL isRelative = ::PathIsRelative(currentFile);
		quotFileName += "\"";
		if (isRelative) {
			::GetFullPathName(currentFile, MAX_PATH, fullFileName, NULL);
			quotFileName += fullFileName;
		} else {
			quotFileName += currentFile;
		}
		quotFileName += "\"";
	}

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

		if (params.size() > 0)	//if there are files to open, use the WM_COPYDATA system
		{
			COPYDATASTRUCT paramData;
			paramData.dwData = COPYDATA_PARAMS;
			paramData.lpData = &cmdLineParams;
			paramData.cbData = sizeof(cmdLineParams);

			COPYDATASTRUCT fileNamesData;
			fileNamesData.dwData = COPYDATA_FILENAMES;
			fileNamesData.lpData = (void *)quotFileName.c_str();
			fileNamesData.cbData = long(quotFileName.length() + 1);

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

	winVer curWinVer = notepad_plus_plus.getWinVersion();

	bool isUpExist = nppGui._doesExistUpdater = (::PathFileExists(updaterFullPath.c_str()) == TRUE);
	bool doUpdate = !nppGui._neverUpdate;
	bool winSupported = (curWinVer >= WV_W2K);

	// Vista UAC de mes couilles!!!
	bool isVista = (curWinVer == WV_VISTA);

	if (!winSupported)
		nppGui._doesExistUpdater = false;

	if (TheFirstOne && isUpExist && doUpdate && winSupported && !isVista)
	{
		Process updater(updaterFullPath.c_str(), version.c_str(), updaterDir.c_str());
		updater.run();
	}

	MSG msg;
	msg.wParam = 0;
	try {
		notepad_plus_plus.init(hInstance, NULL, quotFileName.c_str(), &cmdLineParams);

		bool unicodeSupported = notepad_plus_plus.getWinVersion() >= WV_NT;
		bool going = true;
		while (going)
		{
			going = (unicodeSupported?(::GetMessageW(&msg, NULL, 0, 0)):(::GetMessageA(&msg, NULL, 0, 0))) != 0;
			if (going)
			{
				// if the message doesn't belong to the notepad_plus_plus's dialog
				if (!notepad_plus_plus.isDlgsMsg(&msg, unicodeSupported))
				{
					if (::TranslateAccelerator(notepad_plus_plus.getHSelf(), notepad_plus_plus.getAccTable(), &msg) == 0)
					{
						try {
							::TranslateMessage(&msg);
							if (unicodeSupported)
								::DispatchMessageW(&msg);
							else
								::DispatchMessage(&msg);
						} catch(std::exception ex) {
							::MessageBox(NULL, ex.what(), "Exception", MB_OK);
						} catch(...) {
							systemMessage("System Error");
						}
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

