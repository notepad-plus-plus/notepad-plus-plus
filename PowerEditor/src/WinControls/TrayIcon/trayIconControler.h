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


#pragma once

#include <windows.h>

#define ADD     NIM_ADD
#define REMOVE  NIM_DELETE

// code d'erreur
#define INCORRECT_OPERATION     1
#define OPERATION_INCOHERENT    2

class trayIconControler
{
public:
  trayIconControler(HWND hwnd, UINT uID, UINT uCBMsg, HICON hicon, const TCHAR *tip);
  int doTrayIcon(DWORD op);
  bool isInTray() const {return _isIconShowed;};

private:
  NOTIFYICONDATA _nid;
  bool _isIconShowed = false;
};

