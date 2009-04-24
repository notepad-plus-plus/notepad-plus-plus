#include "trayIconControler.h"
#include <iostream>
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
