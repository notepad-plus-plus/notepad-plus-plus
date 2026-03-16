// lexer_styles.mm — Per-lexer style tables, language switching
// Part of the Notepad++ macOS port modular refactor.

#include "lexer_styles.h"
#include "npp_constants.h"
#include "app_state.h"
#include "language_defs.h"
#include "scintilla_bridge.h"
#include "ILexer.h"
#include "Lexilla.h"
#include <cstring>

// Forward declaration
void applyAppearance();

// C/C++ lexer styles (SCE_C_*)
static const StyleDef s_cppStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},
	{2,  0x008000, 0x6A9955, false, false},
	{3,  0x008000, 0x6A9955, false, true},
	{4,  0x008080, 0xB5CEA8, false, false},
	{5,  0xFF0000, 0x569CD6, true,  false},
	{6,  0x800080, 0xCE9178, false, false},
	{7,  0x800080, 0xCE9178, false, false},
	{9,  0x808080, 0xC586C0, false, false},
	{10, 0x000000, 0xD4D4D4, false, false},
	{16, 0x990000, 0x4EC9B0, false, false},
};

static const StyleDef s_objcStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},
	{2,  0x008000, 0x6A9955, false, false},
	{3,  0x008000, 0x6A9955, false, true},
	{4,  0x008080, 0xB5CEA8, false, false},
	{5,  0xFF0000, 0x569CD6, true,  false},
	{6,  0x800080, 0xCE9178, false, false},
	{7,  0x800080, 0xCE9178, false, false},
	{9,  0x808080, 0xC586C0, false, false},
	{10, 0x000000, 0xD4D4D4, false, false},
	{16, 0x990000, 0x4EC9B0, false, false},
};

static const StyleDef s_pythonStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},
	{2,  0x008080, 0xB5CEA8, false, false},
	{3,  0x800080, 0xCE9178, false, false},
	{4,  0x800080, 0xCE9178, false, false},
	{5,  0xFF0000, 0x569CD6, true,  false},
	{6,  0x800080, 0xCE9178, false, false},
	{7,  0x800080, 0xCE9178, false, false},
	{8,  0x990000, 0x4EC9B0, false, false},
	{9,  0x880000, 0xDCDCAA, false, false},
	{10, 0x000000, 0xD4D4D4, false, false},
	{14, 0x990000, 0x4EC9B0, false, false},
	{15, 0xCC6600, 0xDCDCAA, false, true},
};

static const StyleDef s_htmlStyles[] = {
	{1,  0x800000, 0x569CD6, true,  false},
	{2,  0x800000, 0x569CD6, false, false},
	{3,  0xFF0000, 0x9CDCFE, false, false},
	{5,  0x008080, 0xB5CEA8, false, false},
	{6,  0x800080, 0xCE9178, false, false},
	{7,  0x800080, 0xCE9178, false, false},
	{9,  0x008000, 0x6A9955, false, false},
};

static const StyleDef s_cssStyles[] = {
	{1,  0x800000, 0x569CD6, true,  false},
	{2,  0x990000, 0xD7BA7D, false, false},
	{4,  0x990000, 0xD7BA7D, false, false},
	{5,  0x000000, 0xD4D4D4, false, false},
	{7,  0x008080, 0xCE9178, false, false},
	{9,  0x008000, 0x6A9955, false, false},
	{13, 0xFF0000, 0x569CD6, true,  false},
};

static const StyleDef s_jsonStyles[] = {
	{1,  0x008080, 0xB5CEA8, false, false},
	{2,  0x800080, 0xCE9178, false, false},
	{4,  0x990000, 0x9CDCFE, false, false},
	{8,  0xFF0000, 0x569CD6, true,  false},
	{9,  0x008080, 0xB5CEA8, false, false},
	{5,  0x000000, 0xD4D4D4, false, false},
};

static const StyleDef s_sqlStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},
	{2,  0x008000, 0x6A9955, false, false},
	{4,  0x008080, 0xB5CEA8, false, false},
	{5,  0xFF0000, 0x569CD6, true,  false},
	{6,  0x800080, 0xCE9178, false, false},
	{7,  0x000000, 0xD4D4D4, false, false},
	{8,  0x990000, 0x4EC9B0, false, false},
};

static const StyleDef s_bashStyles[] = {
	{2,  0x008000, 0x6A9955, false, false},
	{3,  0x008080, 0xB5CEA8, false, false},
	{4,  0xFF0000, 0x569CD6, true,  false},
	{5,  0x800080, 0xCE9178, false, false},
	{6,  0x800080, 0xCE9178, false, false},
	{7,  0x000000, 0xD4D4D4, false, false},
	{8,  0x990000, 0x9CDCFE, false, false},
	{9,  0xCC6600, 0xDCDCAA, false, false},
};

static const StyleDef s_rustStyles[] = {
	{1,  0x008000, 0x6A9955, false, false},
	{2,  0x008000, 0x6A9955, false, false},
	{3,  0x008000, 0x6A9955, false, true},
	{4,  0x008000, 0x6A9955, false, true},
	{5,  0xFF0000, 0x569CD6, true,  false},
	{6,  0x800080, 0xCE9178, false, false},
	{8,  0x008080, 0xB5CEA8, false, false},
	{10, 0x000000, 0xD4D4D4, false, false},
	{12, 0x800080, 0xCE9178, false, false},
	{14, 0x990000, 0x4EC9B0, false, false},
	{16, 0xCC6600, 0xDCDCAA, false, false},
};

static const StyleDef s_markdownStyles[] = {
	{4,  0x000000, 0xD4D4D4, true,  false},
	{5,  0x000000, 0xD4D4D4, true,  false},
	{6,  0x000000, 0xD4D4D4, false, true},
	{7,  0x000000, 0xD4D4D4, false, true},
	{8,  0x800000, 0x569CD6, true,  false},
	{9,  0x800000, 0x569CD6, true,  false},
	{10, 0x800000, 0x569CD6, true,  false},
	{11, 0x800000, 0x569CD6, true,  false},
	{12, 0x800000, 0x569CD6, true,  false},
	{19, 0x808080, 0xCE9178, false, false},
	{20, 0x808080, 0xCE9178, false, false},
	{21, 0x0000FF, 0x569CD6, false, false},
};

static const StyleDef s_xmlStyles[] = {
	{1,  0x800000, 0x569CD6, true,  false},
	{3,  0xFF0000, 0x9CDCFE, false, false},
	{5,  0x008080, 0xB5CEA8, false, false},
	{6,  0x800080, 0xCE9178, false, false},
	{7,  0x800080, 0xCE9178, false, false},
	{9,  0x008000, 0x6A9955, false, false},
};

static const LexerStyles s_allLexerStyles[] = {
	{"cpp",        s_cppStyles,      sizeof(s_cppStyles) / sizeof(s_cppStyles[0])},
	{"objc",       s_objcStyles,     sizeof(s_objcStyles) / sizeof(s_objcStyles[0])},
	{"python",     s_pythonStyles,   sizeof(s_pythonStyles) / sizeof(s_pythonStyles[0])},
	{"hypertext",  s_htmlStyles,     sizeof(s_htmlStyles) / sizeof(s_htmlStyles[0])},
	{"css",        s_cssStyles,      sizeof(s_cssStyles) / sizeof(s_cssStyles[0])},
	{"json",       s_jsonStyles,     sizeof(s_jsonStyles) / sizeof(s_jsonStyles[0])},
	{"sql",        s_sqlStyles,      sizeof(s_sqlStyles) / sizeof(s_sqlStyles[0])},
	{"bash",       s_bashStyles,     sizeof(s_bashStyles) / sizeof(s_bashStyles[0])},
	{"rust",       s_rustStyles,     sizeof(s_rustStyles) / sizeof(s_rustStyles[0])},
	{"markdown",   s_markdownStyles, sizeof(s_markdownStyles) / sizeof(s_markdownStyles[0])},
	{"xml",        s_xmlStyles,      sizeof(s_xmlStyles) / sizeof(s_xmlStyles[0])},
};

const LexerStyles* findLexerStyles(const char* lexerName)
{
	if (!lexerName) return nullptr;
	for (const auto& ls : s_allLexerStyles)
	{
		if (strcmp(ls.lexerName, lexerName) == 0)
			return &ls;
	}
	return nullptr;
}

void applyLanguageToView(void* sci, int langIndex)
{
	if (!sci) return;
	if (langIndex < 0 || langIndex >= g_numLanguages) return;

	const auto& lang = g_languages[langIndex];

	ILexer5* lexer = CreateLexer(lang.lexerName);
	ScintillaBridge_sendMessage(sci, SCI_SETILEXER, 0, (intptr_t)lexer);

	ScintillaBridge_sendMessage(sci, SCI_SETKEYWORDS, 0, (intptr_t)lang.keywords);
	if (lang.keywords2 && lang.keywords2[0])
		ScintillaBridge_sendMessage(sci, SCI_SETKEYWORDS, 1, (intptr_t)lang.keywords2);

	ScintillaBridge_sendMessage(sci, SCI_SETPROPERTY, (uintptr_t)"fold", (intptr_t)"1");
	ScintillaBridge_sendMessage(sci, SCI_SETPROPERTY, (uintptr_t)"fold.compact", (intptr_t)"0");
	ScintillaBridge_sendMessage(sci, SCI_SETPROPERTY, (uintptr_t)"fold.comment", (intptr_t)"1");
	ScintillaBridge_sendMessage(sci, SCI_SETPROPERTY, (uintptr_t)"fold.preprocessor", (intptr_t)"1");

	ScintillaBridge_sendMessage(sci, SCI_STYLESETFONT, 32, (intptr_t)ctx().fontName.c_str());
	ScintillaBridge_sendMessage(sci, SCI_STYLESETSIZE, 32, ctx().fontSize);

	applyAppearance();
	ScintillaBridge_sendMessage(sci, SCI_COLOURISE, 0, -1);
}

void applyLanguage(int langIndex)
{
	applyLanguageToView(ctx().activeScintillaView(), langIndex);
}
