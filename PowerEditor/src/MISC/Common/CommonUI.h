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

#pragma once

#include <wtypes.h>

#include "StringUtils.h"

generic_string folderBrowser(HWND parent, const generic_string& title, int outputCtrlID = 0, const TCHAR* defaultStr = nullptr);
generic_string getFolderName(HWND parent, const TCHAR* defaultDir = nullptr);

HWND CreateToolTip(int toolID, HWND hDlg, HINSTANCE hInst, const PTSTR pszText);
HWND CreateToolTipRect(int toolID, HWND hWnd, HINSTANCE hInst, const PTSTR pszText, const RECT rc);
