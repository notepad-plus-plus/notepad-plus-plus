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


#include "trayIconControler.h"

trayIconControler::trayIconControler(HWND hwnd, UINT uID, UINT uCBMsg, HICON hicon, const wchar_t *tip)
{
  _nid.cbSize = sizeof(_nid);
  _nid.hWnd = hwnd;
  _nid.uID = uID;
  _nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  _nid.uCallbackMessage = uCBMsg;
  _nid.hIcon = hicon;
  wcscpy_s(_nid.szTip, tip);
  
  _isIconShown = false;
}

int trayIconControler::doTrayIcon(DWORD op)
{
  if (op != ADD && op != REMOVE)
      return INCORRECT_OPERATION;

  if ((_isIconShown && op == ADD) || (!_isIconShown && op == REMOVE))
    return OPERATION_INCOHERENT;

  ::Shell_NotifyIcon(op, &_nid);
  _isIconShown = !_isIconShown;

  return 0;
}

int trayIconControler::reAddTrayIcon()
{
    if (!_isIconShown)
        return -1;

    // we only re-add the tray icon when it has been lost and no longer exists,
    // but we still delete it first to ensure it's not duplicated 
    // due to an incorrect call or something
    ::Shell_NotifyIcon(NIM_DELETE, &_nid);

    // reset state and re-add tray icon
    _isIconShown = false;
    return doTrayIcon(ADD);
}
