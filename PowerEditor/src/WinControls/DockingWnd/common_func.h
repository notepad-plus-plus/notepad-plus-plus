#ifndef COMMON_FUNC_H
#define COMMON_FUNC_H

#include "windows.h"
// need this header for SHBrowseForFolder
#include <Shlobj.h>

void ClientToScreen(HWND hWnd, RECT* rect);
void ScreenToClient(HWND hWnd, RECT* rect);
void folderBrowser(HWND parent, int outputCtrlID);

// Set a call back with the handle after init to set the path.
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/shellcc/platform/shell/reference/callbackfunctions/browsecallbackproc.asp
static int __stdcall BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM, LPARAM pData)
{
	if (uMsg == BFFM_INITIALIZED)
		::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
	return 0;
};
#endif //COMMON_FUNC_H
