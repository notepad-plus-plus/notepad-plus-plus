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


#ifndef NOTEPAD_PLUS_MSGS_H
#define NOTEPAD_PLUS_MSGS_H

#include <windows.h>
#include <tchar.h>

enum LangType {L_TEXT, L_PHP , L_C, L_CPP, L_CS, L_OBJC, L_JAVA, L_RC,\
			   L_HTML, L_XML, L_MAKEFILE, L_PASCAL, L_BATCH, L_INI, L_ASCII, L_USER,\
			   L_ASP, L_SQL, L_VB, L_JS, L_CSS, L_PERL, L_PYTHON, L_LUA,\
			   L_TEX, L_FORTRAN, L_BASH, L_FLASH, L_NSIS, L_TCL, L_LISP, L_SCHEME,\
			   L_ASM, L_DIFF, L_PROPS, L_PS, L_RUBY, L_SMALLTALK, L_VHDL, L_KIX, L_AU3,\
			   L_CAML, L_ADA, L_VERILOG, L_MATLAB, L_HASKELL, L_INNO, L_SEARCHRESULT,\
			   L_CMAKE, L_YAML, L_COBOL, L_GUI4CLI, L_D, L_POWERSHELL, L_R, L_JSP,\
			   L_COFFEESCRIPT,\
			   // The end of enumated language type, so it should be always at the end
			   L_EXTERNAL};
enum winVer{WV_UNKNOWN, WV_WIN32S, WV_95, WV_98, WV_ME, WV_NT, WV_W2K, WV_XP, WV_S2003, WV_XPX64, WV_VISTA, WV_WIN7, WV_WIN8, WV_WIN81};



//Here you can find how to use these messages : http://docs.notepad-plus-plus.org/index.php/Messages_And_Notifications 
#define NPPMSG  (WM_USER + 1000)

	#define NPPM_GETCURRENTSCINTILLA  (NPPMSG + 4)
	#define NPPM_GETCURRENTLANGTYPE  (NPPMSG + 5)
	#define NPPM_SETCURRENTLANGTYPE  (NPPMSG + 6)

	#define NPPM_GETNBOPENFILES			(NPPMSG + 7)
		#define ALL_OPEN_FILES			0
		#define PRIMARY_VIEW			1
		#define SECOND_VIEW				2

	#define NPPM_GETOPENFILENAMES		(NPPMSG + 8)


	#define NPPM_MODELESSDIALOG		 (NPPMSG + 12)
		#define MODELESSDIALOGADD		0
		#define MODELESSDIALOGREMOVE	1

	#define NPPM_GETNBSESSIONFILES (NPPMSG + 13)
	#define NPPM_GETSESSIONFILES (NPPMSG + 14)
	#define NPPM_SAVESESSION (NPPMSG + 15)
	#define NPPM_SAVECURRENTSESSION (NPPMSG + 16)

		struct sessionInfo {
			TCHAR* sessionFilePathName;
			int nbFile;
			TCHAR** files;
		};

	#define NPPM_GETOPENFILENAMESPRIMARY (NPPMSG + 17)
	#define NPPM_GETOPENFILENAMESSECOND (NPPMSG + 18)
	
	#define NPPM_CREATESCINTILLAHANDLE (NPPMSG + 20)
	#define NPPM_DESTROYSCINTILLAHANDLE (NPPMSG + 21)
	#define NPPM_GETNBUSERLANG (NPPMSG + 22)

	#define NPPM_GETCURRENTDOCINDEX (NPPMSG + 23)
		#define MAIN_VIEW 0
		#define SUB_VIEW 1

	#define NPPM_SETSTATUSBAR (NPPMSG + 24)
		#define STATUSBAR_DOC_TYPE 0
		#define STATUSBAR_DOC_SIZE 1
		#define STATUSBAR_CUR_POS 2
		#define STATUSBAR_EOF_FORMAT 3
		#define STATUSBAR_UNICODE_TYPE 4
		#define STATUSBAR_TYPING_MODE 5

	#define NPPM_GETMENUHANDLE (NPPMSG + 25)
		#define NPPPLUGINMENU 0
		#define NPPMAINMENU 1
	// INT NPPM_GETMENUHANDLE(INT menuChoice, 0)
	// Return: menu handle (HMENU) of choice (plugin menu handle or Notepad++ main menu handle)

	#define NPPM_ENCODESCI (NPPMSG + 26)
	//ascii file to unicode
	//int NPPM_ENCODESCI(MAIN_VIEW/SUB_VIEW, 0)
	//return new unicodeMode
	
	#define NPPM_DECODESCI (NPPMSG + 27)
	//unicode file to ascii
	//int NPPM_DECODESCI(MAIN_VIEW/SUB_VIEW, 0)
	//return old unicodeMode

	#define NPPM_ACTIVATEDOC (NPPMSG + 28)
	//void NPPM_ACTIVATEDOC(int view, int index2Activate)

	#define NPPM_LAUNCHFINDINFILESDLG (NPPMSG + 29)
	//void NPPM_LAUNCHFINDINFILESDLG(TCHAR * dir2Search, TCHAR * filtre)

	#define NPPM_DMMSHOW (NPPMSG + 30)
	//void NPPM_DMMSHOW(0, tTbData->hClient)

	#define NPPM_DMMHIDE	(NPPMSG + 31)
	//void NPPM_DMMHIDE(0, tTbData->hClient)

	#define NPPM_DMMUPDATEDISPINFO (NPPMSG + 32)
	//void NPPM_DMMUPDATEDISPINFO(0, tTbData->hClient)

	#define NPPM_DMMREGASDCKDLG (NPPMSG + 33)
	//void NPPM_DMMREGASDCKDLG(0, &tTbData)

	#define NPPM_LOADSESSION (NPPMSG + 34)
	//void NPPM_LOADSESSION(0, const TCHAR* file name)

	#define NPPM_DMMVIEWOTHERTAB (NPPMSG + 35)
	//void WM_DMM_VIEWOTHERTAB(0, tTbData->pszName)

	#define NPPM_RELOADFILE (NPPMSG + 36)
	//BOOL NPPM_RELOADFILE(BOOL withAlert, TCHAR *filePathName2Reload)

	#define NPPM_SWITCHTOFILE (NPPMSG + 37)
	//BOOL NPPM_SWITCHTOFILE(0, TCHAR *filePathName2switch)

	#define NPPM_SAVECURRENTFILE (NPPMSG + 38)
	//BOOL NPPM_SAVECURRENTFILE(0, 0)

	#define NPPM_SAVEALLFILES	(NPPMSG + 39)
	//BOOL NPPM_SAVEALLFILES(0, 0)

	#define NPPM_SETMENUITEMCHECK	(NPPMSG + 40)
	//void WM_PIMENU_CHECK(UINT	funcItem[X]._cmdID, TRUE/FALSE)

	#define NPPM_ADDTOOLBARICON (NPPMSG + 41)
	//void WM_ADDTOOLBARICON(UINT funcItem[X]._cmdID, toolbarIcons icon)
		struct toolbarIcons {
			HBITMAP	hToolbarBmp;
			HICON	hToolbarIcon;
		};

	#define NPPM_GETWINDOWSVERSION (NPPMSG + 42)
	//winVer NPPM_GETWINDOWSVERSION(0, 0)

	#define NPPM_DMMGETPLUGINHWNDBYNAME (NPPMSG + 43)
	//HWND WM_DMM_GETPLUGINHWNDBYNAME(const TCHAR *windowName, const TCHAR *moduleName)
	// if moduleName is NULL, then return value is NULL
	// if windowName is NULL, then the first found window handle which matches with the moduleName will be returned
	
	#define NPPM_MAKECURRENTBUFFERDIRTY (NPPMSG + 44)
	//BOOL NPPM_MAKECURRENTBUFFERDIRTY(0, 0)

	#define NPPM_GETENABLETHEMETEXTUREFUNC (NPPMSG + 45)
	//BOOL NPPM_GETENABLETHEMETEXTUREFUNC(0, 0)

	#define NPPM_GETPLUGINSCONFIGDIR (NPPMSG + 46)
	//void NPPM_GETPLUGINSCONFIGDIR(int strLen, TCHAR *str)

	#define NPPM_MSGTOPLUGIN (NPPMSG + 47)
	//BOOL NPPM_MSGTOPLUGIN(TCHAR *destModuleName, CommunicationInfo *info)
	// return value is TRUE when the message arrive to the destination plugins.
	// if destModule or info is NULL, then return value is FALSE
		struct CommunicationInfo {
			long internalMsg;
			const TCHAR * srcModuleName;
			void * info; // defined by plugin
		};

	#define NPPM_MENUCOMMAND (NPPMSG + 48)
	//void NPPM_MENUCOMMAND(0, int cmdID)
	// uncomment //#include "menuCmdID.h"
	// in the beginning of this file then use the command symbols defined in "menuCmdID.h" file
	// to access all the Notepad++ menu command items
	
	#define NPPM_TRIGGERTABBARCONTEXTMENU (NPPMSG + 49)
	//void NPPM_TRIGGERTABBARCONTEXTMENU(int view, int index2Activate)

	#define NPPM_GETNPPVERSION (NPPMSG + 50)
	// int NPPM_GETNPPVERSION(0, 0)
	// return version 
	// ex : v4.6
	// HIWORD(version) == 4
	// LOWORD(version) == 6

	#define NPPM_HIDETABBAR (NPPMSG + 51)
	// BOOL NPPM_HIDETABBAR(0, BOOL hideOrNot)
	// if hideOrNot is set as TRUE then tab bar will be hidden
	// otherwise it'll be shown.
	// return value : the old status value

	#define NPPM_ISTABBARHIDDEN (NPPMSG + 52)
	// BOOL NPPM_ISTABBARHIDDEN(0, 0)
	// returned value : TRUE if tab bar is hidden, otherwise FALSE

	#define NPPM_GETPOSFROMBUFFERID (NPPMSG + 57)
	// INT NPPM_GETPOSFROMBUFFERID(INT bufferID, INT priorityView)
	// Return VIEW|INDEX from a buffer ID. -1 if the bufferID non existing
	// if priorityView set to SUB_VIEW, then SUB_VIEW will be search firstly
	//
	// VIEW takes 2 highest bits and INDEX (0 based) takes the rest (30 bits) 
	// Here's the values for the view :
	//  MAIN_VIEW 0
	//  SUB_VIEW  1

	#define NPPM_GETFULLPATHFROMBUFFERID (NPPMSG + 58)
	// INT NPPM_GETFULLPATHFROMBUFFERID(INT bufferID, TCHAR *fullFilePath)
	// Get full path file name from a bufferID. 
	// Return -1 if the bufferID non existing, otherwise the number of TCHAR copied/to copy
	// User should call it with fullFilePath be NULL to get the number of TCHAR (not including the nul character),
	// allocate fullFilePath with the return values + 1, then call it again to get  full path file name

	#define NPPM_GETBUFFERIDFROMPOS (NPPMSG + 59)
	// INT NPPM_GETBUFFERIDFROMPOS(INT index, INT iView)
	// wParam: Position of document
	// lParam: View to use, 0 = Main, 1 = Secondary
	// Returns 0 if invalid

	#define NPPM_GETCURRENTBUFFERID (NPPMSG + 60)
	// INT NPPM_GETCURRENTBUFFERID(0, 0)
	// Returns active Buffer

	#define NPPM_RELOADBUFFERID (NPPMSG + 61)
	// VOID NPPM_RELOADBUFFERID(0, 0)
	// Reloads Buffer
	// wParam: Buffer to reload
	// lParam: 0 if no alert, else alert


	#define NPPM_GETBUFFERLANGTYPE (NPPMSG + 64)
	// INT NPPM_GETBUFFERLANGTYPE(INT bufferID, 0)
	// wParam: BufferID to get LangType from
	// lParam: 0
	// Returns as int, see LangType. -1 on error

	#define NPPM_SETBUFFERLANGTYPE (NPPMSG + 65)
	// BOOL NPPM_SETBUFFERLANGTYPE(INT bufferID, INT langType)
	// wParam: BufferID to set LangType of
	// lParam: LangType
	// Returns TRUE on success, FALSE otherwise
	// use int, see LangType for possible values
	// L_USER and L_EXTERNAL are not supported

	#define NPPM_GETBUFFERENCODING (NPPMSG + 66)
	// INT NPPM_GETBUFFERENCODING(INT bufferID, 0)
	// wParam: BufferID to get encoding from
	// lParam: 0
	// returns as int, see UniMode. -1 on error

	#define NPPM_SETBUFFERENCODING (NPPMSG + 67)
	// BOOL NPPM_SETBUFFERENCODING(INT bufferID, INT encoding)
	// wParam: BufferID to set encoding of
	// lParam: encoding
	// Returns TRUE on success, FALSE otherwise
	// use int, see UniMode
	// Can only be done on new, unedited files

	#define NPPM_GETBUFFERFORMAT (NPPMSG + 68)
	// INT NPPM_GETBUFFERFORMAT(INT bufferID, 0)
	// wParam: BufferID to get format from
	// lParam: 0
	// returns as int, see formatType. -1 on error

	#define NPPM_SETBUFFERFORMAT (NPPMSG + 69)
	// BOOL NPPM_SETBUFFERFORMAT(INT bufferID, INT format)
	// wParam: BufferID to set format of
	// lParam: format
	// Returns TRUE on success, FALSE otherwise
	// use int, see formatType

/*
	#define NPPM_ADDREBAR (NPPMSG + 57)
	// BOOL NPPM_ADDREBAR(0, REBARBANDINFO *)
	// Returns assigned ID in wID value of struct pointer
	#define NPPM_UPDATEREBAR (NPPMSG + 58)
	// BOOL NPPM_ADDREBAR(INT ID, REBARBANDINFO *)
	//Use ID assigned with NPPM_ADDREBAR
	#define NPPM_REMOVEREBAR (NPPMSG + 59)
	// BOOL NPPM_ADDREBAR(INT ID, 0)
	//Use ID assigned with NPPM_ADDREBAR
*/

	#define NPPM_HIDETOOLBAR (NPPMSG + 70)
	// BOOL NPPM_HIDETOOLBAR(0, BOOL hideOrNot)
	// if hideOrNot is set as TRUE then tool bar will be hidden
	// otherwise it'll be shown.
	// return value : the old status value

	#define NPPM_ISTOOLBARHIDDEN (NPPMSG + 71)
	// BOOL NPPM_ISTOOLBARHIDDEN(0, 0)
	// returned value : TRUE if tool bar is hidden, otherwise FALSE

	#define NPPM_HIDEMENU (NPPMSG + 72)
	// BOOL NPPM_HIDEMENU(0, BOOL hideOrNot)
	// if hideOrNot is set as TRUE then menu will be hidden
	// otherwise it'll be shown.
	// return value : the old status value

	#define NPPM_ISMENUHIDDEN (NPPMSG + 73)
	// BOOL NPPM_ISMENUHIDDEN(0, 0)
	// returned value : TRUE if menu is hidden, otherwise FALSE

	#define NPPM_HIDESTATUSBAR (NPPMSG + 74)
	// BOOL NPPM_HIDESTATUSBAR(0, BOOL hideOrNot)
	// if hideOrNot is set as TRUE then STATUSBAR will be hidden
	// otherwise it'll be shown.
	// return value : the old status value

	#define NPPM_ISSTATUSBARHIDDEN (NPPMSG + 75)
	// BOOL NPPM_ISSTATUSBARHIDDEN(0, 0)
	// returned value : TRUE if STATUSBAR is hidden, otherwise FALSE

	#define NPPM_GETSHORTCUTBYCMDID (NPPMSG + 76)
	// BOOL NPPM_GETSHORTCUTBYCMDID(int cmdID, ShortcutKey *sk)
	// get your plugin command current mapped shortcut into sk via cmdID
	// You may need it after getting NPPN_READY notification
	// returned value : TRUE if this function call is successful and shorcut is enable, otherwise FALSE

	#define NPPM_DOOPEN (NPPMSG + 77)
	// BOOL NPPM_DOOPEN(0, const TCHAR *fullPathName2Open)
	// fullPathName2Open indicates the full file path name to be opened.
	// The return value is TRUE (1) if the operation is successful, otherwise FALSE (0).

	#define NPPM_SAVECURRENTFILEAS (NPPMSG + 78)
	// BOOL NPPM_SAVECURRENTFILEAS (BOOL asCopy, const TCHAR* filename)

    #define NPPM_GETCURRENTNATIVELANGENCODING (NPPMSG + 79)
	// INT NPPM_GETCURRENTNATIVELANGENCODING(0, 0)
	// returned value : the current native language enconding

    #define NPPM_ALLOCATESUPPORTED   (NPPMSG + 80)
    // returns TRUE if NPPM_ALLOCATECMDID is supported
    // Use to identify if subclassing is necessary

	#define NPPM_ALLOCATECMDID   (NPPMSG + 81)
    // BOOL NPPM_ALLOCATECMDID(int numberRequested, int* startNumber)
    // sets startNumber to the initial command ID if successful
    // Returns: TRUE if successful, FALSE otherwise. startNumber will also be set to 0 if unsuccessful

	#define NPPM_ALLOCATEMARKER  (NPPMSG + 82)
    // BOOL NPPM_ALLOCATEMARKER(int numberRequested, int* startNumber)
    // sets startNumber to the initial command ID if successful
    // Allocates a marker number to a plugin
    // Returns: TRUE if successful, FALSE otherwise. startNumber will also be set to 0 if unsuccessful

	#define NPPM_GETLANGUAGENAME  (NPPMSG + 83)
	// INT NPPM_GETLANGUAGENAME(int langType, TCHAR *langName)
	// Get programing language name from the given language type (LangType)
	// Return value is the number of copied character / number of character to copy (\0 is not included)
	// You should call this function 2 times - the first time you pass langName as NULL to get the number of characters to copy.
    // You allocate a buffer of the length of (the number of characters + 1) then call NPPM_GETLANGUAGENAME function the 2nd time 
	// by passing allocated buffer as argument langName

	#define NPPM_GETLANGUAGEDESC  (NPPMSG + 84)
	// INT NPPM_GETLANGUAGEDESC(int langType, TCHAR *langDesc)
	// Get programing language short description from the given language type (LangType)
	// Return value is the number of copied character / number of character to copy (\0 is not included)
	// You should call this function 2 times - the first time you pass langDesc as NULL to get the number of characters to copy.
    // You allocate a buffer of the length of (the number of characters + 1) then call NPPM_GETLANGUAGEDESC function the 2nd time 
	// by passing allocated buffer as argument langDesc

	#define NPPM_SHOWDOCSWITCHER    (NPPMSG + 85)
	// VOID NPPM_ISDOCSWITCHERSHOWN(0, BOOL toShowOrNot)
	// Send this message to show or hide doc switcher.
	// if toShowOrNot is TRUE then show doc switcher, otherwise hide it.

	#define NPPM_ISDOCSWITCHERSHOWN    (NPPMSG + 86)
	// BOOL NPPM_ISDOCSWITCHERSHOWN(0, 0)
	// Check to see if doc switcher is shown.

	#define NPPM_GETAPPDATAPLUGINSALLOWED    (NPPMSG + 87)
	// BOOL NPPM_GETAPPDATAPLUGINSALLOWED(0, 0)
	// Check to see if loading plugins from "%APPDATA%\Notepad++\plugins" is allowed.

	#define NPPM_GETCURRENTVIEW    (NPPMSG + 88)
	// INT NPPM_GETCURRENTVIEW(0, 0)
	// Return: current edit view of Notepad++. Only 2 possible values: 0 = Main, 1 = Secondary

	#define NPPM_DOCSWITCHERDISABLECOLUMN    (NPPMSG + 89)
	// VOID NPPM_DOCSWITCHERDISABLECOLUMN(0, BOOL disableOrNot)
	// Disable or enable extension column of doc switcher

	#define NPPM_GETEDITORDEFAULTFOREGROUNDCOLOR    (NPPMSG + 90)
	// INT NPPM_GETEDITORDEFAULTFOREGROUNDCOLOR(0, 0)
	// Return: current editor default foreground color. You should convert the returned value in COLORREF

	#define NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR    (NPPMSG + 91)
	// INT NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR(0, 0)
	// Return: current editor default background color. You should convert the returned value in COLORREF


#define	RUNCOMMAND_USER    (WM_USER + 3000)
	#define NPPM_GETFULLCURRENTPATH		(RUNCOMMAND_USER + FULL_CURRENT_PATH)
	#define NPPM_GETCURRENTDIRECTORY	(RUNCOMMAND_USER + CURRENT_DIRECTORY)
	#define NPPM_GETFILENAME			(RUNCOMMAND_USER + FILE_NAME)
	#define NPPM_GETNAMEPART			(RUNCOMMAND_USER + NAME_PART)
	#define NPPM_GETEXTPART				(RUNCOMMAND_USER + EXT_PART)
	#define NPPM_GETCURRENTWORD			(RUNCOMMAND_USER + CURRENT_WORD)
	#define NPPM_GETNPPDIRECTORY		(RUNCOMMAND_USER + NPP_DIRECTORY)
	// BOOL NPPM_GETXXXXXXXXXXXXXXXX(size_t strLen, TCHAR *str)
	// where str is the allocated TCHAR array,
	//	     strLen is the allocated array size
	// The return value is TRUE when get generic_string operation success
	// Otherwise (allocated array size is too small) FALSE

	#define NPPM_GETCURRENTLINE			(RUNCOMMAND_USER + CURRENT_LINE)
	// INT NPPM_GETCURRENTLINE(0, 0)
	// return the caret current position line
	#define NPPM_GETCURRENTCOLUMN			(RUNCOMMAND_USER + CURRENT_COLUMN)
	// INT NPPM_GETCURRENTCOLUMN(0, 0)
	// return the caret current position column

		#define VAR_NOT_RECOGNIZED 0
		#define FULL_CURRENT_PATH 1
		#define CURRENT_DIRECTORY 2
		#define FILE_NAME 3
		#define NAME_PART 4
		#define EXT_PART 5
		#define CURRENT_WORD 6
		#define NPP_DIRECTORY 7
		#define CURRENT_LINE 8
		#define CURRENT_COLUMN 9


// Notification code
#define NPPN_FIRST 1000
	#define NPPN_READY (NPPN_FIRST + 1) // To notify plugins that all the procedures of launchment of notepad++ are done.
	//scnNotification->nmhdr.code = NPPN_READY;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = 0;

	#define NPPN_TBMODIFICATION (NPPN_FIRST + 2) // To notify plugins that toolbar icons can be registered
	//scnNotification->nmhdr.code = NPPN_TB_MODIFICATION;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = 0;

	#define NPPN_FILEBEFORECLOSE (NPPN_FIRST + 3) // To notify plugins that the current file is about to be closed
	//scnNotification->nmhdr.code = NPPN_FILEBEFORECLOSE;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = BufferID;

	#define NPPN_FILEOPENED (NPPN_FIRST + 4) // To notify plugins that the current file is just opened
	//scnNotification->nmhdr.code = NPPN_FILEOPENED;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = BufferID;

	#define NPPN_FILECLOSED (NPPN_FIRST + 5) // To notify plugins that the current file is just closed
	//scnNotification->nmhdr.code = NPPN_FILECLOSED;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = BufferID;

	#define NPPN_FILEBEFOREOPEN (NPPN_FIRST + 6) // To notify plugins that the current file is about to be opened
	//scnNotification->nmhdr.code = NPPN_FILEBEFOREOPEN;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = BufferID;
	
	#define NPPN_FILEBEFORESAVE (NPPN_FIRST + 7) // To notify plugins that the current file is about to be saved
	//scnNotification->nmhdr.code = NPPN_FILEBEFOREOPEN;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = BufferID;
	
	#define NPPN_FILESAVED (NPPN_FIRST + 8) // To notify plugins that the current file is just saved
	//scnNotification->nmhdr.code = NPPN_FILESAVED;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = BufferID;

	#define NPPN_SHUTDOWN (NPPN_FIRST + 9) // To notify plugins that Notepad++ is about to be shutdowned.
	//scnNotification->nmhdr.code = NPPN_SHUTDOWN;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = 0;

	#define NPPN_BUFFERACTIVATED (NPPN_FIRST + 10) // To notify plugins that a buffer was activated (put to foreground).
	//scnNotification->nmhdr.code = NPPN_BUFFERACTIVATED;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = activatedBufferID;

	#define NPPN_LANGCHANGED (NPPN_FIRST + 11) // To notify plugins that the language in the current doc is just changed.
	//scnNotification->nmhdr.code = NPPN_LANGCHANGED;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = currentBufferID;

	#define NPPN_WORDSTYLESUPDATED (NPPN_FIRST + 12) // To notify plugins that user initiated a WordStyleDlg change.
	//scnNotification->nmhdr.code = NPPN_WORDSTYLESUPDATED;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = currentBufferID;

	#define NPPN_SHORTCUTREMAPPED (NPPN_FIRST + 13) // To notify plugins that plugin command shortcut is remapped.
	//scnNotification->nmhdr.code = NPPN_SHORTCUTSREMAPPED;
	//scnNotification->nmhdr.hwndFrom = ShortcutKeyStructurePointer;
	//scnNotification->nmhdr.idFrom = cmdID;
		//where ShortcutKeyStructurePointer is pointer of struct ShortcutKey:
		//struct ShortcutKey {
		//	bool _isCtrl;
		//	bool _isAlt;
		//	bool _isShift;
		//	UCHAR _key;
		//};

	#define NPPN_FILEBEFORELOAD (NPPN_FIRST + 14) // To notify plugins that the current file is about to be loaded
	//scnNotification->nmhdr.code = NPPN_FILEBEFOREOPEN;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = NULL;

	#define NPPN_FILELOADFAILED (NPPN_FIRST + 15)  // To notify plugins that file open operation failed
	//scnNotification->nmhdr.code = NPPN_FILEOPENFAILED;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = BufferID;

	#define NPPN_READONLYCHANGED (NPPN_FIRST + 16)  // To notify plugins that current document change the readonly status,
	//scnNotification->nmhdr.code = NPPN_READONLYCHANGED;
	//scnNotification->nmhdr.hwndFrom = bufferID;
	//scnNotification->nmhdr.idFrom = docStatus;
		// where bufferID is BufferID
		//       docStatus can be combined by DOCSTAUS_READONLY and DOCSTAUS_BUFFERDIRTY

		#define DOCSTAUS_READONLY 1
		#define DOCSTAUS_BUFFERDIRTY 2

	#define NPPN_DOCORDERCHANGED (NPPN_FIRST + 17)  // To notify plugins that document order is changed
	//scnNotification->nmhdr.code = NPPN_DOCORDERCHANGED;
	//scnNotification->nmhdr.hwndFrom = newIndex;
	//scnNotification->nmhdr.idFrom = BufferID;

	#define NPPN_SNAPSHOTDIRTYFILELOADED (NPPN_FIRST + 18)  // To notify plugins that a snapshot dirty file is loaded on startup
	//scnNotification->nmhdr.code = NPPN_SNAPSHOTDIRTYFILELOADED;
	//scnNotification->nmhdr.hwndFrom = NULL;
	//scnNotification->nmhdr.idFrom = BufferID;

	#define NPPN_BEFORESHUTDOWN (NPPN_FIRST + 19)  // To notify plugins that Npp shutdown has been triggered, files have not been closed yet
	//scnNotification->nmhdr.code = NPPN_BEFORESHUTDOWN;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = 0;

	#define NPPN_CANCELSHUTDOWN (NPPN_FIRST + 20)  // To notify plugins that Npp shutdown has been cancelled
	//scnNotification->nmhdr.code = NPPN_CANCELSHUTDOWN;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = 0;

	#define NPPN_FILEBEFORERENAME (NPPN_FIRST + 21)  // To notify plugins that file is to be renamed
	//scnNotification->nmhdr.code = NPPN_FILEBEFORERENAME;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = BufferID;

	#define NPPN_FILERENAMECANCEL (NPPN_FIRST + 22)  // To notify plugins that file rename has been cancelled
	//scnNotification->nmhdr.code = NPPN_FILERENAMECANCEL;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = BufferID;

	#define NPPN_FILERENAMED (NPPN_FIRST + 23)  // To notify plugins that file has been renamed
	//scnNotification->nmhdr.code = NPPN_FILERENAMED;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = BufferID;

	#define NPPN_FILEBEFOREDELETE (NPPN_FIRST + 24)  // To notify plugins that file is to be deleted
	//scnNotification->nmhdr.code = NPPN_FILEBEFOREDELETE;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = BufferID;

	#define NPPN_FILEDELETEFAILED (NPPN_FIRST + 25)  // To notify plugins that file deletion has failed
	//scnNotification->nmhdr.code = NPPN_FILEDELETEFAILED;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = BufferID;

	#define NPPN_FILEDELETED (NPPN_FIRST + 26)  // To notify plugins that file has been deleted
	//scnNotification->nmhdr.code = NPPN_FILEDELETED;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = BufferID;

#endif //NOTEPAD_PLUS_MSGS_H
