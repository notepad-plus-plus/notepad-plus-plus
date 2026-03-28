// appearance.mm — Dark/light mode, theme switching
// Part of the Notepad++ macOS port modular refactor.

#import <Cocoa/Cocoa.h>
#include "appearance.h"
#include "npp_constants.h"
#include "app_state.h"
#include "language_defs.h"
#include "lexer_styles.h"
#include "scintilla_config.h"
#include "scintilla_bridge.h"
#include "brace_match.h"
#include "smart_highlight.h"
#include "change_history.h"

void applyFoldMarkerColorsToView(void* sci, bool isDark)
{
	if (!sci) return;
	int fgColor = isDark ? 0xD4D4D4 : 0x808080;
	int bgColor = isDark ? 0x2D2D2D : 0xF0F0F0;
	for (int m = SC_MARKNUM_FOLDEREND; m <= SC_MARKNUM_FOLDEROPEN; ++m)
	{
		ScintillaBridge_sendMessage(sci, SCI_MARKERSETFORE, m, fgColor);
		ScintillaBridge_sendMessage(sci, SCI_MARKERSETBACK, m, bgColor);
	}
}

void applyAppearanceToView(void* sci, int langIdx, bool isDark)
{
	if (!sci) return;

	ScintillaBridge_sendMessage(sci, SCI_STYLESETFORE, 32, isDark ? 0xD4D4D4 : 0x000000);
	ScintillaBridge_sendMessage(sci, SCI_STYLESETBACK, 32, isDark ? 0x1E1E1E : 0xFFFFFF);
	ScintillaBridge_sendMessage(sci, SCI_STYLECLEARALL, 0, 0);

	if (langIdx >= 0 && langIdx < g_numLanguages)
	{
		const LexerStyles* ls = findLexerStyles(g_languages[langIdx].lexerName);
		if (ls)
		{
			for (int i = 0; i < ls->numStyles; ++i)
			{
				const auto& s = ls->styles[i];
				ScintillaBridge_sendMessage(sci, SCI_STYLESETFORE, s.styleId,
					isDark ? s.darkFore : s.lightFore);
				if (s.bold)
					ScintillaBridge_sendMessage(sci, SCI_STYLESETBOLD, s.styleId, 1);
				if (s.italic)
					ScintillaBridge_sendMessage(sci, SCI_STYLESETITALIC, s.styleId, 1);
			}
		}
	}

	ScintillaBridge_sendMessage(sci, SCI_SETCARETFORE, isDark ? 0xAEAFAD : 0x000000, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETCARETLINEVISIBLE, ctx().showCaretLine ? 1 : 0, 0);
	if (ctx().showCaretLine)
		ScintillaBridge_sendMessage(sci, SCI_SETCARETLINEBACK, isDark ? 0x2A2A2A : 0xF0F0F0, 0);

	ScintillaBridge_sendMessage(sci, SCI_STYLESETFORE, 33, isDark ? 0x858585 : 0x808080);
	ScintillaBridge_sendMessage(sci, SCI_STYLESETBACK, 33, isDark ? 0x1E1E1E : 0xF0F0F0);

	ScintillaBridge_sendMessage(sci, SCI_MARKERSETBACK, BOOKMARK_MARKER,
		isDark ? 0xFFA050 : 0xFF8000);

	applyFoldMarkerColorsToView(sci, isDark);
	applyChangeHistoryColors(sci, isDark);
	configureBraceStyles(sci, isDark);
	configureSmartHighlightIndicator(sci, isDark);

	// Incremental search indicator colors
	ScintillaBridge_sendMessage(sci, SCI_INDICSETFORE, INDIC_INCREMENTAL_SEARCH,
		isDark ? 0x50C8FF : 0xFF8000);

	refreshLineNumberMargin(sci);
}

void applyAppearance()
{
	if (!ctx().scintillaView) return;

	NSAppearanceName appearanceName = [NSApp.effectiveAppearance
		bestMatchFromAppearancesWithNames:@[NSAppearanceNameAqua, NSAppearanceNameDarkAqua]];
	bool isDark = [appearanceName isEqualToString:NSAppearanceNameDarkAqua];

	int langIdx = 0;
	if (ctx().activeTab >= 0 && ctx().activeTab < static_cast<int>(ctx().documents.size()))
		langIdx = ctx().documents[ctx().activeTab].languageIndex;
	applyAppearanceToView(ctx().scintillaView, langIdx, isDark);

	if (ctx().isSplit && ctx().scintillaView2)
	{
		int langIdx2 = 0;
		if (ctx().activeTab2 >= 0 && ctx().activeTab2 < static_cast<int>(ctx().documents2.size()))
			langIdx2 = ctx().documents2[ctx().activeTab2].languageIndex;
		applyAppearanceToView(ctx().scintillaView2, langIdx2, isDark);
	}
}
