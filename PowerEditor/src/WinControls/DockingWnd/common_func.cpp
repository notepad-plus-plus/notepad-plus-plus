#include "common_func.h"
#include <string>
#include "UniConversion.h"

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
		BROWSEINFOW info;
		memset(&info, 0, sizeof(info));
		info.hwndOwner = parent;
		info.pidlRoot = NULL;
		wchar_t szDisplayName[MAX_PATH];
		info.pszDisplayName = szDisplayName;
		wstring title = L"Select a folder to search from";
		info.lpszTitle = title.c_str();
		info.ulFlags = 0;
		info.lpfn = BrowseCallbackProc;
		wchar_t directory[MAX_PATH];
		::GetDlgItemTextW(parent, outputCtrlID, directory, sizeof(directory));
		info.lParam = reinterpret_cast<LPARAM>(directory);

		// Execute the browsing dialog.
		LPITEMIDLIST pidl = ::SHBrowseForFolderW(&info);

		// pidl will be null if they cancel the browse dialog.
		// pidl will be not null when they select a folder.
		if (pidl) 
		{
			// Try to convert the pidl to a display string.
			// Return is true if success.
			wchar_t szDirW[MAX_PATH];
			if (::SHGetPathFromIDListW(pidl, szDirW))
				// Set edit control to the directory path.
				::SetDlgItemTextW(parent, outputCtrlID, szDirW);
			pShellMalloc->Free(pidl);
		}
		pShellMalloc->Release();
	}
}
