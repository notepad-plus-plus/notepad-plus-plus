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

// staticWnd.cxx

#include "staticWnd.hxx"
#include <Notepad_plus_rc.h>
#include <Notepad_plus_msgs.h>
#include <stdlib.h>

#define WM_CONSOLE_DBKLICK	WM_USER+1
#define WM_CONSOLE_CTRL_C	WM_USER+2

extern FuncItem		g_funcItem[];
extern unsigned		g_showWndInd;
extern HANDLE		g_hModule;
extern NppData		g_nppData;

HWND CStaticWnd::s_hWnd = 0;
HHOOK CStaticWnd::s_oldHookMouse = 0, CStaticWnd::s_oldHookKeyBoard = 0;

CStaticWnd::CStaticWnd()
	: m_hParWnd(NULL), m_hConWnd(NULL), 
	  m_hStdOut(INVALID_HANDLE_VALUE), m_conWndStyle(0), m_showFlag(SW_HIDE), m_ctrlCAction(CTRL_C_IGNORE),
	  m_pGetCurrentConsoleFont(NULL), m_pGetConsoleFontSize(NULL)
{
	memset(&m_pi, 0, sizeof(PROCESS_INFORMATION));
	memset(&m_curPos, 0, sizeof(WINDOWPOS));
	memset(&m_conFontSize, 0, sizeof(COORD));
	HMODULE kernelDll=::LoadLibrary(_T("Kernel32.dll"));
	IFV(!kernelDll);
	if (kernelDll) {
		m_pGetCurrentConsoleFont=(pGetCurrentConsoleFont)::GetProcAddress(kernelDll, "GetCurrentConsoleFont");
		m_pGetConsoleFontSize=(pGetConsoleFontSize)::GetProcAddress(kernelDll, "GetConsoleFontSize");
	}
	IFV(!m_pGetCurrentConsoleFont || !m_pGetConsoleFontSize);
}

CStaticWnd::~CStaticWnd()
{
	if (m_pi.hProcess) {
		if (WAIT_TIMEOUT == WaitForSingleObject(m_pi.hProcess, 0)) {
			SLog("TerminateProcess");
			TerminateProcess(m_pi.hProcess, 0);
		}
		CloseHandle(m_pi.hProcess);
		m_pi.hProcess = NULL;
	}
	if (m_pi.hThread) {
		CloseHandle(m_pi.hThread);
		m_pi.hThread = NULL;
	}
	m_pi.dwProcessId = 0;
	m_pi.dwThreadId = 0;
	FreeConsole();
	if(s_oldHookMouse) UnhookWindowsHookEx(s_oldHookMouse);
	if(s_oldHookKeyBoard) UnhookWindowsHookEx(s_oldHookKeyBoard);
}

LRESULT CALLBACK CStaticWnd::WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CStaticWnd *pWnd = (CStaticWnd*)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (uMsg) {
		case WM_NOTIFY: 
		{
			LPNMHDR	pnmh = (LPNMHDR)lParam;
			if (pnmh->hwndFrom == pWnd->m_hParWnd) {
				switch (LOWORD(pnmh->code)) {
					case DMN_CLOSE:
					{
						::SendMessage(pWnd->m_hParWnd, NPPM_SETMENUITEMCHECK, g_funcItem[g_showWndInd]._cmdID, 0);
						pWnd->Hide(false);
						break;
					}
					case DMN_FLOAT:
					{
						break;
					}
					case DMN_DOCK:
					{
						break;
					}
				}
			}
			break;
		}
		
		case WM_WINDOWPOSCHANGED:
		{
			LPWINDOWPOS currPos=(LPWINDOWPOS)lParam;
			SLog("WM_WINDOWPOSCHANGED: currPos.cx: "<<currPos->cx<<" currPos.cy: "<<currPos->cy);
			SLog("currPos->x: "<<currPos->x<<" currPos->y: "<<currPos->y);
			pWnd->m_curPos=*currPos;
			break;
		}
		
		case WM_ERASEBKGND:
		{
			RECT conRect={0,0,pWnd->m_curPos.cx+pWnd->m_curPos.x, pWnd->m_curPos.cy+pWnd->m_curPos.y};
			AdjustWindowRect(&conRect, pWnd->m_conWndStyle, FALSE);
			MoveWindow(pWnd->m_hConWnd, conRect.left, conRect.top, (-conRect.left)+conRect.right, (-conRect.top)+conRect.bottom, FALSE);
			::SendMessage(pWnd->m_hConWnd, WM_NCPAINT, 1, 0);
			::InvalidateRect(pWnd->m_hConWnd, 0, true);
			return TRUE;
		}
		case WM_CONSOLE_DBKLICK:
		{
			SLog("WM_CONSOLE_DBKLICK: wParam: "<<wParam<<" lParam: "<<lParam);
			pWnd->ProcessConsoleDBClick(wParam, lParam);
			return TRUE;
		}
		case WM_CONSOLE_CTRL_C:
		{
			SLog("WM_CONSOLE_CTRL_C");
			pWnd->ProcessConsoleCtrlC();
			return TRUE;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT WINAPI CStaticWnd::MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	static DWORD s_lastUp=0;
	static const UINT dbClickTime = GetDoubleClickTime();
	if (nCode < 0) {
		return CallNextHookEx(s_oldHookMouse, nCode, wParam, lParam);
	}
	if (nCode == HC_ACTION && wParam == WM_LBUTTONUP) {
		PMSLLHOOKSTRUCT hs=(PMSLLHOOKSTRUCT)lParam;
		CStaticWnd *pWnd = (CStaticWnd*)::GetWindowLongPtr(s_hWnd, GWLP_USERDATA);
		if (WindowFromPoint(hs->pt)==pWnd->m_hConWnd) {
			if (::GetTickCount()-s_lastUp < dbClickTime) {
				::PostMessage(s_hWnd, WM_CONSOLE_DBKLICK, hs->pt.x, hs->pt.y);
			}
			else {
				s_lastUp=::GetTickCount();
			}
		}
	}
	return CallNextHookEx(s_oldHookMouse, nCode, wParam, lParam);
}

LRESULT WINAPI CStaticWnd::KeyBoardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	static bool bCtrlDown = false;
	if (nCode < 0 || HC_ACTION != nCode) {
		return CallNextHookEx(s_oldHookKeyBoard, nCode, wParam, lParam);
	}
	KBDLLHOOKSTRUCT &hs = *((PKBDLLHOOKSTRUCT)lParam);
	if (WM_KEYDOWN == wParam) {
		if ((VK_LCONTROL == hs.vkCode || VK_RCONTROL == hs.vkCode) && (hs.flags & LLKHF_INJECTED) == 0) {
			bCtrlDown = true;
		}
		// 0x43 - C
		if (0x43 == hs.vkCode && (hs.flags & LLKHF_INJECTED) == 0 && bCtrlDown) {
			CStaticWnd *pWnd = (CStaticWnd*)::GetWindowLongPtr(s_hWnd, GWLP_USERDATA);
			IFR(!pWnd, CallNextHookEx(s_oldHookKeyBoard, nCode, wParam, lParam));
			HWND forGrWnd = ::GetForegroundWindow();
			IFR(!forGrWnd || pWnd->m_hConWnd != forGrWnd, CallNextHookEx(s_oldHookKeyBoard, nCode, wParam, lParam));
			::PostMessage(s_hWnd, WM_CONSOLE_CTRL_C, 0, 0);
			return TRUE;
		}
	}
	if (WM_KEYUP == wParam) {
		if (VK_LCONTROL == hs.vkCode || VK_RCONTROL == hs.vkCode) {
			bCtrlDown = false;
		}
	}
	return CallNextHookEx(s_oldHookKeyBoard, nCode, wParam, lParam);
}

BOOL CStaticWnd::CreateConsoleProcess(LPCTSTR cmd)
{
	SLogFu;
	SLog("cmd: "<<cmd);
	Str cmdTmp = cmd, paramTmp;
	STARTUPINFO si={0};
	LPCTSTR param = _tcsstr(cmd, _T("${"));
	TCHAR tmpParam[MAX_PATH] = {0};
	if (param) {
		if (_tcslen(param) > 3) {
			SLog("param: "<<param);
			param+=2;
			LPCTSTR paramEnd = _tcschr(param, _T('}'));
			if (paramEnd){
				SLog("paramEnd: "<<paramEnd);
				unsigned paramLen = _tcslen(param),
						 paramEndLen = _tcslen(paramEnd);
				const size_t parsedLen = paramLen - paramEndLen;
				if (parsedLen < MAX_PATH) {
					_tcsncpy_s(tmpParam, param, parsedLen);
					paramTmp = tmpParam;
					SLog("tmpParam: "<<tmpParam);
					Str tmpStr = cmdTmp.substr(0, param - cmd - 2);
					SLog("tmpStr: "<<tmpStr);
					cmdTmp = tmpStr;
				}
				else {
					goto CreateConsoleProcess_err;
				}
			}
			else {
				goto CreateConsoleProcess_err;
			}
		}
		else {
			goto CreateConsoleProcess_err;
		}
	}
	if (m_pi.hProcess) {
		if (WAIT_TIMEOUT == WaitForSingleObject(m_pi.hProcess, 0)) {
			SLog(__FUNCTION__<<" TerminateProcess");
			TerminateProcess(m_pi.hProcess, 0);
		}
		CloseHandle(m_pi.hProcess);
		m_pi.hProcess = NULL;
	}
	if (m_pi.hThread) {
		CloseHandle(m_pi.hThread);
		m_pi.hThread = NULL;
	}
	m_pi.dwProcessId = 0;
	m_pi.dwThreadId = 0;
	
	si.cb = sizeof(si);
	si.dwXCountChars=500;
	si.dwYCountChars=300;
	si.wShowWindow=SW_HIDE;
	si.dwFlags=STARTF_USESHOWWINDOW|STARTF_USECOUNTCHARS;
	TCHAR processParam[MAX_PATH] = {0};
	if (!paramTmp.empty())
		_tcsncpy_s(processParam, paramTmp.c_str(), _TRUNCATE);

	if (!CreateProcess(cmdTmp.c_str(), paramTmp.empty() ? NULL : processParam, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &m_pi)) {
		cmdTmp = m_cmd;
		paramTmp = m_param;
		memset(processParam, 0, sizeof(processParam));
		if (!paramTmp.empty())
			_tcsncpy_s(processParam, paramTmp.c_str(), _TRUNCATE);
		IFR(!CreateProcess(cmdTmp.c_str(), paramTmp.empty() ? NULL : processParam, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &m_pi), NULL);
	}
	WaitForInputIdle(m_pi.hProcess, 3000);
	m_cmd = cmdTmp;
	m_param = paramTmp;
	return TRUE;
CreateConsoleProcess_err:
	SLog("CreateConsoleProcess_err");
	return FALSE;
}

HWND CStaticWnd::Create(HWND hParWnd, LPCTSTR cmd, LPCTSTR matchLine)
{
	IFR(!m_pGetCurrentConsoleFont || !m_pGetConsoleFontSize, NULL);
	IFR(!CreateConsoleProcess(cmd), NULL);
	WNDCLASSEX wcx={0};
	wcx.cbSize =        sizeof(wcx);
	wcx.style =         CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc =   WinProc; 
	wcx.hInstance =     (HINSTANCE)g_hModule;
	wcx.hbrBackground = (HBRUSH) (COLOR_WINDOW); 
	wcx.hCursor =       LoadCursor(NULL, IDC_ARROW);
	wcx.lpszClassName = _T("CmdParent");
	IFR(!RegisterClassEx(&wcx), NULL);
	m_hParWnd=hParWnd;
	s_hWnd=CreateWindow(_T("CmdParent"), _T("Cmd"), WS_CLIPCHILDREN|WS_CHILD, 0,
				  0, 100, 100, m_hParWnd, NULL, (HINSTANCE)g_hModule, NULL);                   
	IFR(!s_hWnd, NULL);
	::SendMessage(m_hParWnd, NPPM_MODELESSDIALOG, MODELESSDIALOGADD, (WPARAM)s_hWnd);
	 LONG_PTR pLast=::SetWindowLongPtr(s_hWnd, GWLP_USERDATA, (LONG_PTR)this);
	m_matchLine=matchLine;
	return s_hWnd;
}

bool CStaticWnd::Restart(LPCTSTR cmd, LPCTSTR matchLine)
{
	if (Restart(cmd)) {
		m_matchLine=matchLine;
		return true;
	}
	return Restart(m_cmd.c_str());
}

bool CStaticWnd::Restart(LPCTSTR cmd)
{
	SLogFu;
	DWORD showFlag = m_showFlag;
	IFR(!m_pGetCurrentConsoleFont || !m_pGetConsoleFontSize, NULL);
	Hide(false);
	FreeConsole();
	IFR(!CreateConsoleProcess(cmd), false);
	if (SW_SHOW == showFlag) {
		Show(false);
	}
	return true;
}

HWND CStaticWnd::GetHandle()
{
	return s_hWnd;
}

void CStaticWnd::Show(bool notifyPar)
{
	if (notifyPar) SendMessage(m_hParWnd, NPPM_DMMSHOW, 0, (LPARAM)s_hWnd);
	
	FreeConsole();
	IFV(!AttachConsole(m_pi.dwProcessId));
	
	m_hConWnd=GetConsoleWindow();
	IFV(!m_hConWnd);

	m_hStdOut=GetStdHandle(STD_OUTPUT_HANDLE);
	IFV(INVALID_HANDLE_VALUE==m_hStdOut);

	CONSOLE_FONT_INFO finf={0};
	IFV(!m_pGetCurrentConsoleFont(m_hStdOut, FALSE, &finf));
	m_conFontSize=m_pGetConsoleFontSize(m_hStdOut, finf.nFont);

	IFV(!SetParent(m_hConWnd, s_hWnd));
	m_conWndStyle=GetWindowLong(m_hConWnd, GWL_STYLE);

	if(s_oldHookMouse) UnhookWindowsHookEx(s_oldHookMouse);
	if(s_oldHookKeyBoard) UnhookWindowsHookEx(s_oldHookKeyBoard);
	IFV(!(s_oldHookMouse=SetWindowsHookEx(WH_MOUSE_LL, MouseProc, (HINSTANCE)g_hModule, 0)));
    if(m_ctrlCAction != CStaticWnd::CTRL_C_PROCESS) {
	   IFV(!(s_oldHookKeyBoard=SetWindowsHookEx(WH_KEYBOARD_LL, KeyBoardProc, (HINSTANCE)g_hModule, 0)));
    }

	ShowWindow(m_hConWnd, SW_SHOW);
	ShowWindow(s_hWnd, SW_SHOW);
	m_showFlag = SW_SHOW;
}

void CStaticWnd::Hide(bool notifyPar)
{
	if (s_oldHookMouse){
		UnhookWindowsHookEx(s_oldHookMouse);
		s_oldHookMouse=0;
	}
	if (s_oldHookKeyBoard) {
		UnhookWindowsHookEx(s_oldHookKeyBoard);
		s_oldHookKeyBoard=0;
	}
	if (notifyPar) ::SendMessage(m_hParWnd, NPPM_DMMHIDE, 0, (LPARAM)s_hWnd);
	ShowWindow(m_hConWnd, SW_HIDE);
	ShowWindow(s_hWnd, SW_HIDE);
	m_showFlag = SW_HIDE;
}

BOOL CStaticWnd::isWindowVisible()
{
    return ::IsWindowVisible(s_hWnd);
}

void CStaticWnd::ProcessConsoleDBClick(UINT ptX, UINT ptY)
{
	SLog(__FUNCTION__);
	static const TCHAR lineStr[]=_T("${LINE}");
	FoundState searchState=SEARCH_STEP_1;
	RECT conRect={0};
	POINT clickPt={ptX, ptY};
	IFV(!ScreenToClient(m_hConWnd, &clickPt));
	IFV(!GetClientRect(m_hConWnd, &conRect) || !PtInRect(&conRect, clickPt));
	SLog("clickPt.x: "<<clickPt.x<<" clickPt.y:"<<clickPt.y);
	SCROLLINFO hScrInfo={0}, vScrInfo={0};
	hScrInfo.cbSize=vScrInfo.cbSize=sizeof(SCROLLINFO);
	hScrInfo.fMask=vScrInfo.fMask=SIF_POS;
	IFV(!GetScrollInfo(m_hConWnd, SB_HORZ, &hScrInfo));
	SLog("hScrInfo.nPos "<<hScrInfo.nPos<<" hScrInfo.nPage:"<<hScrInfo.nPage);
	IFV(!GetScrollInfo(m_hConWnd, SB_VERT, &vScrInfo));
	SLog("vScrInfo.nPos "<<vScrInfo.nPos<<" vScrInfo.nPage:"<<vScrInfo.nPage);
	SLog("CONSOLE_FONT_INFO dwFontSize.X: "<<m_conFontSize.X<<" dwFontSize.Y: "<<m_conFontSize.Y);
	UINT currCol = clickPt.x/m_conFontSize.X+hScrInfo.nPos;
	UINT currRow = clickPt.y/m_conFontSize.Y+vScrInfo.nPos;
	SLog("Clicked row: "<<currRow<<" col:"<<currCol);
	CHAR_INFO chiBuffer[MAX_PATH]={0};
	COORD coordBufSize={0};
    COORD coordBufCoord={0};
	SMALL_RECT srctReadRect={0};
	// Set the source rectangle.  
    srctReadRect.Top = currRow;    // top left: row col  
    srctReadRect.Left = 0; 
    srctReadRect.Bottom = currRow; // bot. right: row  col 
    srctReadRect.Right = MAX_PATH-1; 
	// The temporary buffer size is 1 row MAX_PATH columns.  
    coordBufSize.Y = 1; 
    coordBufSize.X = MAX_PATH; 
	// The top left destination cell of the temporary buffer is 
    // row 0, col 0.  
    coordBufCoord.X = 0; 
    coordBufCoord.Y = 0;
	BOOL fReadSuccess = ReadConsoleOutput( 
       m_hStdOut,      // screen buffer to read from 
       chiBuffer,      // buffer to copy into 
       coordBufSize,   // col-row size of chiBuffer 
       coordBufCoord,  // top left dest. cell in chiBuffer 
       &srctReadRect); // screen buffer source rectangle
	IFV(!fReadSuccess && srctReadRect.Right);
	Str conText;
	for (int i=srctReadRect.Left; i<srctReadRect.Right; i++) {
#ifdef UNICODE
		conText+=chiBuffer[i].Char.UnicodeChar;
#else
		conText+=chiBuffer[i].Char.AsciiChar;
#endif
	}
	IFV(!conText.length());
	SLog("Clicked line length: "<<conText.length()<<" Clicked line: "<<conText.c_str());
	LPCTSTR currFileCon=NULL;
	const TCHAR* pConText=conText.c_str();
	const TCHAR* pToSearchStart=pConText+currCol;
	const TCHAR* pToSearchEnd=pToSearchStart;
	unsigned foundFileLength=0;
	for (;!_istspace(*pToSearchStart) && pToSearchStart>pConText; pToSearchStart--);
	for (LPCTSTR searchEnd=pConText+_tcslen(pConText);!_istspace(*pToSearchEnd) && pToSearchEnd<searchEnd; pToSearchEnd++);
	int nbFile = (int)::SendMessage(g_nppData._nppHandle, NPPM_GETNBOPENFILES, 0, 0);
	IFV(nbFile<1);
	TCHAR **fileNames = (TCHAR **)new TCHAR*[nbFile];
	for (int i = 0 ; i < nbFile ; i++) {
		fileNames[i] = new TCHAR[MAX_PATH];
	}
	if (!::SendMessage(g_nppData._nppHandle, NPPM_GETOPENFILENAMES, (WPARAM)fileNames, (LPARAM)nbFile)) {
		goto ProcessConsoleDBClick_exit;
	}
ProcessConsoleDBClick_search:
	SLog("Search start: "<<pToSearchStart);
	SLog("Search end: "<<pToSearchEnd);
	for (int i = 0 ; i < nbFile ; i++) {
		TCHAR *currFile=_tcsrchr(fileNames[i], _T('\\'));
		if (currFile) {
			currFileCon=_tcsstr(pToSearchStart, ++currFile);
			if (currFileCon && currFileCon < pToSearchEnd) {
				SLog("File name found: "<<currFileCon<<" index: "<<i);
				foundFileLength=_tcslen(currFile);
				::SendMessage(g_nppData._nppHandle, NPPM_ACTIVATEDOC, 0, i);
				searchState=FILENAME_FOUND;
				break;
			}
		}
	}
	if (searchState==SEARCH_STEP_1) {
		SLog("Second chance");
		searchState=SEARCH_STEP_2;
		pToSearchStart=pConText;
		pToSearchEnd=pConText+_tcslen(pConText);
		goto ProcessConsoleDBClick_search;
	}
	else if (searchState==FILENAME_FOUND && m_matchLine.length()) {
		LPTSTR pattern=_tcsdup(m_matchLine.c_str());
		SLog("pattern: "<<pattern);
		currFileCon+=foundFileLength;
		SLog("currFileCon: "<<currFileCon);
		LPTSTR lineMatch=_tcsstr(pattern, lineStr);
		if (lineMatch) {
			*lineMatch=_T('\0');
			SLog("lineMatch: "<<pattern);
			lineMatch=(LPTSTR)_tcsstr(currFileCon, pattern);
			if (lineMatch) {
				SLog("lineMatch: "<<lineMatch);
				LPTSTR lineDigit=++lineMatch;
				for (;_istdigit(*lineMatch); lineMatch++);
				*lineMatch=_T('\0');
				SLog("lineDigit: "<<lineDigit);
				if (lineDigit) {
					int lineNum=_ttoi(lineDigit);
					if (lineNum>0) {
						SLog("lineNum: "<<lineNum);
						// Get the current scintilla
						int which = -1;
						::SendMessage(g_nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
						if (which == -1)
							goto ProcessConsoleDBClick_noLine;
						HWND curScintilla = (which == 0)?g_nppData._scintillaMainHandle:g_nppData._scintillaSecondHandle;
						::SendMessage(curScintilla, SCI_ENSUREVISIBLE, lineNum-1, 0);
						::SendMessage(curScintilla, SCI_GOTOLINE, lineNum-1, 0);
					}
				}
			}
		}
ProcessConsoleDBClick_noLine:
		free(pattern);
	}
ProcessConsoleDBClick_exit:
	for (int i = 0 ; i < nbFile ; i++){
		delete fileNames[i];
	}
	delete[] fileNames;
}

void CStaticWnd::ProcessConsoleCtrlC()
{
	SLogFu;
	SLog("m_ctrlCAction : "<<m_ctrlCAction);
	IFV(CTRL_C_IGNORE == m_ctrlCAction);
	Str tmp = m_cmd;
	if (m_param.length()) {
		tmp += _T("${") + m_param + _T("}");
	}
	Restart(tmp.c_str());
}

void CStaticWnd::SetCtrlCAction(int action)
{
	SLogFu;
	if (m_ctrlCAction != action) {
		m_ctrlCAction = action;
		SLog("m_ctrlCAction changed to : "<<m_ctrlCAction);
	}
}

