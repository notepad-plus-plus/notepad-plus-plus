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

#include "regExtDlg.h"
#include "resource.h"
#include "Parameters.h"



const wchar_t* nppName   = L"Notepad++_file";
const wchar_t* nppBackup = L"Notepad++_backup";
const wchar_t* nppDoc    = L"Notepad++ Document";

const int nbSupportedLang = 10;
const int nbExtMax = 28;
const int extNameMax = 18;


const wchar_t defExtArray[nbSupportedLang][nbExtMax][extNameMax] =
{
	{L"Notepad",
		L".txt", L".log"
	},
	{L"ms ini/inf",
		L".ini", L".inf"
	},
	{L"c, c++, objc",
		L".h", L".hh", L".hpp", L".hxx", L".c", L".cpp", L".cxx", L".cc",
		L".m", L".mm",
		L".vcxproj", L".vcproj", L".props", L".vsprops", L".manifest"
	},
	{L"java, c#, pascal",
		L".java", L".cs", L".pas", L".pp", L".inc"
	},
	{L"web script",
		L".html", L".htm", L".shtml", L".shtm", L".hta",
		L".asp", L".aspx",
		L".css", L".js", L".json", L".mjs", L".jsm", L".jsp",
		L".php", L".php3", L".php4", L".php5", L".phps", L".phpt", L".phtml",
		L".xml", L".xhtml", L".xht", L".xul", L".kml", L".xaml", L".xsml"
	},
	{L"public script",
		L".sh", L".bsh", L".bash", L".bat", L".cmd", L".nsi",
		L".nsh", L".lua", L".pl", L".pm", L".py"
	},
	{L"property script",
		L".rc", L".as", L".mx", L".vb", L".vbs"
	},
	{L"fortran, TeX, SQL",
		L".f", L".for", L".f90", L".f95", L".f2k", L".tex", L".sql"
	},
	{L"misc",
		L".nfo", L".mak"
	},
	{L"customize"}
};

void RegExtDlg::doDialog(bool isRTL)
{
	if (isRTL)
	{
		DLGTEMPLATE *pMyDlgTemplate = nullptr;
		HGLOBAL hMyDlgTemplate = makeRTLResource(IDD_REGEXT_BOX, &pMyDlgTemplate);
		::DialogBoxIndirectParam(_hInst, pMyDlgTemplate, _hParent, dlgProc, reinterpret_cast<LPARAM>(this));
		::GlobalFree(hMyDlgTemplate);
	}
	else
		::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_REGEXT_BOX), _hParent, dlgProc, reinterpret_cast<LPARAM>(this));
}

intptr_t CALLBACK RegExtDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	NppParameters& nppParam = NppParameters::getInstance();
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			getRegisteredExts();
			getDefSupportedExts();
			::EnableWindow(::GetDlgItem(_hSelf, IDC_ADDFROMLANGEXT_BUTTON), false);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_REMOVEEXT_BUTTON), false);

			if (!nppParam.isAdmin())
			{
				::EnableWindow(::GetDlgItem(_hSelf, IDC_REGEXT_LANG_LIST), false);
				::EnableWindow(::GetDlgItem(_hSelf, IDC_REGEXT_LANGEXT_LIST), false);
				::EnableWindow(::GetDlgItem(_hSelf, IDC_REGEXT_REGISTEREDEXTS_LIST), false);
			}
			else
			{
				::ShowWindow(::GetDlgItem(_hSelf, IDC_ADMINMUSTBEONMSG_STATIC), SW_HIDE);
				::SendDlgItemMessage(_hSelf, IDC_CUSTOMEXT_EDIT, EM_SETLIMITTEXT, extNameMax - 1, 0);
			}
			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			return NppDarkMode::onCtlColorCtrl(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORLISTBOX:
		{
			return NppDarkMode::onCtlColorListbox(wParam, lParam);
		}

		case WM_CTLCOLORDLG:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORSTATIC:
		{
			auto hdcStatic = reinterpret_cast<HDC>(wParam);
			auto dlgCtrlID = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam));

			bool isStaticText = dlgCtrlID == IDC_SUPPORTEDEXTS_STATIC || dlgCtrlID == IDC_REGISTEREDEXTS_STATIC;
			//set the static text colors to show enable/disable instead of ::EnableWindow which causes blurry text
			if (isStaticText)
			{
				return NppDarkMode::onCtlColorDlgStaticText(hdcStatic, nppParam.isAdmin());
			}
			return NppDarkMode::onCtlColorDlg(hdcStatic);
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_COMMAND:
		{
			// Handle File association list extension
			if (LOWORD(wParam) == IDC_REGEXT_LANGEXT_LIST || LOWORD(wParam) == IDC_REGEXT_REGISTEREDEXTS_LIST)
			{
				// On double click an item in the list then toggle the item between both lists
				// by simulating "<-" or "->" button clicked
				if (HIWORD(wParam) == LBN_DBLCLK)
				{
					// Check whether click happened on a item not in empty area
					if (-1 != ::SendDlgItemMessage(_hSelf, LOWORD(wParam), LB_GETCURSEL, 0, 0))
					{
						HWND(lParam) == ::GetDlgItem(_hSelf, IDC_REGEXT_LANGEXT_LIST) ?
							::SendMessage(_hSelf, WM_COMMAND, IDC_ADDFROMLANGEXT_BUTTON, 0) :
							::SendMessage(_hSelf, WM_COMMAND, IDC_REMOVEEXT_BUTTON, 0);
					}
					return TRUE;
				}
			}

			switch (wParam)
			{
				case IDC_ADDFROMLANGEXT_BUTTON :
				{
					writeNppPath();

					wchar_t ext2Add[extNameMax] = L"";
					if (!_isCustomize)
					{
						auto index2Add = ::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANGEXT_LIST, LB_GETCURSEL, 0, 0);
						auto lbTextLen = ::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANGEXT_LIST, LB_GETTEXTLEN, index2Add, 0);
						if (lbTextLen > extNameMax - 1)
							return TRUE;

						::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANGEXT_LIST, LB_GETTEXT, index2Add, reinterpret_cast<LPARAM>(ext2Add));
						addExt(ext2Add);
						::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANGEXT_LIST, LB_DELETESTRING, index2Add, 0);
					}
					else
					{
						::SendDlgItemMessage(_hSelf, IDC_CUSTOMEXT_EDIT, WM_GETTEXT, extNameMax, reinterpret_cast<LPARAM>(ext2Add));
						auto i = ::SendDlgItemMessage(_hSelf, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_FINDSTRINGEXACT, 0, reinterpret_cast<LPARAM>(ext2Add));
						if (i != LB_ERR)
							return TRUE;
						addExt(ext2Add);
						::SendDlgItemMessage(_hSelf, IDC_CUSTOMEXT_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L""));
					}
					::SendDlgItemMessage(_hSelf, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(ext2Add));
					::EnableWindow(::GetDlgItem(_hSelf, IDC_ADDFROMLANGEXT_BUTTON), false);
					return TRUE;
				}

				case IDC_REMOVEEXT_BUTTON :
				{
					wchar_t ext2Sup[extNameMax] = L"";
					auto index2Sup = ::SendDlgItemMessage(_hSelf, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_GETCURSEL, 0, 0);
					auto lbTextLen = ::SendDlgItemMessage(_hSelf, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_GETTEXTLEN, index2Sup, 0);
					if (lbTextLen > extNameMax - 1)
						return TRUE;

					::SendDlgItemMessage(_hSelf, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_GETTEXT, index2Sup, reinterpret_cast<LPARAM>(ext2Sup));
					if (deleteExts(ext2Sup))
						::SendDlgItemMessage(_hSelf, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_DELETESTRING, index2Sup, 0);
					auto langIndex = ::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANG_LIST, LB_GETCURSEL, 0, 0);

					::EnableWindow(::GetDlgItem(_hSelf, IDC_REMOVEEXT_BUTTON), false);

					if (langIndex != LB_ERR)
					{
						for (int i = 1 ; i < nbExtMax ; ++i)
						{
							if (!wcsicmp(ext2Sup, defExtArray[langIndex][i]))
							{
								::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANGEXT_LIST, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(ext2Sup));
								return TRUE;
							}
						}
					}
					return TRUE;
				}

				case IDCANCEL:
				{
					::EndDialog(_hSelf, 0);
					return TRUE;
				}
			}

			if (HIWORD(wParam) == EN_CHANGE)
			{
				wchar_t text[extNameMax] = L"";
				::SendDlgItemMessage(_hSelf, IDC_CUSTOMEXT_EDIT, WM_GETTEXT, extNameMax, reinterpret_cast<LPARAM>(text));
				if ((lstrlen(text) == 1) && (text[0] != '.'))
				{
					text[1] = text[0];
					text[0] = '.';
					text[2] = '\0';
					::SendDlgItemMessage(_hSelf, IDC_CUSTOMEXT_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(text));
					::SendDlgItemMessage(_hSelf, IDC_CUSTOMEXT_EDIT, EM_SETSEL, 2, 2);
				}
				::EnableWindow(::GetDlgItem(_hSelf, IDC_ADDFROMLANGEXT_BUTTON), (lstrlen(text) > 1));
				return TRUE;
			}

			if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				auto i = ::SendDlgItemMessage(_hSelf, LOWORD(wParam), LB_GETCURSEL, 0, 0);
				if (LOWORD(wParam) == IDC_REGEXT_LANG_LIST)
				{
					if (i != LB_ERR)
					{
						const size_t itemNameLen = 32;
						wchar_t itemName[itemNameLen + 1] = { '\0' };
						size_t lbTextLen = ::SendDlgItemMessage(_hSelf, LOWORD(wParam), LB_GETTEXTLEN, i, 0);
						if (lbTextLen > itemNameLen)
							return TRUE;

						::SendDlgItemMessage(_hSelf, LOWORD(wParam), LB_GETTEXT, i, reinterpret_cast<LPARAM>(itemName));

						if (!wcsicmp(defExtArray[nbSupportedLang-1][0], itemName))
						{
							::ShowWindow(::GetDlgItem(_hSelf, IDC_REGEXT_LANGEXT_LIST), SW_HIDE);
							::ShowWindow(::GetDlgItem(_hSelf, IDC_CUSTOMEXT_EDIT), SW_SHOW);
							_isCustomize = true;
						}
						else
						{
							if (_isCustomize)
							{
								::ShowWindow(::GetDlgItem(_hSelf, IDC_REGEXT_LANGEXT_LIST), SW_SHOW);
								::ShowWindow(::GetDlgItem(_hSelf, IDC_CUSTOMEXT_EDIT), SW_HIDE);

								_isCustomize = false;
							}
							LRESULT count = ::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANGEXT_LIST, LB_GETCOUNT, 0, 0);
							for (count -= 1 ; count >= 0 ; count--)
								::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANGEXT_LIST, LB_DELETESTRING, count, 0);

							for (int j = 1 ; j < nbExtMax ; ++j)
							{
								if (lstrcmp(L"", defExtArray[i][j]))
								{
									auto index = ::SendDlgItemMessage(_hSelf, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_FINDSTRINGEXACT, 0, reinterpret_cast<LPARAM>(defExtArray[i][j]));
									if (index == -1)
										::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANGEXT_LIST, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(defExtArray[i][j]));
								}
							}
						}

						::EnableWindow(::GetDlgItem(_hSelf, IDC_ADDFROMLANGEXT_BUTTON), false);
					}
				}
				else if (LOWORD(wParam) == IDC_REGEXT_LANGEXT_LIST)
				{
					if (i != LB_ERR)
						::EnableWindow(::GetDlgItem(_hSelf, IDC_ADDFROMLANGEXT_BUTTON), true);
				}
				else if (LOWORD(wParam) == IDC_REGEXT_REGISTEREDEXTS_LIST)
				{
					if (i != LB_ERR)
						::EnableWindow(::GetDlgItem(_hSelf, IDC_REMOVEEXT_BUTTON), true);
				}
			}

			// break; // no break here
			[[fallthrough]];
		}

		default :
			return FALSE;
	}
	return FALSE;
}

void RegExtDlg::getRegisteredExts()
{
	int nbRegisteredKey = getNbSubKey(HKEY_CLASSES_ROOT);
	for (int i = 0 ; i < nbRegisteredKey ; ++i)
	{
		wchar_t extName[extNameLen]{};
		//FILETIME fileTime;
		int extNameActualLen = extNameLen;
		int res = ::RegEnumKeyEx(HKEY_CLASSES_ROOT, i, extName, reinterpret_cast<LPDWORD>(&extNameActualLen), nullptr, nullptr, nullptr, nullptr);
		if ((res == ERROR_SUCCESS) && (extName[0] == '.'))
		{
			//wchar_t valName[extNameLen];
			wchar_t valData[extNameLen] = { '\0' };
			DWORD valDataLen = extNameLen * sizeof(wchar_t);
			DWORD valType = 0;
			HKEY hKey2Check;
			extNameActualLen = extNameLen;
			::RegOpenKeyEx(HKEY_CLASSES_ROOT, extName, 0, KEY_ALL_ACCESS, &hKey2Check);
			::RegQueryValueEx(hKey2Check, L"", nullptr, &valType, reinterpret_cast<LPBYTE>(valData), &valDataLen);

			if ((valType == REG_SZ) && (!lstrcmp(valData, nppName)))
				::SendDlgItemMessage(_hSelf, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(extName));
			::RegCloseKey(hKey2Check);
		}
	}
}


void RegExtDlg::getDefSupportedExts()
{
	for (int i = 0 ; i < nbSupportedLang ; ++i)
		::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANG_LIST, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(defExtArray[i][0]));
}


void RegExtDlg::addExt(wchar_t *ext)
{
	HKEY  hKey;
	DWORD dwDisp = 0;
	long  nRet;

	nRet = ::RegCreateKeyEx(HKEY_CLASSES_ROOT, ext, 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &hKey, &dwDisp);

	if (nRet == ERROR_SUCCESS)
	{
		if (dwDisp == REG_OPENED_EXISTING_KEY)
		{
			wchar_t valData[MAX_PATH] {};
			DWORD valDataLen = MAX_PATH * sizeof(wchar_t);

			int res = ::RegQueryValueEx(hKey, L"", nullptr, nullptr, reinterpret_cast<LPBYTE>(valData), &valDataLen);
			if (res == ERROR_SUCCESS)
				::RegSetValueEx(hKey, nppBackup, 0, REG_SZ, reinterpret_cast<LPBYTE>(valData), valDataLen);
		}
		::RegSetValueEx(hKey, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE *>(nppName), static_cast<DWORD>((lstrlen(nppName) + 1) * sizeof(wchar_t)));

		::RegCloseKey(hKey);
	}
}


bool RegExtDlg::deleteExts(const wchar_t *ext2Delete)
{
	HKEY hKey;
	::RegOpenKeyEx(HKEY_CLASSES_ROOT, ext2Delete, 0, KEY_ALL_ACCESS, &hKey);

	int nbValue = getNbSubValue(hKey);
	int nbSubkey = getNbSubKey(hKey);

	if ((nbValue <= 1) && (!nbSubkey))
	{
		wchar_t subKey[32] = L"\\";
		wcscat_s(subKey, ext2Delete);
		::RegDeleteKey(HKEY_CLASSES_ROOT, subKey);
	}
	else
	{
		wchar_t valData[extNameLen] = { '\0' };
		DWORD valDataLen = extNameLen*sizeof(wchar_t);
		DWORD valType = 0;
		int res = ::RegQueryValueEx(hKey, nppBackup, nullptr, &valType, (LPBYTE)valData, &valDataLen);

		if (res == ERROR_SUCCESS)
		{
			::RegSetValueEx(hKey, nullptr, 0, valType, (LPBYTE)valData, valDataLen);
			::RegDeleteValue(hKey, nppBackup);
		}
		else
			::RegDeleteValue(hKey, nullptr);
	}

	return true;
}


void RegExtDlg::writeNppPath()
{
	HKEY  hKey, hRootKey;
	DWORD dwDisp = 0;
	long  nRet = 0;
	std::wstring regStr(nppName);
	regStr += L"\\shell\\open\\command";

	nRet = ::RegCreateKeyEx(HKEY_CLASSES_ROOT, regStr.c_str(), 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &hKey, &dwDisp);


	if (nRet == ERROR_SUCCESS)
	{
		//if (dwDisp == REG_CREATED_NEW_KEY)
		{
			// Write the value for new document
			::RegOpenKeyEx(HKEY_CLASSES_ROOT, nppName, 0, KEY_ALL_ACCESS, &hRootKey);
			::RegSetValueEx(hRootKey, nullptr, 0, REG_SZ, (LPBYTE)nppDoc, static_cast<DWORD>((lstrlen(nppDoc) + 1) * sizeof(wchar_t)));
			RegCloseKey(hRootKey);

			wchar_t nppPath[MAX_PATH] = { '\0' };
			::GetModuleFileName(_hInst, nppPath, MAX_PATH);

			wchar_t nppPathParam[MAX_PATH] = L"\""; 
			wcscat_s(nppPathParam, nppPath);
			wcscat_s(nppPathParam, L"\" \"%1\"");

			::RegSetValueEx(hKey, nullptr, 0, REG_SZ, (LPBYTE)nppPathParam, static_cast<DWORD>((lstrlen(nppPathParam) + 1) * sizeof(wchar_t)));
		}
		RegCloseKey(hKey);
	}

	//Set default icon value
	regStr = nppName;
	regStr += L"\\DefaultIcon";
	nRet = ::RegCreateKeyEx(HKEY_CLASSES_ROOT, regStr.c_str(), 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &hKey, &dwDisp);

	if (nRet == ERROR_SUCCESS)
	{
		//if (dwDisp == REG_CREATED_NEW_KEY)
		{
			wchar_t nppPath[MAX_PATH] = { '\0' };
			::GetModuleFileName(_hInst, nppPath, MAX_PATH);

			wchar_t nppPathParam[MAX_PATH] = L"\"";
			wcscat_s(nppPathParam, nppPath);
			wcscat_s(nppPathParam, L"\",0");

			::RegSetValueEx(hKey, nullptr, 0, REG_SZ, (LPBYTE)nppPathParam, static_cast<DWORD>((lstrlen(nppPathParam) + 1) * sizeof(wchar_t)));
		}
		RegCloseKey(hKey);
	}
}
