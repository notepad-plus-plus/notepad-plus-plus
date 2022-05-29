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

#include "Notepad_plus_Window.h"
#include "Processus.h"
#include "Win32Exception.h"	//Win32 exception
#include "MiniDumper.h"			//Write dump files
#include "verifySignedfile.h"
#include "NppDarkMode.h"
#include <memory>

typedef std::vector<generic_string> ParamVector;


namespace
{


void allowWmCopydataMessages(Notepad_plus_Window& notepad_plus_plus, const NppParameters& nppParameters, winVer ver)
{
	#ifndef MSGFLT_ADD
	const DWORD MSGFLT_ADD = 1;
	#endif
	#ifndef MSGFLT_ALLOW
	const DWORD MSGFLT_ALLOW = 1;
	#endif
	// Tell UAC that lower integrity processes are allowed to send WM_COPYDATA messages to this process (or window)
	// This allows opening new files to already opened elevated Notepad++ process via explorer context menu.
	if (ver >= WV_VISTA || ver == WV_UNKNOWN)
	{
		HMODULE hDll = GetModuleHandle(TEXT("user32.dll"));
		if (hDll)
		{
			// According to MSDN ChangeWindowMessageFilter may not be supported in future versions of Windows,
			// that is why we use ChangeWindowMessageFilterEx if it is available (windows version >= Win7).
			if (nppParameters.getWinVersion() == WV_VISTA)
			{
				typedef BOOL (WINAPI *MESSAGEFILTERFUNC)(UINT message,DWORD dwFlag);

				MESSAGEFILTERFUNC func = (MESSAGEFILTERFUNC)::GetProcAddress( hDll, "ChangeWindowMessageFilter" );

				if (func)
					func(WM_COPYDATA, MSGFLT_ADD);
			}
			else
			{
				typedef BOOL (WINAPI *MESSAGEFILTERFUNCEX)(HWND hWnd,UINT message,DWORD action,VOID* pChangeFilterStruct);

				MESSAGEFILTERFUNCEX func = (MESSAGEFILTERFUNCEX)::GetProcAddress( hDll, "ChangeWindowMessageFilterEx" );

				if (func)
					func(notepad_plus_plus.getHSelf(), WM_COPYDATA, MSGFLT_ALLOW, NULL );
			}
		}
	}
}

// parseCommandLine() takes command line arguments part string, cuts arguments by using white space as separater.
// Only white space in double quotes will be kept, such as file path argument or "-settingsDir=" argument (ex.: -settingsDir="c:\my settings\my folder\")
void parseCommandLine(const TCHAR* commandLine, ParamVector& paramVector)
{
	if (!commandLine)
		return;
	
	TCHAR* cmdLine = new TCHAR[lstrlen(commandLine) + 1];
	lstrcpy(cmdLine, commandLine);

	TCHAR* cmdLinePtr = cmdLine;

	bool isInFile = false;
	bool isStringInArg = false;
	bool isInWhiteSpace = true;

	size_t commandLength = lstrlen(cmdLinePtr);
	std::vector<TCHAR *> args;
	for (size_t i = 0; i < commandLength; ++i)
	{
		switch (cmdLinePtr[i])
		{
			case '\"': //quoted filename, ignore any following whitespace
			{
				if (!isStringInArg && i > 0 && cmdLinePtr[i - 1] == '=')
				{
					isStringInArg = true;
				}
				else if (isStringInArg)
				{
					isStringInArg = false;
					//cmdLinePtr[i] = 0;
				}
				else if (!isInFile)	//" will always be treated as start or end of param, in case the user forgot to add an space
				{
					args.push_back(cmdLinePtr + i + 1);	//add next param(since zero terminated original, no overflow of +1)
					isInFile = true;
					cmdLinePtr[i] = 0;
				}
				else if (isInFile)
				{
					isInFile = false;
					//because we dont want to leave in any quotes in the filename, remove them now (with zero terminator)
					cmdLinePtr[i] = 0;
				}
				isInWhiteSpace = false;
			}
			break;

			case '\t': //also treat tab as whitespace
			case ' ':
			{
				isInWhiteSpace = true;
				if (!isInFile && !isStringInArg)
					cmdLinePtr[i] = 0;		//zap spaces into zero terminators, unless its part of a filename	
			}
			break;

			default: //default TCHAR, if beginning of word, add it
			{
				if (!isInFile && !isStringInArg && isInWhiteSpace)
				{
					args.push_back(cmdLinePtr + i);	//add next param
					isInWhiteSpace = false;
				}
			}
		}
	}
	paramVector.assign(args.begin(), args.end());
	delete[] cmdLine;
}

// 1. Converts /p to -quickPrint if it exists as the first parameter
// 2. Concatenates all remaining parameters to form a file path, adding appending .txt extension if necessary
// This seems to mirror Notepad's behaviour
ParamVector convertParamsToNotepadStyle(PWSTR pCmdLine)
{
	ParamVector params;
	if ( _tcsnicmp(TEXT("/p"), pCmdLine, 2) == 0 ) // Notepad accepts both /p and /P, so compare case insensitively
	{
		params.emplace_back(TEXT("-quickPrint"));
		pCmdLine += 2; // Length of "/p"
	}

	// Advance to the first non-whitespace character
	while ( iswspace( *pCmdLine ) )
	{
		++pCmdLine;
	}

	// Now form a file name from the remaining commandline (if any is left)
	if ( *pCmdLine != '\0' )
	{
		generic_string str(pCmdLine);
		if ( *PathFindExtension(str.c_str()) == '\0' )
		{
			str.append(TEXT(".txt")); // If joined path has no extension, Notepad adds a .txt extension
		}
		params.push_back(std::move(str));
	}
	return params;
}

bool isInList(const TCHAR *token2Find, ParamVector& params, bool eraseArg = true)
{
	for (auto it = params.begin(); it != params.end(); ++it)
	{
		if (lstrcmp(token2Find, it->c_str()) == 0)
		{
			if (eraseArg) params.erase(it);
			return true;
		}
	}
	return false;
};

bool getParamVal(TCHAR c, ParamVector & params, generic_string & value)
{
	value = TEXT("");
	size_t nbItems = params.size();

	for (size_t i = 0; i < nbItems; ++i)
	{
		const TCHAR * token = params.at(i).c_str();
		if (token[0] == '-' && lstrlen(token) >= 2 && token[1] == c) //dash, and enough chars
		{
			value = (token+2);
			params.erase(params.begin() + i);
			return true;
		}
	}
	return false;
}

bool getParamValFromString(const TCHAR *str, ParamVector & params, generic_string & value)
{
	value = TEXT("");
	size_t nbItems = params.size();

	for (size_t i = 0; i < nbItems; ++i)
	{
		const TCHAR * token = params.at(i).c_str();
		generic_string tokenStr = token;
		size_t pos = tokenStr.find(str);
		if (pos != generic_string::npos && pos == 0)
		{
			value = (token + lstrlen(str));
			params.erase(params.begin() + i);
			return true;
		}
	}
	return false;
}

LangType getLangTypeFromParam(ParamVector & params)
{
	generic_string langStr;
	if (!getParamVal('l', params, langStr))
		return L_EXTERNAL;
	return NppParameters::getLangIDFromStr(langStr.c_str());
}

generic_string getLocalizationPathFromParam(ParamVector & params)
{
	generic_string locStr;
	if (!getParamVal('L', params, locStr))
		return TEXT("");
	return NppParameters::getLocPathFromStr(locStr.c_str());
}

intptr_t getNumberFromParam(char paramName, ParamVector & params, bool & isParamePresent)
{
	generic_string numStr;
	if (!getParamVal(paramName, params, numStr))
	{
		isParamePresent = false;
		return -1;
	}
	isParamePresent = true;
	return static_cast<intptr_t>(_ttoi64(numStr.c_str()));
};

generic_string getEasterEggNameFromParam(ParamVector & params, unsigned char & type)
{
	generic_string EasterEggName;
	if (!getParamValFromString(TEXT("-qn="), params, EasterEggName))  // get internal easter egg
	{
		if (!getParamValFromString(TEXT("-qt="), params, EasterEggName)) // get user quote from cmdline argument
		{
			if (!getParamValFromString(TEXT("-qf="), params, EasterEggName)) // get user quote from a content of file
				return TEXT("");
			else
			{
				type = 2; // quote content in file
			}
		}
		else
			type = 1; // commandline quote
	}
	else
		type = 0; // easter egg

	if (EasterEggName.c_str()[0] == '"' && EasterEggName.c_str()[EasterEggName.length() - 1] == '"')
	{
		EasterEggName = EasterEggName.substr(1, EasterEggName.length() - 2);
	}

	if (type == 2)
		EasterEggName = relativeFilePathToFullFilePath(EasterEggName.c_str());

	return EasterEggName;
}

int getGhostTypingSpeedFromParam(ParamVector & params)
{
	generic_string speedStr;
	if (!getParamValFromString(TEXT("-qSpeed"), params, speedStr))
		return -1;
	
	int speed = std::stoi(speedStr, 0);
	if (speed <= 0 || speed > 3)
		return -1;

	return speed;
}

const TCHAR FLAG_MULTI_INSTANCE[] = TEXT("-multiInst");
const TCHAR FLAG_NO_PLUGIN[] = TEXT("-noPlugin");
const TCHAR FLAG_READONLY[] = TEXT("-ro");
const TCHAR FLAG_NOSESSION[] = TEXT("-nosession");
const TCHAR FLAG_NOTABBAR[] = TEXT("-notabbar");
const TCHAR FLAG_SYSTRAY[] = TEXT("-systemtray");
const TCHAR FLAG_LOADINGTIME[] = TEXT("-loadingTime");
const TCHAR FLAG_HELP[] = TEXT("--help");
const TCHAR FLAG_ALWAYS_ON_TOP[] = TEXT("-alwaysOnTop");
const TCHAR FLAG_OPENSESSIONFILE[] = TEXT("-openSession");
const TCHAR FLAG_RECURSIVE[] = TEXT("-r");
const TCHAR FLAG_FUNCLSTEXPORT[] = TEXT("-export=functionList");
const TCHAR FLAG_PRINTANDQUIT[] = TEXT("-quickPrint");
const TCHAR FLAG_NOTEPAD_COMPATIBILITY[] = TEXT("-notepadStyleCmdline");
const TCHAR FLAG_OPEN_FOLDERS_AS_WORKSPACE[] = TEXT("-openFoldersAsWorkspace");
const TCHAR FLAG_SETTINGS_DIR[] = TEXT("-settingsDir=");
const TCHAR FLAG_TITLEBAR_ADD[] = TEXT("-titleAdd=");
const TCHAR FLAG_APPLY_UDL[] = TEXT("-udl=");
const TCHAR FLAG_PLUGIN_MESSAGE[] = TEXT("-pluginMessage=");
const TCHAR FLAG_MONITOR_FILES[] = TEXT("-monitor");

void doException(Notepad_plus_Window & notepad_plus_plus)
{
	Win32Exception::removeHandler();	//disable exception handler after excpetion, we dont want corrupt data structurs to crash the exception handler
	::MessageBox(Notepad_plus_Window::gNppHWND, TEXT("Notepad++ will attempt to save any unsaved data. However, dataloss is very likely."), TEXT("Recovery initiating"), MB_OK | MB_ICONINFORMATION);

	TCHAR tmpDir[1024];
	GetTempPath(1024, tmpDir);
	generic_string emergencySavedDir = tmpDir;
	emergencySavedDir += TEXT("\\N++RECOV");

	bool res = notepad_plus_plus.emergency(emergencySavedDir);
	if (res)
	{
		generic_string displayText = TEXT("Notepad++ was able to successfully recover some unsaved documents, or nothing to be saved could be found.\r\nYou can find the results at :\r\n");
		displayText += emergencySavedDir;
		::MessageBox(Notepad_plus_Window::gNppHWND, displayText.c_str(), TEXT("Recovery success"), MB_OK | MB_ICONINFORMATION);
	}
	else
		::MessageBox(Notepad_plus_Window::gNppHWND, TEXT("Unfortunatly, Notepad++ was not able to save your work. We are sorry for any lost data."), TEXT("Recovery failure"), MB_OK | MB_ICONERROR);
}

PWSTR advanceCmdLine(PWSTR pCmdLine, const generic_string& string)
{
	const size_t len = string.length();
	while (true)
	{
		PWSTR ignoredString = wcsstr(pCmdLine, string.c_str());
		if (ignoredString == nullptr)
		{
			// Should never happen - tokenized parameters contain string somewhere, so it HAS to match
			// This is there just in case
			break;
		}
	
		// Match the substring only if it matched an entire substring		
		if ((ignoredString == pCmdLine || iswspace(*(ignoredString - 1))) && // Check start
			(iswspace(*(ignoredString + len)) || *(ignoredString + len) == '\0' || *(ignoredString + len) == '"'))
		{
			ignoredString += len;

			// Advance to the first non-whitespace and not quotation mark character
			while ( iswspace( *ignoredString ) || *ignoredString == L'"' )
			{
				++ignoredString;
			}
			pCmdLine = ignoredString;
			break;
		}
		else
		{
			pCmdLine = ignoredString+len; // Just skip this match and resume from another
		}
	}
	return pCmdLine;
}

// Looks for -z arguments and strips command line arguments following those, if any
// Also advances pCmdLine to point after the last ignored parameter
// -notepadStyleCmdline is also considered an ignored parameter here, as we don't want it to be part of the assembled file name
PWSTR stripIgnoredParams(ParamVector & params, PWSTR pCmdLine)
{
	for ( auto it = params.begin(); it != params.end(); )
	{
		if (lstrcmp(it->c_str(), TEXT("-z")) == 0)
		{
			pCmdLine = advanceCmdLine(pCmdLine, *it);

			auto nextIt = std::next(it);
			if ( nextIt != params.end() )
			{
				pCmdLine = advanceCmdLine(pCmdLine, *nextIt);
				params.erase(nextIt);
			}
			it = params.erase(it);
		}
		else if (lstrcmp(it->c_str(), FLAG_NOTEPAD_COMPATIBILITY) == 0)
		{
			pCmdLine = advanceCmdLine(pCmdLine, *it++);
		}
		else
		{
			++it;
		}
	}
	return pCmdLine;
}

} // namespace




int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int)
{
	generic_string cmdLineString = pCmdLine ? pCmdLine : _T("");
	ParamVector params;
	parseCommandLine(pCmdLine, params);
	PWSTR pCmdLineWithoutIgnores = stripIgnoredParams(params, pCmdLine);

	MiniDumper mdump;	//for debugging purposes.

	bool TheFirstOne = true;
	::SetLastError(NO_ERROR);
	::CreateMutex(NULL, false, TEXT("nppInstance"));
	if (::GetLastError() == ERROR_ALREADY_EXISTS)
		TheFirstOne = false;

	// Convert commandline to notepad-compatible format, if applicable
	if ( isInList(FLAG_NOTEPAD_COMPATIBILITY, params) )
	{
		params = convertParamsToNotepadStyle(pCmdLineWithoutIgnores);
	}

	bool isParamePresent;
	bool showHelp = isInList(FLAG_HELP, params);
	bool isMultiInst = isInList(FLAG_MULTI_INSTANCE, params);
	bool doFunctionListExport = isInList(FLAG_FUNCLSTEXPORT, params);
	bool doPrintAndQuit = isInList(FLAG_PRINTANDQUIT, params);

	CmdLineParams cmdLineParams;
	cmdLineParams._isNoTab = isInList(FLAG_NOTABBAR, params);
	cmdLineParams._isNoPlugin = isInList(FLAG_NO_PLUGIN, params);
	cmdLineParams._isReadOnly = isInList(FLAG_READONLY, params);
	cmdLineParams._isNoSession = isInList(FLAG_NOSESSION, params);
	cmdLineParams._isPreLaunch = isInList(FLAG_SYSTRAY, params);
	cmdLineParams._alwaysOnTop = isInList(FLAG_ALWAYS_ON_TOP, params);
	cmdLineParams._showLoadingTime = isInList(FLAG_LOADINGTIME, params);
	cmdLineParams._isSessionFile = isInList(FLAG_OPENSESSIONFILE, params);
	cmdLineParams._isRecursive = isInList(FLAG_RECURSIVE, params);
	cmdLineParams._openFoldersAsWorkspace = isInList(FLAG_OPEN_FOLDERS_AS_WORKSPACE, params);
	cmdLineParams._monitorFiles = isInList(FLAG_MONITOR_FILES, params);

	cmdLineParams._langType = getLangTypeFromParam(params);
	cmdLineParams._localizationPath = getLocalizationPathFromParam(params);
	cmdLineParams._easterEggName = getEasterEggNameFromParam(params, cmdLineParams._quoteType);
	cmdLineParams._ghostTypingSpeed = getGhostTypingSpeedFromParam(params);

	generic_string pluginMessage;
	if (getParamValFromString(FLAG_PLUGIN_MESSAGE, params, pluginMessage))
	{
		if (pluginMessage.length() >= 2)
		{
			if (pluginMessage.front() == '"' && pluginMessage.back() == '"')
			{
				pluginMessage = pluginMessage.substr(1, pluginMessage.length() - 2);
			}
		}
		cmdLineParams._pluginMessage = pluginMessage;
	}

	// getNumberFromParam should be run at the end, to not consuming the other params
	cmdLineParams._line2go = getNumberFromParam('n', params, isParamePresent);
    cmdLineParams._column2go = getNumberFromParam('c', params, isParamePresent);
    cmdLineParams._pos2go = getNumberFromParam('p', params, isParamePresent);
	cmdLineParams._point.x = static_cast<LONG>(getNumberFromParam('x', params, cmdLineParams._isPointXValid));
	cmdLineParams._point.y = static_cast<LONG>(getNumberFromParam('y', params, cmdLineParams._isPointYValid));

	NppParameters& nppParameters = NppParameters::getInstance();

	nppParameters.setCmdLineString(cmdLineString);

	generic_string path;
	if (getParamValFromString(FLAG_SETTINGS_DIR, params, path))
	{
		// path could contain double quotes if path contains white space
		if (path.c_str()[0] == '"' && path.c_str()[path.length() - 1] == '"')
		{
			path = path.substr(1, path.length() - 2);
		}
		nppParameters.setCmdSettingsDir(path);
	}

	generic_string titleBarAdditional;
	if (getParamValFromString(FLAG_TITLEBAR_ADD, params, titleBarAdditional))
	{
		if (titleBarAdditional.length() >= 2)
		{
			if (titleBarAdditional.front() == '"' && titleBarAdditional.back() == '"')
			{
				titleBarAdditional = titleBarAdditional.substr(1, titleBarAdditional.length() - 2);
			}
		}
		nppParameters.setTitleBarAdd(titleBarAdditional);
	}

	generic_string udlName;
	if (getParamValFromString(FLAG_APPLY_UDL, params, udlName))
	{
		if (udlName.length() >= 2)
		{
			if (udlName.front() == '"' && udlName.back() == '"')
			{
				udlName = udlName.substr(1, udlName.length() - 2);
			}
		}
		cmdLineParams._udlName = udlName;
	}

	if (showHelp)
		::MessageBox(NULL, COMMAND_ARG_HELP, TEXT("Notepad++ Command Argument Help"), MB_OK);

	if (cmdLineParams._localizationPath != TEXT(""))
	{
		// setStartWithLocFileName() should be called before parameters are loaded
		nppParameters.setStartWithLocFileName(cmdLineParams._localizationPath);
	}

	nppParameters.load();

	NppGUI & nppGui = nppParameters.getNppGUI();

	NppDarkMode::initDarkMode();

	bool doUpdateNpp = nppGui._autoUpdateOpt._doAutoUpdate;
	bool doUpdatePluginList = nppGui._autoUpdateOpt._doAutoUpdate;

	if (doFunctionListExport || doPrintAndQuit) // export functionlist feature will serialize fuctionlist on the disk, then exit Notepad++. So it's important to not launch into existing instance, and keep it silent.
	{
		isMultiInst = true;
		doUpdateNpp = doUpdatePluginList = false;
		cmdLineParams._isNoSession = true;
	}

	nppParameters.setFunctionListExportBoolean(doFunctionListExport);
	nppParameters.setPrintAndExitBoolean(doPrintAndQuit);

	// override the settings if notepad style is present
	if (nppParameters.asNotepadStyle())
	{
		isMultiInst = true;
		cmdLineParams._isNoTab = true;
		cmdLineParams._isNoSession = true;
	}

	// override the settings if multiInst is choosen by user in the preference dialog
	const NppGUI & nppGUI = nppParameters.getNppGUI();
	if (nppGUI._multiInstSetting == multiInst)
	{
		isMultiInst = true;
		// Only the first launch remembers the session
		if (!TheFirstOne)
			cmdLineParams._isNoSession = true;
	}

	generic_string quotFileName = TEXT("");
    // tell the running instance the FULL path to the new files to load
	size_t nbFilesToOpen = params.size();

	for (size_t i = 0; i < nbFilesToOpen; ++i)
	{
		const TCHAR * currentFile = params.at(i).c_str();
		if (currentFile[0])
		{
			//check if relative or full path. Relative paths dont have a colon for driveletter

			quotFileName += TEXT("\"");
			quotFileName += relativeFilePathToFullFilePath(currentFile);
			quotFileName += TEXT("\" ");
		}
	}

	//Only after loading all the file paths set the working directory
	::SetCurrentDirectory(NppParameters::getInstance().getNppPath().c_str());	//force working directory to path of module, preventing lock

	if ((!isMultiInst) && (!TheFirstOne))
	{
		HWND hNotepad_plus = ::FindWindow(Notepad_plus_Window::getClassName(), NULL);
		for (int i = 0 ;!hNotepad_plus && i < 5 ; ++i)
		{
			Sleep(100);
			hNotepad_plus = ::FindWindow(Notepad_plus_Window::getClassName(), NULL);
		}

        if (hNotepad_plus)
        {
			// First of all, destroy static object NppParameters
			nppParameters.destroyInstance();

			int sw = 0;

			if (::IsZoomed(hNotepad_plus))
				sw = SW_MAXIMIZE;
			else if (::IsIconic(hNotepad_plus))
				sw = SW_RESTORE;

			if (sw != 0)
				::ShowWindow(hNotepad_plus, sw);

			::SetForegroundWindow(hNotepad_plus);

			if (params.size() > 0)	//if there are files to open, use the WM_COPYDATA system
			{
				CmdLineParamsDTO dto = CmdLineParamsDTO::FromCmdLineParams(cmdLineParams);

				COPYDATASTRUCT paramData;
				paramData.dwData = COPYDATA_PARAMS;
				paramData.lpData = &dto;
				paramData.cbData = sizeof(dto);
				::SendMessage(hNotepad_plus, WM_COPYDATA, reinterpret_cast<WPARAM>(hInstance), reinterpret_cast<LPARAM>(&paramData));

				COPYDATASTRUCT cmdLineData;
				cmdLineData.dwData = COPYDATA_FULL_CMDLINE;
				cmdLineData.lpData = (void*)cmdLineString.c_str();
				cmdLineData.cbData = long(cmdLineString.length() + 1) * (sizeof(TCHAR));
				::SendMessage(hNotepad_plus, WM_COPYDATA, reinterpret_cast<WPARAM>(hInstance), reinterpret_cast<LPARAM>(&cmdLineData));

				COPYDATASTRUCT fileNamesData;
				fileNamesData.dwData = COPYDATA_FILENAMES;
				fileNamesData.lpData = (void *)quotFileName.c_str();
				fileNamesData.cbData = long(quotFileName.length() + 1) * (sizeof(TCHAR));
				::SendMessage(hNotepad_plus, WM_COPYDATA, reinterpret_cast<WPARAM>(hInstance), reinterpret_cast<LPARAM>(&fileNamesData));
			}
			return 0;
        }
	}

	auto upNotepadWindow = std::make_unique<Notepad_plus_Window>();
	Notepad_plus_Window & notepad_plus_plus = *upNotepadWindow.get();

	generic_string updaterDir = nppParameters.getNppPath();
	updaterDir += TEXT("\\updater\\");

	generic_string updaterFullPath = updaterDir + TEXT("gup.exe");

	generic_string updaterParams = TEXT("-v");
	updaterParams += VERSION_VALUE;

	bool isUpExist = nppGui._doesExistUpdater = (::PathFileExists(updaterFullPath.c_str()) == TRUE);

    if (doUpdateNpp) // check more detail
    {
        Date today(0);

        if (today < nppGui._autoUpdateOpt._nextUpdateDate)
            doUpdateNpp = false;
    }

	if (doUpdatePluginList)
	{
		// TODO: detect update frequency
	}

	// wingup doesn't work with the obsolet security layer (API) under xp since downloadings are secured with SSL on notepad_plus_plus.org
	winVer ver = nppParameters.getWinVersion();
	bool isGtXP = ver > WV_XP;

	SecurityGuard securityGuard;
	bool isSignatureOK = securityGuard.checkModule(updaterFullPath, nm_gup);

	if (TheFirstOne && isUpExist && isGtXP && isSignatureOK)
	{
		if (nppParameters.archType() == IMAGE_FILE_MACHINE_AMD64)
		{
			updaterParams += TEXT(" -px64");
		}
		else if (nppParameters.archType() == IMAGE_FILE_MACHINE_ARM64)
		{
			updaterParams += TEXT(" -parm64");
		}

		if (doUpdateNpp)
		{
			Process updater(updaterFullPath.c_str(), updaterParams.c_str(), updaterDir.c_str());
			updater.run();

			// Update next update date
			if (nppGui._autoUpdateOpt._intervalDays < 0) // Make sure interval days value is positive
				nppGui._autoUpdateOpt._intervalDays = 0 - nppGui._autoUpdateOpt._intervalDays;
			nppGui._autoUpdateOpt._nextUpdateDate = Date(nppGui._autoUpdateOpt._intervalDays);
		}

		// to be removed
		doUpdatePluginList = false;

		if (doUpdatePluginList)
		{
			// Update Plugin List
			generic_string upPlParams = TEXT("-v"); 
			upPlParams += notepad_plus_plus.getPluginListVerStr();

			if (nppParameters.archType() == IMAGE_FILE_MACHINE_AMD64)
			{
				upPlParams += TEXT(" -px64");
			}
			else if (nppParameters.archType() == IMAGE_FILE_MACHINE_ARM64)
			{
				upPlParams += TEXT(" -parm64");
			}

			upPlParams += TEXT(" -upZip");

			// overrided "InfoUrl" in gup.xml
			upPlParams += TEXT(" https://notepad-plus-plus.org/update/pluginListDownloadUrl.php");

			// indicate the pluginList installation location
			upPlParams += nppParameters.getPluginConfDir();

			Process updater(updaterFullPath.c_str(), upPlParams.c_str(), updaterDir.c_str());
			updater.run();

			// TODO: Update next update date

		}
	}

	MSG msg;
	msg.wParam = 0;
	Win32Exception::installHandler();
	try
	{
		notepad_plus_plus.init(hInstance, NULL, quotFileName.c_str(), &cmdLineParams);
		allowWmCopydataMessages(notepad_plus_plus, nppParameters, ver);
		bool going = true;
		while (going)
		{
			going = ::GetMessageW(&msg, NULL, 0, 0) != 0;
			if (going)
			{
				// if the message doesn't belong to the notepad_plus_plus's dialog
				if (!notepad_plus_plus.isDlgsMsg(&msg))
				{
					if (::TranslateAccelerator(notepad_plus_plus.getHSelf(), notepad_plus_plus.getAccTable(), &msg) == 0)
					{
						::TranslateMessage(&msg);
						::DispatchMessageW(&msg);
					}
				}
			}
		}
	}
	catch (int i)
	{
		TCHAR str[50] = TEXT("God Damned Exception : ");
		TCHAR code[10];
		wsprintf(code, TEXT("%d"), i);
		wcscat_s(str, code);
		::MessageBox(Notepad_plus_Window::gNppHWND, str, TEXT("Int Exception"), MB_OK);
		doException(notepad_plus_plus);
	}
	catch (std::runtime_error & ex)
	{
		::MessageBoxA(Notepad_plus_Window::gNppHWND, ex.what(), "Runtime Exception", MB_OK);
		doException(notepad_plus_plus);
	}
	catch (const Win32Exception & ex)
	{
		TCHAR message[1024];	//TODO: sane number
		wsprintf(message, TEXT("An exception occured. Notepad++ cannot recover and must be shut down.\r\nThe exception details are as follows:\r\n")
			TEXT("Code:\t0x%08X\r\nType:\t%S\r\nException address: 0x%p"), ex.code(), ex.what(), ex.where());
		::MessageBox(Notepad_plus_Window::gNppHWND, message, TEXT("Win32Exception"), MB_OK | MB_ICONERROR);
		mdump.writeDump(ex.info());
		doException(notepad_plus_plus);
	}
	catch (std::exception & ex)
	{
		::MessageBoxA(Notepad_plus_Window::gNppHWND, ex.what(), "General Exception", MB_OK);
		doException(notepad_plus_plus);
	}
	catch (...) // this shouldnt ever have to happen
	{
		::MessageBoxA(Notepad_plus_Window::gNppHWND, "An exception that we did not yet found its name is just caught", "Unknown Exception", MB_OK);
		doException(notepad_plus_plus);
	}

	return static_cast<int>(msg.wParam);
}
