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
notepad++ [--help] [-multiInst] [-noPlugin] [-lLanguage] [-LlangCode] [-nLineNumber] [-cColumnNumber] [-xLeftPos] [-yTopPos] [-nosession] [-notabbar] [-ro] [-systemtray] [-loadingTime] [-alwaysOnTop] [-openSession] [-r] [-qnEsterEggName | -qtText | -qfCntentFileName] [filePath]\r\
\r\
    --help : This help message\r\
    -multiInst : Launch another Notepad++ instance\r\
    -noPlugin : Launch Notepad++ without loading any plugin\r\
    -l : Open filePath by applying indicated programming language\r\
    -L : Apply indicated localization, langCode is browser language code\r\
    -n : Scroll to indicated line on filePath\r\
    -c : Scroll to indicated column on filePath\r\
    -x : Move Notepad++ to indicated left side position on the screen\r\
    -y : Move Notepad++ to indicated top position on the screen\r\
    -nosession : Launch Notepad++ without previous session\r\
    -notabbar : Launch Notepad++ without tabbar\r\
    -ro : Make the filePath read only\r\
    -systemtray : Launch Notepad++ directly in system tray\r\
    -loadingTime : Display Notepad++ loading time\r\
    -alwaysOnTop : Make Notepad++ always on top\r\
    -openSession : Open a session. filePath must be a session file\r\
    -r : Open files recursively. This argument will be ignored\r\
         if filePath contain no wildcard character\r\
    -qn : Launch ghost typing to disply easter egg via its name\r\
    -qt : Launch ghost typing to display a text via the given text\r\
    -qf : Launch ghost typing to display a file content via the file path\r\
    filePath : file or folder name to open (absolute or relative path name)\r\
");

class Notepad_plus_Window : public Window {
public:
	Notepad_plus_Window() : _isPrelaunch(false), _disablePluginsManager(false) {};
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

	static const TCHAR * getClassName() {
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
	std::string _userQuote; // keep the availability of this string for thread using 
};

#endif //NOTEPAD_PLUS_WINDOW_H
