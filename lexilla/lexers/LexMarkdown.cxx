/******************************************************************
 *  LexMarkdown.cxx
 *
 *  A simple Markdown lexer for scintilla.
 *
 *  Includes highlighting for some extra features from the
 *  Pandoc implementation; strikeout, using '#.' as a default
 *  ordered list item marker, and delimited code blocks.
 *
 *  Limitations:
 *
 *  Standard indented code blocks are not highlighted at all,
 *  as it would conflict with other indentation schemes. Use
 *  delimited code blocks for blanket highlighting of an
 *  entire code block.  Embedded HTML is not highlighted either.
 *  Blanket HTML highlighting has issues, because some Markdown
 *  implementations allow Markdown markup inside of the HTML. Also,
 *  there is a following blank line issue that can't be ignored,
 *  explained in the next paragraph. Embedded HTML and code
 *  blocks would be better supported with language specific
 *  highlighting.
 *
 *  The highlighting aims to accurately reflect correct syntax,
 *  but a few restrictions are relaxed. Delimited code blocks are
 *  highlighted, even if the line following the code block is not blank.
 *  Requiring a blank line after a block, breaks the highlighting
 *  in certain cases, because of the way Scintilla ends up calling
 *  the lexer.
 *
 *  Written by Jon Strait - jstrait@moonloop.net
 *
 *  The License.txt file describes the conditions under which this
 *  software may be distributed.
 *
 *****************************************************************/

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdarg>

#include <string>
#include <string_view>
#include <map>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

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

constexpr bool IsNewline(const int ch) {
    // sc.GetRelative(i) returns '\0' if out of range
    return (ch == '\n' || ch == '\r' || ch == '\0');
}

// True if can follow ch down to the end with possibly trailing whitespace
// Does not set the state SCE_MARKDOWN_LINE_BEGIN as to allow further processing
bool FollowToLineEnd(const int ch, const int state, const Sci_PositionU endPos, StyleContext &sc) {
    Sci_Position i = 0;
    while (sc.GetRelative(++i) == ch)
        ;
    // Skip over whitespace
    while (IsASpaceOrTab(sc.GetRelative(i)) && sc.currentPos + i < endPos)
        ++i;
    if (IsNewline(sc.GetRelative(i)) || sc.currentPos + i == endPos) {
        sc.SetState(state);
        sc.Forward(i);
        return true;
    }
    return false;
}

// Set the state on text section from current to length characters,
// then set the rest until the newline to default, except for any characters matching token
void SetStateAndZoom(const int state, const Sci_Position length, const int token, StyleContext &sc) {
    sc.SetState(state);
    sc.Forward(length);
    sc.SetState(SCE_MARKDOWN_DEFAULT);
    sc.Forward();
    bool started = false;
    while (sc.More() && !IsNewline(sc.ch)) {
        if (sc.ch == token && !started) {
            sc.SetState(state);
            started = true;
        }
        else if (sc.ch != token) {
            sc.SetState(SCE_MARKDOWN_DEFAULT);
            started = false;
        }
        sc.Forward();
    }
    sc.SetState(SCE_MARKDOWN_LINE_BEGIN);
}

// Does the previous line have more than spaces and tabs?
bool HasPrevLineContent(StyleContext &sc) {
    Sci_Position i = 0;
    const Sci_Position currentPos = sc.currentPos;
    // Go back to the previous newline
    while ((--i + currentPos) >= 0 && !IsNewline(sc.GetRelative(i)))
        ;
    while ((--i + currentPos) >= 0) {
        const int ch = sc.GetRelative(i);
        if (ch == '\n')
            break;
        if (!AnyOf(ch, '\r', ' ', '\t'))
            return true;
    }
    return false;
}

bool AtTermStart(const StyleContext &sc) noexcept {
    return sc.currentPos == 0 || sc.chPrev == 0 || isspacechar(sc.chPrev);
}

bool IsCompleteStyleRegion(StyleContext &sc, const char *token) {
    bool found = false;
    const size_t start = strlen(token);
    Sci_Position i = static_cast<Sci_Position>(start);
    while (!IsNewline(sc.GetRelative(i))) {
        // make sure an empty pair of single-char tokens doesn't match
        // with a longer token: {*}{*} != {**}
        if (sc.GetRelative(i) == *token && sc.GetRelative(i - 1) != *token) {
            found = start > 1U ? sc.GetRelative(i + 1) == token[1] : true;
            break;
        }
        i++;
    }
    return AtTermStart(sc) && found;
}

bool IsValidHrule(const Sci_PositionU endPos, StyleContext &sc) {
    int count = 1;
    Sci_Position i = 0;
    for (;;) {
        ++i;
        const int c = sc.GetRelative(i);
        if (c == sc.ch) {
            ++count;
            // hit a terminating character
        } else if (!IsASpaceOrTab(c) || sc.currentPos + i == endPos) {
            // Are we a valid HRULE
            if ((IsNewline(c) || sc.currentPos + i == endPos) &&
                    count >= 3 && !HasPrevLineContent(sc)) {
                sc.SetState(SCE_MARKDOWN_HRULE);
                sc.Forward(i);
                sc.SetState(SCE_MARKDOWN_LINE_BEGIN);
                return true;
            }
            sc.SetState(SCE_MARKDOWN_DEFAULT);
            return false;
        }
    }
}

enum class FrontMatter { None, YAML, TOML, JSON };

constexpr std::string_view markYAML = "---";
constexpr std::string_view markTOML = "+++";
constexpr std::string_view markJSON = ";;;";

constexpr FrontMatter FrontMatterFromString(std::string_view value) {
    if (value == markYAML) {
        return FrontMatter::YAML;
    }
    if (value == markTOML) {
        return FrontMatter::TOML;
    }
    if (value == markJSON) {
        return FrontMatter::JSON;
    }
    return FrontMatter::None;
}

FrontMatter DetectFrontMatter(const Accessor &styler) {
    return FrontMatterFromString(styler.GetRange(0, 3));
}

constexpr std::string_view header5 = "#####";
constexpr std::string_view header6 = "######";

// Options used for LexerMarkdown
struct OptionsMarkdown {
    bool headerEOLFill = false;
};

struct OptionSetMarkdown : public OptionSet<OptionsMarkdown> {
    OptionSetMarkdown() {
        DefineProperty("lexer.markdown.header.eolfill", &OptionsMarkdown::headerEOLFill,
            "Set to 1 to highlight all ATX header text.");
    }
};

// Using "default" for tags as have not defined tags for text roles.

const LexicalClass lexicalClasses[] = {
    // Lexer markdown SCLEX_MARKDOWN SCE_MARKDOWN_
    0, "SCE_MARKDOWN_DEFAULT", "default", "Regular text",
    1, "SCE_MARKDOWN_LINE_BEGIN", "default", "Special",
    2, "SCE_MARKDOWN_STRONG1", "default", "Strong emphasis (bold)",
    3, "SCE_MARKDOWN_STRONG2", "default", "Strong emphasis (bold)",
    4, "SCE_MARKDOWN_EM1", "default", "Emphasis (italic)",
    5, "SCE_MARKDOWN_EM2", "default", "Emphasis (italic)",
    6, "SCE_MARKDOWN_HEADER1", "default", "Level-one header",
    7, "SCE_MARKDOWN_HEADER2", "default", "Level-two header",
    8, "SCE_MARKDOWN_HEADER3", "default", "Level-three header",
    9, "SCE_MARKDOWN_HEADER4", "default", "Level-four header",
    10, "SCE_MARKDOWN_HEADER5", "default", "Level-five header",
    11, "SCE_MARKDOWN_HEADER6", "default", "Level-six header",
    12, "SCE_MARKDOWN_PRECHAR", "default", "Prechar (up to three indent spaces)",
    13, "SCE_MARKDOWN_ULIST_ITEM", "default", "Unordered list item",
    14, "SCE_MARKDOWN_OLIST_ITEM", "default", "Ordered list item",
    15, "SCE_MARKDOWN_BLOCKQUOTE", "default", "Block quote",
    16, "SCE_MARKDOWN_STRIKEOUT", "default", "Strikeout",
    17, "SCE_MARKDOWN_HRULE", "default", "Horizontal rule",
    18, "SCE_MARKDOWN_LINK", "default", "Link or image",
    19, "SCE_MARKDOWN_CODE", "default", "Inline code",
    20, "SCE_MARKDOWN_CODE2", "default", "Inline code (quotes code containing a single backtick)",
    21, "SCE_MARKDOWN_CODEBK", "default", "Code block",
    22, "SCE_MARKDOWN_FRONT_MARK", "default", "Front matter marker",
    23, "SCE_MARKDOWN_FRONT", "default", "Front matter",
    24, "SCE_MARKDOWN_FRONT_KEY", "default", "Front matter key",
};

class LexerMarkdown : public DefaultLexer {
    OptionsMarkdown options;
    OptionSetMarkdown osMarkdown;
    FrontMatter frontMatter = FrontMatter::None;
public:
    LexerMarkdown() :
        DefaultLexer("markdown", SCLEX_MARKDOWN, lexicalClasses, std::size(lexicalClasses)) {
        SetOptionSet(&osMarkdown);
    }
    // Deleted so LexerMarkdown objects can not be copied.
    LexerMarkdown(const LexerMarkdown &) = delete;
    LexerMarkdown(LexerMarkdown &&) = delete;
    void operator=(const LexerMarkdown &) = delete;
    void operator=(LexerMarkdown &&) = delete;
    ~LexerMarkdown() override = default;

    Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;

    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

    static ILexer5 *LexerFactoryMarkdown() {
        return new LexerMarkdown();
    }
};

Sci_Position SCI_METHOD LexerMarkdown::PropertySet(const char *key, const char *val) {
    if (osMarkdown.PropertySet(&options, key, val)) {
        return 0;
    }
    return -1;
}

void SCI_METHOD LexerMarkdown::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
    Accessor styler(pAccess, nullptr);

    const Sci_PositionU endPos = startPos + length;
    int precharCount = 0;
    bool isLinkNameDetecting = false;
    // Don't advance on a new loop iteration and retry at the same position.
    // Useful in the corner case of having to start at the beginning file position
    // in the default state.
    bool freezeCursor = false;

    const bool headerEOLFill = options.headerEOLFill;

    StyleContext sc(startPos, static_cast<Sci_PositionU>(length), initStyle, styler);

    if (startPos == 0) {
        frontMatter = DetectFrontMatter(styler);
        if (frontMatter != FrontMatter::None) {
            sc.SetState(SCE_MARKDOWN_FRONT_MARK);
        }
    }

    while (sc.More()) {
        // Skip past escaped characters
        if (sc.ch == '\\') {
            sc.Forward();
            continue;
        }

        // A blockquotes resets the line semantics
        if (sc.state == SCE_MARKDOWN_BLOCKQUOTE)
            sc.SetState(SCE_MARKDOWN_LINE_BEGIN);

        // Conditional state-based actions
        switch (sc.state) {
        case SCE_MARKDOWN_CODE2:
            if (sc.Match("``")) {
                const int closingSpan = (sc.GetRelative(2) == '`') ? 3 : 2;
                sc.Forward(closingSpan);
                sc.SetState(SCE_MARKDOWN_DEFAULT);
            }
            break;

        case SCE_MARKDOWN_CODE:
            if (sc.ch == '`' && sc.chPrev != ' ')
                sc.ForwardSetState(SCE_MARKDOWN_DEFAULT);
            break;

        case SCE_MARKDOWN_FRONT_MARK:
            if (sc.atLineStart && (sc.currentLine > 0)) {
                if (sc.currentLine > 1) {
                    sc.SetState(SCE_MARKDOWN_DEFAULT);
                } else if (sc.ch == '{') {
                    sc.SetState(SCE_MARKDOWN_FRONT);
                } else {
                    sc.SetState(SCE_MARKDOWN_FRONT_KEY);
                }
            }
            break;

        case SCE_MARKDOWN_FRONT: 
            if (sc.atLineStart) {
                const FrontMatter frontMatterMark = FrontMatterFromString(
                    styler.GetRange(sc.currentPos, sc.currentPos + 3));
                if (frontMatterMark == frontMatter) {
                    sc.SetState(SCE_MARKDOWN_FRONT_MARK);
                } else if (AnyOf(sc.ch, '{', '}')) {
                    sc.SetState(SCE_MARKDOWN_FRONT);
                } else {
                    sc.SetState(SCE_MARKDOWN_FRONT_KEY);
                }
            }
            break;

        case SCE_MARKDOWN_FRONT_KEY:
            if (sc.atLineEnd) {
                sc.SetState(SCE_MARKDOWN_FRONT);
            } else {
                switch (frontMatter) {
                case FrontMatter::YAML:
                    if (AnyOf(sc.ch, ':', ' ')) {
                        sc.SetState(SCE_MARKDOWN_FRONT);
                    }
                    break;
                case FrontMatter::TOML:
                    if (AnyOf(sc.ch, '=', ' ')) {
                        sc.SetState(SCE_MARKDOWN_FRONT);
                    }
                    break;
                case FrontMatter::JSON:
                    if (AnyOf(sc.ch, ':', '{')) {
                        sc.SetState(SCE_MARKDOWN_FRONT);
                    }
                    break;
                default:
                    break;
                }
            }
            break;

            // Strong
        case SCE_MARKDOWN_STRONG1:
            if ((sc.Match("**") && sc.chPrev != ' ') || IsNewline(sc.GetRelative(2))) {
                sc.Forward(2);
                sc.SetState(SCE_MARKDOWN_DEFAULT);
            }
            break;
        case SCE_MARKDOWN_STRONG2:
            if ((sc.Match("__") && sc.chPrev != ' ') || IsNewline(sc.GetRelative(2))) {
                sc.Forward(2);
                sc.SetState(SCE_MARKDOWN_DEFAULT);
            }
            break;

            // Emphasis
        case SCE_MARKDOWN_EM1:
            if ((sc.ch == '*' && sc.chPrev != ' ') || IsNewline(sc.chNext))
                sc.ForwardSetState(SCE_MARKDOWN_DEFAULT);
            break;
        case SCE_MARKDOWN_EM2:
            if ((sc.ch == '_' && sc.chPrev != ' ') || IsNewline(sc.chNext))
                sc.ForwardSetState(SCE_MARKDOWN_DEFAULT);
            break;

        case SCE_MARKDOWN_CODEBK:
            if (sc.atLineStart && sc.Match("~~~")) {
                Sci_Position i = 1;
                while (!IsNewline(sc.GetRelative(i)) && sc.currentPos + i < endPos)
                    i++;
                sc.Forward(i);
                sc.SetState(SCE_MARKDOWN_DEFAULT);
            }
            break;

        case SCE_MARKDOWN_STRIKEOUT:
            if ((sc.Match("~~") && sc.chPrev != ' ') || IsNewline(sc.GetRelative(2))) {
                sc.Forward(2);
                sc.SetState(SCE_MARKDOWN_DEFAULT);
            }
            break;

        case SCE_MARKDOWN_LINE_BEGIN:
            // Header
            if (sc.Match("######")) {
                if (headerEOLFill)
                    sc.SetState(SCE_MARKDOWN_HEADER6);
                else
                    SetStateAndZoom(SCE_MARKDOWN_HEADER6, header6.length(), '#', sc);
            } else if (sc.Match("#####")) {
                if (headerEOLFill)
                    sc.SetState(SCE_MARKDOWN_HEADER5);
                else
                    SetStateAndZoom(SCE_MARKDOWN_HEADER5, header5.length(), '#', sc);
            } else if (sc.Match("####")) {
                if (headerEOLFill)
                    sc.SetState(SCE_MARKDOWN_HEADER4);
                else
                    SetStateAndZoom(SCE_MARKDOWN_HEADER4, 4, '#', sc);
            } else if (sc.Match("###")) {
                if (headerEOLFill)
                    sc.SetState(SCE_MARKDOWN_HEADER3);
                else
                    SetStateAndZoom(SCE_MARKDOWN_HEADER3, 3, '#', sc);
            } else if (sc.Match("##")) {
                if (headerEOLFill)
                    sc.SetState(SCE_MARKDOWN_HEADER2);
                else
                    SetStateAndZoom(SCE_MARKDOWN_HEADER2, 2, '#', sc);
            } else if (sc.Match("#")) {
                // Catch the special case of an unordered list
                if (sc.chNext == '.' && IsASpaceOrTab(sc.GetRelative(2))) {
                    precharCount = 0;
                    sc.SetState(SCE_MARKDOWN_PRECHAR);
                } else if (headerEOLFill) {
                    sc.SetState(SCE_MARKDOWN_HEADER1);
                } else {
                    SetStateAndZoom(SCE_MARKDOWN_HEADER1, 1, '#', sc);
                }
            }
            // Code block
            else if (sc.Match("~~~")) {
                if (!HasPrevLineContent(sc))
                    sc.SetState(SCE_MARKDOWN_CODEBK);
                else
                    sc.SetState(SCE_MARKDOWN_DEFAULT);
            } else if (sc.ch == '=') {
                if (HasPrevLineContent(sc) && FollowToLineEnd('=', SCE_MARKDOWN_HEADER1, endPos, sc)) {
                    if (!headerEOLFill)
                        sc.SetState(SCE_MARKDOWN_LINE_BEGIN);
                } else {
                    sc.SetState(SCE_MARKDOWN_DEFAULT);
                }
            } else if (sc.ch == '-') {
                if (HasPrevLineContent(sc) && FollowToLineEnd('-', SCE_MARKDOWN_HEADER2, endPos, sc)) {
                    if (!headerEOLFill)
                        sc.SetState(SCE_MARKDOWN_LINE_BEGIN);
                } else {
                    precharCount = 0;
                    sc.SetState(SCE_MARKDOWN_PRECHAR);
                }
            } else if (IsNewline(sc.ch)) {
                sc.SetState(SCE_MARKDOWN_LINE_BEGIN);
            } else {
                precharCount = 0;
                sc.SetState(SCE_MARKDOWN_PRECHAR);
            }
            break;

            // The header lasts until the newline
        case SCE_MARKDOWN_HEADER1:
        case SCE_MARKDOWN_HEADER2:
        case SCE_MARKDOWN_HEADER3:
        case SCE_MARKDOWN_HEADER4:
        case SCE_MARKDOWN_HEADER5:
        case SCE_MARKDOWN_HEADER6:
            if (headerEOLFill) {
                if (sc.atLineStart) {
                    sc.SetState(SCE_MARKDOWN_LINE_BEGIN);
                    freezeCursor = true;
                }
            } else if (IsNewline(sc.ch)) {
                sc.SetState(SCE_MARKDOWN_LINE_BEGIN);
            }
            break;

        default:
            break;
        }

        // New state only within the initial whitespace
        if (sc.state == SCE_MARKDOWN_PRECHAR) {
            // Blockquote
            if (sc.ch == '>' && precharCount <= 4) {
                sc.SetState(SCE_MARKDOWN_BLOCKQUOTE);
            }
            /*
            // Begin of code block
            else if (!HasPrevLineContent(sc) && (sc.chPrev == '\t' || precharCount >= 4))
                sc.SetState(SCE_MARKDOWN_CODEBK);
            */
            // HRule - Total of three or more hyphens, asterisks, or underscores
            // on a line by themselves
            else if ((sc.ch == '-' || sc.ch == '*' || sc.ch == '_') && IsValidHrule(endPos, sc)) {
                ;
            }
            // Unordered list
            else if ((sc.ch == '-' || sc.ch == '*' || sc.ch == '+') && IsASpaceOrTab(sc.chNext)) {
                sc.SetState(SCE_MARKDOWN_ULIST_ITEM);
                sc.ForwardSetState(SCE_MARKDOWN_DEFAULT);
            }
            // Ordered list
            else if (IsADigit(sc.ch)) {
                Sci_Position digitCount = 0;
                while (IsADigit(sc.GetRelative(++digitCount)))
                    ;
                if (sc.GetRelative(digitCount) == '.' &&
                        IsASpaceOrTab(sc.GetRelative(digitCount + 1))) {
                    sc.SetState(SCE_MARKDOWN_OLIST_ITEM);
                    sc.Forward(digitCount + 1);
                    sc.SetState(SCE_MARKDOWN_DEFAULT);
                } else {
                    // a textual number at the margin should be plain text
                    sc.SetState(SCE_MARKDOWN_DEFAULT);
                }
            }
            // Alternate Ordered list
            else if (sc.ch == '#' && sc.chNext == '.' && IsASpaceOrTab(sc.GetRelative(2))) {
                sc.SetState(SCE_MARKDOWN_OLIST_ITEM);
                sc.Forward(2);
                sc.SetState(SCE_MARKDOWN_DEFAULT);
            } else if (sc.ch != ' ' || precharCount > 2) {
                sc.SetState(SCE_MARKDOWN_DEFAULT);
            } else {
                ++precharCount;
            }
        }

        // Any link
        if (sc.state == SCE_MARKDOWN_LINK) {
            if (sc.Match("](") && sc.GetRelative(-1) != '\\') {
                sc.Forward(2);
                isLinkNameDetecting = true;
            } else if (sc.Match("]:") && sc.GetRelative(-1) != '\\') {
                sc.Forward(2);
                sc.SetState(SCE_MARKDOWN_DEFAULT);
            } else if (!isLinkNameDetecting && sc.ch == ']' && sc.GetRelative(-1) != '\\') {
                sc.Forward();
                sc.SetState(SCE_MARKDOWN_DEFAULT);
            } else if (isLinkNameDetecting && sc.ch == ')' && sc.GetRelative(-1) != '\\') {
                sc.Forward();
                sc.SetState(SCE_MARKDOWN_DEFAULT);
                isLinkNameDetecting = false;
            }
        }

        // New state anywhere in doc
        if (sc.state == SCE_MARKDOWN_DEFAULT) {
            if (sc.atLineStart && sc.ch == '#') {
                sc.SetState(SCE_MARKDOWN_LINE_BEGIN);
                freezeCursor = true;
            }
            // Links and Images
            if (sc.Match("![")) {
                sc.SetState(SCE_MARKDOWN_LINK);
                sc.Forward(1);
            } else if (sc.ch == '[' && sc.GetRelative(-1) != '\\') {
                sc.SetState(SCE_MARKDOWN_LINK);
            }
            // Code - also a special case for alternate inside spacing
            else if (sc.Match("``") && sc.GetRelative(3) != ' ' && AtTermStart(sc)) {
                const int openingSpan = (sc.GetRelative(2) == '`') ? 2 : 1;
                sc.SetState(SCE_MARKDOWN_CODE2);
                sc.Forward(openingSpan);
            } else if (sc.ch == '`' && sc.chNext != ' ' && IsCompleteStyleRegion(sc, "`")) {
                sc.SetState(SCE_MARKDOWN_CODE);
            }
            // Strong
            else if (sc.Match("**") && sc.GetRelative(2) != ' ' && IsCompleteStyleRegion(sc, "**")) {
                sc.SetState(SCE_MARKDOWN_STRONG1);
                sc.Forward();
            } else if (sc.Match("__") && sc.GetRelative(2) != ' ' && IsCompleteStyleRegion(sc, "__")) {
                sc.SetState(SCE_MARKDOWN_STRONG2);
                sc.Forward();
            }
            // Emphasis
            else if (sc.ch == '*' && sc.chNext != ' ' && IsCompleteStyleRegion(sc, "*")) {
                sc.SetState(SCE_MARKDOWN_EM1);
            } else if (sc.ch == '_' && sc.chNext != ' ' && IsCompleteStyleRegion(sc, "_")) {
                sc.SetState(SCE_MARKDOWN_EM2);
            }
            // Strikeout
            else if (sc.Match("~~") && !(AnyOf(sc.GetRelative(2), '~', ' ')) &&
                     IsCompleteStyleRegion(sc, "~~")) {
                sc.SetState(SCE_MARKDOWN_STRIKEOUT);
                sc.Forward();
            }
            // Beginning of line
            else if (IsNewline(sc.ch)) {
                sc.SetState(SCE_MARKDOWN_LINE_BEGIN);
            }
        }
        // Advance if not holding back the cursor for this iteration.
        if (!freezeCursor)
            sc.Forward();
        freezeCursor = false;
    }
    sc.Complete();
}

}

extern const LexerModule lmMarkdown(SCLEX_MARKDOWN, LexerMarkdown::LexerFactoryMarkdown, "markdown");
