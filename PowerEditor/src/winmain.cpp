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

#include <exception>		//default C++ exception
#include "Win32Exception.h"	//Win32 exception

typedef std::vector<const TCHAR*> ParamVector;

bool checkSingleFile(const TCHAR * commandLine) {
	TCHAR fullpath[MAX_PATH];
	::GetFullPathName(commandLine, MAX_PATH, fullpath, NULL);
	if (::PathFileExists(fullpath)) {
		return true;
	}

	return false;
}

void parseCommandLine(TCHAR * commandLine, ParamVector & paramVector) {
	bool isFile = checkSingleFile(commandLine);	//if the commandline specifies only a file, open it as such
	if (isFile) {
		paramVector.push_back(commandLine);
		return;
	}
	bool isInFile = false;
	bool isInWhiteSpace = true;
	paramVector.clear();
	size_t commandLength = lstrlen(commandLine);
	for(size_t i = 0; i < commandLength; i++) {
		switch(commandLine[i]) {
			case '\"': {										//quoted filename, ignore any following whitespace
				if (!isInFile) {	//" will always be treated as start or end of param, in case the user forgot to add an space
					paramVector.push_back(commandLine+i+1);	//add next param(since zero terminated basic_string<TCHAR> original, no overflow of +1)
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
			default: {											//default TCHAR, if beginning of word, add it
				if (!isInFile && isInWhiteSpace) {
					paramVector.push_back(commandLine+i);	//add next param 
					isInWhiteSpace = false;
				}
				break; }
		}
	}
	//the commandline basic_string<TCHAR> is now a list of zero terminated strings concatenated, and the vector contains all the substrings
}

bool isInList(const TCHAR *token2Find, ParamVector & params) {
	int nrItems = params.size();

	for (int i = 0; i < nrItems; i++)
	{
		if (!lstrcmp(token2Find, params.at(i))) {
			params.erase(params.begin() + i);
			return true;
		}
	}
	return false;
};

bool getParamVal(TCHAR c, ParamVector & params, basic_string<TCHAR> & value) {
	value = TEXT("");
	int nrItems = params.size();

	for (int i = 0; i < nrItems; i++)
	{
		const TCHAR * token = params.at(i);
		if (token[0] == '-' && lstrlen(token) >= 2 && token[1] == c) {	//dash, and enough chars
			value = (token+2);
			params.erase(params.begin() + i);
			return true;
		}
	}
	return false;
}

LangType getLangTypeFromParam(ParamVector & params) {
	basic_string<TCHAR> langStr;
	if (!getParamVal('l', params, langStr))
		return L_EXTERNAL;
	return NppParameters::getLangIDFromStr(langStr.c_str());
};

int getLn2GoFromParam(ParamVector & params) {
	basic_string<TCHAR> lineNumStr;
	if (!getParamVal('n', params, lineNumStr))
		return -1;
	return generic_atoi(lineNumStr.c_str());
};

const TCHAR FLAG_MULTI_INSTANCE[] = TEXT("-multiInst");
const TCHAR FLAG_NO_PLUGIN[] = TEXT("-noPlugin");
const TCHAR FLAG_READONLY[] = TEXT("-ro");
const TCHAR FLAG_NOSESSION[] = TEXT("-nosession");
const TCHAR FLAG_NOTABBAR[] = TEXT("-notabbar");

void doException(Notepad_plus & notepad_plus_plus);

int WINAPI NppMainEntry(HINSTANCE hInstance, HINSTANCE, TCHAR * cmdLine, int nCmdShow)
{
	bool TheFirstOne = true;
	::SetLastError(NO_ERROR);
	::CreateMutex(NULL, false, TEXT("nppInstance"));
	if (::GetLastError() == ERROR_ALREADY_EXISTS)
		TheFirstOne = false;

	ParamVector params;
	parseCommandLine(cmdLine, params);
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

	basic_string<TCHAR> quotFileName = TEXT("");
    // tell the running instance the FULL path to the new files to load
	size_t nrFilesToOpen = params.size();
	const TCHAR * currentFile;
	TCHAR fullFileName[MAX_PATH];

	//TODO: try merging the flenames and see if it exists, user may have typed a single spaced filename without quotes
	for(size_t i = 0; i < nrFilesToOpen; i++)
	{
		currentFile = params.at(i);
		//check if relative or full path. Relative paths dont have a colon for driveletter
		BOOL isRelative = ::PathIsRelative(currentFile);
		quotFileName += TEXT("\"");
		if (isRelative)
		{
			::GetFullPathName(currentFile, MAX_PATH, fullFileName, NULL);
			quotFileName += fullFileName;
		}
		else
		{
			quotFileName += currentFile;
		}
		quotFileName += TEXT("\" ");
	}

	if ((!isMultiInst) && (!TheFirstOne))
	{
		HWND hNotepad_plus = ::FindWindow(Notepad_plus::getClassName(), NULL);
		for (;!(hNotepad_plus = ::FindWindow(Notepad_plus::getClassName(), NULL));)
			Sleep(100);

		// First of all, destroy static object NppParameters
		pNppParameters->destroyInstance();
		MainFileManager->destroyInstance();

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
			fileNamesData.cbData = long(quotFileName.length() + 1)*(sizeof(TCHAR));

			::SendMessage(hNotepad_plus, WM_COPYDATA, (WPARAM)hInstance, (LPARAM)&paramData);
			::SendMessage(hNotepad_plus, WM_COPYDATA, (WPARAM)hInstance, (LPARAM)&fileNamesData);
		}
		return 0;
	}

	pNppParameters->load();
	Notepad_plus notepad_plus_plus;
	
	NppGUI & nppGui = (NppGUI &)pNppParameters->getNppGUI();

	basic_string<TCHAR> updaterDir = pNppParameters->getNppPath();
	updaterDir += TEXT("\\updater\\");

	basic_string<TCHAR> updaterFullPath = updaterDir + TEXT("gup.exe");
 
	basic_string<TCHAR> version = TEXT("-v");
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
	Win32Exception::installHandler();
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
						::TranslateMessage(&msg);
						if (unicodeSupported)
							::DispatchMessageW(&msg);
						else
							::DispatchMessage(&msg);
					}	
				}
			}
		}
	} catch(int i) {
		if (i == 106901)
			::MessageBox(NULL, TEXT("Scintilla.init is failed!"), TEXT("Notepad++ Exception: 106901"), MB_OK);
		else {
			TCHAR str[50] = TEXT("God Damned Exception : ");
			TCHAR code[10];
			wsprintf(code, TEXT("%d"), i);
			::MessageBox(NULL, lstrcat(str, code), TEXT("Notepad++ Exception"), MB_OK);
		}
		doException(notepad_plus_plus);
	} catch (const Win32Exception & ex) {
		TCHAR message[1024];	//TODO: sane number
		wsprintf(message, TEXT("An exception occured. Notepad++ cannot recover and must be shut down.\r\nThe exception details are as follows:\r\n")
			TEXT("Code:\t0x%08X\r\nType:\t%s\r\nException address: 0x%08X"),
			ex.code(), ex.what(), ex.where());
		::MessageBox(NULL, message, TEXT("Win32Exception"), MB_OK | MB_ICONERROR);
		doException(notepad_plus_plus);
	} catch(std::exception ex) {
		::MessageBoxA(NULL, ex.what(), "C++ Exception", MB_OK);
		doException(notepad_plus_plus);
	} catch(...) {	//this shouldnt ever have to happen
		doException(notepad_plus_plus);
	}

	return (UINT)msg.wParam;
}

void doException(Notepad_plus & notepad_plus_plus) {
	Win32Exception::removeHandler();	//disable exception handler after excpetion, we dont want corrupt data structurs to crash the exception handler
	::MessageBox(NULL, TEXT("Notepad++ will attempt to save any unsaved data. However, dataloss is very likely."), TEXT("Recovery initiating"), MB_OK | MB_ICONINFORMATION);
	bool res = notepad_plus_plus.emergency();
	if (res) {
		::MessageBox(NULL, TEXT("Notepad++ was able to successfully recover some unsaved documents, or nothing to be saved could be found.\r\nYou can find the results at C:\\N++RECOV"), TEXT("Recovery success"), MB_OK | MB_ICONINFORMATION);
	} else {
		::MessageBox(NULL, TEXT("Unfortunatly, Notepad++ was not able to save your work. We are sorry for any lost data."), TEXT("Recovery failure"), MB_OK | MB_ICONERROR);
	}
}
