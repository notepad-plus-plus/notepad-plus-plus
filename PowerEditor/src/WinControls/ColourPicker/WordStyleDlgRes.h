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


#ifndef WORD_STYLE_DLG_RES_H
#define WORD_STYLE_DLG_RES_H

#define IDD_STYLER_DLG	2200

    //#define IDC_STYLETYPE_COMBO	(IDD_STYLER_DLG + 1)
    #define IDC_FONT_COMBO		(IDD_STYLER_DLG + 2)
    #define IDC_FONTSIZE_COMBO	(IDD_STYLER_DLG + 3)
    #define IDC_BOLD_CHECK		(IDD_STYLER_DLG + 4)
    #define IDC_ITALIC_CHECK		(IDD_STYLER_DLG + 5)
    #define IDC_FG_STATIC			(IDD_STYLER_DLG + 6)
    #define IDC_BG_STATIC			(IDD_STYLER_DLG + 7)
    #define IDC_FONTNAME_STATIC      (IDD_STYLER_DLG + 8) 
    #define IDC_FONTSIZE_STATIC       (IDD_STYLER_DLG + 9)
    //#define IDC_STYLEDEFAULT_WARNING_STATIC (IDD_STYLER_DLG + 10)  for the sake of compablity of traslation xml files, this number (2210) don't be use anymore by Notepad++
    #define IDC_STYLEDESC_STATIC  (IDD_STYLER_DLG + 11)
    #define IDC_COLOURGROUP_STATIC  (IDD_STYLER_DLG + 12)
    #define IDC_FONTGROUP_STATIC  (IDD_STYLER_DLG + 13)
	
	#define IDC_DEF_EXT_STATIC				(IDD_STYLER_DLG + 14)
    #define IDC_DEF_EXT_EDIT					(IDD_STYLER_DLG + 15)
	#define IDC_USER_EXT_STATIC				(IDD_STYLER_DLG + 16)
	#define IDC_USER_EXT_EDIT					(IDD_STYLER_DLG + 17)
	#define IDC_UNDERLINE_CHECK			(IDD_STYLER_DLG + 18)
	#define IDC_DEF_KEYWORDS_STATIC	(IDD_STYLER_DLG + 19)
	#define IDC_DEF_KEYWORDS_EDIT		(IDD_STYLER_DLG + 20)
	#define IDC_USER_KEYWORDS_STATIC	(IDD_STYLER_DLG + 21)
	#define IDC_USER_KEYWORDS_EDIT		(IDD_STYLER_DLG + 22)
	#define IDC_PLUSSYMBOL_STATIC 		(IDD_STYLER_DLG + 23)
	#define IDC_PLUSSYMBOL2_STATIC 		(IDD_STYLER_DLG + 24)
	#define IDC_LANGDESC_STATIC  (IDD_STYLER_DLG + 25)
	
	#define IDC_GLOBAL_FG_CHECK		(IDD_STYLER_DLG + 26)
	#define IDC_GLOBAL_BG_CHECK		(IDD_STYLER_DLG + 27)
	#define IDC_GLOBAL_FONT_CHECK		(IDD_STYLER_DLG + 28)
	#define IDC_GLOBAL_FONTSIZE_CHECK		(IDD_STYLER_DLG + 29)
	#define IDC_GLOBAL_BOLD_CHECK		(IDD_STYLER_DLG + 30)
	#define IDC_GLOBAL_ITALIC_CHECK		(IDD_STYLER_DLG + 31)
	#define IDC_GLOBAL_UNDERLINE_CHECK		(IDD_STYLER_DLG + 32)
	#define IDC_STYLEDESCRIPTION_STATIC	(IDD_STYLER_DLG + 33)
	                                                    
# define IDD_GLOBAL_STYLER_DLG	2300
    #define IDC_SAVECLOSE_BUTTON	(IDD_GLOBAL_STYLER_DLG + 1)
    #define IDC_SC_PERCENTAGE_SLIDER     (IDD_GLOBAL_STYLER_DLG + 2)
	#define IDC_SC_TRANSPARENT_CHECK    (IDD_GLOBAL_STYLER_DLG + 3)
	#define IDC_LANGUAGES_LIST   (IDD_GLOBAL_STYLER_DLG + 4)
	#define IDC_STYLES_LIST       (IDD_GLOBAL_STYLER_DLG + 5)
	#define IDC_SWITCH2THEME_STATIC       (IDD_GLOBAL_STYLER_DLG + 6)
	#define IDC_SWITCH2THEME_COMBO       (IDD_GLOBAL_STYLER_DLG + 7)

#endif //WORD_STYLE_DLG_RES_H

