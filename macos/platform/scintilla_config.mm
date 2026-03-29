// scintilla_config.mm — Scintilla editor configuration
// Part of the Notepad++ macOS port modular refactor.

#include "scintilla_config.h"
#include "npp_constants.h"
#include "app_state.h"
#include "scintilla_bridge.h"
#include "keyboard_shortcuts.h"
#include "smart_highlight.h"
#include "change_history.h"

namespace
{
constexpr intptr_t kLineNumberStyle = 33;
constexpr intptr_t kLineNumberMarginLeft = 4;
constexpr int kLineNumberMarginExtraWidth = 12;

int lineNumberDigits(intptr_t lineCount)
{
	int digits = 1;
	while (lineCount >= 10)
	{
		lineCount /= 10;
		++digits;
	}
	return digits;
}

int calculateLineNumberMarginWidth(void* sci)
{
	intptr_t lineCount = ScintillaBridge_sendMessage(sci, SCI_GETLINECOUNT, 0, 0);
	if (lineCount < 1)
		lineCount = 1;

	const int digits = lineNumberDigits(lineCount);
	std::string sample(static_cast<size_t>(digits), '9');
	intptr_t textWidth = ScintillaBridge_sendMessage(sci, SCI_TEXTWIDTH,
		static_cast<uintptr_t>(kLineNumberStyle), reinterpret_cast<intptr_t>(sample.data()));
	if (textWidth <= 0)
	{
		const int fontSize = ctx().fontSize > 0 ? ctx().fontSize : 13;
		textWidth = digits * (fontSize - 2);
	}

	return static_cast<int>(textWidth) + kLineNumberMarginExtraWidth;
}
}

void refreshLineNumberMargin(void* sci)
{
	if (!sci)
		return;

	const bool showLineNumbers = ctx().showLineNumbers;
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINLEFT, 0,
		showLineNumbers ? kLineNumberMarginLeft : 0);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINWIDTHN, 0,
		showLineNumbers ? calculateLineNumberMarginWidth(sci) : 0);
}

void refreshLineNumberMargins()
{
	refreshLineNumberMargin(ctx().scintillaView);
	refreshLineNumberMargin(ctx().scintillaView2);
}

void configureScintilla(void* sci)
{
	if (!sci) return;

	ScintillaBridge_sendMessage(sci, SCI_SETCODEPAGE, 65001, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETWRAPMODE, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTABWIDTH, ctx().tabWidth, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETUSETABS, ctx().useTabs ? 1 : 0, 0);

	ScintillaBridge_sendMessage(sci, SCI_SETMARGINTYPEN, 0, 1);
	refreshLineNumberMargin(sci);

	ScintillaBridge_sendMessage(sci, SCI_SETMARGINTYPEN, 1, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINWIDTHN, 1, 16);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINMASKN, 1, BOOKMARK_MASK);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINSENSITIVEN, 1, 1);

	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, BOOKMARK_MARKER, SC_MARK_BOOKMARK);
	ScintillaBridge_sendMessage(sci, SCI_MARKERSETFORE, BOOKMARK_MARKER, 0xFFFFFF);
	ScintillaBridge_sendMessage(sci, SCI_MARKERSETBACK, BOOKMARK_MARKER, 0xFF8000);

	ScintillaBridge_sendMessage(sci, SCI_SETMARGINTYPEN, 2, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINWIDTHN, 2, 16);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINSENSITIVEN, 2, 1);

	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN,    SC_MARK_BOXMINUS);
	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, SC_MARKNUM_FOLDER,        SC_MARK_BOXPLUS);
	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB,     SC_MARK_VLINE);
	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL,    SC_MARK_LCORNER);
	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND,     SC_MARK_BOXPLUSCONNECTED);
	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED);
	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);

	ScintillaBridge_sendMessage(sci, SCI_SETFOLDFLAGS, 16, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_CLICK, 0);

	// Rectangular / column selection (Opt+drag) and multi-cursor
	ScintillaBridge_sendMessage(sci, SCI_SETRECTANGULARSELECTIONMODIFIER, SCMOD_ALT, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETMULTIPLESELECTION, 1, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETADDITIONALSELECTIONTYPING, 1, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETVIRTUALSPACEOPTIONS, SCVS_RECTANGULARSELECTION, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETADDITIONALCARETSVISIBLE, 1, 0);

	// Multi-cursor visual styling
	ScintillaBridge_sendMessage(sci, SCI_SETADDITIONALCARETFORE, 0x0000CC, 0);  // RGB 0x0000CC = blue
	ScintillaBridge_sendMessage(sci, SCI_SETADDITIONALSELALPHA, 80, 0);  // semi-transparent selections

	// Whitespace / EOL / indent guide visibility
	ScintillaBridge_sendMessage(sci, SCI_SETVIEWWS, ctx().showWhitespace ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETVIEWEOL, ctx().showEol ? 1 : 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETINDENTATIONGUIDES, ctx().showIndentGuides ? SC_IV_LOOKBOTH : SC_IV_NONE, 0);

	ScintillaBridge_sendMessage(sci, SCI_SETCARETLINEVISIBLE, ctx().showCaretLine ? 1 : 0, 0);
	if (ctx().showCaretLine)
		ScintillaBridge_sendMessage(sci, SCI_SETCARETLINEBACK, 0xF0F0F0, 0);

	configureSmartHighlightIndicator(sci, false);

	// Incremental search match highlighting (orange rounded box)
	ScintillaBridge_sendMessage(sci, SCI_INDICSETSTYLE, INDIC_INCREMENTAL_SEARCH, INDIC_ROUNDBOX);
	ScintillaBridge_sendMessage(sci, SCI_INDICSETFORE, INDIC_INCREMENTAL_SEARCH, 0xFF8000); // orange
	ScintillaBridge_sendMessage(sci, SCI_INDICSETALPHA, INDIC_INCREMENTAL_SEARCH, 80);
	ScintillaBridge_sendMessage(sci, SCI_INDICSETOUTLINEALPHA, INDIC_INCREMENTAL_SEARCH, 200);
	ScintillaBridge_sendMessage(sci, SCI_INDICSETUNDER, INDIC_INCREMENTAL_SEARCH, 1);

	configureChangeHistory(sci);

	configureKeyboardShortcuts(sci);
}
