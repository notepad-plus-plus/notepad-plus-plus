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
    18|#include <format>
    19|#include "AboutDlg.h"
    20|#include "Parameters.h"
    21|#include "localization.h"
    22|#include "json.hpp"
    23|#if defined __has_include
    24|#if __has_include ("NppLibsVersion.h")
    25|#include "NppLibsVersion.h"
    26|#endif
    27|#endif
    28|
    29|using namespace std;
    30|
    31|#ifdef _MSC_VER
    32|#pragma warning(disable : 4996) // for GetVersion()
    33|#endif
    34|
    35|
    36|// local DebugInfo helper
    37|void AppendDisplayAdaptersInfo(wstring& strOut, const unsigned int maxAdaptersIn)
    38|{
    39|	strOut += L"\r\n    installed Display Class adapters: ";
    40|
    41|	const wchar_t wszRegDisplayClassWinNT[] = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E968-E325-11CE-BFC1-08002BE10318}";
    42|	HKEY hkDisplayClass = nullptr;
    43|	LSTATUS lStatus = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszRegDisplayClassWinNT, 0,
    44|		KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hkDisplayClass);
    45|	if ((lStatus != ERROR_SUCCESS) || !hkDisplayClass)
    46|	{
    47|		strOut += L"\r\n    - error, failed to open the Registry Display Class key!";
    48|		return;
    49|	}
    50|
    51|	DWORD dwSubkeysCount = 0;
    52|	lStatus = ::RegQueryInfoKeyW(hkDisplayClass, nullptr, nullptr, nullptr, &dwSubkeysCount,
    53|		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    54|	if ((lStatus == ERROR_SUCCESS) && (dwSubkeysCount > 0))
    55|	{
    56|		DWORD dwAdapterSubkeysFound = 0;
    57|		for (DWORD i = 0; i < dwSubkeysCount; ++i)
    58|		{
    59|			if (dwAdapterSubkeysFound >= maxAdaptersIn)
    60|			{
    61|				strOut += L"\r\n    - warning, search has been limited to maximum number of adapter records: "
    62|					+ std::to_wstring(maxAdaptersIn);
    63|				break;
    64|			}
    65|
    66|			wstring strAdapterNo = std::format(L"{:#04d}", i); // 0000, 0001, 0002, etc...
    67|			wstring strAdapterSubKey = wszRegDisplayClassWinNT;
    68|			strAdapterSubKey += L'\\' + strAdapterNo;
    69|			HKEY hkAdapterSubKey = nullptr;
    70|			lStatus = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, strAdapterSubKey.c_str(), 0, KEY_READ, &hkAdapterSubKey);
    71|			if ((lStatus == ERROR_SUCCESS) && hkAdapterSubKey)
    72|			{
    73|				strAdapterNo.insert(0, L"\r\n        "); // doubling the output indentation
    74|				const unsigned int nKeyValMaxLen = 127;
    75|				const DWORD dwKeyValMaxSize = nKeyValMaxLen * sizeof(wchar_t);
    76|				wchar_t wszKeyVal[nKeyValMaxLen + 1]{}; // +1 ... to ensure NUL termination
    77|				DWORD dwType = REG_SZ;
    78|				DWORD dwSize = dwKeyValMaxSize;
    79|				if (::RegQueryValueExW(hkAdapterSubKey, L"DriverDesc", nullptr, &dwType, (LPBYTE)wszKeyVal, &dwSize)
    80|					== ERROR_SUCCESS)
    81|				{
    82|					dwAdapterSubkeysFound++;
    83|					strOut += strAdapterNo + L": Description - ";
    84|					strOut += wszKeyVal;
    85|				}
    86|				// for exact HW identification, query about the "MatchingDeviceId"
    87|				dwSize = dwKeyValMaxSize;
    88|				if (::RegQueryValueExW(hkAdapterSubKey, L"DriverVersion", nullptr, &dwType, (LPBYTE)wszKeyVal, &dwSize)
    89|					== ERROR_SUCCESS)
    90|				{
    91|					strOut += strAdapterNo + L": DriverVersion - ";
    92|					strOut += wszKeyVal;
    93|				}
    94|				// to obtain also the above driver date, query about the "DriverDate"
    95|				::RegCloseKey(hkAdapterSubKey);
    96|				hkAdapterSubKey = nullptr;
    97|			}
    98|		}
    99|	}
   100|
   101|	::RegCloseKey(hkDisplayClass);
   102|	hkDisplayClass = nullptr;
   103|}
   104|
   105|
   106|intptr_t CALLBACK AboutDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
   107|{
   108|	switch (message)
   109|	{
   110|		case WM_INITDIALOG:
   111|		{
   112|			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
   113|
   114|			HWND compileDateHandle = ::GetDlgItem(_hSelf, IDC_BUILD_DATETIME);
   115|			wstring buildTime = L"Build time: ";
   116|
   117|			WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
   118|			buildTime +=  wmc.char2wchar(__DATE__, CP_ACP);
   119|			buildTime += L" - ";
   120|			buildTime +=  wmc.char2wchar(__TIME__, CP_ACP);
   121|
   122|			NppParameters& nppParam = NppParameters::getInstance();
   123|			LPCTSTR bitness = nppParam.archType() == IMAGE_FILE_MACHINE_I386 ? L"(32-bit)" : nppParam.archType() == IMAGE_FILE_MACHINE_AMD64 ? L"(64-bit)" : L"(ARM 64-bit)";
   124|			::SetDlgItemText(_hSelf, IDC_VERSION_BIT, bitness);
   125|
   126|			::SendMessage(compileDateHandle, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buildTime.c_str()));
   127|			::EnableWindow(compileDateHandle, FALSE);
   128|
   129|            HWND licenceEditHandle = ::GetDlgItem(_hSelf, IDC_LICENCE_EDIT);
   130|			::SendMessage(licenceEditHandle, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(LICENCE_TXT));
   131|
   132|            //_emailLink.init(_hInst, _hSelf);
   133|			//_emailLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), L"mailto:don.h@free.fr";
   134|			//_emailLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), L"https://github.com/ridermw/np-minus-minus/news/v781-free-uyghur-edition/";
   135|			//_emailLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), L"https://github.com/ridermw/np-minus-minus/news/v792-stand-with-hong-kong/";
   136|			//_emailLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), L"https://github.com/ridermw/np-minus-minus/news/v791-pour-samuel-paty/";
   137|			//_pageLink.create(::GetDlgItem(_hSelf, IDC_HOME_ADDR), L"https://github.com/ridermw/np-minus-minus/news/v843-unhappy-users-edition/";
   138|			//_pageLink.create(::GetDlgItem(_hSelf, IDC_HOME_ADDR), L"https://github.com/ridermw/np-minus-minus/news/v844-happy-users-edition/";
   139|            //_pageLink.create(::GetDlgItem(_hSelf, IDC_HOME_ADDR), L"https://github.com/ridermw/np-minus-minus/news/v86-20thyearanniversary";
   140|            //_pageLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), L"https://github.com/ridermw/np-minus-minus/news/v87-about-taiwan/");
   141|			//_pageLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), L"https://github.com/ridermw/np-minus-minus/news/v881-we-are-with-ukraine/");
   142|            
   143|			_pageLink.init(_hInst, _hSelf);
   144|            _pageLink.create(::GetDlgItem(_hSelf, IDC_HOME_ADDR), L"https://github.com/ridermw/np-minus-minus/");
   145|
   146|			return TRUE;
   147|		}
   148|
   149|		case WM_CTLCOLORDLG:
   150|		case WM_CTLCOLORSTATIC:
   151|		{
   152|			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
   153|		}
   154|
   155|		case WM_PRINTCLIENT:
   156|		{
   157|			if (NppDarkMode::isEnabled())
   158|			{
   159|				return TRUE;
   160|			}
   161|			break;
   162|		}
   163|
   164|		case NPPM_INTERNAL_REFRESHDARKMODE:
   165|		{
   166|			NppDarkMode::autoThemeChildControls(_hSelf);
   167|			if (_hIcon != nullptr)
   168|			{
   169|				::DestroyIcon(_hIcon);
   170|				_hIcon = nullptr;
   171|			}
   172|			return TRUE;
   173|		}
   174|
   175|		case WM_DPICHANGED:
   176|		{
   177|			_dpiManager.setDpiWP(wParam);
   178|			destroy();
   179|			//setPositionDpi(lParam);
   180|			getWindowRect(_rc);
   181|
   182|			return TRUE;
   183|		}
   184|
   185|		case WM_DRAWITEM:
   186|		{
   187|			const int iconSize = _dpiManager.scale(80);
   188|			if (_hIcon == nullptr)
   189|			{
   190|				DPIManagerV2::loadIcon(_hInst, MAKEINTRESOURCE(NppDarkMode::isEnabled() ? IDI_CHAMELEON_DM : IDI_CHAMELEON), iconSize, iconSize, &_hIcon);
   191|				//DPIManagerV2::loadIcon(_hInst, MAKEINTRESOURCE(IDI_WITHUKRAINE), iconSize, iconSize, &_hIcon);
   192|				//DPIManagerV2::loadIcon(_hInst, MAKEINTRESOURCE(NppDarkMode::isEnabled() ? IDI_TAIWANSSOVEREIGNTY_DM : IDI_TAIWANSSOVEREIGNTY), iconSize, iconSize, &_hIcon);
   193|			}
   194|
   195|			//HICON hIcon = (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_JESUISCHARLIE), IMAGE_ICON, 64, 64, LR_DEFAULTSIZE);
   196|			//HICON hIcon = (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_GILETJAUNE), IMAGE_ICON, 64, 64, LR_DEFAULTSIZE);
   197|			//HICON hIcon = (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_SAMESEXMARRIAGE), IMAGE_ICON, 64, 64, LR_DEFAULTSIZE);
   198|
   199|			auto pdis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
   200|			::DrawIconEx(pdis->hDC, 0, 0, _hIcon, iconSize, iconSize, 0, nullptr, DI_NORMAL);
   201|
   202|			return TRUE;
   203|		}
   204|
   205|		case WM_COMMAND:
   206|		{
   207|			switch (wParam)
   208|			{
   209|				case IDCANCEL :
   210|				case IDOK :
   211|					display(false);
   212|					return TRUE;
   213|
   214|				default :
   215|					break;
   216|			}
   217|			break;
   218|		}
   219|
   220|		case WM_DESTROY:
   221|		{
   222|			destroy();
   223|			return TRUE;
   224|		}
   225|	}
   226|	return FALSE;
   227|}
   228|
   229|void AboutDlg::doDialog()
   230|{
   231|	if (!isCreated())
   232|		create(IDD_ABOUTBOX);
   233|
   234|	// Adjust the position of AboutBox
   235|	moveForDpiChange();
   236|	goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);
   237|}
   238|
   239|intptr_t CALLBACK DebugInfoDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
   240|{
   241|	switch (message)
   242|	{
   243|		case WM_INITDIALOG:
   244|		{
   245|			NppParameters& nppParam = NppParameters::getInstance();
   246|			NppGUI& nppGui = nppParam.getNppGUI();
   247|
   248|			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
   249|
   250|			// npminmin version
   251|			_debugInfoStr = NOTEPAD_PLUS_VERSION;
   252|			_debugInfoStr += nppParam.archType() == IMAGE_FILE_MACHINE_I386 ? L"   (32-bit)" : nppParam.archType() == IMAGE_FILE_MACHINE_AMD64 ? L"   (64-bit)" : L"   (ARM 64-bit)";
   253|			_debugInfoStr += L"\r\n";
   254|
   255|			// Build time
   256|			_debugInfoStr += L"Build time: ";
   257|			wstring buildTime;
   258|			WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
   259|			buildTime += wmc.char2wchar(__DATE__, CP_ACP);
   260|			buildTime += L" - ";
   261|			buildTime += wmc.char2wchar(__TIME__, CP_ACP);
   262|			_debugInfoStr += buildTime;
   263|			_debugInfoStr += L"\r\n";
   264|
   265|#if defined(__clang__)
   266|			_debugInfoStr += L"Built with: Clang ";
   267|			_debugInfoStr += wmc.char2wchar(__clang_version__, CP_ACP);
   268|			_debugInfoStr += L"\r\n";
   269|#elif defined(__GNUC__)
   270|			_debugInfoStr += L"Built with: GCC ";
   271|			_debugInfoStr += wmc.char2wchar(__VERSION__, CP_ACP);
   272|			_debugInfoStr += L"\r\n";
   273|#elif !defined(_MSC_VER)
   274|			_debugInfoStr += L"Built with: (unknown)\r\n";
   275|#endif
   276|
   277|			// Scintilla/Lexilla version
   278|			_debugInfoStr += L"Scintilla/Lexilla included: ";
   279|			{
   280|				string strSciLexVer = NPP_SCINTILLA_VERSION;
   281|				strSciLexVer += "/";
   282|				strSciLexVer += NPP_LEXILLA_VERSION;
   283|				_debugInfoStr += wmc.char2wchar(strSciLexVer.c_str(), CP_ACP);
   284|			}
   285|			_debugInfoStr += L"\r\n";
   286|
   287|			// Boost Regex version
   288|			_debugInfoStr += L"Boost Regex included: ";
   289|			_debugInfoStr += wmc.char2wchar(NPP_BOOST_REGEX_VERSION, CP_ACP);
   290|			_debugInfoStr += L"\r\n";
   291|
   292|#if defined(PUGIXML_VERSION)
   293|			// pugixml version
   294|			_debugInfoStr += L"pugixml included: ";
   295|			_debugInfoStr += std::to_wstring(PUGIXML_VERSION / 1000) + L"." + std::to_wstring((PUGIXML_VERSION % 1000) / 10);
   296|			_debugInfoStr += L"\r\n";
   297|#endif
   298|
   299|			// JSON version
   300|			_debugInfoStr += L"nlohmann JSON included: ";
   301|			_debugInfoStr += to_wstring(NLOHMANN_JSON_VERSION_MAJOR) + L"." + to_wstring(NLOHMANN_JSON_VERSION_MINOR) + L"." + to_wstring(NLOHMANN_JSON_VERSION_PATCH);
   302|			_debugInfoStr += L"\r\n";
   303|
   304|			// Binary path
   305|			_debugInfoStr += L"Path: ";
   306|			wchar_t nppFullPath[MAX_PATH]{};
   307|			::GetModuleFileName(NULL, nppFullPath, MAX_PATH);
   308|			_debugInfoStr += nppFullPath;
   309|			_debugInfoStr += L"\r\n";
   310|
   311|			// Command line as specified for program launch
   312|			// The _cmdLinePlaceHolder will be replaced later by refreshDebugInfo()
   313|			_debugInfoStr += L"Command Line: ";
   314|			_debugInfoStr += _cmdLinePlaceHolder;
   315|			_debugInfoStr += L"\r\n";
   316|
   317|			// Administrator mode
   318|			_debugInfoStr += L"Admin mode: ";
   319|			_debugInfoStr += _isAdmin ? L"ON" : L"OFF";
   320|			_debugInfoStr += L"\r\n";
   321|
   322|			// local conf
   323|			_debugInfoStr += L"Local Conf mode: ";
   324|			bool doLocalConf = (NppParameters::getInstance()).isLocal();
   325|			_debugInfoStr += doLocalConf ? L"ON" : L"OFF";
   326|			_debugInfoStr += L"\r\n";
   327|
   328|			// Cloud config directory
   329|			_debugInfoStr += L"Cloud Config: ";
   330|			const wstring& cloudPath = nppParam.getNppGUI()._cloudPath;
   331|			_debugInfoStr += cloudPath.empty() ? L"OFF" : cloudPath;
   332|			_debugInfoStr += L"\r\n";
   333|
   334|			// Periodic Backup
   335|			_debugInfoStr += L"Periodic Backup: ";
   336|			_debugInfoStr += nppGui.isSnapshotMode() ? L"ON" : L"OFF";
   337|			_debugInfoStr += L"\r\n";
   338|
   339|			// Placeholders
   340|			_debugInfoStr += L"Placeholders: ";
   341|			_debugInfoStr += nppGui._keepSessionAbsentFileEntries ? L"ON" : L"OFF";
   342|			_debugInfoStr += L"\r\n";
   343|
   344|			// SC_TECHNOLOGY
   345|			_debugInfoStr += L"Scintilla Rendering Mode: ";
   346|			switch (nppGui._writeTechnologyEngine)
   347|			{
   348|				case defaultTechnology:
   349|					_debugInfoStr += L"SC_TECHNOLOGY_DEFAULT (0)";
   350|					break;
   351|				case directWriteTechnology:
   352|					_debugInfoStr += L"SC_TECHNOLOGY_DIRECTWRITE (1)";
   353|					break;
   354|				case directWriteRetainTechnology:
   355|					_debugInfoStr += L"SC_TECHNOLOGY_DIRECTWRITERETAIN (2)";
   356|					break;
   357|				case directWriteDcTechnology:
   358|					_debugInfoStr += L"SC_TECHNOLOGY_DIRECTWRITEDC (3)";
   359|					break;
   360|				case directWriteDX11Technology:
   361|					_debugInfoStr += L"SC_TECHNOLOGY_DIRECT_WRITE_1 (4)";
   362|					break;
   363|				case directWriteTechnologyUnavailable:
   364|					_debugInfoStr += L"DirectWrite Technology Unavailable (5, same as SC_TECHNOLOGY_DEFAULT)";
   365|					break;
   366|				default:
   367|					_debugInfoStr += L"unknown (" + std::to_wstring(nppGui._writeTechnologyEngine) + L")";
   368|			}
   369|			_debugInfoStr += L"\r\n";
   370|
   371|			// Multi-instance
   372|			_debugInfoStr += L"Multi-instance Mode: ";
   373|			switch (nppGui._multiInstSetting)
   374|			{
   375|				case monoInst:
   376|					_debugInfoStr += L"monoInst";
   377|					break;
   378|				case multiInstOnSession:
   379|					_debugInfoStr += L"multiInstOnSession";
   380|					break;
   381|				case multiInst:
   382|					_debugInfoStr += L"multiInst";
   383|					break;
   384|				default:
   385|					_debugInfoStr += L"unknown(" + std::to_wstring(nppGui._multiInstSetting) + L")";
   386|			}
   387|			_debugInfoStr += L"\r\n";
   388|
   389|			// asNotepad
   390|			_debugInfoStr += L"asNotepad: ";
   391|			_debugInfoStr += nppParam.isAsNotepadStyle() ? L"ON" : L"OFF";
   392|			_debugInfoStr += L"\r\n";
   393|
   394|			// File Status Auto-Detection
   395|			_debugInfoStr += L"File Status Auto-Detection: ";
   396|			if (nppGui._fileAutoDetection == cdDisabled)
   397|			{
   398|				_debugInfoStr += L"cdDisabled";
   399|			}
   400|			else
   401|			{
   402|				if (nppGui._fileAutoDetection & cdEnabledOld)
   403|					_debugInfoStr += L"cdEnabledOld (for all opened files/tabs)";
   404|				else if (nppGui._fileAutoDetection & cdEnabledNew)
   405|					_debugInfoStr += L"cdEnabledNew (for current file/tab only)";
   406|				else
   407|					_debugInfoStr += L"cdUnknown (?!)";
   408|
   409|				if (nppGui._fileAutoDetection & cdAutoUpdate)
   410|					_debugInfoStr += L" + cdAutoUpdate";
   411|				if (nppGui._fileAutoDetection & cdGo2end)
   412|					_debugInfoStr += L" + cdGo2end";
   413|			}
   414|			_debugInfoStr += L"\r\n";
   415|
   416|			// Dark Mode
   417|			_debugInfoStr += L"Dark Mode: ";
   418|			_debugInfoStr += nppGui._darkmode._isEnabled ? L"ON" : L"OFF";
   419|			_debugInfoStr += L"\r\n";
   420|
   421|			// Display Info
   422|			_debugInfoStr += L"Display Info:";
   423|			{
   424|				HDC hdc = ::GetDC(nullptr); // desktop DC
   425|				if (hdc)
   426|				{
   427|					_debugInfoStr += L"\r\n    primary monitor: " + std::to_wstring(::GetDeviceCaps(hdc, HORZRES));
   428|					_debugInfoStr += L"x" + std::to_wstring(::GetDeviceCaps(hdc, VERTRES));
   429|					_debugInfoStr += L", scaling " + std::to_wstring(::GetDeviceCaps(hdc, LOGPIXELSX) * 100 / 96);
   430|					_debugInfoStr += L"%";
   431|					::ReleaseDC(nullptr, hdc);
   432|				}
   433|				_debugInfoStr += L"\r\n    visible monitors count: " + std::to_wstring(::GetSystemMetrics(SM_CMONITORS));
   434|				AppendDisplayAdaptersInfo(_debugInfoStr, 4); // survey up to 4 potential graphics card Registry records
   435|			}
   436|			_debugInfoStr += L"\r\n";
   437|
   438|			// OS information
   439|			HKEY hKey = nullptr;
   440|			DWORD dataSize = 0;
   441|
   442|			constexpr size_t bufSize = 96;
   443|			wchar_t szProductName[bufSize] = {'\0'};
   444|			constexpr size_t bufSizeBuildNumber = 32;
   445|			wchar_t szCurrentBuildNumber[bufSizeBuildNumber] = {'\0'};
   446|			wchar_t szReleaseId[32] = {'\0'};
   447|			DWORD dwUBR = 0;
   448|			constexpr size_t bufSizeUBR = 12;
   449|			wchar_t szUBR[bufSizeUBR] = L"0";
   450|
   451|			// NOTE: RegQueryValueExW is not guaranteed to return null-terminated strings
   452|			if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
   453|			{
   454|				dataSize = sizeof(szProductName);
   455|				RegQueryValueExW(hKey, L"ProductName", NULL, NULL, reinterpret_cast<LPBYTE>(szProductName), &dataSize);
   456|				szProductName[sizeof(szProductName) / sizeof(wchar_t) - 1] = '\0';
   457|
   458|				dataSize = sizeof(szReleaseId);
   459|				if(RegQueryValueExW(hKey, L"DisplayVersion", NULL, NULL, reinterpret_cast<LPBYTE>(szReleaseId), &dataSize) != ERROR_SUCCESS)
   460|				{
   461|					dataSize = sizeof(szReleaseId);
   462|					RegQueryValueExW(hKey, L"ReleaseId", NULL, NULL, reinterpret_cast<LPBYTE>(szReleaseId), &dataSize);
   463|				}
   464|				szReleaseId[sizeof(szReleaseId) / sizeof(wchar_t) - 1] = '\0';
   465|
   466|				dataSize = sizeof(szCurrentBuildNumber);
   467|				RegQueryValueExW(hKey, L"CurrentBuildNumber", NULL, NULL, reinterpret_cast<LPBYTE>(szCurrentBuildNumber), &dataSize);
   468|				szCurrentBuildNumber[sizeof(szCurrentBuildNumber) / sizeof(wchar_t) - 1] = '\0';
   469|
   470|				dataSize = sizeof(DWORD);
   471|				if (RegQueryValueExW(hKey, L"UBR", NULL, NULL, reinterpret_cast<LPBYTE>(&dwUBR), &dataSize) == ERROR_SUCCESS)
   472|				{
   473|					swprintf(szUBR, bufSizeUBR, L"%u", dwUBR);
   474|				}
   475|
   476|				RegCloseKey(hKey);
   477|			}
   478|
   479|			// Get alternative OS information
   480|			if (szProductName[0] == '\0')
   481|			{
   482|				swprintf(szProductName, bufSize, L"%s", (NppParameters::getInstance()).getWinVersionStr().c_str());
   483|			}
   484|			else if (NppDarkMode::isWindows11())
   485|			{
   486|				wstring tmpProductName = szProductName;
   487|				constexpr size_t strLen = 10U;
   488|				const wchar_t strWin10[strLen + 1U] = L"Windows 10";
   489|				const size_t pos = tmpProductName.find(strWin10);
   490|				if (pos < (bufSize - strLen - 1U))
   491|				{
   492|					tmpProductName.replace(pos, strLen, L"Windows 11");
   493|					swprintf(szProductName, bufSize, L"%s", tmpProductName.c_str());
   494|				}
   495|			}
   496|
   497|			if (szCurrentBuildNumber[0] == '\0')
   498|			{
   499|				DWORD dwVersion = GetVersion();
   500|				if (dwVersion < 0x80000000)
   501|