// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid      
// misunderstandings, we consider an application to constitute a          
// "derivative work" for the purpose of this license if it does any of the
// following:                                                             
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#ifndef COLORS_H
#define COLORS_H

const COLORREF red			            = RGB(0xFF,    0,    0);
const COLORREF darkRed                  = RGB(0x80,    0,    0);
const COLORREF offWhite		            = RGB(0xFF, 0xFB, 0xF0);
const COLORREF darkGreen	            = RGB(0,    0x80,    0);
const COLORREF liteGreen	            = RGB(0,    0xFF,    0);
const COLORREF blueGreen	            = RGB(0,    0x80, 0x80);
const COLORREF liteRed					= RGB(0xFF, 0xAA, 0xAA);
const COLORREF liteBlueGreen			= RGB(0xAA, 0xFF, 0xC8);

const COLORREF liteBlue		            = RGB(0xA6, 0xCA, 0xF0);
const COLORREF veryLiteBlue             = RGB(0xC4, 0xF9, 0xFD);
const COLORREF extremeLiteBlue          = RGB(0xF2, 0xF4, 0xFF);

const COLORREF darkBlue  	            = RGB(0,       0, 0x80);
const COLORREF blue      	            = RGB(0,       0, 0xFF);
const COLORREF black     	            = RGB(0,       0,    0);
const COLORREF white     	            = RGB(0xFF, 0xFF, 0xFF);
const COLORREF darkGrey      	        = RGB(64,     64,   64);
const COLORREF grey      	            = RGB(128,   128,  128);
const COLORREF liteGrey  	            = RGB(192,   192,  192);
const COLORREF veryLiteGrey  	        = RGB(224,   224,  224);
const COLORREF brown     	            = RGB(128,    64,    0);
//const COLORREF greenBlue 	            = RGB(192,   128,   64);
const COLORREF darkYellow				= RGB(0xFF, 0xC0,    0);
const COLORREF yellow    	            = RGB(0xFF, 0xFF,    0);
const COLORREF lightYellow				= RGB(0xFF, 0xFF, 0xD5);
const COLORREF cyan      	            = RGB(0,    0xFF, 0xFF);
const COLORREF orange		            = RGB(0xFF, 0x80, 0x00);
const COLORREF purple		            = RGB(0x80, 0x00, 0xFF);
const COLORREF deepPurple	            = RGB(0x87, 0x13, 0x97);

const COLORREF extremeLitePurple        = RGB(0xF8, 0xE8, 0xFF);
const COLORREF veryLitePurple           = RGB(0xE7, 0xD8, 0xE9);
const COLORREF liteBerge				= RGB(0xFE, 0xFC, 0xF5);
const COLORREF berge					= RGB(0xFD, 0xF8, 0xE3);
/*
#define RGB2int(color) 
    (((((long)color) & 0x0000FF) << 16) | ((((long)color) & 0x00FF00)) | ((((long)color) & 0xFF0000) >> 16))
*/
#endif //COLORS_H

