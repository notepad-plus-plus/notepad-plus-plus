// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

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


#pragma once


#include "Scintilla.h"
#include "ScintillaRef.h"
#include "SciLexer.h"
#include "Buffer.h"
#include "colors.h"
#include "UserDefineDialog.h"
#include "rgba_icons.h"


#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif //WM_MOUSEWHEEL

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif //WM_MOUSEHWHEEL

#ifndef WM_APPCOMMAND
#define WM_APPCOMMAND                   0x0319
#define APPCOMMAND_BROWSER_BACKWARD       1
#define APPCOMMAND_BROWSER_FORWARD        2
#define FAPPCOMMAND_MASK  0xF000
#define GET_APPCOMMAND_LPARAM(lParam) ((short)(HIWORD(lParam) & ~FAPPCOMMAND_MASK))
#endif //WM_APPCOMMAND

class NppParameters;

#define NB_WORD_LIST 4
#define WORD_LIST_LEN 256

typedef sptr_t(*SCINTILLA_FUNC) (void *, unsigned int, uptr_t, sptr_t);
typedef void * SCINTILLA_PTR;

#define WM_DOCK_USERDEFINE_DLG      (SCINTILLA_USER + 1)
#define WM_UNDOCK_USERDEFINE_DLG    (SCINTILLA_USER + 2)
#define WM_CLOSE_USERDEFINE_DLG     (SCINTILLA_USER + 3)
#define WM_REMOVE_USERLANG          (SCINTILLA_USER + 4)
#define WM_RENAME_USERLANG          (SCINTILLA_USER + 5)
#define WM_REPLACEALL_INOPENEDDOC   (SCINTILLA_USER + 6)
#define WM_FINDALL_INOPENEDDOC      (SCINTILLA_USER + 7)
#define WM_DOOPEN                   (SCINTILLA_USER + 8)
#define WM_FINDINFILES              (SCINTILLA_USER + 9)
#define WM_REPLACEINFILES           (SCINTILLA_USER + 10)
#define WM_FINDALL_INCURRENTDOC     (SCINTILLA_USER + 11)
#define WM_FRSAVE_INT               (SCINTILLA_USER + 12)
#define WM_FRSAVE_STR               (SCINTILLA_USER + 13)
#define WM_FINDALL_INCURRENTFINDER  (SCINTILLA_USER + 14)
#define WM_FINDINPROJECTS           (SCINTILLA_USER + 15)
#define WM_REPLACEINPROJECTS        (SCINTILLA_USER + 16)

// Codepage
const int CP_CHINESE_TRADITIONAL = 950;
const int CP_CHINESE_SIMPLIFIED = 936;
const int CP_JAPANESE = 932;
const int CP_KOREAN = 949;
const int CP_GREEK = 1253;

//wordList
#define LIST_NONE 0
#define LIST_0 1
#define LIST_1 2
#define LIST_2 4
#define LIST_3 8
#define LIST_4 16
#define LIST_5 32
#define LIST_6 64
#define LIST_7 128
#define LIST_8 256


const bool fold_expand = true;
const bool fold_collapse = false;

const int NB_FOLDER_STATE = 7;
#define MAX_FOLD_COLLAPSE_LEVEL	8
#define MAX_FOLD_LINES_MORE_THAN 99

#define MODEVENTMASK_OFF 0

enum TextCase : UCHAR
{
	UPPERCASE,
	LOWERCASE,
	PROPERCASE_FORCE,
	PROPERCASE_BLEND,
	SENTENCECASE_FORCE,
	SENTENCECASE_BLEND,
	INVERTCASE,
	RANDOMCASE
};

const UCHAR MASK_FORMAT = 0x03;
const UCHAR BASE_10 = 0x00; // Dec
const UCHAR BASE_16 = 0x01; // Hex
const UCHAR BASE_08 = 0x02; // Oct
const UCHAR BASE_02 = 0x03; // Bin


const int MARK_BOOKMARK = 20;
const int MARK_HIDELINESBEGIN = 19;
const int MARK_HIDELINESEND = 18;
// 20 - 18 reserved for Notepad++ internal used
// 17 - 0  are free to use for plugins

constexpr char g_ZWSP[] = "\xE2\x80\x8B";

const std::vector<std::vector<const char*>> g_ccUniEolChars =
{
	// C0
	{"\x00", "NUL", "U+0000"},               // U+0000 : Null
	{"\x01", "SOH", "U+0001"},               // U+0001 : Start of Heading
	{"\x02", "STX", "U+0002"},               // U+0002 : Start of Text
	{"\x03", "ETX", "U+0003"},               // U+0003 : End of Text
	{"\x04", "EOT", "U+0004"},               // U+0004 : End of Transmission
	{"\x05", "ENQ", "U+0005"},               // U+0005 : Enquiry
	{"\x06", "ACK", "U+0006"},               // U+0006 : Acknowledge
	{"\a", "BEL", "U+0007"},                 // U+0007 : Bell
	{"\b", "BS", "U+0008"},                  // U+0008 : Backspace
	{"\v", "VT", "U+000B"},                  // U+000B : Line Tabulation
	{"\f", "FF", "U+000C"},                  // U+000C : Form Feed
	{"\x0E", "SO", "U+000E"},                // U+000E : Shift Out
	{"\x0F", "SI", "U+000F"},                // U+000F : Shift In
	{"\x10", "DLE", "U+0010"},               // U+0010 : Data Link Escape
	{"\x11", "DC1", "U+0011"},               // U+0011 : Device Control One
	{"\x12", "DC2", "U+0012"},               // U+0012 : Device Control Two
	{"\x13", "DC3", "U+0013"},               // U+0013 : Device Control Three
	{"\x14", "DC4", "U+0014"},               // U+0014 : Device Control Four
	{"\x15", "NAK", "U+0015"},               // U+0015 : Negative Acknowledge
	{"\x16", "SYN", "U+0016"},               // U+0016 : Synchronous Idle
	{"\x17", "ETB", "U+0017"},               // U+0017 : End of Transmission Block
	{"\x18", "CAN", "U+0018"},               // U+0018 : Cancel
	{"\x19", "EM", "U+0019"},                // U+0019 : End of Medium
	{"\x1A", "SUB", "U+001A"},               // U+001A : Substitute
	{"\x1B", "ESC", "U+001B"},               // U+001B : Escape
	{"\x1C", "FS", "U+001C"},                // U+001C : Information Separator Four
	{"\x1D", "GS", "U+001D"},                // U+001D : Information Separator Three
	{"\x1E", "RS", "U+001E"},                // U+001E : Information Separator Two
	{"\x1F", "US", "U+001F"},                // U+001F : Information Separator One
	{"\x7F", "DEL", "U+007F"},               // U+007F : Delete
	// C1
	{"\xC2\x80", "PAD", "U+0080"},           // U+0080 : Padding Character
	{"\xC2\x81", "HOP", "U+0081"},           // U+0081 : High Octet Preset
	{"\xC2\x82", "BPH", "U+0082"},           // U+0082 : Break Permitted Here
	{"\xC2\x83", "NBH", "U+0083"},           // U+0083 : No Break Here
	{"\xC2\x84", "IND", "U+0084"},           // U+0084 : Index
	//{"\xC2\x85", "NEL", "U+0085"},          // U+0085 : Next Line
	{"\xC2\x86", "SSA", "U+0086"},           // U+0086 : Start of Selected Area
	{"\xC2\x87", "ESA", "U+0087"},           // U+0087 : End of Selected Area
	{"\xC2\x88", "HTS", "U+0088"},           // U+0088 : Character (Horizontal) Tabulation Set
	{"\xC2\x89", "HTJ", "U+0089"},           // U+0089 : Character (Horizontal) Tabulation With Justification
	{"\xC2\x8A", "VTS", "U+008A"},           // U+008A : Vertical (Line) Tabulation Set
	{"\xC2\x8B", "PLD", "U+008B"},           // U+008B : Partial Line Forward (Down)
	{"\xC2\x8C", "PLU", "U+008C"},           // U+008C : Partial Line Backward (Up)
	{"\xC2\x8D", "RI", "U+008D"},            // U+008D : Reverse Line Feed (Index)
	{"\xC2\x8E", "SS2", "U+008E"},           // U+008E : Single-Shift Two
	{"\xC2\x8F", "SS3", "U+008F"},           // U+008F : Single-Shift Three
	{"\xC2\x90", "DCS", "U+0090"},           // U+0090 : Device Control String
	{"\xC2\x91", "PU1", "U+0091"},           // U+0091 : Private Use One
	{"\xC2\x92", "PU2", "U+0092"},           // U+0092 : Private Use Two
	{"\xC2\x93", "STS", "U+0093"},           // U+0093 : Set Transmit State
	{"\xC2\x94", "CCH", "U+0094"},           // U+0094 : Cancel Character
	{"\xC2\x95", "MW", "U+0095"},            // U+0095 : Message Waiting
	{"\xC2\x96", "SPA", "U+0096"},           // U+0096 : Start of Protected Area
	{"\xC2\x97", "EPA", "U+0097"},           // U+0097 : End of Protected Area
	{"\xC2\x98", "SOS", "U+0098"},           // U+0098 : Start of String
	{"\xC2\x99", "SGCI", "U+0099"},          // U+0099 : Single Graphic Character Introducer
	{"\xC2\x9A", "SCI", "U+009A"},           // U+009A : Single Character Introducer
	{"\xC2\x9B", "CSI", "U+009B"},           // U+009B : Control Sequence Introducer
	{"\xC2\x9C", "ST", "U+009C"},            // U+009C : String Terminator
	{"\xC2\x9D", "OSC", "U+009D"},           // U+009D : Operating System Command
	{"\xC2\x9E", "PM", "U+009E"},            // U+009E : Private Message
	{"\xC2\x9F", "APC", "U+009F"},           // U+009F : Application Program Command
	// Unicode EOL
	{"\xC2\x85", "NEL", "U+0085"},           // U+0085 : Next Line
	{"\xE2\x80\xA8", "LS", "U+2028"},        // U+2028 : Line Separator
	{"\xE2\x80\xA9", "PS", "U+2029"}         // U+2029 : Paragraph Separator
};

const std::vector<std::vector<const char*>> g_nonPrintingChars =
{
	{"\xC2\xA0", "NBSP", "U+00A0"},          // U+00A0 : no-break space
	{"\xC2\xAD", "SHY", "U+00AD"},           // U+00AD : soft hyphen
	{"\xD8\x9C", "ALM", "U+061C"},           // U+061C : arabic letter mark
	{"\xDC\x8F", "SAM", "U+070F"},           // U+070F : syriac abbreviation mark
	{"\xE1\x9A\x80", "OSPM", "U+1680"},      // U+1680 : ogham space mark
	{"\xE1\xA0\x8E", "MVS", "U+180E"},       // U+180E : mongolian vowel separator
	{"\xE2\x80\x80", "NQSP", "U+2000"},      // U+2000 : en quad
	{"\xE2\x80\x81", "MQSP", "U+2001"},      // U+2001 : em quad
	{"\xE2\x80\x82", "ENSP", "U+2002"},      // U+2002 : en space
	{"\xE2\x80\x83", "EMSP", "U+2003"},      // U+2003 : em space
	{"\xE2\x80\x84", "3/MSP", "U+2004"},     // U+2004 : three-per-em space
	{"\xE2\x80\x85", "4/MSP", "U+2005"},     // U+2005 : four-per-em space
	{"\xE2\x80\x86", "6/MSP", "U+2006"},     // U+2006 : six-per-em space
	{"\xE2\x80\x87", "FSP", "U+2007"},       // U+2007 : figure space
	{"\xE2\x80\x88", "PSP", "U+2008"},       // U+2008 : punctation space
	{"\xE2\x80\x89", "THSP", "U+2009"},      // U+2009 : thin space
	{"\xE2\x80\x8A", "HSP", "U+200A"},       // U+200A : hair space
	{"\xE2\x80\x8B", "ZWSP", "U+200B"},      // U+200B : zero-width space
	{"\xE2\x80\x8C", "ZWNJ", "U+200C"},      // U+200C : zero-width non-joiner
	{"\xE2\x80\x8D", "ZWJ", "U+200D"},       // U+200D : zero-width joiner
	{"\xE2\x80\x8E", "LRM", "U+200E"},       // U+200E : left-to-right mark
	{"\xE2\x80\x8F", "RLM", "U+200F"},       // U+200F : right-to-left mark
	{"\xE2\x80\xAA", "LRE", "U+202A"},       // U+202A : left-to-right embedding
	{"\xE2\x80\xAB", "RLE", "U+202B"},       // U+202B : right-to-left embedding
	{"\xE2\x80\xAC", "PDF", "U+202C"},       // U+202C : pop directional formatting
	{"\xE2\x80\xAD", "LRO", "U+202D"},       // U+202D : left-to-right override
	{"\xE2\x80\xAE", "RLO", "U+202E"},       // U+202E : right-to-left override
	{"\xE2\x80\xAF", "NNBSP", "U+202F"},     // U+202F : narrow no-break space
	{"\xE2\x81\x9F", "MMSP", "U+205F"},      // U+205F : medium mathematical space
	{"\xE2\x81\xA0", "WJ", "U+2060"},        // U+2060 : word joiner
	{"\xE2\x81\xA1", "(FA)", "U+2061"},      // U+2061 : function application
	{"\xE2\x81\xA2", "(IT)", "U+2062"},      // U+2062 : invisible times
	{"\xE2\x81\xA3", "(IS)", "U+2063"},      // U+2063 : invisible separator
	{"\xE2\x81\xA4", "(IP)", "U+2064"},      // U+2064 : invisible plus
	{"\xE2\x81\xA6", "LRI", "U+2066"},       // U+2066 : left-to-right isolate
	{"\xE2\x81\xA7", "RLI", "U+2067"},       // U+2067 : right-to-left isolate
	{"\xE2\x81\xA8", "FSI", "U+2068"},       // U+2068 : first strong isolate
	{"\xE2\x81\xA9", "PDI", "U+2069"},       // U+2069 : pop directional isolate
	{"\xE2\x81\xAA", "ISS", "U+206A"},       // U+206A : inhibit symmetric swapping
	{"\xE2\x81\xAB", "ASS", "U+206B"},       // U+206B : activate symmetric swapping
	{"\xE2\x81\xAC", "IAFS", "U+206C"},      // U+206C : inhibit arabic form shaping
	{"\xE2\x81\xAD", "AAFS", "U+206D"},      // U+206D : activate arabic form shaping
	{"\xE2\x81\xAE", "NADS", "U+206E"},      // U+206E : national digit shapes
	{"\xE2\x81\xAF", "NODS", "U+206F"},      // U+206F : nominal digit shapes
	{"\xE3\x80\x80", "IDSP", "U+3000"},      // U+3000 : ideographic space
	{"\xEF\xBB\xBF", "ZWNBSP", "U+FEFF"},    // U+FEFF : zero-width no-break space
	{"\xEF\xBF\xB9", "IAA", "U+FFF9"},       // U+FFF9 : interlinear annotation anchor
	{"\xEF\xBF\xBA", "IAS", "U+FFFA"},       // U+FFFA : interlinear annotation separator
	{"\xEF\xBF\xBB", "IAT", "U+FFFB"}        // U+FFFB : interlinear annotation terminator
};

size_t getNbDigits(size_t aNum, size_t base);

template<typename T>
T* variedFormatNumber2String(T* str, size_t strLen, size_t number, size_t base, size_t nbDigits, ColumnEditorParam::leadingChoice lead)
{
	if (nbDigits == 0 || nbDigits >= strLen) return NULL;

	//
	// Reset the output string
	//
	memset(str, 0, sizeof(T) * strLen);

	//
	// Form number string according its base
	//
	std::string numberStr;

	if (base == 2)
	{
		std::string tmpStr;
		size_t aNum = number;
		do
		{
			tmpStr += aNum % 2 ? "1" : "0";
			aNum = aNum / 2;

		} while (aNum != 0);

		size_t i = 0;
		size_t j = tmpStr.length() - 1;
		for (; j >= 0 && i < tmpStr.length(); i++, j--)
		{
			numberStr += tmpStr[j];
		}
	}
	else if (base == 8)
	{
		std::stringstream stream;
		stream << std::oct << number;
		numberStr = stream.str();
	}
	else if (base == 16)
	{
		std::stringstream stream;
		stream << std::hex << number;
		numberStr = stream.str();
	}
	else //if (base == 10)
	{
		numberStr = std::to_string(number);
	}

	size_t numberStrLen = numberStr.length();
	size_t noneUsedZoneLen = nbDigits - numberStrLen;

	size_t nbStart = 0;
	size_t nbEnd = 0;

	size_t noneUsedStart = 0;
	size_t noneUsedEnd = 0;

	T noUsedSymbol = ' ';

	//
	// Determinate leading zero/space or none
	//
	if (lead == ColumnEditorParam::spaceLeading)
	{
		noneUsedStart = 0;
		noneUsedEnd = nbStart = noneUsedZoneLen;
		nbEnd = nbDigits;
	}
	else if (lead == ColumnEditorParam::zeroLeading)
	{
		noUsedSymbol = '0';

		noneUsedStart = 0;
		noneUsedEnd = nbStart = noneUsedZoneLen;
		nbEnd = nbDigits;
	}
	else //if (lead != ColumnEditorParam::noneLeading)
	{
		nbStart = 0;
		nbEnd = noneUsedStart = numberStrLen;
		noneUsedEnd = nbDigits;
	}

	//
	// Fill str with the correct position
	//
	size_t i = 0;
	for (size_t k = nbStart; k < nbEnd; ++k)
		str[k] = numberStr[i++];

	size_t j = 0;
	for (j = noneUsedStart; j < noneUsedEnd; ++j)
		str[j] = noUsedSymbol;

	return str;
}

typedef LRESULT (WINAPI *CallWindowProcFunc) (WNDPROC,HWND,UINT,WPARAM,LPARAM);

const bool L2R = true;
const bool R2L = false;

struct ColumnModeInfo {
	intptr_t _selLpos = 0;
	intptr_t _selRpos = 0;
	intptr_t _order = -1; // 0 based index
	bool _direction = L2R; // L2R or R2L
	intptr_t _nbVirtualAnchorSpc = 0;
	intptr_t _nbVirtualCaretSpc = 0;

	ColumnModeInfo(intptr_t lPos, intptr_t rPos, intptr_t order, bool dir = L2R, intptr_t vAnchorNbSpc = 0, intptr_t vCaretNbSpc = 0)
		: _selLpos(lPos), _selRpos(rPos), _order(order), _direction(dir), _nbVirtualAnchorSpc(vAnchorNbSpc), _nbVirtualCaretSpc(vCaretNbSpc){};

	bool isValid() const {
		return (_order >= 0 && _selLpos >= 0 && _selRpos >= 0 && _selLpos <= _selRpos);
	};
};

//
// SortClass for vector<ColumnModeInfo>
// sort in _order : increased order
struct SortInSelectOrder {
	bool operator() (const ColumnModeInfo & l, const ColumnModeInfo & r) {
		return (l._order < r._order);
	}
};

//
// SortClass for vector<ColumnModeInfo>
// sort in _selLpos : increased order
struct SortInPositionOrder {
	bool operator() (const ColumnModeInfo & l, const ColumnModeInfo & r) {
		return (l._selLpos < r._selLpos);
	}
};

typedef std::vector<ColumnModeInfo> ColumnModeInfos;

struct LanguageNameInfo {
	const wchar_t* _langName = nullptr;
	const wchar_t* _shortName = nullptr;
	const wchar_t* _longName = nullptr;
	LangType _langID = L_TEXT;
	const char* _lexerID = nullptr;
};

#define URL_INDIC 8
class ISorter;

class ScintillaEditView : public Window
{
friend class Finder;
public:
	ScintillaEditView(): Window() {
		++_refCount;
	};
	
	ScintillaEditView(bool isMainEditZone) : Window() {
		_isMainEditZone = isMainEditZone;
		++_refCount;
	};

	virtual ~ScintillaEditView()
	{
		--_refCount;

		if ((!_refCount)&&(_SciInit))
		{
			Scintilla_ReleaseResources();

			for (BufferStyleMap::iterator it(_hotspotStyles.begin()); it != _hotspotStyles.end(); ++it )
			{
				delete it->second;
			}
		}
	};

	virtual void destroy()
	{
		if (_blankDocument != 0)
		{
			execute(SCI_RELEASEDOCUMENT, 0, _blankDocument);
			_blankDocument = 0;
		}
		::DestroyWindow(_hSelf);
		_hSelf = NULL;
		_pScintillaFunc = NULL;
	};

	virtual void init(HINSTANCE hInst, HWND hPere);

	LRESULT execute(UINT Msg, WPARAM wParam=0, LPARAM lParam=0) const {
		try {
			return (_pScintillaFunc) ? _pScintillaFunc(_pScintillaPtr, Msg, wParam, lParam) : -1;
		}
		catch (...)
		{
			return -1;
		}
	};

	void activateBuffer(BufferID buffer, bool force);



	void getText(char *dest, size_t start, size_t end) const;
	void getGenericText(wchar_t *dest, size_t destlen, size_t start, size_t end) const;
	void getGenericText(wchar_t *dest, size_t deslen, size_t start, size_t end, intptr_t* mstart, intptr_t* mend) const;
	std::wstring getGenericTextAsString(size_t start, size_t end) const;
	void insertGenericTextFrom(size_t position, const wchar_t *text2insert) const;
	void replaceSelWith(const char * replaceText);

	intptr_t getSelectedTextCount() {
		Sci_CharacterRangeFull range = getSelection();
		return (range.cpMax - range.cpMin);
	};

	void getVisibleStartAndEndPosition(intptr_t* startPos, intptr_t* endPos);
    char * getWordFromRange(char * txt, size_t size, size_t pos1, size_t pos2);
	char * getSelectedText(char * txt, size_t size, bool expand = true);
    char * getWordOnCaretPos(char * txt, size_t size);
    wchar_t * getGenericWordOnCaretPos(wchar_t * txt, int size);
	wchar_t * getGenericSelectedText(wchar_t * txt, int size, bool expand = true);
	intptr_t searchInTarget(const wchar_t * Text2Find, size_t lenOfText2Find, size_t fromPos, size_t toPos) const;
	void appandGenericText(const wchar_t * text2Append) const;
	void addGenericText(const wchar_t * text2Append) const;
	void addGenericText(const wchar_t * text2Append, intptr_t* mstart, intptr_t* mend) const;
	intptr_t replaceTarget(const wchar_t * str2replace, intptr_t fromTargetPos = -1, intptr_t toTargetPos = -1) const;
	intptr_t replaceTargetRegExMode(const wchar_t * re, intptr_t fromTargetPos = -1, intptr_t toTargetPos = -1) const;
	void showAutoComletion(size_t lenEntered, const wchar_t * list);
	void showCallTip(size_t startPos, const wchar_t * def);
	std::wstring getLine(size_t lineNumber) const;
	void getLine(size_t lineNumber, wchar_t * line, size_t lineBufferLen) const;
	void addText(size_t length, const char *buf);

	void insertNewLineAboveCurrentLine();
	void insertNewLineBelowCurrentLine();

	void saveCurrentPos();
	void restoreCurrentPosPreStep();
	void restoreCurrentPosPostStep();

	void beginOrEndSelect(bool isColumnMode);
	bool beginEndSelectedIsStarted() const {
		return _beginSelectPosition != -1;
	};

	size_t getCurrentDocLen() const {
		return size_t(execute(SCI_GETLENGTH));
	};

	Sci_CharacterRangeFull getSelection() const {
		Sci_CharacterRangeFull crange{};
		crange.cpMin = execute(SCI_GETSELECTIONSTART);
		crange.cpMax = execute(SCI_GETSELECTIONEND);
		return crange;
	};

	void getWordToCurrentPos(wchar_t * str, intptr_t strLen) const {
		auto caretPos = execute(SCI_GETCURRENTPOS);
		auto startPos = execute(SCI_WORDSTARTPOSITION, caretPos, true);

		str[0] = '\0';
		if ((caretPos - startPos) < strLen)
			getGenericText(str, strLen, startPos, caretPos);
	};

    void doUserDefineDlg(bool willBeShown = true, bool isRTL = false) {
        _userDefineDlg.doDialog(willBeShown, isRTL);
    };

    static UserDefineDialog * getUserDefineDlg() {return &_userDefineDlg;};

	void beSwitched() {
		_userDefineDlg.setScintilla(this);
	};

    //Marge member and method
    static const int _SC_MARGE_LINENUMBER;
    static const int _SC_MARGE_SYMBOL;
    static const int _SC_MARGE_FOLDER;
    static const int _SC_MARGE_CHANGEHISTORY;

    void showMargin(int whichMarge, bool willBeShowed = true);
    void showChangeHistoryMargin(bool willBeShowed = true);

    bool hasMarginShowed(int witchMarge) {
		return (execute(SCI_GETMARGINWIDTHN, witchMarge, 0) != 0);
    };

    void updateBeginEndSelectPosition(bool is_insert, size_t position, size_t length);
    void marginClick(Sci_Position position, int modifiers);

    void setMakerStyle(folderStyle style) {
		bool display;
		if (style == FOLDER_STYLE_NONE)
		{
			style = FOLDER_STYLE_BOX;
			display = false;
		}
		else
		{
			display = true;
		}

		COLORREF foldfgColor = white, foldbgColor = grey, activeFoldFgColor = red;
		getFoldColor(foldfgColor, foldbgColor, activeFoldFgColor);

		for (int i = 0 ; i < NB_FOLDER_STATE ; ++i)
			defineMarker(_markersArray[FOLDER_TYPE][i], _markersArray[style][i], foldfgColor, foldbgColor, activeFoldFgColor);
		showMargin(ScintillaEditView::_SC_MARGE_FOLDER, display);
    };


	void setWrapMode(lineWrapMethod meth) {
		int mode = (meth == LINEWRAP_ALIGNED)?SC_WRAPINDENT_SAME:\
				(meth == LINEWRAP_INDENT)?SC_WRAPINDENT_INDENT:SC_WRAPINDENT_FIXED;
		execute(SCI_SETWRAPINDENTMODE, mode);
	};


	void showWSAndTab(bool willBeShowed = true) {
		execute(SCI_SETVIEWWS, willBeShowed?SCWS_VISIBLEALWAYS:SCWS_INVISIBLE);
		execute(SCI_SETWHITESPACESIZE, 2, 0);
	};

	bool isShownSpaceAndTab() {
		return (execute(SCI_GETVIEWWS) != 0);
	};

	void showEOL(bool willBeShowed = true) {
		execute(SCI_SETVIEWEOL, willBeShowed);
	};

	bool isShownEol() {
		return (execute(SCI_GETVIEWEOL) != 0);
	};

	void showNpc(bool willBeShowed = true, bool isSearchResult = false);

	bool isShownNpc() {
		const auto& svp = NppParameters::getInstance().getSVP();
		return svp._npcShow;
	};

	void maintainStateForNpc() {
		const auto& svp = NppParameters::getInstance().getSVP();
		const bool isShownNpc = svp._npcShow;
		const bool isShownCcUniEol = svp._ccUniEolShow;

		if (isShownNpc && isShownCcUniEol)
		{
			showNpc(true);
			showCcUniEol(true);

			if (svp._eolMode != svp.roundedRectangleText)
			{
				setCRLF();
			}
		}
		else if (!isShownNpc && isShownCcUniEol)
		{
			showNpc(false);
		}
		else
		{
			showCcUniEol(false);
		}
	}

	void showCcUniEol(bool willBeShowed = true, bool isSearchResult = false);

	bool isShownCcUniEol() {
		const auto& svp = NppParameters::getInstance().getSVP();
		return svp._ccUniEolShow;
	};

	void showInvisibleChars(bool willBeShowed = true) {
		showNpc(willBeShowed);
		showCcUniEol(willBeShowed);
		showWSAndTab(willBeShowed);
		showEOL(willBeShowed);
	};

	//bool isShownInvisibleChars() {
	//	return isShownSpaceTab() && isShownEol() && isShownNpc();
	//};

	void showIndentGuideLine(bool willBeShowed = true);

	bool isShownIndentGuide() const {
		return (execute(SCI_GETINDENTATIONGUIDES) != 0);
	};

    void wrap(bool willBeWrapped = true) {
        execute(SCI_SETWRAPMODE, willBeWrapped);
    };

    bool isWrap() const {
        return (execute(SCI_GETWRAPMODE) == SC_WRAP_WORD);
    };

	bool isWrapSymbolVisible() const {
		return (execute(SCI_GETWRAPVISUALFLAGS) != SC_WRAPVISUALFLAG_NONE);
	};

    void showWrapSymbol(bool willBeShown = true) {
		execute(SCI_SETWRAPVISUALFLAGSLOCATION, SC_WRAPVISUALFLAGLOC_DEFAULT);
		execute(SCI_SETWRAPVISUALFLAGS, willBeShown?SC_WRAPVISUALFLAG_END:SC_WRAPVISUALFLAG_NONE);
    };

	intptr_t getCurrentLineNumber()const {
		return execute(SCI_LINEFROMPOSITION, execute(SCI_GETCURRENTPOS));
	};

	intptr_t lastZeroBasedLineNumber() const {
		auto endPos = execute(SCI_GETLENGTH);
		return execute(SCI_LINEFROMPOSITION, endPos);
	};

	intptr_t getCurrentXOffset()const{
		return execute(SCI_GETXOFFSET);
	};

	void setCurrentXOffset(long xOffset){
		execute(SCI_SETXOFFSET,xOffset);
	};

	void scroll(intptr_t column, intptr_t line){
		execute(SCI_LINESCROLL, column, line);
	};

	intptr_t getCurrentPointX()const{
		return execute(SCI_POINTXFROMPOSITION, 0, execute(SCI_GETCURRENTPOS));
	};

	intptr_t getCurrentPointY()const{
		return execute(SCI_POINTYFROMPOSITION, 0, execute(SCI_GETCURRENTPOS));
	};

	intptr_t getTextHeight()const{
		return execute(SCI_TEXTHEIGHT);
	};

	int getTextZoneWidth() const;

	void gotoLine(intptr_t line){
		if (line < execute(SCI_GETLINECOUNT))
			execute(SCI_GOTOLINE,line);
	};

	intptr_t getCurrentColumnNumber() const {
        return execute(SCI_GETCOLUMN, execute(SCI_GETCURRENTPOS));
    };

	std::pair<size_t, size_t> getSelectedCharsAndLinesCount(long long maxSelectionsForLineCount = -1) const;

	size_t getUnicodeSelectedLength() const;

	intptr_t getLineLength(size_t line) const {
		return execute(SCI_GETLINEENDPOSITION, line) - execute(SCI_POSITIONFROMLINE, line);
	};

	intptr_t getLineIndent(size_t line) const {
		return execute(SCI_GETLINEINDENTATION, line);
	};

	void setLineIndent(size_t line, size_t indent) const;

	void updateLineNumbersMargin(bool forcedToHide) {
		const ScintillaViewParams& svp = NppParameters::getInstance().getSVP();
		if (forcedToHide)
		{
			execute(SCI_SETMARGINWIDTHN, _SC_MARGE_LINENUMBER, 0);
		}
		else if (svp._lineNumberMarginShow)
		{
			updateLineNumberWidth();
		}
		else
		{
			execute(SCI_SETMARGINWIDTHN, _SC_MARGE_LINENUMBER, 0);
		}
	}

	void updateLineNumberWidth();
	void performGlobalStyles();

	std::pair<size_t, size_t> getSelectionLinesRange(intptr_t selectionNumber = -1) const;
    void currentLinesUp() const;
    void currentLinesDown() const;

	intptr_t caseConvertRange(intptr_t start, intptr_t end, TextCase caseToConvert);
	void changeCase(__inout wchar_t * const strWToConvert, const int & nbChars, const TextCase & caseToConvert) const;
	void convertSelectedTextTo(const TextCase & caseToConvert);
	void setMultiSelections(const ColumnModeInfos & cmi);

    void convertSelectedTextToLowerCase() {
		// if system is w2k or xp
		if ((NppParameters::getInstance()).isTransparentAvailable())
			convertSelectedTextTo(LOWERCASE);
		else
			execute(SCI_LOWERCASE);
	};

    void convertSelectedTextToUpperCase() {
		// if system is w2k or xp
		if ((NppParameters::getInstance()).isTransparentAvailable())
			convertSelectedTextTo(UPPERCASE);
		else
			execute(SCI_UPPERCASE);
	};

	void convertSelectedTextToNewerCase(const TextCase & caseToConvert) {
		// if system is w2k or xp
		if ((NppParameters::getInstance()).isTransparentAvailable())
			convertSelectedTextTo(caseToConvert);
		else
			::MessageBox(_hSelf, L"This function needs a newer OS version.", L"Change Case Error", MB_OK | MB_ICONHAND);
	};

	void getCurrentFoldStates(std::vector<size_t> & lineStateVector);
	void syncFoldStateWith(const std::vector<size_t> & lineStateVectorNew);
	bool isFoldIndentationBased() const;
	void foldIndentationBasedLevel(int level, bool mode);
	void foldLevel(int level, bool mode);
	void foldAll(bool mode);
	void fold(size_t line, bool mode, bool shouldBeNotified = true);
	bool isFolded(size_t line) const {
		return (execute(SCI_GETFOLDEXPANDED, line) != 0);
	};
	void expand(size_t& line, bool doExpand, bool force = false, intptr_t visLevels = 0, intptr_t level = -1);

	bool isCurrentLineFolded() const;
	void foldCurrentPos(bool mode);
	int getCodepage() const {return _codepage;};

	ColumnModeInfos getColumnModeSelectInfo();

	void columnReplace(ColumnModeInfos & cmi, const wchar_t *str);
	void columnReplace(ColumnModeInfos & cmi, size_t initial, size_t incr, size_t repeat, UCHAR format, ColumnEditorParam::leadingChoice lead);

	void clearIndicator(int indicatorNumber) {
		size_t docStart = 0;
		size_t docEnd = getCurrentDocLen();
		execute(SCI_SETINDICATORCURRENT, indicatorNumber);
		execute(SCI_INDICATORCLEARRANGE, docStart, docEnd - docStart);
	};

	bool getIndicatorRange(size_t indicatorNumber, size_t* from = NULL, size_t* to = NULL, size_t* cur = NULL);

	static LanguageNameInfo _langNameInfoArray[L_EXTERNAL+1];

	void bufferUpdated(Buffer * buffer, int mask);
	BufferID getCurrentBufferID() { return _currentBufferID; };
	Buffer * getCurrentBuffer() { return _currentBuffer; };
	void setCurrentBuffer(Buffer *buf2set) { _currentBuffer = buf2set; };
	void styleChange();

	void hideLines();
	bool hidelineMarkerClicked(intptr_t lineNumber);	//true if it did something
	void notifyHidelineMarkers(Buffer * buf, bool isHide, size_t location, bool del);
	void hideMarkedLines(size_t searchStart, bool endOfDoc);
	void showHiddenLines(size_t searchStart, bool endOfDoc, bool doDelete);
	void restoreHiddenLines();

	bool hasSelection() const { return !execute(SCI_GETSELECTIONEMPTY); };

	bool isPythonStyleIndentation(LangType typeDoc) const{
		return (typeDoc == L_PYTHON || typeDoc == L_COFFEESCRIPT || typeDoc == L_HASKELL ||\
			typeDoc == L_C || typeDoc == L_CPP || typeDoc == L_OBJC || typeDoc == L_CS || typeDoc == L_JAVA ||\
			typeDoc == L_PHP || typeDoc == L_JS || typeDoc == L_JAVASCRIPT || typeDoc == L_MAKEFILE ||\
			typeDoc == L_ASN1 || typeDoc == L_GDSCRIPT);
	};

	void setLanguage(LangType langType);
	void defineDocType(LangType typeDoc);	//setup stylers for active document

	void addCustomWordChars();
	void restoreDefaultWordChars();
	void setWordChars();
	void setCRLF(long color = -1);
	void setNpcAndCcUniEOL(long color = -1);

	void mouseWheel(WPARAM wParam, LPARAM lParam) {
		scintillaNew_Proc(_hSelf, WM_MOUSEWHEEL, wParam, lParam);
	};

	void setHotspotStyle(const Style& styleToSet);
    void setTabSettings(Lang *lang);
	bool isWrapRestoreNeeded() const {return _wrapRestoreNeeded;};
	void setWrapRestoreNeeded(bool isWrapRestoredNeeded) {_wrapRestoreNeeded = isWrapRestoredNeeded;};

	bool isCJK() const {
		return ((_codepage == CP_CHINESE_TRADITIONAL) || (_codepage == CP_CHINESE_SIMPLIFIED) ||
			    (_codepage == CP_JAPANESE) || (_codepage == CP_KOREAN));
	};
	void scrollPosToCenter(size_t pos);
	std::wstring getEOLString() const;
	void setBorderEdge(bool doWithBorderEdge);
	void sortLines(size_t fromLine, size_t toLine, ISorter *pSort);
	void changeTextDirection(bool isRTL);
	bool isTextDirectionRTL() const;
	void setPositionRestoreNeeded(bool val) { _positionRestoreNeeded = val; };
	void markedTextToClipboard(int indiStyle, bool doAll = false);
	void removeAnyDuplicateLines();
	bool expandWordSelection();
	bool pasteToMultiSelection() const;
	void setElementColour(int element, COLORREF color) const { execute(SCI_SETELEMENTCOLOUR, element, color | 0xFF000000); };

	Document getBlankDocument();

protected:
	static bool _SciInit;

	static int _refCount;

    static UserDefineDialog _userDefineDlg;

    static const int _markersArray[][NB_FOLDER_STATE];

	static LRESULT CALLBACK scintillaStatic_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT scintillaNew_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	bool _isMainEditZone = false;
	SCINTILLA_FUNC _pScintillaFunc = nullptr;
	SCINTILLA_PTR  _pScintillaPtr = nullptr;
	static WNDPROC _scintillaDefaultProc;
	CallWindowProcFunc _callWindowProc = nullptr;
	BufferID attachDefaultDoc();

	//Store the current buffer so it can be retrieved later
	BufferID _currentBufferID = nullptr;
	Buffer * _currentBuffer = nullptr;

	Buffer* _prevBuffer = nullptr;
	Document _blankDocument = 0;

	int _codepage = CP_ACP;
	bool _wrapRestoreNeeded = false;
	bool _positionRestoreNeeded = false;
	uint32_t _restorePositionRetryCount = 0;

	typedef std::unordered_map<int, Style> StyleMap;
	typedef std::unordered_map<BufferID, StyleMap*> BufferStyleMap;
	BufferStyleMap _hotspotStyles;

	intptr_t _beginSelectPosition = -1;
	static std::string _defaultCharList;
	bool _isMultiPasteActive = false;

//Lexers and Styling
	void restyleBuffer();
	const char * getCompleteKeywordList(std::basic_string<char> & kwl, LangType langType, int keywordIndex);
	void setKeywords(LangType langType, const char *keywords, int index);
	void populateSubStyleKeywords(LangType langType, int baseStyleID, int numSubStyles, int firstLangIndex, const wchar_t **pKwArray);
	void setLexer(LangType langID, int whichList, int baseStyleID = STYLE_NOT_USED, int numSubStyles = 8);
	bool setLexerFromLangID(int langID);
	void makeStyle(LangType langType, const wchar_t **keywordArray = NULL);
	void setStyle(Style styleToSet);			//NOT by reference	(style edited)
	void setSpecialStyle(const Style & styleToSet);	//by reference
	void setSpecialIndicator(const Style & styleToSet) {
		execute(SCI_INDICSETFORE, styleToSet._styleID, styleToSet._bgColor);
	};

	//Complex lexers (same lexer, different language)
	void setXmlLexer(LangType type);
 	void setCppLexer(LangType type);
	void setHTMLLexer();
	void setJsLexer();
	void setTclLexer();
    void setObjCLexer(LangType type);
	void setUserLexer(const wchar_t *userLangName = NULL);
	void setExternalLexer(LangType typeDoc);
	void setEmbeddedJSLexer();
    void setEmbeddedPhpLexer();
    void setEmbeddedAspLexer();
	void setJsonLexer(bool isJson5 = false);
	void setTypeScriptLexer();

	//Simple lexers
	void setCssLexer() {
		setLexer(L_CSS, LIST_0 | LIST_1 | LIST_4 | LIST_6);
	};

	void setLuaLexer() {
		setLexer(L_LUA, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6 | LIST_7, SCE_LUA_IDENTIFIER, 4);
	};

	void setMakefileLexer() {
		setLexer(L_MAKEFILE, LIST_NONE);
	};

	void setPropsLexer(bool isPropsButNotIni = true) {
		LangType L_id = isPropsButNotIni ? L_PROPS : L_INI;
		setLexer(L_id, LIST_NONE);
		execute(SCI_STYLESETEOLFILLED, SCE_PROPS_SECTION, true);
	};

	void setSqlLexer() {
		const bool kbBackSlash = NppParameters::getInstance().getNppGUI()._backSlashIsEscapeCharacterForSql;
		setLexer(L_SQL, LIST_0 | LIST_1 | LIST_4);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("sql.backslash.escapes"), reinterpret_cast<LPARAM>(kbBackSlash ? "1" : "0"));
	};

	void setMSSqlLexer() {
		setLexer(L_MSSQL, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5);
	};

	void setBashLexer() {
		setLexerFromLangID(L_BASH);

		const wchar_t *pKwArray[NB_LIST] = {NULL};
		makeStyle(L_BASH, pKwArray);

		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		const char * keyWords_char = wmc.wchar2char(pKwArray[LANG_INDEX_INSTR], CP_ACP);
		setKeywords(L_BASH, keyWords_char, LANG_INDEX_INSTR);

		populateSubStyleKeywords(L_BASH, SCE_SH_IDENTIFIER, 4, LANG_INDEX_SUBSTYLE1, pKwArray);
		populateSubStyleKeywords(L_BASH, SCE_SH_SCALAR, 4, LANG_INDEX_SUBSTYLE5, pKwArray);

		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<LPARAM>("0"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.comment"), reinterpret_cast<LPARAM>("1"));
	};

	void setVBLexer() {
		setLexer(L_VB, LIST_0);
	};

	void setPascalLexer() {
		setLexer(L_PASCAL, LIST_0);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
	};

	void setPerlLexer() {
		setLexer(L_PERL, LIST_0);
	};

	void setPythonLexer() {
		setLexer(L_PYTHON, LIST_0 | LIST_1, SCE_P_IDENTIFIER);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.quotes.python"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.python.decorator.attributes"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.python.identifier.attributes"), reinterpret_cast<LPARAM>("1"));
	};
	
	void setGDScriptLexer() {
		setLexer(L_GDSCRIPT, LIST_0 | LIST_1, SCE_GD_IDENTIFIER);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.gdscript.keywords2.no.sub.identifiers"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.gdscript.whinge.level"), reinterpret_cast<LPARAM>("1"));
	};

	void setBatchLexer() {
		setLexer(L_BATCH, LIST_0);
	};

	void setTeXLexer() {
		for (int i = 0 ; i < 4 ; ++i)
			execute(SCI_SETKEYWORDS, i, reinterpret_cast<LPARAM>(L""));
		setLexer(L_TEX, LIST_NONE);
	};

	void setNsisLexer() {
		setLexer(L_NSIS, LIST_0 | LIST_1 | LIST_2 | LIST_3);
	};

	void setFortranLexer() {
		setLexer(L_FORTRAN, LIST_0 | LIST_1 | LIST_2);
	};

	void setFortran77Lexer() {
		setLexer(L_FORTRAN_77, LIST_0 | LIST_1 | LIST_2);
	};

	void setLispLexer(){
		setLexer(L_LISP, LIST_0 | LIST_1);
	};

	void setSchemeLexer(){
		setLexer(L_SCHEME, LIST_0 | LIST_1);
	};

	void setAsmLexer(){
		setLexer(L_ASM, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6 | LIST_7);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.asm.syntax.based"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.asm.comment.multiline"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.asm.comment.explicit"), reinterpret_cast<LPARAM>("1"));
	};

	void setDiffLexer(){
		setLexer(L_DIFF, LIST_NONE);
	};

	void setPostscriptLexer(){
		setLexer(L_PS, LIST_0 | LIST_1 | LIST_2 | LIST_3);
	};

	void setRubyLexer(){
		setLexer(L_RUBY, LIST_0);
		execute(SCI_STYLESETEOLFILLED, SCE_RB_POD, true);
	};

	void setSmalltalkLexer(){
		setLexer(L_SMALLTALK, LIST_0);
	};

	void setVhdlLexer(){
		setLexer(L_VHDL, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6);
	};

	void setKixLexer(){
		setLexer(L_KIX, LIST_0 | LIST_1 | LIST_2);
	};

	void setAutoItLexer(){
		setLexer(L_AU3, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
	};

	void setCamlLexer(){
		setLexer(L_CAML, LIST_0 | LIST_1 | LIST_2);
	};

	void setAdaLexer(){
		setLexer(L_ADA, LIST_0);
	};

	void setVerilogLexer(){
		setLexer(L_VERILOG, LIST_0 | LIST_1);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
	};

	void setMatlabLexer(){
		setLexer(L_MATLAB, LIST_0);
	};

	void setHaskellLexer(){
		setLexer(L_HASKELL, LIST_0);
	};

	void setInnoLexer() {
		setLexer(L_INNO, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5);
	};

	void setCmakeLexer() {
		setLexer(L_CMAKE, LIST_0 | LIST_1 | LIST_2);
	};

	void setYamlLexer() {
		setLexer(L_YAML, LIST_0);
	};

    //--------------------

    void setCobolLexer() {
		setLexer(L_COBOL, LIST_0 | LIST_1 | LIST_2);
	};
    void setGui4CliLexer() {
		setLexer(L_GUI4CLI, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4);
	};
    void setDLexer() {
		setLexer(L_D, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6);
	};
    void setPowerShellLexer() {
		setLexer(L_POWERSHELL, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5);
	};
    void setRLexer() {
		setLexer(L_R, LIST_0 | LIST_1 | LIST_2);
	};

    void setCoffeeScriptLexer() {
		setLexer(L_COFFEESCRIPT, LIST_0 | LIST_1 | LIST_2  | LIST_3);
	};

	void setBaanCLexer() {
		setLexer(L_BAANC, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6 | LIST_7 | LIST_8);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.baan.styling.within.preprocessor"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$:"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.baan.syntax.based"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.baan.keywords.based"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.baan.sections"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.baan.inner.level"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_STYLESETEOLFILLED, SCE_BAAN_STRINGEOL, true);
	};

	void setSrecLexer() {
		setLexer(L_SREC, LIST_NONE);
	};

	void setIHexLexer() {
		setLexer(L_IHEX, LIST_NONE);
	};

	void setTEHexLexer() {
		setLexer(L_TEHEX, LIST_NONE);
	};

	void setAsn1Lexer() {
		setLexer(L_ASN1, LIST_0 | LIST_1 | LIST_2 | LIST_3); 
	};

	void setAVSLexer() {
		setLexer(L_AVS, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5);
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#"));
	};

	void setBlitzBasicLexer() {
		setLexer(L_BLITZBASIC, LIST_0 | LIST_1 | LIST_2 | LIST_3); 
	};

	void setPureBasicLexer() {
		setLexer(L_PUREBASIC, LIST_0 | LIST_1 | LIST_2 | LIST_3); 
	};

	void setFreeBasicLexer() {
		setLexer(L_FREEBASIC, LIST_0 | LIST_1 | LIST_2 | LIST_3); 
	};

	void setCsoundLexer() {
		setLexer(L_CSOUND, LIST_0 | LIST_1 | LIST_2);
		execute(SCI_STYLESETEOLFILLED, SCE_CSOUND_STRINGEOL, true);
	};

	void setErlangLexer() {
		setLexer(L_ERLANG, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5); 
	};

	void setESCRIPTLexer() {
		setLexer(L_ESCRIPT, LIST_0 | LIST_1 | LIST_2); 
	};

	void setForthLexer() {
		setLexer(L_FORTH, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5);
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789%-"));
	};

	void setLatexLexer() {
		setLexer(L_LATEX, LIST_NONE); 
	};

	void setMMIXALLexer() {
		setLexer(L_MMIXAL, LIST_0 | LIST_1 | LIST_2); 
	};

	void setNimrodLexer() {
		setLexer(L_NIM, LIST_0);
	};

	void setNncrontabLexer() {
		setLexer(L_NNCRONTAB, LIST_0 | LIST_1 | LIST_2); 
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789%-"));
	};

	void setOScriptLexer() {
		setLexer(L_OSCRIPT, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5);
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$"));
	};

	void setREBOLLexer() {
		setLexer(L_REBOL, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6);
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789?!.'+-*&|=_~"));
	};

	void setRegistryLexer() {
		setLexer(L_REGISTRY, LIST_NONE); 
	};

	void setRustLexer() {
		setLexer(L_RUST, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6); 
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#"));
	};

	void setSpiceLexer() {
		setLexer(L_SPICE, LIST_0 | LIST_1 | LIST_2); 
	};

	void setTxt2tagsLexer() {
		setLexer(L_TXT2TAGS, LIST_NONE); 
	};

	void setVisualPrologLexer() {
		setLexer(L_VISUALPROLOG, LIST_0 | LIST_1 | LIST_2 | LIST_3);
	}
	
	void setHollywoodLexer() {
		setLexer(L_HOLLYWOOD, LIST_0 | LIST_1 | LIST_2 | LIST_3);
	};	

	void setRakuLexer(){
		setLexer(L_RAKU, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.raku.comment.multiline"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.raku.comment.pod"), reinterpret_cast<LPARAM>("1"));
	};

	void setTomlLexer(){
		setLexer(L_TOML, LIST_0);
	};

	void setSasLexer(){
		setLexer(L_SAS, LIST_0 | LIST_1 | LIST_2 | LIST_3);
	};

    //--------------------

	void setSearchResultLexer() {
		if (execute(SCI_GETLEXER) == SCLEX_SEARCHRESULT)
		{
			makeStyle(L_SEARCHRESULT, nullptr);
			return;
		}
		execute(SCI_STYLESETEOLFILLED, SCE_SEARCHRESULT_FILE_HEADER, true);
		execute(SCI_STYLESETEOLFILLED, SCE_SEARCHRESULT_SEARCH_HEADER, true);
		setLexer(L_SEARCHRESULT, LIST_NONE);
	};

	bool isNeededFolderMarge(LangType typeDoc) const {
		switch (typeDoc)
		{
			case L_ASCII:
			case L_BATCH:
			case L_TEXT:
			case L_MAKEFILE:
			case L_HASKELL:
			case L_SMALLTALK:
			case L_KIX:
			case L_ADA:
				return false;
			default:
				return true;
		}
	};
//END: Lexers and Styling

    void defineMarker(int marker, int markerType, COLORREF fore, COLORREF back, COLORREF foreActive) {
	    execute(SCI_MARKERDEFINE, marker, markerType);
	    execute(SCI_MARKERSETFORE, marker, fore);
	    execute(SCI_MARKERSETBACK, marker, back);
		execute(SCI_MARKERSETBACKSELECTED, marker, foreActive);
	};

	int codepage2CharSet() const {
		switch (_codepage)
		{
			case CP_CHINESE_TRADITIONAL : return SC_CHARSET_CHINESEBIG5;
			case CP_CHINESE_SIMPLIFIED : return SC_CHARSET_GB2312;
			case CP_KOREAN : return SC_CHARSET_HANGUL;
			case CP_JAPANESE : return SC_CHARSET_SHIFTJIS;
			case CP_GREEK : return SC_CHARSET_GREEK;
			default : return 0;
		}
	};

	std::pair<size_t, size_t> getWordRange();
	void getFoldColor(COLORREF& fgColor, COLORREF& bgColor, COLORREF& activeFgColor);
};

