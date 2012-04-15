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


#ifndef USERDEFINE_RC_H
#define USERDEFINE_RC_H

#define    IDD_GLOBAL_USERDEFINE_DLG 20000
	#define	IDC_DOCK_BUTTON 					 (IDD_GLOBAL_USERDEFINE_DLG + 1)
    #define	IDC_RENAME_BUTTON 			     (IDD_GLOBAL_USERDEFINE_DLG + 2)
    #define	IDC_ADDNEW_BUTTON			     (IDD_GLOBAL_USERDEFINE_DLG + 3)
    #define	IDC_REMOVELANG_BUTTON			         (IDD_GLOBAL_USERDEFINE_DLG + 4)
    #define	IDC_SAVEAS_BUTTON			         (IDD_GLOBAL_USERDEFINE_DLG + 5)
    #define	IDC_LANGNAME_COMBO	             (IDD_GLOBAL_USERDEFINE_DLG + 6)
    #define	IDC_LANGNAME_STATIC	             (IDD_GLOBAL_USERDEFINE_DLG + 7)
    #define	IDC_EXT_EDIT	                            (IDD_GLOBAL_USERDEFINE_DLG + 8)
    #define	IDC_EXT_STATIC	                        (IDD_GLOBAL_USERDEFINE_DLG + 9)

    #define	IDC_UD_PERCENTAGE_SLIDER	           (IDD_GLOBAL_USERDEFINE_DLG + 10)
    #define	IDC_UD_TRANSPARENT_CHECK	       (IDD_GLOBAL_USERDEFINE_DLG + 11)
    #define	IDC_LANGNAME_IGNORECASE_CHECK   (IDD_GLOBAL_USERDEFINE_DLG + 12)
    #define	IDC_AUTOCOMPLET_EDIT   (IDD_GLOBAL_USERDEFINE_DLG + 13)
    #define	IDC_AUTOCOMPLET_STATIC   (IDD_GLOBAL_USERDEFINE_DLG + 14)
    #define	IDC_IMPORT_BUTTON			(IDD_GLOBAL_USERDEFINE_DLG + 15)
    #define	IDC_EXPORT_BUTTON            (IDD_GLOBAL_USERDEFINE_DLG + 16)             
#define    IDD_FOLDER_STYLE_DLG   21000

     #define    IDC_DEFAULT   (IDD_FOLDER_STYLE_DLG + 100)
        #define    IDC_DEFAULT_DESCGROUP_STATIC            (IDC_DEFAULT+ 1)
        #define    IDC_DEFAULT_FG_STATIC                            (IDC_DEFAULT+ 2)
        #define    IDC_DEFAULT_BG_STATIC                            (IDC_DEFAULT + 3) 
        #define    IDC_DEFAULT_FONT_COMBO                       (IDC_DEFAULT+ 4)
        #define    IDC_DEFAULT_FONTSIZE_COMBO                (IDC_DEFAULT + 5) 
        #define    IDC_DEFAULT_BOLD_CHECK                        (IDC_DEFAULT + 6) 
        #define    IDC_DEFAULT_ITALIC_CHECK                       (IDC_DEFAULT + 7)
        #define    IDC_DEFAULT_FONTSTYLEGROUP_STATIC     (IDC_DEFAULT+ 8)
        #define    IDC_DEFAULT_COLORSTYLEGROUP_STATIC  (IDC_DEFAULT + 9) 
        #define    IDC_DEFAULT_FONTNAME_STATIC                (IDC_DEFAULT + 10)
        #define    IDC_DEFAULT_FONTSIZE_STATIC                  (IDC_DEFAULT+ 11)
        #define    IDC_DEFAULT_EDIT                                      (IDC_DEFAULT+ 12)
        #define    IDC_DEFAULT_UNDERLINE_CHECK                    (IDC_DEFAULT + 13)
		
    #define    IDC_FOLDEROPEN   (IDD_FOLDER_STYLE_DLG + 200)
        #define    IDC_FOLDEROPEN_DESCGROUP_STATIC            (IDC_FOLDEROPEN + 1)
        #define    IDC_FOLDEROPEN_FG_STATIC                            (IDC_FOLDEROPEN + 2)
        #define    IDC_FOLDEROPEN_BG_STATIC                            (IDC_FOLDEROPEN + 3) 
        #define    IDC_FOLDEROPEN_FONT_COMBO                       (IDC_FOLDEROPEN + 4)
        #define    IDC_FOLDEROPEN_FONTSIZE_COMBO                (IDC_FOLDEROPEN + 5) 
        #define    IDC_FOLDEROPEN_BOLD_CHECK                        (IDC_FOLDEROPEN + 6) 
        #define    IDC_FOLDEROPEN_ITALIC_CHECK                       (IDC_FOLDEROPEN + 7)
        #define    IDC_FOLDEROPEN_FONTSTYLEGROUP_STATIC    (IDC_FOLDEROPEN + 8)
        #define    IDC_FOLDEROPEN_COLORSTYLEGROUP_STATIC  (IDC_FOLDEROPEN + 9) 
        #define    IDC_FOLDEROPEN_FONTNAME_STATIC                (IDC_FOLDEROPEN + 10)
        #define    IDC_FOLDEROPEN_FONTSIZE_STATIC                  (IDC_FOLDEROPEN + 11)
        #define    IDC_FOLDEROPEN_EDIT                                       (IDC_FOLDEROPEN + 12)
        #define    IDC_FOLDEROPEN_UNDERLINE_CHECK                       (IDC_FOLDEROPEN + 13)
		
    #define    IDC_FOLDERCLOSE   (IDD_FOLDER_STYLE_DLG + 300)
        #define    IDC_FOLDERCLOSE_DESCGROUP_STATIC             (IDC_FOLDERCLOSE + 1)
        #define    IDC_FOLDERCLOSE_FG_STATIC                            (IDC_FOLDERCLOSE + 2)
        #define    IDC_FOLDERCLOSE_BG_STATIC                            (IDC_FOLDERCLOSE + 3)
        #define    IDC_FOLDERCLOSE_FONT_COMBO                       (IDC_FOLDERCLOSE + 4)
        #define    IDC_FOLDERCLOSE_FONTSIZE_COMBO                (IDC_FOLDERCLOSE + 5)
        #define    IDC_FOLDERCLOSE_BOLD_CHECK                        (IDC_FOLDERCLOSE + 6)
        #define    IDC_FOLDERCLOSE_ITALIC_CHECK                       (IDC_FOLDERCLOSE + 7)
        #define    IDC_FOLDERCLOSE_FONTSTYLEGROUP_STATIC     (IDC_FOLDERCLOSE + 8) 
        #define    IDC_FOLDERCLOSE_COLORSTYLEGROUP_STATIC  (IDC_FOLDERCLOSE + 9)
        #define    IDC_FOLDERCLOSE_FONTNAME_STATIC                (IDC_FOLDERCLOSE + 10)
        #define    IDC_FOLDERCLOSE_FONTSIZE_STATIC                  (IDC_FOLDERCLOSE + 11) 
        #define    IDC_FOLDERCLOSE_EDIT                                       (IDC_FOLDERCLOSE + 12) 
        #define    IDC_FOLDERCLOSE_UNDERLINE_CHECK                     (IDC_FOLDERCLOSE + 13)
		
#define    IDD_KEYWORD_STYLE_DLG   22000 //(IDD_GLOBAL_USERDEFINE_DLG + 2000)
 
    #define    IDC_KEYWORD1   (IDD_KEYWORD_STYLE_DLG + 100)
        #define    IDC_KEYWORD1_DESCGROUP_STATIC            (IDC_KEYWORD1 + 1)
        #define    IDC_KEYWORD1_FG_STATIC                            (IDC_KEYWORD1 + 2)
        #define    IDC_KEYWORD1_BG_STATIC                            (IDC_KEYWORD1 + 3) 
        #define    IDC_KEYWORD1_FONT_COMBO                       (IDC_KEYWORD1 + 4)
        #define    IDC_KEYWORD1_FONTSIZE_COMBO                (IDC_KEYWORD1 + 5) 
        #define    IDC_KEYWORD1_BOLD_CHECK                        (IDC_KEYWORD1 + 6) 
        #define    IDC_KEYWORD1_ITALIC_CHECK                       (IDC_KEYWORD1 + 7)
        #define    IDC_KEYWORD1_FONTSTYLEGROUP_STATIC    (IDC_KEYWORD1 + 8)
        #define    IDC_KEYWORD1_COLORSTYLEGROUP_STATIC  (IDC_KEYWORD1 + 9) 
        #define    IDC_KEYWORD1_FONTNAME_STATIC                (IDC_KEYWORD1 + 10)
        #define    IDC_KEYWORD1_FONTSIZE_STATIC                  (IDC_KEYWORD1 + 11)
        #define    IDC_KEYWORD1_EDIT                                       (IDC_KEYWORD1 + 12)            
        #define    IDC_KEYWORD1_PREFIX_CHECK					 (IDC_KEYWORD1 + 13) 
		#define    IDC_KEYWORD1_UNDERLINE_CHECK                 (IDC_KEYWORD1 + 14)

    #define    IDC_KEYWORD2   (IDD_KEYWORD_STYLE_DLG + 200)
        #define    IDC_KEYWORD2_DESCGROUP_STATIC            (IDC_KEYWORD2 + 1)
        #define    IDC_KEYWORD2_FG_STATIC                            (IDC_KEYWORD2 + 2)
        #define    IDC_KEYWORD2_BG_STATIC                            (IDC_KEYWORD2 + 3) 
        #define    IDC_KEYWORD2_FONT_COMBO                       (IDC_KEYWORD2 + 4)
        #define    IDC_KEYWORD2_FONTSIZE_COMBO                (IDC_KEYWORD2 + 5) 
        #define    IDC_KEYWORD2_BOLD_CHECK                        (IDC_KEYWORD2 + 6) 
        #define    IDC_KEYWORD2_ITALIC_CHECK                       (IDC_KEYWORD2 + 7)
        #define    IDC_KEYWORD2_FONTSTYLEGROUP_STATIC    (IDC_KEYWORD2 + 8)
        #define    IDC_KEYWORD2_COLORSTYLEGROUP_STATIC  (IDC_KEYWORD2 + 9) 
        #define    IDC_KEYWORD2_FONTNAME_STATIC                (IDC_KEYWORD2 + 10)
        #define    IDC_KEYWORD2_FONTSIZE_STATIC                  (IDC_KEYWORD2 + 11)
        #define    IDC_KEYWORD2_EDIT                                       (IDC_KEYWORD2 + 12)
        #define    IDC_KEYWORD2_PREFIX_CHECK	   					(IDC_KEYWORD2 + 13)
		#define    IDC_KEYWORD2_UNDERLINE_CHECK                 (IDC_KEYWORD2 + 14)
		
    #define    IDC_KEYWORD3   (IDD_KEYWORD_STYLE_DLG + 300)
        #define    IDC_KEYWORD3_DESCGROUP_STATIC            (IDC_KEYWORD3 + 1)
        #define    IDC_KEYWORD3_FG_STATIC                            (IDC_KEYWORD3 + 2)
        #define    IDC_KEYWORD3_BG_STATIC                            (IDC_KEYWORD3 + 3) 
        #define    IDC_KEYWORD3_FONT_COMBO                       (IDC_KEYWORD3 + 4)
        #define    IDC_KEYWORD3_FONTSIZE_COMBO                (IDC_KEYWORD3 + 5) 
        #define    IDC_KEYWORD3_BOLD_CHECK                        (IDC_KEYWORD3 + 6) 
        #define    IDC_KEYWORD3_ITALIC_CHECK                       (IDC_KEYWORD3 + 7)
        #define    IDC_KEYWORD3_FONTSTYLEGROUP_STATIC    (IDC_KEYWORD3 + 8)
        #define    IDC_KEYWORD3_COLORSTYLEGROUP_STATIC  (IDC_KEYWORD3 + 9) 
        #define    IDC_KEYWORD3_FONTNAME_STATIC                (IDC_KEYWORD3 + 10)
        #define    IDC_KEYWORD3_FONTSIZE_STATIC                  (IDC_KEYWORD3 + 11)
        #define    IDC_KEYWORD3_EDIT                                       (IDC_KEYWORD3 + 12)
        #define    IDC_KEYWORD3_PREFIX_CHECK					     (IDC_KEYWORD3 + 13)
		#define    IDC_KEYWORD3_UNDERLINE_CHECK                 (IDC_KEYWORD3 + 14)
		
    #define    IDC_KEYWORD4   (IDD_KEYWORD_STYLE_DLG + 400)
        #define    IDC_KEYWORD4_DESCGROUP_STATIC            (IDC_KEYWORD4 + 1)
        #define    IDC_KEYWORD4_FG_STATIC                            (IDC_KEYWORD4 + 2)
        #define    IDC_KEYWORD4_BG_STATIC                            (IDC_KEYWORD4 + 3) 
        #define    IDC_KEYWORD4_FONT_COMBO                       (IDC_KEYWORD4 + 4)
        #define    IDC_KEYWORD4_FONTSIZE_COMBO                (IDC_KEYWORD4 + 5) 
        #define    IDC_KEYWORD4_BOLD_CHECK                        (IDC_KEYWORD4 + 6) 
        #define    IDC_KEYWORD4_ITALIC_CHECK                       (IDC_KEYWORD4 + 7)
        #define    IDC_KEYWORD4_FONTSTYLEGROUP_STATIC    (IDC_KEYWORD4 + 8)
        #define    IDC_KEYWORD4_COLORSTYLEGROUP_STATIC  (IDC_KEYWORD4 + 9) 
        #define    IDC_KEYWORD4_FONTNAME_STATIC                (IDC_KEYWORD4 + 10)
        #define    IDC_KEYWORD4_FONTSIZE_STATIC                  (IDC_KEYWORD4 + 11)
        #define    IDC_KEYWORD4_EDIT                                       (IDC_KEYWORD4 + 12)
		#define    IDC_KEYWORD4_PREFIX_CHECK							 (IDC_KEYWORD4 + 13)
		#define    IDC_KEYWORD4_UNDERLINE_CHECK                 (IDC_KEYWORD4 + 14)
		
	#define    IDC_KEYWORD_SCROLLBAR  (IDD_KEYWORD_STYLE_DLG + 500)
        
#define    IDD_COMMENT_STYLE_DLG 23000 //(IDD_GLOBAL_USERDEFINE_DLG + 3000)

    #define    IDC_COMMENT   (IDD_COMMENT_STYLE_DLG + 100)
        #define    IDC_COMMENT_DESCGROUP_STATIC            (IDC_COMMENT + 1)
        #define    IDC_COMMENT_FG_STATIC                            (IDC_COMMENT + 2)
        #define    IDC_COMMENT_BG_STATIC                            (IDC_COMMENT+ 3) 
        #define    IDC_COMMENT_FONT_COMBO                       (IDC_COMMENT + 4)
        #define    IDC_COMMENT_FONTSIZE_COMBO                (IDC_COMMENT+ 5) 
        #define    IDC_COMMENT_BOLD_CHECK                        (IDC_COMMENT+ 6) 
        #define    IDC_COMMENT_ITALIC_CHECK                       (IDC_COMMENT+ 7)
        #define    IDC_COMMENT_FONTSTYLEGROUP_STATIC    (IDC_COMMENT+ 8)
        #define    IDC_COMMENT_COLORSTYLEGROUP_STATIC  (IDC_COMMENT+ 9) 
        #define    IDC_COMMENT_FONTNAME_STATIC                (IDC_COMMENT+ 10)
        #define    IDC_COMMENT_FONTSIZE_STATIC                  (IDC_COMMENT+ 11)
        #define    IDC_COMMENTOPEN_EDIT                              (IDC_COMMENT+ 12)            
        #define    IDC_COMMENTOPEN_STATIC                          (IDC_COMMENT+ 13)
        #define    IDC_COMMENTCLOSE_EDIT                            (IDC_COMMENT + 14)
        #define    IDC_COMMENTCLOSE_STATIC                        (IDC_COMMENT + 15)
        #define    IDC_COMMENTLINESYMBOL_CHECK                 (IDC_COMMENT + 16)
        #define    IDC_COMMENTSYMBOL_CHECK                        (IDC_COMMENT + 17)
		#define    IDC_COMMENT_UNDERLINE_CHECK                (IDC_NUMBER + 18)
       
    #define    IDC_NUMBER   (IDD_COMMENT_STYLE_DLG + 200)
        #define    IDC_NUMBER_DESCGROUP_STATIC            (IDC_NUMBER+ 1)
        #define    IDC_NUMBER_FG_STATIC                            (IDC_NUMBER+ 2)
        #define    IDC_NUMBER_BG_STATIC                            (IDC_NUMBER + 3) 
        #define    IDC_NUMBER_FONT_COMBO                       (IDC_NUMBER+ 4)
        #define    IDC_NUMBER_FONTSIZE_COMBO                (IDC_NUMBER + 5) 
        #define    IDC_NUMBER_BOLD_CHECK                        (IDC_NUMBER + 6) 
        #define    IDC_NUMBER_ITALIC_CHECK                       (IDC_NUMBER + 7)
        #define    IDC_NUMBER_FONTSTYLEGROUP_STATIC    (IDC_NUMBER + 8)
        #define    IDC_NUMBER_COLORSTYLEGROUP_STATIC  (IDC_NUMBER  + 9) 
        #define    IDC_NUMBER_FONTNAME_STATIC                (IDC_NUMBER + 10)
        #define    IDC_NUMBER_FONTSIZE_STATIC                  (IDC_NUMBER + 11)
        #define    IDC_NUMBER_UNDERLINE_CHECK                (IDC_NUMBER + 12)
		
    #define    IDC_COMMENTLINE   (IDD_COMMENT_STYLE_DLG + 300)
        #define    IDC_COMMENTLINE_DESCGROUP_STATIC            (IDC_COMMENTLINE + 1)
        #define    IDC_COMMENTLINE_FG_STATIC                            (IDC_COMMENTLINE + 2)
        #define    IDC_COMMENTLINE_BG_STATIC                            (IDC_COMMENTLINE + 3) 
        #define    IDC_COMMENTLINE_FONT_COMBO                       (IDC_COMMENTLINE + 4)
        #define    IDC_COMMENTLINE_FONTSIZE_COMBO                (IDC_COMMENTLINE + 5) 
        #define    IDC_COMMENTLINE_BOLD_CHECK                        (IDC_COMMENTLINE + 6) 
        #define    IDC_COMMENTLINE_ITALIC_CHECK                       (IDC_COMMENTLINE + 7)
        #define    IDC_COMMENTLINE_FONTSTYLEGROUP_STATIC    (IDC_COMMENTLINE + 8)
        #define    IDC_COMMENTLINE_COLORSTYLEGROUP_STATIC  (IDC_COMMENTLINE + 9) 
        #define    IDC_COMMENTLINE_FONTNAME_STATIC                (IDC_COMMENTLINE + 10)
        #define    IDC_COMMENTLINE_FONTSIZE_STATIC                  (IDC_COMMENTLINE + 11)
        #define    IDC_COMMENTLINE_EDIT                                       (IDC_COMMENTLINE + 12)
		#define    IDC_COMMENTLINE_UNDERLINE_CHECK                (IDC_COMMENTLINE + 13)
        
#define    IDD_SYMBOL_STYLE_DLG   24000   //IDD_GLOBAL_USERDEFINE_DLG + 4000
    #define		IDC_HAS_ESCAPE          (IDD_SYMBOL_STYLE_DLG + 1)
    #define		IDC_ESCAPE_CHAR          (IDD_SYMBOL_STYLE_DLG + 2)

    #define    IDC_SYMBOL   (IDD_SYMBOL_STYLE_DLG + 100)
        #define    IDC_ACTIVATED_SYMBOL_STATIC                        (IDC_SYMBOL + 1)
        #define    IDC_ACTIVATED_SYMBOL_LIST                            (IDC_SYMBOL + 2)
        #define    IDC_AVAILABLE_SYMBOLS_STATIC                      (IDC_SYMBOL + 3)
        #define    IDC_AVAILABLE_SYMBOLS_LIST                          (IDC_SYMBOL + 4)
        #define    IDC_ADD_BUTTON                                               (IDC_SYMBOL + 5)
        #define    IDC_REMOVE_BUTTON                                        (IDC_SYMBOL + 6)
        #define    IDC_SYMBOL_DESCGROUP_STATIC                      (IDC_SYMBOL+ 7)
        #define    IDC_SYMBOL_FG_STATIC                                     (IDC_SYMBOL  + 8)
        #define    IDC_SYMBOL_BG_STATIC                                     (IDC_SYMBOL + 9) 
        #define    IDC_SYMBOL_FONT_COMBO                             (IDC_SYMBOL  + 10)
        #define    IDC_SYMBOL_FONTSIZE_COMBO                         (IDC_SYMBOL + 11) 
        #define    IDC_SYMBOL_BOLD_CHECK                                (IDC_SYMBOL+ 12) 
        #define    IDC_SYMBOL_ITALIC_CHECK                               (IDC_SYMBOL  + 13)
        #define    IDC_SYMBOL_FONTSTYLEGROUP_STATIC             (IDC_SYMBOL + 14)
        #define    IDC_SYMBOL_COLORSTYLEGROUP_STATIC            ( IDC_SYMBOL   + 15) 
        #define    IDC_SYMBOL_FONTNAME_STATIC                        (IDC_SYMBOL  + 16)
        #define    IDC_SYMBOL_FONTSIZE_STATIC                           (IDC_SYMBOL + 17)
        #define    IDC_SYMBOL_UNDERLINE_CHECK                        (IDC_SYMBOL + 18)
		
	#define    IDC_SYMBOL2   (IDD_SYMBOL_STYLE_DLG + 200)
		#define IDC_SYMBOL_DELIMGROUP1_STATIC 		(IDC_SYMBOL2 + 1)
		#define IDC_SYMBOL_COLORSTYLEGROUP2_STATIC  (IDC_SYMBOL2 + 2)
		#define IDC_SYMBOL_FONTSTYLEGROUP2_STATIC   (IDC_SYMBOL2 + 3)
		#define IDC_SYMBOL_FG2_STATIC               (IDC_SYMBOL2 + 4)
		#define IDC_SYMBOL_BG2_STATIC               (IDC_SYMBOL2 + 5)
		#define IDC_SYMBOL_FONTNAME2_STATIC         (IDC_SYMBOL2 + 6)
		#define IDC_SYMBOL_BOLD2_CHECK              (IDC_SYMBOL2 + 7)
		#define IDC_SYMBOL_ITALIC2_CHECK            (IDC_SYMBOL2 + 8)
		#define IDC_SYMBOL_FONT2_COMBO              (IDC_SYMBOL2 + 9)
		#define IDC_SYMBOL_UNDERLINE2_CHECK         (IDC_SYMBOL2 + 10)
		#define IDC_SYMBOL_BO2_STATIC               (IDC_SYMBOL2 + 11)
		#define IDC_SYMBOL_BO2_COMBO                (IDC_SYMBOL2 + 12)
		#define IDC_SYMBOL_BC2_COMBO                (IDC_SYMBOL2 + 13)
		#define IDC_SYMBOL_BC2_STATIC               (IDC_SYMBOL2 + 14)
		#define IDC_SYMBOL_FONTSIZE2_COMBO          (IDC_SYMBOL2 + 15)
		#define IDC_SYMBOL_FONTSIZE2_STATIC         (IDC_SYMBOL2 + 16)

	#define    IDC_SYMBOL3   (IDD_SYMBOL_STYLE_DLG + 300)
		#define IDC_SYMBOL_DELIMGROUP2_STATIC   	(IDC_SYMBOL3 + 1)
		#define IDC_SYMBOL_FG3_STATIC         		(IDC_SYMBOL3 + 2)  
		#define IDC_SYMBOL_BG3_STATIC           	(IDC_SYMBOL3 + 3)
		#define IDC_SYMBOL_FONT3_COMBO          	(IDC_SYMBOL3 + 4)
		#define IDC_SYMBOL_BO3_COMBO            	(IDC_SYMBOL3 + 5)
		#define IDC_SYMBOL_BOLD3_CHECK          	(IDC_SYMBOL3 + 6)
		#define IDC_SYMBOL_ITALIC3_CHECK        	(IDC_SYMBOL3 + 7)
		#define IDC_SYMBOL_FONTSTYLEGROUP3_STATIC 	(IDC_SYMBOL3 + 8)
		#define IDC_SYMBOL_COLORSTYLEGROUP3_STATIC 	(IDC_SYMBOL3 + 9)
		#define IDC_SYMBOL_FONTNAME3_STATIC     	(IDC_SYMBOL3 + 10)
		#define IDC_SYMBOL_BO3_STATIC           	(IDC_SYMBOL3 + 11)
		#define IDC_SYMBOL_UNDERLINE3_CHECK     	(IDC_SYMBOL3 + 12)
		#define IDC_SYMBOL_BC3_COMBO            	(IDC_SYMBOL3 + 13)
		#define IDC_SYMBOL_BC3_STATIC           	(IDC_SYMBOL3 + 14)
		#define IDC_SYMBOL_FONTSIZE3_COMBO      	(IDC_SYMBOL3 + 15)
		#define IDC_SYMBOL_FONTSIZE3_STATIC     	(IDC_SYMBOL3 + 16)
	                                                         
#define    IDD_STRING_DLG   25000
    #define    IDC_STRING_STATIC  (IDD_STRING_DLG + 1)
    #define    IDC_STRING_EDIT  (IDD_STRING_DLG + 2)
#endif //USERDEFIN_RC_H
                          
