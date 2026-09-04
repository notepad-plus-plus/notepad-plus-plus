// Scintilla source code edit control
/** @file StyleContext.h
 ** Lexer infrastructure.
 **/
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// This file is in the public domain.

#ifndef STYLECONTEXT_H
#define STYLECONTEXT_H

namespace Lexilla {

// All languages handled so far can treat all characters >= 0x80 as one class
// which just continues the current token or starts an identifier if in default.
// DBCS treated specially as the second character can be < 0x80 and hence
// syntactically significant. UTF-8 avoids this as all trail bytes are >= 0x80
class StyleContext {
	LexAccessor &styler;
	Scintilla::IDocument * const multiByteAccess;
	const Sci_PositionU lengthDocument;
	const Sci_PositionU endPos;
	const Sci_Position lineDocEnd;

	// Used for optimizing GetRelativeCharacter
	Sci_PositionU posRelative = 0;
	Sci_PositionU currentPosLastRelative;
	Sci_Position offsetRelative = 0;

	void GetNextChar() {
		if (multiByteAccess) {
			chNext = multiByteAccess->GetCharacterAndWidth(currentPos+width, &widthNext);
		} else {
			const unsigned char charNext = styler.SafeGetCharAt(currentPos + width, 0);
			chNext = charNext;
		}
		// End of line determined from line end position, allowing CR, LF,
		// CRLF and Unicode line ends as set by document.
		const Sci_Position currentPosSigned = currentPos;
		if (currentLine < lineDocEnd)
			atLineEnd = currentPosSigned >= (lineStartNext-1);
		else // Last line
			atLineEnd = currentPosSigned >= lineStartNext;
	}

public:
	Sci_PositionU currentPos;
	Sci_Position currentLine;
	Sci_Position lineEnd;
	Sci_Position lineStartNext;
	bool atLineStart;
	bool atLineEnd = false;
	int state;
	int chPrev = 0;
	int ch = 0;
	Sci_Position width = 0;
	int chNext = 0;
	Sci_Position widthNext = 1;

	StyleContext(Sci_PositionU startPos, Sci_PositionU length,
                        int initStyle, LexAccessor &styler_, char chMask = '\377');
	// Deleted so StyleContext objects can not be copied.
	StyleContext(const StyleContext &) = delete;
	StyleContext &operator=(const StyleContext &) = delete;
	void Complete() {
		styler.ColourTo(currentPos - ((currentPos > lengthDocument) ? 2 : 1), state);
		styler.Flush();
	}
	bool More() const noexcept {
		return currentPos < endPos;
	}
	void Forward() {
		if (currentPos < endPos) {
			atLineStart = atLineEnd;
			if (atLineStart) {
				currentLine++;
				lineEnd = styler.LineEnd(currentLine);
				lineStartNext = styler.LineStart(currentLine+1);
			}
			chPrev = ch;
			currentPos += width;
			ch = chNext;
			width = widthNext;
			GetNextChar();
		} else {
			atLineStart = false;
			chPrev = ' ';
			ch = ' ';
			chNext = ' ';
			atLineEnd = true;
		}
	}
	void Forward(Sci_Position nb) {
		for (Sci_Position i = 0; i < nb; i++) {
			Forward();
		}
	}
	void ForwardBytes(Sci_Position nb) {
		const Sci_PositionU forwardPos = currentPos + nb;
		while (forwardPos > currentPos) {
			const Sci_PositionU currentPosStart = currentPos;
			Forward();
			if (currentPos == currentPosStart) {
				// Reached end
				return;
			}
		}
	}
	void ChangeState(int state_) noexcept {
		state = state_;
	}
	void SetState(int state_) {
		styler.ColourTo(currentPos - ((currentPos > lengthDocument) ? 2 : 1), state);
		state = state_;
	}
	void ForwardSetState(int state_) {
		Forward();
		styler.ColourTo(currentPos - ((currentPos > lengthDocument) ? 2 : 1), state);
		state = state_;
	}
	Sci_Position LengthCurrent() const noexcept {
		return currentPos - styler.GetStartSegment();
	}
	char GetRelativeChar(Sci_Position n, char chDefault='\0') {
		return styler.SafeGetCharAt(currentPos + n, chDefault);
	}
	int GetRelative(Sci_Position n, char chDefault='\0') {
		const unsigned char chRelative = styler.SafeGetCharAt(currentPos + n, chDefault);
		return chRelative;
	}
	int GetRelativeCharacter(Sci_Position n) {
		if (n == 0)
			return ch;
		if (multiByteAccess) {
			if ((currentPosLastRelative != currentPos) ||
				((n > 0) && ((offsetRelative < 0) || (n < offsetRelative))) ||
				((n < 0) && ((offsetRelative > 0) || (n > offsetRelative)))) {
				posRelative = currentPos;
				offsetRelative = 0;
			}
			const Sci_Position diffRelative = n - offsetRelative;
			const Sci_Position posNew = multiByteAccess->GetRelativePosition(posRelative, diffRelative);
			const int chReturn = multiByteAccess->GetCharacterAndWidth(posNew, nullptr);
			posRelative = posNew;
			currentPosLastRelative = currentPos;
			offsetRelative = n;
			return chReturn;
		} else {
			// fast version for single byte encodings
			const unsigned char chRelative = styler.SafeGetCharAt(currentPos + n, 0);
			return chRelative;
		}
	}
	bool MatchLineEnd() const noexcept {
		const Sci_Position currentPosSigned = currentPos;
		return currentPosSigned == lineEnd;
	}
	bool Match(char ch0) const noexcept {
		const unsigned char uch0 = ch0;
		return ch == uch0;
	}
	bool Match(char ch0, char ch1) const noexcept {
		const unsigned char uch0 = ch0;
		const unsigned char uch1 = ch1;
		return (ch == uch0) && (chNext == uch1);
	}
	bool Match(const char *s) {
		const unsigned char su = *s;
		if (ch != su)
			return false;
		s++;
		if (!*s)
			return true;
		const unsigned char sNext = *s;
		if (chNext != sNext)
			return false;
		s++;
		for (int n=2; *s; n++) {
			if (*s != styler.SafeGetCharAt(currentPos+n, 0))
				return false;
			s++;
		}
		return true;
	}
	// Non-inline
	bool MatchIgnoreCase(const char *s);
	bool MatchIgnoreCase2(const char *s);
	void GetCurrent(char *s, Sci_PositionU len) const;
	void GetCurrentLowered(char *s, Sci_PositionU len) const;
	enum class Transform { none, lower };
	void GetCurrentString(std::string &string, Transform transform) const;
};

}

#endif
