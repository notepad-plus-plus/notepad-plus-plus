// Scintilla source code edit control
/** @file LexAccessor.h
 ** Interfaces between Scintilla and lexers.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LEXACCESSOR_H
#define LEXACCESSOR_H

namespace Scintilla {

enum EncodingType { enc8bit, encUnicode, encDBCS };

class LexAccessor {
private:
	IDocument *pAccess;
	enum {extremePosition=0x7FFFFFFF};
	/** @a bufferSize is a trade off between time taken to copy the characters
	 * and retrieval overhead.
	 * @a slopSize positions the buffer before the desired position
	 * in case there is some backtracking. */
	enum {bufferSize=4000, slopSize=bufferSize/8};
	char buf[bufferSize+1];
	Sci_Position startPos;
	Sci_Position endPos;
	int codePage;
	enum EncodingType encodingType;
	Sci_Position lenDoc;
	char styleBuf[bufferSize];
	Sci_Position validLen;
	Sci_PositionU startSeg;
	Sci_Position startPosStyling;
	int documentVersion;

	void Fill(Sci_Position position) {
		startPos = position - slopSize;
		if (startPos + bufferSize > lenDoc)
			startPos = lenDoc - bufferSize;
		if (startPos < 0)
			startPos = 0;
		endPos = startPos + bufferSize;
		if (endPos > lenDoc)
			endPos = lenDoc;

		pAccess->GetCharRange(buf, startPos, endPos-startPos);
		buf[endPos-startPos] = '\0';
	}

public:
	explicit LexAccessor(IDocument *pAccess_) :
		pAccess(pAccess_), startPos(extremePosition), endPos(0),
		codePage(pAccess->CodePage()),
		encodingType(enc8bit),
		lenDoc(pAccess->Length()),
		validLen(0),
		startSeg(0), startPosStyling(0),
		documentVersion(pAccess->Version()) {
		// Prevent warnings by static analyzers about uninitialized buf and styleBuf.
		buf[0] = 0;
		styleBuf[0] = 0;
		switch (codePage) {
		case 65001:
			encodingType = encUnicode;
			break;
		case 932:
		case 936:
		case 949:
		case 950:
		case 1361:
			encodingType = encDBCS;
		}
	}
	char operator[](Sci_Position position) {
		if (position < startPos || position >= endPos) {
			Fill(position);
		}
		return buf[position - startPos];
	}
	IDocument *MultiByteAccess() const {
		return pAccess;
	}
	/** Safe version of operator[], returning a defined value for invalid position. */
	char SafeGetCharAt(Sci_Position position, char chDefault=' ') {
		if (position < startPos || position >= endPos) {
			Fill(position);
			if (position < startPos || position >= endPos) {
				// Position is outside range of document
				return chDefault;
			}
		}
		return buf[position - startPos];
	}
	bool IsLeadByte(char ch) const {
		return pAccess->IsDBCSLeadByte(ch);
	}
	EncodingType Encoding() const {
		return encodingType;
	}
	bool Match(Sci_Position pos, const char *s) {
		for (int i=0; *s; i++) {
			if (*s != SafeGetCharAt(pos+i))
				return false;
			s++;
		}
		return true;
	}
	char StyleAt(Sci_Position position) const {
		return pAccess->StyleAt(position);
	}
	Sci_Position GetLine(Sci_Position position) const {
		return pAccess->LineFromPosition(position);
	}
	Sci_Position LineStart(Sci_Position line) const {
		return pAccess->LineStart(line);
	}
	Sci_Position LineEnd(Sci_Position line) const {
		return pAccess->LineEnd(line);
	}
	int LevelAt(Sci_Position line) const {
		return pAccess->GetLevel(line);
	}
	Sci_Position Length() const {
		return lenDoc;
	}
	void Flush() {
		if (validLen > 0) {
			pAccess->SetStyles(validLen, styleBuf);
			startPosStyling += validLen;
			validLen = 0;
		}
	}
	int GetLineState(Sci_Position line) const {
		return pAccess->GetLineState(line);
	}
	int SetLineState(Sci_Position line, int state) {
		return pAccess->SetLineState(line, state);
	}
	// Style setting
	void StartAt(Sci_PositionU start) {
		pAccess->StartStyling(start);
		startPosStyling = start;
	}
	Sci_PositionU GetStartSegment() const {
		return startSeg;
	}
	void StartSegment(Sci_PositionU pos) {
		startSeg = pos;
	}
	void ColourTo(Sci_PositionU pos, int chAttr) {
		// Only perform styling if non empty range
		if (pos != startSeg - 1) {
			assert(pos >= startSeg);
			if (pos < startSeg) {
				return;
			}

			if (validLen + (pos - startSeg + 1) >= bufferSize)
				Flush();
			const char attr = static_cast<char>(chAttr);
			if (validLen + (pos - startSeg + 1) >= bufferSize) {
				// Too big for buffer so send directly
				pAccess->SetStyleFor(pos - startSeg + 1, attr);
			} else {
				for (Sci_PositionU i = startSeg; i <= pos; i++) {
					assert((startPosStyling + validLen) < Length());
					styleBuf[validLen++] = attr;
				}
			}
		}
		startSeg = pos+1;
	}
	void SetLevel(Sci_Position line, int level) {
		pAccess->SetLevel(line, level);
	}
	void IndicatorFill(Sci_Position start, Sci_Position end, int indicator, int value) {
		pAccess->DecorationSetCurrentIndicator(indicator);
		pAccess->DecorationFillRange(start, value, end - start);
	}

	void ChangeLexerState(Sci_Position start, Sci_Position end) {
		pAccess->ChangeLexerState(start, end);
	}
};

struct LexicalClass {
	int value;
	const char *name;
	const char *tags;
	const char *description;
};

}

#endif
