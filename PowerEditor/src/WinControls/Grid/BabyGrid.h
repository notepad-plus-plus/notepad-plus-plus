//BABYGRID code is copyrighted (C) 20002 by David Hillard
//
//This code must retain this copyright message
//
//Printed BABYGRID message reference and tutorial available.
//email: mudcat@mis.net for more information.

#pragma once

#include <windows.h>
#include "resource.h"

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif //WM_MOUSEWHEEL

#define BGN_LBUTTONDOWN         0x0001
#define BGN_MOUSEMOVE           0x0002
#define BGN_OUTOFRANGE          0x0003
#define BGN_OWNERDRAW           0x0004
#define BGN_SELCHANGE           0x0005
#define BGN_ROWCHANGED          0x0006
#define BGN_COLCHANGED          0x0007
#define BGN_EDITBEGIN           0x0008
#define BGN_DELETECELL          0x0009
#define BGN_EDITEND             0x000A
#define BGN_F1                  0x000B
#define BGN_F2                  0x000C
#define BGN_F3                  0x000D
#define BGN_F4                  0x000E
#define BGN_F5                  0x000F
#define BGN_F6                  0x0010
#define BGN_F7                  0x0011
#define BGN_F8                  0x0012
#define BGN_F9                  0x0013
#define BGN_F10                 0x0014
#define BGN_F11                 0x0015
#define BGN_F12                 0x0016
#define BGN_GOTFOCUS            0x0017
#define BGN_LOSTFOCUS           0x0018
#define BGN_CELLCLICKED         0x0019
#define BGN_CELLDBCLICKED       0x001A
#define BGN_CELLRCLICKED        0x001B



#define BGM_PROTECTCELL                        BABYGRID_USER +  1
#define BGM_SETPROTECT                         BABYGRID_USER +  2
#define BGM_SETCELLDATA                        BABYGRID_USER +  3
#define BGM_GETCELLDATA                        BABYGRID_USER +  4
#define BGM_CLEARGRID                          BABYGRID_USER +  5
#define BGM_SETGRIDDIM                         BABYGRID_USER +  6
#define BGM_DELETECELL                         BABYGRID_USER +  7
#define BGM_SETCURSORPOS                       BABYGRID_USER +  8
#define BGM_AUTOROW                            BABYGRID_USER +  9
#define BGM_GETOWNERDRAWITEM                   BABYGRID_USER + 10
#define BGM_SETCOLWIDTH                        BABYGRID_USER + 11
#define BGM_SETHEADERROWHEIGHT                 BABYGRID_USER + 12
#define BGM_GETTYPE                            BABYGRID_USER + 13
#define BGM_GETPROTECTION                      BABYGRID_USER + 14
#define BGM_DRAWCURSOR                         BABYGRID_USER + 15
#define BGM_SETROWHEIGHT                       BABYGRID_USER + 16
#define BGM_SETCURSORCOLOR                     BABYGRID_USER + 17
#define BGM_SETPROTECTCOLOR                    BABYGRID_USER + 18
#define BGM_SETUNPROTECTCOLOR                  BABYGRID_USER + 19
#define BGM_SETROWSNUMBERED                    BABYGRID_USER + 20
#define BGM_SETCOLSNUMBERED                    BABYGRID_USER + 21 
#define BGM_SHOWHILIGHT                        BABYGRID_USER + 22
#define BGM_GETROWS                            BABYGRID_USER + 23
#define BGM_GETCOLS                            BABYGRID_USER + 24
#define BGM_NOTIFYROWCHANGED                   BABYGRID_USER + 25
#define BGM_NOTIFYCOLCHANGED                   BABYGRID_USER + 26
#define BGM_GETROW                             BABYGRID_USER + 27
#define BGM_GETCOL                             BABYGRID_USER + 28
#define BGM_PAINTGRID                          BABYGRID_USER + 29
#define BGM_GETCOLWIDTH                        BABYGRID_USER + 30
#define BGM_GETROWHEIGHT                       BABYGRID_USER + 31
#define BGM_GETHEADERROWHEIGHT                 BABYGRID_USER + 32
#define BGM_SETTITLEHEIGHT                     BABYGRID_USER + 33

#define BGM_SETHILIGHTCOLOR                    BABYGRID_USER + 34
#define BGM_SETHILIGHTTEXTCOLOR                BABYGRID_USER + 35
#define BGM_SETEDITABLE                        BABYGRID_USER + 36
#define BGM_SETGRIDLINECOLOR                   BABYGRID_USER + 37
#define BGM_EXTENDLASTCOLUMN                   BABYGRID_USER + 38
#define BGM_SHOWINTEGRALROWS                   BABYGRID_USER + 39
#define BGM_SETELLIPSIS                        BABYGRID_USER + 40
#define BGM_SETCOLAUTOWIDTH                    BABYGRID_USER + 41
#define BGM_SETALLOWCOLRESIZE                  BABYGRID_USER + 42
#define BGM_SETTITLEFONT                       BABYGRID_USER + 43
#define BGM_SETHEADINGFONT                     BABYGRID_USER + 44
#define BGM_GETHOMEROW                         BABYGRID_USER + 45
#define BGM_SETLASTVIEW                        BABYGRID_USER + 46
#define BGM_SETINITIALCONTENT                  BABYGRID_USER + 47
#define BGM_SETHILIGHTCOLOR_NOFOCUS            BABYGRID_USER + 48
#define BGM_SETHILIGHTCOLOR_PROTECT            BABYGRID_USER + 49
#define BGM_SETHILIGHTCOLOR_PROTECT_NOFOCUS    BABYGRID_USER + 50
#define BGM_SETTEXTCOLOR                       BABYGRID_USER + 51
#define BGM_SETBACKGROUNDCOLOR                 BABYGRID_USER + 52
#define BGM_SETTITLETEXTCOLOR                  BABYGRID_USER + 53
#define BGM_SETTITLECOLOR                      BABYGRID_USER + 54
#define BGM_SETTITLEGRIDLINECOLOR              BABYGRID_USER + 55

struct BGCELL {
    int row = 0;
    int col = 0;
};

//function forward declarations
ATOM RegisterGridClass(HINSTANCE);
LRESULT CALLBACK GridProc(HWND, UINT, WPARAM, LPARAM);
void SetCell(BGCELL *cell,int row, int col);
