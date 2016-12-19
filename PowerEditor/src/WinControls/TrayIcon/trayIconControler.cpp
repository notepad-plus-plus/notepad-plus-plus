// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid      
// misunderstandings, we consider an application to constitute a          
// "derivative work" for the purpose of this license if it does any of the
// following:                                                             
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include "trayIconControler.h"

trayIconControler::trayIconControler(HWND hwnd, UINT uID, UINT uCBMsg, HICON hicon, TCHAR *tip)
{
  _nid.cbSize = sizeof(_nid);
  _nid.hWnd = hwnd;
  _nid.uID = uID;
  _nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  _nid.uCallbackMessage = uCBMsg;
  _nid.hIcon = hicon;
  lstrcpy(_nid.szTip, tip);
  
  ::RegisterWindowMessage(TEXT("TaskbarCreated"));
  _isIconShowed = false;
}

int trayIconControler::doTrayIcon(DWORD op)
{
  if ((op != ADD)&&(op != REMOVE)) return INCORRECT_OPERATION;
  if (((_isIconShowed)&&(op == ADD))||((!_isIconShowed)&&(op == REMOVE)))
    return OPERATION_INCOHERENT;
  ::Shell_NotifyIcon(op, &_nid);
  _isIconShowed = !_isIconShowed;

  return 0;
}
