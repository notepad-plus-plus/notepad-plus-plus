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

#include "md5.h"
#include <stdint.h>
#include "sha-256.h"
#include "md5Dlgs.h"
#include "md5Dlgs_rc.h"
#include "CustomFileDialog.h"
#include "Parameters.h"
#include <shlwapi.h>
#include "resource.h"

intptr_t CALLBACK HashFromFilesDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG:
		{
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			int fontDpiDynamicalHeight = NppParameters::getInstance()._dpiManager.scaleY(13);
			HFONT hFont = ::CreateFontA(fontDpiDynamicalHeight, 0, 0, 0, 0, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				DEFAULT_PITCH | FF_DONTCARE, "Courier New");

			const HWND hHashPathEdit = ::GetDlgItem(_hSelf, IDC_HASH_PATH_EDIT);
			const HWND hHashResult = ::GetDlgItem(_hSelf, IDC_HASH_RESULT_EDIT);

			::SendMessage(hHashPathEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
			::SendMessage(hHashResult, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

			::SetWindowLongPtr(hHashPathEdit, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
			_oldHashPathEditProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hHashPathEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HashPathEditStaticProc)));

			::SetWindowLongPtr(hHashResult, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
			_oldHashResultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hHashResult, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HashResultStaticProc)));
		}
		return TRUE;

		case WM_CTLCOLORDLG:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				HWND hwnd = reinterpret_cast<HWND>(lParam);
				if (hwnd == ::GetDlgItem(_hSelf, IDC_HASH_PATH_EDIT) || hwnd == ::GetDlgItem(_hSelf, IDC_HASH_RESULT_EDIT))
				{
					return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
				}
				else
				{
					return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
				}
			}
			break;
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

				case IDC_HASH_FILEBROWSER_BUTTON:
				{
					CustomFileDialog fDlg(_hSelf);
					fDlg.setExtFilter(TEXT("All types"), TEXT(".*"));

					const auto& fns = fDlg.doOpenMultiFilesDlg();
					if (!fns.empty())
					{
						std::wstring files2check, hashResultStr;
						for (const auto& it : fns)
						{
							if (_ht == hashType::hash_md5)
							{
								WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
								const char *path = wmc.wchar2char(it.c_str(), CP_ACP);

								MD5 md5;
								char *md5Result = md5.digestFile(path);

								if (md5Result)
								{
									files2check += it;
									files2check += TEXT("\r\n");

									wchar_t* fileName = ::PathFindFileName(it.c_str());
									hashResultStr += wmc.char2wchar(md5Result, CP_ACP);
									hashResultStr += TEXT("  ");
									hashResultStr += fileName;
									hashResultStr += TEXT("\r\n");
								}
							}
							else if (_ht == hashType::hash_sha256)
							{
								std::string content = getFileContent(it.c_str());

								uint8_t sha2hash[32];
								calc_sha_256(sha2hash, reinterpret_cast<const uint8_t*>(content.c_str()), content.length());

								wchar_t sha2hashStr[65] = { '\0' };
								for (size_t i = 0; i < 32; i++)
									wsprintf(sha2hashStr + i * 2, TEXT("%02x"), sha2hash[i]);

								files2check += it;
								files2check += TEXT("\r\n");

								wchar_t* fileName = ::PathFindFileName(it.c_str());
								hashResultStr += sha2hashStr;
								hashResultStr += TEXT("  ");
								hashResultStr += fileName;
								hashResultStr += TEXT("\r\n");
							}
							else
							{
								// unknown
							}
						}

						if (!files2check.empty() && !hashResultStr.empty())
						{
							::SetDlgItemText(_hSelf, IDC_HASH_PATH_EDIT, files2check.c_str());
							::SetDlgItemText(_hSelf, IDC_HASH_RESULT_EDIT, hashResultStr.c_str());
						}
					}
				}
				return TRUE;

				case IDC_HASH_TOCLIPBOARD_BUTTON:
				{
					int len = static_cast<int>(::SendMessage(::GetDlgItem(_hSelf, IDC_HASH_RESULT_EDIT), WM_GETTEXTLENGTH, 0, 0));
					if (len)
					{
						wchar_t* rStr = new wchar_t[len+1];
						::GetDlgItemText(_hSelf, IDC_HASH_RESULT_EDIT, rStr, len + 1);
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

LRESULT run_textEditProc(WNDPROC oldEditProc, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_GETDLGCODE:
		{
			return DLGC_WANTALLKEYS | ::CallWindowProc(oldEditProc, hwnd, message, wParam, lParam);
		}

		case WM_CHAR:
		{
			if (wParam == 1) // Ctrl+A
			{
				::SendMessage(hwnd, EM_SETSEL, 0, -1);
				return TRUE;
			}
			break;
		}

		default:
			break;
	}
	return ::CallWindowProc(oldEditProc, hwnd, message, wParam, lParam);
}

void HashFromFilesDlg::setHashType(hashType hashType2set)
{
	_ht = hashType2set;
}

void HashFromFilesDlg::doDialog(bool isRTL)
{
	if (!isCreated())
	{
		create(IDD_HASHFROMFILES_DLG, isRTL);

		if (_ht == hash_sha256)
		{
			generic_string title = TEXT("Generate SHA-256 digest from files");
			::SetWindowText(_hSelf, title.c_str());

			generic_string buttonText = TEXT("Choose files to generate SHA-256...");
			::SetDlgItemText(_hSelf, IDC_HASH_FILEBROWSER_BUTTON, buttonText.c_str());
		}
	}

	// Adjust the position in the center
	goToCenter();
};

void HashFromTextDlg::generateHash()
{
	if (_ht != hash_md5 && _ht != hash_sha256)
		return;

	int len = static_cast<int>(::SendMessage(::GetDlgItem(_hSelf, IDC_HASH_TEXT_EDIT), WM_GETTEXTLENGTH, 0, 0));
	if (len)
	{
		// it's important to get text from UNICODE then convert it to UTF8
		// So we get the result of UTF8 text (tested with Chinese).
		wchar_t* text = new wchar_t[len + 1];
		::GetDlgItemText(_hSelf, IDC_HASH_TEXT_EDIT, text, len + 1);
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		const char *newText = wmc.wchar2char(text, SC_CP_UTF8);
		if (_ht == hash_md5)
		{
			MD5 md5;
			char* md5Result = md5.digestString(newText);
			::SetDlgItemTextA(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT, md5Result);
		}
		else if (_ht == hash_sha256)
		{
			uint8_t sha2hash[32];
			calc_sha_256(sha2hash, reinterpret_cast<const uint8_t*>(newText), strlen(newText));

			wchar_t sha2hashStr[65] = { '\0' };
			for (size_t i = 0; i < 32; i++)
				wsprintf(sha2hashStr + i * 2, TEXT("%02x"), sha2hash[i]);

			::SetDlgItemText(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT, sha2hashStr);
		}
		delete[] text;
	}
	else
	{
		::SetDlgItemTextA(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT, "");
	}
}

void HashFromTextDlg::generateHashPerLine()
{
	int len = static_cast<int>(::SendMessage(::GetDlgItem(_hSelf, IDC_HASH_TEXT_EDIT), WM_GETTEXTLENGTH, 0, 0));
	if (len)
	{
		wchar_t* text = new wchar_t[len + 1];
		::GetDlgItemText(_hSelf, IDC_HASH_TEXT_EDIT, text, len + 1);

		std::wstringstream ss(text);
		std::wstring aLine;
		std::string result;
		MD5 md5;
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
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
				const char *newText = wmc.wchar2char(aLine.c_str(), SC_CP_UTF8);

				if (_ht == hash_md5)
				{
					char* md5Result = md5.digestString(newText);
					result += md5Result;
					result += "\r\n";
				}
				else if (_ht == hash_sha256)
				{
					uint8_t sha2hash[32];
					calc_sha_256(sha2hash, reinterpret_cast<const uint8_t*>(newText), strlen(newText));

					char sha2hashStr[65] = { '\0' };
					for (size_t i = 0; i < 32; i++)
						sprintf(sha2hashStr + i * 2, "%02x", sha2hash[i]);

					result += sha2hashStr;
					result += "\r\n";
				}
			}
		}
		delete[] text;
		::SetDlgItemTextA(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT, result.c_str());
	}
	else
	{
		::SetDlgItemTextA(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT, "");
	}
}

intptr_t CALLBACK HashFromTextDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG:
		{
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			int fontDpiDynamicalHeight = NppParameters::getInstance()._dpiManager.scaleY(13);
			HFONT hFont = ::CreateFontA(fontDpiDynamicalHeight, 0, 0, 0, 0, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				DEFAULT_PITCH | FF_DONTCARE, "Courier New");

			const HWND hHashTextEdit = ::GetDlgItem(_hSelf, IDC_HASH_TEXT_EDIT);
			const HWND hHashResult = ::GetDlgItem(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT);

			::SendMessage(hHashTextEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
			::SendMessage(hHashResult, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

			::SetWindowLongPtr(hHashTextEdit, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
			_oldHashTextEditProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hHashTextEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HashTextEditStaticProc)));

			::SetWindowLongPtr(hHashResult, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
			_oldHashResultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hHashResult, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HashResultStaticProc)));
		}
		return TRUE;

		case WM_CTLCOLOREDIT:
		{
			if (NppDarkMode::isEnabled())
			{
				HWND hwnd = reinterpret_cast<HWND>(lParam);
				if (hwnd == ::GetDlgItem(_hSelf, IDC_HASH_TEXT_EDIT))
				{
					return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
				}
				else
				{
					return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
				}
			}
			break;
		}

		case WM_CTLCOLORDLG:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				HWND hwnd = reinterpret_cast<HWND>(lParam);
				if (hwnd == ::GetDlgItem(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT))
				{
					return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
				}
				else
				{
					return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
				}
			}
			break;
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

		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_HASH_TEXT_EDIT)
			{
				if (isCheckedOrNot(IDC_HASH_EACHLINE_CHECK))
				{
					generateHashPerLine();
				}
				else
				{
					generateHash();
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

				case IDC_HASH_EACHLINE_CHECK:
				{
					if (isCheckedOrNot(IDC_HASH_EACHLINE_CHECK))
					{
						generateHashPerLine();
					}
					else
					{
						generateHash();
					}
				}
				return TRUE;

				case IDC_HASH_FROMTEXT_TOCLIPBOARD_BUTTON:
				{
					int len = static_cast<int>(::SendMessage(::GetDlgItem(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT), WM_GETTEXTLENGTH, 0, 0));
					if (len)
					{
						wchar_t* rStr = new wchar_t[len+1];
						::GetDlgItemText(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT, rStr, len + 1);
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

void HashFromTextDlg::setHashType(hashType hashType2set)
{
	_ht = hashType2set;
}

void HashFromTextDlg::doDialog(bool isRTL)
{
	if (!isCreated())
	{
		create(IDD_HASHFROMTEXT_DLG, isRTL);

		if (_ht == hash_sha256)
		{
			generic_string title = TEXT("Generate SHA-256 digest");
			::SetWindowText(_hSelf, title.c_str());
		}
	}

	// Adjust the position in the center
	goToCenter();
};
