// Scintilla Lexer for EDIFACT
// Written by Iain Clarke, IMCSoft & Inobiz AB.
// EDIFACT documented here: https://www.unece.org/cefact/edifact/welcome.html
// and more readably here: https://en.wikipedia.org/wiki/EDIFACT
// This code is subject to the same license terms as the rest of the scintilla project:
// The License.txt file describes the conditions under which this software may be distributed.
//

// Header order must match order in scripts/HeaderOrder.txt
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cctype>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "LexAccessor.h"
#include "LexerModule.h"
#include "DefaultLexer.h"

using namespace Scintilla;

class LexerEDIFACT : public DefaultLexer
{
public:
	LexerEDIFACT();
	virtual ~LexerEDIFACT() {} // virtual destructor, as we inherit from ILexer

	static ILexer4 *Factory() {
		return new LexerEDIFACT;
	}

	int SCI_METHOD Version() const override
	{
		return lvRelease4;
	}
	void SCI_METHOD Release() override
	{
		delete this;
	}

	const char * SCI_METHOD PropertyNames() override
	{
		return "fold\nlexer.edifact.highlight.un.all";
	}
	int SCI_METHOD PropertyType(const char *) override
	{
		return SC_TYPE_BOOLEAN; // Only one property!
	}
	const char * SCI_METHOD DescribeProperty(const char *name) override
	{
		if (!strcmp(name, "fold"))
			return "Whether to apply folding to document or not";
		if (!strcmp(name, "lexer.edifact.highlight.un.all"))
			return "Whether to apply UN* highlighting to all UN segments, or just to UNH";
		return NULL;
	}

	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override
	{
		if (!strcmp(key, "fold"))
		{
			m_bFold = strcmp(val, "0") ? true : false;
			return 0;
		}
		if (!strcmp(key, "lexer.edifact.highlight.un.all"))	// GetProperty
		{
			m_bHighlightAllUN = strcmp(val, "0") ? true : false;
			return 0;
		}
		return -1;
	}
	const char * SCI_METHOD DescribeWordListSets() override
	{
		return NULL;
	}
	Sci_Position SCI_METHOD WordListSet(int, const char *) override
	{
		return -1;
	}
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void * SCI_METHOD PrivateCall(int, void *) override
	{
		return NULL;
	}

protected:
	Sci_Position InitialiseFromUNA(IDocument *pAccess, Sci_PositionU MaxLength);
	Sci_Position FindPreviousEnd(IDocument *pAccess, Sci_Position startPos) const;
	Sci_Position ForwardPastWhitespace(IDocument *pAccess, Sci_Position startPos, Sci_Position MaxLength) const;
	int DetectSegmentHeader(char SegmentHeader[3]) const;

	bool m_bFold;

	// property lexer.edifact.highlight.un.all
	//	Set to 0 to highlight only UNA segments, or 1 to highlight all UNx segments.
	bool m_bHighlightAllUN;

	char m_chComponent;
	char m_chData;
	char m_chDecimal;
	char m_chRelease;
	char m_chSegment;
};

LexerModule lmEDIFACT(SCLEX_EDIFACT, LexerEDIFACT::Factory, "edifact");

///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////

LexerEDIFACT::LexerEDIFACT()
{
	m_bFold = false;
	m_bHighlightAllUN = false;
	m_chComponent = ':';
	m_chData = '+';
	m_chDecimal = '.';
	m_chRelease = '?';
	m_chSegment = '\'';
}

void LexerEDIFACT::Lex(Sci_PositionU startPos, Sci_Position length, int, IDocument *pAccess)
{
	Sci_PositionU posFinish = startPos + length;
	InitialiseFromUNA(pAccess, posFinish);

	// Look backwards for a ' or a document beginning
	Sci_PositionU posCurrent = FindPreviousEnd(pAccess, startPos);
	// And jump past the ' if this was not the beginning of the document
	if (posCurrent != 0)
		posCurrent++;

	// Style buffer, so we're not issuing loads of notifications
	LexAccessor styler (pAccess);
	pAccess->StartStyling(posCurrent);
	styler.StartSegment(posCurrent);
	Sci_Position posSegmentStart = -1;

	while ((posCurrent < posFinish) && (posSegmentStart == -1))
	{
		posCurrent = ForwardPastWhitespace(pAccess, posCurrent, posFinish);
		// Mark whitespace as default
		styler.ColourTo(posCurrent - 1, SCE_EDI_DEFAULT);
		if (posCurrent >= posFinish)
			break;

		// Does is start with 3 charaters? ie, UNH
		char SegmentHeader[4] = { 0 };
		pAccess->GetCharRange(SegmentHeader, posCurrent, 3);

		int SegmentStyle = DetectSegmentHeader(SegmentHeader);
		if (SegmentStyle == SCE_EDI_BADSEGMENT)
			break;
		if (SegmentStyle == SCE_EDI_UNA)
		{
			posCurrent += 9;
			styler.ColourTo(posCurrent - 1, SCE_EDI_UNA); // UNA
			continue;
		}
		posSegmentStart = posCurrent;
		posCurrent += 3;

		styler.ColourTo(posCurrent - 1, SegmentStyle); // UNH etc

		// Colour in the rest of the segment
		for (char c; posCurrent < posFinish; posCurrent++)
		{
			pAccess->GetCharRange(&c, posCurrent, 1);

			if (c == m_chRelease) // ? escape character, check first, in case of ?'
				posCurrent++;
			else if (c == m_chSegment) // '
			{
				// Make sure the whole segment is on one line. styler won't let us go back in time, so we'll settle for marking the ' as bad.
				Sci_Position lineSegmentStart = pAccess->LineFromPosition(posSegmentStart);
				Sci_Position lineSegmentEnd = pAccess->LineFromPosition(posCurrent);
				if (lineSegmentStart == lineSegmentEnd)
					styler.ColourTo(posCurrent, SCE_EDI_SEGMENTEND);
				else
					styler.ColourTo(posCurrent, SCE_EDI_BADSEGMENT);
				posSegmentStart = -1;
				posCurrent++;
				break;
			}
			else if (c == m_chComponent) // :
				styler.ColourTo(posCurrent, SCE_EDI_SEP_COMPOSITE);
			else if (c == m_chData) // +
				styler.ColourTo(posCurrent, SCE_EDI_SEP_ELEMENT);
			else
				styler.ColourTo(posCurrent, SCE_EDI_DEFAULT);
		}
	}
	styler.Flush();

	if (posSegmentStart == -1)
		return;

	pAccess->StartStyling(posSegmentStart);
	pAccess->SetStyleFor(posFinish - posSegmentStart, SCE_EDI_BADSEGMENT);
}

void LexerEDIFACT::Fold(Sci_PositionU startPos, Sci_Position length, int, IDocument *pAccess)
{
	if (!m_bFold)
		return;

	Sci_PositionU endPos = startPos + length;
	startPos = FindPreviousEnd(pAccess, startPos);
	char c;
	char SegmentHeader[4] = { 0 };

	bool AwaitingSegment = true;
	Sci_PositionU currLine = pAccess->LineFromPosition(startPos);
	int levelCurrentStyle = SC_FOLDLEVELBASE;
	if (currLine > 0)
		levelCurrentStyle = pAccess->GetLevel(currLine - 1); // bottom 12 bits are level
	int indentCurrent = levelCurrentStyle & SC_FOLDLEVELNUMBERMASK;
	int indentNext = indentCurrent;

	while (startPos < endPos)
	{
		pAccess->GetCharRange(&c, startPos, 1);
		switch (c)
		{
		case '\t':
		case '\r':
		case ' ':
			startPos++;
			continue;
		case '\n':
			currLine = pAccess->LineFromPosition(startPos);
			pAccess->SetLevel(currLine, levelCurrentStyle | indentCurrent);
			startPos++;
			levelCurrentStyle = SC_FOLDLEVELBASE;
			indentCurrent = indentNext;
			continue;
		}
		if (c == m_chRelease)
		{
			startPos += 2;
			continue;
		}
		if (c == m_chSegment)
		{
			AwaitingSegment = true;
			startPos++;
			continue;
		}

		if (!AwaitingSegment)
		{
			startPos++;
			continue;
		}
		
		// Segment!
		pAccess->GetCharRange(SegmentHeader, startPos, 3);
		if (SegmentHeader[0] != 'U' || SegmentHeader[1] != 'N')
		{
			startPos++;
			continue;
		}

		AwaitingSegment = false;
		switch (SegmentHeader[2])
		{
		case 'H':
		case 'G':
			indentNext++;
			levelCurrentStyle = SC_FOLDLEVELBASE | SC_FOLDLEVELHEADERFLAG;
			break;

		case 'T':
		case 'E':
			if (indentNext > 0)
				indentNext--;
			break;
		}

		startPos += 3;
	}
}

Sci_Position LexerEDIFACT::InitialiseFromUNA(IDocument *pAccess, Sci_PositionU MaxLength)
{
	MaxLength -= 9; // drop 9 chars, to give us room for UNA:+.? '

	Sci_PositionU startPos = 0;
	startPos += ForwardPastWhitespace(pAccess, 0, MaxLength);
	if (startPos < MaxLength)
	{
		char bufUNA[9];
		pAccess->GetCharRange(bufUNA, startPos, 9);

		// Check it's UNA segment
		if (!memcmp(bufUNA, "UNA", 3))
		{
			m_chComponent = bufUNA[3];
			m_chData = bufUNA[4];
			m_chDecimal = bufUNA[5];
			m_chRelease = bufUNA[6];
			// bufUNA [7] should be space - reserved.
			m_chSegment = bufUNA[8];

			return 0; // success!
		}
	}

	// We failed to find a UNA, so drop to defaults
	m_chComponent = ':';
	m_chData = '+';
	m_chDecimal = '.';
	m_chRelease = '?';
	m_chSegment = '\'';

	return -1;
}

Sci_Position LexerEDIFACT::ForwardPastWhitespace(IDocument *pAccess, Sci_Position startPos, Sci_Position MaxLength) const
{
	char c;

	while (startPos < MaxLength)
	{
		pAccess->GetCharRange(&c, startPos, 1);
		switch (c)
		{
		case '\t':
		case '\r':
		case '\n':
		case ' ':
			break;
		default:
			return startPos;
		}

		startPos++;
	}

	return MaxLength;
}

int LexerEDIFACT::DetectSegmentHeader(char SegmentHeader[3]) const
{
	if (
		SegmentHeader[0] < 'A' || SegmentHeader[0] > 'Z' ||
		SegmentHeader[1] < 'A' || SegmentHeader[1] > 'Z' ||
		SegmentHeader[2] < 'A' || SegmentHeader[2] > 'Z')
		return SCE_EDI_BADSEGMENT;

	if (!memcmp(SegmentHeader, "UNA", 3))
		return SCE_EDI_UNA;

	if (m_bHighlightAllUN && !memcmp(SegmentHeader, "UN", 2))
		return SCE_EDI_UNH;
	else if (!memcmp(SegmentHeader, "UNH", 3))
		return SCE_EDI_UNH;
	else if (!memcmp(SegmentHeader, "UNG", 3))
		return SCE_EDI_UNH;

	return SCE_EDI_SEGMENTSTART;
}

// Look backwards for a ' or a document beginning
Sci_Position LexerEDIFACT::FindPreviousEnd(IDocument *pAccess, Sci_Position startPos) const
{
	for (char c; startPos > 0; startPos--)
	{
		pAccess->GetCharRange(&c, startPos, 1);
		if (c == m_chSegment)
			return startPos;
	}
	// We didn't find a ', so just go with the beginning
	return 0;
}


