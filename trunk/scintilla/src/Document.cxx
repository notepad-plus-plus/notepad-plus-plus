// Scintilla source code edit control
/** @file Document.cxx
 ** Text document that handles notifications, DBCS, styling, words and end of line.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "Platform.h"

#include "Scintilla.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "CellBuffer.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "Document.h"
#include "RESearch.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

//Vitaliy
#include "UniConversion.h"

// Win32 only !!!
static bool IsMustDie9x(void) 
{
    OSVERSIONINFO osver;
    osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if ( GetVersionEx( &osver ) ) 
    {
        if ( (osver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
             (osver.dwMajorVersion == 4) ) 
        {
            //MessageBox(NULL, "MustDie9x == true", "Test", MB_OK);
            return true;
        }
    }
    //MessageBox(NULL, "MustDie9x == false", "Test", MB_OK);
    return false;
}

static inline void Platform_MakeUpperW(wchar_t* wstr, unsigned int len) {
    // TODO: Add platform-specific function here
  
    // Win32 example:
    static bool bIsMustDie9x = IsMustDie9x();

    if ( !bIsMustDie9x )
    {
        ::CharUpperW(wstr);
    }
    else
    {
        char* str = new char[len + 1];
        if ( str )
        {
            ::WideCharToMultiByte(CP_ACP, 0, wstr, len, str, len, NULL, NULL);
            str[len] = 0;
            ::CharUpperA(str);
            ::MultiByteToWideChar(CP_ACP, 0, str, len, wstr, len);
            wstr[len] = 0;
            delete [] str;
        }
    }
}

static inline char Platform_MakeUpperChar(char ch) {
    // TODO: Add platform-specific function here
  
    // Win32 example:
    char str[2] = {ch, 0};
    ::CharUpperA(str);
    ch = str[0];
    
    // default: no conversion
    return ch;
}

static inline char Platform_MakeLowerChar(char ch) {
    // TODO: Add platform-specific function here
  
    // Win32 example:
    char str[2] = {ch, 0};
    ::CharLowerA(str);
    ch = str[0];
    
    // default: no conversion
    return ch;
}
// yilatiV

// This is ASCII specific but is safe with chars >= 0x80
static inline bool isspacechar(unsigned char ch) {
	return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d));
}

static inline bool IsPunctuation(char ch) {
	return isascii(ch) && ispunct(ch);
}

static inline bool IsADigit(char ch) {
	return isascii(ch) && isdigit(ch);
}

static inline bool IsLowerCase(char ch) {
	return isascii(ch) && islower(ch);
}

static inline bool IsUpperCase(char ch) {
	return isascii(ch) && isupper(ch);
}

Document::Document() {
	refCount = 0;
#ifdef unix
	eolMode = SC_EOL_LF;
#else
	eolMode = SC_EOL_CRLF;
#endif
	dbcsCodePage = 0;
	stylingBits = 5;
	stylingBitsMask = 0x1F;
	stylingMask = 0;
	endStyled = 0;
	styleClock = 0;
	enteredModification = 0;
	enteredStyling = 0;
	enteredReadOnlyCount = 0;
	tabInChars = 8;
	indentInChars = 0;
	actualIndentInChars = 8;
	useTabs = true;
	tabIndents = true;
	backspaceUnindents = false;
	watchers = 0;
	lenWatchers = 0;

	matchesValid = false;
	pre = 0;
	substituted = 0;
}

Document::~Document() {
	for (int i = 0; i < lenWatchers; i++) {
		watchers[i].watcher->NotifyDeleted(this, watchers[i].userData);
	}
	delete []watchers;
	watchers = 0;
	lenWatchers = 0;
	delete pre;
	pre = 0;
	delete []substituted;
	substituted = 0;
}

// Increase reference count and return its previous value.
int Document::AddRef() {
	return refCount++;
}

// Decrease reference count and return its previous value.
// Delete the document if reference count reaches zero.
int Document::Release() {
	int curRefCount = --refCount;
	if (curRefCount == 0)
		delete this;
	return curRefCount;
}

void Document::SetSavePoint() {
	cb.SetSavePoint();
	NotifySavePoint(true);
}

int Document::AddMark(int line, int markerNum) {
	int prev = cb.AddMark(line, markerNum);
	DocModification mh(SC_MOD_CHANGEMARKER, LineStart(line), 0, 0, 0, line);
	NotifyModified(mh);
	return prev;
}

void Document::AddMarkSet(int line, int valueSet) {
	unsigned int m = valueSet;
	for (int i = 0; m; i++, m >>= 1)
		if (m & 1)
			cb.AddMark(line, i);
	DocModification mh(SC_MOD_CHANGEMARKER, LineStart(line), 0, 0, 0, line);
	NotifyModified(mh);
}

void Document::DeleteMark(int line, int markerNum) {
	cb.DeleteMark(line, markerNum);
	DocModification mh(SC_MOD_CHANGEMARKER, LineStart(line), 0, 0, 0, line);
	NotifyModified(mh);
}

void Document::DeleteMarkFromHandle(int markerHandle) {
	cb.DeleteMarkFromHandle(markerHandle);
	DocModification mh(SC_MOD_CHANGEMARKER, 0, 0, 0, 0);
	mh.line = -1;
	NotifyModified(mh);
}

void Document::DeleteAllMarks(int markerNum) {
	cb.DeleteAllMarks(markerNum);
	DocModification mh(SC_MOD_CHANGEMARKER, 0, 0, 0, 0);
	mh.line = -1;
	NotifyModified(mh);
}

int Document::LineStart(int line) const {
	return cb.LineStart(line);
}

int Document::LineEnd(int line) const {
	if (line == LinesTotal() - 1) {
		return LineStart(line + 1);
	} else {
		int position = LineStart(line + 1) - 1;
		// When line terminator is CR+LF, may need to go back one more
		if ((position > LineStart(line)) && (cb.CharAt(position - 1) == '\r')) {
			position--;
		}
		return position;
	}
}

int Document::LineFromPosition(int pos) {
	return cb.LineFromPosition(pos);
}

int Document::LineEndPosition(int position) {
	return LineEnd(LineFromPosition(position));
}

int Document::VCHomePosition(int position) {
	int line = LineFromPosition(position);
	int startPosition = LineStart(line);
	int endLine = LineStart(line + 1) - 1;
	int startText = startPosition;
	while (startText < endLine && (cb.CharAt(startText) == ' ' || cb.CharAt(startText) == '\t' ) )
		startText++;
	if (position == startText)
		return startPosition;
	else
		return startText;
}

int Document::SetLevel(int line, int level) {
	int prev = cb.SetLevel(line, level);
	if (prev != level) {
		DocModification mh(SC_MOD_CHANGEFOLD | SC_MOD_CHANGEMARKER,
		                   LineStart(line), 0, 0, 0, line);
		mh.foldLevelNow = level;
		mh.foldLevelPrev = prev;
		NotifyModified(mh);
	}
	return prev;
}

static bool IsSubordinate(int levelStart, int levelTry) {
	if (levelTry & SC_FOLDLEVELWHITEFLAG)
		return true;
	else
		return (levelStart & SC_FOLDLEVELNUMBERMASK) < (levelTry & SC_FOLDLEVELNUMBERMASK);
}

int Document::GetLastChild(int lineParent, int level) {
	if (level == -1)
		level = GetLevel(lineParent) & SC_FOLDLEVELNUMBERMASK;
	int maxLine = LinesTotal();
	int lineMaxSubord = lineParent;
	while (lineMaxSubord < maxLine - 1) {
		EnsureStyledTo(LineStart(lineMaxSubord + 2));
		if (!IsSubordinate(level, GetLevel(lineMaxSubord + 1)))
			break;
		lineMaxSubord++;
	}
	if (lineMaxSubord > lineParent) {
		if (level > (GetLevel(lineMaxSubord + 1) & SC_FOLDLEVELNUMBERMASK)) {
			// Have chewed up some whitespace that belongs to a parent so seek back
			if (GetLevel(lineMaxSubord) & SC_FOLDLEVELWHITEFLAG) {
				lineMaxSubord--;
			}
		}
	}
	return lineMaxSubord;
}

int Document::GetFoldParent(int line) {
	int level = GetLevel(line) & SC_FOLDLEVELNUMBERMASK;
	int lineLook = line - 1;
	while ((lineLook > 0) && (
	            (!(GetLevel(lineLook) & SC_FOLDLEVELHEADERFLAG)) ||
	            ((GetLevel(lineLook) & SC_FOLDLEVELNUMBERMASK) >= level))
	      ) {
		lineLook--;
	}
	if ((GetLevel(lineLook) & SC_FOLDLEVELHEADERFLAG) &&
	        ((GetLevel(lineLook) & SC_FOLDLEVELNUMBERMASK) < level)) {
		return lineLook;
	} else {
		return -1;
	}
}

int Document::ClampPositionIntoDocument(int pos) {
	return Platform::Clamp(pos, 0, Length());
}

bool Document::IsCrLf(int pos) {
	if (pos < 0)
		return false;
	if (pos >= (Length() - 1))
		return false;
	return (cb.CharAt(pos) == '\r') && (cb.CharAt(pos + 1) == '\n');
}

static const int maxBytesInDBCSCharacter=5;

int Document::LenChar(int pos) {
	if (pos < 0) {
		return 1;
	} else if (IsCrLf(pos)) {
		return 2;
	} else if (SC_CP_UTF8 == dbcsCodePage) {
		unsigned char ch = static_cast<unsigned char>(cb.CharAt(pos));
		if (ch < 0x80)
			return 1;
		int len = 2;
		if (ch >= (0x80 + 0x40 + 0x20 + 0x10))
			len = 4;
		else if (ch >= (0x80 + 0x40 + 0x20))
			len = 3;
		int lengthDoc = Length();
		if ((pos + len) > lengthDoc)
			return lengthDoc -pos;
		else
			return len;
	} else if (dbcsCodePage) {
		char mbstr[maxBytesInDBCSCharacter+1];
		int i;
		for (i=0; i<Platform::DBCSCharMaxLength(); i++) {
			mbstr[i] = cb.CharAt(pos+i);
		}
		mbstr[i] = '\0';
		return Platform::DBCSCharLength(dbcsCodePage, mbstr);
	} else {
		return 1;
	}
}

static bool IsTrailByte(int ch) {
	return (ch >= 0x80) && (ch < (0x80 + 0x40));
}

static int BytesFromLead(int leadByte) {
	if (leadByte > 0xF4) {
		// Characters longer than 4 bytes not possible in current UTF-8
		return 0;
	} else if (leadByte >= 0xF0) {
		return 4;
	} else if (leadByte >= 0xE0) {
		return 3;
	} else if (leadByte >= 0xC2) {
		return 2;
	}
	return 0;
}

bool Document::InGoodUTF8(int pos, int &start, int &end) {
	int lead = pos;
	while ((lead>0) && (pos-lead < 4) && IsTrailByte(static_cast<unsigned char>(cb.CharAt(lead-1))))
		lead--;
	start = 0;
	if (lead > 0) {
		start = lead-1;
	}
	int leadByte = static_cast<unsigned char>(cb.CharAt(start));
	int bytes = BytesFromLead(leadByte);
	if (bytes == 0) {
		return false;
	} else {
		int trailBytes = bytes - 1;
		int len = pos - lead + 1;
		if (len > trailBytes)
			// pos too far from lead
			return false;
		// Check that there are enough trails for this lead
		int trail = pos + 1;
		while ((trail-lead<trailBytes) && (trail < Length())) {
			if (!IsTrailByte(static_cast<unsigned char>(cb.CharAt(trail)))) {
				return false;
			}
			trail++;
		}
		end = start + bytes;
		return true;
	}
}

// Normalise a position so that it is not halfway through a two byte character.
// This can occur in two situations -
// When lines are terminated with \r\n pairs which should be treated as one character.
// When displaying DBCS text such as Japanese.
// If moving, move the position in the indicated direction.
int Document::MovePositionOutsideChar(int pos, int moveDir, bool checkLineEnd) {
	//Platform::DebugPrintf("NoCRLF %d %d\n", pos, moveDir);
	// If out of range, just return minimum/maximum value.
	if (pos <= 0)
		return 0;
	if (pos >= Length())
		return Length();

	// PLATFORM_ASSERT(pos > 0 && pos < Length());
	if (checkLineEnd && IsCrLf(pos - 1)) {
		if (moveDir > 0)
			return pos + 1;
		else
			return pos - 1;
	}

	// Not between CR and LF

	if (dbcsCodePage) {
		if (SC_CP_UTF8 == dbcsCodePage) {
			unsigned char ch = static_cast<unsigned char>(cb.CharAt(pos));
			int startUTF = pos;
			int endUTF = pos;
			if (IsTrailByte(ch) && InGoodUTF8(pos, startUTF, endUTF)) {
				// ch is a trail byte within a UTF-8 character
				if (moveDir > 0)
					pos = endUTF;
				else
					pos = startUTF;
			}
		} else {
			// Anchor DBCS calculations at start of line because start of line can
			// not be a DBCS trail byte.
			int posCheck = LineStart(LineFromPosition(pos));
			while (posCheck < pos) {
				char mbstr[maxBytesInDBCSCharacter+1];
				int i;
				for(i=0;i<Platform::DBCSCharMaxLength();i++) {
					mbstr[i] = cb.CharAt(posCheck+i);
				}
				mbstr[i] = '\0';

				int mbsize = Platform::DBCSCharLength(dbcsCodePage, mbstr);
				if (posCheck + mbsize == pos) {
					return pos;
				} else if (posCheck + mbsize > pos) {
					if (moveDir > 0) {
						return posCheck + mbsize;
					} else {
						return posCheck;
					}
				}
				posCheck += mbsize;
			}
		}
	}

	return pos;
}

void Document::ModifiedAt(int pos) {
	if (endStyled > pos)
		endStyled = pos;
}

void Document::CheckReadOnly() {
	if (cb.IsReadOnly() && enteredReadOnlyCount == 0) {
		enteredReadOnlyCount++;
		NotifyModifyAttempt();
		enteredReadOnlyCount--;
	}
}

// Document only modified by gateways DeleteChars, InsertString, Undo, Redo, and SetStyleAt.
// SetStyleAt does not change the persistent state of a document

bool Document::DeleteChars(int pos, int len) {
	if (len == 0)
		return false;
	if ((pos + len) > Length())
		return false;
	CheckReadOnly();
	if (enteredModification != 0) {
		return false;
	} else {
		enteredModification++;
		if (!cb.IsReadOnly()) {
			NotifyModified(
			    DocModification(
			        SC_MOD_BEFOREDELETE | SC_PERFORMED_USER,
			        pos, len,
			        0, 0));
			int prevLinesTotal = LinesTotal();
			bool startSavePoint = cb.IsSavePoint();
			bool startSequence = false;
			const char *text = cb.DeleteChars(pos, len, startSequence);
			if (startSavePoint && cb.IsCollectingUndo())
				NotifySavePoint(!startSavePoint);
			if ((pos < Length()) || (pos == 0))
				ModifiedAt(pos);
			else
				ModifiedAt(pos-1);
			NotifyModified(
			    DocModification(
			        SC_MOD_DELETETEXT | SC_PERFORMED_USER | (startSequence?SC_STARTACTION:0),
			        pos, len,
			        LinesTotal() - prevLinesTotal, text));
		}
		enteredModification--;
	}
	return !cb.IsReadOnly();
}

/**
 * Insert a string with a length.
 */
bool Document::InsertString(int position, const char *s, int insertLength) {
	if (insertLength <= 0) {
		return false;
	}
	CheckReadOnly();
	if (enteredModification != 0) {
		return false;
	} else {
		enteredModification++;
		if (!cb.IsReadOnly()) {
			NotifyModified(
			    DocModification(
			        SC_MOD_BEFOREINSERT | SC_PERFORMED_USER,
			        position, insertLength,
			        0, s));
			int prevLinesTotal = LinesTotal();
			bool startSavePoint = cb.IsSavePoint();
			bool startSequence = false;
			const char *text = cb.InsertString(position, s, insertLength, startSequence);
			if (startSavePoint && cb.IsCollectingUndo())
				NotifySavePoint(!startSavePoint);
			ModifiedAt(position);
			NotifyModified(
			    DocModification(
			        SC_MOD_INSERTTEXT | SC_PERFORMED_USER | (startSequence?SC_STARTACTION:0),
			        position, insertLength,
			        LinesTotal() - prevLinesTotal, text));
		}
		enteredModification--;
	}
	return !cb.IsReadOnly();
}

int Document::Undo() {
	int newPos = -1;
	CheckReadOnly();
	if (enteredModification == 0) {
		enteredModification++;
		if (!cb.IsReadOnly()) {
			bool startSavePoint = cb.IsSavePoint();
			bool multiLine = false;
			int steps = cb.StartUndo();
			//Platform::DebugPrintf("Steps=%d\n", steps);
			for (int step = 0; step < steps; step++) {
				const int prevLinesTotal = LinesTotal();
				const Action &action = cb.GetUndoStep();
				if (action.at == removeAction) {
					NotifyModified(DocModification(
									SC_MOD_BEFOREINSERT | SC_PERFORMED_UNDO, action));
				} else {
					NotifyModified(DocModification(
									SC_MOD_BEFOREDELETE | SC_PERFORMED_UNDO, action));
				}
				cb.PerformUndoStep();
				int cellPosition = action.position;
				ModifiedAt(cellPosition);
				newPos = cellPosition;

				int modFlags = SC_PERFORMED_UNDO;
				// With undo, an insertion action becomes a deletion notification
				if (action.at == removeAction) {
					newPos += action.lenData;
					modFlags |= SC_MOD_INSERTTEXT;
				} else {
					modFlags |= SC_MOD_DELETETEXT;
				}
				if (steps > 1)
					modFlags |= SC_MULTISTEPUNDOREDO;
				const int linesAdded = LinesTotal() - prevLinesTotal;
				if (linesAdded != 0)
					multiLine = true;
				if (step == steps - 1) {
					modFlags |= SC_LASTSTEPINUNDOREDO;
					if (multiLine)
						modFlags |= SC_MULTILINEUNDOREDO;
				}
				NotifyModified(DocModification(modFlags, cellPosition, action.lenData,
											   linesAdded, action.data));
			}

			bool endSavePoint = cb.IsSavePoint();
			if (startSavePoint != endSavePoint)
				NotifySavePoint(endSavePoint);
		}
		enteredModification--;
	}
	return newPos;
}

int Document::Redo() {
	int newPos = -1;
	CheckReadOnly();
	if (enteredModification == 0) {
		enteredModification++;
		if (!cb.IsReadOnly()) {
			bool startSavePoint = cb.IsSavePoint();
			bool multiLine = false;
			int steps = cb.StartRedo();
			for (int step = 0; step < steps; step++) {
				const int prevLinesTotal = LinesTotal();
				const Action &action = cb.GetRedoStep();
				if (action.at == insertAction) {
					NotifyModified(DocModification(
									SC_MOD_BEFOREINSERT | SC_PERFORMED_REDO, action));
				} else {
					NotifyModified(DocModification(
									SC_MOD_BEFOREDELETE | SC_PERFORMED_REDO, action));
				}
				cb.PerformRedoStep();
				ModifiedAt(action.position);
				newPos = action.position;

				int modFlags = SC_PERFORMED_REDO;
				if (action.at == insertAction) {
					newPos += action.lenData;
					modFlags |= SC_MOD_INSERTTEXT;
				} else {
					modFlags |= SC_MOD_DELETETEXT;
				}
				if (steps > 1)
					modFlags |= SC_MULTISTEPUNDOREDO;
				const int linesAdded = LinesTotal() - prevLinesTotal;
				if (linesAdded != 0)
					multiLine = true;
				if (step == steps - 1) {
					modFlags |= SC_LASTSTEPINUNDOREDO;
					if (multiLine)
						modFlags |= SC_MULTILINEUNDOREDO;
				}
				NotifyModified(
					DocModification(modFlags, action.position, action.lenData,
									linesAdded, action.data));
			}

			bool endSavePoint = cb.IsSavePoint();
			if (startSavePoint != endSavePoint)
				NotifySavePoint(endSavePoint);
		}
		enteredModification--;
	}
	return newPos;
}

/**
 * Insert a single character.
 */
bool Document::InsertChar(int pos, char ch) {
	char chs[1];
	chs[0] = ch;
	return InsertString(pos, chs, 1);
}

/**
 * Insert a null terminated string.
 */
bool Document::InsertCString(int position, const char *s) {
	return InsertString(position, s, strlen(s));
}

void Document::ChangeChar(int pos, char ch) {
	DeleteChars(pos, 1);
	InsertChar(pos, ch);
}

void Document::DelChar(int pos) {
	DeleteChars(pos, LenChar(pos));
}

void Document::DelCharBack(int pos) {
	if (pos <= 0) {
		return;
	} else if (IsCrLf(pos - 2)) {
		DeleteChars(pos - 2, 2);
	} else if (dbcsCodePage) {
		int startChar = MovePositionOutsideChar(pos - 1, -1, false);
		DeleteChars(startChar, pos - startChar);
	} else {
		DeleteChars(pos - 1, 1);
	}
}

static bool isindentchar(char ch) {
	return (ch == ' ') || (ch == '\t');
}

static int NextTab(int pos, int tabSize) {
	return ((pos / tabSize) + 1) * tabSize;
}

static void CreateIndentation(char *linebuf, int length, int indent, int tabSize, bool insertSpaces) {
	length--;	// ensure space for \0
	if (!insertSpaces) {
		while ((indent >= tabSize) && (length > 0)) {
			*linebuf++ = '\t';
			indent -= tabSize;
			length--;
		}
	}
	while ((indent > 0) && (length > 0)) {
		*linebuf++ = ' ';
		indent--;
		length--;
	}
	*linebuf = '\0';
}

int Document::GetLineIndentation(int line) {
	int indent = 0;
	if ((line >= 0) && (line < LinesTotal())) {
		int lineStart = LineStart(line);
		int length = Length();
		for (int i = lineStart;i < length;i++) {
			char ch = cb.CharAt(i);
			if (ch == ' ')
				indent++;
			else if (ch == '\t')
				indent = NextTab(indent, tabInChars);
			else
				return indent;
		}
	}
	return indent;
}

void Document::SetLineIndentation(int line, int indent) {
	int indentOfLine = GetLineIndentation(line);
	if (indent < 0)
		indent = 0;
	if (indent != indentOfLine) {
		char linebuf[1000];
		CreateIndentation(linebuf, sizeof(linebuf), indent, tabInChars, !useTabs);
		int thisLineStart = LineStart(line);
		int indentPos = GetLineIndentPosition(line);
		BeginUndoAction();
		DeleteChars(thisLineStart, indentPos - thisLineStart);
		InsertCString(thisLineStart, linebuf);
		EndUndoAction();
	}
}

int Document::GetLineIndentPosition(int line) const {
	if (line < 0)
		return 0;
	int pos = LineStart(line);
	int length = Length();
	while ((pos < length) && isindentchar(cb.CharAt(pos))) {
		pos++;
	}
	return pos;
}

int Document::GetColumn(int pos) {
	int column = 0;
	int line = LineFromPosition(pos);
	if ((line >= 0) && (line < LinesTotal())) {
		for (int i = LineStart(line);i < pos;) {
			char ch = cb.CharAt(i);
			if (ch == '\t') {
				column = NextTab(column, tabInChars);
				i++;
			} else if (ch == '\r') {
				return column;
			} else if (ch == '\n') {
				return column;
			} else if (i >= Length()) {
				return column;
			} else {
				column++;
				i = MovePositionOutsideChar(i + 1, 1, false);
			}
		}
	}
	return column;
}

int Document::FindColumn(int line, int column) {
	int position = LineStart(line);
	int columnCurrent = 0;
	if ((line >= 0) && (line < LinesTotal())) {
		while ((columnCurrent < column) && (position < Length())) {
			char ch = cb.CharAt(position);
			if (ch == '\t') {
				columnCurrent = NextTab(columnCurrent, tabInChars);
				position++;
			} else if (ch == '\r') {
				return position;
			} else if (ch == '\n') {
				return position;
			} else {
				columnCurrent++;
				position = MovePositionOutsideChar(position + 1, 1, false);
			}
		}
	}
	return position;
}

void Document::Indent(bool forwards, int lineBottom, int lineTop) {
	// Dedent - suck white space off the front of the line to dedent by equivalent of a tab
	for (int line = lineBottom; line >= lineTop; line--) {
		int indentOfLine = GetLineIndentation(line);
		if (forwards) {
			if (LineStart(line) < LineEnd(line)) {
				SetLineIndentation(line, indentOfLine + IndentSize());
			}
		} else {
			SetLineIndentation(line, indentOfLine - IndentSize());
		}
	}
}

// Convert line endings for a piece of text to a particular mode.
// Stop at len or when a NUL is found.
// Caller must delete the returned pointer.
char *Document::TransformLineEnds(int *pLenOut, const char *s, size_t len, int eolMode) {
	char *dest = new char[2 * len + 1];
	const char *sptr = s;
	char *dptr = dest;
	for (size_t i = 0; (i < len) && (*sptr != '\0'); i++) {
		if (*sptr == '\n' || *sptr == '\r') {
			if (eolMode == SC_EOL_CR) {
				*dptr++ = '\r';
			} else if (eolMode == SC_EOL_LF) {
				*dptr++ = '\n';
			} else { // eolMode == SC_EOL_CRLF
				*dptr++ = '\r';
				*dptr++ = '\n';
			}
			if ((*sptr == '\r') && (i+1 < len) && (*(sptr+1) == '\n')) {
				i++;
				sptr++;
			}
			sptr++;
		} else {
			*dptr++ = *sptr++;
		}
	}
	*dptr++ = '\0';
	*pLenOut = (dptr - dest) - 1;
	return dest;
}

void Document::ConvertLineEnds(int eolModeSet) {
	BeginUndoAction();

	for (int pos = 0; pos < Length(); pos++) {
		if (cb.CharAt(pos) == '\r') {
			if (cb.CharAt(pos + 1) == '\n') {
				// CRLF
				if (eolModeSet == SC_EOL_CR) {
					DeleteChars(pos + 1, 1); // Delete the LF
				} else if (eolModeSet == SC_EOL_LF) {
					DeleteChars(pos, 1); // Delete the CR
				} else {
					pos++;
				}
			} else {
				// CR
				if (eolModeSet == SC_EOL_CRLF) {
					InsertString(pos + 1, "\n", 1); // Insert LF
					pos++;
				} else if (eolModeSet == SC_EOL_LF) {
					InsertString(pos, "\n", 1); // Insert LF
					DeleteChars(pos + 1, 1); // Delete CR
				}
			}
		} else if (cb.CharAt(pos) == '\n') {
			// LF
			if (eolModeSet == SC_EOL_CRLF) {
				InsertString(pos, "\r", 1); // Insert CR
				pos++;
			} else if (eolModeSet == SC_EOL_CR) {
				InsertString(pos, "\r", 1); // Insert CR
				DeleteChars(pos + 1, 1); // Delete LF
			}
		}
	}

	EndUndoAction();
}

bool Document::IsWhiteLine(int line) const {
	int currentChar = LineStart(line);
	int endLine = LineEnd(line);
	while (currentChar < endLine) {
		if (cb.CharAt(currentChar) != ' ' && cb.CharAt(currentChar) != '\t') {
			return false;
		}
		++currentChar;
	}
	return true;
}

int Document::ParaUp(int pos) {
	int line = LineFromPosition(pos);
	line--;
	while (line >= 0 && IsWhiteLine(line)) { // skip empty lines
		line--;
	}
	while (line >= 0 && !IsWhiteLine(line)) { // skip non-empty lines
		line--;
	}
	line++;
	return LineStart(line);
}

int Document::ParaDown(int pos) {
	int line = LineFromPosition(pos);
	while (line < LinesTotal() && !IsWhiteLine(line)) { // skip non-empty lines
		line++;
	}
	while (line < LinesTotal() && IsWhiteLine(line)) { // skip empty lines
		line++;
	}
	if (line < LinesTotal())
		return LineStart(line);
	else // end of a document
		return LineEnd(line-1);
}

CharClassify::cc Document::WordCharClass(unsigned char ch) {
	if ((SC_CP_UTF8 == dbcsCodePage) && (ch >= 0x80))
		return CharClassify::ccWord;
	return charClass.GetClass(ch);
}

/**
 * Used by commmands that want to select whole words.
 * Finds the start of word at pos when delta < 0 or the end of the word when delta >= 0.
 */
int Document::ExtendWordSelect(int pos, int delta, bool onlyWordCharacters) {
	CharClassify::cc ccStart = CharClassify::ccWord;
	if (delta < 0) {
		if (!onlyWordCharacters)
			ccStart = WordCharClass(cb.CharAt(pos-1));
		while (pos > 0 && (WordCharClass(cb.CharAt(pos - 1)) == ccStart))
			pos--;
	} else {
		if (!onlyWordCharacters && pos < Length())
			ccStart = WordCharClass(cb.CharAt(pos));
		while (pos < (Length()) && (WordCharClass(cb.CharAt(pos)) == ccStart))
			pos++;
	}
	return MovePositionOutsideChar(pos, delta);
}

/**
 * Find the start of the next word in either a forward (delta >= 0) or backwards direction
 * (delta < 0).
 * This is looking for a transition between character classes although there is also some
 * additional movement to transit white space.
 * Used by cursor movement by word commands.
 */
int Document::NextWordStart(int pos, int delta) {
	if (delta < 0) {
		while (pos > 0 && (WordCharClass(cb.CharAt(pos - 1)) == CharClassify::ccSpace))
			pos--;
		if (pos > 0) {
			CharClassify::cc ccStart = WordCharClass(cb.CharAt(pos-1));
			while (pos > 0 && (WordCharClass(cb.CharAt(pos - 1)) == ccStart)) {
				pos--;
			}
		}
	} else {
		CharClassify::cc ccStart = WordCharClass(cb.CharAt(pos));
		while (pos < (Length()) && (WordCharClass(cb.CharAt(pos)) == ccStart))
			pos++;
		while (pos < (Length()) && (WordCharClass(cb.CharAt(pos)) == CharClassify::ccSpace))
			pos++;
	}
	return pos;
}

/**
 * Find the end of the next word in either a forward (delta >= 0) or backwards direction
 * (delta < 0).
 * This is looking for a transition between character classes although there is also some
 * additional movement to transit white space.
 * Used by cursor movement by word commands.
 */
int Document::NextWordEnd(int pos, int delta) {
	if (delta < 0) {
		if (pos > 0) {
			CharClassify::cc ccStart = WordCharClass(cb.CharAt(pos-1));
			if (ccStart != CharClassify::ccSpace) {
				while (pos > 0 && WordCharClass(cb.CharAt(pos - 1)) == ccStart) {
					pos--;
				}
			}
			while (pos > 0 && WordCharClass(cb.CharAt(pos - 1)) == CharClassify::ccSpace) {
				pos--;
			}
		}
	} else {
		while (pos < Length() && WordCharClass(cb.CharAt(pos)) == CharClassify::ccSpace) {
			pos++;
		}
		if (pos < Length()) {
			CharClassify::cc ccStart = WordCharClass(cb.CharAt(pos));
			while (pos < Length() && WordCharClass(cb.CharAt(pos)) == ccStart) {
				pos++;
			}
		}
	}
	return pos;
}

/**
 * Check that the character at the given position is a word or punctuation character and that
 * the previous character is of a different character class.
 */
bool Document::IsWordStartAt(int pos) {
	if (pos > 0) {
		CharClassify::cc ccPos = WordCharClass(CharAt(pos));
		return (ccPos == CharClassify::ccWord || ccPos == CharClassify::ccPunctuation) &&
			(ccPos != WordCharClass(CharAt(pos - 1)));
	}
	return true;
}

/**
 * Check that the character at the given position is a word or punctuation character and that
 * the next character is of a different character class.
 */
bool Document::IsWordEndAt(int pos) {
	if (pos < Length()) {
		CharClassify::cc ccPrev = WordCharClass(CharAt(pos-1));
		return (ccPrev == CharClassify::ccWord || ccPrev == CharClassify::ccPunctuation) &&
			(ccPrev != WordCharClass(CharAt(pos)));
	}
	return true;
}

/**
 * Check that the given range is has transitions between character classes at both
 * ends and where the characters on the inside are word or punctuation characters.
 */
bool Document::IsWordAt(int start, int end) {
	return IsWordStartAt(start) && IsWordEndAt(end);
}

// The comparison and case changing functions here assume ASCII
// or extended ASCII such as the normal Windows code page.

//Vitaliy
// NOTE: this function is called for non-Unicode characters only!
//       ( i.e. when (!dbcsCodePage || isascii(ch)) )
static inline char MakeUpperCase(char ch) {
    if (ch >= 'A' && ch <= 'Z')
        return ch;
    else if (ch >= 'a' && ch <= 'z')
        return static_cast<char>(ch - 'a' + 'A');
    else
        return Platform_MakeUpperChar(ch);
}



// NOTE: this function is called for non-Unicode characters only!
//       ( i.e. when (!dbcsCodePage || isascii(ch)) )
static inline char MakeLowerCase(char ch) {
    if (ch >= 'a' && ch <= 'z')
        return ch;
    else if (ch >= 'A' && ch <= 'Z')
        return static_cast<char>(ch - 'A' + 'a');
    else
        return Platform_MakeLowerChar(ch);
}
//yilatiV

// Define a way for the Regular Expression code to access the document
class DocumentIndexer : public CharacterIndexer {
	Document *pdoc;
	int end;
public:
	DocumentIndexer(Document *pdoc_, int end_) :
		pdoc(pdoc_), end(end_) {
	}

	virtual ~DocumentIndexer() {
	}

	virtual char CharAt(int index) {
		if (index < 0 || index >= end)
			return 0;
		else
			return pdoc->CharAt(index);
	}
};

/**
 * Find text in document, supporting both forward and backward
 * searches (just pass minPos > maxPos to do a backward search)
 * Has not been tested with backwards DBCS searches yet.
 */
long Document::FindText(int minPos, int maxPos, const char *s,
                        bool caseSensitive, bool word, bool wordStart, bool regExp, bool posix,
                        int *length) {
	if (regExp) {
		if (!pre)
			pre = new RESearch(&charClass);
		if (!pre)
			return -1;

		int increment = (minPos <= maxPos) ? 1 : -1;

		int startPos = minPos;
		int endPos = maxPos;

		// Range endpoints should not be inside DBCS characters, but just in case, move them.
		startPos = MovePositionOutsideChar(startPos, 1, false);
		endPos = MovePositionOutsideChar(endPos, 1, false);

		const char *errmsg = pre->Compile(s, *length, caseSensitive, posix);
		if (errmsg) {
			return -1;
		}
		// Find a variable in a property file: \$(\([A-Za-z0-9_.]+\))
		// Replace first '.' with '-' in each property file variable reference:
		//     Search: \$(\([A-Za-z0-9_-]+\)\.\([A-Za-z0-9_.]+\))
		//     Replace: $(\1-\2)
		int lineRangeStart = LineFromPosition(startPos);
		int lineRangeEnd = LineFromPosition(endPos);
		if ((increment == 1) &&
			(startPos >= LineEnd(lineRangeStart)) &&
			(lineRangeStart < lineRangeEnd)) {
			// the start position is at end of line or between line end characters.
			lineRangeStart++;
			startPos = LineStart(lineRangeStart);
		}
		int pos = -1;
		int lenRet = 0;
		char searchEnd = s[*length - 1];
		int lineRangeBreak = lineRangeEnd + increment;
		for (int line = lineRangeStart; line != lineRangeBreak; line += increment) {
			int startOfLine = LineStart(line);
			int endOfLine = LineEnd(line);
			if (increment == 1) {
				if (line == lineRangeStart) {
					if ((startPos != startOfLine) && (s[0] == '^'))
						continue;	// Can't match start of line if start position after start of line
					startOfLine = startPos;
				}
				if (line == lineRangeEnd) {
					if ((endPos != endOfLine) && (searchEnd == '$'))
						continue;	// Can't match end of line if end position before end of line
					endOfLine = endPos;
				}
			} else {
				if (line == lineRangeEnd) {
					if ((endPos != startOfLine) && (s[0] == '^'))
						continue;	// Can't match start of line if end position after start of line
					startOfLine = endPos;
				}
				if (line == lineRangeStart) {
					if ((startPos != endOfLine) && (searchEnd == '$'))
						continue;	// Can't match end of line if start position before end of line
					endOfLine = startPos;
				}
			}

			DocumentIndexer di(this, endOfLine);
			int success = pre->Execute(di, startOfLine, endOfLine);
			if (success) {
				pos = pre->bopat[0];
				lenRet = pre->eopat[0] - pre->bopat[0];
				if (increment == -1) {
					// Check for the last match on this line.
					int repetitions = 1000;	// Break out of infinite loop
					while (success && (pre->eopat[0] <= endOfLine) && (repetitions--)) {
						success = pre->Execute(di, pos+1, endOfLine);
						if (success) {
							if (pre->eopat[0] <= minPos) {
								pos = pre->bopat[0];
								lenRet = pre->eopat[0] - pre->bopat[0];
							} else {
								success = 0;
							}
						}
					}
				}
				break;
			}
		}
		*length = lenRet;
		return pos;

	} else {

		bool forward = minPos <= maxPos;
		int increment = forward ? 1 : -1;

		// Range endpoints should not be inside DBCS characters, but just in case, move them.
		int startPos = MovePositionOutsideChar(minPos, increment, false);
		int endPos = MovePositionOutsideChar(maxPos, increment, false);

		// Compute actual search ranges needed
		int lengthFind = *length;
		if (lengthFind == -1)
			lengthFind = static_cast<int>(strlen(s));
		int endSearch = endPos;
		if (startPos <= endPos) {
			endSearch = endPos - lengthFind + 1;
		}
		//Platform::DebugPrintf("Find %d %d %s %d\n", startPos, endPos, ft->lpstrText, lengthFind);
		char firstChar = s[0];
		wchar_t* ws_upr = NULL;
		int ws_len = 0;
		char str[8];
		wchar_t wstr[4];
		if (!caseSensitive && !dbcsCodePage)
			firstChar = static_cast<char>(MakeUpperCase(firstChar));
		int pos = forward ? startPos : (startPos - 1);
		if (dbcsCodePage) {
			if (!caseSensitive && dbcsCodePage == SC_CP_UTF8) {
				ws_len = (int) UTF16Length(s, lengthFind);
				if (ws_len != lengthFind) {
					int ws_size = (((ws_len + 1) >> 4) + 1) << 4; // 16-chars alignment
					ws_upr = new wchar_t[ws_size]; 
					if (ws_upr != NULL) {
						UTF16FromUTF8(s, lengthFind, ws_upr, ws_size);
						ws_upr[ws_len] = 0;
						Platform_MakeUpperW(ws_upr, ws_len);
						// now ws_upr is UCS2 s in upper-case
					}
				} 
			}
			if (!caseSensitive && ws_upr == NULL) {
				// the text is Latin i.e. one character is one byte
				// ws_upr is NULL
				// BUT !!! ws_upr can be NULL if dbcsCodePage != SC_CP_UTF8
				// (also ws_upr = new wchar_t[ws_size] can be NULL)
				
				// for latin characters in non-UTF8 Unicode text
				// (thanks to Airix Z)
				if (isascii(firstChar))
					firstChar = static_cast<char>(MakeUpperCase(firstChar));
			}
			if (pos >= 0)
				pos = MovePositionOutsideChar(pos, increment, false);
		}
		while (forward ? (pos < endSearch) : (pos >= endSearch)) {
			char ch = CharAt(pos);
			if (caseSensitive) {
				if (ch == firstChar) {
					bool found = true;
					if (pos + lengthFind > Platform::Maximum(startPos, endPos)) found = false;
					for (int posMatch = 1; posMatch < lengthFind && found; posMatch++) {
						ch = CharAt(pos + posMatch);
						if (ch != s[posMatch])
							found = false;
					}
					if (found) {
						if ((!word && !wordStart) ||
						        word && IsWordAt(pos, pos + lengthFind) ||
						        wordStart && IsWordStartAt(pos))
							return pos;
					}
				}
			} else {
				bool bMatch = false;
				int  charLen = 0;

				if (!dbcsCodePage) {
					bMatch = (MakeUpperCase(ch) == firstChar);
				}
				else if (ws_upr == NULL) {
					// for latin characters in non-UTF8 Unicode text
					// (thanks to Airix Z)

					if (isascii(ch))
						bMatch = (MakeUpperCase(ch) == firstChar);
					else
						bMatch = (ch == firstChar);
				}
				else {
					// LenChar returns 2 for "\r\n"
					// this is wrong for UTF8 because "\r\n" 
					// is not one character with length=2
					charLen = IsCrLf(pos) ? 1 : LenChar(pos);
					for (int i = 0; i < charLen; i++) {
						str[i] = CharAt(pos+i);
					}
					str[charLen] = 0;
					UTF16FromUTF8(str, charLen, wstr, 2);
					wstr[1] = 0;
					Platform_MakeUpperW(wstr, 1);
					bMatch = (ws_upr[0] == wstr[0]);

					/*
					if (bMatch)
						MessageBoxA(NULL, "MatchCaseInsensitive is true!!!", "", 0);
					// OK
                        	        */
				}
				if (bMatch) {
					bool found = true;
					if (pos + lengthFind > Platform::Maximum(startPos, endPos)) found = false;
					if (!dbcsCodePage || ws_upr == NULL) {
						/*
						MessageBoxA(NULL, "Text is Latin (ws_upr == NULL)", "First character matched", 0);
                                                */
						for (int posMatch = 1; posMatch < lengthFind && found; posMatch++) {
							ch = CharAt(pos + posMatch);
							char ch2 = s[posMatch];
							// for latin characters in non-UTF8 Unicode text
							// (thanks to Airix Z)
                if (!dbcsCodePage || (isascii(ch) && isascii(ch2))) {
								if (MakeUpperCase(ch) != MakeUpperCase(ch2))
									found = false;
							} else {
								if (ch != ch2)
									found = false;
							}
						}
					} 
					else {
						int i1, i2;

						/*
						MessageBoxA(NULL, "first matched!!!", "", 0);
						// OK
                                                */
						i1 = 1;
						i2 = pos + charLen;
						while (found && i1 < ws_len) {
							// LenChar returns 2 for "\r\n"
							// this is wrong for UTF8 because "\r\n" 
							// is not one character with length=2
							charLen = IsCrLf(i2) ? 1 : LenChar(i2);
							for (int i = 0; i < charLen; i++) {
								str[i] = CharAt(i2+i);
							}
							str[charLen] = 0;
							UTF16FromUTF8(str, charLen, wstr, 2);
							wstr[1] = 0;
							Platform_MakeUpperW(wstr, 1);
							found = (ws_upr[i1] == wstr[0]);
							i1++;
							i2 += charLen;
						}
					}
					if (found) {
						if ((!word && !wordStart) ||
						    word && IsWordAt(pos, pos + lengthFind) ||
						    wordStart && IsWordStartAt(pos)) {
						        if (ws_upr != NULL) {
								delete [] ws_upr;
								ws_upr = NULL;
							}
							return pos;
						}
					}
				}
			}
			pos += increment;
			if (dbcsCodePage && (pos >= 0)) {
				// Ensure trying to match from start of character
				pos = MovePositionOutsideChar(pos, increment, false);
			}
		}
		if (ws_upr != NULL) {
			delete [] ws_upr;
			ws_upr = NULL;
		}
	}
	//Platform::DebugPrintf("Not found\n");
	return -1;
}

const char *Document::SubstituteByPosition(const char *text, int *length) {
	if (!pre)
		return 0;
	delete []substituted;
	substituted = 0;
	DocumentIndexer di(this, Length());
	if (!pre->GrabMatches(di))
		return 0;
	unsigned int lenResult = 0;
	for (int i = 0; i < *length; i++) {
		if (text[i] == '\\') {
			if (text[i + 1] >= '1' && text[i + 1] <= '9') {
				unsigned int patNum = text[i + 1] - '0';
				lenResult += pre->eopat[patNum] - pre->bopat[patNum];
				i++;
			} else {
				switch (text[i + 1]) {
				case 'a':
				case 'b':
				case 'f':
				case 'n':
				case 'r':
				case 't':
				case 'v':
					i++;
				}
				lenResult++;
			}
		} else {
			lenResult++;
		}
	}
	substituted = new char[lenResult + 1];
	if (!substituted)
		return 0;
	char *o = substituted;
	for (int j = 0; j < *length; j++) {
		if (text[j] == '\\') {
			if (text[j + 1] >= '1' && text[j + 1] <= '9') {
				unsigned int patNum = text[j + 1] - '0';
				unsigned int len = pre->eopat[patNum] - pre->bopat[patNum];
				if (pre->pat[patNum])	// Will be null if try for a match that did not occur
					memcpy(o, pre->pat[patNum], len);
				o += len;
				j++;
			} else {
				j++;
				switch (text[j]) {
				case 'a':
					*o++ = '\a';
					break;
				case 'b':
					*o++ = '\b';
					break;
				case 'f':
					*o++ = '\f';
					break;
				case 'n':
					*o++ = '\n';
					break;
				case 'r':
					*o++ = '\r';
					break;
				case 't':
					*o++ = '\t';
					break;
				case 'v':
					*o++ = '\v';
					break;
				default:
					*o++ = '\\';
					j--;
				}
			}
		} else {
			*o++ = text[j];
		}
	}
	*o = '\0';
	*length = lenResult;
	return substituted;
}

int Document::LinesTotal() const {
	return cb.Lines();
}

void Document::ChangeCase(Range r, bool makeUpperCase) {
	for (int pos = r.start; pos < r.end;) {
		int len = LenChar(pos);
		if (len == 1) {
			char ch = CharAt(pos);
			if (makeUpperCase) {
				if (IsLowerCase(ch)) {
					ChangeChar(pos, static_cast<char>(MakeUpperCase(ch)));
				}
			} else {
				if (IsUpperCase(ch)) {
					ChangeChar(pos, static_cast<char>(MakeLowerCase(ch)));
				}
			}
		}
		pos += len;
	}
}

void Document::SetDefaultCharClasses(bool includeWordClass) {
    charClass.SetDefaultCharClasses(includeWordClass);
}

void Document::SetCharClasses(const unsigned char *chars, CharClassify::cc newCharClass) {
    charClass.SetCharClasses(chars, newCharClass);
}

void Document::SetStylingBits(int bits) {
	stylingBits = bits;
	stylingBitsMask = (1 << stylingBits) - 1;
}

void Document::StartStyling(int position, char mask) {
	stylingMask = mask;
	endStyled = position;
}

bool Document::SetStyleFor(int length, char style) {
	if (enteredStyling != 0) {
		return false;
	} else {
		enteredStyling++;
		style &= stylingMask;
		int prevEndStyled = endStyled;
		if (cb.SetStyleFor(endStyled, length, style, stylingMask)) {
			DocModification mh(SC_MOD_CHANGESTYLE | SC_PERFORMED_USER,
			                   prevEndStyled, length);
			NotifyModified(mh);
		}
		endStyled += length;
		enteredStyling--;
		return true;
	}
}

bool Document::SetStyles(int length, char *styles) {
	if (enteredStyling != 0) {
		return false;
	} else {
		enteredStyling++;
		bool didChange = false;
		int startMod = 0;
		int endMod = 0;
		for (int iPos = 0; iPos < length; iPos++, endStyled++) {
			PLATFORM_ASSERT(endStyled < Length());
			if (cb.SetStyleAt(endStyled, styles[iPos], stylingMask)) {
				if (!didChange) {
					startMod = endStyled;
				}
				didChange = true;
				endMod = endStyled;
			}
		}
		if (didChange) {
			DocModification mh(SC_MOD_CHANGESTYLE | SC_PERFORMED_USER,
			                   startMod, endMod - startMod + 1);
			NotifyModified(mh);
		}
		enteredStyling--;
		return true;
	}
}

void Document::EnsureStyledTo(int pos) {
	if ((enteredStyling == 0) && (pos > GetEndStyled())) {
		IncrementStyleClock();
		// Ask the watchers to style, and stop as soon as one responds.
		for (int i = 0; pos > GetEndStyled() && i < lenWatchers; i++) {
			watchers[i].watcher->NotifyStyleNeeded(this, watchers[i].userData, pos);
		}
	}
}

int Document::SetLineState(int line, int state) { 
	int statePrevious = cb.SetLineState(line, state);
	if (state != statePrevious) {
		DocModification mh(SC_MOD_CHANGELINESTATE, 0, 0, 0, 0, line);
		NotifyModified(mh);
	}
	return statePrevious;
}

void Document::IncrementStyleClock() {
	styleClock = (styleClock + 1) % 0x100000;
}

void Document::DecorationFillRange(int position, int value, int fillLength) {
	if (decorations.FillRange(position, value, fillLength)) {
		DocModification mh(SC_MOD_CHANGEINDICATOR | SC_PERFORMED_USER,
							position, fillLength);
		NotifyModified(mh);
	}
}

bool Document::AddWatcher(DocWatcher *watcher, void *userData) {
	for (int i = 0; i < lenWatchers; i++) {
		if ((watchers[i].watcher == watcher) &&
		        (watchers[i].userData == userData))
			return false;
	}
	WatcherWithUserData *pwNew = new WatcherWithUserData[lenWatchers + 1];
	if (!pwNew)
		return false;
	for (int j = 0; j < lenWatchers; j++)
		pwNew[j] = watchers[j];
	pwNew[lenWatchers].watcher = watcher;
	pwNew[lenWatchers].userData = userData;
	delete []watchers;
	watchers = pwNew;
	lenWatchers++;
	return true;
}

bool Document::RemoveWatcher(DocWatcher *watcher, void *userData) {
	for (int i = 0; i < lenWatchers; i++) {
		if ((watchers[i].watcher == watcher) &&
		        (watchers[i].userData == userData)) {
			if (lenWatchers == 1) {
				delete []watchers;
				watchers = 0;
				lenWatchers = 0;
			} else {
				WatcherWithUserData *pwNew = new WatcherWithUserData[lenWatchers];
				if (!pwNew)
					return false;
				for (int j = 0; j < lenWatchers - 1; j++) {
					pwNew[j] = (j < i) ? watchers[j] : watchers[j + 1];
				}
				delete []watchers;
				watchers = pwNew;
				lenWatchers--;
			}
			return true;
		}
	}
	return false;
}

void Document::NotifyModifyAttempt() {
	for (int i = 0; i < lenWatchers; i++) {
		watchers[i].watcher->NotifyModifyAttempt(this, watchers[i].userData);
	}
}

void Document::NotifySavePoint(bool atSavePoint) {
	for (int i = 0; i < lenWatchers; i++) {
		watchers[i].watcher->NotifySavePoint(this, watchers[i].userData, atSavePoint);
	}
}

void Document::NotifyModified(DocModification mh) {
	if (mh.modificationType & SC_MOD_INSERTTEXT) {
		decorations.InsertSpace(mh.position, mh.length);
	} else if (mh.modificationType & SC_MOD_DELETETEXT) {
		decorations.DeleteRange(mh.position, mh.length);
	}
	for (int i = 0; i < lenWatchers; i++) {
		watchers[i].watcher->NotifyModified(this, mh, watchers[i].userData);
	}
}

bool Document::IsWordPartSeparator(char ch) {
	return (WordCharClass(ch) == CharClassify::ccWord) && IsPunctuation(ch);
}

int Document::WordPartLeft(int pos) {
	if (pos > 0) {
		--pos;
		char startChar = cb.CharAt(pos);
		if (IsWordPartSeparator(startChar)) {
			while (pos > 0 && IsWordPartSeparator(cb.CharAt(pos))) {
				--pos;
			}
		}
		if (pos > 0) {
			startChar = cb.CharAt(pos);
			--pos;
			if (IsLowerCase(startChar)) {
				while (pos > 0 && IsLowerCase(cb.CharAt(pos)))
					--pos;
				if (!IsUpperCase(cb.CharAt(pos)) && !IsLowerCase(cb.CharAt(pos)))
					++pos;
			} else if (IsUpperCase(startChar)) {
				while (pos > 0 && IsUpperCase(cb.CharAt(pos)))
					--pos;
				if (!IsUpperCase(cb.CharAt(pos)))
					++pos;
			} else if (IsADigit(startChar)) {
				while (pos > 0 && IsADigit(cb.CharAt(pos)))
					--pos;
				if (!IsADigit(cb.CharAt(pos)))
					++pos;
			} else if (IsPunctuation(startChar)) {
				while (pos > 0 && IsPunctuation(cb.CharAt(pos)))
					--pos;
				if (!IsPunctuation(cb.CharAt(pos)))
					++pos;
			} else if (isspacechar(startChar)) {
				while (pos > 0 && isspacechar(cb.CharAt(pos)))
					--pos;
				if (!isspacechar(cb.CharAt(pos)))
					++pos;
			} else if (!isascii(startChar)) {
				while (pos > 0 && !isascii(cb.CharAt(pos)))
					--pos;
				if (isascii(cb.CharAt(pos)))
					++pos;
			} else {
				++pos;
			}
		}
	}
	return pos;
}

int Document::WordPartRight(int pos) {
	char startChar = cb.CharAt(pos);
	int length = Length();
	if (IsWordPartSeparator(startChar)) {
		while (pos < length && IsWordPartSeparator(cb.CharAt(pos)))
			++pos;
		startChar = cb.CharAt(pos);
	}
	if (!isascii(startChar)) {
		while (pos < length && !isascii(cb.CharAt(pos)))
			++pos;
	} else if (IsLowerCase(startChar)) {
		while (pos < length && IsLowerCase(cb.CharAt(pos)))
			++pos;
	} else if (IsUpperCase(startChar)) {
		if (IsLowerCase(cb.CharAt(pos + 1))) {
			++pos;
			while (pos < length && IsLowerCase(cb.CharAt(pos)))
				++pos;
		} else {
			while (pos < length && IsUpperCase(cb.CharAt(pos)))
				++pos;
		}
		if (IsLowerCase(cb.CharAt(pos)) && IsUpperCase(cb.CharAt(pos - 1)))
			--pos;
	} else if (IsADigit(startChar)) {
		while (pos < length && IsADigit(cb.CharAt(pos)))
			++pos;
	} else if (IsPunctuation(startChar)) {
		while (pos < length && IsPunctuation(cb.CharAt(pos)))
			++pos;
	} else if (isspacechar(startChar)) {
		while (pos < length && isspacechar(cb.CharAt(pos)))
			++pos;
	} else {
		++pos;
	}
	return pos;
}

bool IsLineEndChar(char c) {
	return (c == '\n' || c == '\r');
}

int Document::ExtendStyleRange(int pos, int delta, bool singleLine) {
	int sStart = cb.StyleAt(pos);
	if (delta < 0) {
		while (pos > 0 && (cb.StyleAt(pos) == sStart) && (!singleLine || !IsLineEndChar(cb.CharAt(pos))) )
			pos--;
		pos++;
	} else {
		while (pos < (Length()) && (cb.StyleAt(pos) == sStart) && (!singleLine || !IsLineEndChar(cb.CharAt(pos))) )
			pos++;
	}
	return pos;
}

static char BraceOpposite(char ch) {
	switch (ch) {
	case '(':
		return ')';
	case ')':
		return '(';
	case '[':
		return ']';
	case ']':
		return '[';
	case '{':
		return '}';
	case '}':
		return '{';
	case '<':
		return '>';
	case '>':
		return '<';
	default:
		return '\0';
	}
}

// TODO: should be able to extend styled region to find matching brace
int Document::BraceMatch(int position, int /*maxReStyle*/) {
	char chBrace = CharAt(position);
	char chSeek = BraceOpposite(chBrace);
	if (chSeek == '\0')
		return - 1;
	char styBrace = static_cast<char>(StyleAt(position) & stylingBitsMask);
	int direction = -1;
	if (chBrace == '(' || chBrace == '[' || chBrace == '{' || chBrace == '<')
		direction = 1;
	int depth = 1;
	position = position + direction;
	while ((position >= 0) && (position < Length())) {
		position = MovePositionOutsideChar(position, direction);
		char chAtPos = CharAt(position);
		char styAtPos = static_cast<char>(StyleAt(position) & stylingBitsMask);
		if ((position > GetEndStyled()) || (styAtPos == styBrace)) {
			if (chAtPos == chBrace)
				depth++;
			if (chAtPos == chSeek)
				depth--;
			if (depth == 0)
				return position;
		}
		position = position + direction;
	}
	return - 1;
}
