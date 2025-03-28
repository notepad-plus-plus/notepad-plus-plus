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
#include "dpiManagerV2.h"
#include <memory>

typedef std::vector<std::wstring> ParamVector;


namespace
{


void allowPrivilegeMessages(const Notepad_plus_Window& notepad_plus_plus, winVer winVer)
{
	#ifndef MSGFLT_ADD
	const DWORD MSGFLT_ADD = 1;
	#endif
	#ifndef MSGFLT_ALLOW
	const DWORD MSGFLT_ALLOW = 1;
	#endif
	// Tell UAC that lower integrity processes are allowed to send WM_COPYDATA (or other) messages to this process (or window)
	// This (WM_COPYDATA) allows opening new files to already opened elevated Notepad++ process via explorer context menu.
	if (winVer >= WV_VISTA || winVer == WV_UNKNOWN)
	{
		HMODULE hDll = GetModuleHandle(L"user32.dll");
		if (hDll)
		{
			// According to MSDN ChangeWindowMessageFilter may not be supported in future versions of Windows,
			// that is why we use ChangeWindowMessageFilterEx if it is available (windows version >= Win7).
			if (winVer == WV_VISTA)
			{
				typedef BOOL (WINAPI *MESSAGEFILTERFUNC)(UINT message,DWORD dwFlag);

				MESSAGEFILTERFUNC func = (MESSAGEFILTERFUNC)::GetProcAddress( hDll, "ChangeWindowMessageFilter" );

				if (func)
				{
					func(WM_COPYDATA, MSGFLT_ADD);
					func(NPPM_INTERNAL_RESTOREFROMTRAY, MSGFLT_ADD);
				}
			}
			else
			{
				typedef BOOL (WINAPI *MESSAGEFILTERFUNCEX)(HWND hWnd,UINT message,DWORD action,VOID* pChangeFilterStruct);

				MESSAGEFILTERFUNCEX funcEx = (MESSAGEFILTERFUNCEX)::GetProcAddress( hDll, "ChangeWindowMessageFilterEx" );

				if (funcEx)
				{
					funcEx(notepad_plus_plus.getHSelf(), WM_COPYDATA, MSGFLT_ALLOW, NULL);
					funcEx(notepad_plus_plus.getHSelf(), NPPM_INTERNAL_RESTOREFROMTRAY, MSGFLT_ALLOW, NULL);
				}
			}
		}
	}
}

// parseCommandLine() takes command line arguments part string, cuts arguments by using white space as separater.
// Only white space in double quotes will be kept, such as file path argument or '-settingsDir=' argument (ex.: -settingsDir="c:\my settings\my folder\")
// if '-z' is present, the 3rd argument after -z wont be cut - ie. all the space will also be kept
// ex.: '-notepadStyleCmdline -z "C:\WINDOWS\system32\NOTEPAD.EXE" C:\my folder\my file with whitespace.txt' will be separated to: 
// 1. "-notepadStyleCmdline"
// 2. "-z"
// 3. "C:\WINDOWS\system32\NOTEPAD.EXE"
// 4. "C:\my folder\my file with whitespace.txt" 
void parseCommandLine(const wchar_t* commandLine, ParamVector& paramVector)
{
	if (!commandLine)
		return;
	
	wchar_t* cmdLine = new wchar_t[lstrlen(commandLine) + 1];
	lstrcpy(cmdLine, commandLine);

	wchar_t* cmdLinePtr = cmdLine;

	bool isBetweenFileNameQuotes = false;
	bool isStringInArg = false;
	bool isInWhiteSpace = true;

	int zArg = 0; // for "-z" argument: Causes Notepad++ to ignore the next command line argument (a single word, or a phrase in quotes).
	              // The only intended and supported use for this option is for the Notepad Replacement syntax.

	bool shouldBeTerminated = false; // If "-z" argument has been found, zArg value will be increased from 0 to 1.
	                                 // then after processing next argument of "-z", zArg value will be increased from 1 to 2.
	                                 // when zArg == 2 shouldBeTerminated will be set to true - it will trigger the treatment which consider the rest as a argument, with or without white space(s).

	size_t commandLength = lstrlen(cmdLinePtr);
	std::vector<wchar_t *> args;
	for (size_t i = 0; i < commandLength && !shouldBeTerminated; ++i)
	{
		switch (cmdLinePtr[i])
		{
			case '\"': //quoted filename, ignore any following whitespace
			{
				if (!isStringInArg && !isBetweenFileNameQuotes && i > 0 && cmdLinePtr[i-1] == '=')
				{
					isStringInArg = true;
				}
				else if (isStringInArg)
				{
					isStringInArg = false;
				}
				else if (!isBetweenFileNameQuotes)	//" will always be treated as start or end of param, in case the user forgot to add an space
				{
					args.push_back(cmdLinePtr + i + 1);	//add next param(since zero terminated original, no overflow of +1)
					isBetweenFileNameQuotes = true;
					cmdLinePtr[i] = 0;

					if (zArg == 1)
					{
						++zArg; // zArg == 2
					}
				}
				else //if (isBetweenFileNameQuotes)
				{
					isBetweenFileNameQuotes = false;
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
				if (!isBetweenFileNameQuotes && !isStringInArg)
				{
					cmdLinePtr[i] = 0;		//zap spaces into zero terminators, unless its part of a filename

					size_t argsLen = args.size();
					if (argsLen > 0 && lstrcmp(args[argsLen-1], L"-z") == 0)
						++zArg; // "-z" argument is found: change zArg value from 0 (initial) to 1
				}
			}
			break;

			default: //default wchar_t, if beginning of word, add it
			{
				if (!isBetweenFileNameQuotes && !isStringInArg && isInWhiteSpace)
				{
					args.push_back(cmdLinePtr + i);	//add next param
					if (zArg == 2)
					{
						shouldBeTerminated = true; // stop the processing, and keep the rest string as it in the vector
					}

					isInWhiteSpace = false;
				}
			}
		}
	}
	paramVector.assign(args.begin(), args.end());
	delete[] cmdLine;
}

// Converts /p or /P to -quickPrint if it exists as the first parameter
// This seems to mirror Notepad's behaviour
void convertParamsToNotepadStyle(ParamVector& params)
{
	for (auto it = params.begin(); it != params.end(); ++it)
	{
		if (lstrcmp(it->c_str(), L"/p") == 0 || lstrcmp(it->c_str(), L"/P") == 0)
		{
			it->assign(L"-quickPrint");
		}
	}
}

bool isInList(const wchar_t *token2Find, ParamVector& params, bool eraseArg = true)
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
}

bool getParamVal(wchar_t c, ParamVector & params, std::wstring & value)
{
	value = L"";
	size_t nbItems = params.size();

	for (size_t i = 0; i < nbItems; ++i)
	{
		const wchar_t * token = params.at(i).c_str();
		if (token[0] == '-' && lstrlen(token) >= 2 && token[1] == c) //dash, and enough chars
		{
			value = (token+2);
			params.erase(params.begin() + i);
			return true;
		}
	}
	return false;
}

bool getParamValFromString(const wchar_t *str, ParamVector & params, std::wstring & value)
{
	value = L"";
	size_t nbItems = params.size();

	for (size_t i = 0; i < nbItems; ++i)
	{
		const wchar_t * token = params.at(i).c_str();
		std::wstring tokenStr = token;
		size_t pos = tokenStr.find(str);
		if (pos != std::wstring::npos && pos == 0)
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
	std::wstring langStr;
	if (!getParamVal('l', params, langStr))
		return L_EXTERNAL;
	return NppParameters::getLangIDFromStr(langStr.c_str());
}

std::wstring getLocalizationPathFromParam(ParamVector & params)
{
	std::wstring locStr;
	if (!getParamVal('L', params, locStr))
		return L"";
	locStr = stringToLower(stringReplace(locStr, L"_", L"-")); // convert to lowercase format with "-" as separator
	return NppParameters::getLocPathFromStr(locStr);
}

intptr_t getNumberFromParam(char paramName, ParamVector & params, bool & isParamePresent)
{
	std::wstring numStr;
	if (!getParamVal(paramName, params, numStr))
	{
		isParamePresent = false;
		return -1;
	}
	isParamePresent = true;
	return static_cast<intptr_t>(_ttoi64(numStr.c_str()));
}

std::wstring getEasterEggNameFromParam(ParamVector & params, unsigned char & type)
{
	std::wstring EasterEggName;
	if (!getParamValFromString(L"-qn=", params, EasterEggName))  // get internal easter egg
	{
		if (!getParamValFromString(L"-qt=", params, EasterEggName)) // get user quote from cmdline argument
		{
			if (!getParamValFromString(L"-qf=", params, EasterEggName)) // get user quote from a content of file
				return L"";
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
	std::wstring speedStr;
	if (!getParamValFromString(L"-qSpeed", params, speedStr))
		return -1;
	
	int speed = std::stoi(speedStr, 0);
	if (speed <= 0 || speed > 3)
		return -1;

	return speed;
}

const wchar_t FLAG_MULTI_INSTANCE[] = L"-multiInst";
const wchar_t FLAG_NO_PLUGIN[] = L"-noPlugin";
const wchar_t FLAG_READONLY[] = L"-ro";
const wchar_t FLAG_NOSESSION[] = L"-nosession";
const wchar_t FLAG_NOTABBAR[] = L"-notabbar";
const wchar_t FLAG_SYSTRAY[] = L"-systemtray";
const wchar_t FLAG_LOADINGTIME[] = L"-loadingTime";
const wchar_t FLAG_HELP[] = L"--help";
const wchar_t FLAG_ALWAYS_ON_TOP[] = L"-alwaysOnTop";
const wchar_t FLAG_OPENSESSIONFILE[] = L"-openSession";
const wchar_t FLAG_RECURSIVE[] = L"-r";
const wchar_t FLAG_FUNCLSTEXPORT[] = L"-export=functionList";
const wchar_t FLAG_PRINTANDQUIT[] = L"-quickPrint";
const wchar_t FLAG_NOTEPAD_COMPATIBILITY[] = L"-notepadStyleCmdline";
const wchar_t FLAG_OPEN_FOLDERS_AS_WORKSPACE[] = L"-openFoldersAsWorkspace";
const wchar_t FLAG_SETTINGS_DIR[] = L"-settingsDir=";
const wchar_t FLAG_TITLEBAR_ADD[] = L"-titleAdd=";
const wchar_t FLAG_APPLY_UDL[] = L"-udl=";
const wchar_t FLAG_PLUGIN_MESSAGE[] = L"-pluginMessage=";
const wchar_t FLAG_MONITOR_FILES[] = L"-monitor";

void doException(Notepad_plus_Window & notepad_plus_plus)
{
	Win32Exception::removeHandler();	//disable exception handler after excpetion, we dont want corrupt data structurs to crash the exception handler
	::MessageBox(Notepad_plus_Window::gNppHWND, L"Notepad++ will attempt to save any unsaved data. However, dataloss is very likely.", L"Recovery initiating", MB_OK | MB_ICONINFORMATION);

	wchar_t tmpDir[1024];
	GetTempPath(1024, tmpDir);
	std::wstring emergencySavedDir = tmpDir;
	emergencySavedDir += L"\\Notepad++ RECOV";

	bool res = notepad_plus_plus.emergency(emergencySavedDir);
	if (res)
	{
		std::wstring displayText = L"Notepad++ was able to successfully recover some unsaved documents, or nothing to be saved could be found.\r\nYou can find the results at :\r\n";
		displayText += emergencySavedDir;
		::MessageBox(Notepad_plus_Window::gNppHWND, displayText.c_str(), L"Recovery success", MB_OK | MB_ICONINFORMATION);
	}
	else
		::MessageBox(Notepad_plus_Window::gNppHWND, L"Unfortunatly, Notepad++ was not able to save your work. We are sorry for any lost data.", L"Recovery failure", MB_OK | MB_ICONERROR);
}

// Looks for -z arguments and strips command line arguments following those, if any
void stripIgnoredParams(ParamVector & params)
{
	for (auto it = params.begin(); it != params.end(); )
	{
		if (lstrcmp(it->c_str(), L"-z") == 0)
		{
			auto nextIt = std::next(it);
			if ( nextIt != params.end() )
			{
				params.erase(nextIt);
			}
			it = params.erase(it);
		}
		else
		{
			++it;
		}
	}
}

} // namespace


std::chrono::steady_clock::time_point g_nppStartTimePoint{};


int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ PWSTR pCmdLine, _In_ int /*nShowCmd*/)
{
	g_nppStartTimePoint = std::chrono::steady_clock::now();

	bool TheFirstOne = true;
	::SetLastError(NO_ERROR);
	::CreateMutex(NULL, false, L"nppInstance");
	if (::GetLastError() == ERROR_ALREADY_EXISTS)
		TheFirstOne = false;

	std::wstring cmdLineString = pCmdLine ? pCmdLine : L"";
	ParamVector params;
	parseCommandLine(pCmdLine, params);


	// Convert commandline to notepad-compatible format, if applicable
	// For treating "-notepadStyleCmdline" "/P" and "-z"
	stripIgnoredParams(params);
	if ( isInList(FLAG_NOTEPAD_COMPATIBILITY, params) )
	{
		convertParamsToNotepadStyle(params);
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

	std::wstring pluginMessage;
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

	std::wstring path;
	if (getParamValFromString(FLAG_SETTINGS_DIR, params, path))
	{
		// path could contain double quotes if path contains white space
		if (path.c_str()[0] == '"' && path.c_str()[path.length() - 1] == '"')
		{
			path = path.substr(1, path.length() - 2);
		}
		nppParameters.setCmdSettingsDir(path);
	}

	std::wstring titleBarAdditional;
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

	std::wstring udlName;
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
		::MessageBox(NULL, COMMAND_ARG_HELP, L"Notepad++ Command Argument Help", MB_OK);

	if (cmdLineParams._localizationPath != L"")
	{
		// setStartWithLocFileName() should be called before parameters are loaded
		nppParameters.setStartWithLocFileName(cmdLineParams._localizationPath);
	}

	nppParameters.load();

	NppGUI & nppGui = nppParameters.getNppGUI();

	NppDarkMode::initDarkMode();
	DPIManagerV2::initDpiAPI();

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

	std::wstring quotFileName = L"";
    // tell the running instance the FULL path to the new files to load
	size_t nbFilesToOpen = params.size();

	for (size_t i = 0; i < nbFilesToOpen; ++i)
	{
		const wchar_t * currentFile = params.at(i).c_str();
		if (currentFile[0])
		{
			//check if relative or full path. Relative paths dont have a colon for driveletter

			quotFileName += L"\"";
			quotFileName += relativeFilePathToFullFilePath(currentFile);
			quotFileName += L"\" ";
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

			// Restore the window, bring it to front, etc
			bool isInSystemTray = ::SendMessage(hNotepad_plus, NPPM_INTERNAL_RESTOREFROMTRAY, 0, 0);

			if (!isInSystemTray)
			{
				int sw = 0;

				if (::IsZoomed(hNotepad_plus))
					sw = SW_MAXIMIZE;
				else if (::IsIconic(hNotepad_plus))
					sw = SW_RESTORE;

				if (sw != 0)
					::ShowWindow(hNotepad_plus, sw);
			}
			::SetForegroundWindow(hNotepad_plus);

			if (params.size() > 0                         // if there are files to open, use the WM_COPYDATA system
				|| !cmdLineParams._pluginMessage.empty()) // or pluginMessage is present, use the WM_COPYDATA system as well
			{
				CmdLineParamsDTO dto = CmdLineParamsDTO::FromCmdLineParams(cmdLineParams);

				COPYDATASTRUCT paramData{};
				paramData.dwData = COPYDATA_PARAMS;
				paramData.lpData = &dto;
				paramData.cbData = sizeof(dto);
				::SendMessage(hNotepad_plus, WM_COPYDATA, reinterpret_cast<WPARAM>(hInstance), reinterpret_cast<LPARAM>(&paramData));

				COPYDATASTRUCT cmdLineData{};
				cmdLineData.dwData = COPYDATA_FULL_CMDLINE;
				cmdLineData.lpData = (void*)cmdLineString.c_str();
				cmdLineData.cbData = static_cast<DWORD>((cmdLineString.length() + 1) * sizeof(wchar_t));
				::SendMessage(hNotepad_plus, WM_COPYDATA, reinterpret_cast<WPARAM>(hInstance), reinterpret_cast<LPARAM>(&cmdLineData));

				COPYDATASTRUCT fileNamesData{};
				fileNamesData.dwData = COPYDATA_FILENAMESW;
				fileNamesData.lpData = (void *)quotFileName.c_str();
				fileNamesData.cbData = static_cast<DWORD>((quotFileName.length() + 1) * sizeof(wchar_t));
				::SendMessage(hNotepad_plus, WM_COPYDATA, reinterpret_cast<WPARAM>(hInstance), reinterpret_cast<LPARAM>(&fileNamesData));
			}
			return 0;
        }
	}

	auto upNotepadWindow = std::make_unique<Notepad_plus_Window>();
	Notepad_plus_Window & notepad_plus_plus = *upNotepadWindow.get();

	std::wstring updaterDir = nppParameters.getNppPath();
	updaterDir += L"\\updater\\";

	std::wstring updaterFullPath = updaterDir + L"gup.exe";

	std::wstring updaterParams = L"-v";
	updaterParams += VERSION_INTERNAL_VALUE;

	bool isUpExist = nppGui._doesExistUpdater = doesFileExist(updaterFullPath.c_str());

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
			updaterParams += L" -px64";
		}
		else if (nppParameters.archType() == IMAGE_FILE_MACHINE_ARM64)
		{
			updaterParams += L" -parm64";
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
			std::wstring upPlParams = L"-v"; 
			upPlParams += notepad_plus_plus.getPluginListVerStr();

			if (nppParameters.archType() == IMAGE_FILE_MACHINE_AMD64)
			{
				upPlParams += L" -px64";
			}
			else if (nppParameters.archType() == IMAGE_FILE_MACHINE_ARM64)
			{
				upPlParams += L" -parm64";
			}

			upPlParams += L" -upZip";

			// overrided "InfoUrl" in gup.xml
			upPlParams += L" https://notepad-plus-plus.org/update/pluginListDownloadUrl.php";

			// indicate the pluginList installation location
			upPlParams += nppParameters.getPluginConfDir();

			Process updater(updaterFullPath.c_str(), upPlParams.c_str(), updaterDir.c_str());
			updater.run();

			// TODO: Update next update date

		}
	}

	MSG msg{};
	msg.wParam = 0;
	Win32Exception::installHandler();
	MiniDumper mdump;	//for debugging purposes.
	try
	{
		notepad_plus_plus.init(hInstance, NULL, quotFileName.c_str(), &cmdLineParams);
		allowPrivilegeMessages(notepad_plus_plus, ver);
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
		wchar_t str[50] = L"God Damned Exception:";
		wchar_t code[10];
		wsprintf(code, L"%d", i);
		wcscat_s(str, code);
		::MessageBox(Notepad_plus_Window::gNppHWND, str, L"Int Exception", MB_OK);
		doException(notepad_plus_plus);
	}
	catch (std::runtime_error & ex)
	{
		::MessageBoxA(Notepad_plus_Window::gNppHWND, ex.what(), "Runtime Exception", MB_OK);
		doException(notepad_plus_plus);
	}
	catch (const Win32Exception & ex)
	{
		wchar_t message[1024];	//TODO: sane number
		wsprintf(message, L"An exception occured. Notepad++ cannot recover and must be shut down.\r\nThe exception details are as follows:\r\n"
			L"Code:\t0x%08X\r\nType:\t%S\r\nException address: 0x%p", ex.code(), ex.what(), ex.where());
		::MessageBox(Notepad_plus_Window::gNppHWND, message, L"Win32Exception", MB_OK | MB_ICONERROR);
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
