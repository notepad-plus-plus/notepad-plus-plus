//this file contains definitions not available in gcc 3.4.5,
//but are needed for Notepad++
//the makefile will automatically include this header file

//GetLongPathName = 410
//Multimonitor: 410
#define _WIN32_WINDOWS 0x0410
//Toolbar imagelist = 300
//TCS_BOTTOM = 300
//LVS_EX_BORDERSELECT = 500
//TBSTYLE_EX_HIDECLIPPEDBUTTONS = 501
#define _WIN32_IE 0x501
//Theme (uxtheme)
#define _WIN32_WINNT 0x0501

//#include <windows.h>

#if (_WIN32_IE >= 0x0400)
#define TCN_GETOBJECT           (TCN_FIRST - 3)
#endif 

#if (_WIN32_IE >= 0x0500)
#define RBN_CHEVRONPUSHED   (RBN_FIRST - 10)
#endif      // _WIN32_IE >= 0x0500
