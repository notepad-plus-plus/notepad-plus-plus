/*
this file is part of notepad++
Copyright (C)2003 Don HO ( donho@altern.org )

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include <windows.h>
#include "regExtDlg.h"
#include "resource.h"

const char *nppName = "Notepad++_file";
const char *nppBackup = "Notepad++_backup";
const char *nppDoc = "Notepad++ Document";

const int nbSupportedLang = 9;
const int nbExtMax = 10;
const int extNameMax = 18;

char defExtArray[nbSupportedLang][nbExtMax][extNameMax] = {
	{"Notepad",        			".txt", ".log", ".ini"},
	{"c, c++, objc",   	".h", ".hpp", ".hxx", ".c", ".cpp", ".cxx", ".cc", ".m"},
	{"java, c#, pascal", 		".java", ".cs", ".pas", ".inc"},
	{"web(html) script",   ".html", ".htm", ".php", ".phtml", ".js", ".jsp", ".asp", ".css", ".xml"},
	{"public script",		".sh", ".bsh", ".nsi", ".nsh", ".lua", ".pl", ".pm", ".py"},
	{"property script",	".rc", ".as", ".mx", ".vb", ".vbs"},
	{"fortran, TeX, SQL",			".f",  ".for", ".f90",  ".f95", ".f2k", ".tex", ".sql"},
	{"misc",								".nfo", ".mak"},
	{"customize"}
};

void RegExtDlg::doDialog(bool isRTL) 
{
	if (isRTL)
	{
		DLGTEMPLATE *pMyDlgTemplate = NULL;
		HGLOBAL hMyDlgTemplate = makeRTLResource(IDD_REGEXT_BOX, &pMyDlgTemplate);
		::DialogBoxIndirectParam(_hInst, pMyDlgTemplate, _hParent,  (DLGPROC)dlgProc, (LPARAM)this);
		::GlobalFree(hMyDlgTemplate);
	}
	else
		::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_REGEXT_BOX), _hParent,  (DLGPROC)dlgProc, (LPARAM)this);
};

BOOL CALLBACK RegExtDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG :
		{
			getRegisteredExts();
			getDefSupportedExts();
			writeNppPath();
			//goToCenter();
			::EnableWindow(::GetDlgItem(_hSelf, IDC_ADDFROMLANGEXT_BUTTON), false);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_REMOVEEXT_BUTTON), false);
			::SendDlgItemMessage(_hSelf, IDC_CUSTOMEXT_EDIT, EM_SETLIMITTEXT, extNameMax-1, 0);

			return TRUE;
		}

		case WM_DRAWITEM :
		{
			HICON hIcon = ::LoadIcon(_hInst, MAKEINTRESOURCE(IDI_DELETE_ICON));
			DRAWITEMSTRUCT *pdis = (DRAWITEMSTRUCT *)lParam;
			::DrawIcon(pdis->hDC, 0, 0, hIcon);
			return TRUE;
		}

		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDC_ADDFROMLANGEXT_BUTTON :
				{
					//int index2Add;
					char ext2Add[extNameMax] = "";
					if (!_isCustomize)
					{
						int index2Add = ::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANGEXT_LIST, LB_GETCURSEL, 0, 0);
						::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANGEXT_LIST, LB_GETTEXT, index2Add, (LPARAM)ext2Add);
						addExt(ext2Add);
						::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANGEXT_LIST, LB_DELETESTRING, index2Add, 0);
					}
					else
					{
						::SendDlgItemMessage(_hSelf, IDC_CUSTOMEXT_EDIT, WM_GETTEXT, extNameMax, (LPARAM)ext2Add);
						int i = ::SendDlgItemMessage(_hSelf, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_FINDSTRINGEXACT, 0, (LPARAM)ext2Add);
						if (i != LB_ERR) 
							return TRUE;
						addExt(ext2Add);
						::SendDlgItemMessage(_hSelf, IDC_CUSTOMEXT_EDIT, WM_SETTEXT, 0, (LPARAM)"");
					}
					::SendDlgItemMessage(_hSelf, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_ADDSTRING, 0, (LPARAM)ext2Add);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_ADDFROMLANGEXT_BUTTON), false);
					return TRUE;
				}

				case IDC_REMOVEEXT_BUTTON :
				{
					char ext2Sup[extNameMax] = "";
					int index2Sup = ::SendDlgItemMessage(_hSelf, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_GETCURSEL, 0, 0);
					::SendDlgItemMessage(_hSelf, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_GETTEXT, index2Sup, (LPARAM)ext2Sup);
					if (deleteExts(ext2Sup))
						::SendDlgItemMessage(_hSelf, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_DELETESTRING, index2Sup, 0);
					int langIndex = ::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANG_LIST, LB_GETCURSEL, 0, 0);

					::EnableWindow(::GetDlgItem(_hSelf, IDC_REMOVEEXT_BUTTON), false);

					if (langIndex != LB_ERR)
					{
						for (int i = 1 ; i < nbExtMax ; i++)
						{
							if (!stricmp(ext2Sup, defExtArray[langIndex][i]))
							{
								::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANGEXT_LIST, LB_ADDSTRING, 0, (LPARAM)ext2Sup);
								return TRUE;
							}
						}
					}
					return TRUE;
				}

				case IDCANCEL :
					::EndDialog(_hSelf, 0);
					return TRUE;

			}

			if (HIWORD(wParam) == EN_CHANGE)
            {
				char text[extNameMax] = "";
				::SendDlgItemMessage(_hSelf, IDC_CUSTOMEXT_EDIT, WM_GETTEXT, extNameMax, (LPARAM)text);
				if ((strlen(text) == 1) && (text[0] != '.'))
				{
					text[1] = text[0];
					text[0] = '.';
					text[2] = '\0';
					::SendDlgItemMessage(_hSelf, IDC_CUSTOMEXT_EDIT, WM_SETTEXT, 0, (LPARAM)text);
					::SendDlgItemMessage(_hSelf, IDC_CUSTOMEXT_EDIT, EM_SETSEL, 2, 2);
				}
				::EnableWindow(::GetDlgItem(_hSelf, IDC_ADDFROMLANGEXT_BUTTON), (strlen(text) > 1));
				return TRUE;
			}

			if (HIWORD(wParam) == LBN_SELCHANGE)
            {
				int i = ::SendDlgItemMessage(_hSelf, LOWORD(wParam), LB_GETCURSEL, 0, 0);
				if (LOWORD(wParam) == IDC_REGEXT_LANG_LIST)
				{
					if (i != LB_ERR)
					{
						char itemName[32];
						::SendDlgItemMessage(_hSelf, LOWORD(wParam), LB_GETTEXT, i, (LPARAM)itemName);

						if (!stricmp(defExtArray[nbSupportedLang-1][0], itemName))
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
							int count = ::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANGEXT_LIST, LB_GETCOUNT, 0, 0);
							for (count -= 1 ; count >= 0 ; count--)
								::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANGEXT_LIST, LB_DELETESTRING, count, 0);

							for (int j = 1 ; j < nbExtMax ; j++)
								if (strcmp("", defExtArray[i][j]))
								{
									int index = ::SendDlgItemMessage(_hSelf, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_FINDSTRINGEXACT, 0, (LPARAM)defExtArray[i][j]);
									if (index == -1)
										::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANGEXT_LIST, LB_ADDSTRING, 0, (LPARAM)defExtArray[i][j]);
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
		}
		default :
			return FALSE;
	}
	//return FALSE;
}

void RegExtDlg::getRegisteredExts()
{
	int nbRegisteredKey = getNbSubKey(HKEY_CLASSES_ROOT);
	for (int i = 0 ; i < nbRegisteredKey ; i++)
	{
		char extName[extNameLen];
		//FILETIME fileTime;
		int extNameActualLen = extNameLen;
		int res = ::RegEnumKeyEx(HKEY_CLASSES_ROOT, i, extName, (LPDWORD)&extNameActualLen, NULL, NULL, NULL, NULL);
		if ((res == ERROR_SUCCESS) && (extName[0] == '.'))
		{
			//char valName[extNameLen];
			char valData[extNameLen];
			int valDataLen = extNameLen;
			int valType;
			HKEY hKey2Check;
			extNameActualLen = extNameLen;
			::RegOpenKeyEx(HKEY_CLASSES_ROOT, extName, 0, KEY_ALL_ACCESS, &hKey2Check);
			::RegQueryValueEx(hKey2Check, "", NULL, (LPDWORD)&valType, (LPBYTE)valData, (LPDWORD)&valDataLen);
			//::RegEnumValue(hKey2Check, 0, valName, (LPDWORD)&extNameActualLen, NULL, (LPDWORD)&valType, (LPBYTE)valData, (LPDWORD)&valDataLen);
			if ((valType == REG_SZ) && (!strcmp(valData, nppName)))
				::SendDlgItemMessage(_hSelf, IDC_REGEXT_REGISTEREDEXTS_LIST, LB_ADDSTRING, 0, (LPARAM)extName);
			::RegCloseKey(hKey2Check);
		}
	}
}

void RegExtDlg::getDefSupportedExts()
{
	for (int i = 0 ; i < nbSupportedLang ; i++)
		::SendDlgItemMessage(_hSelf, IDC_REGEXT_LANG_LIST, LB_ADDSTRING, 0, (LPARAM)defExtArray[i][0]);
}


void RegExtDlg::addExt(char *ext)
{
    HKEY  hKey;
    DWORD dwDisp;
    long  nRet;
    
	nRet = ::RegCreateKeyEx(HKEY_CLASSES_ROOT,
                ext,
                0,
                NULL,
                0,
                KEY_ALL_ACCESS,
                NULL,
                &hKey,
                &dwDisp);
    
    if (nRet == ERROR_SUCCESS)
    {
		int valDataLen = 256;
		char valData[256];
		

		if (dwDisp == REG_OPENED_EXISTING_KEY)
		{
			int res = ::RegQueryValueEx(hKey, "", NULL, NULL, (LPBYTE)valData, (LPDWORD)&valDataLen);
			if (res == ERROR_SUCCESS)
				::RegSetValueEx(hKey, nppBackup, 0, REG_SZ, (LPBYTE)valData, valDataLen+1);
		}
		::RegSetValueEx(hKey, NULL, 0, REG_SZ, (LPBYTE)nppName, strlen(nppName)+1);

		::RegCloseKey(hKey);
    }
}

bool RegExtDlg::deleteExts(const char *ext2Delete)
{
	HKEY hKey;
	::RegOpenKeyEx(HKEY_CLASSES_ROOT, ext2Delete, 0, KEY_ALL_ACCESS, &hKey);

	int nbValue = getNbSubValue(hKey);
	int nbSubkey = getNbSubKey(hKey);

	if ((nbValue <= 1) && (!nbSubkey))
	{
		char subKey[32] = "\\";
		strcat(subKey, ext2Delete);
		::RegDeleteKey(HKEY_CLASSES_ROOT, subKey);
	}
	else
	{
		char valData[extNameLen];
		int valDataLen = extNameLen;
		int valType;
		int res = ::RegQueryValueEx(hKey, nppBackup, NULL, (LPDWORD)&valType, (LPBYTE)valData, (LPDWORD)&valDataLen);

		if (res == ERROR_SUCCESS)
		{
			::RegSetValueEx(hKey, NULL, 0, valType, (LPBYTE)valData, valDataLen);
			::RegDeleteValue(hKey, nppBackup);
		}
		else
			::RegDeleteValue(hKey, NULL);
	}

	return true;
}

void RegExtDlg::writeNppPath()
{
	HKEY  hKey, hRootKey;
	DWORD dwDisp;
	long  nRet;
	char regStr[MAX_PATH] = "";
	strcat(strcat(regStr, nppName), "\\shell\\open\\command");

	nRet = ::RegCreateKeyEx(
				HKEY_CLASSES_ROOT,
				regStr,
				0,
				NULL,
				0,
				KEY_ALL_ACCESS,
				NULL,
				&hKey,
				&dwDisp);


	if (nRet == ERROR_SUCCESS)
	{
		//if (dwDisp == REG_CREATED_NEW_KEY)
		{
			// Write the value for new document
			::RegOpenKeyEx(HKEY_CLASSES_ROOT, nppName, 0, KEY_ALL_ACCESS, &hRootKey);
			::RegSetValueEx(hRootKey, NULL, 0, REG_SZ, (LPBYTE)nppDoc, strlen(nppDoc)+1);

			char nppPath[MAX_PATH];
			::GetModuleFileName(_hInst, nppPath, MAX_PATH);

			char nppPathParam[256] = "\"";
			strcat(strcat(nppPathParam, nppPath), "\" \"%1\"");

			::RegSetValueEx(hKey, NULL, 0, REG_SZ, (LPBYTE)nppPathParam, strlen(nppPathParam)+1);
		}
		RegCloseKey(hKey);
	}
} 
