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

#include <windows.h>
#include <shlwapi.h>

const int CMD_LEN = 512;
const int PARAM_LEN = 1024;
const char *NPP = "\\notepad++.exe";
const char *FLAG_LEXER_HTML = "-lhtml ";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpszCmdLine, int nCmdShow)
{
	char cmd[CMD_LEN];
	::GetModuleFileName(NULL, cmd, CMD_LEN);
	PathRemoveFileSpec(cmd);
	strcat(cmd, NPP);

	char param[PARAM_LEN] = "";
	
	strcat(strcat(param, FLAG_LEXER_HTML), lpszCmdLine);
	::MessageBox(NULL, param, "", MB_OK);
	HINSTANCE hInst = ::ShellExecute(NULL, "open", cmd, param, ".", SW_SHOW);
	return (UINT)0;
}

