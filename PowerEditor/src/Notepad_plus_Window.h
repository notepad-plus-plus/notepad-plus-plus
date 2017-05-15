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
#pragma once
#include "Notepad_plus.h"



const TCHAR COMMAND_ARG_HELP[] = TEXT("Usage :\r\
\r\
notepad++ [--help] [-multiInst] [-noPlugin] [-lLanguage] [-LlangCode] [-nLineNumber] [-cColumnNumber] [-pPosition] [-xLeftPos] [-yTopPos] [-nosession] [-notabbar] [-ro] [-systemtray] [-loadingTime] [-alwaysOnTop] [-openSession] [-r] [-qnEsterEggName | -qtText | -qfCntentFileName] [filePath]\r\
\r\
--help :\t\tThis help message\r\
-multiInst :\tLaunch another Notepad++ instance\r\
-noPlugin :\tLaunch Notepad++ without loading any plugin\r\
-l :\t\tOpen filePath by applying indicated programming language\r\
-L :\t\tApply indicated localization, langCode is browser \r\
    \t\tlanguage code\r\
-n :\t\tScroll to indicated line on filePath\r\
-c :\t\tScroll to indicated column on filePath\r\
-p :\t\tScroll to indicated position on filePath\r\
-x :\t\tMove Notepad++ to indicated left side position on \r\
    \t\tthe screen\r\
-y :\t\tMove Notepad++ to indicated top position on the screen\r\
-nosession :\tLaunch Notepad++ without previous session\r\
-notabbar :\tLaunch Notepad++ without tabbar\r\
-ro :\t\tMake the filePath read only\r\
-systemtray :\tLaunch Notepad++ directly in system tray\r\
-loadingTime :\tDisplay Notepad++ loading time\r\
-alwaysOnTop :\tMake Notepad++ always on top\r\
-openSession :\tOpen a session. filePath must be a session file\r\
-r :\t\tOpen files recursively. This argument will be ignored\r\
    \t\tif filePath contain no wildcard character\r\
-qn :\t\tLaunch ghost typing to display easter egg via its name\r\
-qt :\t\tLaunch ghost typing to display a text via the given text\r\
-qf :\t\tLaunch ghost typing to display a file content via the file path\r\
filePath :\t\tFile or folder name to open (absolute or relative path name)\
");





class Notepad_plus_Window : public Window
{
public:
	void init(HINSTANCE, HWND, const TCHAR *cmdLine, CmdLineParams *cmdLineParams);

	bool isDlgsMsg(MSG *msg) const;

	HACCEL getAccTable() const
	{
		return _notepad_plus_plus_core.getAccTable();
	}

	bool emergency(generic_string emergencySavedDir)
	{
		return _notepad_plus_plus_core.emergency(emergencySavedDir);
	}

	bool isPrelaunch() const
	{
		return _isPrelaunch;
	}

	void setIsPrelaunch(bool val)
	{
		_isPrelaunch = val;
	}

	virtual void destroy()
	{
		::DestroyWindow(_hSelf);
	}

	static const TCHAR * getClassName()
	{
		return _className;
	}

	static HWND gNppHWND;	//static handle to Notepad++ window, NULL if non-existant


private:
	Notepad_plus _notepad_plus_plus_core;
	static LRESULT CALLBACK Notepad_plus_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	static const TCHAR _className[32];
	bool _isPrelaunch = false;
	bool _disablePluginsManager = false;
	std::string _userQuote; // keep the availability of this string for thread using
};
