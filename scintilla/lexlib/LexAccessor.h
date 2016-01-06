// Scintilla source code edit control
/** @file LexAccessor.h
 ** Interfaces between Scintilla and lexers.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LEXACCESSOR_H
#define LEXACCESSOR_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

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
	int startPos;
	int endPos;
	int codePage;
	enum EncodingType encodingType;
	int lenDoc;
	char styleBuf[bufferSize];
	int validLen;
	unsigned int startSeg;
	int startPosStyling;
	int documentVersion;

	void Fill(int position) {
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
	char operator[](int position) {
		if (position < startPos || position >= endPos) {
			Fill(position);
		}
		return buf[position - startPos];
	}
	IDocumentWithLineEnd *MultiByteAccess() const {
		if (documentVersion >= dvLineEnd) {
			return static_cast<IDocumentWithLineEnd *>(pAccess);
		}
		return 0;
	}
	/** Safe version of operator[], returning a defined value for invalid position. */
	char SafeGetCharAt(int position, char chDefault=' ') {
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
	bool Match(int pos, const char *s) {
		for (int i=0; *s; i++) {
			if (*s != SafeGetCharAt(pos+i))
				return false;
			s++;
		}
		return true;
	}
	char StyleAt(int position) const {
		return static_cast<char>(pAccess->StyleAt(position));
	}
	int GetLine(int position) const {
		return pAccess->LineFromPosition(position);
	}
	int LineStart(int line) const {
		return pAccess->LineStart(line);
	}
	int LineEnd(int line) {
		if (documentVersion >= dvLineEnd) {
			return (static_cast<IDocumentWithLineEnd *>(pAccess))->LineEnd(line);
		} else {
			// Old interface means only '\r', '\n' and '\r\n' line ends.
			int startNext = pAccess->LineStart(line+1);
			char chLineEnd = SafeGetCharAt(startNext-1);
			if (chLineEnd == '\n' && (SafeGetCharAt(startNext-2)  == '\r'))
				return startNext - 2;
			else
				return startNext - 1;
		}
	}
	int LevelAt(int line) const {
		return pAccess->GetLevel(line);
	}
	int Length() const {
		return lenDoc;
	}
	void Flush() {
		if (validLen > 0) {
			pAccess->SetStyles(validLen, styleBuf);
			startPosStyling += validLen;
			validLen = 0;
		}
	}
	int GetLineState(int line) const {
		return pAccess->GetLineState(line);
	}
	int SetLineState(int line, int state) {
		return pAccess->SetLineState(line, state);
	}
	// Style setting
	void StartAt(unsigned int start) {
		pAccess->StartStyling(start, '\377');
		startPosStyling = start;
	}
	unsigned int GetStartSegment() const {
		return startSeg;
	}
	void StartSegment(unsigned int pos) {
		startSeg = pos;
	}
	void ColourTo(unsigned int pos, int chAttr) {
		// Only perform styling if non empty range
		if (pos != startSeg - 1) {
			assert(pos >= startSeg);
			if (pos < startSeg) {
				return;
			}

			if (validLen + (pos - startSeg + 1) >= bufferSize)
				Flush();
			if (validLen + (pos - startSeg + 1) >= bufferSize) {
				// Too big for buffer so send directly
				pAccess->SetStyleFor(pos - startSeg + 1, static_cast<char>(chAttr));
			} else {
				for (unsigned int i = startSeg; i <= pos; i++) {
					assert((startPosStyling + validLen) < Length());
					styleBuf[validLen++] = static_cast<char>(chAttr);
				}
			}
		}
		startSeg = pos+1;
	}
	void SetLevel(int line, int level) {
		pAccess->SetLevel(line, level);
	}
	void IndicatorFill(int start, int end, int indicator, int value) {
		pAccess->DecorationSetCurrentIndicator(indicator);
		pAccess->DecorationFillRange(start, value, end - start);
	}

	void ChangeLexerState(int start, int end) {
		pAccess->ChangeLexerState(start, end);
	}
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
