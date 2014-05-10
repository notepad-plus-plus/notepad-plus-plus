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


#ifndef NOTEPAD_PLUS_WINDOW_H
#define NOTEPAD_PLUS_WINDOW_H

#include "Notepad_plus.h"

const TCHAR COMMAND_ARG_HELP[] = TEXT("Usage :\r\
\r\
notepad++ [--help] [-multiInst] [-noPlugin] [-lLanguage] [-LlangCode] [-nLineNumber] [-cColumnNumber] [-xPos] [-yPos] [-nosession] [-notabbar] [-ro] [-systemtray] [-loadingTime] [-alwaysOnTop] [-openSession] [-r] [fullFilePathName]\r\
\r\
    --help : This help message\r\
    -multiInst : Launch another Notepad++ instance\r\
    -noPlugin : Launch Notepad++ without loading any plugin\r\
    -l : Launch Notepad++ by applying indicated programming language\r\
         to the file to open\r\
    -L : Launch Notepad++ by applying indicated localization\r\
         langCode is browser language code\r\
    -n : Launch Notepad++ by scrolling indicated line on the file to open\r\
    -c : Launch Notepad++ on scrolling indicated column on the file to open\r\
    -x : Launch Notepad++ by indicating its left side position on the screen\r\
    -y : Launch Notepad++ by indicating its top position on the screen\r\
    -nosession : Launch Notepad++ without any session\r\
    -notabbar : Launch Notepad++ without tabbar\r\
    -ro : Launch Notepad++ and make the file to open read only\r\
    -systemtray : Launch Notepad++ directly in system tray\r\
    -loadingTime : Display Notepad++ loading time\r\
    -alwaysOnTop : Make Notepad++ always on top\r\
    -openSession : Open a specific session. fullFilePathName must be a session file\r\
    -r : Open files recursively. This argument will be ignored\r\
         if fullFilePathName contain no wildcard character\r\
    fullFilePathName : file or folder name to open (absolute or relative path name)\r\
");

class Notepad_plus_Window : public Window {
public:
	Notepad_plus_Window() {};
	void init(HINSTANCE, HWND, const TCHAR *cmdLine, CmdLineParams *cmdLineParams);

	bool isDlgsMsg(MSG *msg) const;
	
	HACCEL getAccTable() const {
		return _notepad_plus_plus_core.getAccTable();
	};
	
	bool emergency(generic_string emergencySavedDir) {
		return _notepad_plus_plus_core.emergency(emergencySavedDir);
	};

	bool isPrelaunch() const {
		return _isPrelaunch;
	};

	void setIsPrelaunch(bool val) {
		_isPrelaunch = val;
	};

    virtual void destroy(){
        ::DestroyWindow(_hSelf);
    };

	static const TCHAR * Notepad_plus_Window::getClassName() {
		return _className;
	};
	static HWND gNppHWND;	//static handle to Notepad++ window, NULL if non-existant
	
private:
	Notepad_plus _notepad_plus_plus_core;
	static LRESULT CALLBACK Notepad_plus_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	static const TCHAR _className[32];
	bool _isPrelaunch;
	bool _disablePluginsManager;
};

#endif //NOTEPAD_PLUS_WINDOW_H
