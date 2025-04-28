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
#include "sha512.h"
#include "calc_sha1.h"
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

			_hFont = createFont(L"Courier New", 9, false, _hSelf);

			const HWND hHashPathEdit = ::GetDlgItem(_hSelf, IDC_HASH_PATH_EDIT);
			const HWND hHashResult = ::GetDlgItem(_hSelf, IDC_HASH_RESULT_EDIT);

			::SendMessage(hHashPathEdit, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), TRUE);
			::SendMessage(hHashResult, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), TRUE);

			::SetWindowLongPtr(hHashPathEdit, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
			_oldHashPathEditProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hHashPathEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HashPathEditStaticProc)));

			::SetWindowLongPtr(hHashResult, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
			_oldHashResultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hHashResult, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HashResultStaticProc)));
		}
		return TRUE;

		case WM_CTLCOLORDLG:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORSTATIC:
		{
			const auto hdcStatic = reinterpret_cast<HDC>(wParam);
			const auto dlgCtrlID = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam));
			if (dlgCtrlID == IDC_HASH_PATH_EDIT || dlgCtrlID == IDC_HASH_RESULT_EDIT)
			{
				return NppDarkMode::onCtlColor(hdcStatic);
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

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::autoThemeChildControls(_hSelf);
			return TRUE;
		}

		case WM_DPICHANGED:
		{
			_dpiManager.setDpiWP(wParam);

			destroy();
			_hFont = createFont(L"Courier New", 9, false, _hSelf);

			::SendDlgItemMessageW(_hSelf, IDC_HASH_PATH_EDIT, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), TRUE);
			::SendDlgItemMessageW(_hSelf, IDC_HASH_RESULT_EDIT, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), TRUE);

			setPositionDpi(lParam);

			getWindowRect(_rc);

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
					display(false);
					return TRUE;
				
				case IDOK:
				{
					return TRUE;
				}

				case IDC_HASH_FILEBROWSER_BUTTON:
				{
					CustomFileDialog fDlg(_hSelf);
					fDlg.setExtFilter(L"All types", L".*");

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
									files2check += L"\r\n";

									wchar_t* fileName = ::PathFindFileName(it.c_str());
									hashResultStr += wmc.char2wchar(md5Result, CP_ACP);
									hashResultStr += L"  ";
									hashResultStr += fileName;
									hashResultStr += L"\r\n";
								}
							}
							else
							{
								std::string content = getFileContent(it.c_str());

								uint8_t hash[HASH_MAX_LENGTH]{};
								wchar_t hashStr[HASH_STR_MAX_LENGTH]{};

								switch (_ht)
								{
									case hash_sha1:
									{
										calc_sha1(hash, reinterpret_cast<const uint8_t*>(content.c_str()), content.length());
									}
									break;

									case hash_sha256:
									{
										calc_sha_256(hash, reinterpret_cast<const uint8_t*>(content.c_str()), content.length());
									}
									break;

									case hash_sha512:
									{
										calc_sha_512(hash, reinterpret_cast<const uint8_t*>(content.c_str()), content.length());
									}
									break;

									default:
										return FALSE;

								}

								for (int i = 0; i < _ht; i++)
									wsprintf(hashStr + i * 2, L"%02x", hash[i]);

								files2check += it;
								files2check += L"\r\n";

								wchar_t* fileName = ::PathFindFileName(it.c_str());
								hashResultStr += hashStr;
								hashResultStr += L"  ";
								hashResultStr += fileName;
								hashResultStr += L"\r\n";
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
		std::wstring title;
		std::wstring buttonText;

		switch (_ht)
		{
			case hash_md5:
			{
				title = L"Generate MD5 digest from files";
				buttonText = L"Choose files to &generate MD5...";
			}
			break;

			case hash_sha1:
			{
				title = L"Generate SHA-1 digest from files";
				buttonText = L"Choose files to &generate SHA-1...";
			}
			break;

			case hash_sha256:
			{
				title = L"Generate SHA-256 digest from files";
				buttonText = L"Choose files to &generate SHA-256...";
			}
			break;

			case hash_sha512:
			{
				title = L"Generate SHA-1 digest from files";
				buttonText = L"Choose files to &generate SHA-512...";
			}
			break;

			default:
				return;
		}
		::SetWindowText(_hSelf, title.c_str());
		::SetDlgItemText(_hSelf, IDC_HASH_FILEBROWSER_BUTTON, buttonText.c_str());
	}

	// Adjust the position in the center
	moveForDpiChange();
	goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);
}

void HashFromFilesDlg::destroy()
{
	if (_hFont != nullptr)
	{
		::DeleteObject(_hFont);
		_hFont = nullptr;
	}
}

void HashFromTextDlg::generateHash()
{
	if (_ht != hash_md5 && _ht != hash_sha1 && _ht != hash_sha256 && _ht != hash_sha512)
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
		else
		{
			uint8_t hash[HASH_MAX_LENGTH]{};
			wchar_t hashStr[HASH_STR_MAX_LENGTH]{};

			switch (_ht)
			{
				case hash_sha1:
				{
					calc_sha1(hash, reinterpret_cast<const uint8_t*>(newText), strlen(newText));
				}
				break;

				case hash_sha256:
				{
					calc_sha_256(hash, reinterpret_cast<const uint8_t*>(newText), strlen(newText));
				}
				break;

				case hash_sha512:
				{
					calc_sha_512(hash, reinterpret_cast<const uint8_t*>(newText), strlen(newText));
				}
				break;

				default:
					return;
			}

			for (int i = 0; i < _ht; i++)
				wsprintf(hashStr + i * 2, L"%02x", hash[i]);

			::SetDlgItemText(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT, hashStr);
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

		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();

		while (std::getline(ss, aLine))
		{
			if (aLine.empty()) // in case of UNIX EOL
			{
				result += "\r\n";
			}
			else
			{
				// getline() detect only '\n' but not "\r\n" under Windows
				// this hack is to walk around such bug
				if (aLine.back() == '\r')
					aLine = aLine.substr(0, aLine.size() - 1);

				if (aLine.empty()) // Windows EOL, both \n & \r are removed
				{
					result += "\r\n";
				}
				else
				{
					const char* newText = wmc.wchar2char(aLine.c_str(), SC_CP_UTF8);

					if (_ht == hash_md5)
					{
						MD5 md5;
						char* md5Result = md5.digestString(newText);
						result += md5Result;
						result += "\r\n";
					}
					else
					{
						uint8_t hash[HASH_MAX_LENGTH]{};
						char hashStr[HASH_STR_MAX_LENGTH]{};

						switch (_ht)
						{
							case hash_sha1:
							{
								calc_sha1(hash, reinterpret_cast<const uint8_t*>(newText), strlen(newText));
							}
							break;

							case hash_sha256:
							{
								calc_sha_256(hash, reinterpret_cast<const uint8_t*>(newText), strlen(newText));
							}
							break;

							case hash_sha512:
							{
								calc_sha_512(hash, reinterpret_cast<const uint8_t*>(newText), strlen(newText));
							}
							break;

							default:
								return;
						}

						for (int i = 0; i < _ht; i++)
							sprintf(hashStr + i * 2, "%02x", hash[i]);

						result += hashStr;
						result += "\r\n";
					}
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

			_hFont = createFont(L"Courier New", 9, false, _hSelf);

			const HWND hHashTextEdit = ::GetDlgItem(_hSelf, IDC_HASH_TEXT_EDIT);
			const HWND hHashResult = ::GetDlgItem(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT);

			::SendMessage(hHashTextEdit, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), TRUE);
			::SendMessage(hHashResult, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), TRUE);

			::SetWindowLongPtr(hHashTextEdit, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
			_oldHashTextEditProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hHashTextEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HashTextEditStaticProc)));

			::SetWindowLongPtr(hHashResult, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
			_oldHashResultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hHashResult, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HashResultStaticProc)));
		}
		return TRUE;

		case WM_CTLCOLOREDIT:
		{
			return NppDarkMode::onCtlColorCtrl(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORDLG:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORSTATIC:
		{
			const auto hdcStatic = reinterpret_cast<HDC>(wParam);
			const auto dlgCtrlID = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam));
			if (dlgCtrlID == IDC_HASH_RESULT_FOMTEXT_EDIT)
			{
				return NppDarkMode::onCtlColor(hdcStatic);
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

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::autoThemeChildControls(_hSelf);
			return TRUE;
		}

		case WM_DPICHANGED:
		{
			_dpiManager.setDpiWP(wParam);

			destroy();
			_hFont = createFont(L"Courier New", 9, false, _hSelf);

			::SendDlgItemMessageW(_hSelf, IDC_HASH_TEXT_EDIT, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), TRUE);
			::SendDlgItemMessageW(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), TRUE);

			setPositionDpi(lParam);

			getWindowRect(_rc);

			return TRUE;
		}

		case WM_COMMAND:
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

			switch (LOWORD(wParam))
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

void HashFromTextDlg::setHashType(hashType hashType2set)
{
	_ht = hashType2set;
}

void HashFromTextDlg::doDialog(bool isRTL)
{
	if (!isCreated())
	{
		create(IDD_HASHFROMTEXT_DLG, isRTL);
		std::wstring title;
		switch (_ht)
		{
			case hash_md5:
			{
				title = L"Generate MD5 digest";
			}
			break;
			
			case hash_sha1:
			{
				title = L"Generate SHA-1 digest";
			}
			break;

			case hash_sha256:
			{
				title = L"Generate SHA-256 digest";
			}
			break;

			case hash_sha512:
			{
				title = L"Generate SHA-512 digest";
			}
			break;

			default:
				break;
		}

		::SetWindowText(_hSelf, title.c_str());
	}

	// Adjust the position in the center
	moveForDpiChange();
	goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);
}

void HashFromTextDlg::destroy()
{
	if (_hFont != nullptr)
	{
		::DeleteObject(_hFont);
		_hFont = nullptr;
	}
}
