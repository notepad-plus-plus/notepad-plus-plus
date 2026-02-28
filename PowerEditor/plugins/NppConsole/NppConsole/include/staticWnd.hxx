/*
This file is part of Notepad++ console plugin.
Copyright ©2011 Mykhajlo Pobojnyj <mpoboyny@web.de>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

// staticWnd.hxx

#ifndef _STATICStaticWnd_HXX_
#define _STATICStaticWnd_HXX_

#include <std.hxx>
#include <PluginInterface.h>
#include <Docking.h>

class CStaticWnd
{
	enum FoundState
	{
		SEARCH_STEP_1,
		SEARCH_STEP_2,
		FILENAME_NOT_FOUND,
		FILENAME_FOUND
	};
	
	HWND m_hParWnd, m_hConWnd;
	PROCESS_INFORMATION m_pi;
	HANDLE m_hStdOut;
	WINDOWPOS m_curPos;
	LONG m_conWndStyle;
	DWORD m_showFlag;
	COORD m_conFontSize;
	Str m_cmd, m_param, m_matchLine;
	int m_ctrlCAction;
	
	typedef BOOL (WINAPI *pGetCurrentConsoleFont)(HANDLE hConsoleOutput, BOOL bMaximumWindow, PCONSOLE_FONT_INFO lpConsoleCurrentFont);
	pGetCurrentConsoleFont m_pGetCurrentConsoleFont;

	typedef COORD (WINAPI *pGetConsoleFontSize)(HANDLE hConsoleOutput, DWORD nFont);
	pGetConsoleFontSize m_pGetConsoleFontSize;
	static HHOOK s_oldHookMouse, s_oldHookKeyBoard;
	static HWND s_hWnd;
	static LRESULT CALLBACK WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT WINAPI MouseProc(int, WPARAM, LPARAM);
	static LRESULT WINAPI KeyBoardProc(int, WPARAM, LPARAM);
	
	inline BOOL CreateConsoleProcess(LPCTSTR cmd);
	inline bool Restart(LPCTSTR cmd);
public:
	enum
	{
		CTRL_C_IGNORE 	= 0,
		CTRL_C_RECREATE,
		CTRL_C_PROCESS
	};
	CStaticWnd();
	~CStaticWnd();
	HWND Create(HWND hParWnd, LPCTSTR cmd, LPCTSTR matchLine);
	bool Restart(LPCTSTR cmd, LPCTSTR matchLine);
	HWND GetHandle();
	void Show(bool notifyPar=true);
	void Hide(bool notifyPar=true);
    BOOL isWindowVisible();
	void ProcessConsoleDBClick(UINT ptX, UINT ptY);
	void ProcessConsoleCtrlC();
	void SetCtrlCAction(int action);
};

#endif //_STATICStaticWnd_HXX_
