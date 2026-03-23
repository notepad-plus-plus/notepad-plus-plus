// menu_builder.mm — Menu bar construction
// Part of the Notepad++ macOS port modular refactor.

#include "menu_builder.h"
#include "npp_constants.h"
#include "app_state.h"
#include "string_utils.h"
#include "language_defs.h"
#include "windows.h"

HMENU buildMenuBar()
{
	HMENU hMenuBar = CreateMenu();

	// File menu
	HMENU hFileMenu = CreatePopupMenu();
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_NEW, L"&New\tCtrl+N");
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_OPEN, L"&Open...\tCtrl+O");
	AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);

	ctx().recentMenu = CreatePopupMenu();
	AppendMenuW(ctx().recentMenu, MF_STRING | MF_GRAYED, 0, L"(No recent files)");
	AppendMenuW(hFileMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(ctx().recentMenu), L"Recent &Files");

	AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_SAVE, L"&Save\tCtrl+S");
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_SAVEAS, L"Save &As...\tCtrl+Shift+S");
	AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_REVEAL_FINDER, L"Reveal in &Finder");
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_COPY_FULL_PATH, L"Copy Full &Path");
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_COPY_FILENAME, L"Copy File &Name");
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_COPY_DIR_PATH, L"Copy &Directory Path");
	AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_PRINT, L"&Print...\tCtrl+P");
	AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_CLOSE, L"&Close\tCtrl+W");
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_CLOSEALL, L"Close &All");
	AppendMenuW(hMenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(hFileMenu), L"&File");

	// Edit menu
	HMENU hEditMenu = CreatePopupMenu();
	AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_UNDO, L"&Undo\tCtrl+Z");
	AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_REDO, L"&Redo\tCtrl+Shift+Z");
	AppendMenuW(hEditMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_CUT, L"Cu&t\tCtrl+X");
	AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_COPY, L"&Copy\tCtrl+C");
	AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_PASTE, L"&Paste\tCtrl+V");
	AppendMenuW(hEditMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_SELECTALL, L"Select &All\tCtrl+A");
	AppendMenuW(hEditMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_AUTOCOMPLETE, L"Auto-&Complete\tF5");
	AppendMenuW(hEditMenu, MF_STRING | (ctx().autoCloseBrackets ? MF_CHECKED : MF_UNCHECKED),
	            IDM_EDIT_AUTOCLOSE_BRACKETS, L"Auto-close &Brackets");
	AppendMenuW(hEditMenu, MF_SEPARATOR, 0, nullptr);

	HMENU hCaseMenu = CreatePopupMenu();
	AppendMenuW(hCaseMenu, MF_STRING, IDM_EDIT_UPPERCASE, L"&UPPERCASE\tCtrl+Shift+U");
	AppendMenuW(hCaseMenu, MF_STRING, IDM_EDIT_LOWERCASE, L"&lowercase\tCtrl+U");
	AppendMenuW(hCaseMenu, MF_STRING, IDM_EDIT_TITLECASE, L"&Title Case");
	AppendMenuW(hEditMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hCaseMenu), L"Case Conversion");

	HMENU hLineMenu = CreatePopupMenu();
	AppendMenuW(hLineMenu, MF_STRING, IDM_EDIT_DUP_LINE, L"&Duplicate Line\tCtrl+D");
	AppendMenuW(hLineMenu, MF_STRING, IDM_EDIT_DEL_LINE, L"D&elete Line\tCtrl+Shift+K");
	AppendMenuW(hLineMenu, MF_STRING, IDM_EDIT_MOVEUP, L"Move Line &Up\tAlt+Up");
	AppendMenuW(hLineMenu, MF_STRING, IDM_EDIT_MOVEDOWN, L"Move Line &Down\tAlt+Down");
	AppendMenuW(hLineMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hLineMenu, MF_STRING, IDM_EDIT_SORTASC, L"&Sort Lines (Ascending)");
	AppendMenuW(hLineMenu, MF_STRING, IDM_EDIT_SORTDESC, L"Sort Lines (&Descending)");
	AppendMenuW(hLineMenu, MF_STRING, IDM_EDIT_JOINLINES, L"&Join Lines");
	AppendMenuW(hEditMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hLineMenu), L"Line Operations");

	HMENU hTabSpaceMenu = CreatePopupMenu();
	AppendMenuW(hTabSpaceMenu, MF_STRING, IDM_EDIT_TABS_TO_SPACES, L"&Tabs to Spaces");
	AppendMenuW(hTabSpaceMenu, MF_STRING, IDM_EDIT_SPACES_TO_TABS, L"&Spaces to Tabs");
	AppendMenuW(hEditMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hTabSpaceMenu), L"Tab/Space Conversion");

	AppendMenuW(hEditMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_TOGGLECOMMENT, L"Toggle &Comment\tCtrl+/");
	AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_TRIMTRAILING, L"&Trim Trailing Whitespace");
	AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_REMOVEEMPTY, L"&Remove Empty Lines");
	AppendMenuW(hEditMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_INSERT_DATETIME_SHORT, L"Insert Date/Time (&Short)\tCtrl+Shift+D");
	AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_INSERT_DATETIME_LONG, L"Insert Date/Time (&Long)");

	AppendMenuW(hMenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(hEditMenu), L"&Edit");

	// Search menu
	HMENU hSearchMenu = CreatePopupMenu();
	AppendMenuW(hSearchMenu, MF_STRING, IDM_SEARCH_FIND, L"&Find\tCtrl+F");
	AppendMenuW(hSearchMenu, MF_STRING, IDM_SEARCH_REPLACE, L"Find and &Replace...\tCtrl+H");
	AppendMenuW(hSearchMenu, MF_STRING, IDM_SEARCH_FINDINFILES, L"Find in F&iles...\tCtrl+Shift+F");
	AppendMenuW(hSearchMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hSearchMenu, MF_STRING, IDM_SEARCH_FINDNEXT, L"Find &Next\tF3");
	AppendMenuW(hSearchMenu, MF_STRING, IDM_SEARCH_FINDPREV, L"Find &Previous\tShift+F3");
	AppendMenuW(hSearchMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hSearchMenu, MF_STRING, IDM_SEARCH_GOTOLINE, L"&Go to Line...\tCtrl+G");
	AppendMenuW(hSearchMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hSearchMenu, MF_STRING, IDM_SEARCH_BOOKMARK_TOGGLE, L"Toggle &Bookmark\tCtrl+F2");
	AppendMenuW(hSearchMenu, MF_STRING, IDM_SEARCH_BOOKMARK_NEXT, L"Ne&xt Bookmark\tF2");
	AppendMenuW(hSearchMenu, MF_STRING, IDM_SEARCH_BOOKMARK_PREV, L"Pre&vious Bookmark\tShift+F2");
	AppendMenuW(hSearchMenu, MF_STRING, IDM_SEARCH_BOOKMARK_CLEARALL, L"Clear All Book&marks");
	AppendMenuW(hSearchMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hSearchMenu, MF_STRING, IDM_SEARCH_GOTOMATCHINGBRACE, L"Go to Matching &Brace\tCtrl+B");
	AppendMenuW(hMenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(hSearchMenu), L"&Search");

	// View menu
	HMENU hViewMenu = CreatePopupMenu();
	AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_WORDWRAP, L"&Word Wrap");
	AppendMenuW(hViewMenu, MF_STRING | (ctx().showLineNumbers ? MF_CHECKED : MF_UNCHECKED), IDM_VIEW_LINENUMBER, L"&Line Numbers");
	AppendMenuW(hViewMenu, MF_STRING | (ctx().showWhitespace ? MF_CHECKED : MF_UNCHECKED), IDM_VIEW_SHOW_WS, L"Show &Whitespace && TAB");
	AppendMenuW(hViewMenu, MF_STRING | (ctx().showEol ? MF_CHECKED : MF_UNCHECKED), IDM_VIEW_SHOW_EOL, L"Show &End of Line");
	AppendMenuW(hViewMenu, MF_STRING | (ctx().showIndentGuides ? MF_CHECKED : MF_UNCHECKED), IDM_VIEW_SHOW_INDENT, L"Show &Indent Guides");
	AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_FOLDALL, L"&Fold All");
	AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_UNFOLDALL, L"&Unfold All");
	AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_SPLIT, L"&Split View");
	AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_UNSPLIT, L"&Unsplit View");
	AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_MOVETOOTHER, L"&Move to Other View");
	AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_CLONETOOTHER, L"&Clone to Other View");
	AppendMenuW(hViewMenu, MF_STRING | (ctx().syncScrolling ? MF_CHECKED : MF_UNCHECKED) |
	            (ctx().isSplit ? MF_ENABLED : MF_GRAYED),
	            IDM_VIEW_SYNCHRONIZE_SCROLLING, L"Synchronize Scrolling");
	AppendMenuW(hViewMenu, MF_STRING | (ctx().documentMapEnabled ? MF_CHECKED : MF_UNCHECKED),
	            IDM_VIEW_DOCUMENTMAP, L"&Document Map");
	AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_ZOOMIN, L"Zoom &In\tCtrl+=");
	AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_ZOOMOUT, L"Zoom &Out\tCtrl+-");
	AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_ZOOMRESTORE, L"&Reset Zoom\tCtrl+0");
	AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_TOOLBAR, L"Show/Hide &Toolbar\tCtrl+Alt+T");
	AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_PREFERENCES, L"&Preferences...\tCtrl+,");
	AppendMenuW(hMenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(hViewMenu), L"&View");

	// Format menu
	HMENU hFormatMenu = CreatePopupMenu();

	HMENU hEolMenu = CreatePopupMenu();
	AppendMenuW(hEolMenu, MF_STRING, IDM_FORMAT_EOL_LF, L"Unix (LF)");
	AppendMenuW(hEolMenu, MF_STRING, IDM_FORMAT_EOL_CRLF, L"Windows (CR LF)");
	AppendMenuW(hEolMenu, MF_STRING, IDM_FORMAT_EOL_CR, L"Mac Classic (CR)");
	AppendMenuW(hFormatMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hEolMenu), L"&EOL Conversion");

	HMENU hEncMenu = CreatePopupMenu();
	AppendMenuW(hEncMenu, MF_STRING, IDM_FORMAT_ENC_UTF8, L"UTF-8");
	AppendMenuW(hEncMenu, MF_STRING, IDM_FORMAT_ENC_UTF8BOM, L"UTF-8 BOM");
	AppendMenuW(hEncMenu, MF_STRING, IDM_FORMAT_ENC_UTF16LE, L"UTF-16 LE");
	AppendMenuW(hEncMenu, MF_STRING, IDM_FORMAT_ENC_UTF16BE, L"UTF-16 BE");
	AppendMenuW(hEncMenu, MF_STRING, IDM_FORMAT_ENC_ANSI, L"ANSI");
	AppendMenuW(hFormatMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hEncMenu), L"&Encoding");

	AppendMenuW(hMenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(hFormatMenu), L"F&ormat");

	// Language menu
	HMENU hLangMenu = CreatePopupMenu();
	for (int i = 0; i < g_numLanguages; ++i)
	{
		NSString* name = [NSString stringWithUTF8String:g_languages[i].name];
		std::wstring wname = NSStringToWide(name);
		AppendMenuW(hLangMenu, MF_STRING, g_languages[i].menuId, wname.c_str());
	}
	AppendMenuW(hMenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(hLangMenu), L"&Language");

	// Help menu
	HMENU hHelpMenu = CreatePopupMenu();
	AppendMenuW(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, L"About MacNote++");
	AppendMenuW(hMenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(hHelpMenu), L"&Help");

	return hMenuBar;
}
