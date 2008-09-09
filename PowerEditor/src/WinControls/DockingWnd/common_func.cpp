#include "common_func.h"
#include <string>

using namespace std;

void ClientToScreen(HWND hWnd, RECT* rect)
{
	POINT		pt;

	pt.x		 = rect->left;
	pt.y		 = rect->top;
	::ClientToScreen( hWnd, &pt );
	rect->left   = pt.x;
	rect->top    = pt.y;

	pt.x		 = rect->right;
	pt.y		 = rect->bottom;
	::ClientToScreen( hWnd, &pt );
	rect->right  = pt.x;
	rect->bottom = pt.y;
};


void ScreenToClient(HWND hWnd, RECT* rect)
{
	POINT		pt;

	pt.x		 = rect->left;
	pt.y		 = rect->top;
	::ScreenToClient( hWnd, &pt );
	rect->left   = pt.x;
	rect->top    = pt.y;

	pt.x		 = rect->right;
	pt.y		 = rect->bottom;
	::ScreenToClient( hWnd, &pt );
	rect->right  = pt.x;
	rect->bottom = pt.y;
};

void folderBrowser(HWND parent, int outputCtrlID)
{
	// This code was copied and slightly modifed from:
	// http://www.bcbdev.com/faqs/faq62.htm

	// SHBrowseForFolder returns a PIDL. The memory for the PIDL is
	// allocated by the shell. Eventually, we will need to free this
	// memory, so we need to get a pointer to the shell malloc COM
	// object that will free the PIDL later on.
	LPMALLOC pShellMalloc = 0;
	if (::SHGetMalloc(&pShellMalloc) == NO_ERROR)
	{
		// If we were able to get the shell malloc object,
		// then proceed by initializing the BROWSEINFO stuct
		BROWSEINFO info;
		memset(&info, 0, sizeof(info));
		info.hwndOwner = parent;
		info.pidlRoot = NULL;
		TCHAR szDisplayName[MAX_PATH];
		info.pszDisplayName = szDisplayName;
		generic_string title = TEXT("Select a folder to search from");
		info.lpszTitle = title.c_str();
		info.ulFlags = 0;
		info.lpfn = BrowseCallbackProc;
		TCHAR directory[MAX_PATH];
		::GetDlgItemText(parent, outputCtrlID, directory, sizeof(directory));
		info.lParam = reinterpret_cast<LPARAM>(directory);

		// Execute the browsing dialog.
		LPITEMIDLIST pidl = ::SHBrowseForFolder(&info);

		// pidl will be null if they cancel the browse dialog.
		// pidl will be not null when they select a folder.
		if (pidl) 
		{
			// Try to convert the pidl to a display generic_string.
			// Return is true if success.
			TCHAR szDir[MAX_PATH];
			if (::SHGetPathFromIDList(pidl, szDir))
				// Set edit control to the directory path.
				::SetDlgItemText(parent, outputCtrlID, szDir);
			pShellMalloc->Free(pidl);
		}
		pShellMalloc->Release();
	}
}
