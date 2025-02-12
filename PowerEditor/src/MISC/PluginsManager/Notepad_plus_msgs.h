// This file is part of Notepad++ project
// Copyright (C)2024 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.



// For more comprehensive information on plugin communication, please refer to the following resource:
// https://npp-user-manual.org/docs/plugin-communication/


#pragma once

#include <windows.h>
#include <tchar.h>

enum LangType {L_TEXT, L_PHP , L_C, L_CPP, L_CS, L_OBJC, L_JAVA, L_RC,\
			   L_HTML, L_XML, L_MAKEFILE, L_PASCAL, L_BATCH, L_INI, L_ASCII, L_USER,\
			   L_ASP, L_SQL, L_VB, L_JS, L_CSS, L_PERL, L_PYTHON, L_LUA, \
			   L_TEX, L_FORTRAN, L_BASH, L_FLASH, L_NSIS, L_TCL, L_LISP, L_SCHEME,\
			   L_ASM, L_DIFF, L_PROPS, L_PS, L_RUBY, L_SMALLTALK, L_VHDL, L_KIX, L_AU3,\
			   L_CAML, L_ADA, L_VERILOG, L_MATLAB, L_HASKELL, L_INNO, L_SEARCHRESULT,\
			   L_CMAKE, L_YAML, L_COBOL, L_GUI4CLI, L_D, L_POWERSHELL, L_R, L_JSP,\
			   L_COFFEESCRIPT, L_JSON, L_JAVASCRIPT, L_FORTRAN_77, L_BAANC, L_SREC,\
			   L_IHEX, L_TEHEX, L_SWIFT,\
			   L_ASN1, L_AVS, L_BLITZBASIC, L_PUREBASIC, L_FREEBASIC, \
			   L_CSOUND, L_ERLANG, L_ESCRIPT, L_FORTH, L_LATEX, \
			   L_MMIXAL, L_NIM, L_NNCRONTAB, L_OSCRIPT, L_REBOL, \
			   L_REGISTRY, L_RUST, L_SPICE, L_TXT2TAGS, L_VISUALPROLOG,\
			   L_TYPESCRIPT, L_JSON5, L_MSSQL, L_GDSCRIPT, L_HOLLYWOOD,\
			   L_GOLANG, L_RAKU, L_TOML, L_SAS,\
			   // Don't use L_JS, use L_JAVASCRIPT instead
			   // The end of enumated language type, so it should be always at the end
			   L_EXTERNAL};
enum class ExternalLexerAutoIndentMode { Standard, C_Like, Custom };
enum class MacroStatus { Idle, RecordInProgress, RecordingStopped, PlayingBack };

enum winVer { WV_UNKNOWN, WV_WIN32S, WV_95, WV_98, WV_ME, WV_NT, WV_W2K, WV_XP, WV_S2003, WV_XPX64, WV_VISTA, WV_WIN7, WV_WIN8, WV_WIN81, WV_WIN10, WV_WIN11 };
enum Platform { PF_UNKNOWN, PF_X86, PF_X64, PF_IA64, PF_ARM64 };



#define NPPMSG  (WM_USER + 1000)

	#define NPPM_GETCURRENTSCINTILLA (NPPMSG + 4)
	// BOOL NPPM_GETCURRENTSCINTILLA(0, int* iScintillaView)
	// Get current Scintilla view.
	// wParam: 0 (not used)
	// lParam[out]: iScintillaView could be 0 (Main View) or 1 (Sub View)
	// return TRUE

	#define NPPM_GETCURRENTLANGTYPE (NPPMSG + 5)
	// BOOL NPPM_GETCURRENTLANGTYPE(0, int* langType)
	// Get the programming language type from the current used document.
	// wParam: 0 (not used)
	// lParam[out]: langType - see "enum LangType" for all valid values
	// return TRUE

	#define NPPM_SETCURRENTLANGTYPE (NPPMSG + 6)
	// BOOL NPPM_SETCURRENTLANGTYPE(0, int langType)
	// Set a new programming language type to the current used document.
	// wParam: 0 (not used)
	// lParam[in]: langType - see "enum LangType" for all valid values
	// return TRUE

	#define NPPM_GETNBOPENFILES (NPPMSG + 7)
		#define ALL_OPEN_FILES   0
		#define PRIMARY_VIEW     1
		#define SECOND_VIEW      2
	// int NPPM_GETNBOPENFILES(0, int iViewType)
	// Get the number of files currently open.
	// wParam: 0 (not used)
	// lParam[in]: iViewType - could be PRIMARY_VIEW (value 1), SECOND_VIEW (value 2) or ALL_OPEN_FILES (value 0)
	// return the number of opened files

	#define NPPM_GETOPENFILENAMES  (NPPMSG + 8)
	// BOOL NPPM_GETOPENFILENAMES(wchar_t** fileNames, int nbFileNames)
	// Get the open files full paths of both views. User is responsible to allocate an big enough fileNames array by using NPPM_GETNBOPENFILES.
	// wParam[out]: fileNames - array of file path
	// lParam[in]: nbFileNames is the number of file path.
	// return value: The number of files copied into fileNames array

	#define NPPM_MODELESSDIALOG  (NPPMSG + 12)
		#define MODELESSDIALOGADD    0
		#define MODELESSDIALOGREMOVE 1
	// HWND NPPM_MODELESSDIALOG(int action, HWND hDlg)
	// Register (or unregister) plugin's dialog handle.
	// For each created dialog in your plugin, you should register it (and unregister while destroy it) to Notepad++ by using this message.
	// If this message is ignored, then your dialog won't react with the key stroke messages such as TAB, Ctrl-C or Ctrl-V key.
	// For the good functioning of your plugin dialog, you're recommended to not ignore this message.
	// wParam[in]: action is MODELESSDIALOGADD (for registering your hDlg) or MODELESSDIALOGREMOVE (for unregistering your hDlg)
	// lParam[in]: hDlg is the handle of dialog to register/unregister
	// return hDlg (HWND) on success, NULL on failure

	#define NPPM_GETNBSESSIONFILES (NPPMSG + 13)
	// int NPPM_GETNBSESSIONFILES (BOOL* pbIsValidXML, wchar_t* sessionFileName)
	// Get the number of files to load in the session sessionFileName. sessionFileName should be a full path name of an xml file.
	// wParam[out]: pbIsValidXML, if the lParam pointer is null, then this parameter will be ignored. TRUE if XML is valid, otherwise FALSE.
	// lParam[in]: sessionFileName is XML session full path
	// return value: The number of files in XML session file

	#define NPPM_GETSESSIONFILES (NPPMSG + 14)
	// BOOL NPPM_GETSESSIONFILES (wchar_t** sessionFileArray, wchar_t* sessionFileName)
	// the files' full path name from a session file.
	// wParam[out]: sessionFileArray is the array in which the files' full path of the same group are written. To allocate the array with the proper size, send message NPPM_GETNBSESSIONFILES.
	// lParam[in]: sessionFileName is XML session full path
	// Return FALSE on failure, TRUE on success

	#define NPPM_SAVESESSION (NPPMSG + 15)
		struct sessionInfo {
			wchar_t* sessionFilePathName; // Full session file path name to be saved
			int nbFile;                 // Size of "files" array - number of files to be saved in session
			wchar_t** files;              // Array of file name (full path) to be saved in session
		};
	// NPPM_SAVESESSION(0, sessionInfo* si)
	// Creates an session file for a defined set of files.
	// Contrary to NPPM_SAVECURRENTSESSION (see below), which saves the current opened files, this call can be used to freely define any file which should be part of a session.
	// wParam: 0 (not used)
	// lParam[in]: si is a pointer to sessionInfo structure
	// Returns sessionFileName on success, NULL otherwise

	#define NPPM_SAVECURRENTSESSION (NPPMSG + 16)
	// wchar_t* NPPM_SAVECURRENTSESSION(0, wchar_t* sessionFileName)
	// Saves the current opened files in Notepad++ as a group of files (session) as an xml file.
	// wParam: 0 (not used)
	// lParam[in]: sessionFileName is the xml full path name
	// Returns sessionFileName on success, NULL otherwise


	#define NPPM_GETOPENFILENAMESPRIMARY (NPPMSG + 17)
	// BOOL NPPM_GETOPENFILENAMESPRIMARY(wchar_t** fileNames, int nbFileNames)
	// Get the open files full paths of main view. User is responsible to allocate an big enough fileNames array by using NPPM_GETNBOPENFILES.
	// wParam[out]: fileNames - array of file path
	// lParam[in]: nbFileNames is the number of file path.
	// return value: The number of files copied into fileNames array

	#define NPPM_GETOPENFILENAMESSECOND (NPPMSG + 18)
	// BOOL NPPM_GETOPENFILENAMESSECOND(wchar_t** fileNames, int nbFileNames)
	// Get the open files full paths of sub-view. User is responsible to allocate an big enough fileNames array by using NPPM_GETNBOPENFILES.
	// wParam[out]: fileNames - array of file path
	// lParam[in]: nbFileNames is the number of file path.
	// return value: The number of files copied into fileNames array

	#define NPPM_CREATESCINTILLAHANDLE (NPPMSG + 20)
	// HWND NPPM_CREATESCINTILLAHANDLE(0, HWND hParent)
	// A plugin can create a Scintilla for its usage by sending this message to Notepad++.
	// wParam: 0 (not used)
	// lParam[in]: hParent - If set (non NULL), it will be the parent window of this created Scintilla handle, otherwise the parent window is Notepad++
	// return the handle of created Scintilla handle

	#define NPPM_DESTROYSCINTILLAHANDLE_DEPRECATED (NPPMSG + 21)
	// BOOL NPPM_DESTROYSCINTILLAHANDLE_DEPRECATED(0, HWND hScintilla) - DEPRECATED: It is kept for the compatibility.
	// Notepad++ will deallocate every createed Scintilla control on exit, this message returns TRUE but does nothing.
	// wParam: 0 (not used)
	// lParam[in]: hScintilla is Scintilla handle
	// Return TRUE

	#define NPPM_GETNBUSERLANG (NPPMSG + 22)
	// int NPPM_GETNBUSERLANG(0, int* udlID)
	// Get the number of user defined languages and, optionally, the starting menu id.
	// wParam: 0 (not used)
	// lParam[out]: udlID is optional, if not used set it to 0, otherwise an integer pointer is needed to retrieve the menu identifier.
	// Return the number of user defined languages identified

	#define NPPM_GETCURRENTDOCINDEX (NPPMSG + 23)
		#define MAIN_VIEW 0
		#define SUB_VIEW 1
	// int NPPM_GETCURRENTDOCINDEX(0, int inView)
	// Get the current index of the given view.
	// wParam: 0 (not used)
	// lParam[in]: inView, should be 0 (main view) or 1 (sub-view)
	// Return -1 if the view is invisible (hidden), otherwise is the current index.

	#define NPPM_SETSTATUSBAR (NPPMSG + 24)
		#define STATUSBAR_DOC_TYPE     0
		#define STATUSBAR_DOC_SIZE     1
		#define STATUSBAR_CUR_POS      2
		#define STATUSBAR_EOF_FORMAT   3
		#define STATUSBAR_UNICODE_TYPE 4
		#define STATUSBAR_TYPING_MODE  5
	// BOOL NPPM_SETSTATUSBAR(int whichPart, wchar_t *str2set)
	// Set string in the specified field of a statusbar.
	// wParam[in]: whichPart for indicating the statusbar part you want to set. It can be only the above value (0 - 5)
	// lParam[in]: str2set is the string you want to write to the part of statusbar.
	// Return FALSE on failure, TRUE on success

	#define NPPM_GETMENUHANDLE (NPPMSG + 25)
		#define NPPPLUGINMENU 0
		#define NPPMAINMENU 1
	// int NPPM_GETMENUHANDLE(int menuChoice, 0)
	// Get menu handle (HMENU) of choice.
	// wParam[in]: menuChoice could be main menu (NPPMAINMENU) or Plugin menu (NPPPLUGINMENU)
	// lParam: 0 (not used)
	// Return: menu handle (HMENU) of choice (plugin menu handle or Notepad++ main menu handle)

	#define NPPM_ENCODESCI (NPPMSG + 26)
	// int NPPM_ENCODESCI(int inView, 0)
	// Changes current buffer in view to UTF-8.
	// wParam[in]: inView - main view (0) or sub-view (1)
	// lParam: 0 (not used)
	// return new UniMode, with the following value:
	// 0: ANSI
	// 1: UTF-8 with BOM
	// 2: UTF-16 Big Ending with BOM
	// 3: UTF-16 Little Ending with BOM
	// 4: UTF-8 without BOM
	// 5: uni7Bit
	// 6: UTF-16 Big Ending without BOM
	// 7: UTF-16 Little Ending without BOM

	#define NPPM_DECODESCI (NPPMSG + 27)
	// int NPPM_DECODESCI(int inView, 0)
	// Changes current buffer in view to ANSI.
	// wParam[in]: inView - main view (0) or sub-view (1)
	// lParam: 0 (not used)
	// return old UniMode - see above

	#define NPPM_ACTIVATEDOC (NPPMSG + 28)
	// BOOL NPPM_ACTIVATEDOC(int inView, int index2Activate)
	// Switch to the document by the given view and index.
	// wParam[in]: inView - main view (0) or sub-view (1)
	// lParam[in]: index2Activate - index (in the view indicated above) where is the document to be activated
	// Return TRUE

	#define NPPM_LAUNCHFINDINFILESDLG (NPPMSG + 29)
	// BOOL NPPM_LAUNCHFINDINFILESDLG(wchar_t * dir2Search, wchar_t * filtre)
	// Launch Find in Files dialog and set "Find in" directory and filters with the given arguments.
	// wParam[in]: if dir2Search is not NULL, it will be set as working directory in which Notepad++ will search
	// lParam[in]: if filtre is not NULL, filtre string will be set into filter field
	// Return TRUE

	#define NPPM_DMMSHOW (NPPMSG + 30)
	// BOOL NPPM_DMMSHOW(0, HWND hDlg)
	// Show the dialog which was previously regeistered by NPPM_DMMREGASDCKDLG.
	// wParam: 0 (not used)
	// lParam[in]: hDlg is the handle of dialog to show
	// Return TRUE

	#define NPPM_DMMHIDE	(NPPMSG + 31)
	// BOOL NPPM_DMMHIDE(0, HWND hDlg)
	// Hide the dialog which was previously regeistered by NPPM_DMMREGASDCKDLG.
	// wParam: 0 (not used)
	// lParam[in]: hDlg is the handle of dialog to hide
	// Return TRUE

	#define NPPM_DMMUPDATEDISPINFO (NPPMSG + 32)
	// BOOL NPPM_DMMUPDATEDISPINFO(0, HWND hDlg)
	// Redraw the dialog.
	// wParam: 0 (not used)
	// lParam[in]: hDlg is the handle of dialog to redraw
	// Return TRUE

	#define NPPM_DMMREGASDCKDLG (NPPMSG + 33)
	// BOOL NPPM_DMMREGASDCKDLG(0, tTbData* pData)
	// Pass the necessary dockingData to Notepad++ in order to make your dialog dockable.
	// wParam: 0 (not used)
	// lParam[in]: pData is the pointer of tTbData. Please check tTbData structure in "Docking.h"
	//             Minimum informations which needs to be filled out are hClient, pszName, dlgID, uMask and pszModuleName.
	//             Notice that rcFloatand iPrevCont shouldn't be filled. They are used internally.
	// Return TRUE

	#define NPPM_LOADSESSION (NPPMSG + 34)
	// BOOL NPPM_LOADSESSION(0, wchar_t* sessionFileName)
	// Open all files of same session in Notepad++ via a xml format session file sessionFileName.
	// wParam: 0 (not used)
	// lParam[in]: sessionFileName is the full file path of session file to reload
	// Return TRUE

	#define NPPM_DMMVIEWOTHERTAB (NPPMSG + 35)
	// BOOL WM_DMM_VIEWOTHERTAB(0, wchar_t* name)
	// Show the plugin dialog (switch to plugin tab) with the given name.
	// wParam: 0 (not used)
	// lParam[in]: name should be the same value as previously used to register the dialog (pszName of tTbData)
	// Return TRUE

	#define NPPM_RELOADFILE (NPPMSG + 36)
	// BOOL NPPM_RELOADFILE(BOOL withAlert, wchar_t *filePathName2Reload)
	// Reload the document which matches with the given filePathName2Reload.
	// wParam: 0 (not used)
	// lParam[in]: filePathName2Reload is the full file path of document to reload
	// Return TRUE if reloading file succeeds, otherwise FALSE

	#define NPPM_SWITCHTOFILE (NPPMSG + 37)
	// BOOL NPPM_SWITCHTOFILE(0, wchar_t* filePathName2switch)
	// Switch to the document which matches with the given filePathName2switch.
	// wParam: 0 (not used)
	// lParam[in]: filePathName2switch is the full file path of document to switch
	// Return TRUE

	#define NPPM_SAVECURRENTFILE (NPPMSG + 38)
	// BOOL NPPM_SAVECURRENTFILE(0, 0)
	// Save current activated document.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// Return TRUE if file is saved, otherwise FALSE (the file doesn't need to be saved, or other reasons).

	#define NPPM_SAVEALLFILES	(NPPMSG + 39)
	// BOOL NPPM_SAVEALLFILES(0, 0)
	// Save all opened document.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// Return FALSE when no file needs to be saved, else TRUE if there is at least one file saved.

	#define NPPM_SETMENUITEMCHECK	(NPPMSG + 40)
	// BOOL NPPM_SETMENUITEMCHECK(UINT pluginCmdID, BOOL doCheck)
	// Set or remove the check on a item of plugin menu and tool bar (if any).
	// wParam[in]: pluginCmdID is the plugin command ID which corresponds to the menu item: funcItem[X]._cmdID
	// lParam[in]: if doCheck value is TRUE, item will be checked, FALSE makes item unchecked.
	// Return TRUE

	#define NPPM_ADDTOOLBARICON_DEPRECATED (NPPMSG + 41)
		struct toolbarIcons {
			HBITMAP	hToolbarBmp;
			HICON	hToolbarIcon;
		};
	// BOOL NPPM_ADDTOOLBARICON_DEPRECATED(UINT pluginCmdID, toolbarIcons* iconHandles) -- DEPRECATED: use NPPM_ADDTOOLBARICON_FORDARKMODE instead
	// Add an icon to the toolbar.
	// wParam[in]: pluginCmdID is the plugin command ID which corresponds to the menu item: funcItem[X]._cmdID
	// lParam[in]: iconHandles of toolbarIcons structure. 2 formats ".ico" & ".bmp" are needed
	//             Both handles should be set so the icon will be displayed correctly if toolbar icon sets are changed by users
	// Return TRUE


	#define NPPM_GETWINDOWSVERSION (NPPMSG + 42)
	// winVer NPPM_GETWINDOWSVERSION(0, 0)
	// Get OS (Windows) version.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// Return enum winVer, which is defined at the begining of this file

	#define NPPM_DMMGETPLUGINHWNDBYNAME (NPPMSG + 43)
	// HWND NPPM_DMMGETPLUGINHWNDBYNAME(const wchar_t *windowName, const wchar_t *moduleName)
	// Retrieve the dialog handle corresponds to the windowName and moduleName. You may need this message if you want to communicate with another plugin "dockable" dialog.
	// wParam[in]: windowName - if windowName is NULL, then the first found window handle which matches with the moduleName will be returned
	// lParam[in] : moduleName - if moduleName is NULL, then return value is NULL
	// Return NULL if moduleName is NULL. If windowName is NULL, then the first found window handle which matches with the moduleName will be returned.

	#define NPPM_MAKECURRENTBUFFERDIRTY (NPPMSG + 44)
	// BOOL NPPM_MAKECURRENTBUFFERDIRTY(0, 0)
	// Make the current document dirty, aka set the save state to unsaved.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// Return TRUE

	#define NPPM_GETENABLETHEMETEXTUREFUNC_DEPRECATED (NPPMSG + 45)
	// THEMEAPI NPPM_GETENABLETHEMETEXTUREFUNC(0, 0) -- DEPRECATED: plugin can use EnableThemeDialogTexture directly from uxtheme.h instead
	// Get "EnableThemeDialogTexture" function address.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// Return a proc address or NULL

	#define NPPM_GETPLUGINSCONFIGDIR (NPPMSG + 46)
	// int NPPM_GETPLUGINSCONFIGDIR(int strLen, wchar_t *str)
	// Get user's plugin config directory path. It's useful if plugins want to save/load parameters for the current user
	// wParam[in]: strLen is length of  allocated buffer in which directory path is copied
	// lParam[out] : str is the allocated buffere. User should call this message twice -
	//               The 1st call with "str" be NULL to get the required number of wchar_t (not including the terminating nul character)
	//               The 2nd call to allocate "str" buffer with the 1st call's return value + 1, then call it again to get the path
	// Return value: The 1st call - the number of wchar_t to copy.
	//               The 2nd call - FALSE on failure, TRUE on success

	#define NPPM_MSGTOPLUGIN (NPPMSG + 47)
		struct CommunicationInfo {
			long internalMsg;             // an integer defined by plugin Y, known by plugin X, identifying the message being sent.
			const wchar_t * srcModuleName;  // the complete module name (with the extesion .dll) of caller (plugin X).
			void* info;                   // defined by plugin, the informations to be exchanged between X and Y. It's a void pointer so it should be defined by plugin Y and known by plugin X.
		};
	// BOOL NPPM_MSGTOPLUGIN(wchar_t *destModuleName, CommunicationInfo *info)
	// Send a private information to a plugin with given plugin name. This message allows the communication between 2 plugins.
	// For example, plugin X can execute a command of plugin Y if plugin X knows the command ID and the file name of plugin Y.
	// wParam[in]: destModuleName is the destination complete module file name (with the file extension ".dll")
	// lParam[in]: info - See above "CommunicationInfo" structure
	// The returned value is TRUE if Notepad++ found the plugin by its module name (destModuleName), and pass the info (communicationInfo) to the module.
	// The returned value is FALSE if no plugin with such name is found.

	#define NPPM_MENUCOMMAND (NPPMSG + 48)
	// BOOL NPPM_MENUCOMMAND(0, int cmdID)
	// Run Notepad++ command with the given command ID.
	// wParam: 0 (not used)
	// lParam[in]: cmdID - See "menuCmdID.h" for all the Notepad++ menu command items
	// Return TRUE

	#define NPPM_TRIGGERTABBARCONTEXTMENU (NPPMSG + 49)
	// BOOL NPPM_TRIGGERTABBARCONTEXTMENU(int inView, int index2Activate)
	// Switch to the document by the given view and index and trigger the context menu
	// wParam[in]: inView - main view (0) or sub-view (1)
	// lParam[in]: index2Activate - index (in the view indicated above) where is the document to have the context menu
	// Return TRUE

	#define NPPM_GETNPPVERSION (NPPMSG + 50)
	// int NPPM_GETNPPVERSION(BOOL ADD_ZERO_PADDING, 0)
	// Get Notepad++ version.
	// wParam[in]: ADD_ZERO_PADDING (see below)
	// lParam: 0 (not used)
	// return value:
	// HIWORD(returned_value) is major part of version: the 1st number
	// LOWORD(returned_value) is minor part of version: the 3 last numbers
	//
	// ADD_ZERO_PADDING == TRUE
	//
	// version  | HIWORD | LOWORD
	//------------------------------
	// 8.9.6.4  | 8      | 964
	// 9        | 9      | 0
	// 6.9      | 6      | 900
	// 6.6.6    | 6      | 660
	// 13.6.6.6 | 13     | 666
	//
	//
	// ADD_ZERO_PADDING == FALSE
	//
	// version  | HIWORD | LOWORD
	//------------------------------
	// 8.9.6.4  | 8      | 964
	// 9        | 9      | 0
	// 6.9      | 6      | 9
	// 6.6.6    | 6      | 66
	// 13.6.6.6 | 13     | 666

	#define NPPM_HIDETABBAR (NPPMSG + 51)
	// BOOL NPPM_HIDETABBAR(0, BOOL hideOrNot)
	// Hide (or show) tab bar.
	// wParam: 0 (not used)
	// lParam[in]: if hideOrNot is set as TRUE then tab bar will be hidden, otherwise it'll be shown.
	// return value: the old status value

	#define NPPM_ISTABBARHIDDEN (NPPMSG + 52)
	// BOOL NPPM_ISTABBARHIDDEN(0, 0)
	// Get tab bar status.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// return value: TRUE if tool bar is hidden, otherwise FALSE

	#define NPPM_GETPOSFROMBUFFERID (NPPMSG + 57)
	// int NPPM_GETPOSFROMBUFFERID(UINT_PTR bufferID, int priorityView)
	// Get document position (VIEW and INDEX) from a buffer ID, according priorityView.
	// wParam[in]: BufferID of document
	// lParam[in]: priorityView is the target VIEW. However, if the given bufferID cannot be found in the target VIEW, the other VIEW will be searched.
	// Return -1 if the bufferID non existing, else return value contains VIEW & INDEX:
	//
	// VIEW takes 2 highest bits and INDEX (0 based) takes the rest (30 bits)
	// Here's the values for the view:
	//  MAIN_VIEW 0
	//  SUB_VIEW  1
	//
	// if priorityView set to SUB_VIEW, then SUB_VIEW will be search firstly

	#define NPPM_GETFULLPATHFROMBUFFERID (NPPMSG + 58)
	// int NPPM_GETFULLPATHFROMBUFFERID(UINT_PTR bufferID, wchar_t* fullFilePath)
	// Get full path file name from a bufferID (the pointer of buffer).
	// wParam[in]: bufferID
	// lParam[out]: fullFilePath - User should call it with fullFilePath be NULL to get the number of wchar_t (not including the nul character),
	//         allocate fullFilePath with the return values + 1, then call it again to get full path file name
	// Return -1 if the bufferID non existing, otherwise the number of wchar_t copied/to copy

	#define NPPM_GETBUFFERIDFROMPOS (NPPMSG + 59)
	// UINT_PTR NPPM_GETBUFFERIDFROMPOS(int index, int iView)
	// Get the document bufferID from the given position (iView & index).
	// wParam[in]: Position (0 based) of document
	// lParam[in]: Main or sub View in which document is, 0 = Main, 1 = Sub
	// Returns NULL if invalid, otherwise bufferID

	#define NPPM_GETCURRENTBUFFERID (NPPMSG + 60)
	// UINT_PTR NPPM_GETCURRENTBUFFERID(0, 0)
	// Get active document BufferID.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// Return active document BufferID

	#define NPPM_RELOADBUFFERID (NPPMSG + 61)
	// BOOL NPPM_RELOADBUFFERID(UINT_PTR bufferID, BOOL alert)
	// Reloads document with the given BufferID
	// wParam[in]: BufferID of document to reload
	// lParam[in]: set TRUE to let user confirm or reject the reload; setting FALSE will reload with no alert.
	// Returns TRUE on success, FALSE otherwise

	#define NPPM_GETBUFFERLANGTYPE (NPPMSG + 64)
	// int NPPM_GETBUFFERLANGTYPE(UINT_PTR bufferID, 0)
	// Retrieves the language type of the document with the given bufferID.
	// wParam[in]: BufferID of document to get LangType from
	// lParam: 0 (not used)
	// Returns as int, see LangType. -1 on error

	#define NPPM_SETBUFFERLANGTYPE (NPPMSG + 65)
	// BOOL NPPM_SETBUFFERLANGTYPE(UINT_PTR bufferID, int langType)
	// Set the language type of the document based on the given bufferID.
	// wParam[in]: BufferID to set LangType of
	// lParam[in]: langType as int, enum LangType for valid values (L_USER and L_EXTERNAL are not supported)
	// Returns TRUE on success, FALSE otherwise

	#define NPPM_GETBUFFERENCODING (NPPMSG + 66)
	// int NPPM_GETBUFFERENCODING(UINT_PTR bufferID, 0)
	// Get encoding from the document with the given bufferID
	// wParam[in]: BufferID to get encoding from
	// lParam: 0 (not used)
	// returns -1 on error, otherwise UniMode, with the following value:
	// 0: ANSI
	// 1: UTF-8 with BOM
	// 2: UTF-16 Big Ending with BOM
	// 3: UTF-16 Little Ending with BOM
	// 4: UTF-8 without BOM
	// 5: uni7Bit
	// 6: UTF-16 Big Ending without BOM
	// 7: UTF-16 Little Ending without BOM

	#define NPPM_SETBUFFERENCODING (NPPMSG + 67)
	// BOOL NPPM_SETBUFFERENCODING(UINT_PTR bufferID, int encoding)
	// Set encoding to the document with the given bufferID
	// wParam: BufferID to set encoding of
	// lParam: encoding, see UniMode value in NPPM_GETBUFFERENCODING (above)
	// Returns TRUE on success, FALSE otherwise
	// Can only be done on new, unedited files

	#define NPPM_GETBUFFERFORMAT (NPPMSG + 68)
	// int NPPM_GETBUFFERFORMAT(UINT_PTR bufferID, 0)
	// Get the EOL format of the document with given bufferID.
	// wParam[in]: BufferID to get EolType format from
	// lParam: 0 (not used)
	// Returned value is  -1 on error, otherwize EolType format:
	// 0: Windows (CRLF)
	// 1: Macos (CR)
	// 2: Unix (LF)
	// 3. Unknown

	#define NPPM_SETBUFFERFORMAT (NPPMSG + 69)
	// BOOL NPPM_SETBUFFERFORMAT(UINT_PTR bufferID, int format)
	// Set the EOL format to the document with given bufferID.
	// wParam[in]: BufferID to set EolType format of
	// lParam[in]: EolType format. For EolType format value, see NPPM_GETBUFFERFORMAT (above)
	// Returns TRUE on success, FALSE otherwise

	#define NPPM_HIDETOOLBAR (NPPMSG + 70)
	// BOOL NPPM_HIDETOOLBAR(0, BOOL hideOrNot)
	// Hide (or show) the toolbar.
	// wParam: 0 (not used)
	// lParam[in]: if hideOrNot is set as TRUE then tool bar will be hidden, otherwise it'll be shown.
	// return value: the old status value

	#define NPPM_ISTOOLBARHIDDEN (NPPMSG + 71)
	// BOOL NPPM_ISTOOLBARHIDDEN(0, 0)
	// Get toolbar status.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// return value: TRUE if tool bar is hidden, otherwise FALSE

	#define NPPM_HIDEMENU (NPPMSG + 72)
	// BOOL NPPM_HIDEMENU(0, BOOL hideOrNot)
	// Hide (or show) menu bar.
	// wParam: 0 (not used)
	// lParam[in]: if hideOrNot is set as TRUE then menu will be hidden, otherwise it'll be shown.
	// return value: the old status value

	#define NPPM_ISMENUHIDDEN (NPPMSG + 73)
	// BOOL NPPM_ISMENUHIDDEN(0, 0)
	// Get menu bar status.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// return value: TRUE if menu bar is hidden, otherwise FALSE

	#define NPPM_HIDESTATUSBAR (NPPMSG + 74)
	// BOOL NPPM_HIDESTATUSBAR(0, BOOL hideOrNot)
	// Hide (or show) status bar.
	// wParam: 0 (not used)
	// lParam[in]: if hideOrNot is set as TRUE then status bar will be hidden, otherwise it'll be shown.
	// return value: the old status value

	#define NPPM_ISSTATUSBARHIDDEN (NPPMSG + 75)
	// BOOL NPPM_ISSTATUSBARHIDDEN(0, 0)
	// Get status bar status.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// return value: TRUE if status bar is hidden, otherwise FALSE

	#define NPPM_GETSHORTCUTBYCMDID (NPPMSG + 76)
	// BOOL NPPM_GETSHORTCUTBYCMDID(int cmdID, ShortcutKey* sk)
	// Get your plugin command current mapped shortcut into sk via cmdID.
	// wParam[in]: cmdID is your plugin command ID
	// lParam[out]: sk is a pointer of ShortcutKey strcture which will receive the requested CMD shortcut. It should be allocated in the plugin before being used.
	// For ShortcutKey strcture, see in "PluginInterface.h". You may need it after getting NPPN_READY notification.
	// return value: TRUE if this function call is successful and shortcut is enable, otherwise FALSE

	#define NPPM_DOOPEN (NPPMSG + 77)
	// BOOL NPPM_DOOPEN(0, const wchar_t* fullPathName2Open)
	// Open a file with given fullPathName2Open.
	// If fullPathName2Open has been already opened in Notepad++, the it will be activated and becomes the current document.
	// wParam: 0 (not used)
	// lParam[in]: fullPathName2Open indicates the full file path name to be opened
	// The return value is TRUE if the operation is successful, otherwise FALSE

	#define NPPM_SAVECURRENTFILEAS (NPPMSG + 78)
	// BOOL NPPM_SAVECURRENTFILEAS (BOOL saveAsCopy, const wchar_t* filename)
	// Save the current activated document.
	// wParam[in]: saveAsCopy must be either FALSE to save, or TRUE to save a copy of the current filename ("Save a Copy As..." action)
	// lParam[in]: filename indicates the full file path name to be saved
	// The return value is TRUE if the operation is successful, otherwise FALSE

    #define NPPM_GETCURRENTNATIVELANGENCODING (NPPMSG + 79)
	// int NPPM_GETCURRENTNATIVELANGENCODING(0, 0)
	// Get the code page associated with the current localisation of Notepad++.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// return value: the current native language encoding

	#define NPPM_ALLOCATESUPPORTED_DEPRECATED   (NPPMSG + 80)
	// DEPRECATED: the message has been made (since 2010 AD) for checking if NPPM_ALLOCATECMDID is supported. This message is no more needed.
	// BOOL NPPM_ALLOCATESUPPORTED_DEPRECATED(0, 0)
	// Get NPPM_ALLOCATECMDID supported status. Use to identify if subclassing is necessary
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// returns TRUE if NPPM_ALLOCATECMDID is supported


	#define NPPM_ALLOCATECMDID   (NPPMSG + 81)
	// BOOL NPPM_ALLOCATECMDID(int numberRequested, int* startNumber)
	// Obtain a number of consecutive menu item IDs for creating menus dynamically, with the guarantee of these IDs not clashing with any other plugins.
	// wParam[in]: numberRequested is the number of ID you request for the reservation
	// lParam[out]: startNumber will be set to the initial command ID if successful
	// Returns: TRUE if successful, FALSE otherwise. startNumber will also be set to 0 if unsuccessful
	//
	// Example: If a plugin needs 4 menu item ID, the following code can be used:
	//
	//   int idBegin;
	//   BOOL isAllocatedSuccessful = ::SendMessage(nppData._nppHandle, NPPM_ALLOCATECMDID, 4, &idBegin);
	//
	// if isAllocatedSuccessful is TRUE, and value of idBegin is 46581
	// then menu iten ID 46581, 46582, 46583 and 46584 are preserved by Notepad++, and they are safe to be used by the plugin.

	#define NPPM_ALLOCATEMARKER  (NPPMSG + 82)
    // BOOL NPPM_ALLOCATEMARKER(int numberRequested, int* startNumber)
	// Allocate a number of consecutive marker IDs to a plugin: if a plugin need to add a marker on Notepad++'s Scintilla marker margin,
	// it has to use this message to get marker number, in order to prevent from the conflict with the other plugins.
	// wParam[in]: numberRequested is the number of ID you request for the reservation
	// lParam[out]: startNumber will be set to the initial command ID if successful
    // Return TRUE if successful, FALSE otherwise. startNumber will also be set to 0 if unsuccessful
	//
	// Example: If a plugin needs 3 marker ID, the following code can be used:
	//
	//   int idBegin;
	//   BOOL isAllocatedSuccessful = ::SendMessage(nppData._nppHandle, NPPM_ALLOCATEMARKER, 3, &idBegin);
	//
	// if isAllocatedSuccessful is TRUE, and value of idBegin is 16
	// then marker ID 16, 17 and 18 are preserved by Notepad++, and they are safe to be used by the plugin.

	#define NPPM_GETLANGUAGENAME  (NPPMSG + 83)
	// int NPPM_GETLANGUAGENAME(LangType langType, wchar_t* langName)
	// Get programming language name from the given language type (enum LangType).
	// wParam[in]: langType is the number of LangType
	// lParam[out]: langName is the buffer to recieve the language name string
	// Return value is the number of copied character / number of character to copy (\0 is not included)
	//
	// You should call this function 2 times - the first time you pass langName as NULL to get the number of characters to copy.
    // You allocate a buffer of the length of (the number of characters + 1) then call NPPM_GETLANGUAGENAME function the 2nd time
	// by passing allocated buffer as argument langName

	#define NPPM_GETLANGUAGEDESC  (NPPMSG + 84)
	// INT NPPM_GETLANGUAGEDESC(int langType, wchar_t *langDesc)
	// Get programming language short description from the given language type (enum LangType)
	// wParam[in]: langType is the number of LangType
	// lParam[out]: langDesc is the buffer to recieve the language description string
	// Return value is the number of copied character / number of character to copy (\0 is not included)
	//
	// You should call this function 2 times - the first time you pass langDesc as NULL to get the number of characters to copy.
    // You allocate a buffer of the length of (the number of characters + 1) then call NPPM_GETLANGUAGEDESC function the 2nd time
	// by passing allocated buffer as argument langDesc

	#define NPPM_SHOWDOCLIST    (NPPMSG + 85)
	// BOOL NPPM_SHOWDOCLIST(0, BOOL toShowOrNot)
	// Show or hide the Document List panel.
	// wParam: 0 (not used)
	// lParam[in]: toShowOrNot - if toShowOrNot is TRUE, the Document List panel is shown otherwise it is hidden.
	// Return TRUE

	#define NPPM_ISDOCLISTSHOWN    (NPPMSG + 86)
	// BOOL NPPM_ISDOCLISTSHOWN(0, 0)
	// Checks the visibility of the Document List panel.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// return value: TRUE if the Document List panel is currently shown, FALSE otherwise

	#define NPPM_GETAPPDATAPLUGINSALLOWED    (NPPMSG + 87)
	// BOOL NPPM_GETAPPDATAPLUGINSALLOWED(0, 0)
	// Check to see if loading plugins from "%APPDATA%\..\Local\Notepad++\plugins" is allowed.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// return value: TRUE if loading plugins from %APPDATA% is allowed, FALSE otherwise

	#define NPPM_GETCURRENTVIEW    (NPPMSG + 88)
	// int NPPM_GETCURRENTVIEW(0, 0)
	// Get the current used view.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// Return: current edit view of Notepad++. Only 2 possible values: 0 = Main, 1 = Secondary

	#define NPPM_DOCLISTDISABLEEXTCOLUMN    (NPPMSG + 89)
	// BOOL NPPM_DOCLISTDISABLEEXTCOLUMN(0, BOOL disableOrNot)
	// Disable or enable extension column of Document List
	// wParam: 0 (not used)
	// lParam[in]: disableOrNot - if disableOrNot is TRUE, extension column is hidden otherwise it is visible.
	// Return TRUE

	#define NPPM_DOCLISTDISABLEPATHCOLUMN    (NPPMSG + 102)
	// BOOL NPPM_DOCLISTDISABLEPATHCOLUMN(0, BOOL disableOrNot)
	// Disable or enable path column of Document List
	// wParam: 0 (not used)
	// lParam[in]: disableOrNot - if disableOrNot is TRUE, extension column is hidden otherwise it is visible.
	// Return TRUE

	#define NPPM_GETEDITORDEFAULTFOREGROUNDCOLOR    (NPPMSG + 90)
	// int NPPM_GETEDITORDEFAULTFOREGROUNDCOLOR(0, 0)
	// Get the current editor default foreground color.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// Return: the color as integer with hex format being 0x00bbggrr

	#define NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR    (NPPMSG + 91)
	// int NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR(0, 0)
	// Get the current editor default background color.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// Return: the color as integer with hex format being 0x00bbggrr

	#define NPPM_SETSMOOTHFONT    (NPPMSG + 92)
	// BOOL NPPM_SETSMOOTHFONT(0, BOOL setSmoothFontOrNot)
	// Set (or remove) smooth font. The API uses underlying Scintilla command SCI_SETFONTQUALITY to manage the font quality.
	// wParam: 0 (not used)
	// lParam[in]: setSmoothFontOrNot - if value is TRUE, this message sets SC_EFF_QUALITY_LCD_OPTIMIZED else SC_EFF_QUALITY_DEFAULT
	// Return TRUE

	#define NPPM_SETEDITORBORDEREDGE    (NPPMSG + 93)
	// BOOL NPPM_SETEDITORBORDEREDGE(0, BOOL withEditorBorderEdgeOrNot)
	// Add (or remove) an additional sunken edge style to the Scintilla window else it removes the extended style from the window.
	// wParam: 0 (not used)
	// lParam[in]: withEditorBorderEdgeOrNot - TRUE for adding border edge on Scintilla window, FALSE for removing it
	// Return TRUE

	#define NPPM_SAVEFILE (NPPMSG + 94)
	// BOOL NPPM_SAVEFILE(0, const wchar_t *fileNameToSave)
	// Save the file (opened in Notepad++) with the given full file name path.
	// wParam: 0 (not used)
	// lParam[in]: fileNameToSave must be the full file path for the file to be saved.
	// Return TRUE on success, FALSE on fileNameToSave is not found

	#define NPPM_DISABLEAUTOUPDATE (NPPMSG + 95) // 2119 in decimal
	// BOOL NPPM_DISABLEAUTOUPDATE(0, 0)
	// Disable Notepad++ auto-update.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// Return TRUE

	#define NPPM_REMOVESHORTCUTBYCMDID (NPPMSG + 96) // 2120 in decimal
	// BOOL NPPM_REMOVESHORTCUTBYCMDID(int pluginCmdID, 0)
	// Remove the assigned shortcut mapped to pluginCmdID
	// wParam[in]: pluginCmdID
	// lParam: 0 (not used)
	// return value: TRUE if function call is successful, otherwise FALSE

	#define NPPM_GETPLUGINHOMEPATH (NPPMSG + 97)
	// int NPPM_GETPLUGINHOMEPATH(size_t strLen, wchar_t* pluginRootPath)
	// Get plugin home root path. It's useful if plugins want to get its own path by appending <pluginFolderName> which is the name of plugin without extension part.
	// wParam[in]: strLen - size of allocated buffer "pluginRootPath"
	// lParam[out]: pluginRootPath - Users should call it with pluginRootPath be NULL to get the required number of wchar_t (not including the terminating nul character),
	//              allocate pluginRootPath buffer with the return value + 1, then call it again to get the path.
	// Return the number of wchar_t copied/to copy, 0 on copy failed

	#define NPPM_GETSETTINGSONCLOUDPATH (NPPMSG + 98)
	// int NPPM_GETSETTINGSCLOUDPATH(size_t strLen, wchar_t *settingsOnCloudPath)
	// Get settings on cloud path. It's useful if plugins want to store its settings on Cloud, if this path is set.
	// wParam[in]: strLen - size of allocated buffer "settingsOnCloudPath"
	// lParam[out]: settingsOnCloudPath - Users should call it with settingsOnCloudPath be NULL to get the required number of wchar_t (not including the terminating nul character),
	//              allocate settingsOnCloudPath buffer with the return value + 1, then call it again to get the path.
	// Returns the number of wchar_t copied/to copy. If the return value is 0, then this path is not set, or the "strLen" is not enough to copy the path.

	#define NPPM_SETLINENUMBERWIDTHMODE    (NPPMSG + 99)
		#define LINENUMWIDTH_DYNAMIC     0
		#define LINENUMWIDTH_CONSTANT    1
	// BOOL NPPM_SETLINENUMBERWIDTHMODE(0, int widthMode)
	// Set line number margin width in dynamic width mode (LINENUMWIDTH_DYNAMIC) or constant width mode (LINENUMWIDTH_CONSTANT)
	// It may help some plugins to disable non-dynamic line number margins width to have a smoothly visual effect while vertical scrolling the content in Notepad++
	// wParam: 0 (not used)
	// lParam[in]: widthMode should be LINENUMWIDTH_DYNAMIC or LINENUMWIDTH_CONSTANT
	// return TRUE if calling is successful, otherwise return FALSE

	#define NPPM_GETLINENUMBERWIDTHMODE    (NPPMSG + 100)
	// int NPPM_GETLINENUMBERWIDTHMODE(0, 0)
	// Get line number margin width in dynamic width mode (LINENUMWIDTH_DYNAMIC) or constant width mode (LINENUMWIDTH_CONSTANT)
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// Return current line number margin width mode (LINENUMWIDTH_DYNAMIC or LINENUMWIDTH_CONSTANT)

	#define NPPM_ADDTOOLBARICON_FORDARKMODE (NPPMSG + 101)
	struct toolbarIconsWithDarkMode {
		HBITMAP	hToolbarBmp;
		HICON	hToolbarIcon;
		HICON	hToolbarIconDarkMode;
	};
	// BOOL NPPM_ADDTOOLBARICON_FORDARKMODE(UINT pluginCmdID, toolbarIconsWithDarkMode* iconHandles)
	// Use NPPM_ADDTOOLBARICON_FORDARKMODE instead obsolete NPPM_ADDTOOLBARICON (DEPRECATED) which doesn't support the dark mode
	// wParam[in]: pluginCmdID
	// lParam[in]: iconHandles is the pointer of toolbarIconsWithDarkMode structure
	//             All 3 handles below should be set so the icon will be displayed correctly if toolbar icon sets are changed by users, also in dark mode
	// Return TRUE

	#define NPPM_GETEXTERNALLEXERAUTOINDENTMODE  (NPPMSG + 103)
	// BOOL NPPM_GETEXTERNALLEXERAUTOINDENTMODE(const wchar_t* languageName, ExternalLexerAutoIndentMode* autoIndentMode)
	// Get ExternalLexerAutoIndentMode for an installed external programming language.
	// wParam[in]: languageName is external language name to search
	// lParam[out]: autoIndentMode could recieve one of three following values
	//              - Standard (0) means Notepad++ will keep the same TAB indentation between lines;
	//              - C_Like (1) means Notepad++ will perform a C-Language style indentation for the selected external language;
	//              - Custom (2) means a Plugin will be controlling auto-indentation for the current language.
	// returned values: TRUE for successful searches, otherwise FALSE.

	#define NPPM_SETEXTERNALLEXERAUTOINDENTMODE  (NPPMSG + 104)
	// BOOL NPPM_SETEXTERNALLEXERAUTOINDENTMODE(const wchar_t* languageName, ExternalLexerAutoIndentMode autoIndentMode)
	// Set ExternalLexerAutoIndentMode for an installed external programming language.
	// wParam[in]: languageName is external language name to set
	// lParam[in]: autoIndentMode could recieve one of three following values
	//             - Standard (0) means Notepad++ will keep the same TAB indentation between lines;
	//             - C_Like (1) means Notepad++ will perform a C-Language style indentation for the selected external language;
	//             - Custom (2) means a Plugin will be controlling auto-indentation for the current language.
	// return value: TRUE if function call was successful, otherwise FALSE.

	#define NPPM_ISAUTOINDENTON  (NPPMSG + 105)
	// BOOL NPPM_ISAUTOINDENTON(0, 0)
	// Get the current use Auto-Indentation setting in Notepad++ Preferences.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// Return TRUE if Auto-Indentation is on, FALSE otherwise

	#define NPPM_GETCURRENTMACROSTATUS (NPPMSG + 106)
	// MacroStatus NPPM_GETCURRENTMACROSTATUS(0, 0)
	// Get current enum class MacroStatus { Idle, RecordInProgress, RecordingStopped, PlayingBack }
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// Return MacroStatus as int:
	// 0: Idle - macro is not in use and it's empty
	// 1: RecordInProgress - macro is currently being recorded
	// 2: RecordingStopped - macro recording has been stopped
	// 3: PlayingBack - macro is currently being played back

	#define NPPM_ISDARKMODEENABLED (NPPMSG + 107)
	// BOOL NPPM_ISDARKMODEENABLED(0, 0)
	// Get Notepad++ Dark Mode status (ON or OFF).
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// Return TRUE if Dark Mode is enable, otherwise FALSE

	#define NPPM_GETDARKMODECOLORS (NPPMSG + 108)
	// BOOL NPPM_GETDARKMODECOLORS (size_t cbSize, NppDarkMode::Colors* returnColors)
	// Get the colors used in Dark Mode.
	// wParam[in]: cbSize must be filled with sizeof(NppDarkMode::Colors).
	// lParam[out]: returnColors must be a pre-allocated NppDarkMode::Colors struct.
	// Return TRUE when successful, FALSE otherwise.
	// You need to uncomment the following code to use NppDarkMode::Colors structure:
	//
	// namespace NppDarkMode
	// {
	//	struct Colors
	//	{
	//		COLORREF background = 0;
	//		COLORREF softerBackground = 0;
	//		COLORREF hotBackground = 0;
	//		COLORREF pureBackground = 0;
	//		COLORREF errorBackground = 0;
	//		COLORREF text = 0;
	//		COLORREF darkerText = 0;
	//		COLORREF disabledText = 0;
	//		COLORREF linkText = 0;
	//		COLORREF edge = 0;
	//		COLORREF hotEdge = 0;
	//		COLORREF disabledEdge = 0;
	//	};
	// }
	//
	// Note: in the case of calling failure ("false" is returned), you may need to change NppDarkMode::Colors structure to:
	// https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/NppDarkMode.h#L32

	#define NPPM_GETCURRENTCMDLINE (NPPMSG + 109)
	// int NPPM_GETCURRENTCMDLINE(size_t strLen, wchar_t *commandLineStr)
	// Get the Current Command Line string.
	// Users should call it with commandLineStr as NULL to get the required number of wchar_t (not including the terminating nul character),
	// allocate commandLineStr buffer with the return value + 1, then call it again to get the current command line string.
	// wParam[in]: strLen is "commandLineStr" buffer length
	// lParam[out]: commandLineStr receives all copied command line string
	// Return the number of wchar_t copied/to copy


	#define NPPM_CREATELEXER (NPPMSG + 110)
	// void* NPPM_CREATELEXER(0, const wchar_t* lexer_name)
	// Get the ILexer pointer created by Lexilla. Call the lexilla "CreateLexer()" function to allow plugins to set the lexer for a Scintilla instance created by NPPM_CREATESCINTILLAHANDLE.
	// wParam: 0 (not used)
	// lParam[in]: lexer_name is the name of the lexer
	// Return the ILexer pointer

	#define NPPM_GETBOOKMARKID (NPPMSG + 111)
	// int NPPM_GETBOOKMARKID(0, 0)
	// Get the bookmark ID - use this API to get bookmark ID dynamically that garantees you get always the right bookmark ID even it's been changed through the different versions.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// Return bookmark ID

	#define NPPM_DARKMODESUBCLASSANDTHEME (NPPMSG + 112)
	namespace NppDarkMode
	{
		// Standard flags for main parent after its children are initialized.
		constexpr ULONG dmfInit = 0x0000000BUL;

		// Standard flags for main parent usually used in NPPN_DARKMODECHANGED.
		constexpr ULONG dmfHandleChange = 0x0000000CUL;
	};

	// ULONG NPPM_DARKMODESUBCLASSANDTHEME(ULONG dmFlags, HWND hwnd)
	// Add support for generic dark mode to plugin dialog. Subclassing is applied automatically unless DWS_USEOWNDARKMODE flag is used.
	// Might not work properly in C# plugins.
	// wParam[in]: dmFlags has 2 possible value dmfInit (0x0000000BUL) & dmfHandleChange (0x0000000CUL) - see above definition
	// lParam[in]: hwnd is the dialog handle of plugin -  Docking panels don't need to call NPPM_DARKMODESUBCLASSANDTHEME
	// Returns succesful combinations of flags.

	// Examples:
	//
	// - after controls initializations in WM_INITDIALOG, in WM_CREATE or after CreateWindow:
	//
	//auto success = static_cast<ULONG>(::SendMessage(nppData._nppHandle, NPPM_DARKMODESUBCLASSANDTHEME, static_cast<WPARAM>(NppDarkMode::dmfInit), reinterpret_cast<LPARAM>(mainHwnd)));
	//
	// - handling dark mode change:
	//
	//extern "C" __declspec(dllexport) void beNotified(SCNotification * notifyCode)
	//{
	//	switch (notifyCode->nmhdr.code)
	//	{
	//		case NPPN_DARKMODECHANGED:
	//		{
	//			::SendMessage(nppData._nppHandle, NPPM_DARKMODESUBCLASSANDTHEME, static_cast<WPARAM>(dmfHandleChange), reinterpret_cast<LPARAM>(mainHwnd));
	//			::SetWindowPos(mainHwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED); // to redraw titlebar and window
	//			break;
	//		}
	//	}
	//}

	#define NPPM_ALLOCATEINDICATOR  (NPPMSG + 113)
	// BOOL NPPM_ALLOCATEINDICATOR(int numberRequested, int* startNumber)
	// Allocates an indicator number to a plugin: if a plugin needs to add an indicator,
	// it has to use this message to get the indicator number, in order to prevent a conflict with the other plugins.
	// wParam[in]: numberRequested is the number of ID you request for the reservation
	// lParam[out]: startNumber will be set to the initial command ID if successful
	// Return TRUE if successful, FALSE otherwise. startNumber will also be set to 0 if unsuccessful
	//
	// Example: If a plugin needs 1 indicator ID, the following code can be used :
	//
	//    int idBegin;
	//    BOOL isAllocatedSuccessful = ::SendMessage(nppData._nppHandle, NPPM_ALLOCATEINDICATOR, 1, &idBegin);
	//
	// if isAllocatedSuccessful is TRUE, and value of idBegin is 7
	// then indicator ID 7 is preserved by Notepad++, and it is safe to be used by the plugin.

	#define NPPM_GETTABCOLORID (NPPMSG + 114)
	// int NPPM_GETTABCOLORID(int view, int tabIndex)
	// Get the tab color ID with given tab index and view.
	// wParam[in]: view - main view (0) or sub-view (1) or -1 (active view)
	// lParam[in]: tabIndex - index (in the view indicated above). -1 for currently active tab
	// Return tab color ID which contains the following values: 0 (yellow), 1 (green), 2 (blue), 3 (orange), 4 (pink) or -1 (no color)
	//
	// Note: there's no symetric command NPPM_SETTABCOLORID. Plugins can use NPPM_MENUCOMMAND to set current tab color with the desired tab color ID.

	#define NPPM_SETUNTITLEDNAME (NPPMSG + 115)
	// BOOL NPPM_SETUNTITLEDNAME(BufferID id, const wchar_t* newName)
	// Rename the tab name for an untitled tab.
	// wParam[in]: id - BufferID of the tab. -1 for currently active tab
	// lParam[in]: newName - the desired new name of the tab
	// Return TRUE upon success; FALSE upon failure

	#define NPPM_GETNATIVELANGFILENAME (NPPMSG + 116)
	// int NPPM_GETNATIVELANGFILENAME(size_t strLen, char* nativeLangFileName)
	// Get the Current native language file name string. Use it after getting NPPN_READY notification to find out which native language is used.
	// Users should call it with nativeLangFileName as NULL to get the required number of char (not including the terminating nul character),
	// allocate language file name string buffer with the return value + 1, then call it again to get the current native language file name string.
	// If there's no localization file applied, the returned value is 0.
	// wParam[in]: strLen is "language file name string" buffer length
	// lParam[out]: language file name string receives all copied native language file name string
	// Return the number of char copied/to copy

	#define NPPM_ADDSCNMODIFIEDFLAGS (NPPMSG + 117)
	// BOOL NPPM_ADDSCNMODIFIEDFLAGS(0, unsigned long scnMotifiedFlags2Add)
	// Add the necessary SCN_MODIFIED flags so that your plugin will receive the SCN_MODIFIED notification for these events, enabling your specific treatments.
	// By default, Notepad++ only forwards SCN_MODIFIED with the following 5 flags/events:
	// SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT | SC_PERFORMED_UNDO | SC_PERFORMED_REDO | SC_MOD_CHANGEINDICATOR to plugins.
	// If your plugin needs to process other SCN_MODIFIED events, you should add the required flags by sending this message to Notepad++. You can send it immediately after receiving NPPN_READY,
	// or only when your plugin needs to listen to specific events (to avoid penalizing Notepad++'s performance). Just ensure that the message is sent only once.
	// wParam: 0 (not used)
	// lParam[in]: scnMotifiedFlags2Add - Scintilla SCN_MODIFIED flags to add. 
	// Return TRUE
	//
	// Example:
	//
	//  extern "C" __declspec(dllexport) void beNotified(SCNotification* notifyCode)
	//  {
	//  	switch (notifyCode->nmhdr.code)
	//  	{
	//  		case NPPN_READY:
	//  		{
	//  			// Add SC_MOD_BEFOREDELETE and SC_MOD_BEFOREINSERT to listen to the 2 events of SCN_MODIFIED
	//  			::SendMessage(nppData._nppHandle, NPPM_ADDSCNMODIFIEDFLAGS, 0, SC_MOD_BEFOREDELETE | SC_MOD_BEFOREINSERT); 
	//  		}
	//  		break;
	//  		...
	//  	}
	//  	...
	//  }


	// For RUNCOMMAND_USER
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
	#define NPP_FULL_FILE_PATH 10
	#define GETFILENAMEATCURSOR 11
	#define CURRENT_LINESTR 12

	#define	RUNCOMMAND_USER    (WM_USER + 3000)

	#define NPPM_GETFULLCURRENTPATH		(RUNCOMMAND_USER + FULL_CURRENT_PATH)
	#define NPPM_GETCURRENTDIRECTORY	(RUNCOMMAND_USER + CURRENT_DIRECTORY)
	#define NPPM_GETFILENAME			(RUNCOMMAND_USER + FILE_NAME)
	#define NPPM_GETNAMEPART			(RUNCOMMAND_USER + NAME_PART)
	#define NPPM_GETEXTPART				(RUNCOMMAND_USER + EXT_PART)
	#define NPPM_GETCURRENTWORD			(RUNCOMMAND_USER + CURRENT_WORD)
	#define NPPM_GETNPPDIRECTORY		(RUNCOMMAND_USER + NPP_DIRECTORY)
	#define NPPM_GETNPPFULLFILEPATH		(RUNCOMMAND_USER + NPP_FULL_FILE_PATH)
	#define NPPM_GETFILENAMEATCURSOR	(RUNCOMMAND_USER + GETFILENAMEATCURSOR)
	#define NPPM_GETCURRENTLINESTR      (RUNCOMMAND_USER + CURRENT_LINESTR)
	// BOOL NPPM_GETXXXXXXXXXXXXXXXX(size_t strLen, wchar_t *str)
	// Get XXX string operations.
	// wParam[in]: strLen is the allocated array size
	// lParam[out]: str is the allocated wchar_t array
	// The return value is TRUE when get std::wstring operation success, otherwise FALSE (allocated array size is too small)


	#define NPPM_GETCURRENTLINE			(RUNCOMMAND_USER + CURRENT_LINE)
	// int NPPM_GETCURRENTLINE(0, 0)
	// Get current line number.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// return the caret current position line

	#define NPPM_GETCURRENTCOLUMN			(RUNCOMMAND_USER + CURRENT_COLUMN)
	// int NPPM_GETCURRENTCOLUMN(0, 0)
	// Get current column number.
	// wParam: 0 (not used)
	// lParam: 0 (not used)
	// return the caret current position column



// Notification code
#define NPPN_FIRST 1000
	#define NPPN_READY (NPPN_FIRST + 1) // To notify plugins that all the initialization for launching Notepad++ is complete.
	//scnNotification->nmhdr.code = NPPN_READY;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = 0;

	#define NPPN_TBMODIFICATION (NPPN_FIRST + 2) // To notify plugins that toolbar icons can be registered
	//scnNotification->nmhdr.code = NPPN_TBMODIFICATION;
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
	//scnNotification->nmhdr.code = NPPN_FILEBEFORESAVE;
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
	//scnNotification->nmhdr.code = NPPN_SHORTCUTREMAPPED;
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
	//scnNotification->nmhdr.code = NPPN_FILEBEFORELOAD;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = NULL;

	#define NPPN_FILELOADFAILED (NPPN_FIRST + 15)  // To notify plugins that file open operation failed
	//scnNotification->nmhdr.code = NPPN_FILELOADFAILED;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = BufferID;

	#define NPPN_READONLYCHANGED (NPPN_FIRST + 16)  // To notify plugins that current document change the readonly status,
	//scnNotification->nmhdr.code = NPPN_READONLYCHANGED;
	//scnNotification->nmhdr.hwndFrom = bufferID;
	//scnNotification->nmhdr.idFrom = docStatus;
		// where bufferID is BufferID
		//       docStatus can be combined by DOCSTATUS_READONLY and DOCSTATUS_BUFFERDIRTY

		#define DOCSTATUS_READONLY 1
		#define DOCSTATUS_BUFFERDIRTY 2

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

	#define NPPN_DARKMODECHANGED (NPPN_FIRST + 27) // To notify plugins that Dark Mode was enabled/disabled
	//scnNotification->nmhdr.code = NPPN_DARKMODECHANGED;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = 0;

	#define NPPN_CMDLINEPLUGINMSG (NPPN_FIRST + 28)  // To notify plugins that the new argument for plugins (via '-pluginMessage="YOUR_PLUGIN_ARGUMENT"' in command line) is available
	//scnNotification->nmhdr.code = NPPN_CMDLINEPLUGINMSG;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = pluginMessage; //where pluginMessage is pointer of type wchar_t

	#define NPPN_EXTERNALLEXERBUFFER (NPPN_FIRST + 29)  // To notify lexer plugins that the buffer (in idFrom) is just applied to a external lexer
	//scnNotification->nmhdr.code = NPPN_EXTERNALLEXERBUFFER;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = BufferID; //where pluginMessage is pointer of type wchar_t

	#define NPPN_GLOBALMODIFIED (NPPN_FIRST + 30)  // To notify plugins that the current document is just modified by Replace All action.
                                                   // For solving the performance issue (from v8.6.4), Notepad++ doesn't trigger SCN_MODIFIED during Replace All action anymore.
                                                   // As a result, the plugins which monitor SCN_MODIFIED should also monitor NPPN_GLOBALMODIFIED.
                                                   // This notification is implemented in Notepad++ v8.6.5.
	//scnNotification->nmhdr.code = NPPN_GLOBALMODIFIED;
	//scnNotification->nmhdr.hwndFrom = BufferID;
	//scnNotification->nmhdr.idFrom = 0; // preserved for the future use, must be zero

	#define NPPN_NATIVELANGCHANGED (NPPN_FIRST + 31)  // To notify plugins that the current native language is just changed to another one.
                                                      // Use NPPM_GETNATIVELANGFILENAME to get current native language file name.
                                                      // Use NPPM_GETMENUHANDLE(NPPPLUGINMENU, 0) to get submenu "Plugins" handle (HMENU)
	//scnNotification->nmhdr.code = NPPN_NATIVELANGCHANGED;
	//scnNotification->nmhdr.hwndFrom = hwndNpp
	//scnNotification->nmhdr.idFrom = 0; // preserved for the future use, must be zero
