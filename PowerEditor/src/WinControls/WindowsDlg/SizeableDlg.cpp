#include <windows.h>
#include "WindowsDlg.h"
#include "WindowsDlgRc.h"

SizeableDlg::SizeableDlg(WINRECT* pWinMap)
	: MyBaseClass(), _winMgr(pWinMap)
{
}

BOOL SizeableDlg::onInitDialog()
{
	_winMgr.InitToFitSizeFromCurrent(_hSelf);
	_winMgr.CalcLayout(_hSelf);
	_winMgr.SetWindowPositions(_hSelf);
	//getClientRect(_rc);
	return TRUE;
}

void SizeableDlg::onSize(UINT nType, int cx, int cy)
{
	_winMgr.CalcLayout(cx,cy,_hSelf);
	_winMgr.SetWindowPositions(_hSelf);
}

void SizeableDlg::onGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	_winMgr.GetMinMaxInfo(_hSelf, lpMMI);
}

LRESULT SizeableDlg::onWinMgr(WPARAM wp, LPARAM lp)
{
	return 0;
}

BOOL CALLBACK SizeableDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
	case WM_INITDIALOG :
		return onInitDialog();

	case WM_GETMINMAXINFO :
		onGetMinMaxInfo((MINMAXINFO*)lParam);
		return TRUE;

	case WM_SIZE:
		onSize(wParam, LOWORD(lParam), HIWORD(lParam));
		return TRUE;
	
	default:
		if (message == WM_WINMGR)
		{
			return (BOOL)onWinMgr(wParam, lParam);
		}
		break;
	}
	return FALSE;
}