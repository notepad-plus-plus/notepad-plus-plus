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
#include "Notepad_plus.h"

const int splitterSize = 8;

const TCHAR COMMAND_ARG_HELP[] = TEXT("Usage :\r\
\r\
notepad++ [--help] [-multiInst] [-noPlugin] [-lLanguage] [-udl=\"My UDL Name\"] [-LlangCode] [-nLineNumber] [-cColumnNumber] [-pPosition] [-xLeftPos] [-yTopPos] [-monitor] [-nosession] [-notabbar] [-ro] [-systemtray] [-loadingTime] [-alwaysOnTop] [-openSession] [-r] [-qn=\"Easter egg name\" | -qt=\"a text to display.\" | -qf=\"D:\\my quote.txt\"] [-qSpeed1|2|3] [-quickPrint] [-settingsDir=\"d:\\your settings dir\\\"] [-openFoldersAsWorkspace]  [-titleAdd=\"additional title bar text\"][filePath]\r\
\r\
--help : This help message\r\
-multiInst : Launch another Notepad++ instance\r\
-noPlugin : Launch Notepad++ without loading any plugin\r\
-l : Open file or Ghost type with syntax highlighting of choice\r\
-udl=\"My UDL Name\": Open file by applying User Defined Language\r\
-L : Apply indicated localization, langCode is browser language code\r\
-n : Scroll to indicated line on filePath\r\
-c : Scroll to indicated column on filePath\r\
-p : Scroll to indicated position on filePath\r\
-x : Move Notepad++ to indicated left side position on the screen\r\
-y : Move Notepad++ to indicated top position on the screen\r\
-monitor: Open file with file monitoring enabled\r\
-nosession : Launch Notepad++ without previous session\r\
-notabbar : Launch Notepad++ without tabbar\r\
-ro : Make the filePath read only\r\
-systemtray : Launch Notepad++ directly in system tray\r\
-loadingTime : Display Notepad++ loading time\r\
-alwaysOnTop : Make Notepad++ always on top\r\
-openSession : Open a session. filePath must be a session file\r\
-r : Open files recursively. This argument will be ignored\r\
     if filePath contain no wildcard character\r\
-qn=\"Easter egg name\": Ghost type easter egg via its name\r\
-qt=\"text to display.\": Ghost type the given text\r\
-qf=\"D:\\my quote.txt\": Ghost type a file content via the file path\r\
-qSpeed : Ghost typing speed. Value from 1 to 3 for slow, fast and fastest\r\
-quickPrint : Print the file given as argument then quit Notepad++\r\
-settingsDir=\"d:\\your settings dir\\\": Override the default settings dir\r\
-openFoldersAsWorkspace: open filePath of folder(s) as workspace\r\
-titleAdd=\"string\": add string to Notepad++ title bar\r\
filePath : file or folder name to open (absolute or relative path name)\r\
");


class Notepad_plus_Window : public Window
{
public:
	void init(HINSTANCE, HWND, const TCHAR *cmdLine, CmdLineParams *cmdLineParams);

	bool isDlgsMsg(MSG *msg) const;

	HACCEL getAccTable() const {
		return _notepad_plus_plus_core.getAccTable();
	};

	bool emergency(const generic_string& emergencySavedDir) {
		return _notepad_plus_plus_core.emergency(emergencySavedDir);
	};

	bool isPrelaunch() const {
		return _isPrelaunch;
	};

	void setIsPrelaunch(bool val) {
		_isPrelaunch = val;
	};

	generic_string getPluginListVerStr() const {
		return _notepad_plus_plus_core.getPluginListVerStr();
	};

	virtual void destroy() {
		if (_hIconAbsent)
			::DestroyIcon(_hIconAbsent);
		::DestroyWindow(_hSelf);
	};

	static const TCHAR * getClassName() {
		return _className;
	};

	HICON getAbsentIcoHandle() {
		return _hIconAbsent;
	};

	static HWND gNppHWND;	//static handle to Notepad++ window, NULL if non-existant

	void setStartupBgColor(COLORREF BgColor);

private:
	Notepad_plus _notepad_plus_plus_core;
	static LRESULT CALLBACK Notepad_plus_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	static const TCHAR _className[32];
	bool _isPrelaunch = false;
	bool _disablePluginsManager = false;

	QuoteParams _quoteParams; // keep the availability of quote parameters for thread using
	std::wstring _userQuote; // keep the availability of this string for thread using

	HICON _hIconAbsent = nullptr;
};
