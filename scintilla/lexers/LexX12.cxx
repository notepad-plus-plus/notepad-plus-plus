// Scintilla Lexer for X12
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

#include <vector>
#include <algorithm>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"
#include "LexerModule.h"
#include "DefaultLexer.h"

using namespace Scintilla;

class LexerX12 : public DefaultLexer
{
public:
	LexerX12();
	virtual ~LexerX12() {} // virtual destructor, as we inherit from ILexer

	static ILexer4 *Factory() {
		return new LexerX12;
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
		return NULL;
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

	bool m_bFold;
	char m_chSubElement;
	char m_chElement;
	char m_chSegment[3]; // might be CRLF
};

LexerModule lmX12(SCLEX_X12, LexerX12::Factory, "x12");

///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////

LexerX12::LexerX12()
{
	m_bFold = false;
	m_chSegment[0] = m_chSegment[1] = m_chSegment[2] = m_chElement = m_chSubElement = 0;
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
			T = FindNextTerminator(pAccess, posCurrent);
			if (T.Style == SCE_X12_BAD)
				break;

			int Style = T.Style;
			if (T.Style == SCE_X12_SEGMENTEND && m_chSegment[0] == '\r') // don't style cr/crlf
				Style = SCE_X12_DEFAULT;

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
	if (m_chSegment[0] != '\r' && m_chSegment[0] != '\n') // check for cr,lf,cr+lf.
		return;

	Sci_PositionU posFinish = startPos + length;

	// Look backwards for a segment start or a document beginning
	startPos = FindPreviousSegmentStart(pAccess, startPos);
	Terminator T;

	Sci_PositionU currLine = pAccess->LineFromPosition(startPos);
	int levelCurrentStyle = SC_FOLDLEVELBASE;
	if (currLine > 0)
		levelCurrentStyle = pAccess->GetLevel(currLine - 1); // bottom 12 bits are level
	int indentCurrent = levelCurrentStyle & (SC_FOLDLEVELBASE - 1);

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
		startPos = T.pos + T.length;
		indentCurrent = indentNext;
	}
}

LexerX12::Terminator LexerX12::InitialiseFromISA(IDocument *pAccess)
{
	Sci_Position length = pAccess->Length();
	char c;
	if (length <= 106)
		return { SCE_X12_BAD, 0 };

	pAccess->GetCharRange(&m_chElement, 3, 1);
	pAccess->GetCharRange(&m_chSubElement, 104, 1);
	pAccess->GetCharRange(m_chSegment, 105, 1);
	if (m_chSegment[0] == '\r') // are we CRLF?
	{
		pAccess->GetCharRange(&c, 106, 1);
		if (c == '\n')
			m_chSegment[1] = c;
	}

	// Validate we have an element separator, and it's not silly!
	if (m_chElement == '\0' || m_chElement == '\n' || m_chElement == '\r')
		return { SCE_X12_BAD, 3 };

	// Validate we have an element separator, and it's not silly!
	if (m_chSubElement == '\0' || m_chSubElement == '\n' || m_chSubElement == '\r')
		return { SCE_X12_BAD, 103 };

	if (m_chElement == m_chSubElement)
		return { SCE_X12_BAD, 104 };
	if (m_chElement == m_chSegment[0])
		return { SCE_X12_BAD, 105 };
	if (m_chSubElement == m_chSegment[0])
		return { SCE_X12_BAD, 104 };

	// Check we have element markers at all the right places! ISA element has fixed entries.
	std::vector<Sci_PositionU> ElementMarkers = { 3, 6, 17, 20, 31, 34, 50, 53, 69, 76, 81, 83, 89, 99, 101, 103  };
	for (auto i : ElementMarkers)
	{
		pAccess->GetCharRange(&c, i, 1);
		if (c != m_chElement)
			return { SCE_X12_BAD, i };
	}
	// Check we have no element markers anywhere else!
	for (Sci_PositionU i = 0; i < 105; i++)
	{
		if (std::find(ElementMarkers.begin(), ElementMarkers.end(), i) != ElementMarkers.end())
			continue;

		pAccess->GetCharRange(&c, i, 1);
		if (c == m_chElement)
			return { SCE_X12_BAD, i };
	}

	return { SCE_X12_ENVELOPE };
}

Sci_PositionU LexerX12::FindPreviousSegmentStart(IDocument *pAccess, Sci_Position startPos) const
{
	char c;

	for ( ; startPos > 0; startPos--)
	{
		pAccess->GetCharRange(&c, startPos, 1);
		if (c != m_chSegment[0])
			continue;
		// we've matched one - if this is not crlf we're done.
		if (!m_chSegment[1])
			return startPos + 1;
		pAccess->GetCharRange(&c, startPos+1, 1);
		if (c == m_chSegment[1])
			return startPos + 2;
	}
	// We didn't find a ', so just go with the beginning
	return 0;
}

LexerX12::Terminator LexerX12::DetectSegmentHeader(IDocument *pAccess, Sci_PositionU pos) const
{
	Sci_PositionU posStart = pos;
	Sci_Position Length = pAccess->Length();
	char Buf[6] = { 0 };
	while (pos - posStart < 5 && pos < (Sci_PositionU)Length)
	{
		pAccess->GetCharRange(Buf + pos - posStart, pos, 1);
		if (Buf [pos - posStart] != m_chElement) // more?
		{
			pos++;
			continue;
		}
		if (strcmp(Buf, "ISA*") == 0)
			return { SCE_X12_ENVELOPE, pos, 1, +1 };
		if (strcmp(Buf, "IEA*") == 0)
			return { SCE_X12_ENVELOPE, pos, 1, -1 };
		if (strcmp(Buf, "GS*") == 0)
			return { SCE_X12_FUNCTIONGROUP, pos, 1, +1 };
		if (strcmp(Buf, "GE*") == 0)
			return { SCE_X12_FUNCTIONGROUP, pos, 1, -1 };
		if (strcmp(Buf, "ST*") == 0)
			return { SCE_X12_TRANSACTIONSET, pos, 1, +1 };
		if (strcmp(Buf, "SE*") == 0)
			return { SCE_X12_TRANSACTIONSET, pos, 1, -1 };
		return { SCE_X12_SEGMENTHEADER, pos, 1, 0 };
	}
	return { SCE_X12_BAD, pos, 0, 0 };
}

LexerX12::Terminator LexerX12::FindNextTerminator(IDocument *pAccess, Sci_PositionU pos, bool bJustSegmentTerminator) const
{
	char c;
	Sci_Position Length = pAccess->Length();

	while (pos < (Sci_PositionU)Length)
	{
		pAccess->GetCharRange(&c, pos, 1);
		if (!bJustSegmentTerminator && c == m_chElement)
			return { SCE_X12_SEP_ELEMENT, pos, 1 };
		else if (!bJustSegmentTerminator && c == m_chSubElement)
			return { SCE_X12_SEP_SUBELEMENT, pos, 1 };
		else if (c == m_chSegment[0])
		{
			if (!m_chSegment[1])
				return { SCE_X12_SEGMENTEND, pos, 1 };
			pos++;
			if (pos >= (Sci_PositionU)Length)
				break;
			pAccess->GetCharRange(&c, pos, 1);
			if (c == m_chSegment[1])
				return { SCE_X12_SEGMENTEND, pos-1, 2 };
		}
		pos++;
	}

	return { SCE_X12_BAD, pos };
}
