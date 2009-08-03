#ifndef URLCTRL_INCLUDED
#define URLCTRL_INCLUDED

//
#include <Window.h>
#include "Common.h"

class URLCtrl : public Window {
public:
    URLCtrl():_hfUnderlined(0),_hCursor(0), _msgDest(NULL), _cmdID(0), _oldproc(NULL), \
		_linkColor(), _visitedColor(), _clicking(false), _URL(TEXT("")){};

    void create(HWND itemHandle, TCHAR * link, COLORREF linkColor = RGB(0,0,255));
	void create(HWND itemHandle, int cmd, HWND msgDest = NULL);
    void destroy(){
        	if(_hfUnderlined)
                ::DeleteObject(_hfUnderlined);
	        if(_hCursor)
                ::DestroyCursor(_hCursor);
    };

protected :
    generic_string _URL;
    HFONT	_hfUnderlined;
    HCURSOR	_hCursor;

	HWND _msgDest;
	unsigned long _cmdID;

    WNDPROC  _oldproc;
    COLORREF _linkColor;			
    COLORREF _visitedColor;
    bool  _clicking;

    static LRESULT CALLBACK URLCtrlProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam){
        return ((URLCtrl *)(::GetWindowLongPtr(hwnd, GWL_USERDATA)))->runProc(hwnd, Message, wParam, lParam);
    };
    LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
};

#endif //URLCTRL_INCLUDED
