#pragma once
// Win32 Shim: Common Dialog Boxes for macOS
// Open/Save file dialogs, font/color chooser, etc.

#include "windef.h"
#include "wingdi.h"

// Hook proc types
typedef UINT_PTR (CALLBACK* LPOFNHOOKPROC)(HWND, UINT, WPARAM, LPARAM);
typedef UINT_PTR (CALLBACK* LPCCHOOKPROC)(HWND, UINT, WPARAM, LPARAM);
typedef UINT_PTR (CALLBACK* LPCFHOOKPROC)(HWND, UINT, WPARAM, LPARAM);
typedef UINT_PTR (CALLBACK* LPPRINTHOOKPROC)(HWND, UINT, WPARAM, LPARAM);
typedef UINT_PTR (CALLBACK* LPSETUPHOOKPROC)(HWND, UINT, WPARAM, LPARAM);
typedef UINT_PTR (CALLBACK* LPFRHOOKPROC)(HWND, UINT, WPARAM, LPARAM);

// ============================================================
// Open/Save File Dialog
// ============================================================
typedef struct tagOFNW {
	DWORD         lStructSize;
	HWND          hwndOwner;
	HINSTANCE     hInstance;
	LPCWSTR       lpstrFilter;
	LPWSTR        lpstrCustomFilter;
	DWORD         nMaxCustFilter;
	DWORD         nFilterIndex;
	LPWSTR        lpstrFile;
	DWORD         nMaxFile;
	LPWSTR        lpstrFileTitle;
	DWORD         nMaxFileTitle;
	LPCWSTR       lpstrInitialDir;
	LPCWSTR       lpstrTitle;
	DWORD         Flags;
	WORD          nFileOffset;
	WORD          nFileExtension;
	LPCWSTR       lpstrDefExt;
	LPARAM        lCustData;
	LPOFNHOOKPROC lpfnHook;
	LPCWSTR       lpTemplateName;
	void*         pvReserved;
	DWORD         dwReserved;
	DWORD         FlagsEx;
} OPENFILENAMEW, *LPOPENFILENAMEW;
typedef OPENFILENAMEW OPENFILENAME;

#define OFN_READONLY             0x00000001
#define OFN_OVERWRITEPROMPT      0x00000002
#define OFN_HIDEREADONLY         0x00000004
#define OFN_NOCHANGEDIR          0x00000008
#define OFN_SHOWHELP             0x00000010
#define OFN_ENABLEHOOK           0x00000020
#define OFN_ENABLETEMPLATE       0x00000040
#define OFN_ENABLETEMPLATEHANDLE 0x00000080
#define OFN_NOVALIDATE           0x00000100
#define OFN_ALLOWMULTISELECT     0x00000200
#define OFN_EXTENSIONDIFFERENT   0x00000400
#define OFN_PATHMUSTEXIST        0x00000800
#define OFN_FILEMUSTEXIST        0x00001000
#define OFN_CREATEPROMPT         0x00002000
#define OFN_SHAREAWARE           0x00004000
#define OFN_NOREADONLYRETURN     0x00008000
#define OFN_NOTESTFILECREATE     0x00010000
#define OFN_NONETWORKBUTTON      0x00020000
#define OFN_NOLONGNAMES          0x00040000
#define OFN_EXPLORER             0x00080000
#define OFN_NODEREFERENCELINKS   0x00100000
#define OFN_LONGNAMES            0x00200000
#define OFN_ENABLEINCLUDENOTIFY  0x00400000
#define OFN_ENABLESIZING         0x00800000
#define OFN_DONTADDTORECENT      0x02000000
#define OFN_FORCESHOWHIDDEN      0x10000000

BOOL GetOpenFileNameW(LPOPENFILENAMEW lpofn);
BOOL GetSaveFileNameW(LPOPENFILENAMEW lpofn);
#define GetOpenFileName GetOpenFileNameW
#define GetSaveFileName GetSaveFileNameW

// ============================================================
// Color chooser
// ============================================================
typedef struct tagCHOOSECOLORW {
	DWORD       lStructSize;
	HWND        hwndOwner;
	HWND        hInstance;
	COLORREF    rgbResult;
	COLORREF*   lpCustColors;
	DWORD       Flags;
	LPARAM      lCustData;
	LPCCHOOKPROC lpfnHook;
	LPCWSTR     lpTemplateName;
} CHOOSECOLORW, *LPCHOOSECOLORW;
typedef CHOOSECOLORW CHOOSECOLOR;

#define CC_RGBINIT         0x00000001
#define CC_FULLOPEN        0x00000002
#define CC_PREVENTFULLOPEN 0x00000004
#define CC_SHOWHELP        0x00000008
#define CC_ENABLEHOOK      0x00000010
#define CC_ENABLETEMPLATE  0x00000020
#define CC_ANYCOLOR        0x00000100
#define CC_SOLIDCOLOR      0x00000080

BOOL ChooseColorW(LPCHOOSECOLORW lpcc);
#define ChooseColor ChooseColorW

// ============================================================
// Font chooser
// ============================================================
typedef struct tagCHOOSEFONTW {
	DWORD        lStructSize;
	HWND         hwndOwner;
	HDC          hDC;
	LOGFONTW*    lpLogFont;
	INT          iPointSize;
	DWORD        Flags;
	COLORREF     rgbColors;
	LPARAM       lCustData;
	LPCFHOOKPROC lpfnHook;
	LPCWSTR      lpTemplateName;
	HINSTANCE    hInstance;
	LPWSTR       lpszStyle;
	WORD         nFontType;
	INT          nSizeMin;
	INT          nSizeMax;
} CHOOSEFONTW, *LPCHOOSEFONTW;
typedef CHOOSEFONTW CHOOSEFONT;

#define CF_SCREENFONTS      0x00000001
#define CF_PRINTERFONTS     0x00000002
#define CF_BOTH             (CF_SCREENFONTS | CF_PRINTERFONTS)
#define CF_SHOWHELP         0x00000004
#define CF_ENABLEHOOK       0x00000008
#define CF_ENABLETEMPLATE   0x00000010
#define CF_INITTOLOGFONTSTRUCT 0x00000040
#define CF_USESTYLE         0x00000080
#define CF_EFFECTS          0x00000100
#define CF_APPLY            0x00000200
#define CF_ANSIONLY         0x00000400
#define CF_NOVECTORFONTS    0x00000800
#define CF_NOSIMULATIONS    0x00001000
#define CF_LIMITSIZE        0x00002000
#define CF_FIXEDPITCHONLY   0x00004000
#define CF_WYSIWYG          0x00008000
#define CF_FORCEFONTEXIST   0x00010000
#define CF_SCALABLEONLY     0x00020000
#define CF_TTONLY           0x00040000
#define CF_NOFACESEL        0x00080000
#define CF_NOSTYLESEL       0x00100000
#define CF_NOSIZESEL        0x00200000
#define CF_SELECTSCRIPT     0x00400000
#define CF_NOSCRIPTSEL      0x00800000
#define CF_NOVERTFONTS      0x01000000

BOOL ChooseFontW(LPCHOOSEFONTW lpcf);
#define ChooseFont ChooseFontW

// ============================================================
// Print dialog (stub)
// ============================================================
typedef struct tagPDW {
	DWORD    lStructSize;
	HWND     hwndOwner;
	HGLOBAL  hDevMode;
	HGLOBAL  hDevNames;
	HDC      hDC;
	DWORD    Flags;
	WORD     nFromPage;
	WORD     nToPage;
	WORD     nMinPage;
	WORD     nMaxPage;
	WORD     nCopies;
	HINSTANCE hInstance;
	LPARAM   lCustData;
	LPPRINTHOOKPROC lpfnPrintHook;
	LPSETUPHOOKPROC lpfnSetupHook;
	LPCWSTR  lpPrintTemplateName;
	LPCWSTR  lpSetupTemplateName;
	HGLOBAL  hPrintTemplate;
	HGLOBAL  hSetupTemplate;
} PRINTDLGW, *LPPRINTDLGW;
typedef PRINTDLGW PRINTDLG;

#define PD_ALLPAGES                  0x00000000
#define PD_SELECTION                 0x00000001
#define PD_PAGENUMS                  0x00000002
#define PD_NOSELECTION               0x00000004
#define PD_NOPAGENUMS                0x00000008
#define PD_COLLATE                   0x00000010
#define PD_PRINTTOFILE               0x00000020
#define PD_PRINTSETUP                0x00000040
#define PD_NOWARNING                 0x00000080
#define PD_RETURNDC                  0x00000100
#define PD_RETURNIC                  0x00000200
#define PD_RETURNDEFAULT             0x00000400
#define PD_SHOWHELP                  0x00000800
#define PD_ENABLEPRINTHOOK           0x00001000
#define PD_ENABLESETUPHOOK           0x00002000
#define PD_ENABLEPRINTTEMPLATE       0x00004000
#define PD_ENABLESETUPTEMPLATE       0x00008000
#define PD_ENABLEPRINTTEMPLATEHANDLE 0x00010000
#define PD_ENABLESETUPTEMPLATEHANDLE 0x00020000
#define PD_USEDEVMODECOPIESANDCOLLATE 0x00040000
#define PD_USEDEVMODECOPIES          PD_USEDEVMODECOPIESANDCOLLATE
#define PD_DISABLEPRINTTOFILE        0x00080000
#define PD_HIDEPRINTTOFILE           0x00100000
#define PD_NONETWORKBUTTON           0x00200000

BOOL PrintDlgW(LPPRINTDLGW lppd);
#define PrintDlg PrintDlgW

// ============================================================
// FindText / ReplaceText (stubs)
// ============================================================
typedef struct tagFINDREPLACEW {
	DWORD    lStructSize;
	HWND     hwndOwner;
	HINSTANCE hInstance;
	DWORD    Flags;
	LPWSTR   lpstrFindWhat;
	LPWSTR   lpstrReplaceWith;
	WORD     wFindWhatLen;
	WORD     wReplaceWithLen;
	LPARAM   lCustData;
	LPFRHOOKPROC lpfnHook;
	LPCWSTR  lpTemplateName;
} FINDREPLACEW, *LPFINDREPLACEW;
typedef FINDREPLACEW FINDREPLACE;

#define FR_DOWN       0x00000001
#define FR_WHOLEWORD  0x00000002
#define FR_MATCHCASE  0x00000004
#define FR_FINDNEXT   0x00000008
#define FR_REPLACE    0x00000010
#define FR_REPLACEALL 0x00000020
#define FR_DIALOGTERM 0x00000040

HWND FindTextW(LPFINDREPLACEW lpfr);
HWND ReplaceTextW(LPFINDREPLACEW lpfr);
#define FindText FindTextW
#define ReplaceText ReplaceTextW

// CommDlg_OpenSave_GetSpec and similar macros
#define CommDlg_OpenSave_GetSpecW(hdlg, psz, cbmax) 0
#define CommDlg_OpenSave_GetSpec CommDlg_OpenSave_GetSpecW

DWORD CommDlgExtendedError();
