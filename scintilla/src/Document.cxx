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
#include <assert.h>

#include <string>
#include <vector>

#include "Platform.h"

#include "ILexer.h"
#include "Scintilla.h"

#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "CellBuffer.h"
#include "PerLine.h"
#include "CharClassify.h"
#include "CharacterSet.h"
#include "Decoration.h"
#include "Document.h"
#include "RESearch.h"
#include "UniConversion.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

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

void LexInterface::Colourise(int start, int end) {
	ElapsedTime et;
	if (pdoc && instance && !performingStyle) {
		// Protect against reentrance, which may occur, for example, when
		// fold points are discovered while performing styling and the folding
		// code looks for child lines which may trigger styling.
		performingStyle = true;

		int lengthDoc = pdoc->Length();
		if (end == -1)
			end = lengthDoc;
		int len = end - start;

		PLATFORM_ASSERT(len >= 0);
		PLATFORM_ASSERT(start + len <= lengthDoc);

		int styleStart = 0;
		if (start > 0)
			styleStart = pdoc->StyleAt(start - 1) & pdoc->stylingBitsMask;

		if (len > 0) {
			instance->Lex(start, len, styleStart, pdoc);
			instance->Fold(start, len, styleStart, pdoc);
		}

		performingStyle = false;
	}
}

Document::Document() {
	refCount = 0;
#ifdef __unix__
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
	regex = 0;

	perLineData[ldMarkers] = new LineMarkers();
	perLineData[ldLevels] = new LineLevels();
	perLineData[ldState] = new LineState();
	perLineData[ldMargin] = new LineAnnotation();
	perLineData[ldAnnotation] = new LineAnnotation();

	cb.SetPerLine(this);

	pli = 0;
}

Document::~Document() {
	for (int i = 0; i < lenWatchers; i++) {
		watchers[i].watcher->NotifyDeleted(this, watchers[i].userData);
	}
	delete []watchers;
	for (int j=0; j<ldSize; j++) {
		delete perLineData[j];
		perLineData[j] = 0;
	}
	watchers = 0;
	lenWatchers = 0;
	delete regex;
	regex = 0;
	delete pli;
	pli = 0;
}

void Document::Init() {
	for (int j=0; j<ldSize; j++) {
		if (perLineData[j])
			perLineData[j]->Init();
	}
}

void Document::InsertLine(int line) {
	for (int j=0; j<ldSize; j++) {
		if (perLineData[j])
			perLineData[j]->InsertLine(line);
	}
}

void Document::RemoveLine(int line) {
	for (int j=0; j<ldSize; j++) {
		if (perLineData[j])
			perLineData[j]->RemoveLine(line);
	}
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

int Document::GetMark(int line) {
	return static_cast<LineMarkers *>(perLineData[ldMarkers])->MarkValue(line);
}

int Document::AddMark(int line, int markerNum) {
	if (line >= 0 && line <= LinesTotal()) {
		int prev = static_cast<LineMarkers *>(perLineData[ldMarkers])->
			AddMark(line, markerNum, LinesTotal());
		DocModification mh(SC_MOD_CHANGEMARKER, LineStart(line), 0, 0, 0, line);
		NotifyModified(mh);
		return prev;
	} else {
		return 0;
	}
}

void Document::AddMarkSet(int line, int valueSet) {
	if (line < 0 || line > LinesTotal()) {
		return;
	}
	unsigned int m = valueSet;
	for (int i = 0; m; i++, m >>= 1)
		if (m & 1)
			static_cast<LineMarkers *>(perLineData[ldMarkers])->
				AddMark(line, i, LinesTotal());
	DocModification mh(SC_MOD_CHANGEMARKER, LineStart(line), 0, 0, 0, line);
	NotifyModified(mh);
}

void Document::DeleteMark(int line, int markerNum) {
	static_cast<LineMarkers *>(perLineData[ldMarkers])->DeleteMark(line, markerNum, false);
	DocModification mh(SC_MOD_CHANGEMARKER, LineStart(line), 0, 0, 0, line);
	NotifyModified(mh);
}

void Document::DeleteMarkFromHandle(int markerHandle) {
	static_cast<LineMarkers *>(perLineData[ldMarkers])->DeleteMarkFromHandle(markerHandle);
	DocModification mh(SC_MOD_CHANGEMARKER, 0, 0, 0, 0);
	mh.line = -1;
	NotifyModified(mh);
}

void Document::DeleteAllMarks(int markerNum) {
	bool someChanges = false;
	for (int line = 0; line < LinesTotal(); line++) {
		if (static_cast<LineMarkers *>(perLineData[ldMarkers])->DeleteMark(line, markerNum, true))
			someChanges = true;
	}
	if (someChanges) {
		DocModification mh(SC_MOD_CHANGEMARKER, 0, 0, 0, 0);
		mh.line = -1;
		NotifyModified(mh);
	}
}

int Document::LineFromHandle(int markerHandle) {
	return static_cast<LineMarkers *>(perLineData[ldMarkers])->LineFromHandle(markerHandle);
}

int SCI_METHOD Document::LineStart(int line) const {
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

void SCI_METHOD Document::SetErrorStatus(int status) {
	// Tell the watchers the lexer has changed.
	for (int i = 0; i < lenWatchers; i++) {
		watchers[i].watcher->NotifyErrorOccurred(this, watchers[i].userData, status);
	}
}

int SCI_METHOD Document::LineFromPosition(int pos) const {
	return cb.LineFromPosition(pos);
}

int Document::LineEndPosition(int position) const {
	return LineEnd(LineFromPosition(position));
}

bool Document::IsLineEndPosition(int position) const {
	return LineEnd(LineFromPosition(position)) == position;
}

int Document::VCHomePosition(int position) const {
	int line = LineFromPosition(position);
	int startPosition = LineStart(line);
	int endLine = LineEnd(line);
	int startText = startPosition;
	while (startText < endLine && (cb.CharAt(startText) == ' ' || cb.CharAt(startText) == '\t'))
		startText++;
	if (position == startText)
		return startPosition;
	else
		return startText;
}

int SCI_METHOD Document::SetLevel(int line, int level) {
	int prev = static_cast<LineLevels *>(perLineData[ldLevels])->SetLevel(line, level, LinesTotal());
	if (prev != level) {
		DocModification mh(SC_MOD_CHANGEFOLD | SC_MOD_CHANGEMARKER,
		                   LineStart(line), 0, 0, 0, line);
		mh.foldLevelNow = level;
		mh.foldLevelPrev = prev;
		NotifyModified(mh);
	}
	return prev;
}

int SCI_METHOD Document::GetLevel(int line) const {
	return static_cast<LineLevels *>(perLineData[ldLevels])->GetLevel(line);
}

void Document::ClearLevels() {
	static_cast<LineLevels *>(perLineData[ldLevels])->ClearLevels();
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
		return IsDBCSLeadByte(cb.CharAt(pos)) ? 2 : 1;
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

bool Document::InGoodUTF8(int pos, int &start, int &end) const {
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
			int posStartLine = LineStart(LineFromPosition(pos));
			if (pos == posStartLine)
				return pos;

			// Step back until a non-lead-byte is found.
			int posCheck = pos;
			while ((posCheck > posStartLine) && IsDBCSLeadByte(cb.CharAt(posCheck-1)))
				posCheck--;

			// Check from known start of character.
			while (posCheck < pos) {
				int mbsize = IsDBCSLeadByte(cb.CharAt(posCheck)) ? 2 : 1;
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

// NextPosition moves between valid positions - it can not handle a position in the middle of a
// multi-byte character. It is used to iterate through text more efficiently than MovePositionOutsideChar.
// A \r\n pair is treated as two characters.
int Document::NextPosition(int pos, int moveDir) const {
	// If out of range, just return minimum/maximum value.
	int increment = (moveDir > 0) ? 1 : -1;
	if (pos + increment <= 0)
		return 0;
	if (pos + increment >= Length())
		return Length();

	if (dbcsCodePage) {
		if (SC_CP_UTF8 == dbcsCodePage) {
			pos += increment;
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
			if (moveDir > 0) {
				int mbsize = IsDBCSLeadByte(cb.CharAt(pos)) ? 2 : 1;
				pos += mbsize;
				if (pos > Length())
					pos = Length();
			} else {
				// Anchor DBCS calculations at start of line because start of line can
				// not be a DBCS trail byte.
				int posStartLine = LineStart(LineFromPosition(pos));
				// See http://msdn.microsoft.com/en-us/library/cc194792%28v=MSDN.10%29.aspx
				// http://msdn.microsoft.com/en-us/library/cc194790.aspx
				if ((pos - 1) <= posStartLine) {
					return pos - 1;
				} else if (IsDBCSLeadByte(cb.CharAt(pos - 1))) {
					// Must actually be trail byte
					return pos - 2;
				} else {
					// Otherwise, step back until a non-lead-byte is found.
					int posTemp = pos - 1;
					while (posStartLine <= --posTemp && IsDBCSLeadByte(cb.CharAt(posTemp)))
						;
					// Now posTemp+1 must point to the beginning of a character,
					// so figure out whether we went back an even or an odd
					// number of bytes and go back 1 or 2 bytes, respectively.
					return (pos - 1 - ((pos - posTemp) & 1));
				}
			}
		}
	} else {
		pos += increment;
	}

	return pos;
}

bool Document::NextCharacter(int &pos, int moveDir) {
	// Returns true if pos changed
	int posNext = NextPosition(pos, moveDir);
	if (posNext == pos) {
		return false;
	} else {
		pos = posNext;
		return true;
	}
}

int SCI_METHOD Document::CodePage() const {
	return dbcsCodePage;
}

bool SCI_METHOD Document::IsDBCSLeadByte(char ch) const {
	// Byte ranges found in Wikipedia articles with relevant search strings in each case
	unsigned char uch = static_cast<unsigned char>(ch);
	switch (dbcsCodePage) {
		case 932:
			// Shift_jis
			return ((uch >= 0x81) && (uch <= 0x9F)) ||
				((uch >= 0xE0) && (uch <= 0xEF));
		case 936:
			// GBK
			return (uch >= 0x81) && (uch <= 0xFE);
		case 949:
			// Korean Wansung KS C-5601-1987
			return (uch >= 0x81) && (uch <= 0xFE);
		case 950:
			// Big5
			return (uch >= 0x81) && (uch <= 0xFE);
		case 1361:
			// Korean Johab KS C-5601-1992
			return
				((uch >= 0x84) && (uch <= 0xD3)) ||
				((uch >= 0xD8) && (uch <= 0xDE)) ||
				((uch >= 0xE0) && (uch <= 0xF9));
	}
	return false;
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
				} else if (action.at == containerAction) {
					DocModification dm(SC_MOD_CONTAINER | SC_PERFORMED_UNDO);
					dm.token = action.position;
					NotifyModified(dm);
				} else {
					NotifyModified(DocModification(
									SC_MOD_BEFOREDELETE | SC_PERFORMED_UNDO, action));
				}
				cb.PerformUndoStep();
				int cellPosition = action.position;
				if (action.at != containerAction) {
					ModifiedAt(cellPosition);
					newPos = cellPosition;
				}

				int modFlags = SC_PERFORMED_UNDO;
				// With undo, an insertion action becomes a deletion notification
				if (action.at == removeAction) {
					newPos += action.lenData;
					modFlags |= SC_MOD_INSERTTEXT;
				} else if (action.at == insertAction) {
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
				} else if (action.at == containerAction) {
					DocModification dm(SC_MOD_CONTAINER | SC_PERFORMED_REDO);
					dm.token = action.position;
					NotifyModified(dm);
				} else {
					NotifyModified(DocModification(
									SC_MOD_BEFOREDELETE | SC_PERFORMED_REDO, action));
				}
				cb.PerformRedoStep();
				if (action.at != containerAction) {
					ModifiedAt(action.position);
					newPos = action.position;
				}

				int modFlags = SC_PERFORMED_REDO;
				if (action.at == insertAction) {
					newPos += action.lenData;
					modFlags |= SC_MOD_INSERTTEXT;
				} else if (action.at == removeAction) {
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
		int startChar = NextPosition(pos, -1);
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

int SCI_METHOD Document::GetLineIndentation(int line) {
	int indent = 0;
	if ((line >= 0) && (line < LinesTotal())) {
		int lineStart = LineStart(line);
		int length = Length();
		for (int i = lineStart; i < length; i++) {
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
		UndoGroup ug(this);
		DeleteChars(thisLineStart, indentPos - thisLineStart);
		InsertCString(thisLineStart, linebuf);
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
		for (int i = LineStart(line); i < pos;) {
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
				i = NextPosition(i, 1);
			}
		}
	}
	return column;
}

int Document::FindColumn(int line, int column) {
	int position = LineStart(line);
	if ((line >= 0) && (line < LinesTotal())) {
		int columnCurrent = 0;
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
				position = NextPosition(position, 1);
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
	UndoGroup ug(this);

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
	return MovePositionOutsideChar(pos, delta, true);
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

static inline char MakeLowerCase(char ch) {
	if (ch < 'A' || ch > 'Z')
		return ch;
	else
		return static_cast<char>(ch - 'A' + 'a');
}

static bool GoodTrailByte(int v) {
	return (v >= 0x80) && (v < 0xc0);
}

size_t Document::ExtractChar(int pos, char *bytes) {
	unsigned char ch = static_cast<unsigned char>(cb.CharAt(pos));
	size_t widthChar = UTF8CharLength(ch);
	bytes[0] = ch;
	for (size_t i=1; i<widthChar; i++) {
		bytes[i] = cb.CharAt(pos+i);
		if (!GoodTrailByte(static_cast<unsigned char>(bytes[i]))) { // Bad byte
			widthChar = 1;
		}
	}
	return widthChar;
}

CaseFolderTable::CaseFolderTable() {
	for (size_t iChar=0; iChar<sizeof(mapping); iChar++) {
		mapping[iChar] = static_cast<char>(iChar);
	}
}

CaseFolderTable::~CaseFolderTable() {
}

size_t CaseFolderTable::Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) {
	if (lenMixed > sizeFolded) {
		return 0;
	} else {
		for (size_t i=0; i<lenMixed; i++) {
			folded[i] = mapping[static_cast<unsigned char>(mixed[i])];
		}
		return lenMixed;
	}
}

void CaseFolderTable::SetTranslation(char ch, char chTranslation) {
	mapping[static_cast<unsigned char>(ch)] = chTranslation;
}

void CaseFolderTable::StandardASCII() {
	for (size_t iChar=0; iChar<sizeof(mapping); iChar++) {
		if (iChar >= 'A' && iChar <= 'Z') {
			mapping[iChar] = static_cast<char>(iChar - 'A' + 'a');
		} else {
			mapping[iChar] = static_cast<char>(iChar);
		}
	}
}

bool Document::MatchesWordOptions(bool word, bool wordStart, int pos, int length) {
	return (!word && !wordStart) ||
			(word && IsWordAt(pos, pos + length)) ||
			(wordStart && IsWordStartAt(pos));
}

/**
 * Find text in document, supporting both forward and backward
 * searches (just pass minPos > maxPos to do a backward search)
 * Has not been tested with backwards DBCS searches yet.
 */
long Document::FindText(int minPos, int maxPos, const char *search,
                        bool caseSensitive, bool word, bool wordStart, bool regExp, int flags,
                        int *length, CaseFolder *pcf) {
	if (*length <= 0)
		return minPos;
	if (regExp) {
		if (!regex)
			regex = CreateRegexSearch(&charClass);
		return regex->FindText(this, minPos, maxPos, search, caseSensitive, word, wordStart, flags, length);
	} else {

		const bool forward = minPos <= maxPos;
		const int increment = forward ? 1 : -1;

		// Range endpoints should not be inside DBCS characters, but just in case, move them.
		const int startPos = MovePositionOutsideChar(minPos, increment, false);
		const int endPos = MovePositionOutsideChar(maxPos, increment, false);

		// Compute actual search ranges needed
		const int lengthFind = (*length == -1) ? static_cast<int>(strlen(search)) : *length;
		const int endSearch = (startPos <= endPos) ? endPos - lengthFind + 1 : endPos;

		//Platform::DebugPrintf("Find %d %d %s %d\n", startPos, endPos, ft->lpstrText, lengthFind);
		const int limitPos = Platform::Maximum(startPos, endPos);
		int pos = startPos;
		if (!forward) {
			// Back all of a character
			pos = NextPosition(pos, increment);
		}
		if (caseSensitive) {
			while (forward ? (pos < endSearch) : (pos >= endSearch)) {
				bool found = (pos + lengthFind) <= limitPos;
				for (int indexSearch = 0; (indexSearch < lengthFind) && found; indexSearch++) {
					found = CharAt(pos + indexSearch) == search[indexSearch];
				}
				if (found && MatchesWordOptions(word, wordStart, pos, lengthFind)) {
					return pos;
				}
				if (!NextCharacter(pos, increment))
					break;
			}
		} else if (SC_CP_UTF8 == dbcsCodePage) {
			const size_t maxBytesCharacter = 4;
			const size_t maxFoldingExpansion = 4;
			std::vector<char> searchThing(lengthFind * maxBytesCharacter * maxFoldingExpansion + 1);
			const int lenSearch = pcf->Fold(&searchThing[0], searchThing.size(), search, lengthFind);
			while (forward ? (pos < endSearch) : (pos >= endSearch)) {
				int widthFirstCharacter = 0;
				int indexDocument = 0;
				int indexSearch = 0;
				bool characterMatches = true;
				while (characterMatches &&
					((pos + indexDocument) < limitPos) &&
					(indexSearch < lenSearch)) {
					char bytes[maxBytesCharacter + 1];
					bytes[maxBytesCharacter] = 0;
					const int widthChar = ExtractChar(pos + indexDocument, bytes);
					if (!widthFirstCharacter)
						widthFirstCharacter = widthChar;
					char folded[maxBytesCharacter * maxFoldingExpansion + 1];
					const int lenFlat = pcf->Fold(folded, sizeof(folded), bytes, widthChar);
					folded[lenFlat] = 0;
					// Does folded match the buffer
					characterMatches = 0 == memcmp(folded, &searchThing[0] + indexSearch, lenFlat);
					indexDocument += widthChar;
					indexSearch += lenFlat;
				}
				if (characterMatches && (indexSearch == static_cast<int>(lenSearch))) {
					if (MatchesWordOptions(word, wordStart, pos, indexDocument)) {
						*length = indexDocument;
						return pos;
					}
				}
				if (forward) {
					pos += widthFirstCharacter;
				} else {
					if (!NextCharacter(pos, increment))
						break;
				}
			}
		} else if (dbcsCodePage) {
			const size_t maxBytesCharacter = 2;
			const size_t maxFoldingExpansion = 4;
			std::vector<char> searchThing(lengthFind * maxBytesCharacter * maxFoldingExpansion + 1);
			const int lenSearch = pcf->Fold(&searchThing[0], searchThing.size(), search, lengthFind);
			while (forward ? (pos < endSearch) : (pos >= endSearch)) {
				int indexDocument = 0;
				int indexSearch = 0;
				bool characterMatches = true;
				while (characterMatches &&
					((pos + indexDocument) < limitPos) &&
					(indexSearch < lenSearch)) {
					char bytes[maxBytesCharacter + 1];
					bytes[0] = cb.CharAt(pos + indexDocument);
					const int widthChar = IsDBCSLeadByte(bytes[0]) ? 2 : 1;
					if (widthChar == 2)
						bytes[1] = cb.CharAt(pos + indexDocument + 1);
					char folded[maxBytesCharacter * maxFoldingExpansion + 1];
					const int lenFlat = pcf->Fold(folded, sizeof(folded), bytes, widthChar);
					folded[lenFlat] = 0;
					// Does folded match the buffer
					characterMatches = 0 == memcmp(folded, &searchThing[0] + indexSearch, lenFlat);
					indexDocument += widthChar;
					indexSearch += lenFlat;
				}
				if (characterMatches && (indexSearch == static_cast<int>(lenSearch))) {
					if (MatchesWordOptions(word, wordStart, pos, indexDocument)) {
						*length = indexDocument;
						return pos;
					}
				}
				if (!NextCharacter(pos, increment))
					break;
			}
		} else {
			CaseFolderTable caseFolder;
			std::vector<char> searchThing(lengthFind + 1);
			pcf->Fold(&searchThing[0], searchThing.size(), search, lengthFind);
			while (forward ? (pos < endSearch) : (pos >= endSearch)) {
				bool found = (pos + lengthFind) <= limitPos;
				for (int indexSearch = 0; (indexSearch < lengthFind) && found; indexSearch++) {
					char ch = CharAt(pos + indexSearch);
					char folded[2];
					pcf->Fold(folded, sizeof(folded), &ch, 1);
					found = folded[0] == searchThing[indexSearch];
				}
				if (found && MatchesWordOptions(word, wordStart, pos, lengthFind)) {
					return pos;
				}
				if (!NextCharacter(pos, increment))
					break;
			}
		}
	}
	//Platform::DebugPrintf("Not found\n");
	return -1;
}

const char *Document::SubstituteByPosition(const char *text, int *length) {
	if (regex)
		return regex->SubstituteByPosition(this, text, length);
	else
		return 0;
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

void SCI_METHOD Document::StartStyling(int position, char mask) {
	stylingMask = mask;
	endStyled = position;
}

bool SCI_METHOD Document::SetStyleFor(int length, char style) {
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

bool SCI_METHOD Document::SetStyles(int length, const char *styles) {
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
		if (pli && !pli->UseContainerLexing()) {
			int lineEndStyled = LineFromPosition(GetEndStyled());
			int endStyledTo = LineStart(lineEndStyled);
			pli->Colourise(endStyledTo, pos);
		} else {
			// Ask the watchers to style, and stop as soon as one responds.
			for (int i = 0; pos > GetEndStyled() && i < lenWatchers; i++) {
				watchers[i].watcher->NotifyStyleNeeded(this, watchers[i].userData, pos);
			}
		}
	}
}

void Document::LexerChanged() {
	// Tell the watchers the lexer has changed.
	for (int i = 0; i < lenWatchers; i++) {
		watchers[i].watcher->NotifyLexerChanged(this, watchers[i].userData);
	}
}

int SCI_METHOD Document::SetLineState(int line, int state) {
	int statePrevious = static_cast<LineState *>(perLineData[ldState])->SetLineState(line, state);
	if (state != statePrevious) {
		DocModification mh(SC_MOD_CHANGELINESTATE, LineStart(line), 0, 0, 0, line);
		NotifyModified(mh);
	}
	return statePrevious;
}

int SCI_METHOD Document::GetLineState(int line) const {
	return static_cast<LineState *>(perLineData[ldState])->GetLineState(line);
}

int Document::GetMaxLineState() {
	return static_cast<LineState *>(perLineData[ldState])->GetMaxLineState();
}

void SCI_METHOD Document::ChangeLexerState(int start, int end) {
	DocModification mh(SC_MOD_LEXERSTATE, start, end-start, 0, 0, 0);
	NotifyModified(mh);
}

StyledText Document::MarginStyledText(int line) {
	LineAnnotation *pla = static_cast<LineAnnotation *>(perLineData[ldMargin]);
	return StyledText(pla->Length(line), pla->Text(line),
		pla->MultipleStyles(line), pla->Style(line), pla->Styles(line));
}

void Document::MarginSetText(int line, const char *text) {
	static_cast<LineAnnotation *>(perLineData[ldMargin])->SetText(line, text);
	DocModification mh(SC_MOD_CHANGEMARGIN, LineStart(line), 0, 0, 0, line);
	NotifyModified(mh);
}

void Document::MarginSetStyle(int line, int style) {
	static_cast<LineAnnotation *>(perLineData[ldMargin])->SetStyle(line, style);
}

void Document::MarginSetStyles(int line, const unsigned char *styles) {
	static_cast<LineAnnotation *>(perLineData[ldMargin])->SetStyles(line, styles);
}

int Document::MarginLength(int line) const {
	return static_cast<LineAnnotation *>(perLineData[ldMargin])->Length(line);
}

void Document::MarginClearAll() {
	int maxEditorLine = LinesTotal();
	for (int l=0; l<maxEditorLine; l++)
		MarginSetText(l, 0);
	// Free remaining data
	static_cast<LineAnnotation *>(perLineData[ldMargin])->ClearAll();
}

bool Document::AnnotationAny() const {
	return static_cast<LineAnnotation *>(perLineData[ldAnnotation])->AnySet();
}

StyledText Document::AnnotationStyledText(int line) {
	LineAnnotation *pla = static_cast<LineAnnotation *>(perLineData[ldAnnotation]);
	return StyledText(pla->Length(line), pla->Text(line),
		pla->MultipleStyles(line), pla->Style(line), pla->Styles(line));
}

void Document::AnnotationSetText(int line, const char *text) {
	const int linesBefore = AnnotationLines(line);
	static_cast<LineAnnotation *>(perLineData[ldAnnotation])->SetText(line, text);
	const int linesAfter = AnnotationLines(line);
	DocModification mh(SC_MOD_CHANGEANNOTATION, LineStart(line), 0, 0, 0, line);
	mh.annotationLinesAdded = linesAfter - linesBefore;
	NotifyModified(mh);
}

void Document::AnnotationSetStyle(int line, int style) {
	static_cast<LineAnnotation *>(perLineData[ldAnnotation])->SetStyle(line, style);
	DocModification mh(SC_MOD_CHANGEANNOTATION, LineStart(line), 0, 0, 0, line);
	NotifyModified(mh);
}

void Document::AnnotationSetStyles(int line, const unsigned char *styles) {
	static_cast<LineAnnotation *>(perLineData[ldAnnotation])->SetStyles(line, styles);
}

int Document::AnnotationLength(int line) const {
	return static_cast<LineAnnotation *>(perLineData[ldAnnotation])->Length(line);
}

int Document::AnnotationLines(int line) const {
	return static_cast<LineAnnotation *>(perLineData[ldAnnotation])->Lines(line);
}

void Document::AnnotationClearAll() {
	int maxEditorLine = LinesTotal();
	for (int l=0; l<maxEditorLine; l++)
		AnnotationSetText(l, 0);
	// Free remaining data
	static_cast<LineAnnotation *>(perLineData[ldAnnotation])->ClearAll();
}

void Document::IncrementStyleClock() {
	styleClock = (styleClock + 1) % 0x100000;
}

void SCI_METHOD Document::DecorationFillRange(int position, int value, int fillLength) {
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
		while (pos > 0 && (cb.StyleAt(pos) == sStart) && (!singleLine || !IsLineEndChar(cb.CharAt(pos))))
			pos--;
		pos++;
	} else {
		while (pos < (Length()) && (cb.StyleAt(pos) == sStart) && (!singleLine || !IsLineEndChar(cb.CharAt(pos))))
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
	position = NextPosition(position, direction);
	while ((position >= 0) && (position < Length())) {
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
		int positionBeforeMove = position;
		position = NextPosition(position, direction);
		if (position == positionBeforeMove)
			break;
	}
	return - 1;
}

/**
 * Implementation of RegexSearchBase for the default built-in regular expression engine
 */
class BuiltinRegex : public RegexSearchBase {
public:
	BuiltinRegex(CharClassify *charClassTable) : search(charClassTable), substituted(NULL) {}

	virtual ~BuiltinRegex() {
		delete substituted;
	}

	virtual long FindText(Document *doc, int minPos, int maxPos, const char *s,
                        bool caseSensitive, bool word, bool wordStart, int flags,
                        int *length);

	virtual const char *SubstituteByPosition(Document *doc, const char *text, int *length);

private:
	RESearch search;
	char *substituted;
};

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

long BuiltinRegex::FindText(Document *doc, int minPos, int maxPos, const char *s,
                        bool caseSensitive, bool, bool, int flags,
                        int *length) {
	bool posix = (flags & SCFIND_POSIX) != 0;
	int increment = (minPos <= maxPos) ? 1 : -1;

	int startPos = minPos;
	int endPos = maxPos;

	// Range endpoints should not be inside DBCS characters, but just in case, move them.
	startPos = doc->MovePositionOutsideChar(startPos, 1, false);
	endPos = doc->MovePositionOutsideChar(endPos, 1, false);

	const char *errmsg = search.Compile(s, *length, caseSensitive, posix);
	if (errmsg) {
		return -1;
	}
	// Find a variable in a property file: \$(\([A-Za-z0-9_.]+\))
	// Replace first '.' with '-' in each property file variable reference:
	//     Search: \$(\([A-Za-z0-9_-]+\)\.\([A-Za-z0-9_.]+\))
	//     Replace: $(\1-\2)
	int lineRangeStart = doc->LineFromPosition(startPos);
	int lineRangeEnd = doc->LineFromPosition(endPos);
	if ((increment == 1) &&
		(startPos >= doc->LineEnd(lineRangeStart)) &&
		(lineRangeStart < lineRangeEnd)) {
		// the start position is at end of line or between line end characters.
		lineRangeStart++;
		startPos = doc->LineStart(lineRangeStart);
	}
	int pos = -1;
	int lenRet = 0;
	char searchEnd = s[*length - 1];
	int lineRangeBreak = lineRangeEnd + increment;
	for (int line = lineRangeStart; line != lineRangeBreak; line += increment) {
		int startOfLine = doc->LineStart(line);
		int endOfLine = doc->LineEnd(line);
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

		DocumentIndexer di(doc, endOfLine);
		int success = search.Execute(di, startOfLine, endOfLine);
		if (success) {
			pos = search.bopat[0];
			lenRet = search.eopat[0] - search.bopat[0];
			if (increment == -1) {
				// Check for the last match on this line.
				int repetitions = 1000;	// Break out of infinite loop
				while (success && (search.eopat[0] <= endOfLine) && (repetitions--)) {
					success = search.Execute(di, pos+1, endOfLine);
					if (success) {
						if (search.eopat[0] <= minPos) {
							pos = search.bopat[0];
							lenRet = search.eopat[0] - search.bopat[0];
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
}

const char *BuiltinRegex::SubstituteByPosition(Document *doc, const char *text, int *length) {
	delete []substituted;
	substituted = 0;
	DocumentIndexer di(doc, doc->Length());
	if (!search.GrabMatches(di))
		return 0;
	unsigned int lenResult = 0;
	for (int i = 0; i < *length; i++) {
		if (text[i] == '\\') {
			if (text[i + 1] >= '1' && text[i + 1] <= '9') {
				unsigned int patNum = text[i + 1] - '0';
				lenResult += search.eopat[patNum] - search.bopat[patNum];
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
				case '\\':
					i++;
				}
				lenResult++;
			}
		} else {
			lenResult++;
		}
	}
	substituted = new char[lenResult + 1];
	char *o = substituted;
	for (int j = 0; j < *length; j++) {
		if (text[j] == '\\') {
			if (text[j + 1] >= '1' && text[j + 1] <= '9') {
				unsigned int patNum = text[j + 1] - '0';
				unsigned int len = search.eopat[patNum] - search.bopat[patNum];
				if (search.pat[patNum])	// Will be null if try for a match that did not occur
					memcpy(o, search.pat[patNum], len);
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
				case '\\':
					*o++ = '\\';
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

#ifndef SCI_OWNREGEX

#ifdef SCI_NAMESPACE

RegexSearchBase *Scintilla::CreateRegexSearch(CharClassify *charClassTable) {
	return new BuiltinRegex(charClassTable);
}

#else

RegexSearchBase *CreateRegexSearch(CharClassify *charClassTable) {
	return new BuiltinRegex(charClassTable);
}

#endif

#endif
