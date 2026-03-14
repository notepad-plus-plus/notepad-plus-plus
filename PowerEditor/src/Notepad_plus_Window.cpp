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
    17|
    18|#include <shlwapi.h>
    19|#include "Notepad_plus_Window.h"
    20|
    21|HWND Notepad_plus_Window::gNppHWND = NULL;
    22|
    23|namespace // anonymous
    24|{
    25|
    26|	struct PaintLocker final
    27|	{
    28|		explicit PaintLocker(HWND handle)
    29|			: handle(handle)
    30|		{
    31|			// disallow drawing on the window
    32|			LockWindowUpdate(handle);
    33|		}
    34|
    35|		~PaintLocker()
    36|		{
    37|			// re-allow drawing for the window
    38|			LockWindowUpdate(NULL);
    39|
    40|			// force re-draw
    41|			InvalidateRect(handle, nullptr, TRUE);
    42|			RedrawWindow(handle, nullptr, NULL, RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME | RDW_INVALIDATE);
    43|		}
    44|
    45|		HWND handle;
    46|	};
    47|
    48|} // anonymous namespace
    49|
    50|using namespace std;
    51|
    52|void Notepad_plus_Window::setStartupBgColor(COLORREF BgColor)
    53|{
    54|	RECT windowClientArea;
    55|	HDC hdc = GetDCEx(_hSelf, NULL, DCX_CACHE | DCX_LOCKWINDOWUPDATE); //lock window update flag due to PaintLocker
    56|	GetClientRect(_hSelf, &windowClientArea);
    57|	HBRUSH hBrush = ::CreateSolidBrush(BgColor);
    58|	::FillRect(hdc, &windowClientArea, hBrush);
    59|	::DeleteObject(hBrush);
    60|	ReleaseDC(_hSelf, hdc);
    61|}
    62|
    63|
    64|void Notepad_plus_Window::init(HINSTANCE hInst, HWND parent, const wchar_t *cmdLine, CmdLineParams *cmdLineParams)
    65|{
    66|	Window::init(hInst, parent);
    67|	WNDCLASS nppClass{};
    68|
    69|	nppClass.style = CS_BYTEALIGNWINDOW | CS_DBLCLKS;
    70|	nppClass.lpfnWndProc = Notepad_plus_Proc;
    71|	nppClass.cbClsExtra = 0;
    72|	nppClass.cbWndExtra = 0;
    73|	nppClass.hInstance = _hInst;
    74|	nppClass.hIcon = ::LoadIcon(hInst, MAKEINTRESOURCE(IDI_M30ICON));
    75|	nppClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    76|	nppClass.hbrBackground = ::CreateSolidBrush(::GetSysColor(COLOR_MENU));
    77|	nppClass.lpszMenuName = MAKEINTRESOURCE(IDR_M30_MENU);
    78|	nppClass.lpszClassName = _className;
    79|
    80|	_isPrelaunch = cmdLineParams->_isPreLaunch;
    81|
    82|	if (!::RegisterClass(&nppClass))
    83|	{
    84|		throw std::runtime_error("Notepad_plus_Window::init : RegisterClass() function failed");
    85|	}
    86|
    87|	NppParameters& nppParams = NppParameters::getInstance();
    88|	NppGUI & nppGUI = nppParams.getNppGUI();
    89|
    90|	if (cmdLineParams->_isNoPlugin)
    91|		_notepad_plus_plus_core._pluginsManager.disable();
    92|
    93|	nppGUI._isCmdlineNosessionActivated = cmdLineParams->_isNoSession;
    94|	nppGUI._isFullReadOnly = cmdLineParams->_isFullReadOnly;
    95|	nppGUI._isFullReadOnlySavingForbidden = cmdLineParams->_isFullReadOnlySavingForbidden;
    96|
    97|	_hIconAbsent = ::LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICONABSENT));
    98|
    99|	_hSelf = ::CreateWindowEx(
   100|		WS_EX_ACCEPTFILES | (_notepad_plus_plus_core._nativeLangSpeaker.isRTL() ? WS_EX_LAYOUTRTL : 0),
   101|		_className,
   102|		L"npminmin",
   103|		(WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN),
   104|		// CreateWindowEx bug : set all 0 to walk around the problem
   105|		0, 0, 0, 0,
   106|		_hParent, nullptr, _hInst,
   107|		(LPVOID) this); // pass the ptr of this instantiated object
   108|        // for retrieve it in Notepad_plus_Proc from
   109|        // the CREATESTRUCT.lpCreateParams afterward.
   110|
   111|	if (NULL == _hSelf)
   112|		throw std::runtime_error("Notepad_plus_Window::init : CreateWindowEx() function return null");
   113|
   114|	if (!cmdLineParams->_pluginMessage.empty())
   115|	{
   116|		SCNotification scnN{};
   117|		scnN.nmhdr.code = NPPN_CMDLINEPLUGINMSG;
   118|		scnN.nmhdr.hwndFrom = _hSelf;
   119|		scnN.nmhdr.idFrom = reinterpret_cast<uptr_t>(cmdLineParams->_pluginMessage.c_str());
   120|		_notepad_plus_plus_core._pluginsManager.notify(&scnN);
   121|	}
   122|
   123|	PaintLocker paintLocker{_hSelf};
   124|
   125|	_notepad_plus_plus_core.staticCheckMenuAndTB();
   126|
   127|	gNppHWND = _hSelf;
   128|
   129|	if (cmdLineParams->isPointValid())
   130|	{
   131|		::MoveWindow(_hSelf, cmdLineParams->_point.x, cmdLineParams->_point.y, nppGUI._appPos.right, nppGUI._appPos.bottom, TRUE);
   132|	}
   133|	else
   134|	{
   135|		WINDOWPLACEMENT posInfo{};
   136|		posInfo.length = sizeof(WINDOWPLACEMENT);
   137|		posInfo.flags = 0;
   138|		if (_isPrelaunch)
   139|			posInfo.showCmd = SW_HIDE;
   140|		else
   141|			posInfo.showCmd = nppGUI._isMaximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
   142|
   143|		posInfo.ptMinPosition.x = (LONG)-1;
   144|		posInfo.ptMinPosition.y = (LONG)-1;
   145|		posInfo.ptMaxPosition.x = (LONG)-1;
   146|		posInfo.ptMaxPosition.y = (LONG)-1;
   147|		posInfo.rcNormalPosition.left   = nppGUI._appPos.left;
   148|		posInfo.rcNormalPosition.top    = nppGUI._appPos.top;
   149|		posInfo.rcNormalPosition.bottom = nppGUI._appPos.top + nppGUI._appPos.bottom;
   150|		posInfo.rcNormalPosition.right  = nppGUI._appPos.left + nppGUI._appPos.right;
   151|
   152|		//SetWindowPlacement will take care of situations, where saved position was in no longer available monitor
   153|		::SetWindowPlacement(_hSelf,&posInfo);
   154|		
   155|		if (NppDarkMode::isEnabled())
   156|			setStartupBgColor(NppDarkMode::getDlgBackgroundColor()); //draw dark background when opening Npp without position data
   157|	}
   158|
   159|	if ((nppGUI._tabStatus & TAB_MULTILINE) != 0)
   160|		::SendMessage(_hSelf, NPPM_INTERNAL_MULTILINETABBAR, 0, 0);
   161|
   162|	if (!nppGUI._menuBarShow)
   163|		::SetMenu(_hSelf, NULL);
   164|
   165|	if (cmdLineParams->_isNoTab || (nppGUI._tabStatus & TAB_HIDE))
   166|	{
   167|		const int tabStatusOld = nppGUI._tabStatus;
   168|		::SendMessage(_hSelf, NPPM_HIDETABBAR, 0, TRUE);
   169|		if (cmdLineParams->_isNoTab)
   170|		{
   171|			// Restore old settings when tab bar has been hidden from tab bar.
   172|			if (!(tabStatusOld & TAB_HIDE))
   173|				nppGUI._forceTabbarVisible = true;
   174|		}
   175|	}
   176|
   177|	if (cmdLineParams->_alwaysOnTop)
   178|		::SendMessage(_hSelf, WM_COMMAND, IDM_VIEW_ALWAYSONTOP, 0);
   179|
   180|	std::chrono::steady_clock::duration sessionLoadingTime{};
   181|	if (nppGUI._rememberLastSession && !nppGUI._isCmdlineNosessionActivated)
   182|	{
   183|		std::chrono::steady_clock::time_point sessionLoadingStartTP = std::chrono::steady_clock::now();
   184|		_notepad_plus_plus_core.loadLastSession();
   185|		sessionLoadingTime = std::chrono::steady_clock::now() - sessionLoadingStartTP;
   186|	}
   187|
   188|	if (nppParams.doFunctionListExport() || nppParams.doPrintAndExit())
   189|	{
   190|		::ShowWindow(_hSelf, SW_HIDE);
   191|	}
   192|	else if (!cmdLineParams->_isPreLaunch)
   193|	{
   194|		if (cmdLineParams->isPointValid())
   195|			::ShowWindow(_hSelf, SW_SHOW);
   196|		else
   197|			::ShowWindow(_hSelf, nppGUI._isMaximized ? SW_MAXIMIZE : SW_SHOW);
   198|	}
   199|	else
   200|	{
   201|		HICON icon = nullptr;
   202|		loadTrayIcon(_hInst, &icon);
   203|		_notepad_plus_plus_core._pTrayIco = new trayIconControler(_hSelf, IDI_M30ICON, NPPM_INTERNAL_MINIMIZED_TRAY, icon, L"");
   204|		_notepad_plus_plus_core._pTrayIco->doTrayIcon(ADD);
   205|	}
   206|
   207|	if(cmdLineParams->isPointValid() && NppDarkMode::isEnabled())
   208|		setStartupBgColor(NppDarkMode::getDlgBackgroundColor()); //draw dark background when opening Npp through cmd with position data
   209|
   210|	std::vector<std::wstring> fileNames;
   211|	std::vector<std::wstring> patterns;
   212|	patterns.push_back(L"*.xml");
   213|
   214|	std::wstring nppDir = nppParams.getNppPath();
   215|
   216|	LocalizationSwitcher & localizationSwitcher = nppParams.getLocalizationSwitcher();
   217|	std::wstring localizationDir = nppDir;
   218|	pathAppend(localizationDir, L"localization\\");
   219|
   220|	_notepad_plus_plus_core.getMatchedFileNames(localizationDir.c_str(), 0, patterns, fileNames, false, false);
   221|	for (const auto& fileName : fileNames)
   222|		localizationSwitcher.addLanguageFromXml(fileName);
   223|
   224|	fileNames.clear();
   225|	ThemeSwitcher & themeSwitcher = nppParams.getThemeSwitcher();
   226|
   227|	//  Get themes from both npp install themes dir and user data themes dir (AppData, settingsDir, or cloud) with the per user
   228|	//  overriding default themes of the same name.
   229|
   230|	std::wstring userDataThemeDir = nppParams.getUserPath(); // getUserPath will always pick the right settingsDir/Cloud/AppData location
   231|	if (!userDataThemeDir.empty() && userDataThemeDir != nppDir)	
   232|	{
   233|		// append files from userDataThemeDir, unless it matches nppDir (which means the themes are already in the internal structure)
   234|		pathAppend(userDataThemeDir, L"themes\\");
   235|		_notepad_plus_plus_core.getMatchedFileNames(userDataThemeDir.c_str(), 0, patterns, fileNames, false, false);
   236|		for (const auto& fileName: fileNames)
   237|		{
   238|			themeSwitcher.addThemeFromXml(fileName);
   239|		}
   240|	}
   241|
   242|	fileNames.clear();
   243|
   244|	std::wstring nppThemeDir = nppDir; // <- should use the pointer to avoid the constructor of copy
   245|	pathAppend(nppThemeDir, L"themes\\");
   246|
   247|	// Set theme directory to their installation directory
   248|	themeSwitcher.setThemeDirPath(nppThemeDir);
   249|
   250|	_notepad_plus_plus_core.getMatchedFileNames(nppThemeDir.c_str(), 0, patterns, fileNames, false, false);
   251|	for (const auto& fileName : fileNames)
   252|	{
   253|		std::wstring themeName( themeSwitcher.getThemeFromXmlFileName(fileName.c_str()) );
   254|		if (!themeSwitcher.themeNameExists(themeName.c_str()))
   255|		{
   256|			themeSwitcher.addThemeFromXml(fileName);
   257|			
   258|			if (!userDataThemeDir.empty() && userDataThemeDir != nppDir)
   259|			{
   260|				std::wstring userDataThemePath = userDataThemeDir;
   261|
   262|				if (!doesDirectoryExist(userDataThemePath.c_str()))
   263|				{
   264|					::CreateDirectory(userDataThemePath.c_str(), NULL);
   265|				}
   266|
   267|				wchar_t* fn = PathFindFileName(fileName.c_str());
   268|				pathAppend(userDataThemePath, fn);
   269|				themeSwitcher.addThemeStylerSavePath(fileName, userDataThemePath);
   270|			}
   271|		}
   272|	}
   273|
   274|	if (NppDarkMode::isWindowsModeEnabled())
   275|	{
   276|		std::wstring themePath;
   277|		std::wstring xmlFileName = NppDarkMode::getThemeName();
   278|		if (!xmlFileName.empty())
   279|		{
   280|			if (!nppParams.isLocal() || nppParams.isCloud())
   281|			{
   282|				themePath = nppParams.getUserPath();
   283|				pathAppend(themePath, L"themes\\");
   284|				pathAppend(themePath, xmlFileName);
   285|			}
   286|
   287|			if (themePath.empty() || !doesFileExist(themePath.c_str()))
   288|			{
   289|				themePath = themeSwitcher.getThemeDirPath();
   290|				pathAppend(themePath, xmlFileName);
   291|			}
   292|		}
   293|		else
   294|		{
   295|			const auto& themeInfo = themeSwitcher.getElementFromIndex(0);
   296|			themePath = themeInfo.second;
   297|		}
   298|
   299|		if (doesFileExist(themePath.c_str()))
   300|		{
   301|			nppGUI._themeName.assign(themePath);
   302|			nppParams.reloadStylers(themePath.c_str());
   303|			::SendMessage(_hSelf, WM_UPDATESCINTILLAS, TRUE, 0);
   304|		}
   305|	}
   306|
   307|	// Restore all dockable panels from the last session
   308|	for (size_t i = 0, len = _notepad_plus_plus_core._internalFuncIDs.size() ; i < len ; ++i)
   309|		::SendMessage(_hSelf, WM_COMMAND, _notepad_plus_plus_core._internalFuncIDs[i], 0);
   310|
   311|	std::chrono::steady_clock::duration cmdlineParamsLoadingTime{};
   312|	std::vector<std::wstring> fns;
   313|	if (cmdLine)
   314|	{
   315|		std::chrono::steady_clock::time_point cmdlineParamsLoadingStartTP = std::chrono::steady_clock::now();
   316|		fns = _notepad_plus_plus_core.loadCommandlineParams(cmdLine, cmdLineParams);
   317|		cmdlineParamsLoadingTime = std::chrono::steady_clock::now() - cmdlineParamsLoadingStartTP;
   318|	}
   319|
   320|	// Launch folder as workspace after all this dockable panel being restored from the last session
   321|	// To avoid dockable panel toggle problem.
   322|	if (cmdLineParams->_openFoldersAsWorkspace)
   323|	{
   324|		std::wstring emptyStr;
   325|		_notepad_plus_plus_core.launchFileBrowser(fns, emptyStr, true);
   326|	}
   327|	::SendMessage(_hSelf, WM_ACTIVATE, WA_ACTIVE, 0);
   328|
   329|	::SendMessage(_hSelf, NPPM_INTERNAL_CRLFFORMCHANGED, 0, 0);
   330|
   331|	::SendMessage(_hSelf, NPPM_INTERNAL_NPCFORMCHANGED, 0, 0);
   332|
   333|	::SendMessage(_hSelf, NPPM_INTERNAL_ENABLECHANGEHISTORY, 0, 0);
   334|
   335|	::SendMessage(_hSelf, NPPM_INTERNAL_LINECUTCOPYWITHOUTSELECTION, 0, 0);
   336|
   337|	::SendMessage(_hSelf, NPPM_INTERNAL_DISABLESELECTEDTEXTDRAGDROP, 0, 0);
   338|
   339|	if (nppGUI._newDocDefaultSettings._addNewDocumentOnStartup && nppGUI._rememberLastSession)
   340|	{
   341|		::SendMessage(_hSelf, WM_COMMAND, IDM_FILE_NEW, 0);
   342|	}
   343|
   344|	// Notify plugins that npminmin is ready
   345|	SCNotification scnN{};
   346|	scnN.nmhdr.code = NPPN_READY;
   347|	scnN.nmhdr.hwndFrom = _hSelf;
   348|	scnN.nmhdr.idFrom = 0;
   349|	_notepad_plus_plus_core._pluginsManager.notify(&scnN);
   350|
   351|	if (!cmdLineParams->_easterEggName.empty())
   352|	{
   353|		if (cmdLineParams->_quoteType == 0) // Easter Egg Name
   354|		{
   355|			int iQuote = _notepad_plus_plus_core.getQuoteIndexFrom(cmdLineParams->_easterEggName.c_str());
   356|			if (iQuote != -1)
   357|			{
   358|				_notepad_plus_plus_core.showQuoteFromIndex(iQuote);
   359|			}
   360|		}
   361|		else if (cmdLineParams->_quoteType == 1) // command line quote
   362|		{
   363|			_userQuote = cmdLineParams->_easterEggName;
   364|			_quoteParams.reset();
   365|			_quoteParams._quote = _userQuote.c_str();
   366|			_quoteParams._quoter = L"Anonymous #999";
   367|			_quoteParams._shouldBeTrolling = false;
   368|			_quoteParams._lang = cmdLineParams->_langType;
   369|			if (cmdLineParams->_ghostTypingSpeed == 1)
   370|				_quoteParams._speed = QuoteParams::slow;
   371|			else if (cmdLineParams->_ghostTypingSpeed == 2)
   372|				_quoteParams._speed = QuoteParams::rapid;
   373|			else if (cmdLineParams->_ghostTypingSpeed == 3)
   374|				_quoteParams._speed = QuoteParams::speedOfLight;
   375|
   376|			_notepad_plus_plus_core.showQuote(&_quoteParams);
   377|		}
   378|		else if (cmdLineParams->_quoteType == 2) // content from file
   379|		{
   380|			if (doesFileExist(cmdLineParams->_easterEggName.c_str()))
   381|			{
   382|				bool bLoadingFailed = false;
   383|				std::string content = getFileContent(cmdLineParams->_easterEggName.c_str(), &bLoadingFailed);
   384|				if (!bLoadingFailed)
   385|				{
   386|					WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
   387|					_userQuote = wmc.char2wchar(content.c_str(), SC_CP_UTF8);
   388|					if (!_userQuote.empty())
   389|					{
   390|						_quoteParams.reset();
   391|						_quoteParams._quote = _userQuote.c_str();
   392|						_quoteParams._quoter = L"Anonymous #999";
   393|						_quoteParams._shouldBeTrolling = false;
   394|						_quoteParams._lang = cmdLineParams->_langType;
   395|						if (cmdLineParams->_ghostTypingSpeed == 1)
   396|							_quoteParams._speed = QuoteParams::slow;
   397|						else if (cmdLineParams->_ghostTypingSpeed == 2)
   398|							_quoteParams._speed = QuoteParams::rapid;
   399|						else if (cmdLineParams->_ghostTypingSpeed == 3)
   400|							_quoteParams._speed = QuoteParams::speedOfLight;
   401|
   402|						_notepad_plus_plus_core.showQuote(&_quoteParams);
   403|					}
   404|				}
   405|			}
   406|		}
   407|	}
   408|
   409|	if (cmdLineParams->_showLoadingTime)
   410|	{
   411|		std::chrono::steady_clock::duration nppInitTime = (std::chrono::steady_clock::now() - g_nppStartTimePoint) - g_pluginsLoadingTime - sessionLoadingTime - cmdlineParamsLoadingTime;
   412|		std::wstringstream wss;
   413|		wss << L"npminmin initialization: " << std::chrono::hh_mm_ss{ std::chrono::duration_cast<std::chrono::milliseconds>(nppInitTime) } << std::endl;
   414|		wss << L"Plugins loading: " << std::chrono::hh_mm_ss{ std::chrono::duration_cast<std::chrono::milliseconds>(g_pluginsLoadingTime) } << std::endl;
   415|		wss << L"Last session loading: " << std::chrono::hh_mm_ss{ std::chrono::duration_cast<std::chrono::milliseconds>(sessionLoadingTime) } << std::endl;
   416|		wss << L"Command line params handling: " << std::chrono::hh_mm_ss{ std::chrono::duration_cast<std::chrono::milliseconds>(cmdlineParamsLoadingTime) } << std::endl;
   417|		wss << L"Total loading time: " << std::chrono::hh_mm_ss{ std::chrono::duration_cast<std::chrono::milliseconds>(nppInitTime + g_pluginsLoadingTime + sessionLoadingTime + cmdlineParamsLoadingTime) };
   418|		::MessageBoxW(NULL, wss.str().c_str(), L"npminmin loading time (hh:mm:ss.ms)", MB_OK);
   419|	}
   420|
   421|	if (cmdLineParams->_displayCmdLineArgs)
   422|	{
   423|		_notepad_plus_plus_core.command(IDM_CMDLINEARGUMENTS);
   424|	}
   425|
   426|	bool isSnapshotMode = nppGUI.isSnapshotMode();
   427|	if (isSnapshotMode)
   428|	{
   429|		_notepad_plus_plus_core.checkModifiedDocument(false);
   430|		// Launch backup task
   431|		_notepad_plus_plus_core.launchDocumentBackupTask();
   432|	}
   433|
   434|	// Make this call later to take effect
   435|	::SendMessage(_hSelf, NPPM_INTERNAL_SETWORDCHARS, 0, 0);
   436|	::SendMessage(_hSelf, NPPM_INTERNAL_SETNPC, 0, 0);
   437|
   438|	if (nppParams.doFunctionListExport())
   439|		::SendMessage(_hSelf, NPPM_INTERNAL_EXPORTFUNCLISTANDQUIT, 0, 0);
   440|
   441|	if (nppParams.doPrintAndExit())
   442|		::SendMessage(_hSelf, NPPM_INTERNAL_PRNTANDQUIT, 0, 0);
   443|}
   444|
   445|
   446|bool Notepad_plus_Window::isDlgsMsg(MSG *msg) const
   447|{
   448|	if (_notepad_plus_plus_core.processTabSwitchAccel(msg))
   449|		return true;
   450|
   451|	if (_notepad_plus_plus_core.processIncrFindAccel(msg))
   452|		return true;
   453|
   454|	if (_notepad_plus_plus_core.processFindAccel(msg))
   455|		return true;
   456|
   457|	for (size_t i = 0, len = _notepad_plus_plus_core._hModelessDlgs.size(); i < len; ++i)
   458|	{
   459|		if (::IsDialogMessageW(_notepad_plus_plus_core._hModelessDlgs[i], msg))
   460|			return true;
   461|	}
   462|	return false;
   463|}
   464|