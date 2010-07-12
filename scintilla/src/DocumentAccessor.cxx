// Scintilla source code edit control
/** @file DocumentAccessor.cxx
 ** Rapid easy access to contents of a Scintilla.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "Platform.h"

#include "PropSet.h"
#include "Accessor.h"
#include "DocumentAccessor.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "CellBuffer.h"
#include "Scintilla.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "Document.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

DocumentAccessor::~DocumentAccessor() {
}

bool DocumentAccessor::InternalIsLeadByte(char ch) {
	if (SC_CP_UTF8 == codePage)
		// For lexing, all characters >= 0x80 are treated the
		// same so none is considered a lead byte.
		return false;
	else
		return Platform::IsDBCSLeadByte(codePage, ch);
}

void DocumentAccessor::Fill(int position) {
	if (lenDoc == -1)
		lenDoc = pdoc->Length();
	startPos = position - slopSize;
	if (startPos + bufferSize > lenDoc)
		startPos = lenDoc - bufferSize;
	if (startPos < 0)
		startPos = 0;
	endPos = startPos + bufferSize;
	if (endPos > lenDoc)
		endPos = lenDoc;

	pdoc->GetCharRange(buf, startPos, endPos-startPos);
	buf[endPos-startPos] = '\0';
}

bool DocumentAccessor::Match(int pos, const char *s) {
	for (int i=0; *s; i++) {
		if (*s != SafeGetCharAt(pos+i))
			return false;
		s++;
	}
	return true;
}

char DocumentAccessor::StyleAt(int position) {
	// Mask off all bits which aren't in the 'mask'.
	return static_cast<char>(pdoc->StyleAt(position) & mask);
}

int DocumentAccessor::GetLine(int position) {
	return pdoc->LineFromPosition(position);
}

int DocumentAccessor::LineStart(int line) {
	return pdoc->LineStart(line);
}

int DocumentAccessor::LevelAt(int line) {
	return pdoc->GetLevel(line);
}

int DocumentAccessor::Length() {
	if (lenDoc == -1)
		lenDoc = pdoc->Length();
	return lenDoc;
}

int DocumentAccessor::GetLineState(int line) {
	return pdoc->GetLineState(line);
}

int DocumentAccessor::SetLineState(int line, int state) {
	return pdoc->SetLineState(line, state);
}

void DocumentAccessor::StartAt(unsigned int start, char chMask) {
	// Store the mask specified for use with StyleAt.
	mask = chMask;
	pdoc->StartStyling(start, chMask);
	startPosStyling = start;
}

void DocumentAccessor::StartSegment(unsigned int pos) {
	startSeg = pos;
}

void DocumentAccessor::ColourTo(unsigned int pos, int chAttr) {
	// Only perform styling if non empty range
	if (pos != startSeg - 1) {
		PLATFORM_ASSERT(pos >= startSeg);
		if (pos < startSeg) {
			return;
		}

		if (validLen + (pos - startSeg + 1) >= bufferSize)
			Flush();
		if (validLen + (pos - startSeg + 1) >= bufferSize) {
			// Too big for buffer so send directly
			pdoc->SetStyleFor(pos - startSeg + 1, static_cast<char>(chAttr));
		} else {
			if (chAttr != chWhile)
				chFlags = 0;
			chAttr |= chFlags;
			for (unsigned int i = startSeg; i <= pos; i++) {
				PLATFORM_ASSERT((startPosStyling + validLen) < Length());
				styleBuf[validLen++] = static_cast<char>(chAttr);
			}
		}
	}
	startSeg = pos+1;
}

void DocumentAccessor::SetLevel(int line, int level) {
	pdoc->SetLevel(line, level);
}

void DocumentAccessor::Flush() {
	startPos = extremePosition;
	lenDoc = -1;
	if (validLen > 0) {
		pdoc->SetStyles(validLen, styleBuf);
		startPosStyling += validLen;
		validLen = 0;
	}
}

int DocumentAccessor::IndentAmount(int line, int *flags, PFNIsCommentLeader pfnIsCommentLeader) {
	int end = Length();
	int spaceFlags = 0;

	// Determines the indentation level of the current line and also checks for consistent
	// indentation compared to the previous line.
	// Indentation is judged consistent when the indentation whitespace of each line lines
	// the same or the indentation of one line is a prefix of the other.

	int pos = LineStart(line);
	char ch = (*this)[pos];
	int indent = 0;
	bool inPrevPrefix = line > 0;
	int posPrev = inPrevPrefix ? LineStart(line-1) : 0;
	while ((ch == ' ' || ch == '\t') && (pos < end)) {
		if (inPrevPrefix) {
			char chPrev = (*this)[posPrev++];
			if (chPrev == ' ' || chPrev == '\t') {
				if (chPrev != ch)
					spaceFlags |= wsInconsistent;
			} else {
				inPrevPrefix = false;
			}
		}
		if (ch == ' ') {
			spaceFlags |= wsSpace;
			indent++;
		} else {	// Tab
			spaceFlags |= wsTab;
			if (spaceFlags & wsSpace)
				spaceFlags |= wsSpaceTab;
			indent = (indent / 8 + 1) * 8;
		}
		ch = (*this)[++pos];
	}

	*flags = spaceFlags;
	indent += SC_FOLDLEVELBASE;
	// if completely empty line or the start of a comment...
	if ((ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') ||
	        (pfnIsCommentLeader && (*pfnIsCommentLeader)(*this, pos, end-pos)))
		return indent | SC_FOLDLEVELWHITEFLAG;
	else
		return indent;
}

void DocumentAccessor::IndicatorFill(int start, int end, int indicator, int value) {
	pdoc->decorations.SetCurrentIndicator(indicator);
	pdoc->DecorationFillRange(start, value, end - start);
}
