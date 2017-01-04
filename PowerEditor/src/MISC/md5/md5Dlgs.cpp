//this file is part of notepad++
//Copyright (C)2016 Don HO <don.h@fee.fr>
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

#include "md5.h"
#include "md5Dlgs.h"
#include "md5Dlgs_rc.h"
#include "FileDialog.h"
#include "Parameters.h"
#include <shlwapi.h>

INT_PTR CALLBACK MD5FromFilesDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	switch (message) 
	{
		case WM_INITDIALOG:
		{
			int fontDpiDynamicalHeight = NppParameters::getInstance()->_dpiManager.scaleY(13);
			HFONT hFont = ::CreateFontA(fontDpiDynamicalHeight, 0, 0, 0, 0, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				DEFAULT_PITCH | FF_DONTCARE, "Courier New");
			::SendMessage(::GetDlgItem(_hSelf, IDC_MD5_PATH_EDIT), WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
			::SendMessage(::GetDlgItem(_hSelf, IDC_MD5_RESULT_EDIT), WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
		}
		return TRUE;

		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDCANCEL :
					display(false);
					return TRUE;
				
				case IDOK :
				{
					return TRUE;
				}

				case IDC_MD5_FILEBROWSER_BUTTON:
				{
					FileDialog fDlg(_hSelf, ::GetModuleHandle(NULL));
					fDlg.setExtFilter(TEXT("All types"), TEXT(".*"), NULL);

					if (stringVector *pfns = fDlg.doOpenMultiFilesDlg())
					{
						std::wstring files2check, md5resultStr;
						for (auto it = pfns->begin(); it != pfns->end(); ++it)
						{
							WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
							const char *path = wmc->wchar2char(it->c_str(), CP_ACP);
							
							MD5 md5;
							char *md5Result = md5.digestFile(path);

							if (md5Result)
							{
								files2check += *it;
								files2check += TEXT("\r\n");

								wchar_t* fileName = ::PathFindFileName(it->c_str());
								md5resultStr += wmc->char2wchar(md5Result, CP_ACP);
								md5resultStr += TEXT("  ");
								md5resultStr += fileName;
								md5resultStr += TEXT("\r\n");
							}
						}

						if (not files2check.empty() && not md5resultStr.empty())
						{
							::SetDlgItemText(_hSelf, IDC_MD5_PATH_EDIT, files2check.c_str());
							::SetDlgItemText(_hSelf, IDC_MD5_RESULT_EDIT, md5resultStr.c_str());
						}
					}
				}
				return TRUE;

				case IDC_MD5_TOCLIPBOARD_BUTTON:
				{
					int len = static_cast<int>(::SendMessage(::GetDlgItem(_hSelf, IDC_MD5_RESULT_EDIT), WM_GETTEXTLENGTH, 0, 0));
					if (len)
					{
						wchar_t *rStr = new wchar_t[len+1];
						::GetDlgItemText(_hSelf, IDC_MD5_RESULT_EDIT, rStr, len + 1);
						str2Clipboard(rStr, _hSelf);
						delete[] rStr;
					}
				}
				return TRUE;

				default :
					break;
			}
		}
	}
	return FALSE;	
}

void MD5FromFilesDlg::doDialog(bool isRTL)
{
	if (!isCreated())
		create(IDD_MD5FROMFILES_DLG, isRTL);

    // Adjust the position in the center
	goToCenter();
	//::SetFocus(::GetDlgItem(_hSelf, IDC_COMBO_RUN_PATH));
};

void MD5FromTextDlg::generateMD5()
{
	int len = static_cast<int>(::SendMessage(::GetDlgItem(_hSelf, IDC_MD5_TEXT_EDIT), WM_GETTEXTLENGTH, 0, 0));
	if (len)
	{
		wchar_t *text = new wchar_t[len + 1];
		::GetDlgItemText(_hSelf, IDC_MD5_TEXT_EDIT, text, len + 1);
		WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
		const char *newText = wmc->wchar2char(text, SC_CP_UTF8);

		MD5 md5;
		char* md5Result = md5.digestString(newText);
		::SetDlgItemTextA(_hSelf, IDC_MD5_RESULT_FOMTEXT_EDIT, md5Result);

		delete[] text;
	}
	else
	{
		::SetDlgItemTextA(_hSelf, IDC_MD5_RESULT_FOMTEXT_EDIT, "");
	}
}

void MD5FromTextDlg::generateMD5PerLine()
{
	int len = static_cast<int>(::SendMessage(::GetDlgItem(_hSelf, IDC_MD5_TEXT_EDIT), WM_GETTEXTLENGTH, 0, 0));
	if (len)
	{
		wchar_t *text = new wchar_t[len + 1];
		::GetDlgItemText(_hSelf, IDC_MD5_TEXT_EDIT, text, len + 1);

		std::wstringstream ss(text);
		std::wstring aLine;
		std::string result;
		MD5 md5;
		WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
		while (std::getline(ss, aLine))
		{
			// getline() detect only '\n' but not "\r\n" under windows
			// this hack is to walk around such bug
			if (aLine.back() == '\r')
				aLine = aLine.substr(0, aLine.size() - 1);

			if (aLine.empty())
				result += "\r\n";
			else
			{
				const char *newText = wmc->wchar2char(aLine.c_str(), SC_CP_UTF8);
				char* md5Result = md5.digestString(newText);
				result += md5Result;
				result += "\r\n";
			}
		}
		delete[] text;
		::SetDlgItemTextA(_hSelf, IDC_MD5_RESULT_FOMTEXT_EDIT, result.c_str());
	}
	else
	{
		::SetDlgItemTextA(_hSelf, IDC_MD5_RESULT_FOMTEXT_EDIT, "");
	}
}

INT_PTR CALLBACK MD5FromTextDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	switch (message) 
	{
		case WM_INITDIALOG:
		{
			int fontDpiDynamicalHeight = NppParameters::getInstance()->_dpiManager.scaleY(13);
			HFONT hFont = ::CreateFontA(fontDpiDynamicalHeight, 0, 0, 0, 0, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				DEFAULT_PITCH | FF_DONTCARE, "Courier New");
			::SendMessage(::GetDlgItem(_hSelf, IDC_MD5_TEXT_EDIT), WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
			::SendMessage(::GetDlgItem(_hSelf, IDC_MD5_RESULT_FOMTEXT_EDIT), WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
		}
		return TRUE;

		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_MD5_TEXT_EDIT)
			{
				if (isCheckedOrNot(IDC_MD5_EACHLINE_CHECK))
				{
					generateMD5PerLine();
				}
				else
				{
					generateMD5();
				}
			}

			switch (wParam)
			{
				case IDCANCEL :
					display(false);
					return TRUE;
				
				case IDOK :
				{
					return TRUE;
				}

				case IDC_MD5_EACHLINE_CHECK:
				{
					if (isCheckedOrNot(IDC_MD5_EACHLINE_CHECK))
					{
						generateMD5PerLine();
					}
					else
					{
						generateMD5();
					}
					
				}
				return TRUE;

				case IDC_MD5_FROMTEXT_TOCLIPBOARD_BUTTON:
				{
					int len = static_cast<int>(::SendMessage(::GetDlgItem(_hSelf, IDC_MD5_RESULT_FOMTEXT_EDIT), WM_GETTEXTLENGTH, 0, 0));
					if (len)
					{
						wchar_t *rStr = new wchar_t[len+1];
						::GetDlgItemText(_hSelf, IDC_MD5_RESULT_FOMTEXT_EDIT, rStr, len + 1);
						str2Clipboard(rStr, _hSelf);
						delete[] rStr;
					}
				}
				return TRUE;

				default :
					break;
			}
		}
	}
	return FALSE;	
}

void MD5FromTextDlg::doDialog(bool isRTL)
{
	if (!isCreated())
		create(IDD_MD5FROMTEXT_DLG, isRTL);

    // Adjust the position in the center
	goToCenter();
	//::SetFocus(::GetDlgItem(_hSelf, IDC_COMBO_RUN_PATH));
};
