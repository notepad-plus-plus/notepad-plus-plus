// Scintilla source code edit control
/** @file LexEscSeq.cxx
 ** Lexer for terminal escape sequences.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdarg>

#include <string>
#include <string_view>
#include <map>
#include <initializer_list>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "InList.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "OptionSet.h"
#include "DefaultLexer.h"

using namespace Scintilla;
using namespace Lexilla;

namespace {

#define CSI "\033["

// Options used for LexerEscSeq
struct OptionsEscSeq {
	bool colourText = false;
};


const char *const emptyWordListDesc[] = {
	nullptr
};


struct OptionSetEscSeq : public OptionSet<OptionsEscSeq> {
	OptionSetEscSeq() {
		DefineProperty("lexer.escseq.colour.text", &OptionsEscSeq::colourText,
			"Set to 1 to colour text following the escape sequences."
		);

		DefineWordListSets(emptyWordListDesc);
	}
};


const LexicalClass lexicalClasses[] = {
	// Lexer escseq SCLEX_ESCSEQ SCE_ESCSEQ_
	0, "SCE_ESCSEQ_DEFAULT", "default", "Default",
	1, "SCE_ESCSEQ_BLACK_DEFAULT", "default", "Black Default",
	2, "SCE_ESCSEQ_RED_DEFAULT", "default", "Red Default",
	3, "SCE_ESCSEQ_GREEN_DEFAULT", "default", "Green Default",
	4, "SCE_ESCSEQ_YELLOW_DEFAULT", "default", "Yellow Default",
	5, "SCE_ESCSEQ_BLUE_DEFAULT", "default", "Blue Default",
	6, "SCE_ESCSEQ_MAGENTA_DEFAULT", "default", "Magenta Default",
	7, "SCE_ESCSEQ_CYAN_DEFAULT", "default", "Cyan Default",
	8, "SCE_ESCSEQ_WHITE_DEFAULT", "default", "White Default",
	9, "SCE_ESCSEQ_DEFAULT_BLACK", "default", "Default Black",
	10, "SCE_ESCSEQ_BLACK_BLACK", "default", "Black Black",
	11, "SCE_ESCSEQ_RED_BLACK", "default", "Red Black",
	12, "SCE_ESCSEQ_GREEN_BLACK", "default", "Green Black",
	13, "SCE_ESCSEQ_YELLOW_BLACK", "default", "Yellow Black",
	14, "SCE_ESCSEQ_BLUE_BLACK", "default", "Blue Black",
	15, "SCE_ESCSEQ_MAGENTA_BLACK", "default", "Magenta Black",
	16, "SCE_ESCSEQ_CYAN_BLACK", "default", "Cyan Black",
	17, "SCE_ESCSEQ_WHITE_BLACK", "default", "White Black",
	18, "SCE_ESCSEQ_DEFAULT_RED", "default", "Default Red",
	19, "SCE_ESCSEQ_BLACK_RED", "default", "Black Red",
	20, "SCE_ESCSEQ_RED_RED", "default", "Red Red",
	21, "SCE_ESCSEQ_GREEN_RED", "default", "Green Red",
	22, "SCE_ESCSEQ_YELLOW_RED", "default", "Yellow Red",
	23, "SCE_ESCSEQ_BLUE_RED", "default", "Blue Red",
	24, "SCE_ESCSEQ_MAGENTA_RED", "default", "Magenta Red",
	25, "SCE_ESCSEQ_CYAN_RED", "default", "Cyan Red",
	26, "SCE_ESCSEQ_WHITE_RED", "default", "White Red",
	27, "SCE_ESCSEQ_DEFAULT_GREEN", "default", "Default Green",
	28, "SCE_ESCSEQ_BLACK_GREEN", "default", "Black Green",
	29, "SCE_ESCSEQ_RED_GREEN", "default", "Red Green",
	30, "SCE_ESCSEQ_GREEN_GREEN", "default", "Green Green",
	31, "", "unused", "",
	32, "", "predefined", "",
	33, "", "predefined", "",
	34, "", "predefined", "",
	35, "", "predefined", "",
	36, "", "predefined", "",
	37, "", "predefined", "",
	38, "", "predefined", "",
	39, "", "predefined", "",
	40, "SCE_ESCSEQ_YELLOW_GREEN", "default", "Yellow Green",
	41, "SCE_ESCSEQ_BLUE_GREEN", "default", "Blue Green",
	42, "SCE_ESCSEQ_MAGENTA_GREEN", "default", "Magenta Green",
	43, "SCE_ESCSEQ_CYAN_GREEN", "default", "Cyan Green",
	44, "SCE_ESCSEQ_WHITE_GREEN", "default", "White Green",
	45, "SCE_ESCSEQ_DEFAULT_YELLOW", "default", "Default Yellow",
	46, "SCE_ESCSEQ_BLACK_YELLOW", "default", "Black Yellow",
	47, "SCE_ESCSEQ_RED_YELLOW", "default", "Red Yellow",
	48, "SCE_ESCSEQ_GREEN_YELLOW", "default", "Green Yellow",
	49, "SCE_ESCSEQ_YELLOW_YELLOW", "default", "Yellow Yellow",
	50, "SCE_ESCSEQ_BLUE_YELLOW", "default", "Blue Yellow",
	51, "SCE_ESCSEQ_MAGENTA_YELLOW", "default", "Magenta Yellow",
	52, "SCE_ESCSEQ_CYAN_YELLOW", "default", "Cyan Yellow",
	53, "SCE_ESCSEQ_WHITE_YELLOW", "default", "White Yellow",
	54, "SCE_ESCSEQ_DEFAULT_BLUE", "default", "Default Blue",
	55, "SCE_ESCSEQ_BLACK_BLUE", "default", "Black Blue",
	56, "SCE_ESCSEQ_RED_BLUE", "default", "Red Blue",
	57, "SCE_ESCSEQ_GREEN_BLUE", "default", "Green Blue",
	58, "SCE_ESCSEQ_YELLOW_BLUE", "default", "Yellow Blue",
	59, "SCE_ESCSEQ_BLUE_BLUE", "default", "Blue Blue",
	60, "SCE_ESCSEQ_MAGENTA_BLUE", "default", "Magenta Blue",
	61, "SCE_ESCSEQ_CYAN_BLUE", "default", "Cyan Blue",
	62, "SCE_ESCSEQ_WHITE_BLUE", "default", "White Blue",
	63, "SCE_ESCSEQ_DEFAULT_MAGENTA", "default", "Default Magenta",
	64, "SCE_ESCSEQ_BLACK_MAGENTA", "default", "Black Magenta",
	65, "SCE_ESCSEQ_RED_MAGENTA", "default", "Red Magenta",
	66, "SCE_ESCSEQ_GREEN_MAGENTA", "default", "Green Magenta",
	67, "SCE_ESCSEQ_YELLOW_MAGENTA", "default", "Yellow Magenta",
	68, "SCE_ESCSEQ_BLUE_MAGENTA", "default", "Blue Magenta",
	69, "SCE_ESCSEQ_MAGENTA_MAGENTA", "default", "Magenta Magenta",
	70, "SCE_ESCSEQ_CYAN_MAGENTA", "default", "Cyan Magenta",
	71, "SCE_ESCSEQ_WHITE_MAGENTA", "default", "White Magenta",
	72, "SCE_ESCSEQ_DEFAULT_CYAN", "default", "Default Cyan",
	73, "SCE_ESCSEQ_BLACK_CYAN", "default", "Black Cyan",
	74, "SCE_ESCSEQ_RED_CYAN", "default", "Red Cyan",
	75, "SCE_ESCSEQ_GREEN_CYAN", "default", "Green Cyan",
	76, "SCE_ESCSEQ_YELLOW_CYAN", "default", "Yellow Cyan",
	77, "SCE_ESCSEQ_BLUE_CYAN", "default", "Blue Cyan",
	78, "SCE_ESCSEQ_MAGENTA_CYAN", "default", "Magenta Cyan",
	79, "SCE_ESCSEQ_CYAN_CYAN", "default", "Cyan Cyan",
	80, "SCE_ESCSEQ_WHITE_CYAN", "default", "White Cyan",
	81, "SCE_ESCSEQ_DEFAULT_WHITE", "default", "Default White",
	82, "SCE_ESCSEQ_BLACK_WHITE", "default", "Black White",
	83, "SCE_ESCSEQ_RED_WHITE", "default", "Red White",
	84, "SCE_ESCSEQ_GREEN_WHITE", "default", "Green White",
	85, "SCE_ESCSEQ_YELLOW_WHITE", "default", "Yellow White",
	86, "SCE_ESCSEQ_BLUE_WHITE", "default", "Blue White",
	87, "SCE_ESCSEQ_MAGENTA_WHITE", "default", "Magenta White",
	88, "SCE_ESCSEQ_CYAN_WHITE", "default", "Cyan White",
	89, "SCE_ESCSEQ_WHITE_WHITE", "default", "White White",
	90, "SCE_ESCSEQ_BOLD_DEFAULT", "default", "Bold Default",
	91, "SCE_ESCSEQ_BOLD_BLACK_DEFAULT", "default", "Bold Black Default",
	92, "SCE_ESCSEQ_BOLD_RED_DEFAULT", "default", "Bold Red Default",
	93, "SCE_ESCSEQ_BOLD_GREEN_DEFAULT", "default", "Bold Green Default",
	94, "SCE_ESCSEQ_BOLD_YELLOW_DEFAULT", "default", "Bold Yellow Default",
	95, "SCE_ESCSEQ_BOLD_BLUE_DEFAULT", "default", "Bold Blue Default",
	96, "SCE_ESCSEQ_BOLD_MAGENTA_DEFAULT", "default", "Bold Magenta Default",
	97, "SCE_ESCSEQ_BOLD_CYAN_DEFAULT", "default", "Bold Cyan Default",
	98, "SCE_ESCSEQ_BOLD_WHITE_DEFAULT", "default", "Bold White Default",
	99, "SCE_ESCSEQ_BOLD_DEFAULT_BLACK", "default", "Bold Default Black",
	100, "SCE_ESCSEQ_BOLD_BLACK_BLACK", "default", "Bold Black Black",
	101, "SCE_ESCSEQ_BOLD_RED_BLACK", "default", "Bold Red Black",
	102, "SCE_ESCSEQ_BOLD_GREEN_BLACK", "default", "Bold Green Black",
	103, "SCE_ESCSEQ_BOLD_YELLOW_BLACK", "default", "Bold Yellow Black",
	104, "SCE_ESCSEQ_BOLD_BLUE_BLACK", "default", "Bold Blue Black",
	105, "SCE_ESCSEQ_BOLD_MAGENTA_BLACK", "default", "Bold Magenta Black",
	106, "SCE_ESCSEQ_BOLD_CYAN_BLACK", "default", "Bold Cyan Black",
	107, "SCE_ESCSEQ_BOLD_WHITE_BLACK", "default", "Bold White Black",
	108, "SCE_ESCSEQ_BOLD_DEFAULT_RED", "default", "Bold Default Red",
	109, "SCE_ESCSEQ_BOLD_BLACK_RED", "default", "Bold Black Red",
	110, "SCE_ESCSEQ_BOLD_RED_RED", "default", "Bold Red Red",
	111, "SCE_ESCSEQ_BOLD_GREEN_RED", "default", "Bold Green Red",
	112, "SCE_ESCSEQ_BOLD_YELLOW_RED", "default", "Bold Yellow Red",
	113, "SCE_ESCSEQ_BOLD_BLUE_RED", "default", "Bold Blue Red",
	114, "SCE_ESCSEQ_BOLD_MAGENTA_RED", "default", "Bold Magenta Red",
	115, "SCE_ESCSEQ_BOLD_CYAN_RED", "default", "Bold Cyan Red",
	116, "SCE_ESCSEQ_BOLD_WHITE_RED", "default", "Bold White Red",
	117, "SCE_ESCSEQ_BOLD_DEFAULT_GREEN", "default", "Bold Default Green",
	118, "SCE_ESCSEQ_BOLD_BLACK_GREEN", "default", "Bold Black Green",
	119, "SCE_ESCSEQ_BOLD_RED_GREEN", "default", "Bold Red Green",
	120, "SCE_ESCSEQ_BOLD_GREEN_GREEN", "default", "Bold Green Green",
	121, "SCE_ESCSEQ_BOLD_YELLOW_GREEN", "default", "Bold Yellow Green",
	122, "SCE_ESCSEQ_BOLD_BLUE_GREEN", "default", "Bold Blue Green",
	123, "SCE_ESCSEQ_BOLD_MAGENTA_GREEN", "default", "Bold Magenta Green",
	124, "SCE_ESCSEQ_BOLD_CYAN_GREEN", "default", "Bold Cyan Green",
	125, "SCE_ESCSEQ_BOLD_WHITE_GREEN", "default", "Bold White Green",
	126, "SCE_ESCSEQ_BOLD_DEFAULT_YELLOW", "default", "Bold Default Yellow",
	127, "SCE_ESCSEQ_BOLD_BLACK_YELLOW", "default", "Bold Black Yellow",
	128, "SCE_ESCSEQ_BOLD_RED_YELLOW", "default", "Bold Red Yellow",
	129, "SCE_ESCSEQ_BOLD_GREEN_YELLOW", "default", "Bold Green Yellow",
	130, "SCE_ESCSEQ_BOLD_YELLOW_YELLOW", "default", "Bold Yellow Yellow",
	131, "SCE_ESCSEQ_BOLD_BLUE_YELLOW", "default", "Bold Blue Yellow",
	132, "SCE_ESCSEQ_BOLD_MAGENTA_YELLOW", "default", "Bold Magenta Yellow",
	133, "SCE_ESCSEQ_BOLD_CYAN_YELLOW", "default", "Bold Cyan Yellow",
	134, "SCE_ESCSEQ_BOLD_WHITE_YELLOW", "default", "Bold White Yellow",
	135, "SCE_ESCSEQ_BOLD_DEFAULT_BLUE", "default", "Bold Default Blue",
	136, "SCE_ESCSEQ_BOLD_BLACK_BLUE", "default", "Bold Black Blue",
	137, "SCE_ESCSEQ_BOLD_RED_BLUE", "default", "Bold Red Blue",
	138, "SCE_ESCSEQ_BOLD_GREEN_BLUE", "default", "Bold Green Blue",
	139, "SCE_ESCSEQ_BOLD_YELLOW_BLUE", "default", "Bold Yellow Blue",
	140, "SCE_ESCSEQ_BOLD_BLUE_BLUE", "default", "Bold Blue Blue",
	141, "SCE_ESCSEQ_BOLD_MAGENTA_BLUE", "default", "Bold Magenta Blue",
	142, "SCE_ESCSEQ_BOLD_CYAN_BLUE", "default", "Bold Cyan Blue",
	143, "SCE_ESCSEQ_BOLD_WHITE_BLUE", "default", "Bold White Blue",
	144, "SCE_ESCSEQ_BOLD_DEFAULT_MAGENTA", "default", "Bold Default Magenta",
	145, "SCE_ESCSEQ_BOLD_BLACK_MAGENTA", "default", "Bold Black Magenta",
	146, "SCE_ESCSEQ_BOLD_RED_MAGENTA", "default", "Bold Red Magenta",
	147, "SCE_ESCSEQ_BOLD_GREEN_MAGENTA", "default", "Bold Green Magenta",
	148, "SCE_ESCSEQ_BOLD_YELLOW_MAGENTA", "default", "Bold Yellow Magenta",
	149, "SCE_ESCSEQ_BOLD_BLUE_MAGENTA", "default", "Bold Blue Magenta",
	150, "SCE_ESCSEQ_BOLD_MAGENTA_MAGENTA", "default", "Bold Magenta Magenta",
	151, "SCE_ESCSEQ_BOLD_CYAN_MAGENTA", "default", "Bold Cyan Magenta",
	152, "SCE_ESCSEQ_BOLD_WHITE_MAGENTA", "default", "Bold White Magenta",
	153, "SCE_ESCSEQ_BOLD_DEFAULT_CYAN", "default", "Bold Default Cyan",
	154, "SCE_ESCSEQ_BOLD_BLACK_CYAN", "default", "Bold Black Cyan",
	155, "SCE_ESCSEQ_BOLD_RED_CYAN", "default", "Bold Red Cyan",
	156, "SCE_ESCSEQ_BOLD_GREEN_CYAN", "default", "Bold Green Cyan",
	157, "SCE_ESCSEQ_BOLD_YELLOW_CYAN", "default", "Bold Yellow Cyan",
	158, "SCE_ESCSEQ_BOLD_BLUE_CYAN", "default", "Bold Blue Cyan",
	159, "SCE_ESCSEQ_BOLD_MAGENTA_CYAN", "default", "Bold Magenta Cyan",
	160, "SCE_ESCSEQ_BOLD_CYAN_CYAN", "default", "Bold Cyan Cyan",
	161, "SCE_ESCSEQ_BOLD_WHITE_CYAN", "default", "Bold White Cyan",
	162, "SCE_ESCSEQ_BOLD_DEFAULT_WHITE", "default", "Bold Default White",
	163, "SCE_ESCSEQ_BOLD_BLACK_WHITE", "default", "Bold Black White",
	164, "SCE_ESCSEQ_BOLD_RED_WHITE", "default", "Bold Red White",
	165, "SCE_ESCSEQ_BOLD_GREEN_WHITE", "default", "Bold Green White",
	166, "SCE_ESCSEQ_BOLD_YELLOW_WHITE", "default", "Bold Yellow White",
	167, "SCE_ESCSEQ_BOLD_BLUE_WHITE", "default", "Bold Blue White",
	168, "SCE_ESCSEQ_BOLD_MAGENTA_WHITE", "default", "Bold Magenta White",
	169, "SCE_ESCSEQ_BOLD_CYAN_WHITE", "default", "Bold Cyan White",
	170, "SCE_ESCSEQ_BOLD_WHITE_WHITE", "default", "Bold White White",
	171, "SCE_ESCSEQ_IDENTIFIER", "default", "Sequence Identifier",
	172, "SCE_ESCSEQ_UNKNOWN", "default", "Sequence Unknown",
};


class LexerEscSeq : public DefaultLexer {
	OptionsEscSeq options;
	OptionSetEscSeq osEscSeq;
public:
	LexerEscSeq() :
		DefaultLexer("escseq", SCLEX_ESCSEQ, lexicalClasses, std::size(lexicalClasses)) {
	}

	const char *SCI_METHOD PropertyNames() override {
		return osEscSeq.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) override {
		return osEscSeq.PropertyType(name);
	}
	const char *SCI_METHOD DescribeProperty(const char *name) override {
		return osEscSeq.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char *SCI_METHOD PropertyGet(const char *key) override {
		return osEscSeq.PropertyGet(key);
	}
	const char *SCI_METHOD DescribeWordListSets() override {
		return osEscSeq.DescribeWordListSets();
	}

	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	static ILexer5 *LexerFactoryEscSeq() {
		return new LexerEscSeq();
	}
};


Sci_Position SCI_METHOD LexerEscSeq::PropertySet(const char *key, const char *val) {
	if (osEscSeq.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}


constexpr bool Is0To9(char ch) noexcept {
	return (ch >= '0') && (ch <= '9');
}


bool AtEOL(Accessor &styler, Sci_Position i) {
	return (styler[i] == '\n') ||
	       ((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}


constexpr bool SequenceEnd(int ch) noexcept {
	return (ch == 0) || ((ch >= '@') && (ch <= '~'));
}


int StyleFromSequence(const char *seq) noexcept {
	int fore = 0;
	int back = 0;
	int bold = 0;

	while (!SequenceEnd(*seq)) {
		if (Is0To9(*seq)) {
			// Get up to 3 digits
			int base = *seq - '0';

			for (int i = 0; i < 2; ++i) {
				if (!Is0To9(seq[1])) {
					break;
				}

				base *= 10;
				base += seq[1] - '0';
				seq++;
			}

			if (base == 0) {
				// Reset to default
				fore = 0;
				back = 0;
				bold = 0;
			} else if (base == 1) {
				// Set style as bright
				bold = 1;
			} else if (base == 22) {
				// Set style as not bright
				bold = 0;
			} else if (base >= 30 && base <= 37) {
				// Set dim fore style
				fore = base - 29;
			} else if (base >= 90 && base <= 97) {
				// Set bright fore style
				fore = base - 89;
				bold = 1;
			} else if (base >= 40 && base <= 47) {
				// Set dim back style
				back = base - 39;
			} else if (base >= 100 && base <= 107) {
				// Set dim back style
				back = base - 99;
			}
		}

		seq++;
	}

	// Set fore style as bright style if bold is set
	if (bold) {
		fore += 81;
	}

	// Set to a single style
	int style = fore + back * 9;

	// Skip the reserved style range between 30 and 40
	if (style >= 31) {
		style += 9;
	}

	return style;
}


void ColouriseEscSeqLine(const std::string &lineBuffer,
                         Sci_PositionU endPos,
                         Accessor &styler,
                         bool colourText) {
	const Sci_PositionU lengthLine = lineBuffer.length();
	const int style = SCE_ESCSEQ_DEFAULT;

	if (strstr(lineBuffer.c_str(), CSI)) {
		const Sci_Position startPos = endPos - lengthLine;
		const char *linePortion = lineBuffer.c_str();
		Sci_Position startPortion = startPos;
		int portionStyle = style;

		while (const char *startSeq = strstr(linePortion, CSI)) {
			if (startSeq > linePortion) {
				styler.ColourTo(startPortion + (startSeq - linePortion), portionStyle);
			}

			const char *endSeq = startSeq + 2;

			while (!SequenceEnd(*endSeq))
				endSeq++;

			const Sci_Position endSeqPosition = startPortion + (endSeq - linePortion) + 1;

			switch (*endSeq) {
			case 0:
				styler.ColourTo(endPos, SCE_ESCSEQ_UNKNOWN);
				return;
			case 'm':  // Colour command
				styler.ColourTo(endSeqPosition, SCE_ESCSEQ_IDENTIFIER);
				portionStyle = colourText ? StyleFromSequence(startSeq + 2) : style;
				break;
			case 'K':  // Erase to end of line -> ignore
				styler.ColourTo(endSeqPosition, SCE_ESCSEQ_IDENTIFIER);
				break;
			default:
				styler.ColourTo(endSeqPosition, SCE_ESCSEQ_UNKNOWN);
				portionStyle = style;
			}

			startPortion = endSeqPosition;
			linePortion = endSeq + 1;
		}

		styler.ColourTo(endPos, portionStyle);
	}
}


void LexerEscSeq::Lex(Sci_PositionU startPos, Sci_Position length, int /* initStyle */, IDocument *pAccess) {
	Accessor styler(pAccess, nullptr);
	std::string lineBuffer;
	styler.StartAt(startPos);
	styler.StartSegment(startPos);

	for (Sci_PositionU i = startPos; i < startPos + length; i++) {
		lineBuffer.push_back(styler[i]);

		// End of line met, colourise it
		if (AtEOL(styler, i)) {
			ColouriseEscSeqLine(lineBuffer, i, styler, options.colourText);
			lineBuffer.clear();
		}
	}

	// Last line does not have ending characters
	if (!lineBuffer.empty()) {
		ColouriseEscSeqLine(lineBuffer, startPos + length - 1, styler, options.colourText);
	}

	styler.Flush();
}


}

extern const LexerModule lmEscSeq(SCLEX_ESCSEQ, LexerEscSeq::LexerFactoryEscSeq, "escseq", emptyWordListDesc);
