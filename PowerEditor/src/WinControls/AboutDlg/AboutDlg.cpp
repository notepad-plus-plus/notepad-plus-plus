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


#include "AboutDlg.h"
#include "Parameters.h"
#include "localization.h"
#if defined __has_include
#if __has_include ("NppLibsVersion.h")
#include "NppLibsVersion.h"
#endif
#endif

using namespace std;

#ifdef _MSC_VER
#pragma warning(disable : 4996) // for GetVersion()
#endif

intptr_t CALLBACK AboutDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			HWND compileDateHandle = ::GetDlgItem(_hSelf, IDC_BUILD_DATETIME);
			wstring buildTime = L"Build time: ";

			WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
			buildTime +=  wmc.char2wchar(__DATE__, CP_ACP);
			buildTime += L" - ";
			buildTime +=  wmc.char2wchar(__TIME__, CP_ACP);

			NppParameters& nppParam = NppParameters::getInstance();
			LPCTSTR bitness = nppParam.archType() == IMAGE_FILE_MACHINE_I386 ? L"(32-bit)" : nppParam.archType() == IMAGE_FILE_MACHINE_AMD64 ? L"(64-bit)" : L"(ARM 64-bit)";
			::SetDlgItemText(_hSelf, IDC_VERSION_BIT, bitness);

			::SendMessage(compileDateHandle, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buildTime.c_str()));
			::EnableWindow(compileDateHandle, FALSE);

            HWND licenceEditHandle = ::GetDlgItem(_hSelf, IDC_LICENCE_EDIT);
			::SendMessage(licenceEditHandle, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(LICENCE_TXT));

            //_emailLink.init(_hInst, _hSelf);
			//_emailLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), L"mailto:don.h@free.fr";
			//_emailLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), L"https://notepad-plus-plus.org/news/v781-free-uyghur-edition/";
			//_emailLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), L"https://notepad-plus-plus.org/news/v792-stand-with-hong-kong/";
			//_emailLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), L"https://notepad-plus-plus.org/news/v791-pour-samuel-paty/";
			//_pageLink.create(::GetDlgItem(_hSelf, IDC_HOME_ADDR), L"https://notepad-plus-plus.org/news/v843-unhappy-users-edition/";
			//_pageLink.create(::GetDlgItem(_hSelf, IDC_HOME_ADDR), L"https://notepad-plus-plus.org/news/v844-happy-users-edition/";
            //_pageLink.create(::GetDlgItem(_hSelf, IDC_HOME_ADDR), L"https://notepad-plus-plus.org/news/v86-20thyearanniversary";
            //_pageLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), L"https://notepad-plus-plus.org/news/v87-about-taiwan/");
            
			_pageLink.init(_hInst, _hSelf);
            //_pageLink.create(::GetDlgItem(_hSelf, IDC_HOME_ADDR), L"https://notepad-plus-plus.org/");
			_pageLink.create(::GetDlgItem(_hSelf, IDC_AUTHOR_NAME), L"https://notepad-plus-plus.org/news/v879-we-are-with-ukraine/");

			return TRUE;
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::autoThemeChildControls(_hSelf);
			if (_hIcon != nullptr)
			{
				::DestroyIcon(_hIcon);
				_hIcon = nullptr;
			}
			return TRUE;
		}

		case WM_DPICHANGED:
		{
			_dpiManager.setDpiWP(wParam);
			destroy();
			//setPositionDpi(lParam);
			getWindowRect(_rc);

			return TRUE;
		}

		case WM_DRAWITEM:
		{
			const int iconSize = _dpiManager.scale(80);
			if (_hIcon == nullptr)
			{
				//DPIManagerV2::loadIcon(_hInst, MAKEINTRESOURCE(NppDarkMode::isEnabled() ? IDI_CHAMELEON_DM : IDI_CHAMELEON), iconSize, iconSize, &_hIcon);
				DPIManagerV2::loadIcon(_hInst, MAKEINTRESOURCE(IDI_WITHUKRAINE), iconSize, iconSize, &_hIcon);
				//DPIManagerV2::loadIcon(_hInst, MAKEINTRESOURCE(NppDarkMode::isEnabled() ? IDI_TAIWANSSOVEREIGNTY_DM : IDI_TAIWANSSOVEREIGNTY), iconSize, iconSize, &_hIcon);
			}

			//HICON hIcon = (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_JESUISCHARLIE), IMAGE_ICON, 64, 64, LR_DEFAULTSIZE);
			//HICON hIcon = (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_GILETJAUNE), IMAGE_ICON, 64, 64, LR_DEFAULTSIZE);
			//HICON hIcon = (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_SAMESEXMARRIAGE), IMAGE_ICON, 64, 64, LR_DEFAULTSIZE);

			auto pdis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
			::DrawIconEx(pdis->hDC, 0, 0, _hIcon, iconSize, iconSize, 0, nullptr, DI_NORMAL);

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDCANCEL :
				case IDOK :
					display(false);
					return TRUE;

				default :
					break;
			}
			break;
		}

		case WM_DESTROY:
		{
			destroy();
			return TRUE;
		}
	}
	return FALSE;
}

void AboutDlg::doDialog()
{
	if (!isCreated())
		create(IDD_ABOUTBOX);

	// Adjust the position of AboutBox
	moveForDpiChange();
	goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);
}


intptr_t CALLBACK DebugInfoDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			NppParameters& nppParam = NppParameters::getInstance();
			NppGUI& nppGui = nppParam.getNppGUI();

			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			// Notepad++ version
			_debugInfoStr = NOTEPAD_PLUS_VERSION;
			_debugInfoStr += nppParam.archType() == IMAGE_FILE_MACHINE_I386 ? L"   (32-bit)" : nppParam.archType() == IMAGE_FILE_MACHINE_AMD64 ? L"   (64-bit)" : L"   (ARM 64-bit)";
			_debugInfoStr += L"\r\n";

			// Build time
			_debugInfoStr += L"Build time : ";
			wstring buildTime;
			WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
			buildTime += wmc.char2wchar(__DATE__, CP_ACP);
			buildTime += L" - ";
			buildTime += wmc.char2wchar(__TIME__, CP_ACP);
			_debugInfoStr += buildTime;
			_debugInfoStr += L"\r\n";

#if defined(__clang__)
			_debugInfoStr += L"Built with : Clang ";
			_debugInfoStr += wmc.char2wchar(__clang_version__, CP_ACP);
			_debugInfoStr += L"\r\n";
#elif defined(__GNUC__)
			_debugInfoStr += L"Built with : GCC ";
			_debugInfoStr += wmc.char2wchar(__VERSION__, CP_ACP);
			_debugInfoStr += L"\r\n";
#elif !defined(_MSC_VER)
			_debugInfoStr += L"Built with : (unknown)\r\n";
#endif

			// Scintilla/Lexilla version
			_debugInfoStr += L"Scintilla/Lexilla included : ";
			{
				string strSciLexVer = NPP_SCINTILLA_VERSION;
				strSciLexVer += "/";
				strSciLexVer += NPP_LEXILLA_VERSION;
				_debugInfoStr += wmc.char2wchar(strSciLexVer.c_str(), CP_ACP);
			}
			_debugInfoStr += L"\r\n";

			// Boost Regex version
			_debugInfoStr += L"Boost Regex included : ";
			_debugInfoStr += wmc.char2wchar(NPP_BOOST_REGEX_VERSION, CP_ACP);
			_debugInfoStr += L"\r\n";

			// Binary path
			_debugInfoStr += L"Path : ";
			wchar_t nppFullPath[MAX_PATH]{};
			::GetModuleFileName(NULL, nppFullPath, MAX_PATH);
			_debugInfoStr += nppFullPath;
			_debugInfoStr += L"\r\n";

			// Command line as specified for program launch
			// The _cmdLinePlaceHolder will be replaced later by refreshDebugInfo()
			_debugInfoStr += L"Command Line : ";
			_debugInfoStr += _cmdLinePlaceHolder;
			_debugInfoStr += L"\r\n";

			// Administrator mode
			_debugInfoStr += L"Admin mode : ";
			_debugInfoStr += _isAdmin ? L"ON" : L"OFF";
			_debugInfoStr += L"\r\n";

			// local conf
			_debugInfoStr += L"Local Conf mode : ";
			bool doLocalConf = (NppParameters::getInstance()).isLocal();
			_debugInfoStr += doLocalConf ? L"ON" : L"OFF";
			_debugInfoStr += L"\r\n";

			// Cloud config directory
			_debugInfoStr += L"Cloud Config : ";
			const wstring& cloudPath = nppParam.getNppGUI()._cloudPath;
			_debugInfoStr += cloudPath.empty() ? L"OFF" : cloudPath;
			_debugInfoStr += L"\r\n";

			// Periodic Backup
			_debugInfoStr += L"Periodic Backup : ";
			_debugInfoStr += nppGui.isSnapshotMode() ? L"ON" : L"OFF";
			_debugInfoStr += L"\r\n";

			// Placeholders
			_debugInfoStr += L"Placeholders : ";
			_debugInfoStr += nppGui._keepSessionAbsentFileEntries ? L"ON" : L"OFF";
			_debugInfoStr += L"\r\n";

			// SC_TECHNOLOGY
			_debugInfoStr += L"Scintilla Rendering Mode : ";
			switch (nppGui._writeTechnologyEngine)
			{
				case defaultTechnology:
					_debugInfoStr += L"SC_TECHNOLOGY_DEFAULT (0)";
					break;
				case directWriteTechnology:
					_debugInfoStr += L"SC_TECHNOLOGY_DIRECTWRITE (1)";
					break;
				case directWriteRetainTechnology:
					_debugInfoStr += L"SC_TECHNOLOGY_DIRECTWRITERETAIN (2)";
					break;
				case directWriteDcTechnology:
					_debugInfoStr += L"SC_TECHNOLOGY_DIRECTWRITEDC (3)";
					break;
				case directWriteDX11Technology:
					_debugInfoStr += L"SC_TECHNOLOGY_DIRECT_WRITE_1 (4)";
					break;
				case directWriteTechnologyUnavailable:
					_debugInfoStr += L"DirectWrite Technology Unavailable (5, same as SC_TECHNOLOGY_DEFAULT)";
					break;
				default:
					_debugInfoStr += L"unknown (" + std::to_wstring(nppGui._writeTechnologyEngine) + L")";
			}
			_debugInfoStr += L"\r\n";

			// Multi-instance
			_debugInfoStr += L"Multi-instance Mode : ";
			switch (nppGui._multiInstSetting)
			{
				case monoInst:
					_debugInfoStr += L"monoInst";
					break;
				case multiInstOnSession:
					_debugInfoStr += L"multiInstOnSession";
					break;
				case multiInst:
					_debugInfoStr += L"multiInst";
					break;
				default:
					_debugInfoStr += L"unknown(" + std::to_wstring(nppGui._multiInstSetting) + L")";
			}
			_debugInfoStr += L"\r\n";

			// File Status Auto-Detection
			_debugInfoStr += L"File Status Auto-Detection : ";
			if (nppGui._fileAutoDetection == cdDisabled)
			{
				_debugInfoStr += L"cdDisabled";
			}
			else
			{
				if (nppGui._fileAutoDetection & cdEnabledOld)
					_debugInfoStr += L"cdEnabledOld (for all opened files/tabs)";
				else if (nppGui._fileAutoDetection & cdEnabledNew)
					_debugInfoStr += L"cdEnabledNew (for current file/tab only)";
				else
					_debugInfoStr += L"cdUnknown (?!)";

				if (nppGui._fileAutoDetection & cdAutoUpdate)
					_debugInfoStr += L" + cdAutoUpdate";
				if (nppGui._fileAutoDetection & cdGo2end)
					_debugInfoStr += L" + cdGo2end";
			}
			_debugInfoStr += L"\r\n";

			// Dark Mode
			_debugInfoStr += L"Dark Mode : ";
			_debugInfoStr += nppGui._darkmode._isEnabled ? L"ON" : L"OFF";
			_debugInfoStr += L"\r\n";

			// OS information
			HKEY hKey = nullptr;
			DWORD dataSize = 0;

			constexpr size_t bufSize = 96;
			wchar_t szProductName[bufSize] = {'\0'};
			constexpr size_t bufSizeBuildNumber = 32;
			wchar_t szCurrentBuildNumber[bufSizeBuildNumber] = {'\0'};
			wchar_t szReleaseId[32] = {'\0'};
			DWORD dwUBR = 0;
			constexpr size_t bufSizeUBR = 12;
			wchar_t szUBR[bufSizeUBR] = L"0";

			// NOTE: RegQueryValueExW is not guaranteed to return null-terminated strings
			if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
			{
				dataSize = sizeof(szProductName);
				RegQueryValueExW(hKey, L"ProductName", NULL, NULL, reinterpret_cast<LPBYTE>(szProductName), &dataSize);
				szProductName[sizeof(szProductName) / sizeof(wchar_t) - 1] = '\0';

				dataSize = sizeof(szReleaseId);
				if(RegQueryValueExW(hKey, L"DisplayVersion", NULL, NULL, reinterpret_cast<LPBYTE>(szReleaseId), &dataSize) != ERROR_SUCCESS)
				{
					dataSize = sizeof(szReleaseId);
					RegQueryValueExW(hKey, L"ReleaseId", NULL, NULL, reinterpret_cast<LPBYTE>(szReleaseId), &dataSize);
				}
				szReleaseId[sizeof(szReleaseId) / sizeof(wchar_t) - 1] = '\0';

				dataSize = sizeof(szCurrentBuildNumber);
				RegQueryValueExW(hKey, L"CurrentBuildNumber", NULL, NULL, reinterpret_cast<LPBYTE>(szCurrentBuildNumber), &dataSize);
				szCurrentBuildNumber[sizeof(szCurrentBuildNumber) / sizeof(wchar_t) - 1] = '\0';

				dataSize = sizeof(DWORD);
				if (RegQueryValueExW(hKey, L"UBR", NULL, NULL, reinterpret_cast<LPBYTE>(&dwUBR), &dataSize) == ERROR_SUCCESS)
				{
					swprintf(szUBR, bufSizeUBR, L"%u", dwUBR);
				}

				RegCloseKey(hKey);
			}

			// Get alternative OS information
			if (szProductName[0] == '\0')
			{
				swprintf(szProductName, bufSize, L"%s", (NppParameters::getInstance()).getWinVersionStr().c_str());
			}
			else if (NppDarkMode::isWindows11())
			{
				wstring tmpProductName = szProductName;
				constexpr size_t strLen = 10U;
				const wchar_t strWin10[strLen + 1U] = L"Windows 10";
				const size_t pos = tmpProductName.find(strWin10);
				if (pos < (bufSize - strLen - 1U))
				{
					tmpProductName.replace(pos, strLen, L"Windows 11");
					swprintf(szProductName, bufSize, L"%s", tmpProductName.c_str());
				}
			}

			if (szCurrentBuildNumber[0] == '\0')
			{
				DWORD dwVersion = GetVersion();
				if (dwVersion < 0x80000000)
				{
					swprintf(szCurrentBuildNumber, bufSizeBuildNumber, L"%u", HIWORD(dwVersion));
				}
			}

			_debugInfoStr += L"OS Name : ";
			_debugInfoStr += szProductName;
			_debugInfoStr += L" (";
			_debugInfoStr += (NppParameters::getInstance()).getWinVerBitStr();
			_debugInfoStr += L")";
			_debugInfoStr += L"\r\n";

			if (szReleaseId[0] != '\0')
			{
				_debugInfoStr += L"OS Version : ";
				_debugInfoStr += szReleaseId;
				_debugInfoStr += L"\r\n";
			}

			if (szCurrentBuildNumber[0] != '\0')
			{
				_debugInfoStr += L"OS Build : ";
				_debugInfoStr += szCurrentBuildNumber;
				_debugInfoStr += L".";
				_debugInfoStr += szUBR;
				_debugInfoStr += L"\r\n";
			}

			{
				constexpr size_t bufSizeACP = 32;
				wchar_t szACP[bufSizeACP] = { '\0' };
				swprintf(szACP, bufSizeACP, L"%u", ::GetACP());
				_debugInfoStr += L"Current ANSI codepage : ";
 				_debugInfoStr += szACP;
				_debugInfoStr += L"\r\n";
			}

			// Detect WINE
			PWINEGETVERSION pWGV = nullptr;
			HMODULE hNtdllModule = GetModuleHandle(L"ntdll.dll");
			if (hNtdllModule)
			{
				pWGV = reinterpret_cast<PWINEGETVERSION>(GetProcAddress(hNtdllModule, "wine_get_version"));
			}

			if (pWGV != nullptr)
			{
				constexpr size_t bufSizeWineVer = 32;
				wchar_t szWINEVersion[bufSizeWineVer] = { '\0' };
				swprintf(szWINEVersion, bufSizeWineVer, L"%hs", pWGV());

				_debugInfoStr += L"WINE : ";
				_debugInfoStr += szWINEVersion;
				_debugInfoStr += L"\r\n";
			}

			// Plugins
			_debugInfoStr += L"Plugins : ";
			_debugInfoStr += _loadedPlugins.length() == 0 ? L"none" : _loadedPlugins;
			_debugInfoStr += L"\r\n";

			return TRUE;
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::autoThemeChildControls(_hSelf);
			return TRUE;
		}

		case WM_DPICHANGED:
		{
			_dpiManager.setDpiWP(wParam);
			setPositionDpi(lParam);
			getWindowRect(_rc);

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDCANCEL:
				case IDOK:
					display(false);
					return TRUE;

				case IDC_DEBUGINFO_COPYLINK:
				{
					// Visual effect
					::SendDlgItemMessage(_hSelf, IDC_DEBUGINFO_EDIT, EM_SETSEL, 0, _debugInfoDisplay.length() - 1);

					// Copy to clipboard
					str2Clipboard(_debugInfoDisplay, _hSelf);

					// Set focus to edit control
					::SendMessage(_hSelf, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(::GetDlgItem(_hSelf, IDC_DEBUGINFO_EDIT)), TRUE);

					return TRUE;
				}
				default:
					break;
			}
			break;
		}

		case WM_DESTROY:
		{
			return TRUE;
		}
	}
	return FALSE;
}

void DebugInfoDlg::doDialog()
{
	if (!isCreated())
		create(IDD_DEBUGINFOBOX);

	// Refresh the Debug Information.
	// For example, the command line parameters may have changed since this dialog was last opened during this session.
	refreshDebugInfo();

	// Adjust the position of DebugBox
	moveForDpiChange();
	goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);
}

void DebugInfoDlg::refreshDebugInfo()
{
	_debugInfoDisplay = _debugInfoStr;

	size_t replacePos = _debugInfoDisplay.find(_cmdLinePlaceHolder);
	if (replacePos != std::string::npos)
	{
		_debugInfoDisplay.replace(replacePos, _cmdLinePlaceHolder.length(), NppParameters::getInstance().getCmdLineString());
	}

	// Set Debug Info text and leave the text in selected state
	::SetDlgItemText(_hSelf, IDC_DEBUGINFO_EDIT, _debugInfoDisplay.c_str());
	::SendDlgItemMessage(_hSelf, IDC_DEBUGINFO_EDIT, EM_SETSEL, 0, _debugInfoDisplay.length() - 1);
	::SetFocus(::GetDlgItem(_hSelf, IDC_DEBUGINFO_EDIT));
}


void DoSaveOrNotBox::doDialog(bool isRTL)
{

	if (isRTL)
	{
		DLGTEMPLATE *pMyDlgTemplate = NULL;
		HGLOBAL hMyDlgTemplate = makeRTLResource(IDD_DOSAVEORNOTBOX, &pMyDlgTemplate);
		::DialogBoxIndirectParam(_hInst, pMyDlgTemplate, _hParent, dlgProc, reinterpret_cast<LPARAM>(this));
		::GlobalFree(hMyDlgTemplate);
	}
	else
		::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_DOSAVEORNOTBOX), _hParent, dlgProc, reinterpret_cast<LPARAM>(this));
}

void DoSaveOrNotBox::changeLang()
{
	wstring msg;
	wstring defaultMessage = L"Save file \"$STR_REPLACE$\" ?";
	NativeLangSpeaker* nativeLangSpeaker = NppParameters::getInstance().getNativeLangSpeaker();

	if (nativeLangSpeaker->changeDlgLang(_hSelf, "DoSaveOrNot"))
	{
		constexpr unsigned char len = 255;
		wchar_t text[len]{};
		::GetDlgItemText(_hSelf, IDC_DOSAVEORNOTTEXT, text, len);
		msg = text;
	}

	if (msg.empty())
		msg = defaultMessage;

	msg = stringReplace(msg, L"$STR_REPLACE$", _fn);
	::SetDlgItemText(_hSelf, IDC_DOSAVEORNOTTEXT, msg.c_str());
}

intptr_t CALLBACK DoSaveOrNotBox::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG :
		{
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			changeLang();
			::EnableWindow(::GetDlgItem(_hSelf, IDRETRY), _isMulti);
			::EnableWindow(::GetDlgItem(_hSelf, IDIGNORE), _isMulti);
			goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);
			return TRUE;
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_DPICHANGED:
		{
			_dpiManager.setDpiWP(wParam);
			setPositionDpi(lParam);

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
				{
					::EndDialog(_hSelf, -1);
					clickedButtonId = IDCANCEL;
					return TRUE;
				}

				case IDYES:
				{
					::EndDialog(_hSelf, 0);
					clickedButtonId = IDYES;
					return TRUE;
				}

				case IDNO:
				{
					::EndDialog(_hSelf, 0);
					clickedButtonId = IDNO;
					return TRUE;
				}

				case IDIGNORE:
				{
					::EndDialog(_hSelf, 0);
					clickedButtonId = IDIGNORE;
					return TRUE;
				}

				case IDRETRY:
				{
					::EndDialog(_hSelf, 0);
					clickedButtonId = IDRETRY;
					return TRUE;
				}
			}
			break;
		}
		default:
			return FALSE;
	}
	return FALSE;
}


void DoSaveAllBox::doDialog(bool isRTL)
{

	if (isRTL)
	{
		DLGTEMPLATE* pMyDlgTemplate = NULL;
		HGLOBAL hMyDlgTemplate = makeRTLResource(IDD_DOSAVEALLBOX, &pMyDlgTemplate);
		::DialogBoxIndirectParam(_hInst, pMyDlgTemplate, _hParent, dlgProc, reinterpret_cast<LPARAM>(this));
		::GlobalFree(hMyDlgTemplate);
	}
	else
		::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_DOSAVEALLBOX), _hParent, dlgProc, reinterpret_cast<LPARAM>(this));
}

void DoSaveAllBox::changeLang()
{
	wstring msg;
	wstring defaultMessage = L"Are you sure you want to save all modified documents?\r\rChoose \"Always Yes\" if you don't want to see this dialog again.\rYou can re-activate this dialog in Preferences later.";
	NativeLangSpeaker* nativeLangSpeaker = NppParameters::getInstance().getNativeLangSpeaker();

	if (nativeLangSpeaker->changeDlgLang(_hSelf, "DoSaveAll"))
	{
		constexpr size_t len = 1024;
		wchar_t text[len]{};
		::GetDlgItemText(_hSelf, IDC_DOSAVEALLTEXT, text, len);
		msg = text;
	}

	if (msg.empty())
		msg = defaultMessage;

	::SetDlgItemText(_hSelf, IDC_DOSAVEALLTEXT, msg.c_str());
}

intptr_t CALLBACK DoSaveAllBox::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

		changeLang();
		goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);
		return TRUE;
	}

	case WM_CTLCOLORDLG:
	case WM_CTLCOLORSTATIC:
	{
		return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
	}

	case WM_PRINTCLIENT:
	{
		if (NppDarkMode::isEnabled())
		{
			return TRUE;
		}
		break;
	}

	case WM_DPICHANGED:
	{
		_dpiManager.setDpiWP(wParam);
		setPositionDpi(lParam);

		return TRUE;
	}

	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
			case IDCANCEL:
			{
				::EndDialog(_hSelf, -1);
				clickedButtonId = IDCANCEL;
				return TRUE;
			}

			case IDYES:
			{
				::EndDialog(_hSelf, 0);
				clickedButtonId = IDYES;
				return TRUE;
			}

			case IDNO:
			{
				::EndDialog(_hSelf, 0);
				clickedButtonId = IDNO;
				return TRUE;
			}

			case IDRETRY:
			{
				::EndDialog(_hSelf, 0);
				clickedButtonId = IDRETRY;
				return TRUE;
			}
		}
		break;
	}
	default:
		return FALSE;
	}
	return FALSE;
}
