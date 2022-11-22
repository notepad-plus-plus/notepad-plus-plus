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


#include "lesDlgs.h"
#include "resource.h"
#include "menuCmdID.h"
#include "NppDarkMode.h"

void ValueDlg::init(HINSTANCE hInst, HWND parent, int valueToSet, const TCHAR *text) 
{
	Window::init(hInst, parent);
	_defaultValue = valueToSet;
	_name = text;
}

int ValueDlg::doDialog(POINT p, bool isRTL) 
{
	_p = p;
	if (isRTL)
	{
		DLGTEMPLATE *pMyDlgTemplate = NULL;
		HGLOBAL hMyDlgTemplate = makeRTLResource(IDD_VALUE_DLG, &pMyDlgTemplate);
		int result = static_cast<int32_t>(::DialogBoxIndirectParam(_hInst, pMyDlgTemplate, _hParent, dlgProc, reinterpret_cast<LPARAM>(this)));
		::GlobalFree(hMyDlgTemplate);
		return result;
	}
	return static_cast<int32_t>(::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_VALUE_DLG), _hParent, dlgProc, reinterpret_cast<LPARAM>(this)));
}


int ValueDlg::reSizeValueBox()
{
	if (_nbNumber == DEFAULT_NB_NUMBER) return 0;
	RECT rect;
	POINT p;

	HWND hEdit = ::GetDlgItem(_hSelf, IDC_VALUE_EDIT);

	//get screen coordonnees (x,y)
	::GetWindowRect(hEdit, &rect);
	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top;

	p.x = rect.left;
	p.y = rect.top;

	// convert screen coordonnees to client coordonnees
	::ScreenToClient(_hSelf, &p);

	RECT rcText;
	::SendMessage(hEdit, EM_GETRECT, 0, reinterpret_cast<LPARAM>(&rcText));
	DWORD m = (DWORD)::SendMessage(hEdit, EM_GETMARGINS, 0, 0);
	int margins = LOWORD(m) + HIWORD(m);
	int textWidth = rcText.right - rcText.left;
	int frameWidth = w - textWidth;
	int newTextWidth = ((textWidth - margins) * _nbNumber / DEFAULT_NB_NUMBER) + margins + 1;
	int newWidth = newTextWidth + frameWidth;
	::MoveWindow(hEdit, p.x, p.y, newWidth, h, FALSE);
	return newWidth - w;
}

intptr_t CALLBACK ValueDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM) 
{
	switch (Message)
	{
		case WM_INITDIALOG :
		{
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			::SetDlgItemText(_hSelf, IDC_VALUE_STATIC, _name.c_str());
			::SetDlgItemInt(_hSelf, IDC_VALUE_EDIT, _defaultValue, FALSE);

			RECT rc;
			::GetWindowRect(_hSelf, &rc);
			int size = reSizeValueBox();
			::MoveWindow(_hSelf, _p.x, _p.y, rc.right - rc.left + size, rc.bottom - rc.top, TRUE);
			
			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
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

		case WM_ERASEBKGND:
		{
			if (NppDarkMode::isEnabled())
			{
				RECT rc = {};
				getClientRect(rc);
				::FillRect(reinterpret_cast<HDC>(wParam), &rc, NppDarkMode::getDarkerBackgroundBrush());
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
				case IDOK :
				{
					int i = ::GetDlgItemInt(_hSelf, IDC_VALUE_EDIT, NULL, FALSE);
					::EndDialog(_hSelf, i);
					return TRUE;
				}

				case IDCANCEL :
					::EndDialog(_hSelf, -1);
					return TRUE;

				default:
					return FALSE;
			}
		}
		default :
			return FALSE;
	}
	return FALSE;
}


intptr_t CALLBACK ButtonDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM) 
{
	switch (Message)
	{
		case WM_INITDIALOG :
		{
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
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

		case WM_ERASEBKGND:
		{
			if (NppDarkMode::isEnabled())
			{
				RECT rc = {};
				getClientRect(rc);
				::FillRect(reinterpret_cast<HDC>(wParam), &rc, NppDarkMode::getDarkerBackgroundBrush());
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
				case IDC_RESTORE_BUTTON :
				{
                    int bs = getButtonStatus();

                    bool isDistractionFree = (bs & buttonStatus_distractionFree) != 0;
                    bool isFullScreen = (bs & buttonStatus_fullscreen) != 0;
                    bool isPostIt = (bs & buttonStatus_postit) != 0;
                    int cmd = 0;
                    if (isDistractionFree)
                    {
                        cmd = IDM_VIEW_DISTRACTIONFREE;
                    }
                    else if (isFullScreen && isPostIt)
                    {
                        // remove postit firstly
                        cmd = IDM_VIEW_POSTIT;
                    }
                    else if (isFullScreen)
                    {
                        cmd = IDM_VIEW_FULLSCREENTOGGLE;
                    }
                    else if (isPostIt)
                    {
                        cmd = IDM_VIEW_POSTIT;
                    }
                    ::SendMessage(_hParent, WM_COMMAND, cmd, 0);
					display(false);
					return TRUE;
				}

				default:
					return FALSE;
			}
		}
		default :
			return FALSE;
	}
	return FALSE;
}

void ButtonDlg::doDialog(bool isRTL) 
{
    if (!isCreated())
			create(IDD_BUTTON_DLG, isRTL);
	display();
}
