     1|// This file is part of npminmin project
     2|// Copyright (C)2021 Don HO <don.h@free.fr>
     3|
     4|// This program is free software: you can redistribute it and/or modify
     5|// it under the terms of the GNU General Public License as published by
     6|// the Free Software Foundation, either version 3 of the License, or
     7|// at your option any later version.
     8|//
     9|// This program is distributed in the hope that it will be useful,
    10|// but WITHOUT ANY WARRANTY; without even the implied warranty of
    11|// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    12|// GNU General Public License for more details.
    13|//
    14|// You should have received a copy of the GNU General Public License
    15|// along with this program.  If not, see <https://www.gnu.org/licenses/>.
    16|
    17|#include "Notepad_plus_Window.h"
    18|#include "Processus.h"
    19|#include "Win32Exception.h"	//Win32 exception
    20|#include "MiniDumper.h"			//Write dump files
    21|#include "verifySignedfile.h"
    22|#include "NppDarkMode.h"
    23|#include "dpiManagerV2.h"
    24|#include <memory>
    25|
    26|typedef std::vector<std::wstring> ParamVector;
    27|
    28|
    29|namespace
    30|{
    31|
    32|
    33|void allowPrivilegeMessages(const Notepad_plus_Window& notepad_plus_plus, winVer winVer)
    34|{
    35|	#ifndef MSGFLT_ADD
    36|	const DWORD MSGFLT_ADD = 1;
    37|	#endif
    38|	#ifndef MSGFLT_ALLOW
    39|	const DWORD MSGFLT_ALLOW = 1;
    40|	#endif
    41|	// Tell UAC that lower integrity processes are allowed to send WM_COPYDATA (or other) messages to this process (or window)
    42|	// This (WM_COPYDATA) allows opening new files to already opened elevated npminmin process via explorer context menu.
    43|	if (winVer >= WV_VISTA || winVer == WV_UNKNOWN)
    44|	{
    45|		HMODULE hDll = GetModuleHandle(L"user32.dll");
    46|		if (hDll)
    47|		{
    48|			// According to MSDN ChangeWindowMessageFilter may not be supported in future versions of Windows,
    49|			// that is why we use ChangeWindowMessageFilterEx if it is available (windows version >= Win7).
    50|			if (winVer == WV_VISTA)
    51|			{
    52|				typedef BOOL (WINAPI *MESSAGEFILTERFUNC)(UINT message,DWORD dwFlag);
    53|
    54|				MESSAGEFILTERFUNC func = (MESSAGEFILTERFUNC)::GetProcAddress( hDll, "ChangeWindowMessageFilter" );
    55|
    56|				if (func)
    57|				{
    58|					func(WM_COPYDATA, MSGFLT_ADD);
    59|					func(NPPM_INTERNAL_RESTOREFROMTRAY, MSGFLT_ADD);
    60|				}
    61|			}
    62|			else
    63|			{
    64|				typedef BOOL (WINAPI *MESSAGEFILTERFUNCEX)(HWND hWnd,UINT message,DWORD action,VOID* pChangeFilterStruct);
    65|
    66|				MESSAGEFILTERFUNCEX funcEx = (MESSAGEFILTERFUNCEX)::GetProcAddress( hDll, "ChangeWindowMessageFilterEx" );
    67|
    68|				if (funcEx)
    69|				{
    70|					funcEx(notepad_plus_plus.getHSelf(), WM_COPYDATA, MSGFLT_ALLOW, NULL);
    71|					funcEx(notepad_plus_plus.getHSelf(), NPPM_INTERNAL_RESTOREFROMTRAY, MSGFLT_ALLOW, NULL);
    72|				}
    73|			}
    74|		}
    75|	}
    76|}
    77|
    78|// parseCommandLine() takes command line arguments part string, cuts arguments by using white space as separator.
    79|// Only white space in double quotes will be kept, such as file path argument or '-settingsDir=' argument (ex.: -settingsDir="c:\my settings\my folder\")
    80|// if '-z' is present, the 3rd argument after -z won't be cut - ie. all the space will also be kept
    81|// ex.: '-notepadStyleCmdline -z "C:\WINDOWS\system32\NOTEPAD.EXE" C:\my folder\my file with whitespace.txt' will be separated to: 
    82|// 1. "-notepadStyleCmdline"
    83|// 2. "-z"
    84|// 3. "C:\WINDOWS\system32\NOTEPAD.EXE"
    85|// 4. "C:\my folder\my file with whitespace.txt" 
    86|void parseCommandLine(const wchar_t* commandLine, ParamVector& paramVector)
    87|{
    88|	if (!commandLine)
    89|		return;
    90|	
    91|	wchar_t* cmdLine = new wchar_t[lstrlen(commandLine) + 1];
    92|	lstrcpy(cmdLine, commandLine);
    93|
    94|	wchar_t* cmdLinePtr = cmdLine;
    95|
    96|	bool isBetweenFileNameQuotes = false;
    97|	bool isStringInArg = false;
    98|	bool isInWhiteSpace = true;
    99|
   100|	int zArg = 0; // for "-z" argument: Causes npminmin to ignore the next command line argument (a single word, or a phrase in quotes).
   101|	              // The only intended and supported use for this option is for the Notepad Replacement syntax.
   102|
   103|	bool shouldBeTerminated = false; // If "-z" argument has been found, zArg value will be increased from 0 to 1.
   104|	                                 // then after processing next argument of "-z", zArg value will be increased from 1 to 2.
   105|	                                 // when zArg == 2 shouldBeTerminated will be set to true - it will trigger the treatment which consider the rest as a argument, with or without white space(s).
   106|
   107|	size_t commandLength = lstrlen(cmdLinePtr);
   108|	std::vector<wchar_t *> args;
   109|	for (size_t i = 0; i < commandLength && !shouldBeTerminated; ++i)
   110|	{
   111|		switch (cmdLinePtr[i])
   112|		{
   113|			case '\"': //quoted filename, ignore any following whitespace
   114|			{
   115|				if (!isStringInArg && !isBetweenFileNameQuotes && i > 0 && cmdLinePtr[i-1] == '=')
   116|				{
   117|					isStringInArg = true;
   118|				}
   119|				else if (isStringInArg)
   120|				{
   121|					isStringInArg = false;
   122|				}
   123|				else if (!isBetweenFileNameQuotes)	//" will always be treated as start or end of param, in case the user forgot to add an space
   124|				{
   125|					args.push_back(cmdLinePtr + i + 1);	//add next param(since zero terminated original, no overflow of +1)
   126|					isBetweenFileNameQuotes = true;
   127|					cmdLinePtr[i] = 0;
   128|
   129|					if (zArg == 1)
   130|					{
   131|						++zArg; // zArg == 2
   132|					}
   133|				}
   134|				else //if (isBetweenFileNameQuotes)
   135|				{
   136|					isBetweenFileNameQuotes = false;
   137|					//because we don't want to leave in any quotes in the filename, remove them now (with zero terminator)
   138|					cmdLinePtr[i] = 0;
   139|				}
   140|				isInWhiteSpace = false;
   141|			}
   142|			break;
   143|
   144|			case '\t': //also treat tab as whitespace
   145|			case ' ':
   146|			{
   147|				isInWhiteSpace = true;
   148|				if (!isBetweenFileNameQuotes && !isStringInArg)
   149|				{
   150|					cmdLinePtr[i] = 0;		//zap spaces into zero terminators, unless its part of a filename
   151|
   152|					size_t argsLen = args.size();
   153|					if (argsLen > 0 && lstrcmp(args[argsLen-1], L"-z") == 0)
   154|						++zArg; // "-z" argument is found: change zArg value from 0 (initial) to 1
   155|				}
   156|			}
   157|			break;
   158|
   159|			default: //default wchar_t, if beginning of word, add it
   160|			{
   161|				if (!isBetweenFileNameQuotes && !isStringInArg && isInWhiteSpace)
   162|				{
   163|					args.push_back(cmdLinePtr + i);	//add next param
   164|					if (zArg == 2)
   165|					{
   166|						shouldBeTerminated = true; // stop the processing, and keep the rest string as it in the vector
   167|					}
   168|
   169|					isInWhiteSpace = false;
   170|				}
   171|			}
   172|		}
   173|	}
   174|	paramVector.assign(args.begin(), args.end());
   175|	delete[] cmdLine;
   176|}
   177|
   178|// Converts /p or /P to -quickPrint if it exists as the first parameter
   179|// This seems to mirror Notepad's behaviour
   180|void convertParamsToNotepadStyle(ParamVector& params)
   181|{
   182|	for (auto it = params.begin(); it != params.end(); ++it)
   183|	{
   184|		if (lstrcmp(it->c_str(), L"/p") == 0 || lstrcmp(it->c_str(), L"/P") == 0)
   185|		{
   186|			it->assign(L"-quickPrint");
   187|		}
   188|	}
   189|}
   190|
   191|bool isInList(const wchar_t *token2Find, ParamVector& params, bool eraseArg = true)
   192|{
   193|	for (auto it = params.begin(); it != params.end(); ++it)
   194|	{
   195|		if (lstrcmp(token2Find, it->c_str()) == 0)
   196|		{
   197|			if (eraseArg) params.erase(it);
   198|			return true;
   199|		}
   200|	}
   201|	return false;
   202|}
   203|
   204|bool getParamVal(wchar_t c, ParamVector & params, std::wstring & value)
   205|{
   206|	value = L"";
   207|	size_t nbItems = params.size();
   208|
   209|	for (size_t i = 0; i < nbItems; ++i)
   210|	{
   211|		const wchar_t * token=params...r();
   212|		if (token[0] == '-' && lstrlen(token) >= 2 && token[1] == c) //dash, and enough chars
   213|		{
   214|			value = (token+2);
   215|			params.erase(params.begin() + i);
   216|			return true;
   217|		}
   218|	}
   219|	return false;
   220|}
   221|
   222|bool getParamValFromString(const wchar_t *str, ParamVector & params, std::wstring & value)
   223|{
   224|	value = L"";
   225|	size_t nbItems = params.size();
   226|
   227|	for (size_t i = 0; i < nbItems; ++i)
   228|	{
   229|		const wchar_t * token=params...r();
   230|		std::wstring tokenStr=***
   231|		size_t pos = tokenStr.find(str);
   232|		if (pos != std::wstring::npos && pos == 0)
   233|		{
   234|			value = (token + lstrlen(str));
   235|			params.erase(params.begin() + i);
   236|			return true;
   237|		}
   238|	}
   239|	return false;
   240|}
   241|
   242|LangType getLangTypeFromParam(ParamVector & params)
   243|{
   244|	std::wstring langStr;
   245|	if (!getParamVal('l', params, langStr))
   246|		return L_EXTERNAL;
   247|	return NppParameters::getLangIDFromStr(langStr.c_str());
   248|}
   249|
   250|std::wstring getLocalizationPathFromParam(ParamVector & params)
   251|{
   252|	std::wstring locStr;
   253|	if (!getParamVal('L', params, locStr))
   254|		return L"";
   255|	locStr = stringToLower(stringReplace(locStr, L"_", L"-")); // convert to lowercase format with "-" as separator
   256|	return NppParameters::getLocPathFromStr(locStr);
   257|}
   258|
   259|intptr_t getNumberFromParam(char paramName, ParamVector & params, bool & isParamePresent)
   260|{
   261|	std::wstring numStr;
   262|	if (!getParamVal(paramName, params, numStr))
   263|	{
   264|		isParamePresent = false;
   265|		return -1;
   266|	}
   267|	isParamePresent = true;
   268|	return static_cast<intptr_t>(_ttoi64(numStr.c_str()));
   269|}
   270|
   271|std::wstring getEasterEggNameFromParam(ParamVector & params, unsigned char & type)
   272|{
   273|	std::wstring EasterEggName;
   274|	if (!getParamValFromString(L"-qn=", params, EasterEggName))  // get internal easter egg
   275|	{
   276|		if (!getParamValFromString(L"-qt=", params, EasterEggName)) // get user quote from cmdline argument
   277|		{
   278|			if (!getParamValFromString(L"-qf=", params, EasterEggName)) // get user quote from a content of file
   279|				return L"";
   280|			else
   281|			{
   282|				type = 2; // quote content in file
   283|			}
   284|		}
   285|		else
   286|			type = 1; // commandline quote
   287|	}
   288|	else
   289|		type = 0; // easter egg
   290|
   291|	if (EasterEggName.c_str()[0] == '"' && EasterEggName.c_str()[EasterEggName.length() - 1] == '"')
   292|	{
   293|		EasterEggName = EasterEggName.substr(1, EasterEggName.length() - 2);
   294|	}
   295|
   296|	if (type == 2)
   297|		EasterEggName = relativeFilePathToFullFilePath(EasterEggName.c_str());
   298|
   299|	return EasterEggName;
   300|}
   301|
   302|int getGhostTypingSpeedFromParam(ParamVector & params)
   303|{
   304|	std::wstring speedStr;
   305|	if (!getParamValFromString(L"-qSpeed", params, speedStr))
   306|		return -1;
   307|	
   308|	int speed = std::stoi(speedStr, 0);
   309|	if (speed <= 0 || speed > 3)
   310|		return -1;
   311|
   312|	return speed;
   313|}
   314|
   315|const wchar_t FLAG_MULTI_INSTANCE[] = L"-multiInst";
   316|const wchar_t FLAG_NO_PLUGIN[] = L"-noPlugin";
   317|const wchar_t FLAG_READONLY[] = L"-ro"; // for current cmdline file(s) only
   318|const wchar_t FLAG_FULL_READONLY[] = L"-fullReadOnly"; // user still can manually toggle OFF the R/O-state of N++ tabs, so saving of the tab filebuffers is possible
   319|const wchar_t FLAG_FULL_READONLY_SAVING_FORBIDDEN[] = L"-fullReadOnlySavingForbidden"; // user cannot toggle R/O-state of N++ tabs, impossible to save opened tab filebuffers
   320|const wchar_t FLAG_NOSESSION[] = L"-nosession";
   321|const wchar_t FLAG_NOTABBAR[] = L"-notabbar";
   322|const wchar_t FLAG_SYSTRAY[] = L"-systemtray";
   323|const wchar_t FLAG_LOADINGTIME[] = L"-loadingTime";
   324|const wchar_t FLAG_HELP[] = L"--help";
   325|const wchar_t FLAG_ALWAYS_ON_TOP[] = L"-alwaysOnTop";
   326|const wchar_t FLAG_OPENSESSIONFILE[] = L"-openSession";
   327|const wchar_t FLAG_RECURSIVE[] = L"-r";
   328|const wchar_t FLAG_FUNCLSTEXPORT[] = L"-export=functionList";
   329|const wchar_t FLAG_PRINTANDQUIT[] = L"-quickPrint";
   330|const wchar_t FLAG_NOTEPAD_COMPATIBILITY[] = L"-notepadStyleCmdline";
   331|const wchar_t FLAG_OPEN_FOLDERS_AS_WORKSPACE[] = L"-openFoldersAsWorkspace";
   332|const wchar_t FLAG_SETTINGS_DIR[] = L"-settingsDir=";
   333|const wchar_t FLAG_TITLEBAR_ADD[] = L"-titleAdd=";
   334|const wchar_t FLAG_APPLY_UDL[] = L"-udl=";
   335|const wchar_t FLAG_PLUGIN_MESSAGE[] = L"-pluginMessage=";
   336|const wchar_t FLAG_MONITOR_FILES[] = L"-monitor";
   337|
   338|void doException(Notepad_plus_Window & notepad_plus_plus)
   339|{
   340|	Win32Exception::removeHandler();	//disable exception handler after exception, we don't want corrupt data structures to crash the exception handler
   341|	::MessageBox(Notepad_plus_Window::gNppHWND, L"npminmin will attempt to save any unsaved data. However, data loss is very likely.", L"Recovery initiating", MB_OK | MB_ICONINFORMATION);
   342|
   343|	wchar_t tmpDir[1024];
   344|	GetTempPath(1024, tmpDir);
   345|	std::wstring emergencySavedDir = tmpDir;
   346|	emergencySavedDir += L"\\npminmin RECOV";
   347|
   348|	bool res = notepad_plus_plus.emergency(emergencySavedDir);
   349|	if (res)
   350|	{
   351|		std::wstring displayText = L"npminmin was able to successfully recover some unsaved documents, or nothing to be saved could be found.\r\nYou can find the results at :\r\n";
   352|		displayText += emergencySavedDir;
   353|		::MessageBox(Notepad_plus_Window::gNppHWND, displayText.c_str(), L"Recovery success", MB_OK | MB_ICONINFORMATION);
   354|	}
   355|	else
   356|		::MessageBox(Notepad_plus_Window::gNppHWND, L"Unfortunately, npminmin was not able to save your work. We are sorry for any lost data.", L"Recovery failure", MB_OK | MB_ICONERROR);
   357|}
   358|
   359|// Looks for -z arguments and strips command line arguments following those, if any
   360|void stripIgnoredParams(ParamVector & params)
   361|{
   362|	for (auto it = params.begin(); it != params.end(); )
   363|	{
   364|		if (lstrcmp(it->c_str(), L"-z") == 0)
   365|		{
   366|			auto nextIt = std::next(it);
   367|			if ( nextIt != params.end() )
   368|			{
   369|				params.erase(nextIt);
   370|			}
   371|			it = params.erase(it);
   372|		}
   373|		else
   374|		{
   375|			++it;
   376|		}
   377|	}
   378|}
   379|
   380|bool launchUpdater(const std::wstring& updaterFullPath, const std::wstring& updaterDir)
   381|{
   382|	NppParameters& nppParameters = NppParameters::getInstance();
   383|	NppGUI& nppGui = nppParameters.getNppGUI();
   384|
   385|	// check if update interval elapsed
   386|	Date today(0);
   387|	if (today < nppGui._autoUpdateOpt._nextUpdateDate)
   388|		return false;
   389|
   390|	std::wstring updaterParams;
   391|	nppParameters.buildGupParams(updaterParams);
   392|
   393|	Process updater(updaterFullPath.c_str(), updaterParams.c_str(), updaterDir.c_str());
   394|	updater.run();
   395|
   396|	// Update next update date
   397|	if (nppGui._autoUpdateOpt._intervalDays < 0) // Make sure interval days value is positive
   398|		nppGui._autoUpdateOpt._intervalDays = 0 - nppGui._autoUpdateOpt._intervalDays;
   399|	nppGui._autoUpdateOpt._nextUpdateDate = Date(nppGui._autoUpdateOpt._intervalDays);
   400|
   401|	return true;
   402|}
   403|
   404|DWORD nppUacSave(const wchar_t* wszTempFilePath, const wchar_t* wszProtectedFilePath2Save)
   405|{
   406|	if ((lstrlenW(wszTempFilePath) == 0) || (lstrlenW(wszProtectedFilePath2Save) == 0)) // safe check (lstrlen returns 0 for possible nullptr)
   407|		return ERROR_INVALID_PARAMETER;
   408|	if (!doesFileExist(wszTempFilePath))
   409|		return ERROR_FILE_NOT_FOUND;
   410|
   411|	DWORD dwRetCode = ERROR_SUCCESS;
   412|
   413|	bool isOutputReadOnly = false;
   414|	bool isOutputHidden = false;
   415|	bool isOutputSystem = false;
   416|	WIN32_FILE_ATTRIBUTE_DATA attributes{};
   417|	attributes.dwFileAttributes = INVALID_FILE_ATTRIBUTES;
   418|	if (getFileAttributesExWithTimeout(wszProtectedFilePath2Save, &attributes))
   419|	{
   420|		if (attributes.dwFileAttributes != INVALID_FILE_ATTRIBUTES && !(attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
   421|		{
   422|			isOutputReadOnly = (attributes.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0;
   423|			isOutputHidden = (attributes.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
   424|			isOutputSystem = (attributes.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0;
   425|			if (isOutputReadOnly) attributes.dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
   426|			if (isOutputHidden) attributes.dwFileAttributes &= ~FILE_ATTRIBUTE_HIDDEN;
   427|			if (isOutputSystem) attributes.dwFileAttributes &= ~FILE_ATTRIBUTE_SYSTEM;
   428|			if (isOutputReadOnly || isOutputHidden || isOutputSystem)
   429|				::SetFileAttributes(wszProtectedFilePath2Save, attributes.dwFileAttributes); // temporarily remove the problematic ones
   430|		}
   431|	}
   432|
   433|	// cannot use simple MoveFile here as it retains the tempfile permissions when on the same volume...
   434|	if (!::CopyFileW(wszTempFilePath, wszProtectedFilePath2Save, FALSE))
   435|	{
   436|		// fails if the destination file exists and has the R/O and/or Hidden attribute set
   437|		dwRetCode = ::GetLastError();
   438|	}
   439|	else
   440|	{
   441|		// ok, now dispose of the tempfile used
   442|		::DeleteFileW(wszTempFilePath);
   443|	}
   444|
   445|	// set back the possible original file attributes
   446|	if (isOutputReadOnly || isOutputHidden || isOutputSystem)
   447|	{
   448|		if (isOutputReadOnly) attributes.dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
   449|		if (isOutputHidden) attributes.dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;
   450|		if (isOutputSystem) attributes.dwFileAttributes |= FILE_ATTRIBUTE_SYSTEM;
   451|		::SetFileAttributes(wszProtectedFilePath2Save, attributes.dwFileAttributes);
   452|	}
   453|
   454|	return dwRetCode;
   455|}
   456|
   457|DWORD nppUacSetFileAttributes(const DWORD dwFileAttribs, const wchar_t* wszFilePath)
   458|{
   459|	if (lstrlenW(wszFilePath) == 0) // safe check (lstrlen returns 0 for possible nullptr)
   460|		return ERROR_INVALID_PARAMETER;
   461|	if (!doesFileExist(wszFilePath))
   462|		return ERROR_FILE_NOT_FOUND;
   463|	if (dwFileAttribs == INVALID_FILE_ATTRIBUTES || (dwFileAttribs & FILE_ATTRIBUTE_DIRECTORY))
   464|		return ERROR_INVALID_PARAMETER;
   465|
   466|	if (!::SetFileAttributes(wszFilePath, dwFileAttribs))
   467|		return ::GetLastError();
   468|
   469|	return ERROR_SUCCESS;
   470|}
   471|
   472|DWORD nppUacMoveFile(const wchar_t* wszOriginalFilePath, const wchar_t* wszNewFilePath)
   473|{
   474|	if ((lstrlenW(wszOriginalFilePath) == 0) || (lstrlenW(wszNewFilePath) == 0)) // safe check (lstrlen returns 0 for possible nullptr)
   475|		return ERROR_INVALID_PARAMETER;
   476|	if (!doesFileExist(wszOriginalFilePath))
   477|		return ERROR_FILE_NOT_FOUND;
   478|
   479|	if (!::MoveFileEx(wszOriginalFilePath, wszNewFilePath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH))
   480|		return ::GetLastError();
   481|	else
   482|		return ERROR_SUCCESS;
   483|}
   484|
   485|DWORD nppUacCreateEmptyFile(const wchar_t* wszNewEmptyFilePath)
   486|{
   487|	if (lstrlenW(wszNewEmptyFilePath) == 0) // safe check (lstrlen returns 0 for possible nullptr)
   488|		return ERROR_INVALID_PARAMETER;
   489|	if (doesFileExist(wszNewEmptyFilePath))
   490|		return ERROR_FILE_EXISTS;
   491|
   492|	Win32_IO_File file(wszNewEmptyFilePath);
   493|	if (!file.isOpened())
   494|		return file.getLastErrorCode();
   495|
   496|	return ERROR_SUCCESS;
   497|}
   498|
   499|} // namespace
   500|
   501|