//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#ifndef SCINTILLA_H
#include "Scintilla.h"
#endif //SCINTILLA_H

#ifndef NOTEPAD_PLUS_MSGS_H
#include "Notepad_plus_msgs.h"
#endif //NOTEPAD_PLUS_MSGS_H

const int nbChar = 64;

typedef const TCHAR * (__cdecl * PFUNCGETNAME)();

struct NppData {
	HWND _nppHandle;
	HWND _scintillaMainHandle;
	HWND _scintillaSecondHandle;
};

typedef void (__cdecl * PFUNCSETINFO)(NppData);
typedef void (__cdecl * PFUNCPLUGINCMD)();
typedef void (__cdecl * PBENOTIFIED)(SCNotification *);
typedef LRESULT (__cdecl * PMESSAGEPROC)(UINT Message, WPARAM wParam, LPARAM lParam);


struct ShortcutKey {
	bool _isCtrl;
	bool _isAlt;
	bool _isShift;
	UCHAR _key;
};

struct FuncItem {
	TCHAR _itemName[nbChar];
	PFUNCPLUGINCMD _pFunc;
	int _cmdID;
	bool _init2Check;
	ShortcutKey *_pShKey;
};

typedef FuncItem * (__cdecl * PFUNCGETFUNCSARRAY)(int *);

// You should implement (or define an empty function body) those functions which are called by Notepad++ plugin manager
extern "C" __declspec(dllexport) void setInfo(NppData);
extern "C" __declspec(dllexport) const TCHAR * getName();
extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *);
extern "C" __declspec(dllexport) void beNotified(SCNotification *);
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam);

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode();
#endif //UNICODE

#endif //PLUGININTERFACE_H
