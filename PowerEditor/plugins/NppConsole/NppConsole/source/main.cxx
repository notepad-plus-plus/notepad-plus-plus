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

// main.cxx

#include "staticWnd.hxx"
#include "resources.hxx"

#include <shlwapi.h>
#include <shlobj.h>
#include <string>

#define PLG_FUNCS_COUNT 4
#define NUMDIGIT        64

void pluginCleanUp();

const TCHAR configFileName[]    = TEXT( "NppConsole.ini" );
const TCHAR sectionName[]       = TEXT( "Console" );
const TCHAR iniKeyCommand[]     = TEXT( "Command" );
const TCHAR iniKeyLinePattern[] = TEXT( "LinePattern" );
const TCHAR iniKeyCtrlCAction[] = TEXT( "CtrlCAction" );
const TCHAR iniKeyRaisePanel[]  = TEXT( "PanelRaiseorToggle" );
TCHAR iniFilePath[MAX_PATH];

HANDLE			g_hModule=NULL;
TCHAR			g_plgName[]=_T("NppConsole");
unsigned		g_showWndInd=0;
FuncItem		g_funcItem[PLG_FUNCS_COUNT]={0};
NppData			g_nppData={0};
tTbData			g_tbData = {0};

CStaticWnd		g_staticWnd;
TCHAR			g_savedCmd[MAX_PATH]={0};
TCHAR			g_savedLine[MAX_PATH]={0};
int				g_ctrlCaction = CStaticWnd::CTRL_C_IGNORE;
bool            g_consoleRestart = true;
bool            g_RaisePanel = false;

toolbarIcons	g_ToolBar={0};

extern "C" __declspec(dllexport)
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  reasonForCall, LPVOID lpReserved )
{
	g_hModule = hModule;
	switch (reasonForCall) {
		case DLL_PROCESS_ATTACH:
			break;
		case DLL_PROCESS_DETACH:
            pluginCleanUp();
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
	}

	return TRUE;
}

INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg,
							WPARAM wParam, LPARAM lParam)
{
	static LPCTSTR aboutText=_T("NppConsole is a workaround for windows console.\r\n")
		_T("Double click in console window, will activate the named document.\r\n")
		_T("String after command line of form ${parameters}, will be sent as parameters.\r\n")
		_T("Pattern for line number search after file name: ${LINE}\r\n")
		_T("\r\n")
        _T("With best regards, M.Pobojnyj (mpoboyny@web.de)\r\n")
        _T("Version 1.2.1.1 and newer - VinsWorldcom\r\n");
	static int xScreen=GetSystemMetrics(SM_CXSCREEN);
	static int yScreen=GetSystemMetrics(SM_CYSCREEN);

	switch (uMsg) {
        case WM_INITDIALOG :
		{
			HWND aboutWnd = GetDlgItem(hwndDlg, IDC_EDIT_ABOUT);
			IFR(!aboutWnd, TRUE);
			IFR(!SetWindowText(aboutWnd, aboutText), TRUE);
			if (_tcslen(g_savedLine)) {
				SetWindowText(GetDlgItem(hwndDlg, IDC_EDIT_LINE), g_savedLine);
			}
			RECT wr={0};
			::GetWindowRect(hwndDlg, &wr);
			long w=wr.right-wr.left, h=wr.bottom-wr.top;
			::MoveWindow(hwndDlg, (xScreen-w)/2, (yScreen-h)/2, w, h, FALSE);
			HICON hIcon=(HICON) LoadImage((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 16, 16, LR_SHARED);
			::SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			if (g_ctrlCaction == CStaticWnd::CTRL_C_IGNORE) {
				CheckDlgButton(hwndDlg, IDC_RADIO_IGN, BST_CHECKED);
			}
			else if (g_ctrlCaction == CStaticWnd::CTRL_C_PROCESS) {
				CheckDlgButton(hwndDlg, IDC_RADIO_PROCESS, BST_CHECKED);
			}
			else {
				CheckDlgButton(hwndDlg, IDC_RADIO_RESTR, BST_CHECKED);
			}
            SendMessage( GetDlgItem( hwndDlg, IDC_CHK_PANELTOGGLE ), BM_SETCHECK,
                         ( WPARAM )( g_RaisePanel ? 1 : 0 ), 0 );

            HWND command = GetDlgItem( hwndDlg, IDC_CBO_COMMAND );

            SendMessage( command, CB_ADDSTRING, 0, ( LPARAM )TEXT( "C:\\Windows\\System32\\cmd.exe" ) );
            SendMessage( command, CB_ADDSTRING, 0, ( LPARAM )TEXT( "C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" ) );
            SendMessage( command, CB_ADDSTRING, 0, ( LPARAM )TEXT( "C:\\Windows\\System32\\wsl.exe" ) );
            int err = (int)::SendMessage( command, CB_SELECTSTRING, -1, ( LPARAM )g_savedCmd );
            if ( err == CB_ERR )
            {
                SendMessage( command, CB_ADDSTRING, 0, ( LPARAM )g_savedCmd );
                SendMessage( command, CB_SELECTSTRING, -1, ( LPARAM )g_savedCmd );
            }

            std::string version;
            version = "<a>";
            version += VER_STRING;
            version += "</a>";
            SetDlgItemTextA(hwndDlg, IDC_STC_VER, version.c_str());
		}
			return TRUE;

        case WM_NOTIFY:
        {
            switch (((LPNMHDR)lParam)->code)
            {
                case NM_CLICK:
                case NM_RETURN:
                {
                    PNMLINK pNMLink = (PNMLINK)lParam;
                    LITEM   item    = pNMLink->item;
                    HWND ver = GetDlgItem( hwndDlg, IDC_STC_VER );

                    if ((((LPNMHDR)lParam)->hwndFrom == ver) && (item.iLink == 0))
                        ShellExecute(hwndDlg, TEXT("open"), TEXT("https://github.com/VinsWorldcom/nppConsole"), NULL, NULL, SW_SHOWNORMAL);

                    return TRUE;
                }
            }
            break;
        }

		case WM_COMMAND :
			switch (wParam) {
                case IDC_CHK_PANELTOGGLE:
                {
                    int check = ( int )::SendMessage( GetDlgItem( hwndDlg, IDC_CHK_PANELTOGGLE ),
                                                      BM_GETCHECK, 0, 0 );

                    if ( check & BST_CHECKED )
                        g_RaisePanel = true;
                    else
                        g_RaisePanel = false;
                    return TRUE;
                }

				case IDOK :
				{
					TCHAR cmd[MAX_PATH]={0};
					int cc=GetWindowText(GetDlgItem(hwndDlg, IDC_EDIT_LINE), cmd, MAX_PATH);
                    if (cc > 0)
                        _tcsncpy_s(g_savedLine, cmd, _TRUNCATE);
                    else
                        g_savedLine[0] = TEXT('\0');
					memset(cmd, 0, MAX_PATH*sizeof(TCHAR));

                    HWND commandCombo = GetDlgItem(hwndDlg, IDC_CBO_COMMAND);
                    int sel = (int)::SendMessage(commandCombo, CB_GETCURSEL, 0, 0);
                    if (sel != CB_ERR)
                        cc = (int)::SendMessage(commandCombo, CB_GETLBTEXT, sel, (LPARAM)cmd);
                    else
                        cc = GetWindowText(commandCombo, cmd, MAX_PATH);

                    if (cc > 0)
                        _tcsncpy_s(g_savedCmd, cmd, _TRUNCATE);
                    else
                        g_savedCmd[0] = TEXT('\0');

					SLog("g_savedCmd: "<<g_savedCmd);
					SLog("g_savedLine: "<<g_savedLine);
					if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO_IGN)) {
						g_ctrlCaction = CStaticWnd::CTRL_C_IGNORE;
					}
					else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO_PROCESS)) {
						g_ctrlCaction = CStaticWnd::CTRL_C_PROCESS;
					}
					else {
						g_ctrlCaction = CStaticWnd::CTRL_C_RECREATE;
					}
					g_staticWnd.SetCtrlCAction(g_ctrlCaction);
					IFR(!g_staticWnd.Restart(g_savedCmd, g_savedLine), TRUE);

					::EndDialog(hwndDlg, IDOK);
				}
					return TRUE;
				case IDCANCEL :
					::EndDialog(hwndDlg, IDCANCEL);
					return TRUE;
				default:
					break;
			}
	}
	return FALSE;
}

void menuRestart()
{
    g_staticWnd.Restart(g_savedCmd, g_savedLine);
    g_consoleRestart = false;
}

void AboutPlugin()
{
	DialogBoxParam((HINSTANCE)g_hModule,  MAKEINTRESOURCE(IDD_DIALOG_ABOUT),
				g_nppData._nppHandle, DialogProc, 0);
}

void ShowPlugin()
{
	static bool wndFlag=false;
	IFV(!g_nppData._nppHandle);
	// HMENU hMenu = ::GetMenu(g_nppData._nppHandle);
	// UINT state = ::GetMenuState(hMenu, g_funcItem[g_showWndInd]._cmdID, MF_BYCOMMAND);
	if (g_staticWnd.isWindowVisible() && !(g_RaisePanel)) {
		g_staticWnd.Hide();
		::SendMessage(g_nppData._nppHandle, NPPM_SETMENUITEMCHECK, g_funcItem[g_showWndInd]._cmdID, MF_UNCHECKED);
	}
	else {
		if (!wndFlag) {
			::SendMessage(g_nppData._nppHandle, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&g_tbData);
			wndFlag=true;

		}
		g_staticWnd.Show();
		::SendMessage(g_nppData._nppHandle, NPPM_SETMENUITEMCHECK, g_funcItem[g_showWndInd]._cmdID, MF_CHECKED);
	}

    if ( g_consoleRestart )
        menuRestart();
}

void pluginCleanUp()
{
    if (!iniFilePath[0])
        return;

    TCHAR buf[NUMDIGIT];

    ::WritePrivateProfileString( sectionName, iniKeyCommand,
                                 g_savedCmd, iniFilePath);
    ::WritePrivateProfileString( sectionName, iniKeyLinePattern,
                                 g_savedLine, iniFilePath);
    _itot_s( g_ctrlCaction, buf, NUMDIGIT, 10 );
    ::WritePrivateProfileString( sectionName, iniKeyCtrlCAction, buf,
                                 iniFilePath );
    ::WritePrivateProfileString( sectionName, iniKeyRaisePanel,
                                 g_RaisePanel ? TEXT( "1" ) : TEXT( "0" ), iniFilePath );

    if (g_ToolBar.hToolbarBmp) ::DeleteObject(g_ToolBar.hToolbarBmp);
}

extern "C" __declspec(dllexport)
void setInfo(NppData nppData)
{
	SLog(__FUNCTION__);
	char modName[MAX_PATH] = {0};
	g_nppData=nppData;

    // get path of plugin configuration
    ::SendMessage( nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH,
                   ( LPARAM )iniFilePath );

    // if config path doesn't exist, we create it
    if ( PathFileExists( iniFilePath ) == FALSE )
        ::CreateDirectory( iniFilePath, NULL );

    // make your plugin config file full file path name
    PathAppend( iniFilePath, configFileName );
    g_ctrlCaction = ::GetPrivateProfileInt( sectionName, iniKeyCtrlCAction,
                                            0, iniFilePath );
    g_RaisePanel  = ::GetPrivateProfileInt( sectionName, iniKeyRaisePanel,
                                            0, iniFilePath );
    ::GetPrivateProfileString( sectionName, iniKeyCommand, TEXT("C:\\Windows\\System32\\cmd.exe"),
                               g_savedCmd, MAX_PATH, iniFilePath );
    ::GetPrivateProfileString( sectionName, iniKeyLinePattern, TEXT(""),
                               g_savedLine, MAX_PATH, iniFilePath );

	TCHAR cmd[MAX_PATH]={0}, sysDir[MAX_PATH]={0};
	g_staticWnd.SetCtrlCAction(g_ctrlCaction);
	g_tbData.hClient=g_staticWnd.Create(g_nppData._nppHandle, g_savedCmd, g_savedLine);
	if (!g_tbData.hClient) {
		memset(cmd, 0, MAX_PATH*sizeof(TCHAR));
		if (GetSystemDirectory(sysDir,MAX_PATH) && 0 < _stprintf_s(cmd, _T("%s\\cmd.exe"), sysDir)) {
			::memcpy(g_savedCmd, cmd, MAX_PATH*sizeof(TCHAR));
			g_tbData.hClient=g_staticWnd.Create(g_nppData._nppHandle, g_savedCmd, g_savedLine);
		}
	}
	IFV(!g_tbData.hClient);
	g_tbData.uMask = DWS_DF_CONT_BOTTOM | DWS_ICONTAB;
	::GetModuleFileNameA((HINSTANCE)g_hModule, modName, MAX_PATH);
	g_tbData.pszModuleName = modName;
    g_tbData.dlgID = g_showWndInd;
	g_tbData.pszName=g_plgName;
	g_tbData.hIconTab = ( HICON )::LoadImage( (HINSTANCE)g_hModule,
		MAKEINTRESOURCE( IDI_APPICON ), IMAGE_ICON, 0, 0,
		LR_LOADTRANSPARENT );

}

extern "C" __declspec(dllexport)
const TCHAR * getName()
{
	return g_plgName;
}

extern "C" __declspec(dllexport)
FuncItem * getFuncsArray(int *count)
{
	g_funcItem[2]._pFunc=menuRestart;
	_tcscpy_s(g_funcItem[2]._itemName, _T("&Restart Console"));
	g_funcItem[2]._pShKey=NULL;

	g_funcItem[3]._pFunc=AboutPlugin;
	_tcscpy_s(g_funcItem[3]._itemName, _T("&Settings"));
	g_funcItem[3]._pShKey=NULL;

	g_funcItem[g_showWndInd]._pFunc=ShowPlugin;
	_tcscpy_s(g_funcItem[g_showWndInd]._itemName, _T("&NppConsole Show"));
	g_funcItem[g_showWndInd]._pShKey=NULL;

	*count=PLG_FUNCS_COUNT;
	return g_funcItem;
}

extern "C" __declspec(dllexport)
void beNotified(SCNotification *notifyCode)
{
	if (notifyCode->nmhdr.hwndFrom == g_nppData._nppHandle) {
		if (notifyCode->nmhdr.code == NPPN_TBMODIFICATION) {
			g_ToolBar.hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDB_TLB_IMG), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);
			IFV(!g_ToolBar.hToolbarBmp);
			::SendMessage(g_nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)g_funcItem[g_showWndInd]._cmdID, (LPARAM)&g_ToolBar);
		}
	}

}

extern "C" __declspec(dllexport)
LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport)
BOOL isUnicode()
{
	return TRUE;
}
#endif //UNICODE

