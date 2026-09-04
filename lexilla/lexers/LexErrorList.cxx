// Scintilla source code edit control
/** @file LexErrorList.cxx
 ** Lexer for error lists. Used for the output pane in SciTE.
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

// Options used for LexerErrorList
struct OptionsErrorList {
	bool valueSeparate = false;
	bool escapeSequences = false;
};

const char *const emptyWordListDesc[] = {
	nullptr
};

struct OptionSetErrorList : public OptionSet<OptionsErrorList> {
	OptionSetErrorList() {
		DefineProperty("lexer.errorlist.value.separate", &OptionsErrorList::valueSeparate,
			"For lines in the output pane that are matches from Find in Files or GCC-style"
			"diagnostics, style the path and line number separately from the rest of the"
			"line with style 21 used for the rest of the line."
			"This allows matched text to be more easily distinguished from its location."
		);

		DefineProperty("lexer.errorlist.escape.sequences", &OptionsErrorList::escapeSequences,
			"Set to 1 to interpret escape sequences."
		);
	}
};

const LexicalClass lexicalClasses[] = {
	// Lexer errorlist SCLEX_ERRORLIST SCE_ERR_
	0, "SCE_ERR_DEFAULT", "diagnostic", "Text",
	1, "SCE_ERR_PYTHON", "diagnostic", "Python Error",
	2, "SCE_ERR_GCC", "diagnostic", "GCC Error",
	3, "SCE_ERR_MS", "diagnostic", "Microsoft Error",
	4, "SCE_ERR_CMD", "default", "Command or return status",
	5, "SCE_ERR_BORLAND", "diagnostic", "Borland error and warning messages",
	6, "SCE_ERR_PERL", "diagnostic", "Perl error and warning messages",
	7, "SCE_ERR_NET", "diagnostic", ".NET tracebacks",
	8, "SCE_ERR_LUA", "diagnostic", "Lua error and warning messages",
	9, "SCE_ERR_CTAG", "diagnostic", "ctags",
	10, "SCE_ERR_DIFF_CHANGED", "default", "Diff changed !",
	11, "SCE_ERR_DIFF_ADDITION", "default", "Diff addition +",
	12, "SCE_ERR_DIFF_DELETION", "default", "Diff deletion -",
	13, "SCE_ERR_DIFF_MESSAGE", "default", "Diff message ---",
	14, "SCE_ERR_PHP", "diagnostic", "PHP error",
	15, "SCE_ERR_ELF", "diagnostic", "Essential Lahey Fortran 90 error",
	16, "SCE_ERR_IFC", "diagnostic", "Intel Fortran Compiler error",
	17, "SCE_ERR_IFORT", "diagnostic", "Intel Fortran Compiler v8.0 error/warning",
	18, "SCE_ERR_ABSF", "diagnostic", "Absoft Pro Fortran 90/95 v8.2 error or warning",
	19, "SCE_ERR_TIDY", "diagnostic", "HTML Tidy",
	20, "SCE_ERR_JAVA_STACK", "diagnostic", "Java runtime stack trace",
	21, "SCE_ERR_VALUE", "default", "Text matched with find in files and message part of GCC errors",
	22, "SCE_ERR_GCC_INCLUDED_FROM", "diagnostic", "GCC showing include path to following error",
	23, "SCE_ERR_ESCSEQ", "escapesequence", "Escape sequence",
	24, "SCE_ERR_ESCSEQ_UNKNOWN", "escapesequence", "Escape sequence unknown",
	25, "SCE_ERR_GCC_EXCERPT", "diagnostic", "GCC showing excerpt of code with pointer",
	26, "SCE_ERR_BASH", "diagnostic", "Bash diagnostic",
	27, "", "unused", "",
	28, "", "unused", "",
	29, "", "unused", "",
	30, "", "unused", "",
	31, "", "unused", "",
	32, "", "predefined", "",
	33, "", "predefined", "",
	34, "", "predefined", "",
	35, "", "predefined", "",
	36, "", "predefined", "",
	37, "", "predefined", "",
	38, "", "predefined", "",
	39, "", "predefined", "",
	40, "SCE_ERR_ES_BLACK", "default", "Black",
	41, "SCE_ERR_ES_RED", "default", "Red",
	42, "SCE_ERR_ES_GREEN", "default", "Green",
	43, "SCE_ERR_ES_BROWN", "default", "Brown",
	44, "SCE_ERR_ES_BLUE", "default", "Blue",
	45, "SCE_ERR_ES_MAGENTA", "default", "Magenta",
	46, "SCE_ERR_ES_CYAN", "default", "Cyan",
	47, "SCE_ERR_ES_GRAY", "default", "Gray",
	48, "SCE_ERR_ES_DARK_GRAY", "default", "Dark Gray",
	49, "SCE_ERR_ES_BRIGHT_RED", "default", "Bright Red",
	50, "SCE_ERR_ES_BRIGHT_GREEN", "default", "Bright Green",
	51, "SCE_ERR_ES_YELLOW", "default", "Yellow",
	52, "SCE_ERR_ES_BRIGHT_BLUE", "default", "Bright Blue",
	53, "SCE_ERR_ES_BRIGHT_MAGENTA", "default", "Bright Magenta",
	54, "SCE_ERR_ES_BRIGHT_CYAN", "default", "Bright Cyan",
	55, "SCE_ERR_ES_WHITE", "default", "White",
};

class LexerErrorList : public DefaultLexer {
	OptionsErrorList options;
	OptionSetErrorList osErrorList;
public:
	LexerErrorList() :
		DefaultLexer("errorlist", SCLEX_ERRORLIST, lexicalClasses, std::size(lexicalClasses)) {
	}

	const char *SCI_METHOD PropertyNames() override {
		return osErrorList.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) override {
		return osErrorList.PropertyType(name);
	}
	const char *SCI_METHOD DescribeProperty(const char *name) override {
		return osErrorList.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char *SCI_METHOD PropertyGet(const char *key) override {
		return osErrorList.PropertyGet(key);
	}
	const char *SCI_METHOD DescribeWordListSets() override {
		return osErrorList.DescribeWordListSets();
	}

	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	static ILexer5 *LexerFactoryErrorList() {
		return new LexerErrorList();
	}
};

Sci_Position SCI_METHOD LexerErrorList::PropertySet(const char *key, const char *val) {
	if (osErrorList.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

constexpr bool StartsWith(std::string_view haystack, std::string_view needle) noexcept {
	return (needle.length() <= haystack.length()) &&
		(haystack.substr(0, needle.length()) == needle);
}

constexpr bool Contains(std::string_view text, std::string_view a) noexcept {
	const size_t pos = text.find(a);
	return pos != std::string_view::npos;
}

// Does text contain both a and b with b after a
constexpr bool ContainsOrdered(std::string_view text, std::string_view a, std::string_view b) noexcept {
	const size_t posA = text.find(a);
	if (posA == std::string_view::npos) {
		return false;
	}
	const size_t posB = text.find(b, posA + a.length());
	return posB != std::string_view::npos;
}

constexpr bool Is0To9(char ch) noexcept {
	return (ch >= '0') && (ch <= '9');
}

constexpr bool Is1To9(char ch) noexcept {
	return (ch >= '1') && (ch <= '9');
}

bool AtEOL(Accessor &styler, Sci_Position i) {
	return (styler[i] == '\n') ||
	       ((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}

std::string_view LetterPrefix(std::string_view sv) noexcept {
	Sci_PositionU i = 0;
	while (i < sv.length() && IsUpperOrLowerCase(sv[i]))
		i++;
	return sv.substr(0, i);
}

bool IsGccExcerpt(std::string_view sv) noexcept {
	while (!sv.empty()) {
		if ((sv.length() >= 3) && (sv[0] == ' ' && sv[1] == '|' && (sv[2] == ' ' || sv[2] == '+'))) {
			return true;
		}
		if (!(sv[0] == ' ' || sv[0] == '+' || Is0To9(sv[0]))) {
			return false;
		}
		sv.remove_prefix(1);
	}
	return true;
}

const std::string_view bashDiagnosticMark = ": line ";
bool IsBashDiagnostic(std::string_view sv) {
	const size_t mark = sv.find(bashDiagnosticMark);
	if (mark == std::string_view::npos) {
		return false;
	}
	std::string_view rest = sv.substr(mark + bashDiagnosticMark.length());
	if (rest.empty() || !Is0To9(rest.front())) {
		return false;
	}
	while (!rest.empty() && Is0To9(rest.front())) {
		rest.remove_prefix(1);
	}
	return !rest.empty() && (rest.front() == ':');
}


int RecogniseErrorListLine(std::string_view lineBuffer, Sci_Position &startValue) {
	if (lineBuffer.empty())
		return SCE_ERR_DEFAULT;

	switch (lineBuffer.front()) {
	case '>':
		// Command or return status
		return SCE_ERR_CMD;
	case '<':
		// Diff removal.
		return SCE_ERR_DIFF_DELETION;
	case '!':
		return SCE_ERR_DIFF_CHANGED;
	case '+':
		if (StartsWith(lineBuffer, "+++ ")) {
			return SCE_ERR_DIFF_MESSAGE;
		} else {
			return SCE_ERR_DIFF_ADDITION;
		}
	case '-':
		if (StartsWith(lineBuffer, "--- ")) {
			return SCE_ERR_DIFF_MESSAGE;
		} else {
			return SCE_ERR_DIFF_DELETION;
		}
	default:
		break;
	}

	if (StartsWith(lineBuffer, "cf90-")) {
		// Absoft Pro Fortran 90/95 v8.2 error and/or warning message
		return SCE_ERR_ABSF;
	} else if (StartsWith(lineBuffer, "fortcom:")) {
		// Intel Fortran Compiler v8.0 error/warning message
		return SCE_ERR_IFORT;
	} else if (Contains(lineBuffer, "File \"") && Contains(lineBuffer, ", line ")) {
		return SCE_ERR_PYTHON;
	} else if (Contains(lineBuffer, " in ") && Contains(lineBuffer, " on line ")) {
		return SCE_ERR_PHP;
	} else if ((StartsWith(lineBuffer, "Error ") ||
	            StartsWith(lineBuffer, "Warning ")) &&
	           ContainsOrdered(lineBuffer, " at (", ") : ")) {
		// Intel Fortran Compiler error/warning message
		return SCE_ERR_IFC;
	} else if (StartsWith(lineBuffer, "Error ")) {
		// Borland error message
		return SCE_ERR_BORLAND;
	} else if (StartsWith(lineBuffer, "Warning ")) {
		// Borland warning message
		return SCE_ERR_BORLAND;
	} else if (Contains(lineBuffer, "at line ") &&
	           Contains(lineBuffer, "file ")) {
		// Lua 4 error message
		return SCE_ERR_LUA;
	} else if (ContainsOrdered(lineBuffer, " at ", " line ")) {
		// perl error message:
		// <message> at <file> line <line>
		return SCE_ERR_PERL;
	} else if (StartsWith(lineBuffer, "   at ") &&
	           Contains(lineBuffer, ":line ")) {
		// A .NET traceback
		return SCE_ERR_NET;
	} else if (StartsWith(lineBuffer, "Line ") &&
	           Contains(lineBuffer, ", file ")) {
		// Essential Lahey Fortran error message
		return SCE_ERR_ELF;
	} else if (StartsWith(lineBuffer, "line ") &&
	           Contains(lineBuffer, " column ")) {
		// HTML tidy style: line 42 column 1
		return SCE_ERR_TIDY;
	} else if (StartsWith(lineBuffer, "\tat ") &&
	           Contains(lineBuffer, "(") &&
	           Contains(lineBuffer, ".java:")) {
		// Java stack back trace
		return SCE_ERR_JAVA_STACK;
	} else if (StartsWith(lineBuffer, "In file included from ") ||
	           StartsWith(lineBuffer, "                 from ")) {
		// GCC showing include path to following error
		return SCE_ERR_GCC_INCLUDED_FROM;
	} else if (StartsWith(lineBuffer, "NMAKE : fatal error")) {
		// Microsoft nmake fatal error:
		// NMAKE : fatal error <code>: <program> : return code <return>
		return SCE_ERR_MS;
	} else if (Contains(lineBuffer, "warning LNK") ||
		Contains(lineBuffer, "error LNK")) {
		// Microsoft linker warning:
		// {<object> : } (warning|error) LNK9999
		return SCE_ERR_MS;
	} else if (IsBashDiagnostic(lineBuffer)) {
		// Bash diagnostic
		// <filename>: line <line>:<message>
		return SCE_ERR_BASH;
	} else if (IsGccExcerpt(lineBuffer)) {
		// GCC code excerpt and pointer to issue
		//    73 |   GTimeVal last_popdown;
		//       |            ^~~~~~~~~~~~
		return SCE_ERR_GCC_EXCERPT;
	} else {
		// Look for one of the following formats:
		// GCC: <filename>:<line>:<message>
		// Microsoft: <filename>(<line>) :<message>
		// Common: <filename>(<line>): warning|error|note|remark|catastrophic|fatal
		// Common: <filename>(<line>) warning|error|note|remark|catastrophic|fatal
		// Microsoft: <filename>(<line>,<column>)<message>
		// CTags: <identifier>\t<filename>\t<message>
		// Lua 5 traceback: \t<filename>:<line>:<message>
		// Lua 5.1: <exe>: <filename>:<line>:<message>
		const bool initialTab = (lineBuffer[0] == '\t');
		bool initialColonPart = false;
		bool canBeCtags = !initialTab;	// For ctags must have an identifier with no spaces then a tab
		enum { stInitial,
			stGccStart, stGccDigit, stGccColumn, stGcc,
			stMsStart, stMsDigit, stMsBracket, stMsVc, stMsDigitComma, stMsDotNet,
			stCtagsStart, stCtagsFile, stCtagsStartString, stCtagsStringDollar, stCtags,
			stUnrecognized
		} state = stInitial;
		for (Sci_PositionU i = 0; i < lineBuffer.length(); i++) {
			const char ch = lineBuffer[i];
			const char chNext = ((i + 1) < lineBuffer.length()) ? lineBuffer[i + 1] : ' ';
			if (state == stInitial) {
				if (ch == ':') {
					// May be GCC, or might be Lua 5 (Lua traceback same but with tab prefix)
					if ((chNext != '\\') && (chNext != '/') && (chNext != ' ')) {
						// This check is not completely accurate as may be on
						// GTK+ with a file name that includes ':'.
						state = stGccStart;
					} else if (chNext == ' ') { // indicates a Lua 5.1 error message
						initialColonPart = true;
					}
				} else if ((ch == '(') && Is1To9(chNext) && (!initialTab)) {
					// May be Microsoft
					// Check against '0' often removes phone numbers
					state = stMsStart;
				} else if ((ch == '\t') && canBeCtags) {
					// May be CTags
					state = stCtagsStart;
				} else if (ch == ' ') {
					canBeCtags = false;
				}
			} else if (state == stGccStart) {	// <filename>:
				state = ((ch == '-') || Is0To9(ch)) ? stGccDigit : stUnrecognized;
			} else if (state == stGccDigit) {	// <filename>:<line>
				if (ch == ':') {
					state = stGccColumn;	// :9.*: is GCC
					startValue = i + 1;
				} else if (!Is0To9(ch)) {
					state = stUnrecognized;
				}
			} else if (state == stGccColumn) {	// <filename>:<line>:<column>
				if (!Is0To9(ch)) {
					state = stGcc;
					if (ch == ':')
						startValue = i + 1;
					break;
				}
			} else if (state == stMsStart) {	// <filename>(
				state = Is0To9(ch) ? stMsDigit : stUnrecognized;
			} else if (state == stMsDigit) {	// <filename>(<line>
				if (ch == ',') {
					state = stMsDigitComma;
				} else if (ch == ')') {
					state = stMsBracket;
				} else if ((ch != ' ') && !Is0To9(ch)) {
					state = stUnrecognized;
				}
			} else if (state == stMsBracket) {	// <filename>(<line>)
				if ((ch == ' ') && (chNext == ':')) {
					state = stMsVc;
				} else if ((ch == ':' && chNext == ' ') || (ch == ' ')) {
					// Possibly Microsoft or Delphi.
					// Move past above prefix: " " -> 1, ": " -> 2
					// " " is likely a Delphi diagnostic, ": " a Microsoft diagnostic
					const Sci_PositionU numstep = (ch == ' ') ? 1 : 2;
					const std::string_view word = LetterPrefix(lineBuffer.substr(i + numstep));
					if (InListCaseInsensitive(word, {"error", "warning", "fatal", "catastrophic", "note", "remark"})) {
						state = stMsVc;
					} else {
						state = stUnrecognized;
					}
				} else {
					state = stUnrecognized;
				}
			} else if (state == stMsDigitComma) {	// <filename>(<line>,
				if (ch == ')') {
					state = stMsDotNet;
					break;
				} else if ((ch != ' ') && !Is0To9(ch)) {
					state = stUnrecognized;
				}
			} else if (state == stCtagsStart) {
				if (ch == '\t') {
					state = stCtagsFile;
				}
			} else if (state == stCtagsFile) {
				if ((lineBuffer[i - 1] == '\t') &&
				        ((ch == '/' && chNext == '^') || Is0To9(ch))) {
					state = stCtags;
					break;
				} else if ((ch == '/') && (chNext == '^')) {
					state = stCtagsStartString;
				}
			} else if ((state == stCtagsStartString) && ((lineBuffer[i] == '$') && (lineBuffer[i + 1] == '/'))) {
				state = stCtagsStringDollar;
				break;
			}
		}
		if (state == stGcc) {
			return initialColonPart ? SCE_ERR_LUA : SCE_ERR_GCC;
		} else if ((state == stMsVc) || (state == stMsDotNet)) {
			return SCE_ERR_MS;
		} else if ((state == stCtagsStringDollar) || (state == stCtags)) {
			return SCE_ERR_CTAG;
		} else if (initialColonPart && Contains(lineBuffer, ": warning C")) {
			// Microsoft warning without line number
			// <filename>: warning C9999
			return SCE_ERR_MS;
		} else {
			return SCE_ERR_DEFAULT;
		}
	}
}

#define CSI "\033["

constexpr bool SequenceEnd(int ch) noexcept {
	return (ch == 0) || ((ch >= '@') && (ch <= '~'));
}

int StyleFromSequence(const char *seq) noexcept {
	int bold = 0;
	int style = 0;
	while (!SequenceEnd(*seq)) {
		if (Is0To9(*seq)) {
			int base = *seq - '0';
			if (Is0To9(seq[1])) {
				base = base * 10;
				base += seq[1] - '0';
				seq++;
			}
			if (base == 0) {
				// Reset to default.
				style = 0;
				bold = 0;
			} else if (base == 1) {
				// Set style as bright.
				bold = 1;
				if (style == 0) {
					style = 40;
				}
			} else if (base >= 30 && base <= 37) {
				// Set dim style which starts at 40.
				style = base + 10;
			} else if (base >= 90 && base <= 97) {
				// Set bright style which starts at 48.
				style = base - 42;
			}
		}
		seq++;
	}
	// Set dim style as bright style if bold is set.
	if (bold && (style >= 40 && style <= 47)) {
		style += 8;
	}
	// Return the style which if 0 will be SCE_ERR_DEFAULT style.
	return style;
}

void ColouriseErrorListLine(
    const std::string &lineBuffer,
    Sci_PositionU endPos,
    Accessor &styler,
	bool valueSeparate,
	bool escapeSequences) {
	Sci_Position startValue = -1;
	const int style = RecogniseErrorListLine(lineBuffer, startValue);
	if (escapeSequences && Contains(lineBuffer, CSI)) {
		const Sci_Position startPos = endPos - lineBuffer.length();
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
				styler.ColourTo(endPos, SCE_ERR_ESCSEQ_UNKNOWN);
				return;
			case 'm':	// Colour command
				styler.ColourTo(endSeqPosition, SCE_ERR_ESCSEQ);
				portionStyle = StyleFromSequence(startSeq+2);
				break;
			case 'K':	// Erase to end of line -> ignore
				styler.ColourTo(endSeqPosition, SCE_ERR_ESCSEQ);
				break;
			default:
				styler.ColourTo(endSeqPosition, SCE_ERR_ESCSEQ_UNKNOWN);
				portionStyle = style;
			}
			startPortion = endSeqPosition;
			linePortion = endSeq + 1;
		}
		styler.ColourTo(endPos, portionStyle);
	} else {
		if (valueSeparate && (startValue >= 0)) {
			styler.ColourTo(endPos - (lineBuffer.length() - startValue), style);
			styler.ColourTo(endPos, SCE_ERR_VALUE);
		} else {
			styler.ColourTo(endPos, style);
		}
	}
}

void LexerErrorList::Lex(Sci_PositionU startPos, Sci_Position length, int /* initStyle */, IDocument *pAccess) {
	Accessor styler(pAccess, nullptr);
	std::string lineBuffer;
	styler.StartAt(startPos);
	styler.StartSegment(startPos);

	for (Sci_PositionU i = startPos; i < startPos + length; i++) {
		lineBuffer.push_back(styler[i]);
		if (AtEOL(styler, i)) {
			// End of line met, colourise it
			ColouriseErrorListLine(lineBuffer, i, styler, options.valueSeparate, options.escapeSequences);
			lineBuffer.clear();
		}
	}
	if (!lineBuffer.empty()) {	// Last line does not have ending characters
		ColouriseErrorListLine(lineBuffer, startPos + length - 1, styler, options.valueSeparate, options.escapeSequences);
	}

	styler.Flush();
}

}

extern const LexerModule lmErrorList(SCLEX_ERRORLIST, LexerErrorList::LexerFactoryErrorList, "errorlist", emptyWordListDesc);
