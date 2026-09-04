// Scintilla source code edit control
/** @file LexTOML.cxx
 ** Lexer for TOML language.
 **/
// Based on Zufu Liu's Notepad4 TOML lexer
// Modified for Scintilla by Jiri Techet, 2024
// The License.txt file describes the conditions under which this software may be distributed.

#include <cassert>
#include <cstring>

#include <string>
#include <string_view>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

using namespace Lexilla;

namespace {
// Use an unnamed namespace to protect the functions and classes from name conflicts

constexpr bool IsEOLChar(int ch) noexcept {
	return ch == '\r' || ch == '\n';
}

constexpr bool IsIdentifierChar(int ch) noexcept {
	return IsAlphaNumeric(ch) || ch == '_';
}

constexpr bool IsNumberContinue(int chPrev, int ch, int chNext) noexcept {
	return ((ch == '+' || ch == '-') && (chPrev == 'e' || chPrev == 'E'))
		|| (ch == '.' && chNext != '.');
}

constexpr bool IsDecimalNumber(int chPrev, int ch, int chNext) noexcept {
	return IsIdentifierChar(ch) || IsNumberContinue(chPrev, ch, chNext);
}

constexpr bool IsISODateTime(int ch, int chNext) noexcept {
	return ((ch == '+' || ch == '-' || ch == ':' || ch == '.') && IsADigit(chNext))
		|| (ch == ' ' && (chNext == '+' || chNext == '-' || IsADigit(chNext)));
}

struct EscapeSequence {
	int outerState = SCE_TOML_DEFAULT;
	int digitsLeft = 0;

	// highlight any character as escape sequence.
	bool resetEscapeState(int state, int chNext) noexcept {
		if (IsEOLChar(chNext)) {
			return false;
		}
		outerState = state;
		digitsLeft = 1;
		if (chNext == 'x') {
			digitsLeft = 3;
		} else if (chNext == 'u') {
			digitsLeft = 5;
		} else if (chNext == 'U') {
			digitsLeft = 9;
		}
		return true;
	}
	bool atEscapeEnd(int ch) noexcept {
		--digitsLeft;
		return digitsLeft <= 0 || !IsAHeXDigit(ch);
	}
};

constexpr bool IsTripleString(int state) noexcept {
	return state == SCE_TOML_TRIPLE_STRING_SQ || state == SCE_TOML_TRIPLE_STRING_DQ;
}

constexpr bool IsDoubleQuoted(int state) noexcept {
	return state == SCE_TOML_STRING_DQ || state == SCE_TOML_TRIPLE_STRING_DQ;
}

constexpr int GetStringQuote(int state) noexcept {
	return IsDoubleQuoted(state) ? '\"' : '\'';
}

constexpr bool IsTOMLOperator(int ch) noexcept {
	return AnyOf(ch, '[', ']', '{', '}', ',', '=', '.', '+', '-');
}

constexpr bool IsTOMLUnquotedKey(int ch) noexcept {
	return IsIdentifierChar(ch) || ch == '-';
}

constexpr bool IsWhiteSpace(int ch) noexcept {
	return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d));
}

int GetLineNextChar(StyleContext& sc) {
	if (!IsWhiteSpace(sc.ch)) {
		return sc.ch;
	}
	if (static_cast<Sci_Position>(sc.currentPos) + 1 == sc.lineStartNext) {
		return '\0';
	}
	if (!IsWhiteSpace(sc.chNext)) {
		return sc.chNext;
	}
	for (Sci_Position pos = 2; pos < sc.lineStartNext; pos++) {
		const unsigned char chPos = sc.GetRelative(pos);
		if (!IsWhiteSpace(chPos)) {
			return chPos;
		}
	}
	return '\0';
}

bool IsTOMLKey(StyleContext& sc, int braceCount, const WordList *kwList) {
	if (braceCount) {
		const int chNext = GetLineNextChar(sc);
		if (chNext == '=' || chNext == '.' || chNext == '-') {
			sc.ChangeState(SCE_TOML_KEY);
			return true;
		}
	}
	if (sc.state == SCE_TOML_IDENTIFIER) {
		char s[8];
		sc.GetCurrentLowered(s, sizeof(s));
#if defined(__clang__)
		__builtin_assume(kwList != nullptr); // suppress [clang-analyzer-core.CallAndMessage]
#endif
		if (kwList->InList(s)) {
			sc.ChangeState(SCE_TOML_KEYWORD);
		}
	}
	sc.SetState(SCE_TOML_DEFAULT);
	return false;
}

enum class TOMLLineType {
	None = 0,
	Table,
	CommentLine,
};

enum class TOMLKeyState {
	Unquoted = 0,
	Literal, // single-quoted
	Quoted,  // double-quoted
	End,
};

void ColouriseTOMLDoc(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, WordList *keywordLists[], Accessor &styler) {
	int visibleChars = 0;
	int chPrevNonWhite = 0;
	int tableLevel = 0;
	int braceCount = 0;
	TOMLLineType lineType = TOMLLineType::None;
	TOMLKeyState keyState = TOMLKeyState::Unquoted;
	EscapeSequence escSeq;

	if (initStyle == SCE_TOML_STRINGEOL) {
		initStyle = SCE_TOML_DEFAULT;
	}

	StyleContext sc(startPos, lengthDoc, initStyle, styler);
	if (sc.currentLine > 0) {
		const int lineState = styler.GetLineState(sc.currentLine - 1);
		/*
		2: lineType
		8: tableLevel
		8: braceCount
		*/
		braceCount = (lineState >> 10) & 0xff;
	}

	while (sc.More()) {
		switch (sc.state) {
		case SCE_TOML_OPERATOR:
			sc.SetState(SCE_TOML_DEFAULT);
			break;

		case SCE_TOML_NUMBER:
			if (!IsDecimalNumber(sc.chPrev, sc.ch, sc.chNext)) {
				if (IsISODateTime(sc.ch, sc.chNext)) {
					sc.ChangeState(SCE_TOML_DATETIME);
				} else if (IsTOMLKey(sc, braceCount, nullptr)) {
					keyState = TOMLKeyState::Unquoted;
					continue;
				}
			}
			break;

		case SCE_TOML_DATETIME:
			if (!(IsIdentifierChar(sc.ch) || IsISODateTime(sc.ch, sc.chNext))) {
				if (IsTOMLKey(sc, braceCount, nullptr)) {
					keyState = TOMLKeyState::Unquoted;
					continue;
				}
			}
			break;

		case SCE_TOML_IDENTIFIER:
			if (!IsIdentifierChar(sc.ch)) {
				if (IsTOMLKey(sc, braceCount, keywordLists[0])) {
					keyState = TOMLKeyState::Unquoted;
					continue;
				}
			}
			break;

		case SCE_TOML_TABLE:
		case SCE_TOML_KEY:
			if (sc.atLineStart) {
				sc.SetState(SCE_TOML_DEFAULT);
			} else {
				switch (keyState) {
				case TOMLKeyState::Literal:
					if (sc.ch == '\'') {
						keyState = TOMLKeyState::Unquoted;
						sc.Forward();
					}
					break;
				case TOMLKeyState::Quoted:
					if (sc.ch == '\\') {
						sc.Forward();
					} else if (sc.ch == '\"') {
						keyState = TOMLKeyState::Unquoted;
						sc.Forward();
					}
					break;
				default:
					break;
				}
				if (keyState == TOMLKeyState::Unquoted) {
					if (sc.ch == '\'') {
						keyState = TOMLKeyState::Literal;
					} else if (sc.ch == '\"') {
						keyState = TOMLKeyState::Quoted;
					} else if (sc.ch == '.') {
						if (sc.state == SCE_TOML_TABLE) {
							++tableLevel;
						} else {
							chPrevNonWhite = '.';
							sc.SetState(SCE_TOML_OPERATOR);
							sc.ForwardSetState(SCE_TOML_KEY);
							// TODO: skip space after dot
							continue;
						}
					} else if (sc.state == SCE_TOML_TABLE && sc.ch == ']') {
						keyState = TOMLKeyState::End;
						sc.Forward();
						if (sc.ch == ']') {
							sc.Forward();
						}
						const int chNext = GetLineNextChar(sc);
						if (chNext == '#') {
							sc.SetState(SCE_TOML_DEFAULT);
						}
					} else if (sc.state == SCE_TOML_KEY && !IsTOMLUnquotedKey(sc.ch)) {
						const int chNext = GetLineNextChar(sc);
						if (chNext == '=' || (chNext != '.' && chPrevNonWhite != '.')) {
							keyState = TOMLKeyState::End;
							sc.SetState(SCE_TOML_DEFAULT);
						}
					}
				}
			}
			break;

		case SCE_TOML_STRING_SQ:
		case SCE_TOML_STRING_DQ:
		case SCE_TOML_TRIPLE_STRING_SQ:
		case SCE_TOML_TRIPLE_STRING_DQ:
			if (sc.atLineStart && !IsTripleString(sc.state)) {
				sc.SetState(SCE_TOML_DEFAULT);
			} else if (sc.atLineEnd && !IsTripleString(sc.state)) {
				sc.ChangeState(SCE_TOML_STRINGEOL);
			} else if (sc.ch == '\\' && IsDoubleQuoted(sc.state)) {
				if (escSeq.resetEscapeState(sc.state, sc.chNext)) {
					sc.SetState(SCE_TOML_ESCAPECHAR);
					sc.Forward();
				}
			} else if (sc.ch == GetStringQuote(sc.state) &&
					(!IsTripleString(sc.state) || (sc.Match(IsDoubleQuoted(sc.state) ? R"(""")" : R"(''')")))) {
				while (sc.ch == sc.chNext) {
					sc.Forward();
				}
				sc.Forward();
				if (!IsTripleString(sc.state) && IsTOMLKey(sc, braceCount, nullptr)) {
					keyState = TOMLKeyState::Unquoted;
					continue;
				}
				sc.SetState(SCE_TOML_DEFAULT);
			}
			break;

		case SCE_TOML_STRINGEOL:
			if (sc.atLineStart) {
				sc.SetState(SCE_TOML_DEFAULT);
			}
			break;

		case SCE_TOML_ESCAPECHAR:
			if (escSeq.atEscapeEnd(sc.ch)) {
				sc.SetState(escSeq.outerState);
				continue;
			}
			break;

		case SCE_TOML_ERROR:
			if (sc.atLineStart) {
				sc.SetState(SCE_TOML_DEFAULT);
			} else if (sc.ch == '#') {
				sc.SetState(SCE_TOML_COMMENT);
			}
			break;

		case SCE_TOML_COMMENT:
			if (sc.atLineStart) {
				sc.SetState(SCE_TOML_DEFAULT);
			}
			break;
		}

		if (sc.state == SCE_TOML_DEFAULT) {
			if (sc.ch == '#') {
				sc.SetState(SCE_TOML_COMMENT);
				if (visibleChars == 0) {
					lineType = TOMLLineType::CommentLine;
				}
			} else if (visibleChars == 0 && braceCount == 0) {
				if (sc.ch == '[') {
					tableLevel = 0;
					sc.SetState(SCE_TOML_TABLE);
					if (sc.chNext == '[') {
						sc.Forward();
					}
					keyState = TOMLKeyState::Unquoted;
					lineType = TOMLLineType::Table;
				} else if (sc.ch == '\'' || sc.ch == '\"') {
					keyState = (sc.ch == '\'')? TOMLKeyState::Literal : TOMLKeyState::Quoted;
					sc.SetState(SCE_TOML_KEY);
				} else if (IsTOMLUnquotedKey(sc.ch)) {
					keyState = TOMLKeyState::Unquoted;
					sc.SetState(SCE_TOML_KEY);
				} else if (!isspacechar(sc.ch)) {
					// each line must be: key = value
					sc.SetState(SCE_TOML_ERROR);
				}
			} else {
				 if (sc.ch == '\'') {
					if (sc.Match(R"(''')")) {
						sc.SetState(SCE_TOML_TRIPLE_STRING_SQ);
						sc.Forward(2);
					} else {
						sc.SetState(SCE_TOML_STRING_SQ);
					}
				} else if (sc.ch == '"') {
					if (sc.Match(R"(""")")) {
						sc.SetState(SCE_TOML_TRIPLE_STRING_DQ);
						sc.Forward(2);
					} else {
						sc.SetState(SCE_TOML_STRING_DQ);
					}
				} else if (IsADigit(sc.ch)) {
					sc.SetState(SCE_TOML_NUMBER);
				} else if (IsLowerCase(sc.ch)) {
					sc.SetState(SCE_TOML_IDENTIFIER);
				} else if (IsTOMLOperator(sc.ch)) {
					sc.SetState(SCE_TOML_OPERATOR);
					if (sc.ch == '[' || sc.ch == '{') {
						++braceCount;
					} else if (sc.ch == ']' || sc.ch == '}') {
						if (braceCount > 0) {
							--braceCount;
						}
					}
				} else if (braceCount && IsTOMLUnquotedKey(sc.ch)) {
					// Inline Table
					keyState = TOMLKeyState::Unquoted;
					sc.SetState(SCE_TOML_KEY);
				}
			}
		}

		if (!isspacechar(sc.ch)) {
			chPrevNonWhite = sc.ch;
			++visibleChars;
		}
		if (sc.atLineEnd) {
			const int lineState = (tableLevel << 2) | (braceCount << 10) | static_cast<int>(lineType);
			styler.SetLineState(sc.currentLine, lineState);
			lineType = TOMLLineType::None;
			visibleChars = 0;
			chPrevNonWhite = 0;
			tableLevel = 0;
			keyState = TOMLKeyState::Unquoted;
		}
		sc.Forward();
	}

	sc.Complete();
}

constexpr TOMLLineType GetLineType(int lineState) noexcept {
	return static_cast<TOMLLineType>(lineState & 3);
}

constexpr int GetTableLevel(int lineState) noexcept {
	return (lineState >> 2) & 0xff;
}

// code folding based on LexProps
void FoldTOMLDoc(Sci_PositionU startPos, Sci_Position lengthDoc, int /*initStyle*/, WordList *[] /*keywordLists*/, Accessor &styler) {
	const Sci_Position endPos = startPos + lengthDoc;
	const Sci_Position maxLines = styler.GetLine((endPos == styler.Length()) ? endPos : endPos - 1);

	Sci_Position lineCurrent = styler.GetLine(startPos);

	int prevLevel = SC_FOLDLEVELBASE;
	TOMLLineType prevType = TOMLLineType::None;
	TOMLLineType prev2Type = TOMLLineType::None;
	if (lineCurrent > 0) {
		prevLevel = styler.LevelAt(lineCurrent - 1);
		prevType = GetLineType(styler.GetLineState(lineCurrent - 1));
		if (lineCurrent >= 2) {
			prev2Type = GetLineType(styler.GetLineState(lineCurrent - 2));
		}
	}

	bool commentHead = (prevType == TOMLLineType::CommentLine) && (prevLevel & SC_FOLDLEVELHEADERFLAG);
	while (lineCurrent <= maxLines) {
		int nextLevel;
		const int lineState = styler.GetLineState(lineCurrent);
		const TOMLLineType lineType = GetLineType(lineState);

		if (lineType == TOMLLineType::CommentLine) {
			if (prevLevel & SC_FOLDLEVELHEADERFLAG) {
				nextLevel = (prevLevel & SC_FOLDLEVELNUMBERMASK) + 1;
			} else {
				nextLevel = prevLevel;
			}
			commentHead = prevType != TOMLLineType::CommentLine;
			nextLevel |= commentHead ? SC_FOLDLEVELHEADERFLAG : 0;
		} else {
			if (lineType == TOMLLineType::Table) {
				nextLevel = SC_FOLDLEVELBASE + GetTableLevel(lineState);
				if ((prevType == TOMLLineType::CommentLine) && prevLevel <= nextLevel) {
					// comment above nested table
					commentHead = true;
					prevLevel = nextLevel - 1;
				} else if ((prevType == TOMLLineType::Table) && (prevLevel & SC_FOLDLEVELNUMBERMASK) >= nextLevel) {
					commentHead = true; // empty table
				}
				nextLevel |= SC_FOLDLEVELHEADERFLAG;
			} else {
				if (commentHead) {
					nextLevel = prevLevel & SC_FOLDLEVELNUMBERMASK;
				} else if (prevLevel & SC_FOLDLEVELHEADERFLAG) {
					nextLevel = (prevLevel & SC_FOLDLEVELNUMBERMASK) + 1;
				} else if ((prevType == TOMLLineType::CommentLine) && (prev2Type == TOMLLineType::CommentLine)) {
					nextLevel = prevLevel - 1;
				} else {
					nextLevel = prevLevel;
				}
			}

			if (commentHead) {
				commentHead = false;
				styler.SetLevel(lineCurrent - 1, prevLevel & SC_FOLDLEVELNUMBERMASK);
			}
		}

		styler.SetLevel(lineCurrent, nextLevel);
		prevLevel = nextLevel;
		prev2Type = prevType;
		prevType = lineType;
		lineCurrent++;
	}
}

}  // unnamed namespace end

static const char *const tomlWordListDesc[] = {
	"Keywords",
	0
};

extern const LexerModule lmTOML(SCLEX_TOML, ColouriseTOMLDoc, "toml", FoldTOMLDoc, tomlWordListDesc);
