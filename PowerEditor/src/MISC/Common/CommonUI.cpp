// This file is part of Notepad++ project
// Copyright (C) 2021 The Notepad++ Contributors.

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

#include "CommonUI.h"

#include "nppDarkMode.h"
#include "CustomFileDialog.h"

#include <CommCtrl.h>


generic_string folderBrowser(HWND parent, const generic_string& title, int outputCtrlID, const TCHAR* defaultStr)
{
	generic_string folderName;
	CustomFileDialog dlg(parent);
	dlg.setTitle(title.c_str());

	// Get an initial directory from the edit control or from argument provided
	TCHAR directory[MAX_PATH] = {};
	if (outputCtrlID != 0)
		::GetDlgItemText(parent, outputCtrlID, directory, _countof(directory));
	directory[_countof(directory) - 1] = '\0';
	if (!directory[0] && defaultStr)
		dlg.setFolder(defaultStr);
	else if (directory[0])
		dlg.setFolder(directory);

	folderName = dlg.pickFolder();
	if (!folderName.empty())
	{
		// Send the result back to the edit control
		if (outputCtrlID != 0)
			::SetDlgItemText(parent, outputCtrlID, folderName.c_str());
	}
	return folderName;
}

generic_string getFolderName(HWND parent, const TCHAR* defaultDir)
{
	return folderBrowser(parent, TEXT("Select a folder"), 0, defaultDir);
}

HWND CreateToolTip(int toolID, HWND hDlg, HINSTANCE hInst, const PTSTR pszText)
{
	if (!toolID || !hDlg || !pszText)
	{
		return NULL;
	}

	// Get the window of the tool.
	HWND hwndTool = GetDlgItem(hDlg, toolID);
	if (!hwndTool)
	{
		return NULL;
	}

	// Create the tooltip. g_hInst is the global instance handle.
	HWND hwndTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hDlg, NULL,
		hInst, NULL);

	if (!hwndTip)
	{
		return NULL;
	}

	NppDarkMode::setDarkTooltips(hwndTip, NppDarkMode::ToolTipsType::tooltip);

	// Associate the tooltip with the tool.
	TOOLINFO toolInfo = { 0 };
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = hDlg;
	toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	toolInfo.uId = (UINT_PTR)hwndTool;
	toolInfo.lpszText = pszText;
	if (!SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo))
	{
		DestroyWindow(hwndTip);
		return NULL;
	}

	SendMessage(hwndTip, TTM_ACTIVATE, TRUE, 0);
	SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0, 200);
	// Make tip stay 15 seconds
	SendMessage(hwndTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELPARAM((15000), (0)));

	return hwndTip;
}

HWND CreateToolTipRect(int toolID, HWND hWnd, HINSTANCE hInst, const PTSTR pszText, const RECT rc)
{
	if (!toolID || !hWnd || !pszText)
	{
		return NULL;
	}

	// Create the tooltip. g_hInst is the global instance handle.
	HWND hwndTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hWnd, NULL,
		hInst, NULL);

	if (!hwndTip)
	{
		return NULL;
	}

	// Associate the tooltip with the tool.
	TOOLINFO toolInfo = { 0 };
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = hWnd;
	toolInfo.uFlags = TTF_SUBCLASS;
	toolInfo.uId = toolID;
	toolInfo.lpszText = pszText;
	toolInfo.rect = rc;
	if (!SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo))
	{
		DestroyWindow(hwndTip);
		return NULL;
	}

	SendMessage(hwndTip, TTM_ACTIVATE, TRUE, 0);
	SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0, 200);
	// Make tip stay 15 seconds
	SendMessage(hwndTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELPARAM((15000), (0)));

	return hwndTip;
}
