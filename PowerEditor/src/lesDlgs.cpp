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


intptr_t CALLBACK ButtonDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM /*lParam*/)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
			return TRUE;
		}

		case WM_CTLCOLORDLG:
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

		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDC_RESTORE_BUTTON:
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
