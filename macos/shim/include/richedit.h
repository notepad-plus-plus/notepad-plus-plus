#pragma once
// Win32 Shim: Rich Edit Control definitions for macOS (stubs)
// Notepad++ uses some richedit constants for Scintilla interaction

#include "windef.h"
#include "wingdi.h"

// Rich edit class names
#define RICHEDIT_CLASSW L"RichEdit50W"
#define RICHEDIT_CLASS  RICHEDIT_CLASSW

// Rich edit messages
#define EM_CANPASTE          0x0432
#define EM_DISPLAYBAND       0x0433
#define EM_EXGETSEL          0x0434
#define EM_EXLIMITTEXT       0x0435
#define EM_EXLINEFROMCHAR    0x0436
#define EM_EXSETSEL          0x0437
#define EM_FINDTEXT          0x0438
#define EM_FORMATRANGE       0x0439
#define EM_GETCHARFORMAT     0x043A
#define EM_GETEVENTMASK      0x043B
#define EM_GETOLEINTERFACE   0x043C
#define EM_GETPARAFORMAT     0x043D
#define EM_GETSELTEXT        0x043E
#define EM_HIDESELECTION     0x043F
#define EM_PASTESPECIAL      0x0440
#define EM_REQUESTRESIZE     0x0441
#define EM_SELECTIONTYPE     0x0442
#define EM_SETBKGNDCOLOR     0x0443
#define EM_SETCHARFORMAT     0x0444
#define EM_SETEVENTMASK      0x0445
#define EM_SETOLECALLBACK    0x0446
#define EM_SETPARAFORMAT     0x0447
#define EM_SETTARGETDEVICE   0x0448
#define EM_STREAMIN          0x0449
#define EM_STREAMOUT         0x044A
#define EM_GETTEXTRANGE      0x044B
#define EM_FINDWORDBREAK     0x044C
#define EM_SETOPTIONS        0x044D
#define EM_GETOPTIONS        0x044E
#define EM_FINDTEXTEX        0x044F
#define EM_GETWORDBREAKPROCEX 0x0450
#define EM_SETWORDBREAKPROCEX 0x0451
#define EM_SETUNDOLIMIT      0x0452
#define EM_REDO              0x0454
#define EM_CANREDO           0x0455
#define EM_GETUNDONAME       0x0456
#define EM_GETREDONAME       0x0457
#define EM_STOPGROUPTYPING   0x0458
#define EM_SETTEXTMODE       0x0459
#define EM_GETTEXTMODE       0x045A
#define EM_AUTOURLDETECT     0x045B
#define EM_GETZOOM           0x04E0
#define EM_SETZOOM           0x04E1

// Character format
#define CFM_BOLD      0x00000001
#define CFM_ITALIC    0x00000002
#define CFM_UNDERLINE 0x00000004
#define CFM_STRIKEOUT 0x00000008
#define CFM_PROTECTED 0x00000010
#define CFM_LINK      0x00000020
#define CFM_SIZE      0x80000000
#define CFM_COLOR     0x40000000
#define CFM_FACE      0x20000000
#define CFM_OFFSET    0x10000000
#define CFM_CHARSET   0x08000000

#define CFE_BOLD      0x0001
#define CFE_ITALIC    0x0002
#define CFE_UNDERLINE 0x0004
#define CFE_STRIKEOUT 0x0008
#define CFE_PROTECTED 0x0010
#define CFE_LINK      0x0020
#define CFE_AUTOCOLOR 0x40000000

// Event masks
#define ENM_CHANGE     0x00000001
#define ENM_UPDATE     0x00000002
#define ENM_SCROLL     0x00000004
#define ENM_SCROLLEVENTS 0x00000008
#define ENM_DRAGDROPDONE 0x00000010
#define ENM_PARAGRAPHEXPANDED 0x00000020
#define ENM_PAGECHANGE 0x00000040
#define ENM_CLIPFORMAT 0x00000080
#define ENM_KEYEVENTS  0x00010000
#define ENM_MOUSEEVENTS 0x00020000
#define ENM_REQUESTRESIZE 0x00040000
#define ENM_SELCHANGE  0x00080000
#define ENM_DROPFILES  0x00100000
#define ENM_PROTECTED  0x00200000
#define ENM_LINK       0x04000000

// CHARRANGE
typedef struct _charrange {
	LONG cpMin;
	LONG cpMax;
} CHARRANGE;

// TEXTRANGE
typedef struct _textrangew {
	CHARRANGE chrg;
	LPWSTR    lpstrText;
} TEXTRANGEW;
typedef TEXTRANGEW TEXTRANGE;

// CHARFORMAT
typedef struct _charformatw {
	UINT  cbSize;
	DWORD dwMask;
	DWORD dwEffects;
	LONG  yHeight;
	LONG  yOffset;
	COLORREF crTextColor;
	BYTE  bCharSet;
	BYTE  bPitchAndFamily;
	WCHAR szFaceName[LF_FACESIZE];
} CHARFORMATW;
typedef CHARFORMATW CHARFORMAT;

// CHARFORMAT2
typedef struct _charformat2w {
	UINT  cbSize;
	DWORD dwMask;
	DWORD dwEffects;
	LONG  yHeight;
	LONG  yOffset;
	COLORREF crTextColor;
	BYTE  bCharSet;
	BYTE  bPitchAndFamily;
	WCHAR szFaceName[LF_FACESIZE];
	WORD  wWeight;
	SHORT sSpacing;
	COLORREF crBackColor;
	DWORD lcid;
	DWORD dwReserved;
	SHORT sStyle;
	WORD  wKerning;
	BYTE  bUnderlineType;
	BYTE  bAnimation;
	BYTE  bRevAuthor;
	BYTE  bReserved1;
} CHARFORMAT2W;
typedef CHARFORMAT2W CHARFORMAT2;

// Rich edit notifications
#define EN_MSGFILTER  0x0700
#define EN_REQUESTRESIZE 0x0701
#define EN_SELCHANGE  0x0702
#define EN_DROPFILES_RE  0x0703
#define EN_PROTECTED  0x0704
#define EN_CORRECTTEXT 0x0705
#define EN_STOPNOUNDO 0x0706
#define EN_IMECHANGE  0x0707
#define EN_SAVECLIPBOARD 0x0708
#define EN_OLEOPFAILED 0x0709
#define EN_OBJECTPOSITIONS 0x070A
#define EN_LINK       0x070B
#define EN_DRAGDROPDONE 0x070C
#define EN_PARAGRAPHEXPANDED 0x070D
#define EN_PAGECHANGE 0x070E
#define EN_LOWFIRTF   0x070F
#define EN_ALIGNLTR   0x0710
#define EN_ALIGNRTL   0x0711
