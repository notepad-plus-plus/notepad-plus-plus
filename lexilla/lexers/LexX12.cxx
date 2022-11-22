// Scintilla Lexer for X12
// @file LexX12.cxx
// Written by Iain Clarke, IMCSoft & Inobiz AB.
// X12 official documentation is behind a paywall, but there's a description of the syntax here:
// http://www.rawlinsecconsulting.com/x12tutorial/x12syn.html
// This code is subject to the same license terms as the rest of the scintilla project:
// The License.txt file describes the conditions under which this software may be distributed.
//

// Header order must match order in scripts/HeaderOrder.txt
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cctype>

#include <string>
#include <string_view>

#include <vector>
#include <algorithm>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"
#include "LexerModule.h"
#include "DefaultLexer.h"

using namespace Scintilla;
using namespace Lexilla;

class LexerX12 : public DefaultLexer
{
public:
	LexerX12();
	virtual ~LexerX12() {} // virtual destructor, as we inherit from ILexer

	static ILexer5 *Factory() {
		return new LexerX12;
	}

	int SCI_METHOD Version() const override
	{
		return lvRelease5;
	}
	void SCI_METHOD Release() override
	{
		delete this;
	}

	const char * SCI_METHOD PropertyNames() override
	{
		return "fold";
	}
	int SCI_METHOD PropertyType(const char *) override
	{
		return SC_TYPE_BOOLEAN; // Only one property!
	}
	const char * SCI_METHOD DescribeProperty(const char *name) override
	{
		if (!strcmp(name, "fold"))
			return "Whether to apply folding to document or not";
		return "";
	}

	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override
	{
		if (!strcmp(key, "fold"))
		{
			m_bFold = strcmp(val, "0") ? true : false;
			return 0;
		}
		return -1;
	}
	const char * SCI_METHOD PropertyGet(const char *) override {
		return "";
	}
	const char * SCI_METHOD DescribeWordListSets() override
	{
		return "";
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
	struct Terminator
	{
		int Style = SCE_X12_BAD;
		Sci_PositionU pos = 0;
		Sci_PositionU length = 0;
		int FoldChange = 0;
	};
	Terminator InitialiseFromISA(IDocument *pAccess);
	Sci_PositionU FindPreviousSegmentStart(IDocument *pAccess, Sci_Position startPos) const;
	Terminator DetectSegmentHeader(IDocument *pAccess, Sci_PositionU pos) const;
	Terminator FindNextTerminator(IDocument *pAccess, Sci_PositionU pos, bool bJustSegmentTerminator = false) const;

	bool m_bFold = false;
	char m_SeparatorSubElement = 0;
	char m_SeparatorElement = 0;
	std::string m_SeparatorSegment; // might be multiple characters
	std::string m_LineFeed;
};

LexerModule lmX12(SCLEX_X12, LexerX12::Factory, "x12");

///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////

LexerX12::LexerX12() : DefaultLexer("x12", SCLEX_X12)
{
}

void LexerX12::Lex(Sci_PositionU startPos, Sci_Position length, int, IDocument *pAccess)
{
	Sci_PositionU posFinish = startPos + length;

	Terminator T = InitialiseFromISA(pAccess);

	if (T.Style == SCE_X12_BAD)
	{
		if (T.pos < startPos)
			T.pos = startPos; // we may be colouring in batches.
		pAccess->StartStyling(startPos);
		pAccess->SetStyleFor(T.pos - startPos, SCE_X12_ENVELOPE);
		pAccess->SetStyleFor(posFinish - T.pos, SCE_X12_BAD);
		return;
	}

	// Look backwards for a segment start or a document beginning
	Sci_PositionU posCurrent = FindPreviousSegmentStart (pAccess, startPos);

	// Style buffer, so we're not issuing loads of notifications
	pAccess->StartStyling(posCurrent);

	while (posCurrent < posFinish)
	{
		// Look for first element marker, so we can denote segment
		T = DetectSegmentHeader(pAccess, posCurrent);
		if (T.Style == SCE_X12_BAD)
			break;

		pAccess->SetStyleFor(T.pos - posCurrent, T.Style);
		pAccess->SetStyleFor(T.length, SCE_X12_SEP_ELEMENT);
		posCurrent = T.pos + T.length;

		while (T.Style != SCE_X12_BAD && T.Style != SCE_X12_SEGMENTEND) // Break on bad or segment ending
		{
			T = FindNextTerminator(pAccess, posCurrent, false);
			if (T.Style == SCE_X12_BAD)
				break;

			int Style = T.Style;

			pAccess->SetStyleFor(T.pos - posCurrent, SCE_X12_DEFAULT);
			pAccess->SetStyleFor(T.length, Style);
			posCurrent = T.pos + T.length;
		}
		if (T.Style == SCE_X12_BAD)
			break;
	}

	pAccess->SetStyleFor(posFinish - posCurrent, SCE_X12_BAD);
}

void LexerX12::Fold(Sci_PositionU startPos, Sci_Position length, int, IDocument *pAccess)
{
	if (!m_bFold)
		return;

	// Are we even foldable?
	// check for cr,lf,cr+lf.
	if (m_LineFeed.empty())
		return;

	Sci_PositionU posFinish = startPos + length;

	// Look backwards for a segment start or a document beginning
	startPos = FindPreviousSegmentStart(pAccess, startPos);
	Terminator T;

	Sci_PositionU currLine = pAccess->LineFromPosition(startPos);
	int levelCurrentStyle = SC_FOLDLEVELBASE;
	int indentCurrent = 0;
	if (currLine > 0)
	{
		levelCurrentStyle = pAccess->GetLevel(currLine - 1); // bottom 12 bits are level
		indentCurrent = levelCurrentStyle & (SC_FOLDLEVELBASE - 1); // indent from previous line
		Sci_PositionU posLine = pAccess->LineStart(currLine - 1);
		T = DetectSegmentHeader(pAccess, posLine);
		indentCurrent += T.FoldChange;
	}

	while (startPos < posFinish)
	{
		T = DetectSegmentHeader(pAccess, startPos);
		int indentNext = indentCurrent + T.FoldChange;
		if (indentNext < 0)
			indentNext = 0;

		levelCurrentStyle = (T.FoldChange > 0) ? (SC_FOLDLEVELBASE | SC_FOLDLEVELHEADERFLAG) : SC_FOLDLEVELBASE;

		currLine = pAccess->LineFromPosition(startPos);
		pAccess->SetLevel(currLine, levelCurrentStyle | indentCurrent);

		T = FindNextTerminator(pAccess, startPos, true);

		if (T.Style == SCE_X12_BAD)
			break;

		startPos = T.pos + T.length;
		indentCurrent = indentNext;
	}
}

LexerX12::Terminator LexerX12::InitialiseFromISA(IDocument *pAccess)
{
	Sci_Position length = pAccess->Length();
	if (length <= 108)
		return { SCE_X12_BAD, 0 };

	pAccess->GetCharRange(&m_SeparatorElement, 3, 1);
	pAccess->GetCharRange(&m_SeparatorSubElement, 104, 1);

	// Look for GS, as that's the next segment. Anything between 105 and GS is our segment separator.
	Sci_Position posGS;
	char bufGS[3] = { 0 };
	for (posGS = 105; posGS < length - 2; posGS++)
	{
		pAccess->GetCharRange(bufGS, posGS, 2);
		if (bufGS[0] == 'G' && bufGS[1] == 'S')
		{
			m_SeparatorSegment.resize(posGS - 105);
			pAccess->GetCharRange(&m_SeparatorSegment.at(0), 105, posGS - 105);

			// Is some of that CR+LF?
			size_t nPos = m_SeparatorSegment.find_last_not_of("\r\n");
			m_LineFeed = m_SeparatorSegment.substr(nPos + 1);
			m_SeparatorSegment = m_SeparatorSegment.substr(0, nPos + 1);
			break;
		}
	}
	if (m_SeparatorSegment.empty() && m_LineFeed.empty())
	{
		return { SCE_X12_BAD, 105 };
	}

	// Validate we have an element separator, and it's not silly!
	if (m_SeparatorElement == '\0' || m_SeparatorElement == '\n' || m_SeparatorElement == '\r')
		return { SCE_X12_BAD, 3 };

	// Validate we have an element separator, and it's not silly!
	if (m_SeparatorSubElement == '\0' || m_SeparatorSubElement == '\n' || m_SeparatorSubElement == '\r')
		return { SCE_X12_BAD, 103 };
	if (m_SeparatorElement == m_SeparatorSubElement)
		return { SCE_X12_BAD, 104 };
	for (auto& c : m_SeparatorSegment)
	{
		if (m_SeparatorElement == c)
			return { SCE_X12_BAD, 105 };
		if (m_SeparatorSubElement == c)
			return { SCE_X12_BAD, 105 };
	}

	// Check we have element markers at all the right places! ISA element has fixed entries.
	std::vector<Sci_PositionU> ElementMarkers = { 3, 6, 17, 20, 31, 34, 50, 53, 69, 76, 81, 83, 89, 99, 101, 103 };
	for (auto i : ElementMarkers)
	{
		char c;
		pAccess->GetCharRange(&c, i, 1);
		if (c != m_SeparatorElement)
			return { SCE_X12_BAD, i };
	}
	// Check we have no element markers anywhere else!
	for (Sci_PositionU i = 0; i < 105; i++)
	{
		if (std::find(ElementMarkers.begin(), ElementMarkers.end(), i) != ElementMarkers.end())
			continue;

		char c;
		pAccess->GetCharRange(&c, i, 1);
		if (c == m_SeparatorElement)
			return { SCE_X12_BAD, i };
	}

	return { SCE_X12_ENVELOPE };
}

Sci_PositionU LexerX12::FindPreviousSegmentStart(IDocument *pAccess, Sci_Position startPos) const
{
	Sci_PositionU length = pAccess->Length();
	std::string bufTest = m_SeparatorSegment + m_LineFeed; // quick way of making the lengths the same
	std::string bufCompare = bufTest;

	for (; startPos > 0; startPos--)
	{
		if (startPos + bufTest.size() > length)
			continue;

		pAccess->GetCharRange(&bufTest.at(0), startPos, bufTest.size());
		if (bufTest == bufCompare)
		{
			return startPos + bufTest.size();
		}
	}
	// We didn't find a ', so just go with the beginning
	return 0;
}

LexerX12::Terminator LexerX12::DetectSegmentHeader(IDocument *pAccess, Sci_PositionU pos) const
{
	Sci_PositionU Length = pAccess->Length();
	Length -= pos;
	char c, Buf[4] = { 0 }; // max 3 + separator
	for (Sci_PositionU posOffset = 0; posOffset < std::size(Buf) && posOffset < Length; posOffset++)
	{
		pAccess->GetCharRange(&c, pos + posOffset, 1);
		if (c != m_SeparatorElement)
		{
			Buf[posOffset] = c;
			continue;
		}

		// check for special segments, involved in folding start/stop.
		if (memcmp(Buf, "ISA", 3) == 0)
			return { SCE_X12_ENVELOPE, pos + posOffset, 1, +1 };
		if (memcmp(Buf, "IEA", 3) == 0)
			return { SCE_X12_ENVELOPE, pos + posOffset, 1, -1 };
		if (memcmp(Buf, "GS", 2) == 0)
			return { SCE_X12_FUNCTIONGROUP, pos + posOffset, 1, +1 };
		if (memcmp(Buf, "GE", 2) == 0)
			return { SCE_X12_FUNCTIONGROUP, pos + posOffset, 1, -1 };
		if (memcmp(Buf, "ST", 2) == 0)
			return { SCE_X12_TRANSACTIONSET, pos + posOffset, 1, +1 };
		if (memcmp(Buf, "SE", 2) == 0)
			return { SCE_X12_TRANSACTIONSET, pos + posOffset, 1, -1 };
		return { SCE_X12_SEGMENTHEADER, pos + posOffset, 1, 0 };
	}
	return { SCE_X12_BAD, pos, 0, 0 };
}

LexerX12::Terminator LexerX12::FindNextTerminator(IDocument *pAccess, Sci_PositionU pos, bool bJustSegmentTerminator) const
{
	char c;
	Sci_PositionU length = pAccess->Length();
	std::string bufTestSegment = m_SeparatorSegment; // quick way of making the lengths the same
	std::string bufTestLineFeed = m_LineFeed; // quick way of making the lengths the same


	while (pos < (Sci_PositionU)length)
	{
		pAccess->GetCharRange(&c, pos, 1);
		if (pos + m_SeparatorSegment.size() > length)
			bufTestSegment.clear(); // going up - so once we can't get this, we're done with the buffer.
		else if (!bufTestSegment.empty())
			pAccess->GetCharRange(&bufTestSegment.at(0), pos, bufTestSegment.size());
		if (pos + m_LineFeed.size() > length)
			bufTestLineFeed.clear(); // going up - so once we can't get this, we're done with the buffer.
		else if (!bufTestLineFeed.empty())
			pAccess->GetCharRange(&bufTestLineFeed.at(0), pos, bufTestLineFeed.size());

		if (!bJustSegmentTerminator && c == m_SeparatorElement)
			return { SCE_X12_SEP_ELEMENT, pos, 1 };
		else if (!bJustSegmentTerminator && c == m_SeparatorSubElement)
			return { SCE_X12_SEP_SUBELEMENT, pos, 1 };
		else if (!m_SeparatorSegment.empty() && bufTestSegment == m_SeparatorSegment)
		{
			if (m_LineFeed.empty())
				return { SCE_X12_SEGMENTEND, pos, m_SeparatorSegment.size() };
			// is this the end?
			if (pos + m_SeparatorSegment.size() == length)
				return { SCE_X12_SEGMENTEND, pos, m_SeparatorSegment.size() };
			// Check if we're followed by a linefeed.
			if (pos + m_SeparatorSegment.size() + m_LineFeed.size() > length)
				return { SCE_X12_BAD, pos };
			bufTestSegment = m_LineFeed;
			pAccess->GetCharRange(&bufTestSegment.at(0), pos + m_SeparatorSegment.size(), bufTestSegment.size());
			if (bufTestSegment == m_LineFeed)
				return { SCE_X12_SEGMENTEND, pos, m_SeparatorSegment.size() + m_LineFeed.size() };
			break;
		}
		else if (m_SeparatorSegment.empty() && bufTestLineFeed == m_LineFeed)
		{
			return { SCE_X12_SEGMENTEND, pos, m_LineFeed.size() };
		}
		pos++;
	}

	return { SCE_X12_BAD, pos };
}
